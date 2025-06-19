#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>

typedef enum {
  OP_CONSTANT,
  OP_POP,

  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,

  OP_OUT,
  OP_RETURN,
} OpCode;

typedef struct {
  int count;
  int capacity;
  uint8_t* code;

  int* constants;
  int constants_count;
  int constants_capacity;
} Chunk;

void InitChunk(Chunk* chunk);
void WriteChunk(Chunk* chunk, uint8_t byte);
void FreeChunk(Chunk* chunk);
int AddConstant(Chunk* chucnk, int value);

#endif
