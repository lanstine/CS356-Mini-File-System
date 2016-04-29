#ifndef ALLOC_HEADER
#define ALLOC_HEADER

#include "utils.h"

#define UNOCCUPIED 0
#define D_OCCUPIED 1
#define F_OCCUPIED 2
#define I_OCCUPIED 3

unsigned char get_state(signed short id); 
signed short get_free_id(unsigned char state);
void set_free_id(signed short id);
void format(void);

void alloc_init(int port_no);
void alloc_final(void);

zone_t *read_zone(signed short id);
void write_zone(signed short id, zone_t *zone);

#endif /* ALLOC_HEADER */

