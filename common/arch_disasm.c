#include "arch_disasm.h"

#include <stdio.h>
#include <string.h>

#include "misctools.h"

// TODO: this tree is duplicated from the emulation step - should emulation use
// this or is it more valuable to dedup instruction logic within the emulator?
enum arch_instr arch_identify(const arch_word_t* location) {
    arch_word_t instr = *location;
    if((instr & 0x00C00000) == 0x00C00000) { // lur
        return I_LUR;
    } else if((instr & 0x00C00000) == 0x00800000) { // lui
        return I_LUI;
    } else if((instr & 0x00C00000) == 0x00400000) { // j
        return I_J;
    } else { // register instrs
        if((instr & 0x00300800) == 0x00300800) { // store
            int fcode = (instr >> 24) & 0xF;
            switch (fcode) {
                case 0x0: return I_STP;
                default: return I_INVALID;
            }
        } else if((instr & 0x00300800) == 0x00200000) { // comparison
            switch((instr >> 16) & 0xF) {
                case 0x0: return I_EQ;
                case 0x1: return I_LT;
                case 0x2: return I_EQ;
                case 0x3: return I_BIT;
                default: return I_INVALID;
            }
        } else {
            if((instr & 0x00300800) == 0x00200800) { // jx, misc
                int fcode = (instr >> 16) & 0xF;
                switch(fcode) {
                    case 0x0: return I_JX;
                    default: return I_INVALID;
                }
            } else if((instr & 0x00300800) == 0x00300000) { // loads
                int fcode = (instr >> 16) & 0xF;
                switch (fcode) {
                    case 0x0: return I_LDP;
                    default: return I_INVALID;
                }
            } else { // normal arithmetic
                switch(instr & 0x001F0800) {
                    case 0x00000000: return I_ADD;
                    case 0x00010000: return I_SUB;
                    case 0x00020000: return I_AND;
                    case 0x00060000: return I_SR;
                    default: return I_INVALID;
                }
            }
        }
    }
}

typedef char* (*disasm_t)(char*, size_t, const arch_word_t*);

static char* disasm_type_r(char* buf, size_t len, const arch_word_t* instr) {
    int chars;
    if(*instr & 0x400) {
        snprintf(buf, len, "%s %s, %s, %i%n", arch_mnemonics[arch_identify(instr)],
                arch_regnames[(*instr >> 24) & 0xF],
                arch_regnames[(*instr >> 12) & 0xF],
                (int) signExtend(*instr, 10),
                &chars);
    } else {
        snprintf(buf, len, "%s %s, %s, %s%n", arch_mnemonics[arch_identify(instr)],
                arch_regnames[(*instr >> 24) & 0xF],
                arch_regnames[(*instr >> 12) & 0xF],
                arch_regnames[*instr & 0xF],
                &chars);
    }
    return buf+len;
}

static char* disasm_type_c(char* buf, size_t len, const arch_word_t* instr) {
    int chars;
    if(*instr & 0x400) {
        snprintf(buf, len, "%s %s%s, %s, %i%n", arch_mnemonics[arch_identify(instr)],
                (*instr & 0x08000000) ? "!" : "",
                arch_prednames[(*instr >> 16) & 0x7],
                arch_regnames[(*instr >> 12) & 0xF],
                (int) signExtend(*instr, 10),
                &chars);
    } else {
        snprintf(buf, len, "%s %s%s, %s, %s%n", arch_mnemonics[arch_identify(instr)],
                (*instr & 0x08000000) ? "!" : "",
                arch_prednames[(*instr >> 16) & 0x7],
                arch_regnames[(*instr >> 12) & 0xF],
                arch_regnames[*instr & 0xF],
                &chars);
    }
    return buf+len;
}

static char* disasm_type_m(char* buf, size_t len, const arch_word_t* instr) {
    int chars;
    if(*instr & 0x400) {
        snprintf(buf, len, "%s %s, %s, %i%n", arch_mnemonics[arch_identify(instr)],
                arch_regnames[(*instr >> 16) & 0xF],
                arch_regnames[(*instr >> 12) & 0xF],
                (int) signExtend(*instr, 10),
                &chars);
    } else {
        snprintf(buf, len, "%s %s, %s, %s%n", arch_mnemonics[arch_identify(instr)],
                arch_regnames[(*instr >> 16) & 0xF],
                arch_regnames[(*instr >> 12) & 0xF],
                arch_regnames[*instr & 0xF],
                &chars);
    }
    return buf+len;
}

static char* disasm_j(char* buf, size_t len, const arch_word_t* instr) {
    int chars;
    int offset = signExtend(*instr, 22) << 2;
    snprintf(buf, len, "%s %s, %c0x%x%n", arch_mnemonics[arch_identify(instr)],
            arch_regnames[(*instr >> 24) & 0xF],
            (offset >= 0) ? '+' : '-',
            (offset >= 0) ? offset : -offset,
            &chars);
    return buf+len;
}

static char* disasm_lui(char* buf, size_t len, const arch_word_t* instr) {
    int chars;
    snprintf(buf, len, "%s %s, 0x%x%n", arch_mnemonics[arch_identify(instr)],
            arch_regnames[(*instr >> 24) & 0xF],
            *instr << 10,
            &chars);
    return buf+len;
}

static char* disasm_lur(char* buf, size_t len, const arch_word_t* instr) {
    int chars;
    int offset = *instr << 10;
    snprintf(buf, len, "%s %s, %c0x%x%n", arch_mnemonics[arch_identify(instr)],
            arch_regnames[(*instr >> 24) & 0xF],
            (offset >= 0) ? '+' : '-',
            (offset >= 0) ? offset : -offset,
            &chars);
    return buf+len;
}


static const disasm_t disasm_funcs[N_INSTRS] = {
    [I_J] =     disasm_j,
    [I_LUI] =   disasm_lui,
    [I_LUR] =   disasm_lur,
    [I_ADD] =   disasm_type_r,
    [I_SUB] =   disasm_type_r,
    [I_AND] =   disasm_type_r,
    [I_OR] =    disasm_type_r,
    [I_XOR] =   disasm_type_r,
    [I_SL] =    disasm_type_r,
    [I_SR] =    disasm_type_r,
    [I_SRA] =   disasm_type_r,
    [I_LTU] =   disasm_type_c,
    [I_LT] =    disasm_type_c,
    [I_EQ] =    disasm_type_c,
    [I_BIT] =   disasm_type_c,
    [I_LDP] =   disasm_type_r,
    [I_STP] =   disasm_type_m,
    [I_JX] =    disasm_type_r
};

char* arch_disasm(char* buf, size_t len, const arch_word_t* instr) {
    enum arch_instr type = arch_identify(instr);
    if(type == I_INVALID) {
        int chars;
        snprintf(buf, len, "%s%n", "[unknown]", &chars);
        return buf+len;
    }
    if(*instr & 0xF0000000) {
        int chars;
        snprintf(buf, len, "%c%s %n", (*instr & 0x80000000) ? '!' : '?', 
                arch_prednames[(*instr >> 28) & 0x7], &chars);
        buf += chars;
        len -= chars;
    }
    disasm_t disfunc = disasm_funcs[type];
    if(!disfunc) {
        int chars;
        snprintf(buf, len, "%s ...%n", arch_mnemonics[type], &chars);
        return buf+len;
    }
    return disfunc(buf, len, instr);
}