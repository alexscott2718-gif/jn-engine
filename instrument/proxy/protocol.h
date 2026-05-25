/*
 * OMT2 instrumentation wire protocol (M4+)
 * Shared schema between proxy (C) and receiver (Python).
 * Binary, little-endian. Version mismatch rejected by receiver.
 */

#ifndef OMTC_PROTOCOL_H
#define OMTC_PROTOCOL_H

#include <stdint.h>

#define OMTC_MAGIC       0x434d544f /* "OMTC" little-endian */
#define OMTC_VERSION     4
#define OMTC_VERSION_MIN 1          /* Receiver/diff accept [MIN, VERSION].
                                     * v1 is the 621 MB m5_session.omtc;
                                     * v2 adds FRAME_MARK + commands (M7b);
                                     * v3 adds TEXTURE_PIXELS for replay
                                     * fidelity (one-shot pixel payload per
                                     * texture; pre-v3 streams omit it);
                                     * v4 adds DirectDraw pixel-format masks
                                     * and texture color-key metadata. */

#pragma pack(push, 1)

/* Stream header. */
struct omtc_header {
    uint32_t magic;       /* OMTC_MAGIC */
    uint16_t version;     /* OMTC_VERSION */
    uint32_t pid;         /* Process ID */
    uint16_t screen_w;    /* Screen width (info only) */
    uint16_t screen_h;    /* Screen height (info only) */
};

/* Record header. */
struct omtc_record_header {
    uint8_t  type;        /* omtc_record_type */
    uint8_t  len_hi;      /* Payload length (24-bit) */
    uint16_t len_lo;
};

#define OMTC_RECORD_TYPE_FRAME_BEGIN        1
#define OMTC_RECORD_TYPE_FRAME_END          2
#define OMTC_RECORD_TYPE_SET_TRANSFORM      3
#define OMTC_RECORD_TYPE_VIEWPORT           4
#define OMTC_RECORD_TYPE_SET_TEXTURE        5
#define OMTC_RECORD_TYPE_TEXTURE_DEF        6
#define OMTC_RECORD_TYPE_SET_RENDERSTATE    7
#define OMTC_RECORD_TYPE_SET_TEXSTAGESTATE  8
#define OMTC_RECORD_TYPE_SET_LIGHT          9
#define OMTC_RECORD_TYPE_SET_MATERIAL      10
#define OMTC_RECORD_TYPE_DRAW_PRIMITIVE    11
#define OMTC_RECORD_TYPE_DRAW_INDEXED      12
#define OMTC_RECORD_TYPE_FRAME_MARK        13  /* v2: tagged frame from receiver */
#define OMTC_RECORD_TYPE_TEXTURE_PIXELS    14  /* v3: raw locked-surface pixel
                                                * bytes, one-shot per texture,
                                                * so the .omtc replayer can
                                                * render with exact original
                                                * textures (no PNG matching). */
#define OMTC_RECORD_TYPE_TEXTURE_FORMAT    15  /* v4: DDPIXELFORMAT masks */
#define OMTC_RECORD_TYPE_TEXTURE_COLORKEY  16  /* v4: DirectDraw color key */

/* --- M7b command records (receiver -> proxy on the same TCP socket) -------
 * Type space is disjoint from proxy->receiver records so a misdirected byte
 * is obvious.  Wire framing is identical: {type u8, len u24, payload}. */
#define OMTC_CMD_CAMERA_DELTA     0x80  /* float m[16] -- row-major D       */
#define OMTC_CMD_CAMERA_CLEAR     0x81  /* clear active camera delta        */
#define OMTC_CMD_CAPTURE_START    0x82
#define OMTC_CMD_CAPTURE_STOP     0x83
#define OMTC_CMD_MARK_FRAME       0x84  /* tag u32 -- emitted with next FB  */
#define OMTC_CMD_REDUMP_TEXTURES  0x85
#define OMTC_CMD_KILLSWITCH       0x86  /* force pass-through (no capture)  */

/* FRAME_BEGIN: seq u32, t_ms u32 */
struct omtc_frame_begin {
    uint32_t seq;         /* Frame sequence number */
    uint32_t t_ms;        /* Timestamp ms */
};

/* FRAME_END: seq u32, draw_count u32, dropped u32 */
struct omtc_frame_end {
    uint32_t seq;         /* Frame sequence number */
    uint32_t draw_count;  /* DrawPrimitive calls in this frame */
    uint32_t dropped;     /* Frames dropped due to buffer overflow */
};

/* SET_TRANSFORM: which u8, m f32[16] */
#define OMTC_TRANSFORM_WORLD  0
#define OMTC_TRANSFORM_VIEW   1
#define OMTC_TRANSFORM_PROJ   2
struct omtc_set_transform {
    uint8_t  which;       /* OMTC_TRANSFORM_* */
    float    m[16];       /* Row-major 4x4 matrix */
};

/* VIEWPORT: x,y,w,h i32, minz,maxz f32 */
struct omtc_viewport {
    int32_t  x, y, w, h;
    float    minz, maxz;
};

/* SET_TEXTURE: stage u8, tex_id u32 (0 = none) */
struct omtc_set_texture {
    uint8_t  stage;       /* Texture stage (0+) */
    uint32_t tex_id;      /* Surface pointer hash, 0=none */
};

