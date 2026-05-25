/*
 * OMT2 instrumentation capture (M5 — TCP streaming)
 *
 * Lock-free SPSC ring buffer + dedicated Winsock send thread.
 *
 *   producer = the game's render thread (com_wrappers.c hooks)
 *   consumer = omtc_send_thread() below
 *
 * The render thread writes records into the ring tentatively and publishes
 * a whole frame at FRAME_END; it never touches the socket, so receiver
 * latency cannot distort game timing. The send thread connects OUT to the
 * Debian receiver and drains the ring. If a frame would overflow the ring
 * (receiver too slow) the whole frame is dropped and counted.
 *
 * CRT-free (no malloc, no CRT): fixed-size static buffers, Win32 + Winsock
 * only. ws2_32.dll ships with Windows XP.
 */

#include <winsock2.h>
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "protocol.h"
#include "capture.h"

/* --- receiver endpoint (proxy connects OUT to the Debian box) --- */
#define OMTC_RECEIVER_IP    "<DEBIAN_HOST>"
#define OMTC_RECEIVER_PORT  7070

/* --- SPSC ring buffer ----------------------------------------------------
 * Free-running uint32 byte counters: a buffer position is counter & MASK,
 * and the counters wrap cleanly because RING_SIZE is a power of two that
 * divides 2^32. Unsigned (head - tail) is the live byte count.
 *   g_head      : render-thread private tentative write cursor
 *   g_committed : render publishes (release) / sender reads (acquire)
 *   g_tail      : sender publishes (release) / render reads (acquire) */
#define OMTC_RING_SIZE  (32u * 1024u * 1024u)
#define OMTC_RING_MASK  (OMTC_RING_SIZE - 1u)
static uint8_t  g_ring[OMTC_RING_SIZE];
static uint32_t g_head;
static uint32_t g_committed;
static uint32_t g_tail;

/* --- connection / thread state --- */
static uint32_t g_capture_active;   /* 1 while the TCP connection is up   */
static uint32_t g_conn_epoch;       /* ++ on each successful connect      */
static HANDLE   g_send_thread;
static HANDLE   g_data_event;       /* render -> sender: data available   */
static int      g_started;

/* --- per-frame render-thread state --- */
static uint32_t g_frame_seq;
static uint32_t g_frame_dropped;
static uint32_t g_frame_draw_count;
static int      g_frame_capturing;  /* frame began while a receiver was up */
static int      g_frame_overflow;   /* ring would overflow -> drop frame   */
static uint32_t g_frame_head0;      /* g_head snapshot at FRAME_BEGIN      */
static int      g_frame_tex0;       /* g_tex_count snapshot at FRAME_BEGIN */
static uint32_t g_last_epoch;

/* --- M7b: render-thread-private command state ---------------------------
 * Mutated only by omtc_cmd_drain (render thread).  Defaults preserve M5
 * behaviour: capture is enabled, no camera nudge, not killed. */
static float    g_cam_delta[16];
static int      g_cam_delta_active;
static int      g_capture_enabled = 1;
static int      g_killed;
static int      g_pending_mark;
static uint32_t g_pending_mark_tag;

/* --- M7b: SPSC command ring (send thread produces, render thread consumes)
 * The send thread reads bytes off the socket via omtc_poll_commands(), parses
 * complete records and pushes commands here.  omtc_cmd_drain() then runs on
 * the render thread at the top of frame_begin. */
struct omtc_cmd {
    uint32_t type;
    uint32_t arg;       /* tag for MARK_FRAME; otherwise unused */
    float    m[16];     /* matrix for CAMERA_DELTA; otherwise unused */
};
#define OMTC_CMD_RING_N 32u
#define OMTC_CMD_RING_MASK (OMTC_CMD_RING_N - 1u)
static struct omtc_cmd g_cmd_ring[OMTC_CMD_RING_N];
static uint32_t g_cmd_head;   /* send-thread producer (release)        */
static uint32_t g_cmd_tail;   /* render-thread consumer (release)      */

/* --- Texture identity table ---
 * Maps Direct3D surface pointers to a tex_id and remembers which have had
 * a TEXTURE_DEF emitted. Surfaces first seen in a dropped frame are rolled
 * back (g_tex_count rewinds) so the def re-emits in a later committed frame. */
