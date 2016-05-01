#include "fs_header.h"


static buf_sock *sock = NULL;


static void add_sub_cont(signed short p_id, signed short id, char *name);
static signed short rm_tail_sub(signed short p_id, char *name);
static void rm_sub_cont(signed short p_id, signed short id);
static void rm_idx_zone(signed short id, unsigned char flag);
static void rm_dir_zone(signed short id);
static void show_mode(buf_sock *sock, unsigned char d, signed short mode);
static void show_owner(buf_sock *sock, signed short owner);
static void show_sub_name(buf_sock *sock, signed short p_id, signed short id);
static void show_sub_name_cont(buf_sock *sock, signed short p_id, signed short id);
static int get_ls_len_cont(signed short id);
static int get_size(dentry_t *dentry);
static int get_size_cont(dir_zone_t *dzone);
static void show_dir(buf_sock *sock, signed short id);
static void cat_dir_cont(buf_sock *sock, signed short id);
static void cat_idx(buf_sock *sock, signed short id, unsigned char flag);
static void cat_data(buf_sock *sock, signed short id);
static int update_size(inode_t *inode);
static int update_size_idx(signed short idx, unsigned char flag);
static int wrt_idx(signed short id, char *str, unsigned char flag);
static int wrt_data(signed short id, char *str);


void add_sub(signed short p_id, signed short id, char *name)
{
    zone_t *p_zone = read_zone(p_id);

    if (NULL == p_zone) {
        fprintf(stderr, "Error: failed to read zone %d.\n", p_id);
    } else {
        dentry_t dentry = p_zone->dentry_val;

        dentry.time = (unsigned long) time(NULL);
        if (dentry.next) {
            write_zone(p_id, (zone_t*)&dentry);
            add_sub_cont(dentry.next, id, name);
        } else if (dentry.subs[NUM_OF_DENTRY_SUBS-1].id) {
            zone_t next_p_zone;

            dentry.next = get_free_id(F_OCCUPIED);
            write_zone(p_id, (zone_t*)&dentry);
            bzero((char*)&next_p_zone, ZONESIZE);
            next_p_zone.dzone_val.parent = dentry.parent;
            next_p_zone.dzone_val.subs[0].id = id;
            strcpy(next_p_zone.dzone_val.subs[0].name, name);
            write_zone(dentry.next, &next_p_zone);
        } else {
            signed short idx;

            for (idx=NUM_OF_DENTRY_SUBS-1; idx>0; idx--) {
                if (dentry.subs[idx-1].id) {
                    break;
                }
            }
            dentry.subs[idx].id = id;
            strcpy(dentry.subs[idx].name, name);
            write_zone(p_id, (zone_t*)&dentry);
        }
    }
}

static void add_sub_cont(signed short p_id, signed short id, char *name)
{
    zone_t *p_zone = read_zone(p_id);

    if (NULL == p_zone) {
        fprintf(stderr, "Error: failed to read zone %d.\n", p_id);
    } else {
        dir_zone_t dzone = p_zone->dzone_val;

        if (dzone.next) {
            write_zone(p_id, (zone_t*)&dzone);
            add_sub_cont(dzone.next, id, name);
        } else if (dzone.subs[NUM_OF_DZONE_SUBS-1].id) {
            zone_t next_p_zone;

            dzone.next = get_free_id(F_OCCUPIED);
            write_zone(p_id, (zone_t*)&dzone);
            bzero((char*)&next_p_zone, ZONESIZE);
            next_p_zone.dzone_val.parent = dzone.parent;
            next_p_zone.dzone_val.subs[0].id = id;
            strcpy(next_p_zone.dzone_val.subs[0].name, name);
            write_zone(dzone.next, &next_p_zone);
        } else {
            signed short idx;

            for (idx=NUM_OF_DZONE_SUBS-1; idx>0; idx--) {
                if (dzone.subs[idx-1].id) {
                    break;
                }
            }
            dzone.subs[idx].id = id;
            strcpy(dzone.subs[idx].name, name);
            write_zone(p_id, (zone_t*)&dzone);
        }
    }
}

