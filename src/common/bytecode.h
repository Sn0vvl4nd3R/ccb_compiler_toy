#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>

typedef enum {
  OP_CONSTANT,
  OP_POP,

  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,

  OP_GET_LOCAL,
  OP_SET_LOCAL,

  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,

  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,

  OP_LESS,
  OP_GREATER,
  OP_LESS_EQUAL,
  OP_GREATER_EQUAL,
  OP_EQUAL,
  OP_NOT_EQUAL,

  OP_IN,
  OP_IN_LOCAL,
  OP_OUT,

  OP_CALL,
  OP_RETURN
} OpCode;

typedef int Value;

typedef struct {
  int count;
  int capacity;
  uint8_t* code;

  Value* constants;
  int constants_count;
  int constants_capacity;
} Chunk;

void InitChunk(Chunk* chunk);
void WriteChunk(Chunk* chunk, uint8_t byte);
void FreeChunk(Chunk* chunk);
int AddConstant(Chunk* chucnk, Value value);

#endif

