#ifndef VM_H
#define VM_H

#include "../common/bytecode.h"

#define STACK_MAX 256
#define GLOBALS_MAX 256

typedef struct {
  Chunk* chunk;
  uint8_t* ip;

  Value stack[STACK_MAX];
  Value* stack_top;

  Value globals[GLOBALS_MAX];
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void InitVM();
void FreeVM();
InterpretResult Interpret(const char* source);

#endif
