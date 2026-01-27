#include "arch_elf.h"

#include <elf.h>

#include "arch.h"

struct elf_write_common {
    FILE* f;
    int error;
};

static void elf_write_section(void* global, const char* name, void* value) {
    struct elf_write_common* common = global;
    struct bin_section* section = value;


}

int elf_write(FILE* f, const struct string_map* sections) {
    Elf32_Ehdr hdr = {
        .e_ident = "\x7F" "ELF"
    };

    fwrite(&hdr, sizeof(hdr), 1, f);

    struct elf_write_common common = {
        .f = f,
        .error = 0
    };
    sm_foreach(sections, elf_write_section, &common);
    return common.error;
}