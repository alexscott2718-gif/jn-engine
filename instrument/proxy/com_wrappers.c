/*
 * com_wrappers.c -- DX7 COM wrapper chain for the omtc instrumentation proxy.
 * M2: wrap IDirectDraw7 -> IDirect3D7 -> IDirect3DDevice7 plus
 * IDirectDrawSurface7. M3: pre-forward hooks on BeginScene/EndScene/
 * DrawPrimitive/DrawIndexedPrimitive/CreateDevice emit frame markers, draw
 * counts and the device-acquisition path to C:\omtc.log. Forwarding itself
 * is still verbatim -- the hooks are pure side effects.
 *
 * Design:
 *  - Uniform wrapper `ComWrap { lpVtbl; real; }` -- lpVtbl first, so a
 *    ComWrap* is ABI-identical to any IFoo* the game expects.
 *  - One static vtable of __stdcall thunks per interface; the 136 thunks are
 *    machine-generated into com_wrappers_gen.inc by gen_wrappers.py from the
 *    same ddraw.h/d3d.h this file includes (layout is therefore guaranteed
 *    to match the compiler's struct view).
 *  - Identity table: a fixed static pool, keyed on (real, vtbl), so a given
 *    real object always yields the same wrapper -- required for stable
 *    surface IDs in M4. No malloc (the proxy is CRT-free).
 *  - unwrap(): the game hands our wrappers back as method arguments; those
 *    are translated to the real object before forwarding.
 */

#define CINTERFACE          /* C-style COM: interfaces expose ->lpVtbl */
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include "com_wrappers.h"

/* R1 cross-check, compile time: the mingw DX7 device vtable must match the
 * ABI Ghidra measured (DrawPrimitive at byte offset 0x64 == slot 25). */
_Static_assert(__builtin_offsetof(IDirect3DDevice7Vtbl, DrawPrimitive) == 0x64,
               "IDirect3DDevice7 vtable layout mismatch vs DX7 ABI");

/* --- wrapper pool / identity ------------------------------------------- */

typedef struct { const void *lpVtbl; void *real; } ComWrap;

#define POOL_N 8192
static ComWrap g_pool[POOL_N];
static int     g_pool_hw;          /* high-water mark */

static int in_pool(const void *p)
{
    return p >= (const void *)&g_pool[0] && p < (const void *)&g_pool[POOL_N];
}

/* If p is one of our wrappers, return the real object; otherwise return p. */
static void *unwrap(void *p)
{
    if (p && in_pool(p))
        return ((ComWrap *)p)->real;
    return p;
}

#define REAL(IF, This) ((IF *)(((ComWrap *)(This))->real))

/* forward declarations referenced by the generated thunks */
static HRESULT STDMETHODCALLTYPE generic_QueryInterface(void *This, REFIID riid,
                                                        void **ppv);
static ULONG   STDMETHODCALLTYPE generic_Release(void *This);
static void *wrap_dd7(void *r);
static void *wrap_d3d7(void *r);
static void *wrap_device(void *r);
static void *wrap_surface(void *r);

/* --- M3: frame markers + draw counts ------------------------------------
 * BeginScene/EndScene bracket a frame; DrawPrimitive + DrawIndexedPrimitive
 * are counted within it (the strided/VB draw variants are intentionally not
 * counted -- Ghidra shows OMT2 renders only through DrawPrimitive). The
 * generated thunks call these hooks pre-forward; behaviour is unchanged.
 *
 * State is plain (no Interlocked): DX7 rendering is single-threaded here --
 * the game inits ddraw single-threaded (see M1 notes) and Begin/Draw/End all
 * run on that one render thread.
 *
 * Output is text to C:\omtc.log: the first few frames are logged
 * individually (confirms the path is live immediately), then a rolling
 * summary every SUMMARY_EVERY frames so the log does not flood. All maths is
 * 32-bit -- the proxy is CRT-free and 64-bit division would need libgcc
 * helpers that -nostdlib does not link. */

#define SUMMARY_EVERY 300u

static unsigned g_frame_seq;     /* frames seen (BeginScene count)         */
static unsigned g_dp_frame;      /* DrawPrimitive calls, current frame     */
static unsigned g_dip_frame;     /* DrawIndexedPrimitive calls, this frame */
static DWORD    g_frame_t0;      /* GetTickCount at current BeginScene     */

static unsigned g_win_frames;    /* frames since last summary              */
static DWORD    g_win_t0;        /* GetTickCount at summary-window start    */
static unsigned g_win_dp;        /* DrawPrimitive total over the window    */
static unsigned g_win_dip;       /* DrawIndexedPrimitive total, window     */
static unsigned g_win_min;       /* min draws/frame over the window        */
static unsigned g_win_max;       /* max draws/frame over the window        */

