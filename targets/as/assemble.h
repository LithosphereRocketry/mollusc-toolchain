#ifndef ASSEMBLE_H
#define ASSEMBLE_H

#include "asm_parse.h"

enum reloc_type {
    REL_WORD_ABSOLUTE,
    REL_WORD_RELATIVE,
    REL_IMM_ABSOLUTE,
    REL_IMM_RELATIVE,
    REL_UPPER_ABSOLUTE,
    REL_UPPER_RELATIVE,
    REL_JUMP_ABSOLUTE,
    REL_JUMP_RELATIVE
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
    uint32_t* data;
};

struct assembly_result assemble(const struct parse_section* parse);

#endif