static signed short rm_tail_sub(signed short p_id, char *name)
{
    zone_t *p_zone = read_zone(p_id);

    if (NULL != p_zone) {
        dir_zone_t p_dzone = p_zone->dzone_val;

        if (0 == p_dzone.next) {
            signed short sub_idx = NUM_OF_DZONE_SUBS-1;

            while (sub_idx>=0 && 0 == p_dzone.subs[sub_idx].id) {
                sub_idx--;
            }
            if (sub_idx >= 0) {
                signed short ret = p_dzone.subs[sub_idx].id;

                strcpy(name, p_dzone.subs[sub_idx].name);
                p_dzone.subs[sub_idx].id = 0;
                bzero(p_dzone.subs[sub_idx].name, 10);
                write_zone(p_id, (zone_t*)&p_dzone);
            } else {
                return -1;
            }
        } else {
            return rm_tail_sub(p_dzone.next, name);
        }
    }
}

void rm_sub(signed short p_id, signed short id)
{
    zone_t *p_zone = read_zone(p_id);

    if (NULL != p_zone) {
        dentry_t p_dentry = p_zone->dentry_val;
        signed short sub_idx;

        for (sub_idx=0; sub_idx<NUM_OF_DENTRY_SUBS && p_dentry.subs[sub_idx].id; sub_idx++) {
            if (id == p_dentry.subs[sub_idx].id) {
                if (!p_dentry.next) {
                    signed short i = sub_idx;

                    while (i < NUM_OF_DENTRY_SUBS-1 && p_dentry.subs[i+1].id) {
                        i++;
                    }
                    if (i > sub_idx) {
                        p_dentry.subs[sub_idx].id = p_dentry.subs[i].id;
                        strcpy(p_dentry.subs[sub_idx].name, p_dentry.subs[i].name);
                    }
                    p_dentry.subs[i].id = 0;
                    bzero(p_dentry.subs[i].name, 10);
                    write_zone(p_id, (zone_t*)&p_dentry);
                } else {
                    p_dentry.subs[sub_idx].id = rm_tail_sub(p_dentry.next, p_dentry.subs[sub_idx].name);
                    write_zone(p_id, (zone_t*)&p_dentry);
                }
                return;
            }
        }
        if (p_dentry.next) {
            rm_sub_cont(p_dentry.next, id);
        }
    }
}

static void rm_sub_cont(signed short p_id, signed short id)
{
    zone_t *p_zone = read_zone(p_id);

    if (NULL != p_zone) {
        dir_zone_t p_dzone = p_zone->dzone_val;
        signed short sub_idx;

        for (sub_idx=0; sub_idx<NUM_OF_DZONE_SUBS && p_dzone.subs[sub_idx].id; sub_idx++) {
            if (id == p_dzone.subs[sub_idx].id) {
                if (!p_dzone.next) {
                    signed short i = sub_idx;

                    while (i < NUM_OF_DZONE_SUBS-1 && p_dzone.subs[i+1].id) {
                        i++;
                    }
                    if (i > sub_idx) {
                        p_dzone.subs[sub_idx].id = p_dzone.subs[i].id;
                        strcpy(p_dzone.subs[sub_idx].name, p_dzone.subs[i].name);
                    }
                    p_dzone.subs[i].id = 0;
                    bzero(p_dzone.subs[i].name, 10);
                    write_zone(p_id, (zone_t*)&p_dzone);
                } else {
                    p_dzone.subs[sub_idx].id = rm_tail_sub(p_dzone.next, p_dzone.subs[sub_idx].name);
                    write_zone(p_id, (zone_t*)&p_dzone);
                }
                return;
            }
        }
        if (p_dzone.next) {
            rm_sub_cont(p_dzone.next, id);
        }
    }
}

