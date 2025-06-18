#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include "../common/ast.h"

typedef struct {
  Lexer* l;
  Token current_token;
  Token peek_token;
} Parser;

Parser* NewParser(Lexer* l);
Program* ParseProgram(Parser* p);

#endif
