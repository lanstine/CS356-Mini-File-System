#include "path.h"


static buf_sock *sock = NULL;

static unsigned char add_sub_cont(signed short p_id, signed short id, char *name)
{
    zone_t *p_zone = read_zone(p_id);

    if (NULL == p_zone) {
        return -1;
    } else {
        signed short sub_idx;

        for (sub_idx=0; sub_idx<10; sub_idx++) {
            if (0 == p_zone->dzone_val.subs[sub_idx].id) {
                p_zone->dzone_val.subs[sub_idx].id = id;
                strcpy(p_zone->dzone_val.subs[sub_idx].name, name);
                write_zone(p_id, p_zone);
                return 0;
            }
        }
        if (0 != p_zone->dzone_val.next) {
            write_zone(p_id, p_zone);
            add_sub_cont(p_zone->dzone_val.next, id, name);
        } else {
            signed short next_id = get_free_id(F_OCCUPIED);

            if (next_id < 0) {
                return 1;
            } else {
                zone_t zone;

                p_zone->dzone_val.next = next_id;
                write_zone(p_id, p_zone);
                bzero((char*)&zone, sizeof(zone));
                zone.dzone_val.parent = p_id;
                zone.dzone_val.subs[0].id = id;
                strcpy(zone.dzone_val.subs[0].name, name);
                write_zone(next_id, &zone);
            }
        }
        return 0;
    }
}

static unsigned char add_sub(signed short p_id, signed short id, char *name)
{
    zone_t *p_zone = read_zone(p_id);

    if (NULL == p_zone) {
        return -1;
    } else {
        signed short sub_idx;

        p_zone->dentry_val.time = (unsigned long) time(NULL);
        for (sub_idx=0; sub_idx<8; sub_idx++) {
            if (0 == p_zone->dentry_val.subs[sub_idx].id) {
                p_zone->dentry_val.subs[sub_idx].id = id;
                strcpy(p_zone->dentry_val.subs[sub_idx].name, name);
                write_zone(p_id, p_zone);
                return 0;
            }
        }
        if (0 != p_zone->dentry_val.next) {
            write_zone(p_id, p_zone);
            add_sub_cont(p_zone->dentry_val.next, id, name);
        } else {
            signed short next_id = get_free_id(D_OCCUPIED);

            if (next_id < 0) {
                return 1;
            } else {
                zone_t zone;

                p_zone->dentry_val.next = next_id;
                write_zone(p_id, p_zone);
                bzero((char*)&zone, sizeof(zone));
                zone.dzone_val.parent = p_id;
                zone.dzone_val.subs[0].id = id;
                strcpy(zone.dzone_val.subs[0].name, name);
                write_zone(next_id, &zone);
            }
        }
        return 0;
    }
}

static unsigned char mk_file(char *file_path)
{
    path_t *path = get_path(file_path);

    if (NULL != path) {
        free_path(path);
        return 1;
    } else {
        char *file_name = rm_name(&file_path);
        signed short id;

        path = get_path(file_path);
        id = get_free_id(I_OCCUPIED);
        if (id < 0) {
            free_path(path);
            return 1;
        } else {
            zone_t zone;

            bzero((char*)&(zone.inode_val), sizeof(zone.inode_val));
            zone.inode_val.parent = path->id;
            zone.inode_val.mode = 0755;
            zone.inode_val.time = (unsigned long)time(NULL);
            write_zone(id, &zone);
            switch (add_sub(path->id, id, file_name)) {
                case 0:
                    free_path(path);
                    return 0;
                case 1:
                    free_path(path);
                    return 1;
                default:
                    fprintf(stderr, "Error: parent id not found.\n");
                    free_path(path);
                    return -1;
            }
        }
    }
}

static unsigned char mk_dir(char *dir_path)
{
    path_t *path = get_path(dir_path);

    if (NULL != path) {
        free_path(path);
        return 1;
    } else {
        char *dir_name = rm_name(&dir_path);
        signed short id;

        path = get_path(dir_path);
        id = get_free_id(D_OCCUPIED);
        if (id < 0) {
            free_path(path);
            return 1;
        } else {
            zone_t zone;

            bzero((char*)&(zone.dentry_val), sizeof(zone.dentry_val));
            zone.dentry_val.parent = path->id;
            zone.dentry_val.mode = 0755;
            zone.dentry_val.time = (unsigned long)time(NULL);
            write_zone(id, &zone);
            switch (add_sub(path->id, id, dir_name)) {
                case 0:
                    free_path(path);
                    return 0;
                case 1:
                    free_path(path);
                    return 1;
                default:
                    fprintf(stderr, "Error: parent id not found.\n");
                    free_path(path);
                    return -1;
            }
        }
    }
}