#define OMTC_TEX_TABLE_SIZE 256
struct omtc_tex_entry {
    void *surface;                  /* IDirectDrawSurface7* */
    uint32_t tex_id;                /* surface pointer as id */
    uint8_t sha1[OMTC_SHA1_LEN];    /* SHA-1 of pixels (computed once) */
    int sha1_valid;
};
static struct omtc_tex_entry g_tex_table[OMTC_TEX_TABLE_SIZE];
static int g_tex_count = 0;

#define OMTC_COLORKEY_SLOTS 4
#define OMTC_SURFACE_META_SIZE 512
struct omtc_colorkey_state {
    uint32_t flags;
    uint32_t low;
    uint32_t high;
    int active;
    int known;
};
struct omtc_surface_meta {
    void *surface;
    uint32_t tex_id;
    struct omtc_colorkey_state colorkey[OMTC_COLORKEY_SLOTS];
};
static struct omtc_surface_meta g_surface_meta[OMTC_SURFACE_META_SIZE];
static int g_surface_meta_count;

/* --- SHA-1 (minimal inline implementation) ---
 * Self-contained, no external deps. Copied from public domain sources. */
typedef struct {
    uint32_t h[5];
    uint64_t bits;
    uint8_t buf[64];
    int buflen;
} SHA1_CTX;

static inline uint32_t sha1_rol(uint32_t x, int bits) {
    return (x << bits) | (x >> (32 - bits));
}

static void sha1_init(SHA1_CTX *ctx) {
    ctx->h[0] = 0x67452301;
    ctx->h[1] = 0xefcdab89;
    ctx->h[2] = 0x98badcfe;
    ctx->h[3] = 0x10325476;
    ctx->h[4] = 0xc3d2e1f0;
    ctx->bits = 0;
    ctx->buflen = 0;
}

static void sha1_block(SHA1_CTX *ctx, const uint8_t *block) {
    uint32_t w[80], a, b, c, d, e, t;
    for (int i = 0; i < 16; i++)
        w[i] = (block[i*4] << 24) | (block[i*4+1] << 16)
             | (block[i*4+2] << 8) | block[i*4+3];
    for (int i = 16; i < 80; i++)
        w[i] = sha1_rol(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    a = ctx->h[0]; b = ctx->h[1]; c = ctx->h[2]; d = ctx->h[3]; e = ctx->h[4];
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20) { f = (b & c) | (~b & d); k = 0x5a827999; }
        else if (i < 40) { f = b ^ c ^ d; k = 0x6ed9eba1; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8f1bbcdc; }
        else { f = b ^ c ^ d; k = 0xca62c1d6; }
        t = sha1_rol(a, 5) + f + e + k + w[i];
        e = d; d = c; c = sha1_rol(b, 30); b = a; a = t;
    }
    ctx->h[0] += a; ctx->h[1] += b; ctx->h[2] += c; ctx->h[3] += d; ctx->h[4] += e;
}

static void sha1_update(SHA1_CTX *ctx, const uint8_t *data, size_t len) {
    ctx->bits += len * 8;
    while (len > 0) {
        int remain = 64 - ctx->buflen;
        int copy = (len < (size_t)remain) ? len : remain;
        memcpy(ctx->buf + ctx->buflen, data, copy);
        ctx->buflen += copy;
        data += copy;
        len -= copy;
        if (ctx->buflen == 64) {
            sha1_block(ctx, ctx->buf);
            ctx->buflen = 0;
        }
    }
}

static void sha1_final(SHA1_CTX *ctx, uint8_t out[20]) {
    uint8_t final_buf[128];
    memcpy(final_buf, ctx->buf, ctx->buflen);
    final_buf[ctx->buflen++] = 0x80;
    while ((ctx->buflen % 64) != 56)
        final_buf[ctx->buflen++] = 0;
    uint64_t bits = ctx->bits;
    for (int i = 7; i >= 0; i--)
        final_buf[ctx->buflen++] = (bits >> (i*8)) & 0xff;
    for (int i = 0; i < (int)ctx->buflen; i += 64)
        sha1_block(ctx, final_buf + i);
    for (int i = 0; i < 5; i++) {
        out[i*4+0] = (ctx->h[i] >> 24) & 0xff;
        out[i*4+1] = (ctx->h[i] >> 16) & 0xff;
        out[i*4+2] = (ctx->h[i] >> 8) & 0xff;
        out[i*4+3] = ctx->h[i] & 0xff;
    }
}

/* --- logging (defined in ddraw_proxy.c) --- */
extern void omtc_log(const char *msg);

