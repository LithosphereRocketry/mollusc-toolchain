#include "assemble.h"

#include <stdio.h>
#include <stdlib.h>

struct assembly_result assemble(const struct parse_section* parse) {
    size_t data_cap = parse->instrs.len;
    struct assembly_result res = {
        .relocations = hl_make(),
        .absolute_syms = sm_make(),
        .relative_syms = sm_make(),
        .data = malloc(data_cap)
    };

    size_t offset_word = 0;
    for(size_t i = 0; i < parse->instrs.len; i++) {
        struct parse_instr* instr = parse->instrs.buf[i];
        printf("Error in %s, line %zu: help I'm lost\n", instr->file, instr->line);
        exit(-1);
    }

    return res;
}