#ifndef ARCH_H
#define ARCH_H

#include <stddef.h>
#include <stdint.h>

typedef uint32_t arch_word_t;

enum arch_instr {
    I_INVALID = (size_t) NULL, // make sure (int) nullptr always maps to an error
    I_ADD,
    N_INSTRS
};

enum arch_register {
    R_ZERO,
    R_RA,
    R_SP,
    R_S2,
    R_S3,
    R_S4,
    R_S5,
    R_A0,
    R_A1,
    R_A2,
    R_A3,
    R_A4,
    R_A5,
    R_A6,
    R_T0,
    R_T1
};

extern const char* arch_mnemonics[N_INSTRS];
extern const arch_word_t arch_basebits[N_INSTRS];
// TODO: support more than one word per instr

extern const char* arch_regnames[];

#endif