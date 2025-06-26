#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

typedef enum {
  PREC_LOWEST,
  PREC_ASSIGNMENT,
  PREC_LESSGREATER,
  PREC_SUM,
  PREC_PRODUCT,
  PREC_PREFIX,
  PREC_CALL
} Precedence;

Precedence precedences[] = {
  [TOKEN_ASSIGN] = PREC_ASSIGNMENT,
  [TOKEN_LESS] = PREC_LESSGREATER,
  [TOKEN_GREATER] = PREC_LESSGREATER,
  [TOKEN_PLUS] = PREC_SUM,
  [TOKEN_MINUS] = PREC_SUM,
  [TOKEN_SLASH] = PREC_PRODUCT,
  [TOKEN_ASTERISK] = PREC_PRODUCT,
};

Statement* ParseStatement(Parser* p);
Expression* ParseExpression(Parser* p, Precedence precedence);
BlockStatement* ParseBlockStatement(Parser* p);
Statement* ParseLetStatement(Parser* p);
Statement* ParseWhileStatement(Parser* p);
Statement* ParseOutStatement(Parser* p);
Statement* ParseInStatement(Parser* p);
Statement* ParseExpressionStatement(Parser* p);
Expression* ParseIdentifier(Parser* p);
Expression* ParseIntegerLiteral(Parser* p);
Expression* ParseIfExpression(Parser* p);
Expression* ParseInfixExpression(Parser* p, Expression* left);
Expression* ParseAssignmentExpression(Parser* p, Expression* left);

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

