#include "asm_instrs.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "arch.h"
#include "structures.h"
#include "strtools.h"

const char* asm_parse_instr(const char* file, size_t line,
        const char* text, struct parse_section* dest) {
    static struct string_map instr_map; // enum arch_instr
    static bool firstrun = true;
    if(firstrun) {
        instr_map = arr_inv_to_sm(arch_mnemonics, N_INSTRS);
    }

    const char* mnem_end = eow(text);
    // this can definitely be done without malloc but I'm lazy
    char* mnem_in = strncpy_dup(text, mnem_end - text);
    // clang likes to complain that this is an UB cast, but it should be fine(tm)
    // as long as enum roundtrips with void*
    enum arch_instr type = (enum arch_instr) sm_get(&instr_map, mnem_in);
    free(mnem_in);

    if(type == I_INVALID) {
        return NULL;
    }

    struct parse_instr* ins = malloc(sizeof(struct parse_instr));
    ins->file = file;
    ins->line = line;
    ins->type = type;
    ins->args = hl_make();

    text = mnem_end;
    const char* line_end = eol(text);
    while(1) {
        const char* next_comma = strchr(text, ',');
        if(!next_comma || next_comma > line_end) next_comma = line_end;
        const char* arg_start = ftrim(text);
        const char* arg_end = eow(arg_start);
        if(arg_end > next_comma) arg_end = next_comma;
        char* arg = strncpy_dup(arg_start, arg_end-arg_start);
        hl_append(&ins->args, arg);
        if(next_comma == line_end) {
            text = arg_end;
            break;
        } else {
            text = next_comma + 1;
        }
    }

    hl_append(&dest->instrs, ins);
    return text;
}