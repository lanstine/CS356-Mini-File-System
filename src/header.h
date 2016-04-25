#ifndef MINI_FS_HEADER
#define MINI_FS_HEADER

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

typedef struct {
    signed int sockfd;
    signed char buffer[BUFFER_SIZE];
} buf_sock;

void itoa(char *str, int val);
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
void cat_int(buf_sock *sock, int val);
int send_str(buf_sock *sock);

#endif  /* MINI_FS_HEADER */
