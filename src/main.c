#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm/vm.h"
#include "common/token.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

static char* ReadFile(const char* path) {
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    return NULL;
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char* buffer = (char*)malloc(file_size + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    fclose(file);
    return NULL;
  }

  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    free(buffer);
    fclose(file);
    return NULL;
  }

  buffer[bytes_read] = '\0';
  fclose(file);
  return buffer;
}

static int HasCcbExtension(const char* path) {
  const char* dot = strrchr(path, '.');
  if (dot == NULL) {
    return 0;
  }
  return strcasecmp(dot, ".ccb") == 0;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s [path]\n", argv[0]);
    return 1;
  }

  if (!HasCcbExtension(argv[1])) {
    fprintf(stderr, "ERROR: \"%s\" has unsupported extension (expected .ccb).\n", argv[1]);
    return 1;
  }

  char* source = ReadFile(argv[1]);
  if (source == NULL) {
    return 1;
  }

  printf("--- Compiling and running %s ---\n", argv[1]);
  InterpretResult result = Interpret(source);

  free(source);

  if (result == INTERPRET_COMPILE_ERROR) {
    return 65;
  }
  if (result == INTERPRET_RUNTIME_ERROR) {
    return 70;
  }

  return 0;
}
