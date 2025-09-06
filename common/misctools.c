#include "misctools.h"

uint32_t signExtend(uint32_t value, uint32_t bits) {
	uint32_t mask = ~((1 << bits) - 1);
	if(value & (1 << (bits-1))) {
		return value | mask;
	} else {
		return value & ~mask;
	}
}