static signed short rm_tail_sub(signed short p_id, char *name)
{
    zone_t *p_zone = read_zone(p_id);

    if (NULL != p_zone) {
        if (0 == p_zone->dzone_val.next) {
            signed short sub_idx = 9;

            while (sub_idx>=0 && 0 == p_zone->dzone_val.subs[sub_idx].id) {
                sub_idx--;
            }
            if (sub_idx >= 0) {
                signed short ret = p_zone->dzone_val.subs[sub_idx].id;

                strcpy(name, p_zone->dzone_val.subs[sub_idx].name);
                p_zone->dzone_val.subs[sub_idx].id = 0;
                bzero(p_zone->dzone_val.subs[sub_idx].name, 4);
                write_zone(p_id, p_zone);
            } else {
                return -1;
            }
        } else {
            return rm_tail_sub(p_zone->dzone_val.next, name);
        }
    }
}

static void rm_sub_cont(signed short p_id, signed short id)
{
    zone_t *p_zone = read_zone(p_id);

    if (NULL != p_zone) {
        signed short sub_idx;

        for (sub_idx=0; sub_idx<10; sub_idx++) {
            if (0 == p_zone->dzone_val.subs[sub_idx].id) {
                if (0 == p_zone->dzone_val.next) {
                    signed short i = sub_idx;

                    while (i < 9 && p_zone->dzone_val.subs[i+1].id) {
                        i++;
                    }
                    if (i > sub_idx) {
                        p_zone->dzone_val.subs[sub_idx].id = p_zone->dzone_val.subs[i].id;
                        strcpy(p_zone->dzone_val.subs[sub_idx].name, p_zone->dzone_val.subs[i].name);
                    }
                    p_zone->dzone_val.subs[i].id = 0;
                    bzero(p_zone->dzone_val.subs[i].name, 4);
                    write_zone(p_id, p_zone);
                } else {
                    signed char name[4];
                    signed short id = rm_tail_sub(p_zone->dzone_val.next, name);

                    p_zone = read_zone(p_id);
                    p_zone->dzone_val.subs[sub_idx].id = id;
                    strcpy(p_zone->dzone_val.subs[sub_idx].name, name);
                    write_zone(p_id, p_zone);
                }
                return;
            }
        }
        if (0 != p_zone->dzone_val.next) {
            rm_sub_cont(p_zone->dzone_val.next, id);
        }
    }
}

static void rm_sub(signed short p_id, signed short id)
{
    zone_t *p_zone = read_zone(p_id);

    if (NULL != p_zone) {
        signed short sub_idx;

        for (sub_idx=0; sub_idx<8; sub_idx++) {
            if (0 == p_zone->dentry_val.subs[sub_idx].id) {
                if (0 == p_zone->dentry_val.next) {
                    signed short i = sub_idx;

                    while (i < 7 && p_zone->dzone_val.subs[i+1].id) {
                        i++;
                    }
                    if (i > sub_idx) {
                        p_zone->dentry_val.subs[sub_idx].id = p_zone->dentry_val.subs[i].id;
                        strcpy(p_zone->dentry_val.subs[sub_idx].name, p_zone->dzone_val.subs[i].name);
                    }
                    p_zone->dzone_val.subs[i].id = 0;
                    bzero(p_zone->dzone_val.subs[i].name, 4);
                    write_zone(p_id, p_zone);
                } else {
                    signed char name[4];
                    signed short id = rm_tail_sub(p_zone->dentry_val.next, name);

                    p_zone = read_zone(p_id);
                    p_zone->dzone_val.subs[sub_idx].id = id;
                    strcpy(p_zone->dzone_val.subs[sub_idx].name, name);
                    write_zone(p_id, p_zone);
                }
                return;
            }
        }
        if (0 != p_zone->dentry_val.next) {
            rm_sub_cont(p_zone->dentry_val.next, id);
        }
    }
}

