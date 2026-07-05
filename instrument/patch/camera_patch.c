/*
 * Camera-fix patch shim -- ddraw.dll (M1: hook-mechanism validation, OFF by
 * default, zero behaviour change unless explicitly enabled).
 *
 * Loads into Neutron.exe via the same DLL-search-order trick as the capture
 * proxy (instrument/proxy/ddraw_proxy.c): a ddraw.dll placed next to the exe
 * is loaded ahead of system32\ddraw.dll. Unlike the capture proxy, this shim
 * does not care about any DirectDraw call at all -- every export in
 * camera_patch.def is a plain forwarder to ddraw_orig.dll, so it rides along
 * purely to get code running inside the game process before the game's own
 * entry point runs.
 *
 * What this milestone does and does NOT do:
 *   - Locates UpdateWalkingCameraB (00439900) in the live process and CAN
 *     install a full-replacement hook (hook_install.c) at its entry.
 *   - The hook is gated behind C:\camerapatch_enable -- absent by default,
 *     so dropping this DLL next to the game changes nothing until someone
 *     deliberately opts in.
 *   - When enabled, the replacement logs the live camera record (the global
 *     DAT_00509a50 struct, offsets per src/game/camera_record.h) and the
 *     player's turn-rate input (this+0x6d4, per docs/decomp/C3DPlayer.md)
 *     and then RETURNS WITHOUT DOING THE ORIGINAL WORK. This freezes the
 *     walking-camera follow behaviour while enabled -- it is a hook-
 *     mechanism smoke test, not the actual fix. Do not enable during normal
 *     play.
 *
 * What's still needed before this can call the real fix
 * (src/game/camera_record.c's camera_record_walkcam_write, already ported
 * and oracle-certified) with real inputs instead of a no-op:
 *   1. Confirm Neutron.exe's actual image base / ASLR status against the
 *      real binary -- addresses below assume the untouched-since-2003
 *      default of 0x00400000 with no relocation, which is very likely for a
 *      VC6/7-era non-/DYNAMICBASE game exe, but has not been checked against
 *      the actual PE header (XP box was unreachable when this was written).
 *   2. Recover the player object's world position + yaw fields. C3DPlayer's
 *      own accumulator fields (walk_speed, turn rate, etc.) are already
 *      mapped in docs/decomp/C3DPlayer.md starting at 0x608, but the base
 *      position/rotation (inherited from OMediaWorldPosition/OMediaWorldAngle
 *      further up the class chain) is NOT yet a named, recovered offset in
 *      this repo -- see the caveat in docs/decomp/C3DBaseball.md. Until that
 *      lands, camera_record_walkcam_write can't be fed real pos_native/
 *      yaw_deg/roll_deg.
 */

#include <windows.h>
#include "hook_install.h"

static const char LOG_PATH[]     = "C:\\camerapatch.log";
static const char ENABLE_FLAG[]  = "C:\\camerapatch_enable";

/* Recovered addresses (see docs/decomp/evidence/walking_camera_record_write.md
 * and docs/decomp/evidence/camera_record_layout.md). Assumed default image
 * base for an unrelocated 2003-era PE32 exe -- TODO verify against the real
 * Neutron.exe PE header (see file header note #1). */
#define ASSUMED_IMAGE_BASE      0x00400000u
#define ADDR_UPDATE_WALKCAM_B   0x00439900u
#define ADDR_GLOBAL_RECORD      0x00509a50u /* DAT_00509a50 */

/* CameraRecord field offsets from DAT_00509a50, per src/game/camera_record.h. */
#define REC_OFF_POS   0x44
#define REC_OFF_ANGLE 0x50

/* C3DPlayer turn-rate accumulator, per docs/decomp/C3DPlayer.md (0x6d4). */
#define PLAYER_OFF_TURN_RATE 0x6d4

static HookPatch g_walkcam_hook;
static int       g_call_count;

static void plog(const char *msg) {
    HANDLE h = CreateFileA(LOG_PATH, FILE_APPEND_DATA,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[256];
    int n = wsprintfA(buf, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] %s\r\n",
                      st.wYear, st.wMonth, st.wDay,
                      st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg);
    DWORD written;
    SetFilePointer(h, 0, NULL, FILE_END);
    WriteFile(h, buf, (DWORD)n, &written, NULL);
    CloseHandle(h);
}

static int file_exists(const char *path) {
    DWORD a = GetFileAttributesA(path);
    return a != INVALID_FILE_ATTRIBUTES;
}

/* Replacement for UpdateWalkingCameraB_00439900. Must match its exact
 * __thiscall (this in ECX) / stack (dt) shape -- this function's own entry
 * IS the landing site for the hijacked call, so it has to honor the same
 * ABI the original did. See hook_install.h for why a bare jmp-patch works
 * here (the caller's return address on the stack is untouched). */
static void __thiscall patched_UpdateWalkingCameraB(void *this_ptr, float dt) {
    (void)dt;
    g_call_count++;

    /* Throttle logging -- this runs every frame. */
    if (g_call_count <= 5 || (g_call_count % 300) == 0) {
        unsigned char *rec = (unsigned char *)ADDR_GLOBAL_RECORD;
        float *pos   = (float *)(rec + REC_OFF_POS);
        short *angle = (short *)(rec + REC_OFF_ANGLE);
        float  turn_rate = *(float *)((unsigned char *)this_ptr + PLAYER_OFF_TURN_RATE);

        char buf[192];
        wsprintfA(buf,
            "[CAMPATCH] call #%d this=%08x rec.pos=(%d,%d,%d)e-3 "
            "rec.angle=(%d,%d,%d) turn_rate=%d e-3 (NO-OP -- fix not wired)",
            g_call_count, (unsigned int)this_ptr,
            (int)(pos[0] * 1000.0f), (int)(pos[1] * 1000.0f), (int)(pos[2] * 1000.0f),
            (int)angle[0], (int)angle[1], (int)angle[2],
            (int)(turn_rate * 1000.0f));
        plog(buf);
    }

    /* M1: deliberately does none of the original work. See file header. */
}

static void install_camera_patch(void) {
    if (!file_exists(ENABLE_FLAG)) {
        plog("[CAMPATCH] enable flag absent -- hook NOT installed, zero behaviour change");
        return;
    }

    unsigned int base = (unsigned int)GetModuleHandleA(NULL);
    unsigned int target = base + (ADDR_UPDATE_WALKCAM_B - ASSUMED_IMAGE_BASE);

    char buf[128];
    wsprintfA(buf, "[CAMPATCH] enable flag present -- module base=%08x, target=%08x",
              base, target);
    plog(buf);

    if (base != ASSUMED_IMAGE_BASE) {
        wsprintfA(buf,
            "[CAMPATCH] WARNING: module base %08x != assumed %08x -- "
            "target address may be wrong, proceeding anyway (rebased offset)",
            base, ASSUMED_IMAGE_BASE);
        plog(buf);
    }

    if (hook_install(&g_walkcam_hook, (void *)target,
                     (void *)patched_UpdateWalkingCameraB)) {
        plog("[CAMPATCH] hook installed OK");
    } else {
        plog("[CAMPATCH] hook install FAILED (VirtualProtect?)");
    }
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinst);
        plog("[CAMPATCH] ddraw.dll shim loaded");
        install_camera_patch();
    }
    return TRUE;
}
