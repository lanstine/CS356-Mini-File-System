#include "path.h"


static path_t root_dir;

static path_t *pwd;

static path_t *get_sub_path(path_t *parent_path, char *name)
{
    zone_t *p_zone = read_zone(parent_path->id);
    if (NULL != p_zone) {
        dentry_t p_dir = p_zone->dentry_val;
        void *ptr = (void*)&(p_dir.subs[10]);
        void *end = (void*)&(p_dir.next);

        while (ptr < end) {
            signed short sub_id;
            signed char *sub_name;

            sub_id = *((signed short*)ptr);
            ptr = (void*)(((signed short*)ptr)+1);
            sub_name = (char*)ptr;
            ptr = (void*)(sub_name+4);
            if (strcmp(name, sub_name)) {
                path_t *sub_path;
                
                sub_path = (path_t*)calloc(1, sizeof(path_t));
                sub_path->id = sub_id;
                sub_path->name[0] = '\0';
                strcpy(sub_path->name, sub_name);
                sub_path->prev = parent_path;
                return sub_path;
            }
        }
    }
    return NULL;
}

static path_t *path_clone(path_t *old_node)
{
    if (old_node == &root_dir) {
        return &root_dir;
    } else {
        path_t *new_node;
        int i;
        
        new_node = (path_t*)calloc(1, sizeof(path_t));
        new_node->id = old_node->id;
        for (i=0; i<4; i++) {
            new_node->name[i] = old_node->name[i];
        }
        new_node->prev = path_clone(old_node->prev);
        return new_node;
    }
}

static path_t *get_path_from_root(path_t *curr, char *path)
{
    if (0 == strlen(path) || !strcmp(path, ".")) {
        return curr;
    } else if (!strcmp(path, "..")) {
        if (NULL != curr->prev) {
            path_t *ret = curr->prev;

            free(curr);
            return ret;
        } else {
            return NULL;
        }
    } else if (!strncmp(path, "./", 2)) {
        return get_path_from_root(curr, path+2);
    } else if (!strncmp(path, "../", 3)) {
        if (NULL != curr->prev) {
            path_t *ret = get_path_from_root(curr->prev, path+3);
            
            free(curr);
            return ret;
        } else {
            return NULL;
        }
    } else if (D_OCCUPIED != get_state(curr->id)) {
        free_path(curr);
        return NULL;
    } else {
        path_t *sub_path;
        char *ch = path;

        while (ch) {
            if ('/' == *ch) {
                *ch = '\0';
                sub_path = get_sub_path(curr, path);
                if (NULL == sub_path) {
                    free_path(curr);
                    return NULL;
                } else {
                    return get_path_from_root(sub_path, ch+1);
                }
            }
            ch++;
        }
        sub_path = get_sub_path(curr, path);
        if (NULL == sub_path) {
            free_path(curr);
            return NULL;
        } else {
            return sub_path;
        }
    }
}

static path_t *get_path_from_pwd(path_t *curr, char *path)
{
    if (0 == strlen(path) || !strcmp(path, ".")) {
        return path_clone(curr);
    } else if (!strcmp(path, "..")) {
        if (NULL != curr->prev) {
            return path_clone(curr->prev);
        } else {
            return NULL;
        }
    } else if (!strncmp(path, "./", 2)) {
        return get_path_from_pwd(curr, path+2);
    } else if (!strncmp(path, "../", 3)) {
        if (NULL != curr->prev) {
            return get_path_from_root(path_clone(curr->prev), path+3);
        } else {
            return NULL;
        }
    } else if (D_OCCUPIED != get_state(curr->id)) {
        return NULL;
    } else {
        return get_path_from_root(path_clone(curr), path);
    }
}

path_t *get_path(char *path)
{
    if ('/' == path[0]) {
        return get_path_from_root(&root_dir, path+1);
    } else {
        return get_path_from_pwd(pwd, path);
    }
}

void free_path(path_t *node)
{
    if (node != &root_dir) {
        free_path(node->prev);
        free(node);
    }
}

void path_init(void)
{
    root_dir.id = 4;
    bzero((char*)root_dir.name, sizeof(root_dir.name));
    root_dir.prev = NULL;
    pwd = &root_dir;
}

char *rm_name(char **path)
{
    char *ch = (*path)+strlen(*path);

    while (--ch > *path) {
        if ('/' == *ch) {
            *ch = '\0';
            return ch+1;
        }
    }
    (*path) += strlen(*path);
    return ch;
}

