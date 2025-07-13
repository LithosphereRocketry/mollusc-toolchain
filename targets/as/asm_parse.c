#include "asm_parse.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "strtools.h"
#include "asm_instrs.h"
#include "arch.h"

static bool valid_in_name(char c, size_t pos) {
    return c == '_'
        || isalpha(c)
        || (isdigit(c) && pos != 0);
}

static void parse_err(const char* msg, const char* filename, const char* text, size_t lineno) {
    fprintf(stderr, "Error at %s line %zu: %s\n> %.*s\n",
                    filename, lineno, msg, (int) (eol(text)-text), text);
}

static struct parse_section* add_section(struct parse_result* res, const char* name) {
    // if a section already exists in the table, reuse it
    struct parse_section* old_section = sm_get(&res->sections, name);
    if(old_section) return old_section;
    // if the section doesn't already exist, we need to build one
    struct parse_section* new_section = malloc(sizeof(struct parse_section));

    // fill in the actual section fields
    new_section->name = name; // todo: lifetime?
    new_section->globals = hl_make();
    new_section->instrs = hl_make();
    new_section->instr_labels = sm_make();

    sm_put(&res->sections, name, new_section);
    return new_section;
}

static const char* parse_name(const char* text, char** name) {
    text = ftrim(text);
    const char* endname = text;
    while(valid_in_name(*endname, endname-text)) endname++;
    if(endname == text) return NULL;
    *name = strncpy_dup(text, endname-text);
    return endname;
}

struct parse_result asm_parse(const char* text, const char* filename) {
    struct parse_result result = {
        .filenames = hl_make(),
        .sections = sm_make()
    };
    // mildly hacky to avoid double free
    char* fn = strcpy_dup(filename);
    hl_append(&result.filenames, fn);
    size_t lineno = 1;
    const char* nextpos;
    struct parse_section* current_section = NULL;
    bool line_full = false;
    while(1) {
        if(*text == '\0') break;
        else if(isspace(*text)) {
            if(*text == '\n') {
                lineno++;
                line_full = false;
            }
            text++;
        } else if(*text == ';') {
            text = eol(text);
        } else if(line_full) {
            // anything past this point must be on a fresh line
            parse_err("Unexpected text after statement", fn, text, lineno);
            exit(-1);
        } else if((nextpos = (startswith("# ", text)))){
            int nchar;
            size_t new_lineno;
            if(sscanf(nextpos, "%lu \"%n", &new_lineno, &nchar) != 1) {
                parse_err("Note: malformed line number annotation", fn, text, lineno);
            } else {
                lineno = new_lineno;
                nextpos += nchar;
                const char* endstr = strchr(nextpos, '\"');
                char* new_filename = strncpy_dup(nextpos, endstr-nextpos);
                fn = new_filename;
                hl_append(&result.filenames, fn);
                nextpos = endstr + 1;
            }
            text = eol(nextpos) + 1;
        } else if((nextpos = startswith(".section", text))) {
            char* name;
            const char* end_name = parse_name(nextpos, &name);
            if(!end_name) {
                parse_err("Unrecognized syntax in section declaration",
                        fn, text, lineno);
                exit(-1);
            }
            current_section = add_section(&result, name);
            text = end_name;
            line_full = true;
        } else if((nextpos = startswith(".global", text))) {
            if(!current_section) {
                parse_err("No section active", fn, text, lineno);
                exit(-1);
            }
            char* name;
            const char* end_name = parse_name(nextpos, &name);
            if(!end_name) {
                parse_err("Unrecognized syntax in section declaration",
                        fn, text, lineno);
                exit(-1);
            }
            hl_append(&current_section->globals, name);
            text = end_name;
            line_full = true;
        } else if((nextpos = strchr(text, ':'))) {
            if(!current_section) {
                parse_err("No section active", fn, text, lineno);
                exit(-1);
            }
            char* name;
            const char* endlbl = parse_name(ftrim(text), &name);
            if(ftrim(endlbl) != nextpos) {
                parse_err("Invalid format for label", fn, text, lineno);
                exit(-1);
            }
            sm_put(&current_section->instr_labels, name, (void*) current_section->instrs.len);
            free(name);
            text = nextpos+1;
        } else if((nextpos = asm_parse_instr(fn, lineno, text, current_section))) {
            text = nextpos;
            line_full = true;
        } else {
            parse_err("Unrecognized syntax", fn, text, lineno);
            exit(-1);
        }
    }
    return result;
}

static void print_punned_addr(void* global, const char* key, void* value) {
    (void) global;
    printf("\t%s : %zx\n", key, (size_t) value);
}

void print_section(const struct parse_section* sect) {
    printf("SECTION %s\nGLOBALS\n", sect->name);
    for(size_t i = 0; i < sect->globals.len; i++) {
        printf("\t%s\n", (char*) sect->globals.buf[i]);
    }
    printf("INSTRUCTION LABELS\n");
    sm_foreach(&sect->instr_labels, print_punned_addr, NULL);
    printf("INSTRUCTIONS\n");
    for(size_t i = 0; i < sect->instrs.len; i++) {
        const struct parse_instr* ins = sect->instrs.buf[i];
        if(ins->pred != NULL) {
            printf("%c%s", ins->pred_inv ? '!' : '?', ins->pred);
        }
        printf("\t%s\t", arch_mnemonics[ins->type]);
        for(size_t j = 0; j < ins->args.len; j++) {
            if(j != 0) printf(", ");
            printf("%s", (char*) ins->args.buf[j]);
        }
        printf("\n");
    }
}

static void print_section_wrapper(void* global, const char* key, void* value) {
    // intentionally ignore without warnings
    (void) global; (void) key;
    print_section(value);
}

void print_parse(const struct parse_result* res) {
    sm_foreach(&res->sections, print_section_wrapper, NULL) ;
}