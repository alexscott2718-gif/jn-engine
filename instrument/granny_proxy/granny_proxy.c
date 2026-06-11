/*
 * granny.dll capture proxy for JNvsJN (Jimmy Neutron vs Jimmy Negatron).
 *
 * M3a capture build. Most exports stay forwarders to granny_orig.dll
 * (granny_proxy.def). A focused set of data-flow exports are real thunks here
 * that call granny_orig, dump decoded mesh streams to C:\grn_dump, and keep a
 * small handle->source .grn name map so captures are not all named "llun"/"null".
 *
 * LockNextNewTexture uses an OUT buffer too. The decoded layout is dumped to
 * .grntex files: source/material names, dimensions, pitch, and raw RGB rows.
 *
 * M3a ANIMATION CAPTURE (opt-in, off by default): if the environment variable
 * GRN_ANIM_SRC is set, every LockNextRenderingState whose source .grn name
 * contains that (case-insensitive) substring also appends the current POSED
 * float streams (Granny already deformed them for the live frame) plus the
 * per-frame transform to a per-descriptor C:\grn_dump\a<descp>.grnanim file.
 * The Debian exporter (grnanim_to_glb.py) turns base mesh + .grnanim into a
 * glTF morph-target animation. Hard-bounded (descriptors, samples, bytes) and
 * disabled when GRN_ANIM_SRC is empty, so the DLL is safe to leave deployed.
 *
 * XP-safety: -nostdlib (no UCRT). kernel32 + user32 (wsprintfA) only. DllMain is
 * the raw entry point (-Wl,-e,DllMain@12).
 *
 * Decorated-export trick: the real export name is `_GrannyX@N`; to emit that,
 * the C thunk is *named* `_GrannyX` so stdcall mangling yields `__GrannyX@N`,
 * which lld matches when resolving the .def name `_GrannyX@N` (it prepends `_`).
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdarg.h>

/* ---- capture sink -------------------------------------------------------- */

#define LOG_PATH        "C:\\granny_cap.log"
#define FIRSTN          8       /* verbatim records logged per function */
#define SUMMARY_TICK_MS 2000    /* min gap between SUMMARY blocks */

enum { H_OpenModel, H_OpenSeq, H_LockSeqRender, H_StatesLeft,
       H_LockState, H_UnlockState, H_TexLeft, H_LockTex, H_COUNT };

static const char *const HNAME[H_COUNT] = {
    "OpenModel", "OpenSequence", "LockSequenceForRendering",
    "GetRenderingStatesLeft", "LockNextRenderingState",
    "UnlockRenderingState", "GetNewTexturesLeft", "LockNextNewTexture"
};

static HMODULE          g_orig = NULL;
static HANDLE           g_log  = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION g_cs;
static int              g_ready = 0;

static LONG  g_calls[H_COUNT];     /* total calls */
static LONG  g_nonnull[H_COUNT];   /* calls returning non-zero */
static LONG  g_maxret[H_COUNT];    /* max return value seen */
static DWORD g_last_summary = 0;

static void logf(const char *fmt, ...);
static int readable(uint32_t p, uint32_t len);
static uint32_t peek(uint32_t p);

/* ---- model/sequence/render-handle -> source filename map ---------------- */

#define MAX_SRC_NAME 96
#define MAX_TEX_TEXT 128
#define MAX_NAME_MAP 512

typedef struct {
    uint32_t handle;
    char     name[MAX_SRC_NAME];
} NameMap;

static NameMap g_name_map[MAX_NAME_MAP];
static int     g_name_map_n = 0;
static char    g_pending_seq_name[MAX_SRC_NAME];
static int     g_pending_seq_valid = 0;
static LONG    g_map_logs = 0;

static int streq(const char *a, const char *b)
{
    int i = 0;
    while (a[i] && b[i] && a[i] == b[i]) ++i;
    return a[i] == b[i];
}

static void strcopy(char *dst, int cap, const char *src)
{
    int i = 0;
    if (cap <= 0) return;
    while (i < cap - 1 && src && src[i]) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = 0;
}

static void wr(HANDLE h, const void *p, DWORD n)
{
    DWORD w;
    WriteFile(h, p, n, &w, NULL);
}

