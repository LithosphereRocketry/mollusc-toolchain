#include "arch.h"

const char* arch_mnemonics[] = {
    [I_J] =     "j",
    [I_LUI] =   "lui",
    [I_AUIPC] = "auipc",
    [I_ADD] =   "add",
    [I_SUB] =   "sub",
    [I_AND] =   "and",
    [I_OR] =    "or",
    [I_XOR] =   "xor",
    [I_SL] =    "sl",
    [I_SR] =    "sr",
    [I_SRA] =   "sra",
    [I_LTU] =   "ltu",
    [I_LT] =    "lt",
    [I_EQ] =    "eq",
    [I_BIT] =   "bit",
    [I_LDP] =   "ldp",
    [I_STP] =   "stp",
    [I_JX] =    "jx",
};

const arch_word_t arch_basebits[] = {
    [I_J] =     0x00400000,
    [I_LUI] =   0x00800000,
    [I_AUIPC] = 0x00C00000,
    [I_ADD] =   0x00000000,
    [I_SUB] =   0x00010000,
    [I_AND] =   0x00020000,
    [I_OR] =    0x00030000,
    [I_XOR] =   0x00040000,
    [I_SL] =    0x00050000,
    [I_SR] =    0x00060000,
    [I_SRA] =   0x00070000,
    [I_LTU] =   0x00200000,
    [I_LT] =    0x00210000,
    [I_EQ] =    0x00220000,
    [I_BIT] =   0x00230000,
    [I_LDP] =   0x00300000,
    [I_STP] =   0x00300800,
    [I_JX] =    0x00200800,
};

const char* arch_prednames[] = {
    [P_P0] = "p0",
    [P_P1] = "p1",
    [P_P2] = "p2",
    [P_P3] = "p3",
    [P_P4] = "p4",
    [P_P5] = "p5",
    [P_P6] = "p6",
    [P_P7] = "p7"
};

const char* arch_regnames[] = {
    [R_ZERO] = "zero",
    [R_RA] = "ra",
    [R_SP] = "sp",
    [R_S2] = "s2",
    [R_S3] = "s3",
    [R_S4] = "s4",
    [R_S5] = "s5",
    [R_A0] = "a0",
    [R_A1] = "a1",
    [R_A2] = "a2",
    [R_A3] = "a3",
    [R_A4] = "a4",
    [R_A5] = "a5",
    [R_A6] = "a6",
    [R_T0] = "t0",
    [R_T1] = "t1"
};

const char* arch_relocnames[] = {
    [RELOC_J_REL] = "RELOC_J_REL"
};