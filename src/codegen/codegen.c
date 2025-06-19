#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

typedef struct {
  Chunk* chunk;
} Compiler;

void CompileNode(Compiler* compiler, Node* node);
void CompileExpression(Compiler* compiler, Expression* expr);
void CompileStatement(Compiler* compiler, Statement* stmt);

void InitChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->constants = NULL;
  chunk->constants_count = 0;
  chunk->constants_capacity = 0;
}

void WriteChunk(Chunk* chunk, uint8_t byte) {
  if (chunk->capacity < chunk->count + 1) {
    int old_capacity = chunk->capacity;
    chunk->capacity = old_capacity < 8 ? 8 : old_capacity * 2;
    chunk->code = realloc(chunk->code, chunk->capacity * sizeof(uint8_t));
  }
  chunk->code[chunk->count] = byte;
  chunk->count++;
}

void FreeChunk(Chunk* chunk) {
  free(chunk->code);
  free(chunk->constants);
  InitChunk(chunk);
}

int AddConstant(Chunk* chunk, int value) {
  if (chunk->constants_capacity < chunk->constants_count + 1) {
    int old_capacity = chunk->constants_capacity;
    chunk->constants_capacity = old_capacity < 8 ? 8 : old_capacity * 2;
    chunk->constants = realloc(chunk->constants, chunk->constants_capacity * sizeof(int));
  }
  chunk->constants[chunk->constants_count] = value;
  return chunk->constants_count++;
}

Chunk* Compile(Program* program) {
  Compiler compiler;
  Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
  InitChunk(chunk);
  compiler.chunk = chunk;

  for (int i = 0; i < program->statement_count; i++) {
    CompileNode(&compiler, (Node*)program->statements[i]);
  }

  WriteChunk(compiler.chunk, OP_POP);
  WriteChunk(compiler.chunk, OP_RETURN);
  return compiler.chunk;
}

void CompileNode(Compiler* compiler, Node* node) {
  if (node == NULL) {
    return;
  }

  if (node->type >= NODE_IDENTIFIER && node->type <= NODE_IF_EXPRESSION) { 
    CompileExpression(compiler, (Expression*)node);
  } else if (node->type >= NODE_LET_STATEMENT && node->type <= NODE_OUT_STATEMENT) {
    CompileStatement(compiler, (Statement*)node);
  } else {
    printf("CODEGEN ERROR: Unknown node type %d\n", node->type);
  }
}

void CompileExpression(Compiler* compiler, Expression* expr) {
  if (expr == NULL) {
    return;
  }

  switch (expr->node.type) {
    case NODE_INTEGER_LITERAL: {
      IntegerLiteral* lit = (IntegerLiteral*)expr;
      int const_index = AddConstant(compiler->chunk, lit->value);
      WriteChunk(compiler->chunk, OP_CONSTANT);
      WriteChunk(compiler->chunk, (uint8_t)const_index);
      break;
    }
    case NODE_INFIX_EXPRESSION: {
      InfixExpression* infix = (InfixExpression*)expr;
      CompileExpression(compiler, infix->left);
      CompileExpression(compiler, infix->right);

      if (strcmp(infix->operator, "+") == 0) {
        WriteChunk(compiler->chunk, OP_ADD);
      } else if (strcmp(infix->operator, "-") == 0) {
        WriteChunk(compiler->chunk, OP_SUBTRACT);
      } else if (strcmp(infix->operator, "*") == 0) {
        WriteChunk(compiler->chunk, OP_MULTIPLY);
      } else if (strcmp(infix->operator, "/") == 0) {
        WriteChunk(compiler->chunk, OP_DIVIDE);
      }
      break;
    }
  }
}

void CompileStatement(Compiler* compiler, Statement* stmt) {
  if (stmt == NULL) {
    return;
  }

  switch (stmt->node.type) {
    case NODE_EXPRESSION_STATEMENT: {
      ExpressionStatement* expr_stmt = (ExpressionStatement*)stmt;
      CompileExpression(compiler, expr_stmt->expression);
      WriteChunk(compiler->chunk, OP_POP);
      break;
    }
    case NODE_OUT_STATEMENT: {
      OutStatement* out_stmt = (OutStatement*)stmt;
      CompileExpression(compiler, out_stmt->value);
      WriteChunk(compiler->chunk, OP_OUT);
      break;
    }
  }
}
