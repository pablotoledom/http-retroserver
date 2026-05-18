/*
 * win95_compat.c — i486-safe overrides for libgcc/mingwex functions
 * that contain CMOVcc (i686-only) instructions.
 *
 * MinGW prepends '_' to C symbol names on Win32, so we use __asm__ to
 * force the exact symbol name that the linker expects, overriding the
 * i686-compiled versions from libgcc/mingwex.
 */
#ifdef PLAT_WIN95

#include <windows.h>

/* ── __GetPEImageBase ──────────────────────────────────────────────────
 * Returns 0x400000 if a valid PE32 image is loaded there, else 0.
 * libgcc exports this as "__GetPEImageBase" (2 underscores, no extra prefix).
 */
void *compat_GetPEImageBase(void) __asm__("__GetPEImageBase");
void *compat_GetPEImageBase(void) {
    unsigned char *base = (unsigned char *)0x400000;
    unsigned short mz, magic;
    int pe_off;
    unsigned int pe_sig;

    mz = *(unsigned short *)base;
    if (mz != 0x5A4D) return (void *)0;
    pe_off  = *(int *)(base + 0x3C);
    pe_sig  = *(unsigned int *)(base + pe_off);
    if (pe_sig != 0x00004550) return (void *)0;
    magic = *(unsigned short *)(base + pe_off + 0x18);
    return (magic == 0x010B) ? (void *)0x400000 : (void *)0;
}

/* ── _mark_section_writable ────────────────────────────────────────────
 * Makes PE sections writable for SEH registration — safe no-op for us
 * since this project uses no structured exception handling.
 * C name "mark_section_writable" exports as "_mark_section_writable".
 */
void mark_section_writable(void *addr) __asm__("_mark_section_writable");
void mark_section_writable(void *addr) {
    (void)addr;
}

/* ── __stub ────────────────────────────────────────────────────────────
 * MinGW runtime dispatch stub — return 0 as safe default.
 */
int compat_stub(void) __asm__("__stub");
int compat_stub(void) {
    return 0;
}

#endif /* PLAT_WIN95 */
