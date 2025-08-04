#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

typedef enum {
  PREC_LOWEST,
  PREC_ASSIGNMENT,
  PREC_LESSGREATER,
  PREC_COMPARISON,
  PREC_EQUALITY,
  PREC_SUM,
  PREC_PRODUCT,
  PREC_PREFIX,
  PREC_CALL
} Precedence;

static Precedence precedences[] = {
  [TOKEN_ASSIGN] = PREC_ASSIGNMENT,
  [TOKEN_LESS] = PREC_LESSGREATER,
  [TOKEN_GREATER] = PREC_LESSGREATER,
  [TOKEN_LESS_EQUAL] = PREC_COMPARISON,
  [TOKEN_GREATER_EQUAL] = PREC_COMPARISON,
  [TOKEN_EQUAL] = PREC_EQUALITY,
  [TOKEN_NOT_EQUAL] = PREC_EQUALITY,
  [TOKEN_PLUS] = PREC_SUM,
  [TOKEN_MINUS] = PREC_SUM,
  [TOKEN_SLASH] = PREC_PRODUCT,
  [TOKEN_ASTERISK] = PREC_PRODUCT,
};

static Precedence GetPrecedence(TokenType type) {
  size_t n = sizeof(precedences) / sizeof(precedences[0]);
  if ((size_t)type >= n) {
    return PREC_LOWEST;
  }
  return precedences[type];
}

const char* TokenName(TokenType t) {
  switch (t) {
    case TOKEN_ILLEGAL: return "ILLEGAL";
    case TOKEN_EOF: return "EOF";
    case TOKEN_IDENT: return "IDENT";
    case TOKEN_INT: return "INT";
    case TOKEN_ASSIGN: return "=";
    case TOKEN_PLUS: return "+";
    case TOKEN_MINUS: return "-";
    case TOKEN_ASTERISK: return "*";
    case TOKEN_SLASH: return "/";
    case TOKEN_LESS: return "<";
    case TOKEN_GREATER: return ">";
    case TOKEN_LESS_EQUAL: return "<=";
    case TOKEN_GREATER_EQUAL: return ">=";
    case TOKEN_EQUAL: return "==";
    case TOKEN_NOT_EQUAL: return "!=";
    case TOKEN_SEMICOLON: return ";";
    case TOKEN_LPAREN: return "(";
    case TOKEN_RPAREN: return ")";
    case TOKEN_LBRACE: return "{";
    case TOKEN_RBRACE: return "}";
    case TOKEN_COMMA: return ",";
    case TOKEN_DOT: return ".";
    case TOKEN_ARROW: return "->";
    case TOKEN_LET: return "let";
    case TOKEN_IF: return "if";
    case TOKEN_ELSE: return "else";
    case TOKEN_WHILE: return "while";
    case TOKEN_OUT: return "out";
    case TOKEN_IN: return "in";
    case TOKEN_NS: return "ns";
    case TOKEN_FN: return "fn";
    case TOKEN_RETURN: return "return";
    default: return "?";
  }
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
  }
  printf("ERROR: Expected token %s, got %s\n", TokenName(t), TokenName(p->peek_token.type));
  return 0;
}

static char* StrDup(const char* s) {
  size_t n = strlen(s);
  char* r = (char*)malloc(n + 1);
  memcpy(r, s, n + 1);
  return r;
}

static char* JoinQualified(const char* ns, const char* name) {
  if (!ns || ns[0] == '\0') {
    return StrDup(name);
  }
  size_t ln = strlen(ns), lm = strlen(name);
  char* s = (char*)malloc(ln + 1 + lm + 1);
  memcpy(s, ns, ln);
  s[ln] = '.';
  memcpy(s + ln + 1, name, lm);
  s[ln + 1 + lm] = '\0';
  return s;
}

static Statement* ParseStatement(Parser* p);
static Expression* ParseExpression(Parser* p, Precedence precedence);
static BlockStatement* ParseBlockStatement(Parser* p);
static Statement* ParseLetStatement(Parser* p);
static Statement* ParseWhileStatement(Parser* p);
static Statement* ParseOutStatement(Parser* p);
static Statement* ParseInStatement(Parser* p);
static Statement* ParseExpressionStatement(Parser* p);
static Expression* ParseIdentifier(Parser* p);
static Expression* ParseIntegerLiteral(Parser* p);
static Expression* ParseIfExpression(Parser* p);
static Expression* ParseInfixExpression(Parser* p, Expression* left);
static Expression* ParseAssignmentExpression(Parser* p, Expression* left);
static Statement* ParseNamespace(Parser* p);
static Statement* ParseFunction(Parser* p);
static Statement* ParseReturn(Parser* p);
static Expression* ParseCallExpression(Parser* p, Expression* function);