static int ExpectPeek(Parser* p, TokenType t) {
  if (p->peek_token.type == TOKEN_ILLEGAL) {
    printf("LEXER ERROR: illegal character '%s'\n", p->peek_token.literal);
    return 0;
  }

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

Statement* ParseStatement(Parser* p) {
  switch (p->current_token.type) {
    case TOKEN_LET:
      return ParseLetStatement(p);
    case TOKEN_WHILE:
      return ParseWhileStatement(p);
    case TOKEN_OUT:
      return ParseOutStatement(p);
    case TOKEN_IN:
      return ParseInStatement(p);
    default:
      return ParseExpressionStatement(p);
  }
}

BlockStatement* ParseBlockStatement(Parser* p) {
  BlockStatement* block = (BlockStatement*)malloc(sizeof(BlockStatement));
  block->base.node.type = NODE_BLOCK_STATEMENT;
  block->token = p->current_token;
  block->statements = NULL;
  block->statement_count = 0;

  ParserNextToken(p);

  while (p->current_token.type != TOKEN_RBRACE && p->current_token.type != TOKEN_EOF) {
    Statement* stmt = ParseStatement(p);
    if (stmt != NULL) {
      block->statement_count++;
      block->statements = (Statement**)realloc(block->statements, block->statement_count * sizeof(Statement*));
      block->statements[block->statement_count - 1] = stmt;
    }
    ParserNextToken(p);
  }

  if (p->current_token.type != TOKEN_RBRACE) {
    printf("ERROR: expected '}'\n");
  }

  return block;
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

Statement* ParseOutStatement(Parser* p) {
  OutStatement* stmt = (OutStatement*)malloc(sizeof(OutStatement));
  stmt->base.node.type = NODE_OUT_STATEMENT;
  stmt->token = p->current_token;

  ParserNextToken(p);
  stmt->value = ParseExpression(p, PREC_LOWEST);

  if (p->peek_token.type == TOKEN_SEMICOLON) {
    ParserNextToken(p);
  }

  return (Statement*)stmt;
}

Statement* ParseInStatement(Parser* p) {
  InStatement* stmt = (InStatement*)malloc(sizeof(InStatement));
  stmt->base.node.type = NODE_IN_STATEMENT;
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

  if (p->peek_token.type == TOKEN_SEMICOLON) {
    ParserNextToken(p);
  }

  return (Statement*)stmt;
}

Expression* ParseAssignmentExpression(Parser* p, Expression* left) {
  if (left->node.type != NODE_IDENTIFIER) {
    printf("ERROR: Invalid assignment target.\n");
    return NULL;
  }

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


Statement* ParseExpressionStatement(Parser* p) {
  ExpressionStatement* stmt = (ExpressionStatement*)malloc(sizeof(ExpressionStatement));
  stmt->base.node.type = NODE_EXPRESSION_STATEMENT;
  stmt->token = p->current_token;

  stmt->expression = ParseExpression(p, PREC_LOWEST);

  if (p->peek_token.type == TOKEN_SEMICOLON) {
    ParserNextToken(p);
  }

  return (Statement*)stmt;
}

Expression* ParseExpression(Parser* p, Precedence precedence) {
  Expression* (*prefix_fn)(Parser*) = NULL;
  if (p->current_token.type == TOKEN_IDENT) {
    prefix_fn = ParseIdentifier;
  } else if (p->current_token.type == TOKEN_INT) {
    prefix_fn = ParseIntegerLiteral;
  } else if (p->current_token.type == TOKEN_IF) {
    prefix_fn = ParseIfExpression;
  }

  if (prefix_fn == NULL) {
    printf("ERROR: Doesn't found prefix-function for token %d\n", p->current_token.type);
    return NULL;
  }

  Expression* left_exp = prefix_fn(p);

  while (precedence < GetPrecedence(p->peek_token.type)) {
    Expression* (*infix_fn)(Parser*, Expression*) = NULL;
    switch (p->peek_token.type) {
      case TOKEN_PLUS:
      case TOKEN_MINUS:
      case TOKEN_SLASH:
      case TOKEN_ASTERISK:
        infix_fn = ParseInfixExpression;
        break;
      case TOKEN_LESS:
      case TOKEN_GREATER:
        infix_fn = ParseInfixExpression;
        break;
      case TOKEN_ASSIGN:
        infix_fn = ParseAssignmentExpression;
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

Statement* ParseWhileStatement(Parser* p) {
  WhileStatement* stmt = (WhileStatement*)malloc(sizeof(WhileStatement));
  stmt->base.node.type = NODE_WHILE_STATEMENT;
  stmt->token = p->current_token;

  if (!ExpectPeek(p, TOKEN_LPAREN)) {
    return NULL;
  }

  ParserNextToken(p);
  stmt->condition = ParseExpression(p, PREC_LOWEST);

  if (!ExpectPeek(p, TOKEN_RPAREN)) {
    return NULL;
  }
  if (!ExpectPeek(p, TOKEN_LBRACE)) {
    return NULL;
  }

  stmt->body = ParseBlockStatement(p);
  return (Statement*)stmt;
}

Expression* ParseIfExpression(Parser* p) {
  IfExpression* exp = (IfExpression*)malloc(sizeof(IfExpression));
  exp->base.node.type = NODE_IF_EXPRESSION;
  exp->token = p->current_token;

  if (!ExpectPeek(p, TOKEN_LPAREN)) {
    return NULL;
  }

  ParserNextToken(p);
  exp->condition = ParseExpression(p, PREC_LOWEST);

  if (!ExpectPeek(p, TOKEN_RPAREN)) {
    return NULL;
  }
  if (!ExpectPeek(p, TOKEN_LBRACE)) {
    return NULL;
  }

  exp->consequence = ParseBlockStatement(p);

  if (p->peek_token.type == TOKEN_ELSE) {
    ParserNextToken(p);
    if (p->current_token.literal) {
      free(p->current_token.literal);
      p->current_token.literal = NULL;
    }

    if (!ExpectPeek(p, TOKEN_LBRACE)) {
      free(exp->token.literal);
      FreeStatement((Statement*)exp->consequence);
      free(exp);
      return NULL;
    }
    exp->alternative = ParseBlockStatement(p);
  } else {
    exp->alternative = NULL;
  }
  return (Expression*)exp;
}
