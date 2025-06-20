#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

typedef struct {
  Chunk* chunk;
  char* strings[256];
  int string_count;
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

int AddConstant(Chunk* chunk, Value value) {
  if (chunk->constants_capacity < chunk->constants_count + 1) {
    int old_capacity = chunk->constants_capacity;
    chunk->constants_capacity = old_capacity < 8 ? 8 : old_capacity * 2;
    chunk->constants = realloc(chunk->constants, chunk->constants_capacity * sizeof(Value));
  }
  chunk->constants[chunk->constants_count] = value;
  return chunk->constants_count++;
}

uint8_t IdentifierConstant(Compiler* compiler, const char* name) {
  for (int i = 0; i < compiler->string_count; i++) {
    if (strcmp(compiler->strings[i], name) == 0) {
      return (uint8_t)i;
    }
  }
  if (compiler->string_count == 256) {
    printf("Too many string constants.\n");
    exit(1);
  }
  compiler->strings[compiler->string_count] = (char*)name;
  return (uint8_t)compiler->string_count++;
}

int EmitJump(Compiler* compiler, uint8_t instruction) {
  WriteChunk(compiler->chunk, instruction);
  WriteChunk(compiler->chunk, 0xff);
  WriteChunk(compiler->chunk, 0xff);
  return compiler->chunk->count - 2;
}

int EmitLoop(Compiler* compiler, int loop_start) {
  WriteChunk(compiler->chunk, OP_LOOP);
  WriteChunk(compiler->chunk, (loop_start >> 8) & 0xff);
  WriteChunk(compiler->chunk, loop_start & 0xff);
}

void PatchJump(Compiler* compiler, int offset) {
  int jump = compiler->chunk->count - offset - 2;
  if (jump > UINT16_MAX) {
    printf("ERROR: Too much code to jump over.\n");
  }
  compiler->chunk->code[offset] = (jump >> 8) & 0xff;
  compiler->chunk->code[offset + 1] = jump & 0xff;
}

Chunk* Compile(Program* program) {
  Compiler compiler;
  compiler.string_count = 0;
  
  Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
  InitChunk(chunk);
  compiler.chunk = chunk;

  for (int i = 0; i < program->statement_count; i++) {
    CompileNode(&compiler, (Node*)program->statements[i]);
  }

  WriteChunk(compiler.chunk, OP_RETURN);
  return compiler.chunk;
}

void CompileNode(Compiler* compiler, Node* node) {
  if (node == NULL) {
    return;
  }

  switch (node->type) {
    case NODE_IDENTIFIER:
    case NODE_INTEGER_LITERAL:
    case NODE_INFIX_EXPRESSION:
    case NODE_IF_EXPRESSION:
      CompileExpression(compiler, (Expression*)node);
      break;
    case NODE_LET_STATEMENT:
    case NODE_EXPRESSION_STATEMENT:
    case NODE_OUT_STATEMENT:
    case NODE_BLOCK_STATEMENT:
    case NODE_WHILE_STATEMENT:
      CompileStatement(compiler, (Statement*)node);
      break;
    default:
      printf("CODEGEN ERROR: Unknown node type %d\n", node->type);
      break;
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

      if (strcmp(infix->operator, "=") == 0) {
        CompileExpression(compiler, infix->right);
        Identifier* ident = (Identifier*)infix->left;
        uint8_t arg = IdentifierConstant(compiler, ident->value);
        WriteChunk(compiler->chunk, OP_SET_GLOBAL);
        WriteChunk(compiler->chunk, arg);
      } else {
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
        } else if (strcmp(infix->operator, "<") == 0) {
          WriteChunk(compiler->chunk, OP_LESS);
        } else if (strcmp(infix->operator, ">") == 0) {
          WriteChunk(compiler->chunk, OP_GREATER);
        }
      }
      break;
    }
    case NODE_IDENTIFIER: {
      Identifier* ident = (Identifier*)expr;
      uint8_t arg = IdentifierConstant(compiler, ident->value);
      WriteChunk(compiler->chunk, OP_GET_GLOBAL);
      WriteChunk(compiler->chunk, arg);
      break;
    }
    case NODE_IF_EXPRESSION: {
      IfExpression* if_exp = (IfExpression*)expr;
      CompileExpression(compiler, if_exp->condition);
      int then_jump = EmitJump(compiler, OP_JUMP_IF_FALSE);
      WriteChunk(compiler->chunk, OP_POP);
      CompileStatement(compiler, (Statement*)if_exp->consequence);
      int else_jump = EmitJump(compiler, OP_JUMP);
      PatchJump(compiler, then_jump);
      WriteChunk(compiler->chunk, OP_POP);
      if (if_exp->alternative != NULL) {
        CompileStatement(compiler, (Statement*)if_exp->alternative);
      }
      PatchJump(compiler, else_jump);
      break;
    }
    default: break;
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
    case NODE_LET_STATEMENT: {
      LetStatement* let_stmt = (LetStatement*)stmt;
      CompileExpression(compiler, let_stmt->value);
      uint8_t arg = IdentifierConstant(compiler, let_stmt->name->value);
      WriteChunk(compiler->chunk, OP_DEFINE_GLOBAL);
      WriteChunk(compiler->chunk, arg);
      break;
    }
    case NODE_BLOCK_STATEMENT: {
      BlockStatement* block = (BlockStatement*)stmt;
      for (int i = 0; i < block->statement_count; i++) {
        CompileNode(compiler, (Node*)block->statements[i]);
      }
      break;
    }
    case NODE_WHILE_STATEMENT: {
      WhileStatement* while_stmt = (WhileStatement*)stmt;
      int loop_start = compiler->chunk->count;
      CompileExpression(compiler, while_stmt->condition);
      int exit_jump = EmitJump(compiler, OP_JUMP_IF_FALSE);
      CompileStatement(compiler, (Statement*)while_stmt->body);
      EmitLoop(compiler, loop_start);
      PatchJump(compiler, exit_jump);
      break;
    }
    default: break;
  }
}
