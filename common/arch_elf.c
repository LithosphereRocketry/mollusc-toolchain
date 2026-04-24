#include "arch_elf.h"

#include <stdlib.h>
#include <string.h>
#include "arch.h"
#include "misctools.h"
#include "strtools.h"

static const Elf32_Shdr SHDR_UNDEF = {
    .sh_name = 0,
    .sh_type = SHT_NULL,
    .sh_flags = 0,
    .sh_addr = 0,
    .sh_offset = 0,
    .sh_size = 0,
    .sh_link = SHN_UNDEF,
    .sh_info = 0,
    .sh_addralign = 0,
    .sh_entsize = 0
};

static const Elf32_Sym SYM_UNDEF = {
    .st_name = 0,
    .st_value = 0,
    .st_size = 0,
    .st_info = 0,
    .st_other = 0,
    .st_shndx = SHN_UNDEF
};

struct elf_unroll_section {
    const char* name;
    struct bin_section* section;
};
struct elf_unroll_section_context {
    struct elf_unroll_section* sections;
    size_t len;
    struct string_map unroll_lookup; // size_t
};

static void elf_unroll_sections(void* ctx, const char* name, void* value) {
    struct elf_unroll_section_context* context = ctx;
    struct bin_section* section = value;
    sm_put(&context->unroll_lookup, name, (void*) context->len, false);
    context->sections[context->len].name = name;
    context->sections[context->len].section = section;
    context->len ++;
}

static const size_t n_static_sections = 4;
// Initial contents of section header string table - names of sections that are
// implied rather than being part of the assembly
// Do a bit of #define fuckery to make these offsets not hardcoded
#define BEFORE_SHSTRTAB ""
static const size_t offs_shstrtab = sizeof(BEFORE_SHSTRTAB);
#define BEFORE_STRTAB BEFORE_SHSTRTAB "\0.shstrtab"
static const size_t offs_strtab = sizeof(BEFORE_STRTAB);
#define BEFORE_SYMTAB BEFORE_STRTAB "\0.strtab"
static const size_t offs_symtab = sizeof(BEFORE_SYMTAB);
#define INIT_SHSTRTAB BEFORE_SYMTAB "\0.symtab"
static const size_t sz_init_shstrtab = sizeof(INIT_SHSTRTAB);

#define REL_PREFIX ".rel"

struct elf_create_symtab_context {
    const struct string_map* symbols;
    const struct string_map* section_idxs;
    struct string_map* sym_offsets;
    Elf32_Sym* buf;
    size_t idx;
    size_t last_local;
};
static void elf_create_symtab(void* ctx, const char* name, void* value) {
    struct elf_create_symtab_context* context = ctx;
    struct bin_label* lbl = value;

    Elf32_Section section;
    if(lbl->flags & BL_UNDEF) {
        section = SHN_UNDEF;
    } else if(lbl->section == NULL) {
        section = SHN_ABS;
    } else {
        section = n_static_sections + 2*(size_t) sm_get(context->section_idxs, lbl->section);
    }

    Elf32_Sym sym = {
        // Offset is shifted by 1 due to null
        // We shouldn't need to error check this because name is guaranteed to
        // come from symbols
        .st_name = sm_stoffs(context->symbols, name) + 1,
        .st_value = lbl->offset,
        .st_size = 0,
        .st_info = ELF32_ST_INFO(
            (lbl->flags & BL_EXPORTED) ? STB_GLOBAL : STB_LOCAL,
            (lbl->flags & BL_SECTION) ? STT_SECTION : STT_NOTYPE
        ),
        .st_other = 0,
        .st_shndx = section
    };
    context->buf[context->idx] = sym;
    sm_put(context->sym_offsets, name, (void*) context->idx, false);
    context->idx ++;
    if(!(lbl->flags & BL_EXPORTED)) {
        context->last_local = context->idx;
    }
}