void rm_inode(signed short id)
{
    zone_t *zone = read_zone(id);

    if (NULL != zone) {
        inode_t inode = zone->inode_val;
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
}

static void rm_idx_zone(signed short id, unsigned char flag)
{
    zone_t *zone = read_zone(id);

    if (NULL != zone) {
        idx_zone_t idx_zone = zone->idx_zone_val;
        signed short idx = 0;

        set_free_id(id);
        while (idx < 32 && idx_zone.zones[idx]) {
            if (flag) {
                rm_idx_zone(idx_zone.zones[idx++], 0);
            } else {
                set_free_id(idx_zone.zones[idx++]);
            }
        }
    }
}

void rm_dentry(signed short id)
{
    zone_t *zone = read_zone(id);

    if (NULL != zone) {
        dentry_t dentry = zone->dentry_val;
        signed short sub_idx = 0;

        set_free_id(id);
        while (sub_idx < NUM_OF_DENTRY_SUBS) {
            if (0 == dentry.subs[sub_idx].id) {
                return;
            } else {
                set_free_id(dentry.subs[sub_idx].id);
                sub_idx++;
            }
        }
        if (dentry.next) {
            rm_dir_zone(dentry.next);
        }
    }
}

static void rm_dir_zone(signed short id)
{
    zone_t *zone = read_zone(id);

    if (NULL != zone) {
        dir_zone_t dzone = zone->dzone_val;
        signed short sub_idx = 0;

        set_free_id(id);
        while (sub_idx < NUM_OF_DZONE_SUBS) {
            if (0 == dzone.subs[sub_idx].id) {
                return;
            } else {
                set_free_id(dzone.subs[sub_idx].id);
                sub_idx++;
            }
        }
        if (dzone.next) {
            rm_dir_zone(dzone.next);
        }
    }
}

int get_ls_len(signed short id)
{
    unsigned char state = get_state(id);

    if (I_OCCUPIED == state) {
        return 1;
    } else if (D_OCCUPIED == state) {
        zone_t *zone = read_zone(id);

        if (NULL != zone) {
            dentry_t dentry = zone->dentry_val;

            if (dentry.next) {
                return NUM_OF_DENTRY_SUBS+get_ls_len_cont(dentry.next);
            } else {
                signed short idx, len=0;

                for (idx=0; idx<NUM_OF_DENTRY_SUBS; idx++) {
                    if (dentry.subs[idx].id) {
                        len++;
                    } else {
                        break;
                    }
                }
                return len;
            }
        } else {
            fprintf(stderr, "Error: no such zone.\n");
        }
    } else {
        return 0;
    }
}

static int get_ls_len_cont(signed short id)
{
    zone_t *zone = read_zone(id);

    if (NULL == zone) {
        fprintf(stderr, "Error: no such zone.\n");
    } else {
        dir_zone_t dzone = zone->dzone_val;

        if (dzone.next) {
            return NUM_OF_DZONE_SUBS+get_ls_len_cont(dzone.next);
        } else {
            signed short idx, len=0;

            for (idx=0; idx<NUM_OF_DZONE_SUBS; idx++) {
                if (dzone.subs[idx].id) {
                    len++;
                } else {
                    break;
                }
            }
            return len;
        }
    }
}

int get_cat_len(unsigned int size)
{
    if (size <= 21) {
        return size-1;
    } else if (size <= 44) {
        return size-2;
    } else if (size <= 87) {
        return size-3;
    } else if (size <= 1144) {
        return size-3-(size-55)/33;
    } else {
        return size-37-(size-1112)/33;
    }
}

static void show_mode(buf_sock *sock, unsigned char d, signed short mode)
{
    char *ch = (sock->buffer)+strlen(sock->buffer);
    int idx;

    *(ch++) = d? 'd' : '-';
    for (idx=8; idx>=0; idx--) {
        if (!(mode & (1<<idx))) {
            *(ch++) = '-';
        } else {
            switch (idx%3) {
                case 2:
                    *(ch++) = 'r';
                    break;
                case 1:
                    *(ch++) = 'w';
                    break;
                default:
                    *(ch++) = 'x';
            }
        }
    }
    *ch = '\0';
}

static void show_owner(buf_sock *sock, signed short owner)
{
    unsigned char own = (unsigned char) (owner>>8);
    unsigned char grp = (unsigned char) owner;

    cat_str(sock, "\t");
    if (!own) {
        cat_str(sock, "root");
    } else {
        cat_str(sock, "user_");
        cat_int(sock, own);
    }
    cat_str(sock, "\t");
    if (!grp) {
        cat_str(sock, "root");
    } else {
        cat_str(sock, "group_");
        cat_int(sock, grp);
    }
}


static void show_sub_name(buf_sock *sock, signed short p_id, signed short id)
{
    zone_t *zone = read_zone(p_id);

    if (NULL != zone) {
        dentry_t dentry = zone->dentry_val;
        int sub_idx;

        for (sub_idx=0; sub_idx<NUM_OF_DENTRY_SUBS && dentry.subs[sub_idx].id; sub_idx++) {
            if (id == dentry.subs[sub_idx].id) {
                cat_str(sock, "\t");
                cat_str(sock, dentry.subs[sub_idx].name);
                return;
            }
        }
        if (dentry.next) {
            show_sub_name_cont(sock, dentry.next, id);
        } else {
            fprintf(stderr, "Error: cannot find sub.\n");
        }
    } else {
        fprintf(stderr, "Error: no such zone.\n");
    }
}

static void show_sub_name_cont(buf_sock *sock, signed short p_id, signed short id)
{
    zone_t *zone = read_zone(p_id);

    if (NULL != zone) {
        dir_zone_t dzone = zone->dzone_val;
        int sub_idx;

        for (sub_idx=0; sub_idx<NUM_OF_DZONE_SUBS && dzone.subs[sub_idx].id; sub_idx++) {
            if (id == dzone.subs[sub_idx].id) {
                cat_str(sock, "\t");
                cat_str(sock, dzone.subs[sub_idx].name);
                return;
            }
        }
        if (dzone.next) {
            show_sub_name_cont(sock, dzone.next, id);
        } else {
            fprintf(stderr, "Error: cannot find sub.\n");
        }
    } else {
        fprintf(stderr, "Error: no such zone.\n");
    }
}

static int get_size(dentry_t *dentry)
{
    int size = 1, idx;

    for (idx=0; idx<NUM_OF_DENTRY_SUBS; idx++) {
        if (!dentry->subs[idx].id) {
            return size;
        } else if (I_OCCUPIED == get_state(dentry->subs[idx].id)) {
            zone_t *zone = read_zone(dentry->subs[idx].id);

            if (NULL == zone) {
                fprintf(stderr, "Error: no such zone.\n");
                return 0;
            } else {
                size += zone->inode_val.size;
            }
        } else if (D_OCCUPIED == get_state(dentry->subs[idx].id)) {
            zone_t *zone = read_zone(dentry->subs[idx].id);

            if (NULL == zone) {
                fprintf(stderr, "Error: no such zone.\n");
                return 0;
            } else {
                dentry_t dir = zone->dentry_val;

                size += get_size(&dir);
            }
        } else {
            fprintf(stderr, "Error: unexpected state.\n");
            return 0;
        }
    }
    if (dentry->next) {
        zone_t *zone = read_zone(dentry->next);

        if (NULL == zone) {
            fprintf(stderr, "Error: no such zone.\n");
            return 0;
        } else {
            dir_zone_t next = zone->dzone_val;

            return size + get_size_cont(&next);
        }
    } else {
        return size;
    }
}

static int get_size_cont(dir_zone_t *dzone)
{
    int size = 1, idx;

    for (idx=0; idx<NUM_OF_DZONE_SUBS; idx++) {
        if (!dzone->subs[idx].id) {
            return size;
        } else if (I_OCCUPIED == get_state(dzone->subs[idx].id)) {
            zone_t *zone = read_zone(dzone->subs[idx].id);

            if (NULL == zone) {
                fprintf(stderr, "Error: no such zone.\n");
                return 0;
            } else {
                size += zone->inode_val.size;
            }
        } else if (D_OCCUPIED == get_state(dzone->subs[idx].id)) {
            zone_t *zone = read_zone(dzone->subs[idx].id);

            if (NULL == zone) {
                fprintf(stderr, "Error: no such zone.\n");
                return 0;
            } else {
                dentry_t dir = zone->dentry_val;

                size += get_size(&dir);
            }
        } else {
            fprintf(stderr, "Error: unexpected state.\n");
            return 0;
        }
    }
    if (dzone->next) {
        zone_t *zone = read_zone(dzone->next);

        if (NULL == zone) {
            fprintf(stderr, "Error: no such zone.\n");
            return 0;
        } else {
            dir_zone_t next = zone->dzone_val;

            return size + get_size_cont(&next);
        }
    } else {
        return size;
    }
}

void show_file(buf_sock *sock, signed short id)
{
    if (!strcmp(get_msg(sock), "EXPECT")) {
        zone_t *zone = read_zone(id);

        if (NULL != zone) {
            inode_t inode = zone->inode_val;
            signed short p_id = inode.parent;

            init_str(sock);
            show_mode(sock, 0, inode.mode);
            show_owner(sock, inode.owner);
            cat_str(sock, "\t");
            cat_int(sock, inode.size<<6);
            cat_str(sock, "\t");
            cat_int(sock, inode.time);
            show_sub_name(sock, p_id, id);
            cat_str(sock, "\n");
            send_str(sock);
        } else {
            send_msg(sock, "\n");
            fprintf(stderr, "Error: no such zone.\n");
        }
    }
}

static void show_dir(buf_sock *sock, signed short id)
{
    if (!strcmp(get_msg(sock), "EXPECT")) {
        zone_t *zone = read_zone(id);

        if (NULL != zone) {
            dentry_t dentry = zone->dentry_val;
            signed short p_id = dentry.parent;

            init_str(sock);
            show_mode(sock, 1, dentry.mode);
            show_owner(sock, dentry.owner);
            cat_str(sock, "\t");
            cat_int(sock, get_size(&dentry)<<6);
            cat_str(sock, "\t");
            cat_int(sock, dentry.time);
            show_sub_name(sock, p_id, id);
            cat_str(sock, "\n");
            send_str(sock);
        } else {
            send_msg(sock, "\n");
            fprintf(stderr, "Error: no such zone.\n");
        }
    }
}

void cat_dir(buf_sock *sock, signed short id)
{
    zone_t *zone = read_zone(id);

    if (NULL != zone) {
        dentry_t dentry = zone->dentry_val;
        signed short sub_idx;

        for (sub_idx=0; sub_idx<NUM_OF_DENTRY_SUBS; sub_idx++) {
            if (0 == dentry.subs[sub_idx].id) {
                return;
            } else if (D_OCCUPIED == get_state(dentry.subs[sub_idx].id)) {
                show_dir(sock, dentry.subs[sub_idx].id);
            } else if (I_OCCUPIED == get_state(dentry.subs[sub_idx].id)) {
                show_file(sock, dentry.subs[sub_idx].id);
            } else {
                if (!strcmp(get_msg(sock), "EXPECT")) {
                    send_msg(sock, "\n");
                }
                fprintf(stderr, "Error: unexpected state %d.\n", get_state(dentry.subs[sub_idx].id));
            }
        }
        if (dentry.next) {
            cat_dir_cont(sock, dentry.next);
        }
    } else {
        fprintf(stderr, "Error: no such zone.\n");
    }
}

static void cat_dir_cont(buf_sock *sock, signed short id)
{
    zone_t *zone = read_zone(id);

    if (NULL == zone) {
        dir_zone_t dzone = zone->dzone_val;
        signed short sub_idx;

        for (sub_idx=0; sub_idx<NUM_OF_DZONE_SUBS; sub_idx++) {
            if (0 == dzone.subs[sub_idx].id) {
                return;
            } else if (D_OCCUPIED == get_state(dzone.subs[sub_idx].id)) {
                show_dir(sock, dzone.subs[sub_idx].id);
            } else if (I_OCCUPIED == get_state(dzone.subs[sub_idx].id)) {
                show_file(sock, dzone.subs[sub_idx].id);
            } else {
                if (!strcmp(get_msg(sock), "EXPECT")) {
                    send_msg(sock, "\n");
                }
                fprintf(stderr, "Error: unexpected state %d.\n", get_state(dzone.subs[sub_idx].id));
            }
        }
        if (dzone.next) {
            cat_dir_cont(sock, dzone.next);
        }
    } else {
        fprintf(stderr, "Error: no such zone.\n");
    }
}

void cat_file(buf_sock *sock, signed short id)
{
    zone_t *zone = read_zone(id);

    if (NULL != zone) {
        inode_t inode = zone->inode_val;
        int size = inode.size, i;

        for (i=0; i<20; i++) {
            if (!inode.ptrs[i]) {
                return;
            } else {
                cat_data(sock, inode.ptrs[i]);
            }
        }
        for (; i<22; i++) {
            if (!inode.ptrs[i]) {
                return;
            } else {
                cat_idx(sock, inode.ptrs[i], 0);
            }
        }
        for (; i<24; i++) {
            if (!inode.ptrs[i]) {
                return;
            } else {
                cat_idx(sock, inode.ptrs[i], 1);
            }
        }
    } else {
        fprintf(stderr, "Error: no such zone.\n");
    }
}

static void cat_idx(buf_sock *sock, signed short id, unsigned char flag)
{
    zone_t *zone = read_zone(id);

    if (NULL != zone) {
        idx_zone_t idx_zone = zone->idx_zone_val;
        signed short idx;

        for (idx=0; idx<32; idx++) {
            if (!idx_zone.zones[idx]) {
                return;
            } else if (flag) {
                cat_idx(sock, idx_zone.zones[idx], 0);
            } else {
                cat_data(sock, idx_zone.zones[idx]);
            }
        }
    } else {
        fprintf(stderr, "Error: no such zone.\n");
    }
}

static void cat_data(buf_sock *sock, signed short id)
{
    if (!strcmp(get_msg(sock), "EXPECT")) {
        zone_t *zone = read_zone(id);

        memcpy(sock->buffer, zone, ZONESIZE);
        sock->buffer[ZONESIZE] = '\0';
        send_str(sock);
    }
}

void add_data(inode_t *inode, char *str)
{
    int idx = 23;

    if (inode->ptrs[idx--]) {
        str += wrt_idx(inode->ptrs[++idx], str, 1);
    } else if (inode->ptrs[idx--]) {
        str += wrt_idx(inode->ptrs[++idx], str, 1);
    } else if (inode->ptrs[idx--]) {
        str += wrt_idx(inode->ptrs[++idx], str, 0);
    } else if (inode->ptrs[idx--]) {
        str += wrt_idx(inode->ptrs[++idx], str, 0);
    } else {
        if (!inode->ptrs[0] && strlen(str)>0) {
            signed short n_id = get_free_id(F_OCCUPIED);

            if (n_id >= 0) {
                zone_t n_zone;

                bzero((char*)&n_zone, ZONESIZE);
                write_zone(n_id, &n_zone);
                inode->ptrs[0] = n_id;
            }
        }
        for (; idx>0; idx--) {
            if (inode->ptrs[idx]) {
                break;
            }
        }
        str += wrt_data(inode->ptrs[idx], str);
    }
    if (strlen(str) > 0 && idx < 23) {
        signed short id = get_free_id(F_OCCUPIED);

        if (id >= 0) {
            zone_t n_zone;

            bzero((char*)&n_zone, ZONESIZE);
            write_zone(id, &n_zone);
            inode->ptrs[idx+1] = id;
            add_data(inode, str);
        }
    } else {
        inode->size = update_size(inode);
        inode->time = (unsigned long) time(NULL);
    }
}

static int wrt_idx(signed short id, char *str, unsigned char flag)
{
    zone_t *zone = read_zone(id);

    if (NULL == zone) {
        fprintf(stderr, "Error: no such zone.\n");
    } else {
        idx_zone_t idx_zone = zone->idx_zone_val;
        signed short len=0, idx;

        if (!idx_zone.zones[0] && strlen(str)>0) {
            signed short n_id = get_free_id(F_OCCUPIED);

            if (n_id >= 0) {
                zone_t n_zone;

                bzero((char*)&n_zone, ZONESIZE);
                write_zone(n_id, &n_zone);
                idx_zone.zones[0] = n_id;
            }
        }
        for (idx=31; idx>0; idx--) {
            if (idx_zone.zones[idx]) {
                break;
            }
        }
        while (1) {
            if (flag) {
                len += wrt_idx(idx_zone.zones[idx], str+len, 0);
            } else {
                len += wrt_data(idx_zone.zones[idx], str+len);
            }
            if (strlen(str+len) > 0 && idx < 31) {
                signed short n_id = get_free_id(F_OCCUPIED);

                if (n_id >= 0) {
                    zone_t n_zone;

                    bzero((char*)&n_zone, ZONESIZE);
                    write_zone(n_id, (zone_t*)&n_zone);
                    idx_zone.zones[++idx] = n_id;
                } else {
                    return len;
                }
            } else {
                write_zone(id, (zone_t*)&idx_zone);
                return len;
            }
        }
    }
}

static int wrt_data(signed short id, char *str)
{
    zone_t *zone = read_zone(id);

    if (NULL == zone) {
        return 0;
    } else {
        data_zone_t data_zone = zone->data_zone_val;

        if (strlen(data_zone.data)+strlen(str)+1 <= ZONESIZE) {
            strcat(data_zone.data, str);
            write_zone(id, (zone_t*)&data_zone);
            return strlen(str);
        } else {
            int len = ZONESIZE-1-strlen(data_zone.data);

            strncat(data_zone.data, str, len);
            write_zone(id, (zone_t*)&data_zone);
            return len;
        }
    }
}

static int update_size(inode_t *inode)
{
    if (inode->ptrs[23]) {
        return 1144+update_size_idx(inode->ptrs[23], 1);
    } else if (inode->ptrs[22]) {
        return 87+update_size_idx(inode->ptrs[22], 1);
    } else if (inode->ptrs[21]) {
        return 54+update_size_idx(inode->ptrs[21], 0);
    } else if (inode->ptrs[20]) {
        return 21+update_size_idx(inode->ptrs[20], 0);
    } else {
        signed short size=1, idx;

        for (idx=0; idx<20; idx++) {
            if (inode->ptrs[idx]) {
                size++;
            } else {
                break;
            }
        }
        return size;           
    }
}

static int update_size_idx(signed short id, unsigned char flag)
{
    zone_t *zone = read_zone(id);

    if (NULL == zone) {
        fprintf(stderr, "Error: no such zone.\n");
    } else {
        idx_zone_t idx_zone = zone->idx_zone_val;
        signed short size=1, idx;

        for (idx=0; idx<32; idx++) {
            if (!idx_zone.zones[idx]) {
                break;
            } else if (flag) {
                size += update_size_idx(idx_zone.zones[idx], 0);
            } else {
                size++;
            }
        }
        return size;
    }
}

