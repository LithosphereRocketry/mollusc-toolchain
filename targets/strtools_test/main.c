#include <stdio.h>

#include "strtools.h"

int main(int argc, char** argv) {
    const char* word = "horsebooks";
    const char* pattern = "ooks";
    const char* result = endswith(pattern, word);
    printf("%zd %s\n", result-word, result);

    pattern = "s";
    result = endswith(pattern, word);
    printf("%zd %s\n", result-word, result);
}