static void wr_text_record(HANDLE h, const char tag[4], const char *s)
{
    uint32_t len = (uint32_t)lstrlenA(s ? s : "");
    wr(h, tag, 4);
    wr(h, &len, 4);
    if (len) wr(h, s, len);
}

static int copy_cstr(char *dst, int cap, uint32_t p)
{
    int i = 0;
    if (cap <= 0) return 0;
    while (i < cap - 1) {
        char ch;
        if (!readable(p + (uint32_t)i, 1)) break;
        ch = *(volatile char *)(uintptr_t)(p + (uint32_t)i);
        if (!ch) break;
        /* Expected paths are ASCII like "grn\\jimmybase.grn". Stop on control
         * bytes so a bad pointer cannot produce noisy binary names. */
        if ((unsigned char)ch < 0x20 || (unsigned char)ch > 0x7e) break;
        dst[i++] = ch;
    }
    dst[i] = 0;
    return i;
}

static int plausible_handle(uint32_t h)
{
    return h > 0x00010000u && h != 0xffffffffu;
}

static const char *name_lookup(uint32_t h)
{
    int i;
    if (!plausible_handle(h)) return 0;
    for (i = 0; i < g_name_map_n; ++i)
        if (g_name_map[i].handle == h) return g_name_map[i].name;
    return 0;
}

static const char *name_lookup_near(uint32_t h)
{
    int i;
    if (!plausible_handle(h)) return 0;
    for (i = 0; i < g_name_map_n; ++i) {
        uint32_t base = g_name_map[i].handle;
        if (h >= base && h - base <= 0x20u) return g_name_map[i].name;
    }
    return 0;
}

static void name_put(uint32_t h, const char *name)
{
    int i;
    if (!plausible_handle(h) || !name || !name[0]) return;
    for (i = 0; i < g_name_map_n; ++i) {
        if (g_name_map[i].handle == h) {
            if (!streq(g_name_map[i].name, name))
                strcopy(g_name_map[i].name, MAX_SRC_NAME, name);
            return;
        }
    }
    if (g_name_map_n >= MAX_NAME_MAP) return;
    g_name_map[g_name_map_n].handle = h;
    strcopy(g_name_map[g_name_map_n].name, MAX_SRC_NAME, name);
    ++g_name_map_n;
}

static void map_log(const char *kind, uint32_t handle, const char *name)
{
    if (InterlockedIncrement(&g_map_logs) <= 96)
        logf("NAMEMAP %-8s handle=%08x src=%s\n", kind, handle, name ? name : "");
}

static void *real(const char *decorated_name)
{
    return (void *)GetProcAddress(g_orig, decorated_name);
}

static void raw_write(const char *s, int n)
{
    DWORD w;
    if (g_log != INVALID_HANDLE_VALUE)
        WriteFile(g_log, s, (DWORD)n, &w, NULL);
}

static void logf(const char *fmt, ...)
{
    char buf[1024];
    int  n;
    va_list ap;
    if (!g_ready) return;
    va_start(ap, fmt);
    n = wvsprintfA(buf, fmt, ap);
    va_end(ap);
    EnterCriticalSection(&g_cs);
    raw_write(buf, n);
    LeaveCriticalSection(&g_cs);
}

/* true if [p, p+len) is committed, readable memory -- so chasing a dword that
 * turns out NOT to be a pointer can't fault the game. VirtualQuery is XP-safe. */
static int readable(uint32_t p, uint32_t len)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (p < 0x10000) return 0;
    if (VirtualQuery((void *)p, &mbi, sizeof(mbi)) != sizeof(mbi)) return 0;
    if (mbi.State != MEM_COMMIT) return 0;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return 0;
    /* must not run past the end of this committed region */
    return (p + len) <= ((uint32_t)(uintptr_t)mbi.BaseAddress + mbi.RegionSize);
}

static void hexdump(const char *tag, const void *p, int len)
{
    static const char hx[] = "0123456789abcdef";
    const unsigned char *b = (const unsigned char *)p;
    char line[160];
    int  i, col, k;
    if (!g_ready) return;
    if (!readable((uint32_t)(uintptr_t)p, (uint32_t)len)) {
        logf("  %s = <unreadable %08x>\n", tag, (uint32_t)(uintptr_t)p);
        return;
    }
    EnterCriticalSection(&g_cs);
    for (i = 0; i < len; i += 16) {
        k = wsprintfA(line, "  %s+%03x:", tag, i);
        for (col = 0; col < 16 && i + col < len; ++col) {
            unsigned char v = b[i + col];
            line[k++] = ' ';
            line[k++] = hx[v >> 4];
            line[k++] = hx[v & 15];
        }
        line[k++] = '\n';
        raw_write(line, k);
    }
    LeaveCriticalSection(&g_cs);
}

