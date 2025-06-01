#ifndef ASM_PARSE_H
#define ASM_PARSE_H

#include <stddef.h>
#include "structures.h"

struct parse_section {
    const char* name;
    struct heap_list globals;
};

struct parse_result {
    struct string_map sections;
};

struct parse_result asm_parse(const char* text, const char* filename);

void print_section(const struct parse_section* sect);
void print_parse(const struct parse_result* res);

#endif