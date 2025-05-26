#include "asm_parse.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "strtools.h"

static bool valid_in_name(char c, size_t pos) {
    return c == '_'
        || isalpha(c)
        || (isdigit(c) && pos != 0);
}

static void parse_err(const char* msg, const char* text, size_t lineno) {
    fprintf(stderr, "Unrecognized syntax at line %zu: %s\n%*s",
                    lineno, msg, (int) (eol(text)-text), text);
}

static struct parse_section* active_section(const struct parse_result* res) {
    if(res->sections) {
        return res->sections + (res->n_sections-1);
    } else {
        return NULL;
    }
}

static struct parse_section* add_section(struct parse_result* res, const char* name) {
    res->n_sections ++;
    res->sections = realloc(res->sections,
            res->n_sections * sizeof(struct parse_section));
    struct parse_section* new_section = active_section(res);
    new_section->name = name;
    return new_section;
}

struct parse_result asm_parse(const char* text) {
    struct parse_result result = {
        .n_sections = 0,
        .sections = NULL
    };
    size_t lineno = 1;
    const char* nextpos;
    struct parse_section* current_section = active_section(&result);
    while(1) {
        if(*text == '\0') break;
        else if(isspace(*text)) {
            if(*text == '\n') lineno++;
            text++;
        } else if(*text == ';' || *text == '#') {
            // In future it might be nice to parse preprocessor line directives
            // but for now we can just ignore them
            text = eol(text);
        } else if((nextpos = startswith(".section", text))) {
            nextpos = ftrim(nextpos);
            const char* end_name = nextpos;
            while(!isspace(*end_name)) end_name++;
            
            for(const char* c = nextpos; c < end_name; c++) {
                if(!valid_in_name(*c, c - nextpos)) {
                    parse_err("Invalid character in name", text, lineno);
                    exit(-1);
                }
            }

            const char* section_name = strncpy_dup(nextpos, end_name-nextpos);
            add_section(&result, section_name);
            text = end_name;
        } else if((nextpos = startswith(".global", text))) {
            if(!current_section) {
                parse_err("No section active", text, lineno);
                exit(-1);
            }
            
        } else {
            parse_err("Unrecognized syntax", text, lineno);
            exit(-1);
        }
    }
    return result;
}

void print_section(const struct parse_section* sect) {
    printf("SECTION %s\n", sect->name);
}

void print_parse(const struct parse_result* res) {
    for(size_t i = 0; i < res->n_sections; i++) {
        print_section(res->sections+i);
    }
}