#include "arch_elf.h"

#include <stdlib.h>
#include "arch.h"

struct elf_write_common {
    FILE* f;
    int error;
};

static void elf_write_section(void* global, const char* name, void* value) {
    struct elf_write_common* common = global;
    struct bin_section* section = value;


}

int elf_write(FILE* f, const struct string_map* sections, Elf32_Half type) {
    if(type != ET_REL) {
        fprintf(stderr, "ELF type %i not supported\n", type);
        exit(1);
    }
    Elf32_Ehdr hdr = {
        .e_ident = {
            0x7F, 'E', 'L', 'F',
        },
        .e_type = type,
        .e_machine = EM_MOLLUSC,
        .e_version = 1,
        .e_entry = 0,
        .e_phoff = 0,
    };

    fwrite(&hdr, sizeof(hdr), 1, f);

    struct elf_write_common common = {
        .f = f,
        .error = 0
    };
    sm_foreach(sections, elf_write_section, &common);
    return common.error;
}