#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

typedef enum {
  PREC_LOWEST,
  PREC_EQUALS,
  PREC_LESSGREATER,
  PREC_SUM,
  PREC_PRODUCT,
  PREC_PREFIX,
  PREC_CALL
} Precedence;

Precedence precedences[] = {
  [TOKEN_ASSIGN] = PREC_EQUALS,
  [TOKEN_PLUS] = PREC_SUM,
  [TOKEN_MINUS] = PREC_SUM,
  [TOKEN_SLASH] = PREC_PRODUCT,
  [TOKEN_ASTERISK] = PREC_PRODUCT,
};

Expression* ParseExpression(Parser* p, Precedence precedence);
Expression* ParseIntegerLiteral(Parser* p);
Expression* ParseIdentifier(Parser* p);
Expression* ParseInfixExpression(Parser* p, Expression* left);

static Precedence GetPrecedence(TokenType type) {
  if (type >= (sizeof(precedences)/sizeof(precedences[0]))) {
    return PREC_LOWEST;
  }
  return precedences[type];
}

static void ParserNextToken(Parser* p) {
  p->current_token = p->peek_token;
  p->peek_token = NextToken(p->l);
}

int ExpectPeek(Parser* p, TokenType t) {
  if (p->peek_token.type == t) {
    ParserNextToken(p);
    return 1;
  } else {
    printf("ERROR: Expected token %d, got %d\n", t, p->peek_token.type);
    return 0;
  }
}

Parser* NewParser(Lexer* l) {
  Parser* p = (Parser*)malloc(sizeof(Parser));
  p->l = l;

  ParserNextToken(p);
  ParserNextToken(p);

  return p;
}

Statement* ParseLetStatement(Parser* p) {
  LetStatement* stmt = (LetStatement*)malloc(sizeof(LetStatement));
  stmt->base.node.type = NODE_LET_STATEMENT;
  stmt->token = p->current_token;

  if (!ExpectPeek(p, TOKEN_IDENT)) {
    free(stmt);
    return NULL;
  }

  Identifier* name = (Identifier*)malloc(sizeof(Identifier));
  name->base.node.type = NODE_IDENTIFIER;
  name->token = p->current_token;
  name->value = p->current_token.literal;
  stmt->name = name;

  if (!ExpectPeek(p, TOKEN_ASSIGN)) {
    free(name);
    free(stmt);
    return NULL;
  }

  ParserNextToken(p);

  stmt->value = ParseExpression(p, PREC_LOWEST);

  if (p->peek_token.type == TOKEN_SEMICOLON) {
    ParserNextToken(p);
  }

  return (Statement*)stmt;
}

Statement* ParseStatement(Parser* p) {
  switch (p->current_token.type) {
    case TOKEN_LET:
      return ParseLetStatement(p);
    default:
      return NULL;
  }
}

Expression* ParseExpression(Parser* p, Precedence precedence) {
  Expression* (*prefix_fn)(Parser*) = NULL;
  if (p->current_token.type == TOKEN_IDENT) {
    prefix_fn = ParseIdentifier;
  } else if (p->current_token.type == TOKEN_INT) {
    prefix_fn = ParseIntegerLiteral;
  }

  if (prefix_fn == NULL) {
    printf("ERROR: Doesn't found prefix-function for token %d\n", p->current_token.type);
    return NULL;
  }

  Expression* left_exp = prefix_fn(p);

  while (p->peek_token.type != TOKEN_SEMICOLON && precedence < GetPrecedence(p->peek_token.type)) {
    Expression* (*infix_fn)(Parser*, Expression*) = NULL;
    switch (p->peek_token.type) {
      case TOKEN_PLUS:
      case TOKEN_MINUS:
      case TOKEN_SLASH:
      case TOKEN_ASTERISK:
        infix_fn = ParseInfixExpression;
        break;
      default:
        return left_exp;
    }
    ParserNextToken(p);
    left_exp = infix_fn(p, left_exp);
  }
  return left_exp;
}

Expression* ParseIntegerLiteral(Parser* p) {
  IntegerLiteral* lit = (IntegerLiteral*)malloc(sizeof(IntegerLiteral));
  lit->base.node.type = NODE_INTEGER_LITERAL;
  lit->token = p->current_token;
  lit->value = atoi(p->current_token.literal);

  return (Expression*)lit;
}

Expression* ParseIdentifier(Parser* p) {
  Identifier* ident = (Identifier*)malloc(sizeof(Identifier));
  ident->base.node.type = NODE_IDENTIFIER;
  ident->token = p->current_token;
  ident->value = p->current_token.literal;

  return (Expression*)ident;
}

Expression* ParseInfixExpression(Parser* p, Expression* left) {
  InfixExpression* exp = (InfixExpression*)malloc(sizeof(InfixExpression));
  exp->base.node.type = NODE_INFIX_EXPRESSION;
  exp->token = p->current_token;
  exp->operator = p->current_token.literal;
  exp->left = left;

  Precedence precedence = GetPrecedence(p->current_token.type);
  ParserNextToken(p);

  exp->right = ParseExpression(p, precedence);

  return (Expression*)exp;
}

Program* ParseProgram(Parser* p) {
  Program* program = (Program*)malloc(sizeof(Program));
  program->statements = NULL;
  program->statement_count = 0;

  while (p->current_token.type != TOKEN_EOF) {
    Statement* stmt = ParseStatement(p);
    if (stmt != NULL) {
      program->statement_count++;
      program->statements = (Statement**)realloc(program->statements, program->statement_count * sizeof(Statement*));
      program->statements[program->statement_count - 1] = stmt;
    }
    ParserNextToken(p);
  }
  return program;
}