static void chase_dwords(const char *tag, uint32_t base, int bytes, int target_len)
{
    int off;
    char t[48];
    if (!readable(base, (uint32_t)bytes)) return;
    for (off = 0; off + 4 <= bytes; off += 4) {
        uint32_t p = peek(base + (uint32_t)off);
        if (readable(p, (uint32_t)target_len)) {
            wsprintfA(t, "%s+%02x->%08x", tag, off, p);
            hexdump(t, (void *)(uintptr_t)p, target_len);
        }
    }
}

/* Update counters; return the 1-based call index for this function (for FIRSTN). */
static LONG note(int h, uint32_t ret)
{
    LONG n = InterlockedIncrement(&g_calls[h]);
    if (ret) {
        InterlockedIncrement(&g_nonnull[h]);
        if ((LONG)ret > g_maxret[h]) g_maxret[h] = (LONG)ret;  /* racy, fine for a hint */
    }
    return n;
}

/* Periodically emit a per-function tally so we see the call distribution even
 * when nothing returns meaningful data. Driven from the per-frame functions. */
static void summary_maybe(void)
{
    DWORD now;
    int   h;
    if (!g_ready) return;
    now = GetTickCount();
    if (now - g_last_summary < SUMMARY_TICK_MS) return;
    g_last_summary = now;
    EnterCriticalSection(&g_cs);
    {
        char hdr[64];
        int  k = wsprintfA(hdr, "--- SUMMARY @%ums ---\n", now);
        raw_write(hdr, k);
        for (h = 0; h < H_COUNT; ++h) {
            char line[160];
            int  m = wsprintfA(line, "  %-26s calls=%-7d nonnull=%-7d maxret=%08x\n",
                               HNAME[h], g_calls[h], g_nonnull[h], (uint32_t)g_maxret[h]);
            raw_write(line, m);
        }
    }
    LeaveCriticalSection(&g_cs);
}

/* ---- hooked exports ------------------------------------------------------ */

typedef uint32_t (__stdcall *fn1)(uint32_t);
typedef uint32_t (__stdcall *fn2)(uint32_t, uint32_t);
typedef uint32_t (__stdcall *fn3)(uint32_t, uint32_t, uint32_t);
typedef uint32_t (__stdcall *fn4)(uint32_t, uint32_t, uint32_t, uint32_t);

#define RESOLVE(var, name) do { if (!(var)) (var) = (void *)real(name); } while (0)

/* read the dword at p; 0 if not safely readable */
static uint32_t peek(uint32_t p)
{
    if (!readable(p, 4)) return 0;
    return *(volatile uint32_t *)p;
}

uint32_t __stdcall _GrannyOpenModel(uint32_t a, uint32_t b, uint32_t c)
{
    static fn3 r = 0; RESOLVE(r, "_GrannyOpenModel@12");
    uint32_t ret = r(a, b, c);
    {
        char fname[MAX_SRC_NAME];
        uint32_t model = peek(c + 4);
        if (copy_cstr(fname, MAX_SRC_NAME, b) && model) {
            name_put(model, fname);
            map_log("OpenModel", model, fname);
        }
    }
    if (note(H_OpenModel, ret) <= FIRSTN) {
        /* DIAG2: result comes back via an OUT param, not EAX. Dump every arg's
         * target after the call so we can see which one receives the model* and
         * which is the .grn file image. */
        logf("OpenModel(a=%08x, b=%08x, c=%08x) ret=%08x  *a=%08x *b=%08x *c=%08x\n",
             a, b, c, ret, peek(a), peek(b), peek(c));
        hexdump("argB", (void *)b, 48);
        hexdump("argC", (void *)c, 48);
        hexdump("*b->", (void *)peek(b), 128);   /* if b is model** -> model */
        hexdump("*c->", (void *)peek(c), 128);
    }
    return ret;
}

