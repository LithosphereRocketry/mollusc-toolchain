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

void assemble_section(void* global, const char* name, void* section) {
    (void) global;
    struct assembly_result res = assemble(section);
    printf("Assembly for section \"%s\":\n", name);
    print_assembly(&res);
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
    if(!filetext) {
        fprintf(stderr, "Ouch, we ran out of memory or something\n");
        exit(-1);
    }

    struct parse_result parsed = asm_parse(filetext, filename);
    free(filetext);
    printf("Parse result:\n");
    print_parse(&parsed);

    sm_foreach(&parsed.sections, assemble_section, NULL);
}