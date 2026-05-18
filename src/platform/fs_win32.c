/* fs_win32.c — Win32 directory listing (Windows 98+) */

#include "fs.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct fs_dir_t {
    HANDLE          handle;
    WIN32_FIND_DATA data;
    int             has_first;
    char            path[MAX_PATH];
};

static long filetime_to_unix(const FILETIME *ft) {
    /* 100-ns intervals since 1601-01-01 → subtract to 1970-01-01 */
    ULONGLONG t = ((ULONGLONG)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
    t /= 10000000ULL;
    t -= 11644473600ULL;
    return (long)t;
}

fs_dir_t *fs_opendir(const char *path) {
    char pattern[MAX_PATH];
    /* Avoid double separator: if path already ends with \ don't add another */
    size_t plen = strlen(path);
    if (plen > 0 && (path[plen-1] == '\\' || path[plen-1] == '/'))
        snprintf(pattern, sizeof(pattern), "%s*", path);
    else
        snprintf(pattern, sizeof(pattern), "%s\\*", path);

    fs_dir_t *dir = (fs_dir_t *)malloc(sizeof(*dir));
    if (!dir) return NULL;

    dir->handle = FindFirstFile(pattern, &dir->data);
    if (dir->handle == INVALID_HANDLE_VALUE) { free(dir); return NULL; }

    dir->has_first = 1;
    strncpy(dir->path, path, sizeof(dir->path) - 1);
    dir->path[sizeof(dir->path) - 1] = '\0';
    return dir;
}

int fs_readdir(fs_dir_t *dir, fs_entry_t *entry) {
    for (;;) {
        if (!dir->has_first) {
            if (!FindNextFile(dir->handle, &dir->data)) return 0;
        }
        dir->has_first = 0;

        const char *n = dir->data.cFileName;
        if (n[0] == '.' && (n[1] == '\0' || (n[1] == '.' && n[2] == '\0')))
            continue;

        strncpy(entry->name, dir->data.cFileName, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';
        entry->is_dir = (dir->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        entry->size   = ((long long)dir->data.nFileSizeHigh << 32) |
                         (long long)dir->data.nFileSizeLow;
        entry->mtime  = filetime_to_unix(&dir->data.ftLastWriteTime);
        return 1;
    }
}

void fs_closedir(fs_dir_t *dir) {
    if (!dir) return;
    if (dir->handle != INVALID_HANDLE_VALUE) FindClose(dir->handle);
    free(dir);
}
