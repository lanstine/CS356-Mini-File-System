#ifndef FS_HEADER
#define FS_HEADER

#include "path.h"

void add_sub(signed short p_id, signed short id, char *name);
void rm_sub(signed short p_id, signed short id);

void rm_inode(signed short id);
void rm_dentry(signed short id);

int get_ls_len(signed short id);
int get_cat_len(unsigned int size);

void show_file(buf_sock *sock, signed short id);
void cat_dir(buf_sock *sock, signed short id);
void cat_file(buf_sock *sock, signed short id);

void add_data(inode_t *inode, char *str);

#endif /* FS_HEADER */

