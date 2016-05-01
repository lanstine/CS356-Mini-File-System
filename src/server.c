#include "fs_header.h"


static buf_sock *sock = NULL;


static unsigned char mk_file(char *path_str)
{
    path_t *path = get_path(path_str);

    if (NULL != path) {
        free_path(path);
        return 1;
    } else {
        char *file_name = rm_name(&path_str);

        path = get_path(path_str);
        if (NULL == path) {
            return 1;
        } else if (D_OCCUPIED != get_state(path->id)) {
            free_path(path);
            return 1;
        } else {
            signed short id = get_free_id(I_OCCUPIED);

            if (id < 0) {
                free_path(path);
                return 1;
            } else {
                zone_t zone;

                bzero((char*)&zone, ZONESIZE);
                zone.inode_val.parent = path->id;
                zone.inode_val.mode = 0755;
                zone.inode_val.size = 1;
                zone.inode_val.time = (unsigned long)time(NULL);
                write_zone(id, &zone);
                add_sub(path->id, id, file_name);
                free_path(path);
                return 0;
            }
        }
    }
}

static unsigned char mk_dir(char *path_str)
{
    path_t *path = get_path(path_str);

    if (NULL != path) {
        free_path(path);
        return 1;
    } else {
        char *dir_name = rm_name(&path_str);

        path = get_path(path_str);
        if (NULL == path) {
            return 1;
        } else if (D_OCCUPIED != get_state(path->id)) {
            free_path(path);
            return 1;
        } else {
            signed short id = get_free_id(D_OCCUPIED);

            if (id < 0) {
                free_path(path);
                return 1;
            } else {
                zone_t zone;

                bzero((char*)&zone, ZONESIZE);
                zone.dentry_val.parent = path->id;
                zone.dentry_val.mode = 0755;
                zone.dentry_val.time = (unsigned long)time(NULL);
                write_zone(id, &zone);
                add_sub(path->id, id, dir_name);
                free_path(path);
                return 0;
            }
        }
    }
}

static unsigned char rm_file(char *file_path)
{
    path_t *path = get_path(file_path);

    if (NULL == path) {
        return 1;
    } else {
        zone_t *zone = read_zone(path->id);

        if (NULL == zone) {
            free_path(path);
            return 1;
        } else {
            signed short p_id = zone->inode_val.parent;

            rm_inode(path->id);
            rm_sub(p_id, path->id);
            free_path(path);
            return 0;
        }
    }
}

static unsigned char rm_dir(char *dir_path)
{
    path_t *path = get_path(dir_path);

    if (NULL == path) {
        return 1;
    } else {
        zone_t *zone = read_zone(path->id);

        if (NULL == zone) {
            free_path(path);
            return 1;
        } else {
            signed short p_id = zone->dentry_val.parent;

            rm_dentry(path->id);
            rm_sub(p_id, path->id);
            free_path(path);
            return 0;
        }
    }
}

static unsigned char list(char *path_str)
{
    path_t *path = get_path(path_str);

    if (NULL != path) {
        signed short id = path->id;

        free_path(path);
        send_msg(sock, "2");
        if (!strcmp(get_msg(sock), "ACK")) {
            init_str(sock);
            cat_int(sock, get_ls_len(id));
            send_str(sock);
            switch (get_state(id)) {
                case I_OCCUPIED:
                    show_file(sock, id);
                    return 0;
                case D_OCCUPIED:
                    cat_dir(sock, id);
                    return 0;
                default:
                    fprintf(stderr, "Error: unexpected state %d.\n", get_state(id));
            }
        }
    }
    return 1;
}

static unsigned char cat(char *file_path)
{
    path_t *path = get_path(file_path);

    if (NULL != path) {
        signed short id = path->id;

        free_path(path);
        if (I_OCCUPIED == get_state(id)) {
            zone_t *zone = read_zone(id);

            if (NULL != zone) {
                send_msg(sock, "2");
                if (!strcmp(get_msg(sock), "ACK")) {
                    init_str(sock);
                    cat_int(sock, get_cat_len(zone->inode_val.size));
                    send_str(sock);
                    cat_file(sock, id);
                }
                return 0;
            } else {
                fprintf(stderr, "Error: no such zone.\n");
            }
        }
    }
    return 1;
}

