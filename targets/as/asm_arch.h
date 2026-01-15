#ifndef ASM_ARCH_H
#define ASM_ARCH_H

#include "assemble.h"
#include "arch.h"

extern const assemble_arg_t* arch_arguments[N_INSTRS];
extern const assemble_mode_t arch_assemble_modes[N_INSTRS];

bool assemble_predicate(struct bin_section* res, size_t offs, const char* arg, bool nonzero);
bool assemble_rd(struct bin_section* res, size_t offs, const char* arg);
bool assemble_pd(struct bin_section* res, size_t offs, const char* arg);
bool assemble_ra(struct bin_section* res, size_t offs, const char* arg);
bool assemble_rbi(struct bin_section* res, size_t offs, const char* arg);
bool assemble_rm(struct bin_section* res, size_t offs, const char* arg);

#endif