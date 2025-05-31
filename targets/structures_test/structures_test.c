#include "structures.h"
#include <stdio.h>

int main(int argc, char** argv) {
    struct string_map map = sm_make();

    sm_put(&map, "one", (void*) 1);
    sm_put(&map, "two", (void*) 2);
    sm_put(&map, "three", (void*) 3);
    sm_put(&map, "four", (void*) 4);
    sm_put(&map, "five", (void*) 5);
    sm_put(&map, "six", (void*) 6);
    sm_put(&map, "seven", (void*) 7);
    sm_put(&map, "eight", (void*) 8);
    sm_put(&map, "nine", (void*) 9);

    sm_print(&map);

    printf("%p\n", sm_get(&map, "one"));
    printf("%p\n", sm_get(&map, "two"));
    printf("%p\n", sm_get(&map, "three"));
    printf("%p\n", sm_get(&map, "four"));

    sm_destroy(&map, false);
}