#include "arch_link.h"

#include <stdlib.h>
#include <stdio.h>

static void reloc_err(const char* msg, const char* filename, size_t lineno, struct relocation* reloc) {
    fprintf(stderr, "Error at %s line %zu: %s\n[%s %s @ %zx]\n",
                    filename, lineno, msg, reloc->symbol, arch_relocnames[reloc->type], reloc->offset);
}

void link_section(struct bin_section* section) {
    size_t remaining = section->relocations.len;
    for(size_t i = 0; i < section->relocations.len; i++) {
        struct relocation* reloc = section->relocations.buf[i];
        switch(reloc->type) {
            case RELOC_J_REL:
                if(sm_haskey(&section->relative_syms, reloc->symbol)) {
                    size_t target = (size_t) sm_get(&section->relative_syms, reloc->symbol);
                    if((target & 0b11) != 0) {
                        reloc_err("Relocation has unaligned target", "todo", 0, reloc);
                        exit(-1);
                    }
                    size_t offs_words = (target >> 2) - reloc->offset;
                    arch_word_t offs_trimmed = offs_words & ((1 << 22) - 1);
                    // if(...) {
                    //     reloc_err("Relocation out of range", "todo", 0, target, reloc);
                    //     exit
                    // }
                    printf("data before: %08x\n", section->data[reloc->offset]);
                    section->data[reloc->offset] |= offs_trimmed;
                    printf("data after: %08x\n", section->data[reloc->offset]);
                } else {
                    printf("Can't find label %s, skipping\n", reloc->symbol);
                    continue;
                }
                break;
            default:
                printf("Unrecognized relocation type %i\n", reloc->type);
                exit(-1);
        }
        free(reloc);
        section->relocations.buf[i] = NULL;
        remaining --;
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
