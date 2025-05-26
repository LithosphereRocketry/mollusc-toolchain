#ifndef ASM_PARSE_H
#define ASM_PARSE_H

#include <stddef.h>
#include "structures.h"

struct parse_section {
    const char* name;
    struct heap_list* externs;
};

struct parse_result {
    size_t n_sections;
    struct parse_section* sections;
};

struct parse_result asm_parse(const char* text);

void print_section(const struct parse_section* sect);
void print_parse(const struct parse_result* res);

#endif