#ifndef UTILS_HEADER
#define UTILS_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <pthread.h>

#define BUFFER_SIZE 128
#define BLOCKSIZE 512
#define CYLINDERS 128
#define SECTORS 32
#define ZONESIZE 64

typedef struct {
    signed int sockfd;
    signed char buffer[BUFFER_SIZE];
} buf_sock;

void itoa(char *str, long val);
void get_toks(char *str, char **toks, int size_of_toks);

buf_sock *gen_server_sock(int port_no);
buf_sock *get_client_sock(char *hostname, int port_no);
void free_sock(buf_sock *sock);

int send_msg(buf_sock *sock, char *msg);
char *get_msg(buf_sock *sock);

int send_data(buf_sock *sock, int len);
int get_data(buf_sock *sock, int len);

void init_str(buf_sock *sock);
void cat_str(buf_sock *sock, char *str);
void cat_int(buf_sock *sock, long val);
int send_str(buf_sock *sock);

typedef struct {
    unsigned char free_lst[256];
} bitmap_t;

#define NUM_OF_DENTRY_SUBS 4

typedef struct {
    signed short parent;
    signed short mode;
    signed short owner;
    struct {
        signed short id;
        signed char name[10];
    } subs[NUM_OF_DENTRY_SUBS];
    signed short next;
    unsigned long time;
} dentry_t;

#define NUM_OF_DZONE_SUBS 5

typedef struct {
    signed short parent;
    struct {
        signed short id;
        signed char name[10];
    } subs[NUM_OF_DZONE_SUBS];
    signed short next;
} dir_zone_t;

typedef struct {
    signed short parent; 
    signed short mode;
    signed short owner;
    signed short size;
    signed short ptrs[24];  /* 20, 2, 2 */
    unsigned long time;
} inode_t;

#define MAX_FILE_LEN 8340

typedef struct {
    signed char data[64];
} data_zone_t;

typedef struct {
    signed short zones[32];
} idx_zone_t;

typedef union {
    dentry_t dentry_val;
    dir_zone_t dzone_val;
    inode_t inode_val;
    data_zone_t data_zone_val;
    idx_zone_t idx_zone_val;
} zone_t;

typedef struct {
    zone_t zones[BLOCKSIZE/ZONESIZE];
    unsigned int c, s;
} block_t;

void read_block(buf_sock *sock, int c, int s, block_t *dest);
void write_block(buf_sock *sock, block_t *src);

#endif  /* UTILS_HEADER */

