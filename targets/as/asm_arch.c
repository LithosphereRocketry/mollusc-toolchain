#include "asm_arch.h"

static const size_t INVALID_REG = -1;

static size_t regnum(const char* name) {
    static bool firstrun = true;
    static struct string_map map;
    if(firstrun) {
        // TODO: better way to find size (this is basically all that uses it tho)
        map = arr_inv_to_sm(arch_regnames, 16);
        // for now, no aliases for registers
    }
    if(!sm_haskey(&map, name)) return INVALID_REG;
    return (size_t) sm_get(&map, name);
}

static bool assemble_reg(struct assembly_result* res, size_t offs, const char* arg, size_t shift) {
    size_t reg = regnum(arg);
    if(reg == INVALID_REG) {
        return false;
    } else {
        res->data[offs] |= reg << shift;
        return true;
    }
}

static bool assemble_rd(struct assembly_result* res, size_t offs, const char* arg) {
    return assemble_reg(res, offs, arg, 24);
}
static bool assemble_ra(struct assembly_result* res, size_t offs, const char* arg) {
    return assemble_reg(res, offs, arg, 12);
}
static bool assemble_rb(struct assembly_result* res, size_t offs, const char* arg) {
    return assemble_reg(res, offs, arg, 0);
}

static bool assemble_imm11(struct assembly_result* res, size_t offs, const char* arg) {
    return false;
}

static bool assemble_rbi(struct assembly_result* res, size_t offs, const char* arg) {
    return assemble_rb(res, offs, arg) || assemble_imm11(res, offs, arg);
}

static const assemble_arg_t argmap_default[] =
        {assemble_rd, assemble_ra, assemble_rbi, NULL};

const assemble_arg_t* arch_arguments[] = {
    [I_ADD] = argmap_default
};