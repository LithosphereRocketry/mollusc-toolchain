#include "structures.h"
#include <stdio.h>

bool filter(const char* key, const void* value) {
    return *key == 'f' || (size_t) value > 7;
}

int main(int argc, char** argv) {
    (void) argc, (void) argv;
    struct string_map map = sm_make();

    sm_put(&map, "one", (void*) 1, false);
    sm_put(&map, "two", (void*) 2, false);
    sm_put(&map, "three", (void*) 3, false);
    sm_put(&map, "four", (void*) 4, false);
    sm_put(&map, "five", (void*) 5, false);
    sm_put(&map, "six", (void*) 6, false);
    sm_put(&map, "seven", (void*) 7, false);
    sm_put(&map, "eight", (void*) 8, false);
    sm_put(&map, "nine", (void*) 9, false);

    sm_print(&map);

    printf("%p\n", sm_get(&map, "one"));
    printf("%p\n", sm_get(&map, "two"));
    printf("%p\n", sm_get(&map, "three"));
    printf("%p\n", sm_get(&map, "four"));

    sm_filter(&map, filter);

    sm_print(&map);

    sm_destroy(&map);
}