static void win_reset(void)
{
    g_win_frames = 0;
    g_win_t0     = GetTickCount();
    g_win_dp     = 0;
    g_win_dip    = 0;
    g_win_min    = 0xFFFFFFFFu;
    g_win_max    = 0;
}

static void omtc_frame_begin(void)
{
    if (g_frame_seq == 0) {
        omtc_log("M3: first BeginScene -- frame instrumentation live");
        win_reset();
    }
    g_dp_frame  = 0;
    g_dip_frame = 0;
    g_frame_t0  = GetTickCount();
}

static void omtc_count_draw_dp(void)  { g_dp_frame++;  }
static void omtc_count_draw_dip(void) { g_dip_frame++; }

static void omtc_frame_end(void)
{
    unsigned draws = g_dp_frame + g_dip_frame;
    DWORD    dt    = GetTickCount() - g_frame_t0;

    g_win_frames++;
    g_win_dp  += g_dp_frame;
    g_win_dip += g_dip_frame;
    if (draws < g_win_min) g_win_min = draws;
    if (draws > g_win_max) g_win_max = draws;

    if (g_frame_seq < 5) {
        char buf[128];
        wsprintfA(buf, "M3 frame %u: %u draws (DP=%u DIP=%u), %lu ms",
                  g_frame_seq, draws, g_dp_frame, g_dip_frame,
                  (unsigned long)dt);
        omtc_log(buf);
    }
    g_frame_seq++;

    if (g_win_frames >= SUMMARY_EVERY) {
        DWORD    wdt = GetTickCount() - g_win_t0;
        unsigned avg_dp, avg_dip, fps100;
        char buf[224];
        if (wdt == 0) wdt = 1;
        /* fps in hundredths, integer-only: frames * 100000 stays well within
         * 32 bits (300 * 100000 = 3e7), and i386 has native 32-bit divide. */
        fps100  = (g_win_frames * 100000u) / wdt;
        avg_dp  = g_win_dp  / g_win_frames;
        avg_dip = g_win_dip / g_win_frames;
        wsprintfA(buf,
            "M3 summary: frames %u..%u  %u in %lu ms (%u.%02u fps)  "
            "draws/frame min=%u max=%u avg=%u  (DP avg=%u DIP avg=%u)",
            g_frame_seq - g_win_frames, g_frame_seq - 1,
            g_win_frames, (unsigned long)wdt,
            fps100 / 100u, fps100 % 100u,
            g_win_min, g_win_max, avg_dp + avg_dip, avg_dp, avg_dip);
        omtc_log(buf);
        win_reset();
    }
}

/* Device-acquisition path -- closes the notes open question of whether OMT2
 * gets its device via IDirect3D7::CreateDevice or QueryInterface. One-shot;
 * the QI path is logged from generic_QueryInterface below. */
static void omtc_note_createdevice(void)
{
    static int logged;
    if (!logged) {
        logged = 1;
        omtc_log("M3: device acquisition path = IDirect3D7::CreateDevice");
    }
}

/* --- M4: capture hooks (argument-aware) ----------------------------------- */
/* These hooks receive method arguments and call the capture.c functions.
 * The hooks are called pre-forward, so arguments are in their original form
 * -- except interface pointers, which the game hands us as wrappers and we
 * unwrap() to the real object. Forward declarations from capture.h. */

#include "capture.h"
#include "protocol.h"   /* OMTC_TRANSFORM_* */

/* Field offsets into DDSURFACEDESC2 / DDPIXELFORMAT. The structs carry several
 * named-or-anonymous unions whose member spelling depends on header macros;
 * reading by fixed offset (the layout is fixed by the DX7 ABI) sidesteps that. */
#define DDSD2_SIZE            0x7C
#define DDSD2_OFF_HEIGHT      0x08
#define DDSD2_OFF_WIDTH       0x0C
#define DDSD2_OFF_PITCH       0x10
#define DDSD2_OFF_LPSURFACE   0x24
#define DDSD2_OFF_PF_FLAGS    0x4C   /* ddpfPixelFormat(@0x48) + dwFlags(@0x4) */
#define DDSD2_OFF_RGBBITCOUNT 0x54   /* ddpfPixelFormat(@0x48) + dwRGBBitCount(@0xC) */
#define DDSD2_OFF_RBITMASK    0x58
#define DDSD2_OFF_GBITMASK    0x5C
#define DDSD2_OFF_BBITMASK    0x60
#define DDSD2_OFF_ABITMASK    0x64

