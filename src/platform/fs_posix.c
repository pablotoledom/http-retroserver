/* fs_posix.c — POSIX directory listing (Linux + macOS) */

#include "fs.h"
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct fs_dir_t {
    DIR  *d;
    char  path[512];
};

fs_dir_t *fs_opendir(const char *path) {
    DIR *d = opendir(path);
    if (!d) return NULL;

    fs_dir_t *dir = (fs_dir_t *)malloc(sizeof(*dir));
    if (!dir) { closedir(d); return NULL; }

    dir->d = d;
    strncpy(dir->path, path, sizeof(dir->path) - 1);
    dir->path[sizeof(dir->path) - 1] = '\0';
    return dir;
}

int fs_readdir(fs_dir_t *dir, fs_entry_t *entry) {
    struct dirent *de;
    while ((de = readdir(dir->d)) != NULL) {
        if (de->d_name[0] == '.' &&
            (de->d_name[1] == '\0' ||
             (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;

        char full[1024];
        size_t plen = strlen(dir->path);
        if (plen > 0 && dir->path[plen-1] == '/')
            snprintf(full, sizeof(full), "%s%s", dir->path, de->d_name);
        else
            snprintf(full, sizeof(full), "%s/%s", dir->path, de->d_name);

        struct stat st;
        if (stat(full, &st) != 0) continue;

        strncpy(entry->name, de->d_name, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';
        entry->is_dir = S_ISDIR(st.st_mode);
        entry->size   = (long long)st.st_size;
        entry->mtime  = (long)st.st_mtime;
        return 1;
    }
    return 0;
}

void fs_closedir(fs_dir_t *dir) {
    if (dir) { closedir(dir->d); free(dir); }
}
