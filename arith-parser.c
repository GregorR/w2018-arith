#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "arith-parser.h"

/* A single production */
struct Production {
    enum NonTerminal lhs;
    int len;
    struct TerminalOrNonterminal rhs[1];
};

/* A stack element is a symbol and the tree into which that symbol will be expanded */
struct StackEl {
    struct TerminalOrNonterminal symbol;
    struct Tree **target;
};

/* Our predictor table maps nonterminal/token pairs to productions */
static struct Production *predictor[NT_LAST][TOKEN_LAST];
static int predictorInitialized = 0;

/* Create a production of the given LHS and size */
struct Production *newProduction(enum NonTerminal lhs, int len)
{
    struct Production *ret = calloc(1, sizeof(struct Production) + len*sizeof(struct TerminalOrNonterminal));
    if (!ret) {
        perror("calloc");
        exit(1);
    }
    ret->lhs = lhs;
    ret->len = len;
    return ret;
}

/* Create a tree with the given symbol, token and number of children */
struct Tree *newTree(struct TerminalOrNonterminal symbol, struct Token *tok, int childrenCt)
{
    struct Tree *ret = calloc(1, sizeof(struct Tree)+childrenCt*sizeof(struct Tree *));
    ret->symbol = symbol;
    ret->tok = tok;
    ret->childrenCt = childrenCt;
    return ret;
}

/* Initialize the predictor table */
static void predictorInit()
{
    struct Production *p[14];
    int i, ri;

    /* Convenience macros for defining constructions */
#define MK(NUM, NT, SZ) do { \
    i = (NUM); \
    ri = 0; \
    p[i] = newProduction(NT_ ## NT, (SZ)); \
} while (0)
#define TT(TERM) do { \
    p[i]->rhs[ri].terminal = 1; \
    p[i]->rhs[ri].value = TERM; \
    ri++; \
} while (0)
#define TN(NT) p[i]->rhs[ri++].value = NT_ ## NT

    /* First create our productions themselves */

    /* 1. S := A S' */
    MK(0, S, 2);
    TN(A);
    TN(SP);

    /* 2. S' := = S */
    MK(1, SP, 2);
    TT(BECOMES);
    TN(S);

    /* 3. S' := epsilon */
    MK(2, SP, 0);

    /* 4. A := M A' */
    MK(3, A, 2);
    TN(M);
    TN(AP);

    /* 5. A' := add M A' */
    MK(4, AP, 3);
    TT(ADD);
    TN(M);
    TN(AP);

    /* 6. A' := sub M A' */
    MK(5, AP, 3);
    TT(SUB);
    TN(M);
    TN(AP);

    /* 7. A' := epsilon */
    MK(6, AP, 0);

    /* 8. M := P M' */
    MK(7, M, 2);
    TN(P);
    TN(MP);

    /* 9. M' := mul P M' */
    MK(8, MP, 3);
    TT(MUL);
    TN(P);
    TN(MP);

    /* 10. M' := div p M' */
    MK(9, MP, 2);
    TT(DIV);
    TN(P);
    TN(MP);

    /* 11. M' := epsilon */
    MK(10, MP, 0);

    /* 12. P := num */
    MK(11, P, 1);
    TT(NUM);

    /* 13. P := id */
    MK(12, P, 1);
    TT(ID);

    /* 14. P := ( S ) */
    MK(13, P, 3);
    TT(LPAREN);
    TN(S);
    TT(RPAREN);

    /* Get rid of our macros */
#undef MK
#undef TT
#undef TN


    /* Then create the predictor table */
#define pr predictor
    pr[NT_S][NUM] = p[0];
    pr[NT_S][ID] = p[0];
    pr[NT_S][LPAREN] = p[0];

    pr[NT_SP][BECOMES] = p[1];
    pr[NT_SP][RPAREN] = p[2];
    pr[NT_SP][0] = p[2];

    pr[NT_A][NUM] = p[3];
    pr[NT_A][ID] = p[3];
    pr[NT_A][LPAREN] = p[3];

    pr[NT_AP][BECOMES] = p[6];
    pr[NT_AP][ADD] = p[4];
    pr[NT_AP][SUB] = p[5];
    pr[NT_AP][RPAREN] = p[6];
    pr[NT_AP][0] = p[6];

    pr[NT_M][NUM] = p[7];
    pr[NT_M][ID] = p[7];
    pr[NT_M][LPAREN] = p[7];

    pr[NT_MP][BECOMES] = p[10];
    pr[NT_MP][ADD] = p[10];
    pr[NT_MP][SUB] = p[10];
    pr[NT_MP][MUL] = p[8];
    pr[NT_MP][DIV] = p[9];
    pr[NT_MP][RPAREN] = p[10];
    pr[NT_MP][0] = p[10];

    pr[NT_P][NUM] = p[11];
    pr[NT_P][ID] = p[12];
    pr[NT_P][LPAREN] = p[13];
