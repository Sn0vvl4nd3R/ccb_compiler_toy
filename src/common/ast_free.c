#include <stdlib.h>
#include "ast.h"

void FreeExpression(Expression* expr);
void FreeStatement(Statement* stmt);

void FreeExpression(Expression* expr) {
  if (!expr) {
    return;
  }

  switch (expr->node.type) {
    case NODE_INTEGER_LITERAL: {
      IntegerLiteral* lit = (IntegerLiteral*)expr;
      free(lit->token.literal);
      free(lit);
      break;
    }
    case NODE_IDENTIFIER: {
      Identifier* ident = (Identifier*)expr;
      free(ident->value);
      ident->token.literal = NULL;
      free(ident);
      break;
    }
    case NODE_INFIX_EXPRESSION: {
      InfixExpression* inf = (InfixExpression*)expr;
      FreeExpression(inf->left);
      FreeExpression(inf->right);
      free(inf);
      break;
    }
    case NODE_IF_EXPRESSION: {
      IfExpression* ife = (IfExpression*)expr;
      FreeExpression(ife->condition);
      FreeStatement((Statement*)ife->consequence);
      if (ife->alternative) {
        FreeStatement((Statement*)ife->alternative);
      }
      free(ife->token.literal);
      free(ife);
      break;
    }
    default:
      break;
  }
}

void FreeStatement(Statement* stmt) {
  if (!stmt) {
    return;
  }

  switch (stmt->node.type) {
    case NODE_LET_STATEMENT: {
      LetStatement* let = (LetStatement*)stmt;
      FreeExpression((Expression*)let->name);
      FreeExpression(let->value);
      free(let->token.literal);
      free(let);
      break;
    }
    case NODE_EXPRESSION_STATEMENT: {
      ExpressionStatement* es = (ExpressionStatement*)stmt;
      FreeExpression(es->expression);
      free(es);
      break;
    }
    case NODE_OUT_STATEMENT: {
      OutStatement* out = (OutStatement*)stmt;
      FreeExpression(out->value);
      free(out->token.literal);
      free(out);
      break;
    }
    case NODE_IN_STATEMENT: {
      InStatement* in = (InStatement*)stmt;
      FreeExpression((Expression*)in->name);
      free(in->token.literal);
      free(in);
      break;
    }
    case NODE_BLOCK_STATEMENT: {
      BlockStatement* block = (BlockStatement*)stmt;
      for (int i = 0; i < block->statement_count; i++) {
        FreeStatement(block->statements[i]);
      }
      free(block->statements);
      free(block);
      break;
    }
    case NODE_WHILE_STATEMENT: {
      WhileStatement* wh = (WhileStatement*)stmt;
      FreeExpression(wh->condition);
      FreeStatement((Statement*)wh->body);
      free(wh->token.literal);
      free(wh);
      break;
    }
    default:
      break;
  }
}

void FreeProgram(Program* program) {
  if (!program) {
    return;
  }

  for (int i = 0; i < program->statement_count; i++) {
    FreeStatement(program->statements[i]);
  }

  free(program->statements);
  free(program);
}