/* --- M7b: command ring ---------------------------------------------------
 * The producer is the send thread (omtc_cmd_push) and the consumer is the
 * render thread (omtc_cmd_drain at frame_begin).  Single producer + single
 * consumer + free-running uint32 counters + power-of-two ring = the same
 * SPSC discipline used for the capture ring.  On overflow the command is
 * dropped (the receiver can re-issue).  __atomic acquire/release pairs
 * order the struct stores with the head/tail publication. */
static void omtc_cmd_push(const struct omtc_cmd *c)
{
    uint32_t tail = __atomic_load_n(&g_cmd_tail, __ATOMIC_ACQUIRE);
    uint32_t head = g_cmd_head;
    if ((uint32_t)(head - tail) >= OMTC_CMD_RING_N)
        return;  /* ring full -- drop */
    g_cmd_ring[head & OMTC_CMD_RING_MASK] = *c;
    __atomic_store_n(&g_cmd_head, head + 1u, __ATOMIC_RELEASE);
}

static int omtc_cmd_pop(struct omtc_cmd *out)
{
    uint32_t head = __atomic_load_n(&g_cmd_head, __ATOMIC_ACQUIRE);
    uint32_t tail = g_cmd_tail;
    if (head == tail)
        return 0;
    *out = g_cmd_ring[tail & OMTC_CMD_RING_MASK];
    __atomic_store_n(&g_cmd_tail, tail + 1u, __ATOMIC_RELEASE);
    return 1;
}

/* Apply pending commands to the render-private state at frame_begin. */
static void omtc_cmd_drain(void)
{
    struct omtc_cmd c;
    while (omtc_cmd_pop(&c)) {
        switch (c.type) {
        case OMTC_CMD_CAMERA_DELTA:
            memcpy(g_cam_delta, c.m, sizeof(g_cam_delta));
            g_cam_delta_active = 1;
            break;
        case OMTC_CMD_CAMERA_CLEAR:
            g_cam_delta_active = 0;
            break;
        case OMTC_CMD_CAPTURE_START:
            g_capture_enabled = 1;
            break;
        case OMTC_CMD_CAPTURE_STOP:
            g_capture_enabled = 0;
            break;
        case OMTC_CMD_MARK_FRAME:
            g_pending_mark = 1;
            g_pending_mark_tag = c.arg;
            break;
        case OMTC_CMD_REDUMP_TEXTURES:
            /* Forget every surface we have seen -- every still-bound
             * texture will then re-emit its TEXTURE_DEF on next SetTexture. */
            g_tex_count = 0;
            break;
        case OMTC_CMD_KILLSWITCH:
            g_killed = 1;
            break;
        default:
            break;
        }
    }
}

/* M7b: camera-nudge rewrite hook.  Called by the generated SetTransform
 * thunk via gen_wrappers.py's "rewrite" hook spec, *after* r=REAL but before
 * the side-effect capture hook and before the forward call, so both the
 * capture stream and the real driver see the rewritten matrix.
 *
 * Active only for the WORLD transform (OMT2 bakes the camera into every
 * WORLD; D *= WORLD is the camera nudge in view space).  Writes into a
 * static scratch buffer rather than rewriting m in place: if the game ever
 * re-sends a cached WORLD without recomputing it, in-place would compound D
 * every frame.  SetTransform copies the matrix synchronously and rendering
 * is single-threaded here, so one static scratch is safe. */
void *omtc_camera_rewrite(uint32_t which, void *m)
{
    static float scratch[16];
    if (!g_cam_delta_active || !m)
        return m;
    /* D3DTRANSFORMSTATE_WORLD == 1 -- declared in d3d.h.  Hardcoded here to
     * keep capture.c header-clean (no d3d.h dependency). */
    if (which != 1)
        return m;
    const float *D = g_cam_delta;
    const float *M = (const float *)m;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++)
                s += D[i*4 + k] * M[k*4 + j];
            scratch[i*4 + j] = s;
        }
    return scratch;
}

/* --- M7b: read commands off the (bidirectional) capture socket ----------
 * Called by the send thread between socket drains.  Uses select() with a
 * zero timeout so it never blocks: if no bytes are ready we return at once.
 * Records are framed identically to proxy->receiver records, just with
 * disjoint type values (0x80+).  The receive accumulation buffer is static
 * and reset on each new connection (see omtc_cmd_buf_reset). */
static uint8_t  g_cmd_rxbuf[4096];
static int      g_cmd_rxlen;

static void omtc_cmd_buf_reset(void)
{
    g_cmd_rxlen = 0;
}

