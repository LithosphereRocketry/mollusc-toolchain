#ifndef ASM_PARSE_H
#define ASM_PARSE_H

#include <stddef.h>
#include "structures.h"
#include "arch.h"

struct parse_instr {
    const char* file;
    size_t line;
    enum arch_instr type;
    struct heap_list args;
};

struct parse_section {
    const char* name;
    struct heap_list globals; // char*
    struct heap_list instrs; // struct parse_instr*
    struct string_map instr_labels; // size_t + 1
};

struct parse_result {
    struct heap_list filenames; // used to manage lifetime of filename strings
    struct string_map sections;
};

struct parse_result asm_parse(const char* text, const char* filename);

void print_section(const struct parse_section* sect);
void print_parse(const struct parse_result* res);

#endif