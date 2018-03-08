#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "arith-lexer.h"

/* A state in our DFA */
struct State {
    enum TokenType tok; /* 0 if not final state */
    struct State *steps[128]; /* ASCII */
};

/* The start state of the DFA */
static struct State *dfaStart = NULL;

/* Create a new DFA state */
static struct State *newState()
{
    struct State *ret = calloc(1, sizeof(struct State));
    if (!ret) {
        perror("calloc");
        exit(1);
    }
    return ret;
}

/* Initialize our DFA. This is not general; it specifically initializes the DFA
 * for arithmetic expressions. */
static void dfaInit()
{
    struct State *dfa[11];
    int i;
    char c;

    /* Create all the states as empty */
    for (i = 0; i < 11; i++)
        dfa[i] = newState();

    /* Now initialize all our transitions */
    for (c = '0'; c <= '9'; c++) {
        dfa[0]->steps[c] = dfa[1];
        dfa[1]->steps[c] = dfa[1];
    }

    for (c = 'a'; c <= 'z'; c++) {
        dfa[0]->steps[c] = dfa[2];
    }

    dfa[0]->steps['='] = dfa[3];
    dfa[0]->steps['+'] = dfa[4];
    dfa[0]->steps['-'] = dfa[5];
    dfa[0]->steps['*'] = dfa[6];
    dfa[0]->steps['/'] = dfa[7];
    dfa[0]->steps['('] = dfa[8];
    dfa[0]->steps[')'] = dfa[9];

    dfa[0]->steps[' '] = dfa[10];
    dfa[0]->steps['\t'] = dfa[10];
    dfa[0]->steps['\r'] = dfa[10];
    dfa[0]->steps['\n'] = dfa[10];

    /* To support end of input */
    dfa[0]->steps[0] = dfa[0];

    /* And our final states */
    dfa[1]->tok = NUM;
    dfa[2]->tok = ID;
    dfa[3]->tok = BECOMES;
    dfa[4]->tok = ADD;
    dfa[5]->tok = SUB;
    dfa[6]->tok = MUL;
    dfa[7]->tok = DIV;
    dfa[8]->tok = LPAREN;
    dfa[9]->tok = RPAREN;
    dfa[10]->tok = WHITESPACE;

    /* dfa[0] is the start state */
    dfaStart = dfa[0];
}

/* Simple maximal-munch tokenizer */
struct Token **tokenize(char *input)
{
    struct Token **ret;
    struct Token *tok;
    size_t retSz, retUsed;
    struct State *curState;
    int startLine, startCol, curLine, curCol;
    size_t inputLen;
    int startPos, i;
    char c;

    /* Make sure the DFA is initialzed */
    if (!dfaStart)
        dfaInit();

    /* Allocate our return */
    retSz = 16;
    retUsed = 0;
    ret = calloc(retSz, sizeof(struct Token *));
    if (!ret) {
        perror("calloc");
        exit(1);
    }

    /* And start tokenizing */
    inputLen = strlen(input);
    curState = dfaStart;
    startPos = 0;
    startLine = startCol = curLine = curCol = 1;
    for (i = 0; i <= inputLen; i++) {
        c = input[i];
        if (!curState->steps[c]) {
            /* Can't take a step */
            if (curState->tok) {
                /* Accept the input */
                if (retUsed >= retSz - 1) {
                    /* Need more space */
                    retSz *= 2;
                    ret = realloc(ret, retSz * sizeof(struct Token *));
                    if (!ret) {
                        perror("realloc");
                        exit(1);
                    }
                }

                /* Ignore whitespace */
                if (curState->tok != WHITESPACE) {
                    /* Make our token */
                    tok = calloc(1, sizeof(struct Token));
                    if (!tok) {
                        perror("calloc");
                        exit(1);
                    }
                    tok->tok = curState->tok;
                    tok->line = startLine;
                    tok->col = startCol;

                    /* Copy in the text */
                    tok->text = calloc(i-startPos+1, 1);
                    if (!tok->text) {
                        perror("malloc");
                        exit(1);
                    }
                    strncpy(tok->text, input + startPos, i-startPos);

                    ret[retUsed++] = tok;
                }

                /* Start at a new state */
                curState = dfaStart;
                startPos = i;
                startLine = curLine;
                startCol = curCol;

            } else {
                /* Not a final state, reject */
                fprintf(stderr, "ERROR: Unexpected input at line %d col %d (%c)\n", curLine, curCol, c);
                exit(1);

            }

        }

        /* We may still be stimied */
        if (!curState->steps[c]) {
            fprintf(stderr, "ERROR: Unexpected input at line %d col %d (%c)\n", curLine, curCol, c);
            exit(1);
        }

        /* Take a step */
        curState = curState->steps[c];
        if (c == '\n') {
            curLine++;
            curCol = 1;
        } else {
            curCol++;
        }
    }

    /* Make sure it's NULL-terminated */
    ret[retUsed++] = NULL;

    return ret;
}

/* Given a token type, return a string name */
const char *tokenName(enum TokenType tok)
{
    switch (tok) {
        case NUM:       return "NUM";
        case ID:        return "ID";
        case BECOMES:   return "BECOMES";
        case ADD:       return "ADD";
        case SUB:       return "SUB";
        case MUL:       return "MUL";
        case DIV:       return "DIV";
        case LPAREN:    return "LPAREN";
        case RPAREN:    return "RPAREN";
        case WHITESPACE:return "WHITESPACE";
        default:        return "???";
    }
}
