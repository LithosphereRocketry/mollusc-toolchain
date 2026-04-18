#include "arch_elf.h"

#include <stdlib.h>
#include <string.h>
#include "arch.h"
#include "misctools.h"

// struct elf_write_common {
//     FILE* f;
//     int error;
// };

// static void elf_write_section(void* global, const char* name, void* value) {
//     struct elf_write_common* common = global;
//     struct bin_section* section = value;


// }

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

int elf_write(FILE* f, const struct string_map* sections, Elf32_Half type) {
    if(type != ET_REL) {
        fprintf(stderr, "ELF type %i not supported\n", type);
        exit(1);
    }

    // It should be possible to know how many sections we're going to use ahead
    // of time - we have a couple constant sections, and then one for each of
    // our assembly sections
    const size_t n_sections = 2;


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
        .sh_size = align_roundup(sz_init_shstrtab + sm_stsize(sections), 4),
        .sh_link = 0, // I don't think a section header string table needs any
        // linking information
        .sh_addralign = 0,
        .sh_entsize = 0
    };
    section_headers[1] = shdr_shstrtab;
    // Write section header names into file
    char* shstrtab_buffer = malloc(shdr_shstrtab.sh_size);
    memset(shstrtab_buffer, 0, shdr_shstrtab.sh_size);
    memcpy(shstrtab_buffer, INIT_SHSTRTAB, sz_init_shstrtab);
    memcpy(shstrtab_buffer + sz_init_shstrtab, sm_stringtable(sections),
            sm_stsize(sections));
    fwrite(shstrtab_buffer, 1, shdr_shstrtab.sh_size, f);


    Elf32_Word shoff = ftell(f);
    fwrite(section_headers, sizeof(Elf32_Shdr), n_sections, f);
    // Correct the section header location now that we know where it is
    fseek(f, offsetof(Elf32_Ehdr, e_shoff), SEEK_SET);
    fwrite(&shoff, sizeof(Elf32_Word), 1, f);
    return 0;
}