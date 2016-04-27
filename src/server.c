#include "alloc.h"

static buf_sock *sock = NULL;

static void err_exit(char *msg)
{
    fprintf(stderr, "Error: %s\n", msg);
    if (NULL != sock) {
        free_sock(sock);
    }
    alloc_final();
}

static void serv_loop(void)
{
    char *request, *toks[6];

    request = get_msg(sock);
    while (request) {
        int len = 0;

        printf("Get Request: %s\n", request);
        get_toks(request, toks, 6);
        while (len<6 && toks[len]) {
            len++;
        }
        if (1 == len && !strcmp(toks[0], "exit")) {
            send_msg(sock, "3");
            break;
        } else if (2 == len && !strcmp(toks[0], "touch")) {

        } else if (2 == len && !strcmp(toks[0], "mkdir")) {

        } else if (2 == len && !strcmp(toks[0], "rm")) {

        } else if (2 == len && !strcmp(toks[0], "rmdir")) {

        } else if (2 == len && !strcmp(toks[0], "ls")) {

        } else if (1 == len && !strcmp(toks[0], "ls")) {

        } else if (2 == len && !strcmp(toks[0], "cat")) {

        } else if (2 == len && !strcmp(toks[0], "write")) {

        } else if (2 == len && !strcmp(toks[0], "append")) {

        } else {
            send_msg(sock, "-1");
        }
        request = get_msg(sock);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: bin/server <DISK_SERVER_PORT_NO> <CLIENT_PORT_NO>\n");
        exit(0);
    } else {
        alloc_init(atoi(argv[1]));
        sock = gen_server_sock(atoi(argv[2]));
        if (NULL == sock) {
            err_exit("failed to generate server socket");
        } else {
            serv_loop();
            free_sock(sock);
            alloc_final();
        }
    }
}

