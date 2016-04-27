#include "utils.h"


static buf_sock *sock = NULL;

static void err_exit(char *msg)
{
    fprintf(stderr, "Error: %s\n", msg);
    if (NULL != sock) {
        free_sock(sock);
    }
    exit(1);
}

static void cmd_loop(void)
{
    size_t cmd_size = 128;
    char *cmd = (char*)malloc(cmd_size+1);
    
    while(1) {
        int reply;

        printf("Command > ");
        fflush(stdout);
        getline(&cmd, &cmd_size, stdin);
        cmd[strlen(cmd)-1] = '\0';
        send_msg(sock, cmd);
        reply = atoi(get_msg(sock));
        if (0 == reply) {
            continue;
        } else if (1 == reply) {
            getline(&cmd, &cmd_size, stdin);
            cmd[strlen(cmd)-1] = '\0';
            send_msg(sock, cmd);
            if (atoi(get_msg(sock))) {
                printf("%s: execution failed.\n", cmd);
            }
        } else if (2 == reply) {
            int num;

            send_msg(sock, "ACK");
            num = atoi(get_msg(sock));
            while (num >= 0) {
                int len;

                send_msg(sock, "EXPECT");
                len = get_data(sock, num);
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
            printf("\n");
        } else if (3 == reply) {
            break;
        } else if (4 == reply) {
            printf("%s: execution failed.\n", cmd);
        } else {
            printf("%s: command not found.\n", cmd);
        }
    }
    free(cmd);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: bin/client <SERVER_PORT_NO>\n");
        exit(0);
    } else {
        sock = get_client_sock("localhost", atoi(argv[1]));
        if (NULL == sock) {
            err_exit("failed to get client socket");
        } else {
            cmd_loop();
            free_sock(sock);
        }
    }
    return 0;
}