static void omtc_poll_commands(SOCKET s)
{
    fd_set rfds;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_SET(s, &rfds);
    if (select(0, &rfds, NULL, NULL, &tv) <= 0)
        return;

    int room = (int)sizeof(g_cmd_rxbuf) - g_cmd_rxlen;
    if (room <= 0) {
        /* Overflow shouldn't happen with reasonable framing -- a single
         * CAMERA_DELTA is 68 bytes including header.  If it does, drop. */
        g_cmd_rxlen = 0;
        return;
    }
    int n = recv(s, (char *)g_cmd_rxbuf + g_cmd_rxlen, room, 0);
    if (n <= 0)
        return;
    g_cmd_rxlen += n;

    /* Parse out any complete records and push them to the command ring. */
    int off = 0;
    while (off + 4 <= g_cmd_rxlen) {
        /* Record header (matches struct omtc_record_header byte layout):
         *   [0]=type  [1]=len_hi  [2..3]=len_lo (LE u16)
         *   24-bit payload length = (len_hi << 16) | len_lo. */
        uint8_t  rt   = g_cmd_rxbuf[off + 0];
        uint32_t plen = ((uint32_t)g_cmd_rxbuf[off + 1] << 16)
                      | ((uint32_t)g_cmd_rxbuf[off + 2])
                      | ((uint32_t)g_cmd_rxbuf[off + 3] << 8);
        if (off + 4 + (int)plen > g_cmd_rxlen)
            break;

        struct omtc_cmd cmd;
        memset(&cmd, 0, sizeof(cmd));
        cmd.type = rt;
        const uint8_t *p = g_cmd_rxbuf + off + 4;
        switch (rt) {
        case OMTC_CMD_CAMERA_DELTA:
            if (plen >= sizeof(cmd.m)) {
                memcpy(cmd.m, p, sizeof(cmd.m));
                omtc_cmd_push(&cmd);
            }
            break;
        case OMTC_CMD_MARK_FRAME:
            if (plen >= sizeof(uint32_t)) {
                memcpy(&cmd.arg, p, sizeof(uint32_t));
                omtc_cmd_push(&cmd);
            }
            break;
        case OMTC_CMD_CAMERA_CLEAR:
        case OMTC_CMD_CAPTURE_START:
        case OMTC_CMD_CAPTURE_STOP:
        case OMTC_CMD_REDUMP_TEXTURES:
        case OMTC_CMD_KILLSWITCH:
            omtc_cmd_push(&cmd);
            break;
        default:
            /* Unknown command -- drop silently. */
            break;
        }
        off += 4 + (int)plen;
    }
    if (off > 0) {
        if (off < g_cmd_rxlen)
            memmove(g_cmd_rxbuf, g_cmd_rxbuf + off, g_cmd_rxlen - off);
        g_cmd_rxlen -= off;
    }
}

/* --- ring buffer writes (render thread only) ------------------------------
 * Writes are tentative: g_head advances but g_committed is only published
 * at FRAME_END. Once a frame overflows, every further write for that frame
 * is skipped and the whole frame is rewound at FRAME_END. */
static void omtc_ring_put(const void *data, uint32_t len) {
    if (g_frame_overflow)
        return;
    uint32_t tail = __atomic_load_n(&g_tail, __ATOMIC_ACQUIRE);
    if ((uint32_t)(g_head - tail) + len > OMTC_RING_SIZE) {
        g_frame_overflow = 1;       /* receiver too slow -- drop this frame */
        return;
    }
    uint32_t pos = g_head & OMTC_RING_MASK;
    uint32_t first = OMTC_RING_SIZE - pos;
    if (first >= len) {
        memcpy(g_ring + pos, data, len);
    } else {
        memcpy(g_ring + pos, data, first);
        memcpy(g_ring, (const uint8_t *)data + first, len - first);
    }
    g_head += len;
}

/* Emit one record: 4-byte header + up to two payload fragments. */
static void omtc_emit2(uint8_t type, const void *p1, uint32_t l1,
                       const void *p2, uint32_t l2) {
    if (!g_frame_capturing || g_frame_overflow)
        return;
    struct omtc_record_header rh;
    rh.type = type;
    omtc_set_payload_len(&rh, l1 + l2);
    omtc_ring_put(&rh, sizeof(rh));
    if (l1) omtc_ring_put(p1, l1);
    if (l2) omtc_ring_put(p2, l2);
}

static void omtc_emit(uint8_t type, const void *payload, uint32_t len) {
    omtc_emit2(type, payload, len, NULL, 0);
}

/* --- send thread ---------------------------------------------------------- */

