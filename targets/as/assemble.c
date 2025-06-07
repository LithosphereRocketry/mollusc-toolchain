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
    if(res.data) res.data = realloc(res.data, res.data_sz);
    return res;
}

void print_assembly(const struct assembly_result* res) {
    printf("BINARY\n");
    for(size_t i = 0; i < res->data_sz; i++) {
        printf("\t%0*x\n", (int) sizeof(arch_word_t)*2, res->data[i]);
    }
}