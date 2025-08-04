#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "../common/ast.h"
#include "../parser/parser.h"
#include "../codegen/codegen.h"

VM vm;

static inline void Push(Value value) {
  if (vm.stack_top - vm.stack >= STACK_MAX) {
    fprintf(stderr, "RUNTIME ERROR: stack overflow.\n");
    exit(1);
  }
  *vm.stack_top = value;
  vm.stack_top++;
}

static inline Value Pop() {
  vm.stack_top--;
  return *vm.stack_top;
}

void InitVM() {
  vm.stack_top = vm.stack;
  memset(vm.globals, 0, sizeof(vm.globals));
  vm.calltop = 0;
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
      case OP_POP: {
        Pop();
        break;
      }
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

      case OP_GET_LOCAL: {
        uint8_t i = *vm.ip++;
        CallFrame* fr = &vm.frames[vm.calltop - 1];
        Push(fr->base[i]);
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t i = *vm.ip++;
        CallFrame* fr = &vm.frames[vm.calltop - 1];
        fr->base[i] = vm.stack_top[-1];
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
      case OP_LESS_EQUAL: {
        Value b = Pop();
        Value a = Pop();
        Push(a <= b);
        break;
      }
      case OP_GREATER_EQUAL: {
        Value b = Pop();
        Value a = Pop();
        Push(a >= b);
        break;
      }
      case OP_EQUAL: {
        Value b = Pop();
        Value a = Pop();
        Push(a == b);
        break;
      }
      case OP_NOT_EQUAL: {
        Value b = Pop();
        Value a = Pop();
        Push(a != b);
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
      case OP_IN_LOCAL: {
        uint8_t i = *vm.ip++; int val;
        if (scanf("%d", &val) != 1) {
          val = 0;
          while (getchar() != '\n');
        }
        CallFrame* fr = &vm.frames[vm.calltop - 1];
        fr->base[i] = val;
        break;
      }
      case OP_OUT: {
        printf("%d\n", Pop());
        break;
      }

      case OP_CALL: {
        uint16_t offset = (uint16_t)(vm.ip[0] << 8) | vm.ip[1];
        vm.ip += 2;
        uint8_t argc = *vm.ip++;

        if (vm.calltop >= CALLSTACK_MAX) {
          fprintf(stderr, "RUNTIME ERROR: call stack overflow.\n");
          return INTERPRET_RUNTIME_ERROR;
        }
        CallFrame* fr = &vm.frames[vm.calltop++];
        fr->ret_ip = vm.ip;
        fr->base = vm.stack_top - argc;
        vm.ip = vm.chunk->code + offset;
        break;
      }

      case OP_RETURN: {
        if (vm.calltop > 0) {
          Value ret = Pop();
          CallFrame* fr = &vm.frames[vm.calltop - 1];
          vm.stack_top = fr->base;
          vm.calltop--;
          Push(ret);
          vm.ip = fr->ret_ip;
          break;
        }
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
    free(p->ns_prefix);
    free(l);
    free(p);
    return INTERPRET_COMPILE_ERROR;
  }

  Chunk* chunk = Compile(program);
  if (chunk == NULL) {
    free(p->ns_prefix);
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
  FreeProgram(program);
  free(p->ns_prefix);
  free(l);
  free(p);
  return result;
}

