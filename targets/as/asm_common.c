#include "asm_common.h"

#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "strtools.h"

static bool valid_in_name(char c, size_t pos) {
    return c == '_'
        || isalpha(c)
        || (isdigit(c) && pos != 0);
}

const char* parse_name(const char* text, char** name) {
    text = ftrim(text);
    const char* endname = text;
    while(valid_in_name(*endname, endname-text)) endname++;
    if(endname == text) return NULL;
    *name = strncpy_dup(text, endname-text);
    return endname;
}