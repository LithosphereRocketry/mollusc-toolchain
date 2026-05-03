#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "arch.h"
#include "arch_link.h"
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
        FILE* elffile = fopen(argv[i], "r");
        struct bin_file binfile = elf_read(elffile);
        link_incremental(&output, &binfile);
    }
    link_file(&output, NULL);

    FILE* outfile = fopen(arg_out.result.value, "w");
    if(!strcmp(arg_outtype.result.value, "elf")) {
        // TODO: actually output to an executable ELF file
        print_assembly(&output);
    } else if(!strcmp(arg_outtype.result.value, "binary")) {
        // TODO: define ROM with linker scripts instead of hardcoding
        arch_word_t vector_table[32];
        memset(vector_table, 0, sizeof(vector_table));

        struct bin_section* textbin = sm_get(&output.sections, ".text");
        if(!textbin) {
            fprintf(stderr, "ROM link error: text section does not exist\n");
            exit(-1);
        }
        struct bin_section* rodatabin = sm_get(&output.sections, ".rodata");

        struct string_map rom_link_config = sm_make();
        sm_put(&rom_link_config, ".text", (void*) sizeof(vector_table), false);
        sm_put(&rom_link_config, ".rodata", (void*) sizeof(vector_table)
                + textbin->data_sz*sizeof(arch_word_t), false);
        link_file(&output, &rom_link_config);

        struct bin_label* start_label = sm_get(&output.labels, "_start");
        if(!start_label || strcmp(start_label->section, ".text")) {
            fprintf(stderr, "ROM link error: text section has no entry point\n");
            exit(-1);
        }

        if(textbin->relocations.len > 0) {
            fprintf(stderr, "Could not resolve symbols in .text:\n");
            for(size_t i = 0; i < textbin->relocations.len; i++) {
                fprintf(stderr, "\t%s\n",
                        ((struct relocation*) textbin->relocations.buf[i])->symbol);
            }
            exit(-1);
        }
        if(rodatabin && rodatabin->relocations.len > 0) {
            fprintf(stderr, "Could not resolve symbols in .rodata:\n");
            for(size_t i = 0; i < textbin->relocations.len; i++) {
                fprintf(stderr, "\t%s\n",
                        ((struct relocation*) textbin->relocations.buf[i])->symbol);
            }
            exit(-1);
        }

        vector_table[0] = sizeof(vector_table) + start_label->offset;
        fwrite(vector_table, sizeof(arch_word_t), 32, outfile);
        fwrite(textbin->data, sizeof(arch_word_t), textbin->data_sz, outfile);
        if(rodatabin) {
            fwrite(rodatabin->data, sizeof(arch_word_t), rodatabin->data_sz, outfile);            
        }

        sm_destroy(&rom_link_config);
    }
}