struct elf_create_section_context {
    FILE* f;
    size_t symtab_idx;
    const struct string_map* section_map;
    const struct string_map* rel_section_map;
    const struct string_map* sym_offsets;
    Elf32_Shdr* header_buf;
    size_t section_idx;
};
static void elf_create_section(void* ctx, const char* name, void* value) {
    struct elf_create_section_context* context = ctx;
    struct bin_section* section = value;
    // Binary section
    Elf32_Shdr bheader = {
        .sh_name = sm_stoffs(context->section_map, name) + sz_init_shstrtab,
        .sh_type = SHT_PROGBITS,
        .sh_flags = 0, // todo: this probably needs to be changed
        .sh_addr = 0,
        .sh_offset = ftell(context->f),
        .sh_size = section->data_sz * sizeof(arch_word_t),
        .sh_link = SHN_UNDEF,
        .sh_info = 0,
        .sh_addralign = 4,
        .sh_entsize = 0
    };
    fwrite(section->data, sizeof(arch_word_t), section->data_sz, context->f);
    context->header_buf[context->section_idx] = bheader;
    context->section_idx ++;

    // Relocation section
    Elf32_Shdr rheader = {
        .sh_name = (size_t) sm_get(context->rel_section_map, name),
        .sh_type = SHT_REL,
        .sh_flags = 0,
        .sh_addr = 0,
        .sh_offset = ftell(context->f),
        .sh_size = section->relocations.len * sizeof(Elf32_Rel),
        .sh_link = context->symtab_idx,
        .sh_info = context->section_idx-1,
        .sh_addralign = 0,
        .sh_entsize = sizeof(Elf32_Rel)
    };
    for(size_t i = 0; i < section->relocations.len; i++) {
        struct relocation* rel = section->relocations.buf[i];
        Elf32_Rel elfrel = {
            .r_offset = rel->offset,
            .r_info = ELF32_R_INFO((size_t) sm_get(context->sym_offsets, rel->symbol), rel->type)
        };
        fwrite(&elfrel, sizeof(Elf32_Rel), 1, context->f);
    }
    context->header_buf[context->section_idx] = rheader;
    context->section_idx ++;    
}

