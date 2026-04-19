#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argparse.h"
#include "iotools.h"
#include "asm_parse.h"
#include "arch_elf.h"
#include "assemble.h"
#include "arch_link.h"

argument_t arg_out = {.abbr='o', .name=NULL, .hasval=true, .result={.value = NULL},
    .help = "Filename for output"};
argument_t arg_bin = {.abbr='B', .name=NULL, .hasval=false, .result={.present = false},
    .help = "Force plain binary output (instead of ELF)"};

argument_t* args[] = {
    &arg_out,
    &arg_bin
};

int main(int argc, char** argv) {
    argc = argparse(args, sizeof(args) / sizeof(argument_t*), argc, argv);

    if(argc <= 1) {
        fprintf(stderr, "No file given!\n");
        exit(-1);
    } else if(argc > 2) {
        fprintf(stderr, "Too many files given! (Starting with %s)\n", argv[2]);
        exit(-1);
    }

    char* filename = argv[1];
    FILE* file = fopen(filename, "r");
    if(!file) {
        fprintf(stderr, "Failed to open source file %s", filename);
        exit(-1);
    }
    // I'm operating under the assumption here that any assembly file worth
    // using on this system will fit in system memory
    char* filetext = fread_dup(file);
    fclose(file);
    if(!filetext) {
        fprintf(stderr, "Ouch, we ran out of memory or something\n");
        exit(-1);
    }

    struct parse_result parsed = asm_parse(filetext, filename);
    free(filetext);

    struct asm_result assembled = assemble(&parsed);

    const char* outname = "a.out";
    if(arg_out.result.value) outname = arg_out.result.value;
    FILE* outfile = fopen(outname, "w");
    if(!outfile) {
        fprintf(stderr, "Failed to open file %s for writing\n", outname);
        exit(1);
    }

    if(arg_bin.result.present) {
        arch_word_t vector_table[32];
        memset(vector_table, 0, sizeof(vector_table));

        struct bin_section* textbin = sm_get(&assembled.sections, ".text");
        if(!textbin) {
            fprintf(stderr, "ROM link error: text section does not exist\n");
            exit(-1);
        }
        struct bin_section* rodatabin = sm_get(&assembled.sections, ".rodata");

        struct string_map rom_link_config = sm_make();
        sm_put(&rom_link_config, ".text", (void*) sizeof(vector_table), false);
        sm_put(&rom_link_config, ".rodata", (void*) sizeof(vector_table)
                + textbin->data_sz*sizeof(arch_word_t), false);
        link_file(&assembled, &rom_link_config);

        struct bin_label* start_label = sm_get(&assembled.labels, "_start");
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

    } else {
        elf_write(outfile, &assembled, ET_REL);
    }
    fclose(outfile);
    
    destroy_parse(&parsed);
}