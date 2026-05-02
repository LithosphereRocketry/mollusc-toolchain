#include <stddef.h>
#include <stdio.h>

#include "arch.h"
#include "argparse.h"
#include "arch_elf.h"
#include "structures.h"
#include "ld_link.h"

argument_t arg_out = {.abbr='o', .name=NULL, .hasval=true, .result={.value = "a.out"},
    .help = "Filename for output"};
argument_t arg_outtype = {.abbr='b', .name=NULL, .hasval=true, .result={.value = "elf"},
    .help = "File type for output, currently supports 'elf` or 'binary'"};

argument_t* args[] = {
    &arg_out,
    &arg_outtype
};

int main(int argc, char** argv) {
    argc = argparse(args, sizeof(args) / sizeof(argument_t*), argc, argv);
    
    struct bin_file output = {
        .labels = sm_make(),
        .sections = sm_make()
    };
    for(int i = 1; i < argc; i++) {
        printf("Reading ELF file %s\n", argv[i]);
        FILE* elffile = fopen(argv[i], "r");
        struct bin_file binfile = elf_read(elffile);
        link_incremental(&output, &binfile);
    }
    print_assembly(&output);
}