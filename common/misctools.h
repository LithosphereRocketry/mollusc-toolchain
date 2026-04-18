#ifndef MISCTOOLS_H
#define MISCTOOLS_H

#include <stdint.h>
#include <stdlib.h>

uint32_t signExtend(uint32_t value, uint32_t bits);

size_t align_roundup(size_t n, size_t base);

#endif