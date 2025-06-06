#ifndef ASM_INSTRS_H
#define ASM_INSTRS_H

#include "asm_parse.h"

const char* asm_parse_instr(const char* file, size_t line,
        const char* text, struct parse_section* dest);

#endif