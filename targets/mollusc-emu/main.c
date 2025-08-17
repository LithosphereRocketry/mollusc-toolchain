#include <stdio.h>
#include <stdlib.h>

#include "argparse.h"
#include "iotools.h"
#include "string.h"
#include "arch_disasm.h"

argument_t arg_cycles = {'c', "max-cycles", true, {NULL}};

argument_t* args[] = {
    &arg_cycles
};

static void arg_err(const char* message, const argument_t* arg) {
    fprintf(stderr, "Error in CLI argument -%c/--%s (%s): %s \n",
                    arg->abbr, arg->name,
                    arg->hasval ? arg->result.value : "<no value>", message);
}

static size_t max_cycles; // stop after N instructions

uint32_t signExtend(uint32_t value, uint32_t bits) {
	uint32_t mask = ~((1 << bits) - 1);
	if(value & (1 << (bits-1))) {
		return value | mask;
	} else {
		return value & ~mask;
	}
}

enum mem_type {
    MEM_LOWONLY
};

struct cpu_state {
    enum mem_type mtype;
    uint32_t regs[16];
    uint32_t pc;
    uint8_t pred;
    union {
        struct {
            uint32_t ram[8192];
            uint32_t rom[8192];
        } mem_lowonly;
    } mem;
} state;

void init(struct cpu_state* state, const char* fdata, size_t len) {
    switch(state->mtype) {
        case MEM_LOWONLY:
            state->pc = 0x8000;
            if(len > 0x8000) {
                fprintf(stderr, "ROM image too large for memory (got %zu bytes)\n", len);
                exit(-1);
            }
            memcpy(state->mem.mem_lowonly.rom, fdata, len);
            break;
    }
}

uint32_t load(struct cpu_state* state, uint32_t addr) {
    if((addr & 0x3) != 0) {
        fprintf(stderr, "Attempted load with misaligned address (%08x)\n", addr);
        exit(-1);
    }
    switch(state->mtype) {
        case MEM_LOWONLY:
            if(addr < 0x8000) {
                return state->mem.mem_lowonly.ram[addr >> 2];
            } else if(addr < 0x10000) {
                return state->mem.mem_lowonly.rom[(addr - 0x8000) >> 2];
            } else {
                fprintf(stderr, "Attempted load out of bounds (%08x)\n", addr);
                exit(-1);
            }
            break;
        default:
            fprintf(stderr, "Got unexpected memory type %i\n", state->mtype);
            exit(-1);
    };
}

void store(struct cpu_state* state, uint32_t addr, uint32_t data) {
    if((addr & 0x3) != 0) {
        fprintf(stderr, "Attempted load with misaligned address (%08x)\n", addr);
        exit(-1);
    }
    switch(state->mtype) {
        case MEM_LOWONLY:
            if(addr < 32768) {
                state->mem.mem_lowonly.ram[addr >> 2] = data;
            } else if(addr < 65536) {
                state->mem.mem_lowonly.rom[addr >> 2] = data;
            } else if(addr == 0x01000000) {
                putchar(data & 0xFF);
            } else if(addr == 0x01001000) {
                fprintf(stderr, "Terminating with code %i\n", (int) data);
                exit(data);
            } else {
                fprintf(stderr, "Attempted load out of bounds (%08x)\n", addr);
                exit(-1);
            }
            break;
    };
}

void print_state(struct cpu_state* state) {
    const arch_word_t instr = load(state, state->pc);
    // Note, this relies on MOLLUSC instructions being fixed width
    fprintf(stderr, "%08x : %08x (%s)\tpred: ", state->pc, instr, arch_mnemonics[arch_identify(&instr)]);
    for(size_t i = 0; i < 8; i++) putc((state->pred & (1 << i)) ? '1' : '0', stderr);
    for(size_t row = 0; row < 4; row ++) {
        putc('\n', stderr);
        for(size_t i = row; i < row + 16; i += 4) {
            if(i < 10) putc(' ', stderr);
            fprintf(stderr, "%zu %4s:%08x\t", i, arch_regnames[i], state->regs[i]);
        }
    }
    putc('\n', stderr);
}

