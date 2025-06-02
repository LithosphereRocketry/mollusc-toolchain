#ifndef ARCH_H
#define ARCH_H

#include <stddef.h>

enum arch_instr {
    I_INVALID = (size_t) NULL, // make sure (int) nullptr always maps to an error
    I_ADD,
    N_INSTRS
};

extern const char* arch_mnemonics[N_INSTRS];

#endif