uint32_t __stdcall _GrannyOpenSequence(uint32_t a, uint32_t b, uint32_t c)
{
    static fn3 r = 0; RESOLVE(r, "_GrannyOpenSequence@12");
    uint32_t ret = r(a, b, c);
    {
        const char *src = name_lookup(b);
        uint32_t k;
        if (!src) src = name_lookup_near(b);
        if (src) {
            strcopy(g_pending_seq_name, MAX_SRC_NAME, src);
            g_pending_seq_valid = 1;
            /* Some builds write sequence/render handles into the OUT buffer.
             * Alias any plausible dwords now; LockSequenceForRendering below
             * aliases the handle actually used by the render path. */
            for (k = 0; k < 8; ++k) {
                uint32_t h = peek(c + k * 4);
                if (h != a && h != b && plausible_handle(h))
                    name_put(h, src);
            }
        } else {
            g_pending_seq_valid = 0;
        }
    }
    if (note(H_OpenSeq, ret) <= FIRSTN) {
        logf("OpenSequence(sys=%08x, %08x, mem=%08x) = seq %08x src=%s\n",
             a, b, c, ret, g_pending_seq_valid ? g_pending_seq_name : "");
        if (ret) hexdump("seq", (void *)ret, 96);
    }
    return ret;
}

uint32_t __stdcall _GrannyLockSequenceForRendering(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    static fn4 r = 0; RESOLVE(r, "_GrannyLockSequenceForRendering@16");
    const char *src = name_lookup(b);
    if (!src && g_pending_seq_valid) {
        src = g_pending_seq_name;
        name_put(b, src);
        map_log("LockSeq", b, src);
    }
    if (g_pending_seq_valid) {
        g_pending_seq_valid = 0;
    }
    uint32_t ret = r(a, b, c, d);
    if (note(H_LockSeqRender, ret) <= FIRSTN)
        logf("LockSequenceForRendering(sys=%08x, model=%08x, mesh=%08x, %08x) = %08x src=%s\n",
             a, b, c, d, ret, src ? src : "");
    return ret;
}

uint32_t __stdcall _GrannyGetRenderingStatesLeft(uint32_t a, uint32_t b, uint32_t c)
{
    static fn3 r = 0; RESOLVE(r, "_GrannyGetRenderingStatesLeft@12");
    uint32_t ret = r(a, b, c);
    if (note(H_StatesLeft, ret) <= FIRSTN)
        logf("GetRenderingStatesLeft(sys=%08x, %08x, %08x) = %u\n", a, b, c, ret);
    summary_maybe();
    return ret;
}

/* ---- mesh + texture dumpers --------------------------------------------- */
/*
 * The rendering-state OUT buffer (decoded via DIAG2-4) is:
 *   +10 u32 meshid
 *   +20 ptr -> 4x4 transform matrix (16 float)
 *   +30 array of 24-byte stream descriptors {dtype,ncomp,count,0,dataPtr,flag}
 *        dtype 6 = float32 vertex attribute (positions/normals/uv/color),
 *        dtype 3 = uint16 indices (count = triangle count, ncomp = 3),
 *        table ends when dtype is neither.
 * We write one self-delimiting binary file per unique descriptor pointer; the Debian side
 * classifies streams by content (range = positions, unit = normals, 0..1 = uv,
 * all-1 = color) and emits glTF. Stream ORDER is also preserved as a hint.
 */
#define DUMP_DIR   "C:\\grn_dump"
#define MAX_MESHES 512

static uint32_t g_dumped[MAX_MESHES];
static int      g_dumped_n = 0;
static uint32_t g_tex_dumped[MAX_MESHES];
static int      g_tex_dumped_n = 0;

static int already_in(uint32_t *ids, int *nids, uint32_t id)
{
    int i;
    for (i = 0; i < *nids; ++i) if (ids[i] == id) return 1;
    if (*nids < MAX_MESHES) { ids[(*nids)++] = id; return 0; }
    return 1;
}

static int seen_in(uint32_t *ids, int nids, uint32_t id)
{
    int i;
    for (i = 0; i < nids; ++i) if (ids[i] == id) return 1;
    return 0;
}

static void mark_in(uint32_t *ids, int *nids, uint32_t id)
{
    if (*nids < MAX_MESHES) ids[(*nids)++] = id;
}

static int already_dumped_mesh(uint32_t id)
{
    return already_in(g_dumped, &g_dumped_n, id);
}

