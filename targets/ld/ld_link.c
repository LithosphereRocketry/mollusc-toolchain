#include "ld_link.h"
#include "arch.h"
#include "strtools.h"
#include "structures.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct label_remap {
    const char* section;
    size_t offset;
};

// Just use one big context for all the iterator functions in this file, since
// they all basically need the same things
struct link_incr_ctx {
    struct string_map section_base_offsets; // size_t
    struct bin_file* source;
    struct bin_file* target;

    struct string_map source_remaps; // label_remap
    struct string_map target_remaps; // label_remap
};
static void concat_file_section(void* global, const char* name, void* value) {
    struct link_incr_ctx* ctx = global;
    struct bin_section* section = value;
    if(sm_haskey(&ctx->target->sections, name)) {
        struct bin_section* target_section = sm_get(&ctx->target->sections, name);
        size_t offset = target_section->data_sz * sizeof(arch_word_t);
        target_section->data = realloc(target_section->data,
                (target_section->data_sz + section->data_sz) * sizeof(arch_word_t));
        memcpy(target_section->data + target_section->data_sz, section->data,
                section->data_sz * sizeof(arch_word_t));
        target_section->data_sz += section->data_sz;
        sm_put(&ctx->section_base_offsets, name, (void*) offset, false);
    } else {
        // If the section doesn't exist in our current file, just transplant it
        // Probably we should check whether the section was heap allocated but
        // in practice it always is
        // TODO: this might break if run more than once on the same file
        sm_mark_heap(&ctx->source->sections, name, false);
        sm_put(&ctx->target->sections, name, section, true);
        sm_put(&ctx->section_base_offsets, name, 0, false);
    }
}

static void merge_label_into(struct link_incr_ctx* ctx, const char* name,
            struct bin_label* target, struct bin_label* source) {
    if(source->flags & BL_SECTION) {
        // Incoming section labels will always be dropped, but we need to record
        // a fixup for them in case someone is actually using them
        struct label_remap* remap = malloc(sizeof(struct label_remap));
        remap->section = name;
        remap->offset = (size_t) sm_get(&ctx->section_base_offsets, source->section);
        sm_put(&ctx->source_remaps, name, remap, true);
    } else if(source->flags & BL_UNDEF) {
        // Even if the new is undefined, our life is not necesarily simple
        if(source->flags & BL_EXPORTED) {
            if(target->flags & BL_EXPORTED) {
                // If we are undefined and exported, and our target is also
                // exported, we can just let the target take the wheel. If it's
                // defined, we've resolved our undefined label, and if it's
                // undefined, we're both waiting on the same thing
            } else {
                // An undefined exported label should clobber a defined local label
                struct label_remap* remap = malloc(sizeof(struct label_remap));
                remap->section = target->section;
                remap->offset = target->offset;
                sm_put(&ctx->target_remaps, name, remap, true);
                
                target->section = strcpy_dup(source->section);
                target->offset = source->offset + (size_t) sm_get(&ctx->section_base_offsets, source->section);
                target->flags = source->flags;
            }
        } else {
            // If an undefined local label appears at this stage, we've messed
            // up somewhere, because it can never be resolved
            // The toolchain as is should actually never produce this
            fprintf(stderr, "Undefined local label %s made it to the link stage"
            ". This is probably an indication that something incorrect happened"
            "during assembly\n", name);
            exit(1);
        }
    } else { // We are defined and not a section
        if(source->flags & BL_EXPORTED) {
            // We are defined, exported, and not a section (we are authoritative)
            if(target->flags & BL_EXPORTED) {
                // We are authoritative and target is visible
                if(target->flags & BL_UNDEF) {
                    // Target is looking for us, so fill it in
                    target->section = strcpy_dup(source->section);
                    target->offset = source->offset + (size_t) sm_get(&ctx->section_base_offsets, source->section);
                    target->flags |= source->flags;
                    target->flags &= ~BL_UNDEF;
                } else {
                    // We and target are both authoritative - we have a conflict
                    fprintf(stderr, "Conflicting definition of symbol %s\n", name);
                    exit(1);
                }
            } else {
                // We are authoritative but our target is hidden
                // We need to remap the target and take its place
                struct label_remap* remap = malloc(sizeof(struct label_remap));
                remap->section = target->section;
                remap->offset = target->offset;
                sm_put(&ctx->target_remaps, name, remap, true);
                
                target->section = strcpy_dup(source->section);
                target->offset = source->offset + (size_t) sm_get(&ctx->section_base_offsets, source->section);
                target->flags = source->flags;
            }
        } else {
            // We are defined but not exported: we just want to defer
            struct label_remap* remap = malloc(sizeof(struct label_remap));
            remap->section = source->section;
            remap->offset = source->offset + (size_t) sm_get(&ctx->section_base_offsets, source->section);
            sm_put(&ctx->source_remaps, name, remap, true);
        }
    }
}

