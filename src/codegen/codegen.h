#ifndef CODEGEN_H
#define CODEGEN_H

#include "../parser/parser.h"
#include "../common/bytecode.h"

Chunk* Compile(Program* program);

#endif
