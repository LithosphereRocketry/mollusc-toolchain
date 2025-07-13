#ifndef ASM_ARCH_H
#define ASM_ARCH_H

#include "assemble.h"
#include "arch.h"

extern const assemble_arg_t* arch_arguments[N_INSTRS];

bool assemble_predicate(struct assembly_result* res, size_t offs, const char* arg, bool nonzero);

#endif