#include "utils.h"


static buf_sock *sock = NULL;

static void err_exit(char *msg)
{
    fprintf(stderr, "Error: %s\n.", msg);
    if (NULL != sock) {
        free_sock(sock);
    }
    exit(1);
}

static void cmd_test(void)
{
    size_t cmd_size = 128;
    char *cmd = (char*)malloc(cmd_size+1);

    while (1) {
        printf("Command > ");
        fflush(stdout);
        getline(&cmd, &cmd_size, stdin);
        cmd[strlen(cmd)-1] = '\0';
        send_msg(sock, cmd);
        get_msg(sock);
        printf("Get Response > %s\n", sock->buffer);
        if (!strcmp(sock->buffer, "BYE")) {
            break;
        } else if (!strncmp(sock->buffer, "FOLLOW ", 7)) {
            int num = atoi(sock->buffer+7);

            printf("Get Data: ");
            while (num > 0) {
                if (send_msg(sock, "EXPECT") < 0) {
                    err_exit("failed to write to socket");
                } else {
                    int len = get_data(sock, num);

                    if (len < 0) {
                        err_exit("failed to read from socket");
                    } else {
                        int i;

                        for (i=0; i<len; i++) {
                            printf("%c", sock->buffer[i]);
                        }
                        fflush(stdout);
                        num -= len;
                    }
                }
            }
            printf("\n");
        } else if (!strcmp(sock->buffer, "EXPECT")) {
            while (strcmp(sock->buffer, "OVER")) {
                printf("Data > ");
                fflush(stdout);
                bzero(sock->buffer, BUFFER_SIZE);
                getline(&cmd, &cmd_size, stdin);
                cmd[strlen(cmd)-1] = '\0';
                strcat(sock->buffer, cmd);
                if (send_data(sock, BUFFER_SIZE) < 0) {
                    err_exit("failed to write to socket");
                } else {
                    get_msg(sock);
                    printf("Get Response > %s\n", sock->buffer);
                }
            }
        }
    }
    free(cmd);
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: bin/disk_test <DISK_SERVER_PORT_NO>\n");
        exit(0);
    } else {
        sock = get_client_sock("localhost", atoi(argv[1]));
        if (NULL == sock) {
            err_exit("failed to get client socket");
        } else {
            block_t *a = (block_t*) calloc(1, sizeof(block_t));
            block_t *b = (block_t*) calloc(1, sizeof(block_t));
            int test;

            for (test=0; test<100; test++) {
                char *byte, *bbyte;

                for (byte=(char*)a; byte<(char*)a+BLOCKSIZE; byte++) {
                    *byte = (char) rand();
                }
                a->c = rand()%CYLINDERS;
                a->s = rand()%SECTORS;
                write_block(sock, a); 
                read_block(sock, a->c, a->s, b);
                for (byte=(char*)a, bbyte=(char*)b; bbyte<(char*)b+BLOCKSIZE; byte++, bbyte++) {
                    if (*byte != *bbyte) {
                        fprintf(stderr, "Failure!\n");
                    }
                }
            }
            free(a);
            free(b);
            printf("All tests OK.\n");

            free_sock(sock);
        }
    }
    return 0;
}