int elf_write(FILE* f, const struct bin_file* bin, Elf32_Half type) {
    if(type != ET_REL) {
        fprintf(stderr, "ELF type %i not supported\n", type);
        exit(1);
    }

    // The iteration order of sm_foreach will _probably_ be consistent, but it's
    // not guaranteed by the interface. So I have to unroll the map into an
    // array once, so that section indexes are consistent.
    struct elf_unroll_section_context unrolled = {
        .len = 0,
        .sections = malloc(sm_size(&bin->sections) * sizeof(struct elf_unroll_section)),
        .unroll_lookup = sm_make()
    };
    sm_foreach(&bin->sections, elf_unroll_sections, &unrolled);

    // It should be possible to know how many sections we're going to use ahead
    // of time - we have a couple constant sections, and then one for each of
    // our assembly sections
    // null, shstrtab, strtab, symtab, [file sections, file relocs] 
    const size_t n_sections = 4 + 2*sm_size(&bin->sections);
    // Sections are arranged as code followed by relocations

    // === ELF header ===

    Elf32_Ehdr hdr = {
        .e_ident = {
            0x7F, 'E', 'L', 'F',
            ELFCLASS32,
            ELFDATA2LSB,
            EV_CURRENT,
            0
        },
        .e_type = type,
        .e_machine = EM_MOLLUSC,
        .e_version = 1,
        .e_entry = 0,
        .e_phoff = 0,
        // does it make sense to put the section headers at the beginning or
        // end? existing tools seem to put it at the end but I'm not sure why
        .e_shoff = -1, // for now, put it at the end - we'll have to fix it up later
        .e_flags = 0,
        .e_ehsize = sizeof(Elf32_Ehdr),

        // for now, we only make linkable objects, so no program header
        .e_phentsize = 0,
        .e_phnum = 0,
        .e_shentsize = sizeof(Elf32_Shdr),
        .e_shnum = n_sections,
        .e_shstrndx = 1 // put the section header strings first because why not
    };
    fwrite(&hdr, sizeof(hdr), 1, f);

    Elf32_Shdr* section_headers = malloc(n_sections*sizeof(Elf32_Shdr));

    section_headers[0] = SHDR_UNDEF;


    // === Section header string table ===

    size_t shstrtab_non_rel_sz = sz_init_shstrtab + sm_stsize(&bin->sections);
    const Elf32_Shdr shdr_shstrtab = {
        .sh_name = offs_shstrtab,
        .sh_type = SHT_STRTAB,
        .sh_flags = 0,
        .sh_addr = 0,
        // If we just assemble each part as it goes, we can just use ftell to
        // see how far along we are
        .sh_offset = ftell(f),
        // my implementation guarantees that the string table in my hashtable is
        // sufficiently ELF-shaped to just use
        .sh_size = align_roundup(shstrtab_non_rel_sz + sm_stsize(&bin->sections)
                 + sm_size(&bin->sections)*strlen(REL_PREFIX), 4),
        .sh_link = 0, // I don't think a section header string table needs any
        // linking information
        .sh_addralign = 0,
        .sh_entsize = 0
    };
    // Write section header names into file
    char* shstrtab_buffer = malloc(shdr_shstrtab.sh_size);
    memset(shstrtab_buffer, 0, shdr_shstrtab.sh_size);
    memcpy(shstrtab_buffer, INIT_SHSTRTAB, sz_init_shstrtab);
    memcpy(shstrtab_buffer + sz_init_shstrtab, sm_stringtable(&bin->sections),
            sm_stsize(&bin->sections));
    size_t shstrtab_offs = sz_init_shstrtab + sm_stsize(&bin->sections);
    struct string_map rel_offset_map = sm_make();
    for(size_t i = 0; i < sm_size(&bin->sections); i++) {
        int incr;
        sprintf(shstrtab_buffer + shstrtab_offs, REL_PREFIX "%s%n", unrolled.sections[i].name, &incr);
        sm_put(&rel_offset_map, unrolled.sections[i].name, (void*) shstrtab_offs, false);
        shstrtab_offs += incr+1;
    }
    fwrite(shstrtab_buffer, 1, shdr_shstrtab.sh_size, f);
    section_headers[1] = shdr_shstrtab;
    free(shstrtab_buffer);

    
    // === Symbol string table ===

    const char* strtab_data = sm_stringtable(&bin->labels);
    size_t strtab_dsize = sm_stsize(&bin->labels);
    Elf32_Shdr shdr_strtab = {
        .sh_name = offs_strtab,
        .sh_type = SHT_STRTAB,
        .sh_flags = 0,
        .sh_addr = 0,
        .sh_offset = ftell(f),
        .sh_size = align_roundup(1 + strtab_dsize, 4),
        .sh_link = 0,
        .sh_addralign = 0,
        .sh_entsize = 0
    };
    // ELF expects string tables to start with a null, and it makes more sense
    // to just manually add it than to force every string table to include one
    putc('\0', f);
    fwrite(strtab_data, 1, strtab_dsize, f);
    for(size_t i = 1 + strtab_dsize; i < shdr_strtab.sh_size; i++) {
        // pad out so that the next section starts on a word boundary
        putc('\0', f);
    }
    section_headers[2] = shdr_strtab;


    // === Symbol table ===

    Elf32_Shdr shdr_symtab = {
        .sh_name = offs_symtab,
        .sh_type = SHT_SYMTAB,
        .sh_flags = 0,
        .sh_addr = 0,
        .sh_offset = ftell(f),
        .sh_size = sizeof(Elf32_Sym) * (1 + sm_size(&bin->labels)),
        .sh_link = 2,
        .sh_addralign = 0,
        .sh_entsize = sizeof(Elf32_Sym)
    };
    Elf32_Sym* symbols = malloc(sizeof(Elf32_Sym) * (1 + sm_size(&bin->labels)));
    symbols[0] = SYM_UNDEF;
    struct string_map symbol_idxs = sm_make();
    struct elf_create_symtab_context symtab_ctx = {
        .buf = symbols,
        .idx = 1,
        .sym_offsets = &symbol_idxs,
        .section_idxs = &unrolled.unroll_lookup,
        .symbols = &bin->labels,
        .last_local = 0
    };
    sm_foreach(&bin->labels, elf_create_symtab, &symtab_ctx);
    shdr_symtab.sh_info = symtab_ctx.last_local;
    fwrite(symbols, sizeof(Elf32_Sym), (1 + sm_size(&bin->labels)), f);
    section_headers[3] = shdr_symtab;

    free(symbols);

    
    // === Binary and relocation sections ===
    struct elf_create_section_context main_ctx = {
        .f = f,
        .symtab_idx = 3,
        .section_map = &bin->sections,
        .rel_section_map = &rel_offset_map,
        .sym_offsets = &symbol_idxs,
        .header_buf = section_headers,
        .section_idx = 4
    };
    sm_foreach(&bin->sections, elf_create_section, &main_ctx);

    // Finish by writing section headers
    Elf32_Word shoff = ftell(f);
    fwrite(section_headers, sizeof(Elf32_Shdr), n_sections, f);
    // Correct the section header location now that we know where it is
    fseek(f, offsetof(Elf32_Ehdr, e_shoff), SEEK_SET);
    fwrite(&shoff, sizeof(Elf32_Word), 1, f);
    free(section_headers);
    free(unrolled.sections);
    return 0;
}

