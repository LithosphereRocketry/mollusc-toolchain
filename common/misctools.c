#include "misctools.h"

uint32_t signExtend(uint32_t value, uint32_t bits) {
	uint32_t mask = ~((1 << bits) - 1);
	if(value & (1 << (bits-1))) {
		return value | mask;
	} else {
		return value & ~mask;
	}
}

size_t align_roundup(size_t n, size_t base) {
	size_t nudged_n = n + base - 1;
	return nudged_n - (nudged_n % base);
}