/* Dump the mesh in rendering-state buffer `c` to its own .grnmesh file.
 * Returns vertex/tri counts (0/0 if nothing written) for the text manifest. */
static void dump_mesh(uint32_t c, uint32_t descp, const char *srcname,
                      uint32_t *out_v, uint32_t *out_t)
{
    uint32_t meshid = peek(c + 0x10);
    uint32_t tptr   = peek(c + 0x20);
    char     path[80];
    char     descname[40];
    int      di = 0;
    HANDLE   h;
    uint32_t off, k, vmax = 0, tmax = 0;

    *out_v = *out_t = 0;
    /* file keyed by the descriptor pointer (stable per loaded mesh) */
    wsprintfA(path, "%s\\m%08x.grnmesh", DUMP_DIR, descp);
    h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;

    wr(h, "GRNM", 4);
    wr(h, &meshid, 4);
    wr(h, &descp, 4);

    /* mesh/material name lives at descriptor+0x10 (e.g. "null") */
    if (readable(descp + 0x10, 32)) {
        const char *s = (const char *)(uintptr_t)(descp + 0x10);
        while (di < 39 && s[di]) { descname[di] = s[di]; ++di; }
    }
    descname[di] = 0;

    /* NAME is the useful source .grn path when recovered; otherwise fall back
     * to the old descriptor string so pre-name captures remain comparable. */
    wr(h, "NAME", 4);
    {
        const char *nm = (srcname && srcname[0]) ? srcname : descname;
        uint32_t nl = (uint32_t)lstrlenA(nm);
        wr(h, &nl, 4);
        wr(h, nm, nl);
    }

    wr_text_record(h, "DESC", descname);

    /* transform matrix (zeros if unreadable) */
    wr(h, "XFRM", 4);
    if (readable(tptr, 64)) wr(h, (void *)tptr, 64);
    else { char z[64]; for (k = 0; k < 64; ++k) z[k] = 0; wr(h, z, 64); }

    /* stream descriptor table */
    off = 0x30;
    for (k = 0; k < 32; ++k) {
        uint32_t dtype = peek(c + off);
        uint32_t ncomp = peek(c + off + 4);
        uint32_t count = peek(c + off + 8);
        uint32_t ptr   = peek(c + off + 16);
        uint32_t esz, nbytes;
        if (!(dtype == 6 || dtype == 3)) break;     /* table terminator */
        if (ncomp == 0 || ncomp > 4) break;
        if (count == 0 || count > 500000) break;
        esz    = (dtype == 3) ? 2u : 4u;
        nbytes = count * ncomp * esz;
        if (readable(ptr, nbytes)) {
            wr(h, "STRM", 4);
            wr(h, &dtype, 4);
            wr(h, &ncomp, 4);
            wr(h, &count, 4);
            wr(h, (void *)ptr, nbytes);
            if (dtype == 6 && count > vmax) vmax = count;
            if (dtype == 3) tmax = count;
        }
        off += 24;
    }
    wr(h, "END.", 4);
    CloseHandle(h);
    *out_v = vmax;
    *out_t = tmax;
}

/* M2d LockNextNewTexture OUT buffer, decoded from the M2c capture:
 *   +04 texture descriptor pointer (stable key, first bytes = short name)
 *   +08 source BMP path pointer
 *   +10 format/type value (4 in observed RGB24 samples)
 *   +14 width, +18 height, +1c bytes-per-pixel, +20 pitch, +24 byte size
 *   +28 pixel rows pointer
 *   +40 mesh descriptor pointer, matching GRNM.descp for association
 *   +48 inline OMedia3DMaterial name
 *   +ac inline source .grn path
 */
