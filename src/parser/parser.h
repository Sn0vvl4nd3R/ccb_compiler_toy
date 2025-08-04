#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include "../common/ast.h"

typedef struct {
  Lexer* l;
  Token current_token;
  Token peek_token;

  char* ns_prefix;

  int in_function_depth;
} Parser;

Parser* NewParser(Lexer* l);
Program* ParseProgram(Parser* p);

const char* TokenName(TokenType t);

#endif

