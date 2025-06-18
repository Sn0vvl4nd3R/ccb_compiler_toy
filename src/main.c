#include <stdio.h>
#include <stdlib.h>
#include "common/token.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

void PrintNode(Node* node, int ident);

void PrintWithIndent(const char* text, int indent) {
  for (int i = 0; i < indent; i++) {
    printf("  ");
  }
  printf("%s\n", text);
}

void PrintExpression(Expression* exp, int indent) {
  if (!exp) {
    return;
  }

  switch (exp->node.type) {
    case NODE_IDENTIFIER: {
      Identifier* ident = (Identifier*)exp;
      char buf[100];
      sprintf(buf, "Identifier(%s)", ident->value);
      break;
    }
    case NODE_INTEGER_LITERAL: {
      IntegerLiteral* lit = (IntegerLiteral*)exp;
      char buf[100];
      sprintf(buf, "Integer(%d)", lit->value);
      PrintWithIndent(buf, indent);
      break;
    }
    case NODE_INFIX_EXPRESSION: {
      InfixExpression* infix = (InfixExpression*)exp;
      char buf[100];
      sprintf(buf, "Infix(%s)", infix->operator);
      PrintWithIndent(buf, indent);
      PrintExpression(infix->left, indent + 1);
      PrintExpression(infix->right, indent + 1);
      break;
    }
    default:
      PrintWithIndent("Unknown Expression", indent);
  }
}

void PrintStatement(Statement* stmt, int indent) {
  if (!stmt) {
    return;
  }

  switch (stmt->node.type) {
    case NODE_LET_STATEMENT: {
      LetStatement* let_stmt = (LetStatement*)stmt;
      char buf[100];
      sprintf(buf, "Let: %s =", let_stmt->name->value);
      PrintWithIndent(buf, indent);
      PrintExpression(let_stmt->value, indent + 1);
      break;
    }
    default:
      PrintWithIndent("Unknown Statement", indent);
  }
}

void PrintAst(Program* p) {
  if (!p) {
    return;
  }
  PrintWithIndent("Program", 0);
  for (int i = 0; i < p->statement_count; i++) {
    PrintStatement(p->statements[i], 1);
  }
}

void PrintToken(Token tok) {
  printf("Type: %d, Literal: \"%s\"\n", tok.type, tok.literal);
}

int main(void) {
  const char* input = "let answer = 10 + 2 * 15;";

  Lexer* l = NewLexer(input);
  Parser* p = NewParser(l);
  Program* program = ParseProgram(p);

  if (program) {
    PrintAst(program);
  }

  free(l);
  free(p);

  return 0;
}
