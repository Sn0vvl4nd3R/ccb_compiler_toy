#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

typedef struct {
  char* name;
  int index;
} Local;

typedef struct {
  Chunk* chunk;
  char* strings[256];
  int string_count;

  char* fn_names[256];
  int fn_offsets[256];
  int fn_count;

  struct {
    char* name;
    int patch_pos;
  } unresolved[512];
  int unresolved_count;

  int in_function;
  Local locals[256];
  int param_count;
  int local_count;
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

static int FindFunction(Compiler* c, const char* name) {
  for (int i = 0; i < c->fn_count; i++) {
    if (strcmp(c->fn_names[i], name) == 0) {
      return c->fn_offsets[i];
    }
  }
  return -1;
}

static void RegisterFunction(Compiler* c, const char* name, int offset) {
  for (int i = 0; i < c->fn_count; i++) {
    if (strcmp(c->fn_names[i], name) == 0) {
      c->fn_offsets[i] = offset;
      return;
    }
  }
  if (c->fn_count >= 256) {
    printf("Too many functions.\n");
    exit(1);
  }
  c->fn_names[c->fn_count] = strdup(name);
  c->fn_offsets[c->fn_count] = offset;
  c->fn_count++;
}

static void AddUnresolved(Compiler* c, const char* name, int patch_pos) {
  if (c->unresolved_count >= 512) {
    printf("Too many unresolved calls.\n");
    exit(1);
  }
  c->unresolved[c->unresolved_count].name = strdup(name);
  c->unresolved[c->unresolved_count].patch_pos = patch_pos;
  c->unresolved_count++;
}

static void PatchUnresolved(Compiler* c) {
  for (int i = 0; i < c->unresolved_count; i++) {
    int off = FindFunction(c, c->unresolved[i].name);
    if (off < 0) {
      printf("CODEGEN ERROR: Undefined function '%s'\n", c->unresolved[i].name);
      exit(1);
    }
    c->chunk->code[c->unresolved[i].patch_pos] = (off >> 8) & 0xff;
    c->chunk->code[c->unresolved[i].patch_pos + 1] = off & 0xff;
    free(c->unresolved[i].name);
  }
  c->unresolved_count = 0;
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
  compiler->strings[compiler->string_count] = strdup(name);
  return (uint8_t)compiler->string_count++;
}

int EmitJump(Compiler* compiler, uint8_t instruction) {
  WriteChunk(compiler->chunk, instruction);
  WriteChunk(compiler->chunk, 0xff);
  WriteChunk(compiler->chunk, 0xff);
  return compiler->chunk->count - 2;
}

void PatchJump(Compiler* compiler, int offset_pos) {
  int jump = compiler->chunk->count - offset_pos;
  if (jump > UINT16_MAX) {
    printf("ERROR: Too much code to jump over.\n");
    exit(1);
  }
  compiler->chunk->code[offset_pos] = (jump >> 8) & 0xff;
  compiler->chunk->code[offset_pos + 1] = jump & 0xff;
}

void EmitLoop(Compiler* compiler, int loop_start) {
  WriteChunk(compiler->chunk, OP_LOOP);
  int offset = compiler->chunk->count - loop_start;
  if (offset > UINT16_MAX) {
    printf("ERROR: Loop body too large.\n");
    exit(1);
  }
  WriteChunk(compiler->chunk, (offset >> 8) & 0xff);
  WriteChunk(compiler->chunk, offset & 0xff);
}

static int FindLocal(Compiler* c, const char* name) {
  if (!c->in_function) {
    return -1;
  }
  int total = c->param_count + c->local_count;
  for (int i = 0; i < total; i++) {
    if (strcmp(c->locals[i].name, name) == 0) {
      return c->locals[i].index;
    }
  }
  return -1;
}

Chunk* Compile(Program* program) {
  Compiler compiler;
  compiler.string_count = 0;
  compiler.fn_count = 0;
  compiler.unresolved_count = 0;
  compiler.in_function = 0;
  compiler.param_count = 0;
  compiler.local_count = 0;

  Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
  InitChunk(chunk);
  compiler.chunk = chunk;

  for (int i = 0; i < program->statement_count; i++) {
    CompileNode(&compiler, (Node*)program->statements[i]);
  }

  PatchUnresolved(&compiler);

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
    case NODE_CALL_EXPRESSION:
      CompileExpression(compiler, (Expression*)node);
      break;

    case NODE_LET_STATEMENT:
    case NODE_EXPRESSION_STATEMENT:
    case NODE_OUT_STATEMENT:
    case NODE_BLOCK_STATEMENT:
    case NODE_WHILE_STATEMENT:
    case NODE_IN_STATEMENT:
    case NODE_FUNCTION_STATEMENT:
    case NODE_RETURN_STATEMENT:
      CompileStatement(compiler, (Statement*)node);
      break;

    default:
      printf("CODEGEN ERROR: Unknown node type %d\n", node->type);
      break;
  }
}

