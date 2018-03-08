#ifndef ARITH_LEXER_H
#define ARITH_LEXER_H 1

enum TokenType {
    TOKEN_NONE,
    NUM,
    ID,
    BECOMES,
    ADD,
    SUB,
    MUL,
    DIV,
    LPAREN,
    RPAREN,
    WHITESPACE,
    TOKEN_LAST
};

struct Token {
    enum TokenType tok;
    int line, col;
    char *text;
};

/* Simple maximal-munch tokenizer */
struct Token **tokenize(char *input);

/* Given a token type, return a string name */
const char *tokenName(enum TokenType tok);

#endif
