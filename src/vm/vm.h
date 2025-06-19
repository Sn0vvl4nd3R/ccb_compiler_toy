#ifndef VM_H
#define VM_H

#include "../common/bytecode.h"

#define STACK_MAX 256

typedef struct {
  Chunk* chunk;
  uint8_t* ip;

  int stack[STACK_MAX];
  int* stack_top;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void InitVM(VM* vm);
void FreeVM(VM* vm);
InterpretResult Interpret(const char* source);

#endif
