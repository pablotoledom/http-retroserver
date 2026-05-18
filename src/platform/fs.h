/*
 * fs.h — portable filesystem / directory listing
 */
#ifndef PLATFORM_FS_H
#define PLATFORM_FS_H

typedef struct {
    char      name[512];
    int       is_dir;
    long long size;     /* bytes */
    long      mtime;    /* Unix timestamp */
} fs_entry_t;

typedef struct fs_dir_t fs_dir_t;

fs_dir_t *fs_opendir(const char *path);
int       fs_readdir(fs_dir_t *dir, fs_entry_t *entry);  /* 1=entry, 0=done */
void      fs_closedir(fs_dir_t *dir);

#endif /* PLATFORM_FS_H */