static void merge_labels(void* global, const char* name, void* value) {
    struct link_incr_ctx* ctx = global;
    struct bin_label* lbl = value;
    if(sm_haskey(&ctx->target->labels, name)) {
        struct bin_label* old_lbl = sm_get(&ctx->target->labels, name);
        merge_label_into(ctx, name, old_lbl, lbl);
    } else {
        // If the label doesn't exist, we can just copy it over as long as we
        // update its offset
        // We make a copy here because we don't want to modify the label in the
        // old file on the offchance that we want to use it again
        struct bin_label* new_label = malloc(sizeof(struct bin_label));
        new_label->offset = lbl->offset + (size_t) sm_get(&ctx->section_base_offsets, lbl->section);
        new_label->flags = lbl->flags;
        new_label->section = strcpy_dup(lbl->section);
        sm_put(&ctx->target->labels, name, new_label, true);
    }
}

static void remap_relocations(void* global, const char* name, void* value) {
    struct bin_section* section = value;
    struct string_map* remaps = global;
    for(size_t i = 0; i < section->relocations.len; i++) {
        struct relocation* reloc = section->relocations.buf[i];
        if(sm_haskey(remaps, reloc->symbol)) {
            struct label_remap* remap = sm_get(remaps, reloc->symbol);
            free((char*) reloc->symbol);
            reloc->symbol = strcpy_dup(remap->section);
            reloc->offset = remap->offset;
        }
    }
}

static void import_relocations(void* global, const char* name, void* value) {
    struct link_incr_ctx* ctx = global;
    struct bin_section* section = value;
    struct bin_section* target = sm_get(&ctx->target->sections, name);
    for(size_t i = 0; i < section->relocations.len; i++) {
        struct relocation* reloc = section->relocations.buf[i];
        struct relocation* new_reloc = malloc(sizeof(struct relocation));
        size_t section_offset = (size_t) sm_get(&ctx->section_base_offsets, name);
        if(sm_haskey(&ctx->source_remaps, reloc->symbol)) {
            struct label_remap* remap = sm_get(&ctx->source_remaps, reloc->symbol);
            new_reloc->symbol = strcpy_dup(remap->section);
            new_reloc->offset = remap->offset + section_offset;
        } else {
            new_reloc->symbol = strcpy_dup(reloc->symbol);
            new_reloc->offset = reloc->offset + section_offset;
        }
        new_reloc->type = reloc->type;
        hl_append(&target->relocations, new_reloc);
    }
}

void link_incremental(struct bin_file* target, struct bin_file* incoming) {
    struct link_incr_ctx ctx = {
        .section_base_offsets = sm_make(),
        .source = incoming,
        .target = target,
        .source_remaps = sm_make(),
        .target_remaps = sm_make()
    };
    // Just concatenate the binary parts of each section for now; we can't merge
    // relocations yet because we don't know what symbols will survive the merge
    // process
    sm_foreach(&incoming->sections, concat_file_section, &ctx);
    sm_foreach(&incoming->labels, merge_labels, &ctx);
    sm_foreach(&target->sections, remap_relocations, &ctx.target_remaps);
    sm_foreach(&incoming->sections, import_relocations, &ctx);
}