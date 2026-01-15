#include <stdio.h>
#include <stdlib.h>

#include "argparse.h"
#include "iotools.h"
#include "asm_parse.h"
#include "assemble.h"

argument_t arg_out = {'o', NULL, true, {NULL}};

argument_t* args[] = {
    &arg_out
};

void assemble_section(void* global, const char* name, void* value) {
    struct string_map* bin_sections = global;
    struct parse_section* section = value;
    struct bin_section* res = malloc(sizeof(struct bin_section));
    *res = assemble(section);
    sm_put(bin_sections, name, res);
}

void print_destroy_asm_section(void* global, const char* name, void* res) {
    (void) global, (void) name;
    printf("Assembly for section \"%s\":\n", name);
    // print_assembly(res);
    destroy_assembly(res);
}

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

    struct string_map bin_sections = sm_make();
    sm_foreach(&parsed.sections, assemble_section, &bin_sections);

    struct bin_section* textbin = sm_get(&bin_sections, "text");
    if(!textbin) {
        fprintf(stderr, "No text section in assembly!\n");
        exit(-1);
    }
    if(textbin->relocations.len > 0) {
        fprintf(stderr, "Text section has unresolved labels\n");
        print_assembly(textbin);
        exit(-1);
    }
    const char* outname = "a.out";
    if(arg_out.result.value) outname = arg_out.result.value;
    FILE* outfile = fopen(outname, "w");
    fwrite(textbin->data, sizeof(arch_word_t), textbin->data_sz, outfile);
    fclose(outfile);

    sm_foreach(&bin_sections, print_destroy_asm_section, NULL);
    sm_destroy(&bin_sections, true);
    
    destroy_parse(&parsed);
}