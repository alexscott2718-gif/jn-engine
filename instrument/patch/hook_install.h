#ifndef HOOK_INSTALL_H
#define HOOK_INSTALL_H

/* Generic x86 "replace the function" hook: overwrite a live function's
 * first 5 bytes with a relative near jmp (E9 rel32) to a replacement of the
 * same signature/calling convention. No trampoline back to the original is
 * provided -- this is a full replacement, not a wrap-and-call-through. If a
 * call-original path is ever needed, the bytes saved in HookPatch are enough
 * to reconstruct one (re-execute the saved bytes in a copy + jmp past them),
 * but nothing here does that yet.
 *
 * target must be >= 5 bytes long and not be entered via a branch that lands
 * inside the first 5 bytes (true for any normal compiler-emitted function
 * prologue).
 */

typedef struct HookPatch {
    void          *target;
    unsigned char  saved[5];
    int            installed;
} HookPatch;

/* Patch `target` to jmp to `replacement`. Returns 1 on success, 0 on
 * failure (VirtualProtect failed, etc). Safe to call once per HookPatch. */
int hook_install(HookPatch *hp, void *target, void *replacement);

/* Restore the original 5 bytes. Safe to call on an installed hook only. */
void hook_uninstall(HookPatch *hp);

#endif