static int omtc_send_all(SOCKET s, const void *buf, int len) {
    const char *p = (const char *)buf;
    while (len > 0) {
        int n = send(s, p, len, 0);
        if (n <= 0)
            return -1;
        p += n;
        len -= n;
    }
    return 0;
}

static DWORD WINAPI omtc_send_thread(LPVOID arg) {
    (void)arg;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        omtc_log("[capture] WSAStartup failed -- streaming disabled");
        return 1;
    }

    for (;;) {
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) {
            Sleep(1000);
            continue;
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(OMTC_RECEIVER_PORT);
        addr.sin_addr.s_addr = inet_addr(OMTC_RECEIVER_IP);

        if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
            closesocket(s);
            Sleep(1000);
            continue;
        }

        /* Connected: send the stream header before going active. */
        struct omtc_header hdr;
        hdr.magic    = OMTC_MAGIC;
        hdr.version  = OMTC_VERSION;
        hdr.pid      = GetCurrentProcessId();
        hdr.screen_w = (uint16_t)GetSystemMetrics(SM_CXSCREEN);
        hdr.screen_h = (uint16_t)GetSystemMetrics(SM_CYSCREEN);
        if (omtc_send_all(s, &hdr, sizeof(hdr)) != 0) {
            closesocket(s);
            Sleep(1000);
            continue;
        }
        omtc_log("[capture] receiver connected -- streaming live");

        /* Fast-forward past any stale buffered frames (g_committed always
         * sits on a frame boundary), bump the epoch so the render thread
         * re-emits its texture table, then go active. */
        uint32_t c = __atomic_load_n(&g_committed, __ATOMIC_ACQUIRE);
        __atomic_store_n(&g_tail, c, __ATOMIC_RELEASE);
        __atomic_add_fetch(&g_conn_epoch, 1u, __ATOMIC_RELEASE);
        __atomic_store_n(&g_capture_active, 1u, __ATOMIC_RELEASE);
        omtc_cmd_buf_reset();  /* M7b: fresh command-recv accumulation */

        int alive = 1;
        while (alive) {
            WaitForSingleObject(g_data_event, 200);
            /* Snapshot the commit point ONCE per wake.  Without this, a render
             * thread producing faster than we can send keeps g_committed ahead
             * of g_tail, the inner loop's `avail` never reaches 0, we spin here
             * forever, and omtc_poll_commands() is STARVED -- so receiver
             * commands (stop/start/mark/cam/...) never apply under render load.
             * That was the M7b "commands ignored in gameplay" bug.  Bytes
             * committed after this snapshot ship on the next wake (the auto-
             * reset g_data_event is re-signalled on every commit). */
            uint32_t com = __atomic_load_n(&g_committed, __ATOMIC_ACQUIRE);
            for (;;) {
                /* Poll every pass -- including mid-drain under saturation --
                 * so command latency stays sub-second even when the ring is
                 * continuously full.  select() uses a 0 timeout (non-blocking);
                 * the cost is a handful of cheap syscalls per drain pass. */
                if (alive)
                    omtc_poll_commands(s);
                uint32_t tl  = __atomic_load_n(&g_tail, __ATOMIC_RELAXED);
                uint32_t avail = com - tl;
                if (avail == 0)
                    break;
                uint32_t pos = tl & OMTC_RING_MASK;
                uint32_t chunk = OMTC_RING_SIZE - pos;
                if (chunk > avail)
                    chunk = avail;
                int n = send(s, (const char *)(g_ring + pos), (int)chunk, 0);
                if (n <= 0) {
                    alive = 0;
                    break;
                }
                __atomic_store_n(&g_tail, tl + (uint32_t)n, __ATOMIC_RELEASE);
            }
        }

        __atomic_store_n(&g_capture_active, 0u, __ATOMIC_RELEASE);
        closesocket(s);
        omtc_log("[capture] receiver disconnected -- retrying");
        Sleep(1000);
    }
    /* not reached */
}

/* --- initialization --- */
void omtc_capture_init(void) {
    if (g_started)
        return;
    g_started = 1;
    memset(g_tex_table, 0, sizeof(g_tex_table));
    g_tex_count = 0;
    g_data_event = CreateEventA(NULL, FALSE, FALSE, NULL);  /* auto-reset */
    g_send_thread = CreateThread(NULL, 0, omtc_send_thread, NULL, 0, NULL);
    if (!g_send_thread)
        omtc_log("[capture] CreateThread failed -- streaming disabled");
    else
        omtc_log("[capture] M5 streaming initialized");
}

