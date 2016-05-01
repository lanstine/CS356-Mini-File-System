#ifndef PATH_HEADER
#define PATH_HEADER

#include "alloc.h"

typedef struct path_struct {
    signed short id;
    char name[4];
    struct path_struct *prev;
} path_t;

void path_init(void);

void free_path(path_t *node);

path_t *get_path(char *path);

char *rm_name(char **path);

void cwd_to(path_t *path);

#endif /* PATH_HEADER */

