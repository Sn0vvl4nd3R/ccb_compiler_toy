#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>

typedef enum {
  OP_CONSTANT,
  OP_POP,

  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,

  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,

  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,

  OP_LESS,
  OP_GREATER,

  OP_OUT,
  OP_RETURN,
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
