#ifndef ALLOC_HEADER
#define ALLOC_HEADER

#include "utils.h"

#define UNOCCUPIED 0
#define D_OCCUPIED 1
#define F_OCCUPIED 2
#define I_OCCUPIED 3

signed short get_free_zone(unsigned char state);
void set_free_zone(signed short id);
void format(void);

void alloc_init(int port_no);
void alloc_final(void);

zone_t *read_zone(signed short id);
void write_zone(signed short id, zone_t *zone);

#endif /* ALLOC_HEADER */

