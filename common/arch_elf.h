#ifndef ARCH_ELF_H
#define ARCH_ELF_H

#include <stdio.h>

#include "structures.h"

// String map of struct bin_section
// Retuns 0 on success, error code on failure
int elf_write(FILE* f, const struct string_map* sections);

#endif