static void EnsureFunctionReturn(Compiler* c) {
  int const_index = AddConstant(c->chunk, 0);
  WriteChunk(c->chunk, OP_CONSTANT);
  WriteChunk(c->chunk, (uint8_t)const_index);
  WriteChunk(c->chunk, OP_RETURN);
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
    case NODE_IDENTIFIER: {
      Identifier* ident = (Identifier*)expr;
      int li = FindLocal(compiler, ident->value);
      if (li >= 0) {
        WriteChunk(compiler->chunk, OP_GET_LOCAL);
        WriteChunk(compiler->chunk, (uint8_t)li);
      } else {
        uint8_t arg = IdentifierConstant(compiler, ident->value);
        WriteChunk(compiler->chunk, OP_GET_GLOBAL);
        WriteChunk(compiler->chunk, arg);
      }
      break;
    }
    case NODE_INFIX_EXPRESSION: {
      InfixExpression* infix = (InfixExpression*)expr;
      if (strcmp(infix->operator, "=") == 0) {
        CompileExpression(compiler, infix->right);
        Identifier* ident = (Identifier*)infix->left;
        int li = FindLocal(compiler, ident->value);
        if (li >= 0) {
          WriteChunk(compiler->chunk, OP_SET_LOCAL);
          WriteChunk(compiler->chunk, (uint8_t)li);
        } else {
          uint8_t arg = IdentifierConstant(compiler, ident->value);
          WriteChunk(compiler->chunk, OP_SET_GLOBAL);
          WriteChunk(compiler->chunk, arg);
        }
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
        } else if (strcmp(infix->operator, "<=") == 0) {
          WriteChunk(compiler->chunk, OP_LESS_EQUAL);
        } else if (strcmp(infix->operator, ">=") == 0) {
          WriteChunk(compiler->chunk, OP_GREATER_EQUAL);
        } else if (strcmp(infix->operator, "==") == 0) { 
          WriteChunk(compiler->chunk, OP_EQUAL);
        } else if (strcmp(infix->operator, "!=") == 0) {
          WriteChunk(compiler->chunk, OP_NOT_EQUAL);
        }
      }
      break;
    }
    case NODE_IF_EXPRESSION: {
      IfExpression* if_exp = (IfExpression*)expr;
      CompileExpression(compiler, if_exp->condition);
      int then_jump = EmitJump(compiler, OP_JUMP_IF_FALSE);
      CompileStatement(compiler, (Statement*)if_exp->consequence);
      int else_jump = EmitJump(compiler, OP_JUMP);
      PatchJump(compiler, then_jump);
      if (if_exp->alternative != NULL) {
        CompileStatement(compiler, (Statement*)if_exp->alternative);
      }
      PatchJump(compiler, else_jump);
      break;
    }
    case NODE_CALL_EXPRESSION: {
      CallExpression* call = (CallExpression*)expr;
      for (int i = 0; i < call->arg_count; i++) {
        CompileExpression(compiler, call->arguments[i]);
      }
      if (call->function->node.type != NODE_IDENTIFIER) {
        printf("CODEGEN ERROR: call target must be an identifier.\n");
        exit(1);
      }
      Identifier* ident = (Identifier*)call->function;
      int off = FindFunction(compiler, ident->value);
      WriteChunk(compiler->chunk, OP_CALL);
      int pos = compiler->chunk->count;
      if (off < 0) {
        WriteChunk(compiler->chunk, 0xff);
        WriteChunk(compiler->chunk, 0xff);
        AddUnresolved(compiler, ident->value, pos);
      } else {
        WriteChunk(compiler->chunk, (off >> 8) & 0xff);
        WriteChunk(compiler->chunk, off & 0xff);
      }
      WriteChunk(compiler->chunk, (uint8_t)call->arg_count);
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
    case NODE_LET_STATEMENT: {
      LetStatement* let_stmt = (LetStatement*)stmt;
      if (compiler->in_function) {
        CompileExpression(compiler, let_stmt->value);
        int idx = compiler->param_count + compiler->local_count;
        if (idx >= 256) {
          printf("Too many locals.\n");
          exit(1);
        }
        compiler->locals[idx].name = strdup(let_stmt->name->value);
        compiler->locals[idx].index = idx;
        compiler->local_count++;
      } else {
        CompileExpression(compiler, let_stmt->value);
        uint8_t arg = IdentifierConstant(compiler, let_stmt->name->value);
        WriteChunk(compiler->chunk, OP_DEFINE_GLOBAL);
        WriteChunk(compiler->chunk, arg);
      }
      break;
    }
    case NODE_EXPRESSION_STATEMENT: {
      ExpressionStatement* es = (ExpressionStatement*)stmt;
      CompileExpression(compiler, es->expression);
      if (es->expression->node.type != NODE_IF_EXPRESSION &&
          es->expression->node.type != NODE_CALL_EXPRESSION) {
        WriteChunk(compiler->chunk, OP_POP);
      }
      if (es->expression->node.type == NODE_CALL_EXPRESSION) {
        WriteChunk(compiler->chunk, OP_POP);
      }
      break;
    }
    case NODE_OUT_STATEMENT: {
      OutStatement* out_stmt = (OutStatement*)stmt;
      CompileExpression(compiler, out_stmt->value);
      WriteChunk(compiler->chunk, OP_OUT);
      break;
    }
    case NODE_IN_STATEMENT: {
      InStatement* in_stmt = (InStatement*)stmt;
      if (compiler->in_function) {
        int li = FindLocal(compiler, in_stmt->name->value);
        if (li < 0) {
          printf("CODEGEN ERROR: input to undeclared local '%s'\n", in_stmt->name->value);
          exit(1);
        }
        WriteChunk(compiler->chunk, OP_IN_LOCAL);
        WriteChunk(compiler->chunk, (uint8_t)li);
      } else {
        uint8_t arg = IdentifierConstant(compiler, in_stmt->name->value);
        WriteChunk(compiler->chunk, OP_IN);
        WriteChunk(compiler->chunk, arg);
      }
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
    case NODE_FUNCTION_STATEMENT: {
      FunctionStatement* fn = (FunctionStatement*)stmt;
      int skip = EmitJump(compiler, OP_JUMP);

      int entry = compiler->chunk->count;
      RegisterFunction(compiler, fn->name->value, entry);

      int saved_in_function = compiler->in_function;
      int saved_param_count = compiler->param_count;
      int saved_local_count = compiler->local_count;
      Local saved_locals[256];
      int saved_total = saved_param_count + saved_local_count;
      for (int i = 0; i < saved_total; i++) {
        saved_locals[i] = compiler->locals[i];
      }

      compiler->in_function = 1;
      compiler->param_count = fn->param_count;
      compiler->local_count = 0;
      for (int i = 0; i < fn->param_count; i++) {
        compiler->locals[i].name = strdup(fn->params[i]->value);
        compiler->locals[i].index = i;
      }

      CompileStatement(compiler, (Statement*)fn->body);
      EnsureFunctionReturn(compiler);

      for (int i = 0; i < compiler->param_count + compiler->local_count; i++) {
        free(compiler->locals[i].name);
      }
      compiler->in_function = saved_in_function;
      compiler->param_count = saved_param_count;
      compiler->local_count = saved_local_count;
      for (int i = 0; i < saved_param_count + saved_local_count; i++) {
        compiler->locals[i] = saved_locals[i];
      }

      PatchJump(compiler, skip);
      break;
    }
    case NODE_RETURN_STATEMENT: {
      ReturnStatement* rs = (ReturnStatement*)stmt;
      if (rs->value) {
        CompileExpression(compiler, rs->value);
      } else {
        int ci = AddConstant(compiler->chunk, 0);
        WriteChunk(compiler->chunk, OP_CONSTANT);
        WriteChunk(compiler->chunk, (uint8_t)ci);
      }
      WriteChunk(compiler->chunk, OP_RETURN);
      break;
    }
    default: break;
  }
}

