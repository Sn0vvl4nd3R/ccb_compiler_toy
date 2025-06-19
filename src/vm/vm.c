#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "../parser/parser.h"
#include "../codegen/codegen.h"

VM vm;

void Push(Value value) {
  *vm.stack_top = value;
  vm.stack_top++;
}

Value Pop() {
  vm.stack_top--;
  return *vm.stack_top;
}

void InitVM() {
  vm.stack_top = vm.stack;
  memset(vm.globals, 0, sizeof(vm.globals));
}

void FreeVM() {}

static InterpretResult Run() {
  for (;;) {
    printf("    ");
    for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
      printf("[ %d ]", *slot);
    }
    printf("\n");

    uint8_t instruction = *vm.ip++;
    switch (instruction) {
      case OP_CONSTANT: {
        uint8_t const_index = *vm.ip++;
        Value constant = vm.chunk->constants[const_index];
        Push(constant);
        break;
      }
      case OP_POP: {
        Pop();
        break;
      }
      case OP_DEFINE_GLOBAL: {
        uint8_t global_index = *vm.ip++;
        vm.globals[global_index] = Pop();
        break;
      }
      case OP_GET_GLOBAL: {
        uint8_t global_index = *vm.ip++;
        Push(vm.globals[global_index]);
        break;
      }
      case OP_ADD: {
        Value b = Pop();
        Value a = Pop();
        Push(a + b);
        break;
      }
      case OP_SUBTRACT: {
        Value b = Pop();
        Value a = Pop();
        Push(a - b);
        break;
      }
      case OP_MULTIPLY: {
        Value b = Pop();
        Value a = Pop();
        Push(a * b);
        break;
      }
      case OP_DIVIDE: {
        Value b = Pop();
        Value a = Pop();
        Push(a / b);
        break;
      }
      
      case OP_OUT: {
        printf("%d\n", Pop());
        break;
      }
      case OP_RETURN: {
        return INTERPRET_OK;
      }
      default:
        printf("Unknown opcode %d\n", instruction);
        return INTERPRET_RUNTIME_ERROR;
    }
  }
}

InterpretResult Interpret(const char* source) {
  Lexer* l = NewLexer(source);
  Parser* p = NewParser(l);
  Program* program = ParseProgram(p);

  if (program == NULL) {
    free(l);
    free(p);
    return INTERPRET_COMPILE_ERROR;
  }

  Chunk* chunk = Compile(program);
  if (chunk == NULL) {
    return INTERPRET_COMPILE_ERROR;
  }

  InitVM();
  vm.chunk = chunk;
  vm.ip = chunk->code;

  InterpretResult result = Run();

  FreeChunk(chunk);
  free(chunk);
  free(l);
  free(p);
  return result;
}
