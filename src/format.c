#include "alloc.h"


int main(int argc, char *argv[])
{
    int port_no = 10086;

    if (argc == 2) {
        port_no = atoi(argv[1]);
    } else {
        pid_t pid = fork();

        if (0 == pid) {
            usleep(2000);
            alloc_init(port_no);
            format();
            alloc_final();
        } else if (0 <  pid) {
            char str[100];

            bzero(str, 100);
            strcpy(str, "./bin/disk_mgr ");
            itoa(str, port_no);
            system(str);
        } else {
            fprintf(stderr, "Error: failed to fork process.\n");
            exit(1);
        }
        return 0;
    }
}

