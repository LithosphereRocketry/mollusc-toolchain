#ifndef ARCH_LINK_H
#define ARCH_LINK_H

#include "arch.h"

                    // struct bin_label*
void link_section(struct string_map* labels, struct bin_section* section, const struct string_map* section_offsets);

void link_file(struct asm_result* binfile, const struct string_map* section_offsets);

#endif