static void dump_texture(uint32_t outbuf)
{
    uint32_t texdesc, pathptr, fmt, w, hgt, bpp, pitch, size_field, pixelptr;
    uint32_t descp, rowsize, nbytes, version = 1;
    char texname[MAX_TEX_TEXT], bmp_path[MAX_TEX_TEXT], srcname[MAX_TEX_TEXT];
    char matname[MAX_TEX_TEXT], path[80];
    HANDLE h;

    if (!readable(outbuf, 0xb0)) return;
    texdesc    = peek(outbuf + 0x04);
    pathptr    = peek(outbuf + 0x08);
    fmt        = peek(outbuf + 0x10);
    w          = peek(outbuf + 0x14);
    hgt        = peek(outbuf + 0x18);
    bpp        = peek(outbuf + 0x1c);
    pitch      = peek(outbuf + 0x20);
    size_field = peek(outbuf + 0x24);
    pixelptr   = peek(outbuf + 0x28);
    descp      = peek(outbuf + 0x40);

    if (!plausible_handle(texdesc)) texdesc = pixelptr;
    if (!plausible_handle(texdesc)) return;
    if (seen_in(g_tex_dumped, g_tex_dumped_n, texdesc)) return;

    if (w == 0 || hgt == 0 || w > 4096 || hgt > 4096) return;
    if (!(bpp == 3 || bpp == 4)) return;
    rowsize = w * bpp;
    if (pitch < rowsize || pitch > rowsize + 16384u) return;
    if (hgt > (64u * 1024u * 1024u) / pitch) return;
    nbytes = pitch * hgt;
    if (nbytes == 0 || nbytes > 64u * 1024u * 1024u) return;
    if (!readable(pixelptr, nbytes)) return;

    texname[0] = bmp_path[0] = srcname[0] = matname[0] = 0;
    copy_cstr(texname, MAX_TEX_TEXT, texdesc);
    if (!copy_cstr(bmp_path, MAX_TEX_TEXT, pathptr))
        copy_cstr(bmp_path, MAX_TEX_TEXT, texdesc + 0x30);
    copy_cstr(matname, MAX_TEX_TEXT, outbuf + 0x48);
    copy_cstr(srcname, MAX_TEX_TEXT, outbuf + 0xac);

    wsprintfA(path, "%s\\t%08x.grntex", DUMP_DIR, texdesc);
    h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    mark_in(g_tex_dumped, &g_tex_dumped_n, texdesc);

    wr(h, "GRTX", 4);
    wr(h, &version, 4);
    wr(h, &texdesc, 4);
    wr(h, &pixelptr, 4);
    wr(h, &descp, 4);
    wr(h, &fmt, 4);
    wr(h, &w, 4);
    wr(h, &hgt, 4);
    wr(h, &bpp, 4);
    wr(h, &pitch, 4);
    wr(h, &nbytes, 4);
    wr_text_record(h, "NAME", texname);
    wr_text_record(h, "PATH", bmp_path);
    wr_text_record(h, "SRCN", srcname);
    wr_text_record(h, "MATN", matname);
    wr(h, "DATA", 4);
    wr(h, &nbytes, 4);
    wr(h, (void *)(uintptr_t)pixelptr, nbytes);
    wr(h, "END.", 4);
    CloseHandle(h);
    logf("DUMPTEX tex=%08x desc=%08x %ux%u bpp=%u pitch=%u bytes=%u name=%s src=%s path=%s mat=%s\n",
         texdesc, descp, w, hgt, bpp, pitch, nbytes,
         texname, srcname, bmp_path, matname);
}

/* ---- M3a animation capture --------------------------------------------- */
/*
 * Opt-in (GRN_ANIM_SRC). For each tracked descriptor we keep ONE open file and
 * append a sample every LockNextRenderingState call. A sample is the per-frame
 * transform + the float (dtype 6) attribute streams in their stored order, so
 * the Debian side can diff against the base mesh: streams that change over time
 * are positions/normals (the morph targets); constant ones are uv/color taken
 * from the base. Indices/topology are NOT re-written here (they live in the
 * one-shot .grnmesh base). No trailing END. record -- the parser tolerates a
 * truncated tail because the game may exit without a clean DLL detach.
 *
 *   "GRNA" version:u32 descp:u32            (written once, on the first sample)
 *   "SRCN" len:u32 bytes                    source .grn name
 *   repeat per sample:
 *     "FRAM" sample_index:u32 time_ms:u32
 *     "XFRM" 16*f32                          per-frame 4x4 transform
 *     nstreams:u32
 *     repeat: "STRM" dtype:u32 ncomp:u32 count:u32 data[count*ncomp*4]
 */
#define ANIM_MAX_DESCS        4
#define ANIM_MAX_SAMPLES      2400
#define ANIM_MAX_TOTAL_BYTES  (160u * 1024u * 1024u)

static char  g_anim_filter[64];        /* GRN_ANIM_SRC; empty => disabled */
static int   g_anim_on = 0;

typedef struct {
    uint32_t descp;
    HANDLE   h;
    uint32_t samples;
} AnimSlot;

