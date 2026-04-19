#ifndef ASSEMBLE_H
#define ASSEMBLE_H

#include "asm_parse.h"
#include "arch.h"

typedef bool (*assemble_arg_t)(struct bin_section* res, size_t offs, const char* arg);
typedef bool (*assemble_mode_t)(struct bin_section* res, size_t offs, const char* arg);

struct asm_result assemble(const struct parse_result* parse);

struct bin_section assemble_section(struct string_map* labels, const struct parse_section* parse);

#endif