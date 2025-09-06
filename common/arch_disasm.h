#ifndef ARCH_DISASM_H
#define ARCH_DISASM_H

#include "arch.h"

enum arch_instr arch_identify(const arch_word_t* instr);

char* arch_disasm(char* buf, size_t len, const arch_word_t* instr);

#endif