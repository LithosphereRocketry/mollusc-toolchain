#include "arch_link.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "misctools.h"

static void reloc_err(const char* msg, const char* filename, size_t lineno, struct relocation* reloc) {
    fprintf(stderr, "Error at %s line %zu: %s\n[%s %s @ %zx]\n",
                    filename, lineno, msg, reloc->symbol, arch_relocnames[reloc->type], reloc->offset);
}

static const size_t IMM_MASK = ((1 << 10) - 1);
static const size_t LONG_MASK = ((1 << 22) - 1);

void link_section(struct string_map* labels, struct bin_section* section, const struct string_map* section_offsets) {
    struct string_map dummy_sm;
    if(!section_offsets) {
        // If we're passed null section offsets, make an empty map of section
        // offsets and use that (for convenience)
        dummy_sm = sm_make();
        section_offsets = &dummy_sm;
    }

    size_t remaining = section->relocations.len;
    for(size_t i = 0; i < section->relocations.len; i++) {
        struct relocation* reloc = section->relocations.buf[i];
        if(sm_haskey(labels, reloc->symbol)) {
            struct bin_label* label = sm_get(labels, reloc->symbol);
        
            // Figure out what we know about the source and destination addr
            bool know_abs, know_rel;
            size_t offs_abs;
            long long offs_rel;
            // We can get relative offset by either knowing both the source and
            // destination section locations, or by both being in the same 
            // section
            // A label relative to the section NULL is an absolute label, so we
            // know its offset (it's always 0)
            if(label->flags & BL_UNDEF) {
                know_abs = false;
                know_rel = false;
            } else {
                if(label->section == NULL && sm_haskey(section_offsets, section->name)) {
                    know_rel = true;
                    offs_rel = label->offset - (reloc->offset + (size_t) sm_get(section_offsets, section->name));
                } else if(!strcmp(section->name, label->section)) {
                    know_rel = true;
                    offs_rel = label->offset - reloc->offset;
                } else if(sm_haskey(section_offsets, section->name)
                    && sm_haskey(section_offsets, label->section)) {
                    know_rel = true;
                    offs_rel = (label->offset + (size_t) sm_get(section_offsets, label->section))
                            - (reloc->offset + (size_t) sm_get(section_offsets, section->name));
                } else {
                    know_rel = false;
                }
                if(label->section == NULL) {
                    know_abs = true;
                    offs_abs = label->offset;
                } else if(sm_haskey(section_offsets, label->section)) {
                    know_abs = true;
                    offs_abs = label->offset + (size_t) sm_get(section_offsets, section->name);
                } else {
                    know_abs = false;
                }
            }

            size_t target = label->offset;
            switch(reloc->type) {
                case RELOC_J_REL: {
                    if(!know_rel) continue;
                    if((target & 0b11) != 0) {
                        reloc_err("Relocation has unaligned target", "todo", 0, reloc);
                        exit(-1);
                    }
                    size_t field_offset = (section->data[reloc->offset >> 2] & LONG_MASK) << 2;
                    size_t disp_words = (offs_rel + field_offset) >> 2;
                    arch_word_t disp_trimmed = disp_words & LONG_MASK;
                    // if(...) {
                    //     reloc_err("Relocation out of range", "todo", 0, target, reloc);
                    //     exit
                    // }
                    section->data[reloc->offset >> 2] &= ~LONG_MASK; 
                    section->data[reloc->offset >> 2] |= disp_trimmed;
                } break;
                case RELOC_LUR_REL: {
                    if(!know_rel) continue;
                    size_t field_offset = (section->data[reloc->offset >> 2] & LONG_MASK) << 10;
                    size_t disp = offs_rel + field_offset;
                    arch_word_t disp_trimmed = (disp >> 10) & LONG_MASK;

                    section->data[reloc->offset >> 2] &= ~LONG_MASK; 
                    section->data[reloc->offset >> 2] |= disp_trimmed;
                } break;
                case RELOC_IMM_REL: {
                    if(!know_rel) continue;
                    size_t field_offset = signExtend(section->data[reloc->offset >> 2] & LONG_MASK, 10);
                    size_t disp = offs_rel + field_offset;
                    arch_word_t disp_trimmed = disp & IMM_MASK;

                    section->data[reloc->offset >> 2] &= ~IMM_MASK;
                    section->data[reloc->offset >> 2] |= disp_trimmed;
                } break;
                default:
                    printf("Unrecognized relocation type %i\n", reloc->type);
                    exit(-1);
            }
            free(reloc);
            section->relocations.buf[i] = NULL;
            remaining --;
        }
    }
    struct relocation** new_relocs = remaining == 0 ? NULL : malloc(remaining * sizeof(struct relocation*));
    struct relocation** old_word_ptr = (struct relocation**) section->relocations.buf;
    for(size_t i = 0; i < remaining; i++) {
        while(!(*old_word_ptr)) old_word_ptr++;
        new_relocs[i] = *old_word_ptr;
        old_word_ptr++;
    }
    free(section->relocations.buf);
    section->relocations.buf = (void**) new_relocs;
    section->relocations.len = remaining;
    section->relocations._cap = remaining;
}

struct link_file_iter_context {
    struct string_map* labels;
    struct string_map* section_offsets;
};

static void link_file_iter(void* ctx, const char* name, void* value) {
    struct link_file_iter_context* context = ctx;
    (void) name;
    struct bin_section* section = value;
    link_section(context->labels, section, context->section_offsets);
}

void link_file(struct asm_result* binfile, const struct string_map* section_offsets) {
    struct link_file_iter_context ctx = {
        .labels = &binfile->labels,
        .section_offsets = section_offsets
    };
    sm_foreach(&binfile->sections, link_file_iter, &ctx);
}