/* --- frame brackets --- */

void omtc_capture_frame_begin(void) {
    if (!g_started)
        omtc_capture_init();

    /* M7b: apply pending commands first so CAPTURE_START honoured even while
     * capture is currently disabled, and so MARK/REDUMP take effect this
     * very frame. */
    omtc_cmd_drain();

    int active = __atomic_load_n(&g_capture_active, __ATOMIC_ACQUIRE) ? 1 : 0;
    g_frame_capturing = (active && g_capture_enabled && !g_killed) ? 1 : 0;
    g_frame_overflow  = 0;
    g_frame_draw_count = 0;
    if (!g_frame_capturing)
        return;

    /* Fresh receiver connection: restart the sequence and re-emit textures. */
    uint32_t epoch = __atomic_load_n(&g_conn_epoch, __ATOMIC_ACQUIRE);
    if (epoch != g_last_epoch) {
        g_last_epoch    = epoch;
        g_frame_seq     = 0;
        g_frame_dropped = 0;
        g_tex_count     = 0;
        g_head = __atomic_load_n(&g_committed, __ATOMIC_RELAXED);
    }
    g_frame_head0 = g_head;
    g_frame_tex0  = g_tex_count;

    struct omtc_frame_begin fb;
    fb.seq  = g_frame_seq;
    fb.t_ms = GetTickCount();
    omtc_emit(OMTC_RECORD_TYPE_FRAME_BEGIN, &fb, sizeof(fb));

    /* M7b: receiver-driven mark, emitted right after FRAME_BEGIN. */
    if (g_pending_mark) {
        struct omtc_frame_mark fm;
        fm.seq = g_frame_seq;
        fm.tag = g_pending_mark_tag;
        omtc_emit(OMTC_RECORD_TYPE_FRAME_MARK, &fm, sizeof(fm));
        g_pending_mark = 0;
    }
}

void omtc_capture_frame_end(void) {
    if (!g_frame_capturing)
        return;

    struct omtc_frame_end fe;
    fe.seq        = g_frame_seq;
    fe.draw_count = g_frame_draw_count;
    fe.dropped    = g_frame_dropped;
    omtc_emit(OMTC_RECORD_TYPE_FRAME_END, &fe, sizeof(fe));

    if (g_frame_overflow) {
        /* Drop the whole frame: rewind the ring and un-see this frame's
         * new textures so their TEXTURE_DEF re-emits in a later frame. */
        g_head      = g_frame_head0;
        g_tex_count = g_frame_tex0;
        g_frame_dropped++;
    } else {
        __atomic_store_n(&g_committed, g_head, __ATOMIC_RELEASE);
        SetEvent(g_data_event);
    }
    g_frame_seq++;
    g_frame_capturing = 0;
}

/* --- record emission --- */

void omtc_set_transform(uint8_t which, const float *m) {
    struct omtc_set_transform st;
    st.which = which;
    memcpy(st.m, m, sizeof(st.m));
    omtc_emit(OMTC_RECORD_TYPE_SET_TRANSFORM, &st, sizeof(st));
}

void omtc_set_viewport(int32_t x, int32_t y, int32_t w, int32_t h,
                       float minz, float maxz) {
    struct omtc_viewport vp;
    vp.x = x; vp.y = y; vp.w = w; vp.h = h;
    vp.minz = minz; vp.maxz = maxz;
    omtc_emit(OMTC_RECORD_TYPE_VIEWPORT, &vp, sizeof(vp));
}

void omtc_set_texture(uint8_t stage, void *surface) {
    struct omtc_set_texture st;
    st.stage  = stage;
    st.tex_id = surface ? ((uint32_t)(uintptr_t)surface) : 0;
    omtc_emit(OMTC_RECORD_TYPE_SET_TEXTURE, &st, sizeof(st));
}

void omtc_set_renderstate(uint32_t state, uint32_t value) {
    struct omtc_set_renderstate sr;
    sr.state = state;
    sr.value = value;
    omtc_emit(OMTC_RECORD_TYPE_SET_RENDERSTATE, &sr, sizeof(sr));
}

void omtc_set_texstagestate(uint8_t stage, uint32_t state, uint32_t value) {
    struct omtc_set_texstagestate ts;
    ts.stage = stage;
    ts.state = state;
    ts.value = value;
    omtc_emit(OMTC_RECORD_TYPE_SET_TEXSTAGESTATE, &ts, sizeof(ts));
}