/* M4: frame markers. Bracket each frame on the file sink, alongside the
 * M3 text-log frame summary (kept -- it confirms the path stays live). */
static void omtc_hook_frame_begin(void)
{
    omtc_frame_begin();          /* M3: text-log summary */
    omtc_capture_frame_begin();  /* M4: FRAME_BEGIN record + lazy file open */
}
static void omtc_hook_frame_end(void)
{
    omtc_capture_frame_end();    /* M4: FRAME_END record */
    omtc_frame_end();            /* M3: text-log summary */
}

static void omtc_capture_draw_primitive(D3DPRIMITIVETYPE prim_type, DWORD fvf,
                                        void *vertices, DWORD vtx_count)
{
    /* arg order from the D3D method: (prim_type, fvf, vertices, vtx_count). */
    omtc_draw_primitive((uint8_t)prim_type, (uint32_t)fvf,
                        (uint32_t)vtx_count, vertices);
}

static void omtc_capture_set_transform(D3DTRANSFORMSTATETYPE xformType,
                                       LPD3DMATRIX lpD3DMatrix)
{
    /* Transform types: WORLD=1, VIEW=2, PROJECTION=3 (D3DTRANSFORMSTATETYPE).
     * The protocol uses 0/1/2 -- remap. */
    uint8_t which;
    switch (xformType) {
        case D3DTRANSFORMSTATE_WORLD:      which = OMTC_TRANSFORM_WORLD; break;
        case D3DTRANSFORMSTATE_VIEW:       which = OMTC_TRANSFORM_VIEW;  break;
        case D3DTRANSFORMSTATE_PROJECTION: which = OMTC_TRANSFORM_PROJ;  break;
        default: return;  /* texture / other transform stages: ignored */
    }
    if (lpD3DMatrix)
        omtc_set_transform(which, (const float *)lpD3DMatrix);
}

static void omtc_cache_colorkey_if_present(IDirectDrawSurface7 *s,
                                           void *real, DWORD flags)
{
    DDCOLORKEY ck;
    HRESULT hr = s->lpVtbl->GetColorKey(s, flags, &ck);
    if (SUCCEEDED(hr))
        omtc_note_surface_colorkey(real, flags, ck.dwColorSpaceLowValue,
                                   ck.dwColorSpaceHighValue, 1, 0);
    else
        omtc_note_surface_colorkey(real, flags, 0, 0, 0, 0);
}

static void omtc_cache_surface_colorkeys(IDirectDrawSurface7 *s, void *real)
{
    omtc_cache_colorkey_if_present(s, real, DDCKEY_SRCBLT);
    omtc_cache_colorkey_if_present(s, real, DDCKEY_DESTBLT);
    omtc_cache_colorkey_if_present(s, real, DDCKEY_SRCOVERLAY);
    omtc_cache_colorkey_if_present(s, real, DDCKEY_DESTOVERLAY);
}

static void omtc_dump_surface_texture(void *real, int known_only)
{
    if (!real)
        return;
    if (known_only && !omtc_texture_is_known(real))
        return;

    IDirectDrawSurface7 *s = (IDirectDrawSurface7 *)real;
    uint32_t tid = omtc_texture_id(real);
    unsigned char desc[DDSD2_SIZE];
    for (int i = 0; i < DDSD2_SIZE; i++) desc[i] = 0;
    *(DWORD *)desc = DDSD2_SIZE;  /* dwSize */
    HRESULT hr = s->lpVtbl->Lock(s, NULL, (LPDDSURFACEDESC2)desc,
                                 DDLOCK_READONLY | DDLOCK_WAIT, NULL);
    if (SUCCEEDED(hr)) {
        DWORD  w     = *(DWORD *)(desc + DDSD2_OFF_WIDTH);
        DWORD  h     = *(DWORD *)(desc + DDSD2_OFF_HEIGHT);
        LONG   pitch = *(LONG  *)(desc + DDSD2_OFF_PITCH);
        void  *bits  = *(void **)(desc + DDSD2_OFF_LPSURFACE);
        DWORD  bpp   = *(DWORD *)(desc + DDSD2_OFF_RGBBITCOUNT);
        DWORD  pf_flags = *(DWORD *)(desc + DDSD2_OFF_PF_FLAGS);
        DWORD  r_mask = *(DWORD *)(desc + DDSD2_OFF_RBITMASK);
        DWORD  g_mask = *(DWORD *)(desc + DDSD2_OFF_GBITMASK);
        DWORD  b_mask = *(DWORD *)(desc + DDSD2_OFF_BBITMASK);
        DWORD  a_mask = *(DWORD *)(desc + DDSD2_OFF_ABITMASK);
        uint32_t row_bytes = (uint32_t)w * ((bpp + 7) / 8);
        omtc_cache_surface_colorkeys(s, real);
        omtc_register_texture(tid, (uint16_t)w, (uint16_t)h, bpp,
                              pf_flags, bpp, r_mask, g_mask, b_mask,
                              a_mask, bits, (int32_t)pitch, row_bytes);
        s->lpVtbl->Unlock(s, NULL);
    }
}

