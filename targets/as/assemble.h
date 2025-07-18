#ifndef ASSEMBLE_H
#define ASSEMBLE_H

#include "asm_parse.h"
#include "arch.h"

enum reloc_type {
    RELOC_J_REL
};

struct relocation {
    enum reloc_type type;
    const char* symbol;
    size_t offset;
};

struct assembly_result {
    struct heap_list relocations; // struct relocation*
    struct string_map relative_syms;
    struct string_map absolute_syms;
    size_t data_sz;
    arch_word_t* data;
};

typedef bool (*assemble_arg_t)(struct assembly_result* res, size_t offs, const char* arg);

struct assembly_result assemble(const struct parse_section* parse);

void print_assembly(const struct assembly_result* res);

void destroy_assembly(struct assembly_result* res);

#endif