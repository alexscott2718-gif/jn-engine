/* See hook_install.h. Win32-only, no CRT (matches the ddraw proxy's
 * -nostdlib/XP-safety constraint -- see instrument/proxy/build.sh). */
#include <windows.h>
#include "hook_install.h"

int hook_install(HookPatch *hp, void *target, void *replacement) {
    hp->target = target;
    hp->installed = 0;

    unsigned char *t = (unsigned char *)target;

    DWORD old_prot;
    if (!VirtualProtect(t, 5, PAGE_EXECUTE_READWRITE, &old_prot))
        return 0;

    /* Save the bytes we are about to clobber. */
    for (int i = 0; i < 5; i++)
        hp->saved[i] = t[i];

    /* E9 rel32: rel32 is relative to the address of the NEXT instruction
     * (i.e. target + 5), not to `target` itself. */
    int rel = (int)((unsigned char *)replacement - (t + 5));
    t[0] = 0xE9;
    *(int *)(t + 1) = rel;

    VirtualProtect(t, 5, old_prot, &old_prot);
    FlushInstructionCache(GetCurrentProcess(), t, 5);

    hp->installed = 1;
    return 1;
}

void hook_uninstall(HookPatch *hp) {
    if (!hp->installed)
        return;

    unsigned char *t = (unsigned char *)hp->target;
    DWORD old_prot;
    if (!VirtualProtect(t, 5, PAGE_EXECUTE_READWRITE, &old_prot))
        return;

    for (int i = 0; i < 5; i++)
        t[i] = hp->saved[i];

    VirtualProtect(t, 5, old_prot, &old_prot);
    FlushInstructionCache(GetCurrentProcess(), t, 5);
    hp->installed = 0;
}
