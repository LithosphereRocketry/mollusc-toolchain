#ifndef LD_LINK_H
#define LD_LINK_H

#include "arch.h"

void link_incremental(struct bin_file* target, struct bin_file* incoming);

#endif