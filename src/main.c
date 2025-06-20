#include <stdio.h>
#include <stdlib.h>
#include "vm/vm.h"
#include "common/token.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

void PrintExpression(Expression* exp, int indent);
void PrintStatement(Statement* stmt, int indent);

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

  char buf[100];

  switch (exp->node.type) {
    case NODE_IDENTIFIER: {
      Identifier* ident = (Identifier*)exp;
      sprintf(buf, "Identifier(%s)", ident->value);
      PrintWithIndent(buf, indent);
      break;
    }
    case NODE_INTEGER_LITERAL: {
      IntegerLiteral* lit = (IntegerLiteral*)exp;
      sprintf(buf, "Integer(%d)", lit->value);
      PrintWithIndent(buf, indent);
      break;
    }
    case NODE_INFIX_EXPRESSION: {
      InfixExpression* infix = (InfixExpression*)exp;
      sprintf(buf, "Infix(%s)", infix->operator);
      PrintWithIndent(buf, indent);
      PrintExpression(infix->left, indent + 1);
      PrintExpression(infix->right, indent + 1);
      break;
    }
    case NODE_IF_EXPRESSION: {
      IfExpression* if_exp = (IfExpression*)exp;
      PrintWithIndent("If", indent);
      PrintExpression(if_exp->condition, indent + 1);
      PrintStatement((Statement*)if_exp->consequence, indent + 1);
      if (if_exp->alternative) {
        PrintStatement((Statement*)if_exp->alternative, indent + 1);
      }
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
  
  char buf[100];

  switch (stmt->node.type) {
    case NODE_LET_STATEMENT: {
      LetStatement* let_stmt = (LetStatement*)stmt;
      sprintf(buf, "Let: %s =", let_stmt->name->value);
      PrintWithIndent(buf, indent);
      PrintExpression(let_stmt->value, indent + 1);
      break;
    }
    case NODE_EXPRESSION_STATEMENT: {
      ExpressionStatement* expr_stmt = (ExpressionStatement*)stmt;
      PrintExpression(expr_stmt->expression, indent);
      break;
    }
    case NODE_OUT_STATEMENT: {
        OutStatement* out_stmt = (OutStatement*)stmt;
        PrintWithIndent("Out", indent);
        PrintExpression(out_stmt->value, indent + 1);
        break;
    }
    case NODE_WHILE_STATEMENT: {
      WhileStatement* while_stmt = (WhileStatement*)stmt;
      PrintWithIndent("While", indent);
      PrintExpression(while_stmt->condition, indent + 1);
      PrintStatement((Statement*)while_stmt->body, indent + 1);
      break;
    }
    case NODE_BLOCK_STATEMENT: {
      BlockStatement* block_stmt = (BlockStatement*)stmt;
      PrintWithIndent("Block", indent);
      for (int i = 0; i < block_stmt->statement_count; i++) {
        PrintStatement(block_stmt->statements[i], indent + 1);
      }
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

int main(void) {
  const char* input = "let x = 10; while (x > 5) { x = x - 1; out x }";

  Interpret(input);

  return 0;
}
