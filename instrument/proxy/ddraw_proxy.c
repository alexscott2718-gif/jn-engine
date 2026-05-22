/*
 * OMT2 live-instrumentation proxy -- ddraw.dll  (Milestone M1: pass-through)
 *
 * Stands in for the system ddraw.dll next to Neutron.exe.
 *
 * The proxy MUST mirror the real ddraw.dll's full export table -- all 22
 * symbols, at their exact ordinals (see ddraw_proxy.def). OMT2.dll imports only
 * DirectDrawCreateEx + DirectDrawEnumerateA, but when the game creates a
 * Direct3D device the runtime loads d3dim700.dll, which imports ddraw internals
 * (AcquireDDThreadLock, D3DParseUnknownCommand, ...). Those resolve against
 * THIS module, so every export must be present and correctly ordinalled or the
 * D3D DLL fails to load and the game aborts.
 *
 * DirectDrawCreateEx + DirectDrawEnumerateA are implemented here (the two M2+
 * cares about); the other 20 are .def forwarders straight to ddraw_orig.dll.
 *
 * Single real-ddraw module: both the forwarders and ensure_real() resolve to
 * ddraw_orig.dll (the renamed real ddraw shipped alongside). Loading it by the
 * distinct name "ddraw_orig.dll" cannot recurse into this proxy, and keeps one
 * shared instance -- two copies of ddraw would have independent global state
 * (thread locks, surface tables) and crash.
 *
 * M1 contract: game runs bit-identical, proxy load is logged, zero behaviour
 * change. COM wrapping starts in M2.
 */

#include <windows.h>
#include "com_wrappers.h"

#ifndef E_FAIL
#define E_FAIL ((HRESULT)0x80004005L)
#endif

static const char REAL_DDRAW[]  = "ddraw_orig.dll";
static const char LOG_PATH[]    = "C:\\omtc.log";
static const char DISABLE_FLAG[] = "C:\\omtc_disable";

static HINSTANCE g_hinst    = NULL;  /* this proxy's module handle */
static HMODULE   g_real     = NULL;  /* loaded ddraw_orig.dll      */
static int       g_disabled = 0;     /* C:\omtc_disable present    */

typedef HRESULT (WINAPI *PFN_DDCreateEx)(void *, void *, void *, void *);
typedef HRESULT (WINAPI *PFN_DDEnumerateA)(void *, void *);

static PFN_DDCreateEx   p_DirectDrawCreateEx   = NULL;
static PFN_DDEnumerateA p_DirectDrawEnumerateA = NULL;

/* --- logging --------------------------------------------------------------
 * Append a timestamped line to C:\omtc.log. Self-contained (Win32 file API
 * only) so it is safe to call from DllMain without touching the CRT. */
void omtc_log(const char *msg)
{
    HANDLE h = CreateFileA(LOG_PATH, FILE_APPEND_DATA,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;

    SYSTEMTIME st;
    GetLocalTime(&st);

    char buf[640];
    int n = wsprintfA(buf, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] %s\r\n",
                      st.wYear, st.wMonth, st.wDay,
                      st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg);

    DWORD written;
    SetFilePointer(h, 0, NULL, FILE_END);
    WriteFile(h, buf, (DWORD)n, &written, NULL);
    CloseHandle(h);
}

/* --- real-ddraw resolver --------------------------------------------------
 * Loads ddraw_orig.dll from this proxy's OWN directory (absolute path, so the
 * game's current directory is irrelevant). Done lazily on first export call,
 * never in DllMain. The game initialises ddraw single-threaded, so a plain
 * flag is sufficient. */
static int ensure_real(void)
{
    if (g_real)
        return 1;

    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(g_hinst, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        omtc_log("ERROR: GetModuleFileNameA failed");
        return 0;
    }
    while (n > 0 && path[n - 1] != '\\' && path[n - 1] != '/')
        n--;                                   /* strip "ddraw.dll"        */
    if (n + sizeof(REAL_DDRAW) > MAX_PATH) {
        omtc_log("ERROR: real-ddraw path too long");
        return 0;
    }
    for (DWORD i = 0; i < sizeof(REAL_DDRAW); i++)  /* incl. NUL */
        path[n + i] = REAL_DDRAW[i];

    HMODULE m = LoadLibraryA(path);
    if (!m) {
        omtc_log("ERROR: LoadLibrary of ddraw_orig.dll failed");
        return 0;
    }

    p_DirectDrawCreateEx =
        (PFN_DDCreateEx)GetProcAddress(m, "DirectDrawCreateEx");
    p_DirectDrawEnumerateA =
        (PFN_DDEnumerateA)GetProcAddress(m, "DirectDrawEnumerateA");

    if (!p_DirectDrawCreateEx || !p_DirectDrawEnumerateA) {
        omtc_log("ERROR: GetProcAddress on ddraw_orig.dll failed");
        FreeLibrary(m);
        return 0;
    }

    g_real = m;
    omtc_log("ddraw_orig.dll loaded; DirectDrawCreateEx + "
             "DirectDrawEnumerateA resolved");
    return 1;
}

/* --- exported entry points ------------------------------------------------
 * Args are kept void*-typed: every parameter is pointer-sized, so the __stdcall
 * stack frame matches the real ddraw exactly without pulling in ddraw.h. */
static int g_selfcheck_done = 0;

HRESULT WINAPI DirectDrawCreateEx(void *lpGuid, void *lplpDD,
                                  void *iid, void *pUnkOuter)
{
    if (!ensure_real())
        return E_FAIL;
    HRESULT hr = p_DirectDrawCreateEx(lpGuid, lplpDD, iid, pUnkOuter);

    /* M2: wrap the returned IDirectDraw7 so the game drives our vtable chain.
     * The kill-switch (C:\omtc_disable) forces pure pass-through. */
    if (SUCCEEDED(hr) && !g_disabled && lplpDD && *(void **)lplpDD) {
        if (!g_selfcheck_done) {
            omtc_com_selfcheck();
            g_selfcheck_done = 1;
        }
        *(void **)lplpDD = omtc_wrap_dd7_byiid(iid, *(void **)lplpDD);
    }
    {
        char buf[64];
        wsprintfA(buf, "DirectDrawCreateEx -> hr=0x%08lX", (unsigned long)hr);
        omtc_log(buf);
    }
    return hr;
}

HRESULT WINAPI DirectDrawEnumerateA(void *lpCallback, void *lpContext)
{
    if (!ensure_real())
        return E_FAIL;
    HRESULT hr = p_DirectDrawEnumerateA(lpCallback, lpContext);
    {
        char buf[64];
        wsprintfA(buf, "DirectDrawEnumerateA -> pass-through, hr=0x%08lX",
                  (unsigned long)hr);
        omtc_log(buf);
    }
    return hr;
}

/* --- DllMain -------------------------------------------------------------- */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        g_hinst = hinst;
        DisableThreadLibraryCalls(hinst);
        g_disabled =
            (GetFileAttributesA(DISABLE_FLAG) != INVALID_FILE_ATTRIBUTES);
        omtc_log("=== omtc proxy ddraw.dll loaded (M1 pass-through) ===");
        if (g_disabled)
            omtc_log("C:\\omtc_disable present -> kill-switch armed "
                     "(pure pass-through)");
    }
    return TRUE;
}