static struct omtc_surface_meta *omtc_surface_meta_for(void *surface,
                                                       uint32_t tex_id,
                                                       int create) {
    if (!surface)
        return 0;
    for (int i = 0; i < g_surface_meta_count; i++)
        if (g_surface_meta[i].surface == surface)
            return &g_surface_meta[i];
    if (!create || g_surface_meta_count >= OMTC_SURFACE_META_SIZE)
        return 0;
    struct omtc_surface_meta *m = &g_surface_meta[g_surface_meta_count++];
    memset(m, 0, sizeof(*m));
    m->surface = surface;
    m->tex_id = tex_id;
    return m;
}

static void omtc_emit_texture_colorkey(uint32_t tex_id,
                                       const struct omtc_colorkey_state *ck) {
    if (!ck || !ck->known)
        return;
    struct omtc_texture_colorkey rec;
    rec.tex_id = tex_id;
    rec.flags = ck->flags;
    rec.low = ck->low;
    rec.high = ck->high;
    rec.active = ck->active ? 1u : 0u;
    omtc_emit(OMTC_RECORD_TYPE_TEXTURE_COLORKEY, &rec, sizeof(rec));
}

void omtc_note_surface_colorkey(void *surface, uint32_t flags,
                                uint32_t low, uint32_t high,
                                int active, int emit_now) {
    uint32_t tex_id = omtc_texture_id(surface);
    struct omtc_surface_meta *m = omtc_surface_meta_for(surface, tex_id, 1);
    if (!m)
        return;
    int slot = -1;
    for (int i = 0; i < OMTC_COLORKEY_SLOTS; i++)
        if (m->colorkey[i].known && m->colorkey[i].flags == flags) {
            slot = i;
            break;
        }
    if (slot < 0) {
        for (int i = 0; i < OMTC_COLORKEY_SLOTS; i++)
            if (!m->colorkey[i].known) {
                slot = i;
                break;
            }
    }
    if (slot < 0)
        return;

    struct omtc_colorkey_state *ck = &m->colorkey[slot];
    ck->flags = flags;
    ck->low = low;
    ck->high = high;
    ck->active = active ? 1 : 0;
    ck->known = 1;
    if (emit_now)
        omtc_emit_texture_colorkey(tex_id, ck);
}

/* SET_LIGHT: index u8, then the raw D3DLIGHT7 struct (104 bytes on DX7). */
void omtc_set_light(uint8_t index, const void *d3dlight7, uint32_t blob_len) {
    if (!g_frame_capturing || g_frame_overflow)
        return;
    uint8_t buf[160];
    if (blob_len > sizeof(buf) - 1)
        blob_len = sizeof(buf) - 1;
    buf[0] = index;
    memcpy(buf + 1, d3dlight7, blob_len);
    omtc_emit(OMTC_RECORD_TYPE_SET_LIGHT, buf, 1 + blob_len);
}

/* SET_MATERIAL: the raw D3DMATERIAL7 struct (68 bytes on DX7). */
void omtc_set_material(const void *d3dmaterial7, uint32_t blob_len) {
    omtc_emit(OMTC_RECORD_TYPE_SET_MATERIAL, d3dmaterial7, blob_len);
}

void omtc_draw_primitive(uint8_t prim_type, uint32_t fvf, uint32_t vtx_count,
                         const void *vertices) {
    g_frame_draw_count++;
    if (!g_frame_capturing || g_frame_overflow)
        return;

    /* FVF 0x152: pos(12) + normal(12) + diffuse(4) + uv(8) = 36 bytes. */
    uint32_t vtx_data_len = vtx_count * 36u;

    struct omtc_draw_primitive dp;
    dp.prim_type = prim_type;
    dp.fvf       = fvf;
    dp.vtx_count = vtx_count;
    omtc_emit2(OMTC_RECORD_TYPE_DRAW_PRIMITIVE, &dp, sizeof(dp),
               vertices, vtx_data_len);
}

/* --- texture identity ----------------------------------------------------
 * tex_id is the surface pointer itself (32-bit target). The table records
 * which surfaces have been seen so TEXTURE_DEF is emitted exactly once per
 * receiver session. */

uint32_t omtc_texture_id(void *surface) {
    return surface ? (uint32_t)(uintptr_t)surface : 0;
}

