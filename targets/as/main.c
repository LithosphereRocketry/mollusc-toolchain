#include <stdio.h>
#include <stdlib.h>

#include "argparse.h"
#include "iotools.h"

argument_t arg_out = {'o', NULL, true, {NULL}};

argument_t* args[] = {
    &arg_out
};

int main(int argc, char** argv) {
    argc = argparse(args, sizeof(args) / sizeof(argument_t*), argc, argv);

    if(argc <= 1) {
        fprintf(stderr, "No file given!\n");
        exit(-1);
    } if(argc > 2) {
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
    printf("%s\n", filetext);
}