#include "arch_disasm.h"
#include <stdio.h>

// TODO: this tree is duplicated from the emulation step - should emulation use
// this or is it more valuable to dedup instruction logic within the emulator?
enum arch_instr arch_identify(const arch_word_t* location) {
    arch_word_t instr = *location;
    if((instr & 0x00C00000) == 0x00C00000) { // auipc
        return I_AUIPC;
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