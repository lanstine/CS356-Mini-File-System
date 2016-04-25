#include "header.h"


void itoa(char *str, int val)
{
    if (0 == val) {
        strcat(str, "0");
    } else {
        char digs[10];
        int len = 0, pos;

        if (val < 0) {
            strcat(str, "-");
            val = -val;
        }
        while (val > 0) {
            digs[len++] = '0'+(val%10);
            val /= 10;
        }
        pos = strlen(str);
        while (--len >= 0) {
            str[pos++] = digs[len];
        }
        str[pos] = '\0';
    }
}

void get_toks(char *str, char *toks[], int size_of_toks)
{

    int len = 0, state = 0;
 
    while ('\0' != *str) {
        if (0 == state) {
            if (!isspace(*str)) {
                toks[len] = str;
                if (++len == size_of_toks-1) {
                    break;
                } else {
                    state = 1;
                }
            }
        } else if (isspace(*str)) {
            *str = '\0';
            state = 0;
        }
        str++;
    }
    toks[len] = NULL;
}

buf_sock *get_client_sock(char *hostname, int port_no)
{
    struct hostent *server;

    server = gethostbyname(hostname);
    if (NULL == server) {
        return NULL;
    } else {
        signed int sockfd;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            return NULL;
        } else {
            struct sockaddr_in serv_addr;

            bzero((char*)&serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
            serv_addr.sin_port = htons(port_no);
            if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                return NULL;
            } else {
                buf_sock *sock;
                
                sock = (buf_sock*) calloc(1, sizeof(buf_sock));
                if (NULL != sock) {
                    sock->sockfd = sockfd;
                }
                return sock;
            }
        }
    }
}


buf_sock *gen_server_sock(int port_no)
{
    signed int serv_sockfd;
    
    serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sockfd < 0) {
        return NULL;
    } else {
        struct sockaddr_in serv_addr, cli_addr;

        bzero((char*)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port_no);
        if (bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            return NULL;
        } else {
            buf_sock sock;
            size_t sockfd, addrlen = sizeof(cli_addr);

            listen(serv_sockfd, 5);
            sockfd = accept(serv_sockfd, (struct sockaddr*)&cli_addr, (socklen_t*)&addrlen);
            if (sockfd < 0) {
                return NULL;
            } else {
                buf_sock *sock = (buf_sock*) calloc(1, sizeof(buf_sock));
                if (NULL != sock) {
                    sock->sockfd = sockfd;
                }
                return sock;
            }
        }
    }
}

void free_sock(buf_sock *sock)
{
    if (NULL != sock) {
        close(sock->sockfd);
        free(sock);
    }
}

int send_msg(buf_sock *sock, char *msg)
{
    sock->buffer[0] = '\0';
    if (NULL != msg) {
        strncat(sock->buffer, msg, BUFFER_SIZE-1);
    }
    return write(sock->sockfd, sock->buffer, strlen(sock->buffer)+1)-1;
}

char *get_msg(buf_sock *sock)
{
    ssize_t len;
    
    len = read(sock->sockfd, sock->buffer, BUFFER_SIZE-1);
    if (len <= 0 || sock->buffer[len-1]) {
        return NULL;
    } else {
        return sock->buffer;
    }
}

int send_data(buf_sock *sock, int len)
{
    if (len < 0 || len >= BUFFER_SIZE) {
        return -1;
    } else {
        sock->buffer[len] = '\0';
        return write(sock->sockfd, sock->buffer, len+1)-1;
    }
}

int get_data(buf_sock *sock, int len)
{
    if (len >= BUFFER_SIZE) {
        len = BUFFER_SIZE-1;
    } else if (len < 0) {
        len = 0;
    }
    len = read(sock->sockfd, sock->buffer, len+1)-1;
    if (len < 0 || sock->buffer[len]) {
        return -1;
    } else {
        return len;
    }
}

void init_str(buf_sock *sock)
{
    sock->buffer[0] = '\0';
}

void cat_str(buf_sock *sock, char *str)
{
    if (NULL != str) {
        strncat(sock->buffer, str, BUFFER_SIZE-strlen(sock->buffer)-1);
    }
}

void cat_int(buf_sock *sock, int val)
{
    itoa(sock->buffer, val);
}

int send_str(buf_sock *sock)
{
    return write(sock->sockfd, sock->buffer, strlen(sock->buffer)+1)-1;
}


