#ifndef ARCH_DISASM_H
#define ARCH_DISASM_H

#include "arch.h"

enum arch_instr arch_identify(const arch_word_t* instr);

#endif