static AnimSlot g_anim[ANIM_MAX_DESCS];
static int      g_anim_n = 0;
static uint32_t g_anim_bytes = 0;      /* approx total written, for the cap */

/* case-insensitive substring test (haystack contains needle) */
static int contains_ci(const char *hay, const char *needle)
{
    int i, j;
    if (!hay || !needle || !needle[0]) return 0;
    for (i = 0; hay[i]; ++i) {
        for (j = 0; needle[j]; ++j) {
            char a = hay[i + j], b = needle[j];
            if (a >= 'A' && a <= 'Z') a = (char)(a + 32);
            if (b >= 'A' && b <= 'Z') b = (char)(b + 32);
            if (a != b) break;
        }
        if (!needle[j]) return 1;
    }
    return 0;
}

/* find (or open) the anim file for a descriptor; NULL if disabled/over budget */
static AnimSlot *anim_slot(uint32_t descp, const char *src)
{
    int i;
    char path[80];
    HANDLE h;
    for (i = 0; i < g_anim_n; ++i)
        if (g_anim[i].descp == descp) return &g_anim[i];
    if (g_anim_n >= ANIM_MAX_DESCS) return 0;
    if (g_anim_bytes >= ANIM_MAX_TOTAL_BYTES) return 0;

    wsprintfA(path, "%s\\a%08x.grnanim", DUMP_DIR, descp);
    h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;

    {
        uint32_t version = 1;
        wr(h, "GRNA", 4);
        wr(h, &version, 4);
        wr(h, &descp, 4);
        wr_text_record(h, "SRCN", src ? src : "");
    }
    g_anim[g_anim_n].descp   = descp;
    g_anim[g_anim_n].h       = h;
    g_anim[g_anim_n].samples = 0;
    logf("ANIM open a%08x.grnanim src=%s\n", descp, src ? src : "");
    return &g_anim[g_anim_n++];
}

/* append one posed sample for descriptor `descp` (render-state buffer `c`) */
static void dump_anim_sample(uint32_t c, uint32_t descp, const char *src)
{
    AnimSlot *slot;
    uint32_t tptr, off, k, sidx, nstreams = 0;
    long     nstreams_pos;
    HANDLE   h;

    if (!g_anim_on) return;
    if (!contains_ci(src ? src : "", g_anim_filter)) return;
    if (g_anim_bytes >= ANIM_MAX_TOTAL_BYTES) return;
    slot = anim_slot(descp, src);
    if (!slot || slot->samples >= ANIM_MAX_SAMPLES) return;
    h = slot->h;

    wr(h, "FRAM", 4);
    sidx = slot->samples;
    wr(h, &sidx, 4);
    { uint32_t t = GetTickCount(); wr(h, &t, 4); }

    tptr = peek(c + 0x20);
    wr(h, "XFRM", 4);
    if (readable(tptr, 64)) wr(h, (void *)tptr, 64);
    else { char z[64]; for (k = 0; k < 64; ++k) z[k] = 0; wr(h, z, 64); }

    /* placeholder for the stream count; back-patched after the walk */
    nstreams_pos = (long)SetFilePointer(h, 0, NULL, FILE_CURRENT);
    wr(h, &nstreams, 4);

    off = 0x30;
    for (k = 0; k < 32; ++k) {
        uint32_t dtype = peek(c + off);
        uint32_t ncomp = peek(c + off + 4);
        uint32_t count = peek(c + off + 8);
        uint32_t ptr   = peek(c + off + 16);
        uint32_t nbytes;
        if (!(dtype == 6 || dtype == 3)) break;     /* table terminator */
        if (dtype == 3) { off += 24; continue; }    /* indices: topology, skip */
        if (ncomp == 0 || ncomp > 4) break;
        if (count == 0 || count > 500000) break;
        nbytes = count * ncomp * 4u;
        if (readable(ptr, nbytes)) {
            wr(h, "STRM", 4);
            wr(h, &dtype, 4);
            wr(h, &ncomp, 4);
            wr(h, &count, 4);
            wr(h, (void *)ptr, nbytes);
            g_anim_bytes += nbytes + 16u;
            ++nstreams;
        }
        off += 24;
    }

    /* back-patch nstreams, then return to append position */
    SetFilePointer(h, nstreams_pos, NULL, FILE_BEGIN);
    wr(h, &nstreams, 4);
    SetFilePointer(h, 0, NULL, FILE_END);

    ++slot->samples;
    if (slot->samples == 1 || (slot->samples % 120) == 0)
        logf("ANIM a%08x sample=%u streams=%u bytes=%u\n",
             descp, slot->samples, nstreams, g_anim_bytes);
}

