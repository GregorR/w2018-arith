#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "arith-lexer.h"
#include "arith-parser.h"
#include "arith-treefix.h"

void printTokens(struct Token **toks);
void printTree(struct Tree *tree, int depth);

double evaluate(struct Tree *tree);

int main()
{
#define BUFSZ 4096
    char buf[BUFSZ];
    while (fgets(buf, BUFSZ, stdin)) {
        struct Token **toks = tokenize(buf);
        struct Tree *tree = parse(toks);
        tree = fix(tree);
        printf("%f\n", evaluate(tree));
    }
    return 0;
}

void printTokens(struct Token **toks)
{
    int i;
    for (i = 0; toks[i]; i++) {
        printf("%s: %s\n", tokenName(toks[i]->tok), toks[i]->text);
    }
}

void printTree(struct Tree *tree, int depth)
{
    int i;
    for (i = 0; i < depth; i++) putchar(' ');
    if (tree->tok)
        printf("%s\n", tree->tok->text);
    else
        printf("%s\n", nonterminalName(tree->symbol.value));
    for (i = 0; i < tree->childrenCt; i++)
        printTree(tree->children[i], depth+2);
}

double evaluate(struct Tree *tree)
{
    double left, right;

    if (!tree->symbol.terminal && tree->childrenCt == 1) {
        return evaluate(tree->children[0]);
    }

    if (tree->symbol.terminal && tree->symbol.value == NUM) {
        return atof(tree->tok->text);

    } else if (!tree->symbol.terminal) {
        switch (tree->symbol.value) {
            case NT_A:
                left = evaluate(tree->children[0]);
                right = evaluate(tree->children[2]);
                if (tree->children[1]->symbol.value == SUB)
                    return left - right;
                else
                    return left + right;

            case NT_M:
                left = evaluate(tree->children[0]);
                right = evaluate(tree->children[2]);
                if (tree->children[1]->symbol.value == DIV)
                    return left / right;
                else
                    return left * right;

            case NT_P:
                return evaluate(tree->children[1]);

            default:
                fprintf(stderr, "Unrecognized nonterminal.\n");
                exit(1);
        }

    } else {
        fprintf(stderr, "I'm confused!\n");
        exit(1);
    }
}