static unsigned char append(char *file_path, char *str)
{
    path_t *path = get_path(file_path);

    if (NULL == path) {
        return 1;
    } else if (I_OCCUPIED != get_state(path->id)) {
        free_path(path);
        return 1;
    } else {
        signed short id = path->id;
        zone_t *zone = read_zone(id);

        free_path(path);
        if (NULL == zone) {
            fprintf(stderr, "Error: no such zone.\n");
            return 1;
        } else {
            inode_t inode = zone->inode_val;

            if (strlen(str) > 0) {
                add_data(&inode, str);
                write_zone(id, (zone_t*)&inode);
            }
            return 0;
        }
    }
}

static unsigned char wrt(char *file_path, char *str)
{
    char file_path_1[64], file_path_2[64];
    path_t *path;

    file_path_1[0] = file_path_2[0] = '\0';
    strcpy(file_path_1, file_path);
    strcpy(file_path_2, file_path);
    path = get_path(file_path);
    if (NULL != path) {
        signed short id = path->id;

        free_path(path);
        if (I_OCCUPIED == get_state(id)) {
            rm_file(file_path);
        } else {
            return 1;
        }
    }
    return mk_file(file_path_1) || append(file_path_2, str);
}

static unsigned char change(char *dir_path)
{
    path_t *path = get_path(dir_path);

    if (NULL == path) {
        return 1;
    } else if (D_OCCUPIED != get_state(path->id)) {
        free_path(path);
        return 1;
    } else {
        cwd_to(path);
        return 0;
    }
}

static void interpret(void)
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
            if (strlen(toks[1])>9 || mk_file(toks[1])) {
                send_msg(sock, "4");
            } else {
                send_msg(sock, "0");
            }
        } else if (2 == len && !strcmp(toks[0], "mkdir")) {
            if (strlen(toks[1])>9 || mk_dir(toks[1])) {
                send_msg(sock, "4");
            } else {
                send_msg(sock, "0");
            }
        } else if (2 == len && !strcmp(toks[0], "rm")) {
            if (rm_file(toks[1])) {
                send_msg(sock, "4");
            } else {
                send_msg(sock, "0");
            }
        } else if (2 == len && !strcmp(toks[0], "rmdir")) {
            if (rm_dir(toks[1])) {
                send_msg(sock, "4");
            } else {
                send_msg(sock, "0");
            }
        } else if (2 == len && !strcmp(toks[0], "ls")) {
            if (list(toks[1])) {
                send_msg(sock, "4");
            }
        } else if (1 == len && !strcmp(toks[0], "ls")) {
            list(".");
        } else if (2 == len && !strcmp(toks[0], "cat")) {
            if (cat(toks[1])) {
                send_msg(sock, "4");
            }
        } else if (3 == len && !strcmp(toks[0], "write")) {
            if (wrt(toks[1], toks[2])) {
                send_msg(sock, "4");
            } else {
                send_msg(sock, "0");
            }
        } else if (3 == len && !strcmp(toks[0], "append")) {
            if (append(toks[1], toks[2])) {
                send_msg(sock, "4");
            } else {
                send_msg(sock, "0");
            }
        } else if (2 == len && !strcmp(toks[0], "cd")) {
            if (change(toks[1])) {
                send_msg(sock, "4");
            } else {
                send_msg(sock, "0");
            }
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
    } else {
        alloc_init(atoi(argv[1]));
        sock = gen_server_sock(atoi(argv[2]));
        if (NULL == sock) {
            fprintf(stderr, "Error: failed to generate server socket.\n");
            alloc_final();
            exit(1);
        } else {
            path_init();
            interpret();
            free_sock(sock);
        }
        alloc_final();
    }
}