static void rm_idx_zone(signed short id, unsigned char flag)
{
    zone_t *zone;

    set_free_id(id);
    zone =read_zone(id);
    if (NULL != zone) {
        signed short idx = 0;

        while (idx < 32 && zone->idx_zone_val.zones[idx]) {
            if (flag) {
                rm_idx_zone(zone->idx_zone_val.zones[idx++], 0);
            } else {
                set_free_id(zone->idx_zone_val.zones[idx++]);
            }
        }
    }
}

static void rm_inode(signed short id, inode_t inode)
{
    signed short ptr_idx = 0;

    set_free_id(id); 
    while (ptr_idx < 20) {
        if (0 == inode.ptrs[ptr_idx]) {
            return;
        } else {
            set_free_id(inode.ptrs[ptr_idx]);
            ptr_idx++;
        }
    }
    while (ptr_idx < 22) {
        if (0 == inode.ptrs[ptr_idx]) {
            return;
        } else {
            rm_idx_zone(inode.ptrs[ptr_idx], 0);
            ptr_idx++;
        }
    }
    while (ptr_idx < 24) {
        if (0 == inode.ptrs[ptr_idx]) {
            return;
        } else {
            rm_idx_zone(inode.ptrs[ptr_idx], 1);
            ptr_idx++;
        }
    }
}

static void rm_dir_zone(signed short id)
{
    signed short sub_idx = 0;
    zone_t *zone = read_zone(id);

    set_free_id(id);
    if (NULL != zone) {
        dir_zone_t dir_zone = zone->dzone_val;

        while (sub_idx < 8) {
            if (0 == dir_zone.subs[sub_idx].id) {
                return;
            } else {
                set_free_id(dir_zone.subs[sub_idx].id);
                sub_idx++;
            }
        }
        if (0 != dir_zone.next) {
            rm_dir_zone(dir_zone.next);
        }
    }
}

static void rm_dentry(signed short id, dentry_t dentry)
{
    signed short sub_idx = 0;

    set_free_id(id);
    while (sub_idx < 8) {
        if (0 == dentry.subs[sub_idx].id) {
            return;
        } else {
            set_free_id(dentry.subs[sub_idx].id);
            sub_idx++;
        }
    }
    if (0 != dentry.next) {
        rm_dir_zone(dentry.next);
    }
}

static unsigned char rm_file(char *file_name)
{
    path_t *path = get_path(file_name);

    if (NULL == path) {
        return 1;
    } else {
        signed short id = path->id, p_id;
        zone_t *zone = read_zone(id);

        p_id = zone->inode_val.parent;
        rm_inode(id, zone->inode_val);
        rm_sub(p_id, id);
        return 0;
    }
}

static unsigned char rm_dir(char *dir_name)
{
    path_t *path = get_path(dir_name);

    if (NULL == path) {
        return 1;
    } else {
        signed short id = path->id, p_id;
        zone_t *zone = read_zone(id);

        p_id = zone->dentry_val.parent;
        rm_dentry(id, zone->dentry_val);
        rm_sub(p_id, id);
        return 0;
    }
}

static unsigned char list(char *dir_name)
{
    return 0;
}

static unsigned char cat(char *file_name)
{
    return 0;
}

static unsigned char wrt(char *file_name)
{
    return 0;
}

static unsigned char append(char *file_name)
{
    return 0;
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
            if (mk_file(toks[1])) {
                send_msg(sock, "4");
            } else {
                send_msg(sock, "0");
            }
        } else if (2 == len && !strcmp(toks[0], "mkdir")) {
            if (mk_dir(toks[1])) {
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
        } else if (2 == len && !strcmp(toks[0], "write")) {
            if (wrt(toks[1])) {
                send_msg(sock, "4");
            } else {
                send_msg(sock, "0");
            }
        } else if (2 == len && !strcmp(toks[0], "append")) {
            if (append(toks[1])) {
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
        exit(0);
    } else {
        alloc_init(atoi(argv[1]));
        sock = gen_server_sock(atoi(argv[2]));
        if (NULL == sock) {
            fprintf(stderr, "Error: failed to generate server socket.\n");
            alloc_final();
            exit(1);
        } else {
            interpret();
            free_sock(sock);
        }
        alloc_final();
    }
}