void step(struct cpu_state* state) {
    uint32_t instr = load(state, state->pc);
    uint32_t pred = instr >> 28;
    uint32_t rp = pred & 0b111;
    uint32_t inv = pred >> 3;
    if((((state->pred >> rp) ^ inv) & 1) == 0) {
        if((instr & 0x00C00000) == 0x00C00000) { // auipc
            fprintf(stderr, "Unsupported instruction %08x\n", instr);
            exit(-1);
        } else if((instr & 0x00C00000) == 0x00800000) { // lui
            int rd = (instr >> 24) & 0xF;
            if(rd != 0) state->regs[rd] = instr << 10;
            state->pc += 4;
        } else if((instr & 0x00C00000) == 0x00400000) { // j
            int rd = (instr >> 24) & 0xF;
            if(rd != 0) state->regs[rd] = state->pc+4;
            state->pc += signExtend(instr, 22) << 2;
        } else { // register instrs
            int ra = (instr >> 12) & 0xF;
            int rb = instr & 0xF;
            uint32_t a = state->regs[ra];
            uint32_t b = (instr & 0x400) ? signExtend(instr, 10) : state->regs[rb];
            if((instr & 0x00300800) == 0x00300800) { // store
                int rm = (instr >> 16) & 0xF;
                uint32_t m = state->regs[rm];
                switch((instr >> 24) & 0xF) {
                    case 0x0: store(state, a + b, m); break;
                    default:
                        fprintf(stderr, "Unsupported instruction %08x\n", instr);
                        exit(-1);
                        break;
                }
                state->pc += 4;
            } else if((instr & 0x00300800) == 0x00200000) { // comparison
                int pd = (instr >> 24) & 0x7;
                int pinv = (instr >> 27) & 1;
                int res;
                switch((instr >> 16) & 0xF) {
                    case 0x0: res = a > b ? 1 : 0; break;
                    case 0x1: res = (int16_t) a > (int16_t) b ? 1 : 0; break;
                    case 0x2: res = a == b ? 1 : 0; break;
                    case 0x3: res = a & (1 << b) ? 1 : 0; break;
                    default:
                        fprintf(stderr, "Unsupported instruction %08x\n", instr);
                        exit(-1);
                        break;
                }
                fprintf(stderr, "pd: %i pinv: %i res: %i", pd, pinv, res);
                if(pd) {
                    state->pred &= ~(1 << pd);
                    state->pred |= (res ^ pinv) << pd;
                }
                state->pc += 4;
            } else {
                int rd = (instr >> 24) & 0xF;
                uint32_t res;
                if((instr & 0x00300800) == 0x00200800) { // jx, misc
                    int fcode = (instr >> 16) & 0xF;
                    switch(fcode) {
                        case 0x0: // jx
                            res = state->pc + 4;
                            state->pc = a + b;
                            break;
                        default:
                            fprintf(stderr, "Unsupported instruction %08x\n", instr);
                            exit(-1);
                            break;
                    }
                } else { // normal arithmetic
                    switch(instr & 0x001F0800) {
                        case 0x00000000: res = a + b; break; // add
                        case 0x00010000: res = a - b; break; // sub
                        case 0x00020000: res = a & b; break; // and
                        case 0x00060000: res = a >> b; break;
                        default:
                            fprintf(stderr, "Unsupported instruction %08x\n", instr);
                            exit(-1);
                    }
                    state->pc += 4;
                }
                if(rd) state->regs[rd] = res;
            }
        }
    } else {
        fprintf(stderr, "predicate denied %i\n", (((state->pred >> rp) ^ inv) & 1) == 0);
        state->pc += 4;
    }
}

int main(int argc, char** argv) {
    argc = argparse(args, sizeof(args)/sizeof(argument_t*), argc, argv);

    if(arg_cycles.result.value) {
        char* endstr;
        max_cycles = strtoul(arg_cycles.result.value, &endstr, 10);
        if(*endstr != '\0' || max_cycles == 0) {
            arg_err("Expected a positive integer", &arg_cycles);
            exit(-1);
        }
    }

    if(argc <= 1) {
        fprintf(stderr, "No file given!\n");
        exit(-1);
    } else if(argc > 2) {
        fprintf(stderr, "Too many files given! (Starting with %s)\n", argv[2]);
        exit(-1);
    }
    
    state.pred &= 0b11111110;
    state.regs[0] = 0;

    char* filename = argv[1];
    FILE* file = fopen(filename, "rb");
    if(!file) {
        fprintf(stderr, "Failed to open source file %s", filename);
        exit(-1);
    }
    char* filebin = fread_dup(file);
    init(&state, filebin, ftell(file));
    free(filebin);
    fclose(file);

    for(size_t i = 0; i < max_cycles || max_cycles == 0; i++) {
        print_state(&state);
        step(&state);
    }
}