static struct bin_section* ensure_section(struct bin_file* bin, const char* name) {
    if(!sm_haskey(&bin->sections, name)) {
        struct bin_section* section = malloc(sizeof(struct bin_section));
        section->data = NULL;
        section->data_sz = 0;
        // hugely wasteful dup, but who cares for now
        section->name = strcpy_dup(name);
        section->relocations = hl_make();
        sm_put(&bin->sections, name, section, true);
        return section;
    } else {
        return sm_get(&bin->sections, name);
    }
}

struct bin_file elf_read(FILE* f) {
    struct bin_file out = {
        .labels = sm_make(),
        .sections = sm_make()
    };

    Elf32_Ehdr header;
    fread(&header, sizeof(Elf32_Ehdr), 1, f);

    Elf32_Shdr* section_headers = malloc(sizeof(Elf32_Shdr) * header.e_shnum);
    fseek(f, header.e_shoff, SEEK_SET);
    fread(section_headers, header.e_shnum, sizeof(Elf32_Shdr), f);

    // Probably wasteful to just load all the sections into memory, but it's
    // fine for now
    void** section_contents = malloc(header.e_shnum * sizeof(void*));
    for(size_t i = 0; i < header.e_shnum; i++) {
        if(section_headers[i].sh_size > 0) {
            section_contents[i] = malloc(section_headers[i].sh_size);
            fseek(f, section_headers[i].sh_offset, SEEK_SET);
            fread(section_contents[i], 1, section_headers[i].sh_size, f);
        } else {
            section_contents[i] = NULL;
        }
    }

    char* shstrtab = section_contents[header.e_shstrndx];

    for(size_t i = 0; i < header.e_shnum; i++) {
        if(section_headers[i].sh_type == SHT_PROGBITS) {
            const char* name = shstrtab + section_headers[i].sh_name;
            struct bin_section* section = ensure_section(&out, name);
            section->data = section_contents[i];
            section_contents[i] = NULL;
            section->data_sz = section_headers[i].sh_size;
        } else if(section_headers[i].sh_type == SHT_REL) {
            Elf32_Rel* rels = section_contents[i];

            size_t target_section_index = section_headers[i].sh_info;
            size_t symtab_index = section_headers[i].sh_link;
            
            Elf32_Sym* syms = section_contents[symtab_index];
            size_t strtab_index = section_headers[symtab_index].sh_link;
            const char* strtab = section_contents[strtab_index];
            
            const char* name = shstrtab + section_headers[target_section_index].sh_name;
            struct bin_section* section = ensure_section(&out, name);
            for(size_t i = 0; i < section_headers[i].sh_size / sizeof(Elf32_Rel); i++) {
                struct relocation* rel = malloc(sizeof(struct relocation));
                rel->symbol = strcpy_dup(strtab + ELF32_R_SYM(rels[i].r_info));
                rel->type = ELF32_R_TYPE(rels[i].r_info);
                rel->offset = rels[i].r_offset;
                hl_append(&section->relocations, rel);
            }
        }
    }


    for(size_t i = 0; i < header.e_shnum; i++) {
        if(section_contents[i]) free(section_contents[i]);
    }
    free(section_contents);
    free(section_headers);

    return out;
}