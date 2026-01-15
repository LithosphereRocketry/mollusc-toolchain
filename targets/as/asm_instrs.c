#include "asm_instrs.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "arch.h"
#include "arch_pseudo.h"
#include "structures.h"
#include "strtools.h"

const char* asm_parse_instr(const char* file, size_t line,
        const char* text, struct parse_section* dest) {
    static struct string_map instr_map; // enum arch_instr
    static bool firstrun = true;
    if(firstrun) {
        instr_map = arr_inv_to_sm(arch_mnemonics, N_INSTRS);
        for(size_t i = N_INSTRS; i < N_PSEUDOINSTRS; i++) {
            if(arch_pseudo_mnemonics[i]) {
                sm_put(&instr_map, arch_pseudo_mnemonics[i], (void*) i);
            }
        }
        firstrun = false;
    }

    bool pred_inv = true;
    const char* pred = NULL;
    if(*text == '?' || *text == '!') {
        if(*text == '?') pred_inv = false;
        text++;
        const char* pred_end = eow(text);
        pred = strncpy_dup(text, pred_end - text);
        text = ftrim(pred_end);
    }
    
    const char* mnem_end = eow(text);

    // this can definitely be done without malloc but I'm lazy
    char* mnem_in = strncpy_dup(text, mnem_end - text);
    char* mode_in;
    // split string into mnemonic and mode
    if((mode_in = strchr(mnem_in+1, '.'))) {
        *mode_in = '\0';
        mode_in++;
    }
    const char* mode_str = mode_in ? strcpy_dup(mode_in) : NULL;
    // clang likes to complain that this is an UB cast, but it should be fine(tm)
    // as long as enum roundtrips with void*
    enum arch_pseudoinstr type = (enum arch_pseudoinstr) sm_get(&instr_map, mnem_in);
    free(mnem_in);

    if(type == (enum arch_pseudoinstr) I_INVALID) {
        return NULL;
    }

    struct parse_instr* ins = malloc(sizeof(struct parse_instr));
    ins->file = file;
    ins->line = line;
    ins->pred_inv = pred_inv;
    ins->pred = pred;
    ins->type = type;
    ins->mode = mode_str;
    ins->args = hl_make();

    text = mnem_end;
    // todo: this is a slightly hacky way to handle comments and probably could
    // be done better
    const char* line_end = eol(text);
    for(const char* p = text; p < line_end; p++) {
        if(*p == ';') {
            line_end = p;
            break;
        }
    }
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