/* On first sighting of a texture surface, lock it read-only, hash the pixels
 * and emit one TEXTURE_DEF. The game hands us a wrapper -> unwrap to the real
 * surface before locking and before deriving the id. */
static void omtc_capture_set_texture(DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTexture)
{
    void *real = unwrap((void *)lpTexture);
    uint32_t tid = omtc_texture_id(real);

    if (real && omtc_texture_is_new(real, tid)) {
        omtc_dump_surface_texture(real, 0);
        /* Lock failure (R3): entry stays marked seen; TEXTURE_DEF omitted. */
    }
    omtc_set_texture((uint8_t)dwStage, real);
}

static void omtc_capture_load_result(IDirect3DDevice7 *This, HRESULT hr,
                                     IDirectDrawSurface7 *dst)
{
    (void)This;
    if (FAILED(hr))
        return;
    omtc_dump_surface_texture(unwrap((void *)dst), 1);
}

static void omtc_capture_surface_mutation_result(IDirectDrawSurface7 *This,
                                                 HRESULT hr)
{
    if (FAILED(hr))
        return;
    omtc_dump_surface_texture(unwrap((void *)This), 1);
}

static void omtc_capture_set_colorkey_result(IDirectDrawSurface7 *This,
                                             HRESULT hr, DWORD flags,
                                             LPDDCOLORKEY ck)
{
    if (FAILED(hr))
        return;
    void *real = unwrap((void *)This);
    if (ck)
        omtc_note_surface_colorkey(real, flags, ck->dwColorSpaceLowValue,
                                   ck->dwColorSpaceHighValue, 1, 1);
    else
        omtc_note_surface_colorkey(real, flags, 0, 0, 0, 1);
}

static void omtc_capture_get_colorkey_result(IDirectDrawSurface7 *This,
                                             HRESULT hr, DWORD flags,
                                             LPDDCOLORKEY ck)
{
    if (FAILED(hr) || !ck)
        return;
    void *real = unwrap((void *)This);
    omtc_note_surface_colorkey(real, flags, ck->dwColorSpaceLowValue,
                               ck->dwColorSpaceHighValue, 1, 1);
}

static void omtc_capture_set_renderstate(D3DRENDERSTATETYPE dwRenderStateType,
                                         DWORD dwRenderState)
{
    omtc_set_renderstate((uint32_t)dwRenderStateType, (uint32_t)dwRenderState);
}

static void omtc_capture_set_texstagestate(DWORD dwStage,
                                           D3DTEXTURESTAGESTATETYPE dwState,
                                           DWORD dwValue)
{
    omtc_set_texstagestate((uint8_t)dwStage, (uint32_t)dwState,
                           (uint32_t)dwValue);
}

static void omtc_capture_set_viewport(LPD3DVIEWPORT7 vp)
{
    if (vp)
        omtc_set_viewport((int32_t)vp->dwX, (int32_t)vp->dwY,
                          (int32_t)vp->dwWidth, (int32_t)vp->dwHeight,
                          vp->dvMinZ, vp->dvMaxZ);
}

static void omtc_capture_set_light(DWORD dwIndex, LPD3DLIGHT7 lpLight)
{
    if (lpLight)
        omtc_set_light((uint8_t)dwIndex, lpLight, sizeof(D3DLIGHT7));
}

static void omtc_capture_set_material(LPD3DMATERIAL7 lpMaterial)
{
    if (lpMaterial)
        omtc_set_material(lpMaterial, sizeof(D3DMATERIAL7));
}

#include "com_wrappers_gen.inc"

/* --- interface identity (GUIDs) ---------------------------------------- */

static const GUID IID_IDirectDraw7_ =
    {0x15e65ec0,0x3b9c,0x11d2,{0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b}};
static const GUID IID_IDirect3D7_ =
    {0xf5049e77,0x4861,0x11d2,{0xa4,0x07,0x00,0xa0,0xc9,0x06,0x29,0xa8}};
static const GUID IID_IDirect3DDevice7_ =
    {0xf5049e79,0x4861,0x11d2,{0xa4,0x07,0x00,0xa0,0xc9,0x06,0x29,0xa8}};