int omtc_texture_is_new(void *surface, uint32_t tex_id) {
    if (!surface || !g_frame_capturing)
        return 0;
    for (int i = 0; i < g_tex_count; i++)
        if (g_tex_table[i].surface == surface)
            return 0;
    if (g_tex_count >= OMTC_TEX_TABLE_SIZE)
        return 0;  /* table full -- treat as seen, skip TEXTURE_DEF */
    struct omtc_tex_entry *e = &g_tex_table[g_tex_count++];
    e->surface = surface;
    e->tex_id = tex_id;
    e->sha1_valid = 0;
    return 1;
}

void omtc_register_texture(uint32_t tex_id, uint16_t w, uint16_t h,
                           uint32_t d3dfmt,
                           uint32_t pf_flags, uint32_t pf_rgb_bit_count,
                           uint32_t pf_r_mask, uint32_t pf_g_mask,
                           uint32_t pf_b_mask, uint32_t pf_a_mask,
                           const void *surface_bits, int32_t pitch,
                           uint32_t row_bytes) {
    uint8_t sha1[OMTC_SHA1_LEN];
    SHA1_CTX ctx;
    sha1_init(&ctx);
    if (surface_bits && row_bytes) {
        const uint8_t *row = (const uint8_t *)surface_bits;
        for (uint16_t y = 0; y < h; y++) {
            sha1_update(&ctx, row, row_bytes);
            row += pitch;
        }
    }
    sha1_final(&ctx, sha1);

    for (int i = 0; i < g_tex_count; i++)
        if (g_tex_table[i].tex_id == tex_id) {
            memcpy(g_tex_table[i].sha1, sha1, OMTC_SHA1_LEN);
            g_tex_table[i].sha1_valid = 1;
            break;
        }

    struct omtc_texture_def td;
    td.tex_id = tex_id;
    td.w = w;
    td.h = h;
    td.d3dfmt = d3dfmt;
    memcpy(td.sha1, sha1, OMTC_SHA1_LEN);
    omtc_emit(OMTC_RECORD_TYPE_TEXTURE_DEF, &td, sizeof(td));

    /* v4: carry the DDPIXELFORMAT masks needed to interpret raw pixels. */
    struct omtc_texture_format tf;
    tf.tex_id = tex_id;
    tf.flags = pf_flags;
    tf.rgb_bit_count = pf_rgb_bit_count;
    tf.r_bit_mask = pf_r_mask;
    tf.g_bit_mask = pf_g_mask;
    tf.b_bit_mask = pf_b_mask;
    tf.alpha_bit_mask = pf_a_mask;
    omtc_emit(OMTC_RECORD_TYPE_TEXTURE_FORMAT, &tf, sizeof(tf));

    struct omtc_surface_meta *m = 0;
    for (int i = 0; i < g_surface_meta_count; i++)
        if (g_surface_meta[i].tex_id == tex_id) {
            m = &g_surface_meta[i];
            break;
        }
    if (m) {
        for (int i = 0; i < OMTC_COLORKEY_SLOTS; i++)
            omtc_emit_texture_colorkey(tex_id, &m->colorkey[i]);
    }

    /* v3: also emit a TEXTURE_PIXELS record carrying the raw locked-surface
     * pixel bytes (packed, no pitch padding). One-shot per texture, so the
     * .omtc replayer can render with exact original pixels rather than
     * heuristically-paired PNGs. Limited to <= 1 MB per texture to keep a
     * runaway 4K-mip-chain surface from torching the capture buffer. */
    if (surface_bits && row_bytes && w > 0 && h > 0) {
        uint32_t pixel_bytes = (uint32_t)row_bytes * (uint32_t)h;
        if (pixel_bytes <= (1u << 20)) {
            /* Header + packed payload, stack-buffer-bounded to 1 MB total. */
            static uint8_t buf[(1u << 20) + sizeof(struct omtc_texture_pixels)];
            struct omtc_texture_pixels *tp = (struct omtc_texture_pixels *)buf;
            tp->tex_id = tex_id;
            tp->w = w;
            tp->h = h;
            tp->bpp = d3dfmt;          /* same source as TEXTURE_DEF */
            tp->pixel_bytes = pixel_bytes;
            uint8_t *dst = buf + sizeof(*tp);
            const uint8_t *row = (const uint8_t *)surface_bits;
            for (uint16_t y = 0; y < h; y++) {
                /* Pack each row (skip pitch padding the surface may carry). */
                for (uint32_t i = 0; i < row_bytes; i++) dst[i] = row[i];
                dst += row_bytes;
                row += pitch;
            }
            omtc_emit(OMTC_RECORD_TYPE_TEXTURE_PIXELS, buf,
                      sizeof(*tp) + pixel_bytes);
        }
    }
}