#undef pr

    predictorInitialized = 1;
}

/* Parse an arithmetic expression */
struct Tree *parse(struct Token **toks)
{
    struct Tree *ret = NULL;
    struct StackEl *stack, *stackTop;
    size_t stackSz, stackUsed;
    int tokCt, i, pi;

    /* Initialize the predictor table if necessary */
    if (!predictorInitialized)
        predictorInit();

    /* Start our stack with S */
    stackSz = 16;
    stack = calloc(stackSz, sizeof(struct StackEl));
    if (!stack) {
        perror("calloc");
        exit(1);
    }
    stackUsed = 1;
    stack[0].symbol.value = NT_S;
    stack[0].target = &ret;

    /* Find the length of our input */
    for (tokCt = 0; toks[tokCt]; tokCt++);

    /* Then parse */
    for (i = 0; i <= tokCt; i++) {
        struct Token *cur = toks[i];
        enum TokenType tok = cur?cur->tok:0;
        struct Tree *subTree;

        /* Expand while necessary */
        stackTop = stackUsed?(&stack[stackUsed-1]):NULL;
        while (stackTop && !stackTop->symbol.terminal) {
            struct Production *p = predictor[stackTop->symbol.value][tok];
            if (!p) {
                if (cur)
                    fprintf(stderr, "ERROR: While expanding %s, unexpected token %s at line %d col %d\n",
                        nonterminalName(stackTop->symbol.value), cur->text, cur->line, cur->col);
                else
                    fprintf(stderr, "ERROR: While expanding %s, unexpected end of file\n",
                        nonterminalName(stackTop->symbol.value));
                exit(1);
            }

            /* Pop off the current top of the stack */
            stackUsed--;

            /* Initialize the subtree */
            *stackTop->target = subTree = newTree(stackTop->symbol, NULL, p->len);

            /* Then push on the new */
            for (pi = p->len - 1; pi >= 0; pi--) {
                if (stackUsed >= stackSz) {
                    /* Expand */
                    stackSz *= 2;
                    stack = realloc(stack, stackSz*sizeof(struct StackEl));
                    if (!stack) {
                        perror("realloc");
                        exit(1);
                    }
                }
                stack[stackUsed].symbol = p->rhs[pi];
                stack[stackUsed].target = &subTree->children[pi];
                stackUsed++;
            }

            stackTop = stackUsed?(&stack[stackUsed-1]):NULL;
        }

        /* Check if we're done */
        if (stackUsed == 0) {
            if (cur) {
                fprintf(stderr, "ERROR: Expected end of file, found token %s at line %d col %d\n", cur->text, cur->line, cur->col);
                exit(1);
            }
            break;
        }

        /* We've expanded all the nonterminals, so it must be a terminal */
        if (stackTop->symbol.value != tok) {
            if (cur)
                fprintf(stderr, "ERROR: While matching %s, found token %s at line %d col %d\n",
                    tokenName(stackTop->symbol.value), cur->text, cur->line, cur->col);
            else
                fprintf(stderr, "ERROR: While matching %s, unexpected end of file\n",
                    tokenName(stackTop->symbol.value));
            exit(1);
        }

        /* Make the tree leaf for it */
        *stackTop->target = newTree(stackTop->symbol, cur, 0);

        /* And pop */
        stackUsed--;
    }

    return ret;
}

/* Get a string for a nonterminal */
const char *nonterminalName(enum NonTerminal value)
{
    switch (value) {
        case NT_S:  return "S";
        case NT_SP: return "S'";
        case NT_A:  return "A";
        case NT_AP: return "A'";
        case NT_M:  return "M";
        case NT_MP: return "M'";
        case NT_P:  return "P";
        default:    return "?";
    }
}
