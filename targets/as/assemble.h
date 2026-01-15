#ifndef ASSEMBLE_H
#define ASSEMBLE_H

#include "asm_parse.h"
#include "arch.h"

typedef bool (*assemble_arg_t)(struct bin_section* res, size_t offs, const char* arg);
typedef bool (*assemble_mode_t)(struct bin_section* res, size_t offs, const char* arg);


struct bin_section assemble(const struct parse_section* parse);

void print_assembly(const struct bin_section* res);

void destroy_assembly(struct bin_section* res);

#endif