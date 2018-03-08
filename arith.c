#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "arith-lexer.h"
#include "arith-parser.h"
#include "arith-treefix.h"

void printTokens(struct Token **toks);
void printTree(struct Tree *tree, int depth);

int main()
{
#define BUFSZ 4096
    char buf[BUFSZ];
    while (fgets(buf, BUFSZ, stdin)) {
        struct Token **toks = tokenize(buf);
        struct Tree *tree = parse(toks);

        printf("\nTokens:\n");
        printTokens(toks);

        printf("\nTree (pre-fix):\n");
        printTree(tree, 0);
        
        tree = fix(tree);

        printf("\nTree (post-fix):\n");
        printTree(tree, 0);
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
