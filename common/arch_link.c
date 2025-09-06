#include "arch_link.h"

#include <stdlib.h>
#include <stdio.h>

#include "misctools.h"

static void reloc_err(const char* msg, const char* filename, size_t lineno, struct relocation* reloc) {
    fprintf(stderr, "Error at %s line %zu: %s\n[%s %s @ %zx]\n",
                    filename, lineno, msg, reloc->symbol, arch_relocnames[reloc->type], reloc->offset);
}

static const size_t IMM_MASK = ((1 << 10) - 1);
static const size_t LONG_MASK = ((1 << 22) - 1);

void link_section(struct bin_section* section) {
    size_t remaining = section->relocations.len;
    for(size_t i = 0; i < section->relocations.len; i++) {
        struct relocation* reloc = section->relocations.buf[i];
        if(sm_haskey(&section->relative_syms, reloc->symbol)) {
            switch(reloc->type) {
                case RELOC_J_REL: {
                    size_t target = (size_t) sm_get(&section->relative_syms, reloc->symbol);
                    if((target & 0b11) != 0) {
                        reloc_err("Relocation has unaligned target", "todo", 0, reloc);
                        exit(-1);
                    }
                    size_t field_offset = (section->data[reloc->offset] & LONG_MASK) << 2;
                    section->data[reloc->offset] &= ~LONG_MASK; 
                    size_t disp_words = ((target + field_offset) >> 2) - reloc->offset;
                    arch_word_t disp_trimmed = disp_words & LONG_MASK;
                    // if(...) {
                    //     reloc_err("Relocation out of range", "todo", 0, target, reloc);
                    //     exit
                    // }
                    section->data[reloc->offset] |= disp_trimmed;
                } break;
                case RELOC_LUR_REL: {
                    size_t target = (size_t) sm_get(&section->relative_syms, reloc->symbol);
                    size_t field_offset = (section->data[reloc->offset] & LONG_MASK) << 10;
                    section->data[reloc->offset] &= ~LONG_MASK; 
                    size_t disp = (target + field_offset) - (reloc->offset << 2);
                    arch_word_t disp_trimmed = (disp >> 10) & LONG_MASK;
                    section->data[reloc->offset] |= disp_trimmed;
                } break;
                case RELOC_IMM_REL: {
                    size_t target = (size_t) sm_get(&section->relative_syms, reloc->symbol);
                    size_t field_offset = signExtend(section->data[reloc->offset] & LONG_MASK, 10);
                    section->data[reloc->offset] &= ~IMM_MASK;
                    size_t disp = (target + field_offset) - (reloc->offset << 2);
                    arch_word_t disp_trimmed = disp & IMM_MASK;
                    section->data[reloc->offset] |= disp_trimmed;
                } break;
                default:
                    printf("Unrecognized relocation type %i\n", reloc->type);
                    exit(-1);
            }
            free(reloc);
            section->relocations.buf[i] = NULL;
            remaining --;
        } else {
            printf("Can't find label %s, skipping\n", reloc->symbol);
            continue;
        }
    }
    struct relocation** new_relocs = remaining == 0 ? NULL : malloc(remaining * sizeof(struct relocation*));
    struct relocation** old_word_ptr = (struct relocation**) section->relocations.buf;
    for(size_t i = 0; i < remaining; i++) {
        while(!section->relocations.buf[i]) old_word_ptr++;
        new_relocs[i] = *old_word_ptr;
        old_word_ptr++;
    }
    free(section->relocations.buf);
    section->relocations.buf = (void**) new_relocs;
    section->relocations.len = remaining;
    section->relocations._cap = remaining;
}
