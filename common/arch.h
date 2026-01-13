#ifndef ARCH_H
#define ARCH_H

#include <stddef.h>
#include <stdint.h>

#include "structures.h"

typedef uint32_t arch_word_t;

enum arch_instr {
    I_INVALID = (size_t) NULL, // make sure (int) nullptr always maps to an error
    I_J,
    I_LUI,
    I_LUR,
    I_ADD,
    I_SUB,
    I_AND,
    I_OR,
    I_XOR,
    I_SL,
    I_SR,
    I_SRA,
    I_LTU,
    I_LT,
    I_EQ,
    I_BIT,
    I_LD,
    I_STP,
    I_JX,
    N_INSTRS
};

enum arch_predicate {
    P_P0,
    P_P1,
    P_P2,
    P_P3,
    P_P4,
    P_P5,
    P_P6,
    P_P7
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

enum reloc_type {
    RELOC_J_REL,
    RELOC_LUR_REL,
    RELOC_IMM_REL
};

struct relocation {
    enum reloc_type type;
    const char* symbol;
    size_t offset; // words
};

struct bin_section {
    struct heap_list relocations; // struct relocation*
    struct string_map relative_syms; // size_t, bytes
    struct string_map absolute_syms; // size_t, bytes
    size_t data_sz;
    arch_word_t* data;
};

extern const char* arch_mnemonics[N_INSTRS];
extern const arch_word_t arch_basebits[N_INSTRS];
// TODO: support more than one word per instr

extern const char* arch_regnames[];
extern const char* arch_prednames[];
extern const char* arch_relocnames[];

#endif