uint32_t __stdcall _GrannyLockNextRenderingState(uint32_t a, uint32_t b, uint32_t c)
{
    static fn3 r = 0; RESOLVE(r, "_GrannyLockNextRenderingState@12");
    uint32_t ret = r(a, b, c);
    {
        /* M2b: dump each unique mesh once, keyed by its descriptor pointer
         * (+0x18) which is stable per loaded mesh -- unlike +0x10 which varies
         * per call and collapses distinct meshes. */
        uint32_t descp = peek(c + 0x18);
        if (descp >= 0x00100000 && descp < 0x7f000000) {
            const char *src = name_lookup(b);
            if (!src) src = name_lookup_near(b);
            if (!already_dumped_mesh(descp)) {
                uint32_t nv, nt;
                dump_mesh(c, descp, src, &nv, &nt);
                logf("DUMPED desc=%08x model=%08x meshid=%08x verts=%u tris=%u src=%s\n",
                     descp, b, peek(c + 0x10), nv, nt, src ? src : "");
            }
            /* M3a: opt-in per-frame posed sample (no-op unless GRN_ANIM_SRC matches) */
            dump_anim_sample(c, descp, src);
        }
    }
    return ret;
}

uint32_t __stdcall _GrannyUnlockRenderingState(uint32_t a)
{
    static fn1 r = 0; RESOLVE(r, "_GrannyUnlockRenderingState@4");
    note(H_UnlockState, a);
    return r(a);
}

uint32_t __stdcall _GrannyGetNewTexturesLeft(uint32_t a, uint32_t b)
{
    static fn2 r = 0; RESOLVE(r, "_GrannyGetNewTexturesLeft@8");
    uint32_t ret = r(a, b);
    if (note(H_TexLeft, ret) <= FIRSTN)
        logf("GetNewTexturesLeft(sys=%08x, %08x) = %u\n", a, b, ret);
    return ret;
}

uint32_t __stdcall _GrannyLockNextNewTexture(uint32_t a, uint32_t b)
{
    static fn2 r = 0; RESOLVE(r, "_GrannyLockNextNewTexture@8");
    uint32_t ret = r(a, b);
    dump_texture(b);
    if (note(H_LockTex, ret) <= FIRSTN) {
        logf("LockNextNewTexture(sys=%08x, out=%08x) ret=%08x  *out=%08x *(out+4)=%08x *(out+8)=%08x\n",
             a, b, ret, peek(b), peek(b + 4), peek(b + 8));
        hexdump("texout", (void *)(uintptr_t)b, 192);
        chase_dwords("texout", b, 192, 160);
        if (ret) hexdump("texret", (void *)(uintptr_t)ret, 160);
    }
    return ret;
}

/* ---- entry point --------------------------------------------------------- */

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(inst);
        InitializeCriticalSection(&g_cs);
        CreateDirectoryA(DUMP_DIR, NULL);   /* M2b geometry output (ok if exists) */
        /* M3a: animation capture is opt-in via GRN_ANIM_SRC (substring of the
         * source .grn name, e.g. "jimmy"). Empty/unset => disabled. */
        g_anim_filter[0] = 0;
        GetEnvironmentVariableA("GRN_ANIM_SRC", g_anim_filter, sizeof(g_anim_filter));
        g_anim_on = (g_anim_filter[0] != 0);
        g_orig = LoadLibraryA("granny_orig.dll");
        g_log  = CreateFileA(LOG_PATH, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        g_ready = (g_orig != NULL && g_log != INVALID_HANDLE_VALUE);
        if (g_ready) {
            char hdr[160];
            int n = wsprintfA(hdr,
                "=== JNvsJN granny capture (M3a: names + texture + anim) "
                "anim=%s filter=\"%s\" ===\n",
                g_anim_on ? "ON" : "off", g_anim_filter);
            DWORD w; WriteFile(g_log, hdr, (DWORD)n, &w, NULL);
        }
    }
    return TRUE;
}
