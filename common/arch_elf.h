#ifndef ARCH_ELF_H
#define ARCH_ELF_H

#include <stdio.h>
#include <elf.h>

// big arbitrary number for architecture ID to hopefully not collide with any 
// existing CPUs
#define EM_MOLLUSC (0x739F)

#include "structures.h"
#include "arch.h"

// String map of struct bin_section
// Retuns 0 on success, error code on failure
int elf_write(FILE* f, const struct asm_result* bin, Elf32_Half type);

#endif