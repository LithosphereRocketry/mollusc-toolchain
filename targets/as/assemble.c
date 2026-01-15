#include "assemble.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asm_arch.h"
#include "arch_link.h"
#include "asm_common.h"
#include "charparse.h"
#include "strtools.h"

static void asm_err(const char* msg, const char* hint, const char* filename, size_t lineno) {
    fprintf(stderr, "Error at %s line %zu: %s (%s)\n",
                    filename, lineno, msg, hint);
}

// TODO: this feels like it should be in a different file
static size_t assemble_pseudoinstr(struct bin_section* dest, size_t offs, struct parse_instr* instr) {
    switch(instr->type) {
        case PI_LR:
            // TODO: this is repetitive and should probably be abstracted away
            if(instr->args.len != 2) {
                asm_err("Wrong number of arguments to macro", arch_pseudo_mnemonics[PI_LR],
                        instr->file, instr->line);
                exit(-1);
            }
            char* name;
            const char* endlbl = parse_name(instr->args.buf[1], &name); // This is maybe a little stupid
            if(*endlbl != '\0') { // If we didn't use the whole argument
                asm_err("Argument not a valid label name", name, instr->file, instr->line);
                if(endlbl) free(name);
                exit(-1);
            }

            if(offs == dest->data_sz - 1) {
                dest->data_sz += 1;
                dest->data = realloc(dest->data, dest->data_sz * sizeof(arch_word_t));
            }
            
            dest->data[offs] = arch_basebits[I_LUR];
            if(!assemble_rd(dest, offs, instr->args.buf[0])) {
                asm_err("Failed to assemble destination", instr->args.buf[0],
                        instr->file, instr->line);
                exit(-1);
            }
            struct relocation* lur_reloc = malloc(sizeof(struct relocation));
            lur_reloc->offset = offs;
            lur_reloc->symbol = name;
            lur_reloc->type = RELOC_LUR_REL;
            hl_append(&dest->relocations, lur_reloc);

            dest->data[offs+1] = arch_basebits[I_ADD] | 0x404;
            if(!assemble_rd(dest, offs+1, instr->args.buf[0])) {
                asm_err("Failed to assemble destination", instr->args.buf[0],
                        instr->file, instr->line);
                exit(-1);
            }
            if(!assemble_ra(dest, offs+1, instr->args.buf[0])) {
                asm_err("Failed to assemble intermediate source", instr->args.buf[0],
                        instr->file, instr->line);
                exit(-1);
            }
            struct relocation* add_reloc = malloc(sizeof(struct relocation));
            add_reloc->offset = offs + 1;
            add_reloc->symbol = strcpy_dup(name);
            add_reloc->type = RELOC_IMM_REL;
            hl_append(&dest->relocations, add_reloc);




            return 2; // TODO
        // there's nothing wrong with extra padding bytes, so for now .ascii and
        // .asciz are the same
        case PI_ASCII:
        case PI_ASCIZ: {
            size_t n_chars = 0;
            size_t in_offs = offs;
            for(size_t i = 0; i < instr->args.len; i++) {
                const char* arg = instr->args.buf[i];
                const char* endarg = arg + strlen(arg) - 1;
                if(*arg != '"' || *endarg != '"') {
                    asm_err("String literal must use quotes", arg, instr->file, instr->line);
                    exit(-1);
                }
                arg++;
                // TODO: more elegant solution than just reserving extra space
                // on every string field
                // @brucedawson yes I am being lazy and using a quadratic
                // algorithm, shhhhh
                // +3 to round up, +1 for null, convert to words converts to
                // just always 1 extra word
                // But we are always reserved one full word for our instruction,
                // so we can just increase the allocation by the rounded-down
                // size (note, we increment rather than just allocate the needed
                // amount so that our heuristic for how many bytes we need is
                // still about right)
                size_t req_sz = in_offs + 1 + (endarg - arg) / sizeof(arch_word_t);
                if(dest->data_sz < req_sz) {
                    dest->data_sz += (endarg - arg) / sizeof(arch_word_t);
                    dest->data = realloc(dest->data, dest->data_sz * sizeof(arch_word_t));
                }

                dest->data[in_offs] = 0;
                for(const char* loc = arg; loc < endarg; ) {
                    char c;
                    loc = parseChar(loc, &c, stderr);
                    if(!loc) exit(-1);
                    dest->data[in_offs] |= ((int) c) << (8 * (n_chars & 0x3));
                    n_chars ++;
                    if((n_chars & 0x3) == 0) {
                        in_offs ++;
                        dest->data[in_offs] = 0;
                    }
                }
                // ensure null termination - we only need to do anything if
                // the null terminator overlaps onto a new word
                if((n_chars & 0x3) == 0) {
                    in_offs++;
                    dest->data[in_offs] = 0;
                }
            }
            return in_offs - offs + 1;
        } break;
        default:
            asm_err("Tried to assemble an invalid pseudoinstruction",
                    arch_pseudo_mnemonics[instr->type], instr->file, instr->line);
            exit(-1);
    }
}

