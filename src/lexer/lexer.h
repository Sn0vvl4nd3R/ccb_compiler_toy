#ifndef LEXER_H
#define LEXER_H

#include "../common/token.h"

typedef struct {
  const char* input;
  int position;
  int read_position;
  char ch;
} Lexer;

Lexer* NewLexer(const char* input);

Token NextToken(Lexer* l);

#endif
