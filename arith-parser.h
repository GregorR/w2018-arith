#ifndef ARITH_PARSER_H
#define ARITH_PARSER_H 1

#include "arith-lexer.h"

enum NonTerminal {
    NT_NONE,
    NT_S,
    NT_SP, /* S' */
    NT_A,
    NT_AP, /* A' */
    NT_M,
    NT_MP, /* M' */
    NT_P,
    NT_LAST
};

/* General case of a terminal or nonterminal */
struct TerminalOrNonterminal {
    int terminal;
    /* Either enum TokenType or enum NonTerminal */
    int value;
};

struct Tree {
    struct TerminalOrNonterminal symbol;
    struct Token *tok; /* If terminal */
    int childrenCt;
    struct Tree *children[1];
};

/* Create a tree with the given symbol, token and number of children */
struct Tree *newTree(struct TerminalOrNonterminal symbol, struct Token *tok, int childrenCt);

/* Parse an arithmetic expression */
struct Tree *parse(struct Token **toks);

/* Get a string for a nonterminal */
const char *nonterminalName(enum NonTerminal value);

#endif
