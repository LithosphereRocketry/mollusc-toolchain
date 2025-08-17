#ifndef ASM_PSEUDO_H
#define ASM_PSEUDO_H

#include "arch.h"

// This enum serves as an extension of arch_instr to support built-in assembler
// macros
// Any valid arch_instr should be a valid arch_pseudoinstr; if both names match,
// the pseudoinstruction will be used to allow for the assembler to overload
// instructions for more convenient usage
// However, only arch_instr may be emitted in binary or will be considered in
// emulation
enum arch_pseudoinstr {
    PI_LA = N_INSTRS,
    N_PSEUDOINSTRS
};

extern const char* arch_pseudo_mnemonics[N_PSEUDOINSTRS];
// There's some wasted space here (N_INSTRS words at the beginning are blank)
// but ehhhh whatever

#endif