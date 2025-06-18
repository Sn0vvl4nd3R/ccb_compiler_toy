#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
  TOKEN_ILLEGAL,
  TOKEN_EOF,

  TOKEN_IDENT,
  TOKEN_INT,

  TOKEN_ASSIGN,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_ASTERISK,
  TOKEN_SLASH,

  TOKEN_SEMICOLON,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACE,
  TOKEN_RBRACE,

  TOKEN_LET,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_OUT,
} TokenType;

typedef struct {
  TokenType type;
  char* literal;
} Token;

#endif
