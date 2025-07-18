#include "asm_arch.h"
#include "asm_common.h"

#include <stdlib.h>
#include <stdio.h>

static const size_t INVALID_REG = -1;

static size_t regnum(const char* name) {
    static bool firstrun = true;
    static struct string_map map;
    if(firstrun) {
        // TODO: better way to find size (this is basically all that uses it tho)
        map = arr_inv_to_sm(arch_regnames, 16);
        // for now, no aliases for registers
        firstrun = false;
    }
    if(!sm_haskey(&map, name)) return INVALID_REG;
    return (size_t) sm_get(&map, name);
}

static size_t prednum(const char* name) {
    static bool firstrun = true;
    static struct string_map map;
    if(firstrun) {
        // TODO: better way to find size (this is basically all that uses it tho)
        map = arr_inv_to_sm(arch_prednames, 8);
        // for now, no aliases for predicates
        firstrun = false;
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

bool assemble_predicate(struct assembly_result* res, size_t offs, const char* arg, bool nonzero) {
    if(!arg) {
        // Default (always run) predicate is equivalent to all 0s
        return true;
    }
    size_t reg = prednum(arg);
    if(reg == INVALID_REG) {
        return false;
    } else {
        if(nonzero) reg |= 1<<3;
        res->data[offs] |= reg << 28;
        return true;
    }
}
static bool assemble_pd(struct assembly_result* res, size_t offs, const char* arg) {
    bool invert = false;
    if(*arg == '!') {
        invert = true;
        arg ++;
    } else if(*arg == '?') {
        arg ++;
    }
    size_t reg = prednum(arg);
    if(reg == INVALID_REG) {
        return false;
    } else {
        if(invert) reg |= 1<<3;
        res->data[offs] |= reg << 24;
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

static bool assemble_imm10(struct assembly_result* res, size_t offs, const char* arg) {
    char* endstr;
    long val = strtol(arg, &endstr, 0);
    if(endstr == arg) return false;
    res->data[offs] |= (1 << 10) | (val & ((1 << 10) - 1));
    return true;
}

static bool assemble_immj(struct assembly_result* res, size_t offs, const char* arg) {
    char* endstr;
    long val = strtol(arg, &endstr, 0);
    if(endstr == arg) { // no number
        char* name;
        const char* endlbl = parse_name(arg, &name); // This is maybe a little stupid
        if(*endlbl == '\0') { // If we used the whole argument
            struct relocation* reloc = malloc(sizeof(struct relocation));
            reloc->type = RELOC_J_REL;
            reloc->symbol = name;
            reloc->offset = offs;
            hl_append(&res->relocations, reloc);
            return true;
        } else {
            if(endlbl) free(name);
            return false;
        }
    } else {
        printf("Numbered jumps not yet supported\n");
        return false;
    }
}

static bool assemble_rbi(struct assembly_result* res, size_t offs, const char* arg) {
    return assemble_rb(res, offs, arg) || assemble_imm10(res, offs, arg);
}

static const assemble_arg_t argmap_type_r[] =
        {assemble_rd, assemble_ra, assemble_rbi, NULL};
static const assemble_arg_t argmap_type_c[] = 
        {assemble_pd, assemble_ra, assemble_rbi, NULL};

static const assemble_arg_t argmap_j[] =
        {assemble_rd, assemble_immj, NULL};

const assemble_arg_t* arch_arguments[] = {
    [I_J] =     argmap_j,
    [I_LUI] =   NULL,
    [I_AUIPC] = NULL,
    [I_ADD] =   argmap_type_r,
    [I_SUB] =   argmap_type_r,
    [I_AND] =   argmap_type_r,
    [I_OR] =    argmap_type_r,
    [I_XOR] =   argmap_type_r,
    [I_SL] =    argmap_type_r,
    [I_SR] =    argmap_type_r,
    [I_SRA] =   argmap_type_r,
    [I_LTU] =   argmap_type_c,
    [I_LT] =    argmap_type_c,
    [I_EQ] =    argmap_type_c,
    [I_BIT] =   argmap_type_c,
    [I_LDP] =   NULL,
    [I_STP] =   NULL,
    [I_JX] =    argmap_type_r
};