#include "strtools.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

const char* eol(const char* str) {
    const char* ptr = str;
    while(*ptr != '\0' && *ptr != '\n') {
        ptr++;
    }
    return ptr;
}

const char* startswith(const char* pattern, const char* str) {
    size_t patlen = strlen(pattern);
    if(!strncmp(pattern, str, patlen)) {
        return str + patlen;
    } else {
        return NULL;
    }
}

const char* ftrim(const char* str) {
    while(isspace(*str)) str++;
    return str;
}

char *strncpy_dup(const char *str, size_t n) {
    size_t slen = strlen(str);
    if(slen > n) slen = n;
    char* result = malloc(slen+1);
    memcpy(result, str, slen);
    result[slen] = '\0';
    return result;
}
