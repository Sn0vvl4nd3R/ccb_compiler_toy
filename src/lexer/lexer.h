#ifndef LEXER_H
#define LEXER_H

#include "../common/token.h"

typedef struct {
  const char* input;
  size_t input_len;
  int position;
  int read_position;
  char ch;
} Lexer;

Lexer* NewLexer(const char* input);

Token NextToken(Lexer* l);

#endif
