/*
 * patch_win32.c — Replace CMOVcc (i686) instructions with i486-compatible code.
 *
 * Replaces: 0F 44 C2  (cmove %edx,%eax)
 *     with: 90 8B C2  (nop + mov %edx,%eax)
 *
 * Compile: gcc -O2 -o patch_win32 patch_win32.c
 * Usage:   ./patch_win32 <exe>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned char FIND[] = {0x0F, 0x44, 0xC2};
static const unsigned char REPL[] = {0x90, 0x8B, 0xC2};
#define PAT 3

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <exe>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r+b");
    if (!f) { perror(argv[1]); return 1; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    unsigned char *data = (unsigned char *)malloc(size);
    if (!data) { fclose(f); fprintf(stderr, "out of memory\n"); return 1; }

    if ((long)fread(data, 1, size, f) != size) {
        fprintf(stderr, "read error\n");
        free(data); fclose(f); return 1;
    }

    int count = 0;
    long i;
    for (i = 0; i <= size - PAT; i++) {
        if (memcmp(data + i, FIND, PAT) == 0) {
            memcpy(data + i, REPL, PAT);
            printf("  patched cmove at offset 0x%08lx\n", i);
            count++;
            i += PAT - 1;
        }
    }

    rewind(f);
    fwrite(data, 1, size, f);
    fclose(f);
    free(data);

    printf("Done -- %d instruction(s) patched in %s\n", count, argv[1]);
    return count > 0 ? 0 : 0;
}