/* TEXTURE_DEF: tex_id u32, w u16, h u16, d3dfmt u32, sha1 u8[20] */
#define OMTC_SHA1_LEN 20
struct omtc_texture_def {
    uint32_t tex_id;
    uint16_t w, h;
    uint32_t d3dfmt;
    uint8_t  sha1[OMTC_SHA1_LEN];
};

/* TEXTURE_PIXELS (v3): tex_id u32, w u16, h u16, bpp u32, pixel_bytes u32,
 * then `pixel_bytes` of raw locked-surface bytes (no pitch padding -- packed
 * w*bpp/8 bytes per row, h rows). Emitted one-shot per (tex_id, sha1) on first
 * sighting alongside TEXTURE_DEF; the replayer maps tex_id -> these pixels and
 * uploads as a GL texture. The proxy already has these bytes locked while
 * computing the SHA-1; emitting them is bandwidth-cheap (<= ~64 MB for a full
 * Level-1 texture set) and yields pixel-exact replay. */
struct omtc_texture_pixels {
    uint32_t tex_id;
    uint16_t w, h;
    uint32_t bpp;          /* bits per pixel of the captured surface */
    uint32_t pixel_bytes;  /* w * h * (bpp+7)/8 -- length of payload below */
    /* Followed by pixel_bytes of raw RGB(A)/BGR(A) data. */
};

/* TEXTURE_FORMAT (v4): tex_id u32 and DDPIXELFORMAT fields needed to decode
 * packed surface bits faithfully. Emitted before TEXTURE_PIXELS whenever the
 * proxy has a locked DDSURFACEDESC2. */
struct omtc_texture_format {
    uint32_t tex_id;
    uint32_t flags;              /* DDPIXELFORMAT.dwFlags */
    uint32_t rgb_bit_count;      /* DDPIXELFORMAT.dwRGBBitCount */
    uint32_t r_bit_mask;
    uint32_t g_bit_mask;
    uint32_t b_bit_mask;
    uint32_t alpha_bit_mask;
};

/* TEXTURE_COLORKEY (v4): DirectDraw surface color key state. `active=0`
 * means this key slot is explicitly disabled/cleared. */
struct omtc_texture_colorkey {
    uint32_t tex_id;
    uint32_t flags;              /* DDCKEY_* selector passed to DD */
    uint32_t low;
    uint32_t high;
    uint32_t active;
};

/* SET_RENDERSTATE: state u32, value u32 */
struct omtc_set_renderstate {
    uint32_t state;       /* D3DRENDERSTATETYPE */
    uint32_t value;
};

/* SET_TEXSTAGESTATE: stage u8, state u32, value u32 */
struct omtc_set_texstagestate {
    uint8_t  stage;       /* Texture stage */
    uint32_t state;       /* D3DTEXTURESTAGESTATETYPE */
    uint32_t value;
};

/* SET_LIGHT: index u8, D3DLIGHT7 blob (varies, 90+ bytes) */
/* Emitted with the full D3DLIGHT7 struct as payload. */
struct omtc_set_light {
    uint8_t index;        /* Light index */
    /* Followed by raw D3DLIGHT7 struct */
};

/* SET_MATERIAL: D3DMATERIAL7 blob (sizeof D3DMATERIAL7 bytes) */
/* Emitted with the full D3DMATERIAL7 struct as payload. */

/* DRAW_PRIMITIVE: prim_type u8, fvf u32, vtx_count u32, vertices[] */
/* For FVF 0x152 (XYZ·NORMAL·DIFFUSE·TEX1), vertex = 36 bytes:
 *   pos[3] f32, normal[3] f32, diffuse u32, uv[2] f32 */
#define OMTC_FVF_TEST (0x152)  /* Expected FVF */
struct omtc_draw_primitive {
    uint8_t  prim_type;   /* D3DPRIMITIVETYPE (PT_TRIANGLELIST=4, etc.) */
    uint32_t fvf;         /* FVF flags */
    uint32_t vtx_count;
    /* Followed by vtx_count * vertex_size bytes */
};

/* Vertex for FVF 0x152: 36 bytes */
struct omtc_vertex_fvf152 {
    float pos[3];         /* XYZ */
    float normal[3];      /* Normal */
    uint32_t diffuse;     /* Diffuse colour */
    float uv[2];          /* UV */
};

/* FRAME_MARK: seq u32, tag u32 -- emitted right after FRAME_BEGIN when the
 * receiver has issued OMTC_CMD_MARK_FRAME.  The tag is opaque to the proxy. */
struct omtc_frame_mark {
    uint32_t seq;
    uint32_t tag;
};

/* DRAW_INDEXED: (not used -- OMT2 only calls DrawPrimitive) */
struct omtc_draw_indexed {
    uint8_t  prim_type;
    uint32_t fvf;
    uint32_t vtx_count;
    uint32_t idx_count;
    /* Followed by vtx_count * vertex_size, then idx_count * u16 */
};

#pragma pack(pop)

/* --- Payload length helpers --- */
static inline uint32_t omtc_payload_len(const struct omtc_record_header *hdr) {
    return (((uint32_t)hdr->len_hi << 16) | hdr->len_lo);
}

static inline void omtc_set_payload_len(struct omtc_record_header *hdr, uint32_t len) {
    hdr->len_hi = (len >> 16) & 0xFF;
    hdr->len_lo = len & 0xFFFF;
}

#endif  /* OMTC_PROTOCOL_H */