static const GUID IID_IDirectDrawSurface7_ =
    {0x06675a80,0x3b9b,0x11d2,{0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b}};

static int guid_eq(const GUID *a, const GUID *b)
{
    const unsigned char *x = (const unsigned char *)a;
    const unsigned char *y = (const unsigned char *)b;
    for (int i = 0; i < 16; i++)
        if (x[i] != y[i])
            return 0;
    return 1;
}

/* --- wrap helpers ------------------------------------------------------- */

static void *wrap_generic(void *real, const void *vtbl)
{
    if (!real)
        return 0;
    /* identity: the same (real,vtbl) always yields the same wrapper */
    for (int i = 0; i < g_pool_hw; i++)
        if (g_pool[i].real == real && g_pool[i].lpVtbl == vtbl)
            return &g_pool[i];
    /* allocate: reuse a slot freed by Release, else extend the pool */
    ComWrap *w = 0;
    for (int i = 0; i < g_pool_hw; i++)
        if (g_pool[i].real == 0) { w = &g_pool[i]; break; }
    if (!w) {
        if (g_pool_hw >= POOL_N) {
            omtc_log("ERROR: COM wrapper pool exhausted -- passing unwrapped");
            return real;
        }
        w = &g_pool[g_pool_hw++];
    }
    w->lpVtbl = vtbl;
    w->real   = real;
    return w;
}

static void *wrap_dd7(void *r)     { return wrap_generic(r, &w_IDirectDraw7_vtbl); }
static void *wrap_d3d7(void *r)    { return wrap_generic(r, &w_IDirect3D7_vtbl); }
static void *wrap_device(void *r)  { return wrap_generic(r, &w_IDirect3DDevice7_vtbl); }
static void *wrap_surface(void *r) { return wrap_generic(r, &w_IDirectDrawSurface7_vtbl); }

/* --- generic IUnknown methods ------------------------------------------ */

static HRESULT STDMETHODCALLTYPE generic_QueryInterface(void *This, REFIID riid,
                                                        void **ppv)
{
    IDirectDraw7 *real = (IDirectDraw7 *)((ComWrap *)This)->real;
    HRESULT hr = real->lpVtbl->QueryInterface(real, riid, ppv);
    if (SUCCEEDED(hr) && ppv && *ppv) {
        if (guid_eq(riid, &IID_IDirectDraw7_))
            *ppv = wrap_dd7(*ppv);
        else if (guid_eq(riid, &IID_IDirect3D7_))
            *ppv = wrap_d3d7(*ppv);
        else if (guid_eq(riid, &IID_IDirect3DDevice7_)) {
            static int logged_qi_device;
            if (!logged_qi_device) {
                logged_qi_device = 1;
                omtc_log("M3: device acquisition path = "
                         "QueryInterface(IID_IDirect3DDevice7)");
            }
            *ppv = wrap_device(*ppv);
        }
        else if (guid_eq(riid, &IID_IDirectDrawSurface7_))
            *ppv = wrap_surface(*ppv);
        /* any other interface: passed through unwrapped (M2 is the 4-chain) */
    }
    return hr;
}

static ULONG STDMETHODCALLTYPE generic_Release(void *This)
{
    void *real = ((ComWrap *)This)->real;
    ULONG rc = ((IDirectDraw7 *)real)->lpVtbl->Release((IDirectDraw7 *)real);
    if (rc == 0) {
        /* real object destroyed -- free every wrapper that pointed at it */
        for (int i = 0; i < g_pool_hw; i++)
            if (g_pool[i].real == real) {
                g_pool[i].real   = 0;
                g_pool[i].lpVtbl = 0;
            }
    }
    return rc;
}

/* --- public API --------------------------------------------------------- */

void omtc_com_selfcheck(void)
{
    unsigned off =
        (unsigned)__builtin_offsetof(IDirect3DDevice7Vtbl, DrawPrimitive);
    char buf[112];
    wsprintfA(buf, "COM self-check: IDirect3DDevice7::DrawPrimitive "
                   "vtbl offset=0x%X (expect 0x64) -- %s",
              off, off == 0x64 ? "vtbl OK" : "MISMATCH");
    omtc_log(buf);
}

void *omtc_wrap_dd7_byiid(const void *iid, void *real_dd)
{
    if (real_dd && iid && guid_eq((const GUID *)iid, &IID_IDirectDraw7_)) {
        omtc_log("DirectDraw7 wrapped");
        return wrap_dd7(real_dd);
    }
    return real_dd;
}
