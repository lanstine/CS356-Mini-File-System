#include "utils.h"

#define DELAY 1

static int fd, num = 0, track = 0, delay = 0;
static buf_sock *sock = NULL;

static void err_exit(char *msg)
{
    fprintf(stderr, "Error: %s\n.", msg);
    if (NULL != sock) {
        free_sock(sock);
    }
    close(fd);
    exit(1);
}

static void disk_init(void)
{
    fd = open("data/DISK_FILE", O_RDWR|O_CREAT, 0);
    if (fd < 0) {
        perror("cannot open DISK_FILE");
        exit(1);
    } else {
        long file_size;

        file_size = 1L * BLOCKSIZE * SECTORS * CYLINDERS;
        if (lseek(fd, file_size-1, SEEK_SET) < 0) {
            perror("cannot call lseek() to stretch the file");
            exit(2);
        } else if (write(fd, "", 1) != 1) {
            perror("cannot write last byte of the file");
            exit(3);
        } else {
            char *diskfile;

            diskfile = (char*) mmap(0, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
            if (MAP_FAILED == diskfile) {
                perror("failed to map the file");
                exit(4);
            }
        }
    }

}

static int get_offset(unsigned int c, unsigned int s)
{
    if (c >= CYLINDERS || s >= SECTORS) {
        fprintf(stderr, "c = %d or s = %d out or range.\n", c, s);
        return -1;
    } else {
        delay = DELAY*abs(track-c);
        usleep(delay);
        track = c;
        return BLOCKSIZE * (c*SECTORS+s);
    }
}

static void send_info(void)
{
    init_str(sock);
    cat_str(sock, "Disk Info:\n\t");
    cat_int(sock, CYLINDERS);
    cat_str(sock, " CYLINDERS\t");
    cat_int(sock, SECTORS);
    cat_str(sock, " SECTORS\tBLOCK_SIZE = ");
    cat_int(sock, BLOCKSIZE);
    send_str(sock);
}

static void read_disk(unsigned int c, unsigned int s)
{
    int num, offset;

    offset = get_offset(c, s);
    if (offset < 0 || lseek(fd, offset, SEEK_SET) < 0) {
        printf("failed to call lseek()");
        num = 0;
    } else {
        printf("\tdelay = %d ms\n", delay);
        num = BLOCKSIZE;
    }
    init_str(sock);
    cat_str(sock, "FOLLOW ");
    cat_int(sock, num);
    send_str(sock);
    while (num > 0) {
        int len = BUFFER_SIZE;

        if (len > num) {
            len = num;
        }
        if (get_msg(sock) < 0) {
            err_exit("failed to read from socket");
        } else {
            printf("Get Response > %s\n", sock->buffer);
            len = read(fd, sock->buffer, len);
            if (len < 0) {
                err_exit("failed to read from disk");
            } else {
                len = send_data(sock, len);
                if (len < 0) {
                    err_exit("failed to write to socket");
                } else {
                    num -= len;
                }
            } 
        }
    }
}

static void write_disk(unsigned int c, unsigned int s, int n)
{
    int offset, end;

    offset = get_offset(c, s);
    end = offset + BLOCKSIZE;
    if (offset < 0 || lseek(fd, offset, SEEK_SET) < 0) {
        printf("failed to call lseek()\n");
        n = 0;
    } else {
        printf("\tdelay = %d ms\n", delay);
    }
    while (--n >= 0) {
        int len; 

        send_msg(sock, "EXPECT");
        len = get_data(sock, BUFFER_SIZE);
        if (len < 0) {
            err_exit("failed to read from socket");
        } else {
            if (offset + len > end) {
                len = end - offset;
            }
            if (len > 0) {
                len = write(fd, sock->buffer, len);
                if (len < 0) {
                    err_exit("failed to write to disk");
                } else {
                    offset += len;
                }
            }
        }
    }
    send_msg(sock, "OVER");
}

static void serv_loop(void)
{
    char *request, *toks[6];

    request = get_msg(sock);
    while (request) {
        printf("Get Request: %s\n", request);
        get_toks(request, toks, 6);
        if (!strcmp(toks[0], "E") 
                && NULL == toks[1]) {
            send_msg(sock, "BYE");
            break;
        } else if (!strcmp(toks[0], "I") 
                && NULL == toks[1]) {
            send_info();
        } else if (!strcmp(toks[0], "R")
                && NULL != toks[1]
                && NULL != toks[2]
                && NULL == toks[3]) {
            read_disk(atoi(toks[1]), atoi(toks[2]));
        } else if (!strcmp(toks[0], "W")
                && NULL != toks[1]
                && NULL != toks[2]
                && NULL != toks[3]
                && NULL == toks[4]) {
            write_disk(atoi(toks[1]), atoi(toks[2]), atoi(toks[3]));
        } else {
            send_msg(sock, "INVALID COMMAND");
        }
        request = get_msg(sock);
    }
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: bin/disk_mgr <DISK_SERVER_PORT_NO>\n");
    } else {
        disk_init();
        sock = gen_server_sock(atoi(argv[1]));
        if (NULL == sock) {
            err_exit("failed to generate server socket");
        } else {
            serv_loop();
            free_sock(sock);
            close(fd);
        }
    }
    return 0;
}