static Identifier* MakeRawIdent(Token tok) {
  Identifier* id = (Identifier*)malloc(sizeof(Identifier));
  id->base.node.type = NODE_IDENTIFIER;
  id->token = tok;
  id->value = StrDup(tok.literal);
  return id;
}

Parser* NewParser(Lexer* l) {
  Parser* p = (Parser*)malloc(sizeof(Parser));
  p->l = l;
  p->ns_prefix = (char*)malloc(1);
  p->ns_prefix[0] = '\0';
  p->in_function_depth = 0;

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
      program->statements = (Statement**)realloc(program->statements,
                              program->statement_count * sizeof(Statement*));
      program->statements[program->statement_count - 1] = stmt;
    }
    ParserNextToken(p);
  }
  return program;
}

static Statement* ParseStatement(Parser* p) {
  switch (p->current_token.type) {
    case TOKEN_LET: return ParseLetStatement(p);
    case TOKEN_WHILE: return ParseWhileStatement(p);
    case TOKEN_OUT: return ParseOutStatement(p);
    case TOKEN_IN: return ParseInStatement(p);
    case TOKEN_NS: return ParseNamespace(p);
    case TOKEN_FN: return ParseFunction(p);
    case TOKEN_RETURN: return ParseReturn(p);
    default: return ParseExpressionStatement(p);
  }
}

static Statement* ParseNamespace(Parser* p) {
  if (!ExpectPeek(p, TOKEN_IDENT)) {
    return NULL;
  }
  char* nsname = p->current_token.literal;

  char* old = p->ns_prefix;
  char* combined = (old[0] == '\0') ? StrDup(nsname) : JoinQualified(old, nsname);
  p->ns_prefix = combined;

  if (!ExpectPeek(p, TOKEN_LBRACE)) {
    free(combined);
    p->ns_prefix = old;
    return NULL;
  }

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
      block->statements = (Statement**)realloc(block->statements,
                                block->statement_count * sizeof(Statement*));
      block->statements[block->statement_count - 1] = stmt;
    }
    ParserNextToken(p);
  }
  if (p->current_token.type != TOKEN_RBRACE) {
    printf("ERROR: expected '}'\n");
  }

  free(p->ns_prefix);
  p->ns_prefix = old;

  return (Statement*)block;
}

static Statement* ParseFunction(Parser* p) {
  if (!ExpectPeek(p, TOKEN_IDENT)) {
    return NULL;
  }

  Identifier* name = (Identifier*)malloc(sizeof(Identifier));
  name->base.node.type = NODE_IDENTIFIER;
  name->token = p->current_token;
  name->value = JoinQualified(p->ns_prefix, p->current_token.literal);

  if (!ExpectPeek(p, TOKEN_LPAREN)) {
    free(name->value);
    free(name);
    return NULL;
  }

  Identifier** params = NULL;
  int param_count = 0;

  if (p->peek_token.type != TOKEN_RPAREN) {
    if (!ExpectPeek(p, TOKEN_IDENT)) {
      free(name->value); free(name);
      return NULL;
    }
    params = (Identifier**)realloc(params, (param_count + 1) * sizeof(Identifier*));
    params[param_count++] = MakeRawIdent(p->current_token);

    while (p->peek_token.type == TOKEN_COMMA) {
      ParserNextToken(p);
      if (!ExpectPeek(p, TOKEN_IDENT)) {
        for (int i = 0; i < param_count; i++) {
          FreeExpression((Expression*)params[i]);
        }
        free(params);
        free(name->value);
        free(name);
        return NULL;
      }
      params = (Identifier**)realloc(params, (param_count + 1) * sizeof(Identifier*));
      params[param_count++] = MakeRawIdent(p->current_token);
    }
  }

  if (!ExpectPeek(p, TOKEN_RPAREN)) {
    for (int i = 0; i < param_count; i++) {
      FreeExpression((Expression*)params[i]);
    }
    free(params);
    free(name->value);
    free(name);
    return NULL;
  }

  char* ret_type = NULL;
  if (p->peek_token.type == TOKEN_ARROW) {
    ParserNextToken(p);
    if (!ExpectPeek(p, TOKEN_IDENT)) {
      for (int i = 0; i < param_count; i++) {
        FreeExpression((Expression*)params[i]);
      }
      free(params);
      free(name->value);
      free(name);
      return NULL;
    }
    ret_type = StrDup(p->current_token.literal);
  }

  if (!ExpectPeek(p, TOKEN_LBRACE)) {
    if (ret_type) {
      free(ret_type);
    }
    for (int i = 0; i < param_count; i++) {
      FreeExpression((Expression*)params[i]);
    }
    free(params);
    free(name->value);
    free(name);
    return NULL;
  }

  p->in_function_depth++;
  BlockStatement* body = ParseBlockStatement(p);
  p->in_function_depth--;

  FunctionStatement* fn = (FunctionStatement*)malloc(sizeof(FunctionStatement));
  fn->base.node.type = NODE_FUNCTION_STATEMENT;
  fn->token = (Token){ .type = TOKEN_FN, .literal = StrDup("fn") };
  fn->name = name;
  fn->params = params;
  fn->param_count = param_count;
  fn->body = body;
  fn->return_type = ret_type ? ret_type : StrDup("int");
  return (Statement*)fn;
}

