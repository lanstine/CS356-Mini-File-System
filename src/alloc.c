#include "alloc.h"


#define get_c(id) (((id)*ZONESIZE/BLOCKSIZE) / SECTORS)
#define get_s(id) (((id)*ZONESIZE/BLOCKSIZE) % SECTORS)

static buf_sock *sock;

static bitmap_t map;

#define CACHE_SIZE 8
#define CACHE_GRP_SZ 2
#define CACHE_GRP_NUM (CACHE_SIZE/CACHE_GRP_SZ)

static block_t *b_cache[CACHE_SIZE] = {};

static void set_state(signed short id, unsigned char state);
static void free_cache(void);
static block_t *get_block(unsigned short id);


unsigned char get_state(signed short id)
{
    if (id < 4) {
        return F_OCCUPIED;
    } else {
        unsigned char byte;

        byte = map.free_lst[id>>2];
        byte >>= ((id&3)<<1);
        return byte & 3;
    }
}

static void set_state(signed short id, unsigned char state)
{
    if (id >= 4) {
        unsigned char mask;

        mask = (3 << ((id&3)<<1));
        map.free_lst[id>>2] &= (~mask);
        map.free_lst[id>>2] |= ((state&3)<<((id&3)<<1));
    }
}

signed short get_free_id(unsigned char state)
{
    int id;

    for (id=4; id<=(1<<15)-1; id++) {
        if (UNOCCUPIED == get_state(id)) {
            set_state(id, state);
            return id;
        }
    }
    fprintf(stderr, "Error: failed to get free ID.\n");
    return -1;
}

void set_free_id(signed short id)
{
    if (id >= 4) {
        set_state(id, UNOCCUPIED);
    } else {
        fprintf(stderr, "Error: cannot set free zone %hi.\n", id);
    }
}


static void free_cache(void)
{
    int idx;

    for (idx=0; idx<CACHE_SIZE; idx++) {
        if (NULL != b_cache[idx]) {
            free(b_cache[idx]);
            b_cache[idx] = NULL;
        }
    }
}

static block_t *get_block(unsigned short id)
{
    int c = get_c(id);
    int s = get_s(id);
    int grp_idx = s % CACHE_GRP_NUM;
    int start_idx = grp_idx*CACHE_GRP_SZ, idx;
    block_t *target;

    for (idx=start_idx; idx<start_idx+CACHE_GRP_SZ-1; idx++) {
        if (NULL == b_cache[idx]) {
            break;
        } else if (c == b_cache[idx]->c 
                && s == b_cache[idx]->s) {
            break;
        }
    }
    target = b_cache[idx];
    while (idx > start_idx) {
        b_cache[idx] = b_cache[idx-1];
        idx--;
    }
    if (NULL == target) {
        target = (block_t*) calloc(1, sizeof(block_t));
        read_block(sock, c, s, target); 
    } else if (c != target->c || s != target->s) {
        read_block(sock, c, s, target);
    }
    return b_cache[start_idx] = target;
}

void format(void)
{
    block_t block;

    block.c = block.s = 0;
    bzero((char*)&block, BLOCKSIZE);
    write_block(sock, &block);
    bzero((char*)&map, 256);
    set_state(4, D_OCCUPIED);
    free_cache();
}

void alloc_init(int port_no)
{
    sock = get_client_sock("localhost", port_no);
    if (NULL == sock) {
        perror("failed to get client socket from disk server");
        exit(1);
    } else {
        block_t *block = get_block(0);

        memcpy(map.free_lst, (char*)block, sizeof(map.free_lst));
    }
}

void alloc_final(void)
{
    block_t *block = get_block(0);

    memcpy((char*)block, map.free_lst, sizeof(map.free_lst));
    write_block(sock, block);
    do {
        send_msg(sock, "E");
    } while (strcmp(get_msg(sock), "BYE"));
    free_sock(sock);
    free_cache();
}

zone_t *read_zone(signed short id)
{
    if (id < 4) {
        fprintf(stderr, "Error: cannot read zone %d.\n", id);
        return NULL;
    } else {
        zone_t *b_zone = (zone_t*) get_block(id);
        signed short offset = id % (BLOCKSIZE/ZONESIZE);

        return b_zone+offset;
    }
}

void write_zone(signed short id, zone_t *src)
{
    if (id < 4 || NULL == src) {
        fprintf(stderr, "Error: failed to read zone %d.\n", id);
    } else {
        block_t *block = get_block(id);
        zone_t *dest = (zone_t*) block;
        signed short offset = id % (BLOCKSIZE/ZONESIZE);

        memcpy((char*)(dest+offset), (char*)src, sizeof(zone_t));
        write_block(sock, block);
    }
}

