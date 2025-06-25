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
    uint8_t instruction = *vm.ip++;
    switch (instruction) {
      case OP_CONSTANT: {
        uint8_t i = *vm.ip++;
        Push(vm.chunk->constants[i]);
        break;
      }
      case OP_POP:
        Pop();
        break;
      case OP_DEFINE_GLOBAL: {
        uint8_t i = *vm.ip++;
        vm.globals[i] = Pop();
        break;
      }
      case OP_GET_GLOBAL: {
        uint8_t i = *vm.ip++;
        Push(vm.globals[i]);
        break;
      }
      case OP_SET_GLOBAL: {
        uint8_t i = *vm.ip++;
        vm.globals[i] = vm.stack_top[-1];
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
      case OP_LESS: {
        Value b = Pop();
        Value a = Pop();
        Push(a < b);
        break;
      }
      case OP_GREATER: {
        Value b = Pop();
        Value a = Pop();
        Push(a > b);
        break;
      }
      
      case OP_JUMP: {
        uint16_t offset = (uint16_t)(vm.ip[0] << 8) | vm.ip[1];
        vm.ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = (uint16_t)(vm.ip[0] << 8) | vm.ip[1];
        if (vm.stack_top[-1] == 0) {
          vm.ip += offset;
        } else {
          vm.ip += 2;
        }
        Pop();
        break;
      }
      case OP_LOOP: {
        uint16_t offset = (uint16_t)(vm.ip[0] << 8) | vm.ip[1];
        vm.ip -= offset;
        break;
      }
      
      case OP_IN: {
        uint8_t i = *vm.ip++; int val;
        if (scanf("%d", &val) != 1) {
          val = 0;
          while (getchar() != '\n');
        }
        vm.globals[i] = val;
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
    free(l);
    free(p);
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
