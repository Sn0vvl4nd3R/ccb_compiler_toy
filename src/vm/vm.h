#ifndef VM_H
#define VM_H

#include "../common/bytecode.h"

#define STACK_MAX 256
#define GLOBALS_MAX 256
#define CALLSTACK_MAX 256

typedef struct {
  uint8_t* ret_ip;
  Value* base;
} CallFrame;

typedef struct {
  Chunk* chunk;
  uint8_t* ip;

  Value stack[STACK_MAX];
  Value* stack_top;

  Value globals[GLOBALS_MAX];

  CallFrame frames[CALLSTACK_MAX];
  int calltop;
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

