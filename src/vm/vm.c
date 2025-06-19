#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "../parser/parser.h"
#include "../codegen/codegen.h"

VM vm;

void Push(int value) {
  *vm.stack_top = value;
  vm.stack_top++;
}

int Pop() {
  vm.stack_top--;
  return *vm.stack_top;
}

void InitVM(VM* vm) {
  vm->stack_top = vm->stack;
}

void FreeVM(VM* vm) {}

static InterpretResult Run() {
  for (;;) {
    printf("   ");
    for (int* slot = vm.stack; slot < vm.stack_top; slot++) {
      printf("[ %d ]", *slot);
    }
    printf("\n");

    uint8_t instruction = *vm.ip++;
    switch (instruction) {
      case OP_CONSTANT: {
        uint8_t const_index = *vm.ip++;
        int constant = vm.chunk->constants[const_index];
        Push(constant);
        break;
      }
      case OP_ADD: {
        int b = Pop();
        int a = Pop();
        Push(a + b);
        break;
      }
      case OP_SUBTRACT: {
        int b = Pop();
        int a = Pop();
        Push(a - b);
        break;
      }
      case OP_MULTIPLY: {
        int b = Pop();
        int a = Pop();
        Push(a * b);
        break;
      }
      case OP_DIVIDE: {
        int b = Pop();
        int a = Pop();
        Push(a / b);
        break;
      }
      case OP_POP: {
        Pop();
        break;
      }
      
      case OP_OUT: {
        printf("Output: %d\n", Pop());
        break;
      }
      case OP_RETURN: {
        return INTERPRET_OK;
      }
    }
  }
}

InterpretResult Interpret(const char* source) {
  Lexer* l = NewLexer(source);
  Parser* p = NewParser(l);
  Program* program = ParseProgram(p);

  if (program == NULL) {
    return INTERPRET_COMPILE_ERROR;
  }

  Chunk* chunk = Compile(program);

  InitVM(&vm);
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;

  InterpretResult result = Run();

  FreeChunk(vm.chunk);
  free(chunk);
  return result;
}
