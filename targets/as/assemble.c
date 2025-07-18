#include "assemble.h"

#include <stdio.h>
#include <stdlib.h>

#include "asm_arch.h"

static void asm_err(const char* msg, const char* hint, const char* filename, size_t lineno) {
    fprintf(stderr, "Error at %s line %zu: %s (%s)\n",
                    filename, lineno, msg, hint);
}

static size_t assemble_instr(struct assembly_result* dest, size_t offs, struct parse_instr* instr) {
    if((void*) (instr->type) == NULL) {
        asm_err("Tried to assemble an invalid instruction",
                "N/A", instr->file, instr->line);
        exit(-1);
    }
    dest->data[offs] = arch_basebits[instr->type];

    if(!assemble_predicate(dest, offs, instr->pred, !instr->pred_inv)) {
        asm_err("Tried to assemble invalid predicate", instr->pred, instr->file, instr->line);
    }
    
    const assemble_arg_t* argtype = arch_arguments[instr->type];
    if(!argtype) {
        asm_err("Tried to assemble an instruction with no argument map",
                arch_mnemonics[instr->type], instr->file, instr->line);
        exit(-1);
    }

    for(size_t i = 0; i < instr->args.len; i++) {
        if(*argtype == NULL) {
            asm_err("Too many arguments to instruction",
                    arch_mnemonics[instr->type], instr->file, instr->line);
            exit(-1);
        }
        if(!(*argtype)(dest, offs, instr->args.buf[i])) {
            asm_err("Invalid argument to instruction",
                    instr->args.buf[i], instr->file, instr->line);
            exit(-1);
        }
        argtype++;
    }
    if(*argtype != NULL) {
        asm_err("Not enough arguments to instruction",
                 arch_mnemonics[instr->type], instr->file, instr->line);
        exit(-1);
    }
    return 1;
}

struct assembly_result assemble(const struct parse_section* parse) {
    struct assembly_result res = {
        .relocations = hl_make(),
        .absolute_syms = sm_make(),
        .relative_syms = sm_make(),
        .data_sz = 0,
        .data = NULL
    };

    size_t offset_word = 0;
    for(size_t i = 0; i < parse->instrs.len; i++) {
        if(offset_word >= res.data_sz) {
            res.data_sz += (parse->instrs.len - i);
            res.data = realloc(res.data, res.data_sz * sizeof(arch_word_t));
        }
        struct parse_instr* instr = parse->instrs.buf[i];
        offset_word += assemble_instr(&res, offset_word, instr);
    }

    res.data_sz = offset_word;
    if(res.data) res.data = realloc(res.data, res.data_sz * sizeof(arch_word_t));
    return res;
}

static const char* rel_enum_names[] = {
    [RELOC_J_REL] = "RELOC_J_REL"
};

void print_assembly(const struct assembly_result* res) {
    printf("RELOCATIONS\n");
    for(size_t i = 0; i < res->relocations.len; i++) {
        struct relocation* reloc = res->relocations.buf[i];
        printf("\t%s %s %zu\n", rel_enum_names[reloc->type], reloc->symbol, reloc->offset);
    }
    printf("BINARY\n");
    for(size_t i = 0; i < res->data_sz; i++) {
        printf("\t%0*x\n", (int) sizeof(arch_word_t)*2, res->data[i]);
    }
}

void destroy_assembly(struct assembly_result* res) {
    for(size_t i = 0; i < res->relocations.len; i++) {
        struct relocation* reloc = res->relocations.buf[i];
        free((char*) reloc->symbol);
    }
    hl_destroy(&res->relocations, true);
    sm_destroy(&res->absolute_syms, true);
    sm_destroy(&res->relative_syms, true);
    free(res->data);
    res->data = NULL;
    res->data_sz = 0;
}