static Statement* ParseReturn(Parser* p) {
  ReturnStatement* rs = (ReturnStatement*)malloc(sizeof(ReturnStatement));
  rs->base.node.type = NODE_RETURN_STATEMENT;
  rs->token = p->current_token;

  if (p->peek_token.type == TOKEN_SEMICOLON) {
    ParserNextToken(p);
    rs->value = NULL;
    return (Statement*)rs;
  }

  ParserNextToken(p);
  rs->value = ParseExpression(p, PREC_LOWEST);
  if (p->peek_token.type == TOKEN_SEMICOLON) {
    ParserNextToken(p);
  }
  return (Statement*)rs;
}

static BlockStatement* ParseBlockStatement(Parser* p) {
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
      block->statements = (Statement**)realloc(block->statements,
                                block->statement_count * sizeof(Statement*));
      block->statements[block->statement_count - 1] = stmt;
    }
    ParserNextToken(p);
  }
  if (p->current_token.type != TOKEN_RBRACE) {
    printf("ERROR: expected '}'\n");
  }
  return block;
}

static Statement* ParseLetStatement(Parser* p) {
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
  if (p->in_function_depth > 0) {
    name->value = StrDup(p->current_token.literal);
  } else {
    name->value = JoinQualified(p->ns_prefix, p->current_token.literal);
  }
  stmt->name = name;

  if (!ExpectPeek(p, TOKEN_ASSIGN)) {
    free(name->value);
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

static Statement* ParseOutStatement(Parser* p) {
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

static Statement* ParseInStatement(Parser* p) {
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
  if (p->in_function_depth > 0) {
    name->value = StrDup(p->current_token.literal);
  } else {
    name->value = JoinQualified(p->ns_prefix, p->current_token.literal);
  }
  stmt->name = name;

  if (p->peek_token.type == TOKEN_SEMICOLON) {
    ParserNextToken(p);
  }
  return (Statement*)stmt;
}

static Expression* ParseExpression(Parser* p, Precedence precedence) {
  Expression* (*prefix_fn)(Parser*) = NULL;
  switch (p->current_token.type) {
    case TOKEN_IDENT:
      prefix_fn = ParseIdentifier;
      break;
    case TOKEN_INT:
      prefix_fn = ParseIntegerLiteral;
      break;
    case TOKEN_IF:
      prefix_fn = ParseIfExpression;
      break;
    default:
      printf("ERROR: Doesn't found prefix-function for token %d\n", p->current_token.type);
      return NULL;
  }

  Expression* left_exp = prefix_fn(p);
  if (left_exp == NULL) {
    return NULL;
  }

  for (;;) {
    if (p->peek_token.type == TOKEN_LPAREN) {
      ParserNextToken(p);
      left_exp = ParseCallExpression(p, left_exp);
      if (left_exp == NULL) {
        return NULL;
      }
      continue;
    }

    Precedence next_prec = GetPrecedence(p->peek_token.type);
    if (precedence >= next_prec) {
      break;
    }

    Expression* (*infix_fn)(Parser*, Expression*) = NULL;
    switch (p->peek_token.type) {
      case TOKEN_PLUS:
      case TOKEN_MINUS:
      case TOKEN_SLASH:
      case TOKEN_ASTERISK:
      case TOKEN_LESS:
      case TOKEN_GREATER:
      case TOKEN_LESS_EQUAL:
      case TOKEN_GREATER_EQUAL:
      case TOKEN_EQUAL:
      case TOKEN_NOT_EQUAL:
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
    if (left_exp == NULL) {
      return NULL;
    }
  }
  return left_exp;
}

static Expression* ParseIntegerLiteral(Parser* p) {
  IntegerLiteral* lit = (IntegerLiteral*)malloc(sizeof(IntegerLiteral));
  lit->base.node.type = NODE_INTEGER_LITERAL;
  lit->token = p->current_token;
  lit->value = atoi(p->current_token.literal);
  return (Expression*)lit;
}

static Expression* ParseIdentifier(Parser* p) {
  Token startTok = p->current_token;

  char* full = StrDup(startTok.literal);
  while (p->peek_token.type == TOKEN_DOT) {
    ParserNextToken(p);
    if (!ExpectPeek(p, TOKEN_IDENT)) {
      break;
    }
    size_t ln = strlen(full), lm = strlen(p->current_token.literal);
    char* tmp = (char*)malloc(ln + 1 + lm + 1);
    memcpy(tmp, full, ln);
    tmp[ln] = '.';
    memcpy(tmp + ln + 1, p->current_token.literal, lm);
    tmp[ln + 1 + lm] = '\0';
    free(full);
    full = tmp;
  }

  Identifier* ident = (Identifier*)malloc(sizeof(Identifier));
  ident->base.node.type = NODE_IDENTIFIER;
  ident->token = startTok;
  ident->value = full;
  return (Expression*)ident;
}

static Expression* ParseInfixExpression(Parser* p, Expression* left) {
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

static Expression* ParseAssignmentExpression(Parser* p, Expression* left) {
  if (!left || left->node.type != NODE_IDENTIFIER) {
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

static Expression* ParseCallExpression(Parser* p, Expression* function) {
  CallExpression* call = (CallExpression*)malloc(sizeof(CallExpression));
  call->base.node.type = NODE_CALL_EXPRESSION;
  call->token = p->current_token;
  call->function = function;
  call->arguments = NULL;
  call->arg_count = 0;

  if (p->peek_token.type == TOKEN_RPAREN) {
    ParserNextToken(p);
    return (Expression*)call;
  }

  ParserNextToken(p);
  call->arguments = (Expression**)realloc(call->arguments, (call->arg_count+1)*sizeof(Expression*));
  call->arguments[call->arg_count++] = ParseExpression(p, PREC_LOWEST);

  while (p->peek_token.type == TOKEN_COMMA) {
    ParserNextToken(p);
    ParserNextToken(p);
    call->arguments = (Expression**)realloc(call->arguments, (call->arg_count+1)*sizeof(Expression*));
    call->arguments[call->arg_count++] = ParseExpression(p, PREC_LOWEST);
  }

  if (!ExpectPeek(p, TOKEN_RPAREN)) {
    printf("ERROR: expected ')' after arguments, got %s\n", TokenName(p->peek_token.type));
    free(call->arguments);
    free(call);
    return NULL;
  }
  return (Expression*)call;
}

static Statement* ParseWhileStatement(Parser* p) {
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

static Expression* ParseIfExpression(Parser* p) {
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
    if (!ExpectPeek(p, TOKEN_LBRACE)) {
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

static Statement* ParseExpressionStatement(Parser* p) {
  ExpressionStatement* stmt = (ExpressionStatement*)malloc(sizeof(ExpressionStatement));
  stmt->base.node.type = NODE_EXPRESSION_STATEMENT;
  stmt->token = p->current_token;

  stmt->expression = ParseExpression(p, PREC_LOWEST);

  if (p->peek_token.type == TOKEN_SEMICOLON) {
    ParserNextToken(p);
  }
  return (Statement*)stmt;
}

