#include <stdio.h>

#include "arch_elf.h"

int main(int argc, char** argv) {
    FILE* elffile = fopen(argv[1], "r");
    struct bin_file binfile = elf_read(elffile);
    sm_print(&binfile.sections);
}