static size_t assemble_instr(struct bin_section* dest, size_t offs, struct parse_instr* instr) {
    if((void*) (instr->type) == NULL) {
        asm_err("Tried to assemble an invalid instruction",
                "N/A", instr->file, instr->line);
        exit(-1);
    }
    if(instr->type >= (enum arch_pseudoinstr) N_INSTRS) {
        return assemble_pseudoinstr(dest, offs, instr);
    }

    dest->data[offs] = arch_basebits[instr->type];

    if(!assemble_predicate(dest, offs, instr->pred, !instr->pred_inv)) {
        asm_err("Tried to assemble invalid predicate", instr->pred, instr->file, instr->line);
        exit(-1);
    }
    if(arch_assemble_modes[instr->type] && !arch_assemble_modes[instr->type](dest, offs, instr->mode)) {
        asm_err("Tried to assemble invalid instruction mode", instr->mode, instr->file, instr->line);
        exit(-1);
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

struct transfer_asm_label_global {
    struct string_map* relative_syms;
    size_t* offset_map;
};

static void transfer_asm_label(void* global, const char* key, void* value) {
    struct transfer_asm_label_global* info = global;
    size_t offset = info->offset_map[((size_t) value)] * sizeof(arch_word_t);
    sm_put(info->relative_syms, key, (void*) offset);
}

struct bin_section assemble(const struct parse_section* parse) {
    struct bin_section res = {
        .relocations = hl_make(),
        .absolute_syms = sm_make(),
        .relative_syms = sm_make(),
        .data_sz = 0,
        .data = NULL
    };

    size_t* instr_offsets = malloc(parse->instrs.len * sizeof(size_t));

    size_t offset_word = 0;
    for(size_t i = 0; i < parse->instrs.len; i++) {
        if(offset_word >= res.data_sz) {
            res.data_sz += (parse->instrs.len - i);
            res.data = realloc(res.data, res.data_sz * sizeof(arch_word_t));
        }
        instr_offsets[i] = offset_word;
        struct parse_instr* instr = parse->instrs.buf[i];
        offset_word += assemble_instr(&res, offset_word, instr);
    }

    struct transfer_asm_label_global transfer_info = {
        .relative_syms = &res.relative_syms,
        .offset_map = instr_offsets
    };

    sm_foreach(&parse->instr_labels, transfer_asm_label, &transfer_info);
    free(instr_offsets);

    res.data_sz = offset_word;
    if(res.data) res.data = realloc(res.data, res.data_sz * sizeof(arch_word_t));
    link_section(&res);
    return res;
}

static const char* rel_enum_names[] = {
    [RELOC_J_REL] = "RELOC_J_REL"
};

void print_assembly(const struct bin_section* res) {
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

void destroy_assembly(struct bin_section* res) {
    for(size_t i = 0; i < res->relocations.len; i++) {
        struct relocation* reloc = res->relocations.buf[i];
        free((char*) reloc->symbol);
    }
    hl_destroy(&res->relocations, true);
    sm_destroy(&res->absolute_syms, false);
    sm_destroy(&res->relative_syms, false);
    free(res->data);
    res->data = NULL;
    res->data_sz = 0;
}
