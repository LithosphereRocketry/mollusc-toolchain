#include "arch.h"

const char* arch_mnemonics[] = {
    [I_ADD] = "add"
};

const arch_word_t arch_basebits[] = {
    [I_ADD] = 0x00000000
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