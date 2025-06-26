#ifndef AST_H
#define AST_H

#include "token.h"

typedef enum {
  NODE_PROGRAM,
  NODE_LET_STATEMENT,
  NODE_EXPRESSION_STATEMENT,
  NODE_IDENTIFIER,
  NODE_INTEGER_LITERAL,
  NODE_INFIX_EXPRESSION,
  NODE_IF_EXPRESSION,
  NODE_BLOCK_STATEMENT,
  NODE_WHILE_STATEMENT,
  NODE_OUT_STATEMENT,
  NODE_IN_STATEMENT,
} NodeType;


typedef struct Node {
  NodeType type;
} Node;

typedef struct Expression {
  Node node;
} Expression;

typedef struct Statement {
  Node node;
} Statement;


typedef struct {
  Expression base;
  Token token;
  int value;
} IntegerLiteral;

typedef struct {
  Expression base;
  Token token;
  char* value;
} Identifier;

typedef struct {
  Expression base;
  Token token;
  Expression* left;
  char* operator;
  Expression* right;
} InfixExpression;


typedef struct {
  Statement base;
  Token token;
  Identifier* name;
  Expression* value;
} LetStatement;

typedef struct {
  Statement base;
  Token token;
  Expression* expression;
} ExpressionStatement;


typedef struct {
  Node node;
  Statement** statements;
  int statement_count;
} Program;


typedef struct {
  Statement base;
  Token token;
  Statement** statements;
  int statement_count;
} BlockStatement;

typedef struct {
  Statement base;
  Token token;
  Expression* condition;
  BlockStatement* consequence;
  BlockStatement* alternative;
} IfExpression;


typedef struct {
  Statement base;
  Token token;
  Expression* condition;
  BlockStatement* body;
} WhileStatement;


typedef struct {
  Statement base;
  Token token;
  Expression* value;
} OutStatement;

typedef struct {
  Statement base;
  Token token;
  Identifier* name;
} InStatement;

void FreeProgram(Program* program);
void FreeExpression(Expression* expr);
void FreeStatement(Statement* stmt);
void PrintAst(Program* p);

#endif
