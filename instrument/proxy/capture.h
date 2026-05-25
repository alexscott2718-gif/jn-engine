/*
 * OMT2 instrumentation capture interface (M5 — TCP streaming)
 *
 * capture.c owns the record encoding, a lock-free SPSC ring buffer, a
 * dedicated Winsock send thread and the texture identity table.
 * com_wrappers.c calls these from its pre-forward hooks on the game's
 * render thread. The render thread only ever enqueues into the ring; it
 * never touches the socket, so game timing is unaffected by the receiver.
 *
 * Sink: the proxy connects OUT to the Debian receiver
 * (OMTC_RECEIVER_IP:OMTC_RECEIVER_PORT) and streams the binary protocol.
 * Capture is active only while that connection is up; on disconnect the
 * render hooks become no-ops and the send thread retries the connect.
 *
 * Backpressure: records are written into the ring tentatively; a frame is
 * published as a unit at FRAME_END. If a frame would overflow the ring
 * (receiver too slow), the whole frame is dropped — never a partial frame —
 * and the running count is reported in the next committed FRAME_END.dropped.
 */

#ifndef OMTC_CAPTURE_H
#define OMTC_CAPTURE_H

#include <stdint.h>

void omtc_capture_init(void);

void omtc_capture_frame_begin(void);
void omtc_capture_frame_end(void);

void omtc_set_transform(uint8_t which, const float *m);
void omtc_set_viewport(int32_t x, int32_t y, int32_t w, int32_t h,
                       float minz, float maxz);
void omtc_set_texture(uint8_t stage, void *surface);
void omtc_set_renderstate(uint32_t state, uint32_t value);
void omtc_set_texstagestate(uint8_t stage, uint32_t state, uint32_t value);
void omtc_set_light(uint8_t index, const void *d3dlight7, uint32_t blob_len);
void omtc_set_material(const void *d3dmaterial7, uint32_t blob_len);
void omtc_draw_primitive(uint8_t prim_type, uint32_t fvf, uint32_t vtx_count,
                         const void *vertices);

/* M7b: rewrite hook installed by gen_wrappers.py on SetTransform.  Called
 * from the generated thunk via the "rewrite" hook spec.  For
 * D3DTRANSFORMSTATE_WORLD (which == 1) and an active camera delta, returns
 * a pointer to a static D*M scratch matrix; otherwise returns m unchanged.
 * Single-threaded -- the scratch is shared across calls within one frame. */
void *omtc_camera_rewrite(uint32_t which, void *m);

/* --- texture identity ----------------------------------------------------
 * omtc_texture_id:    stable id for a (real) surface pointer.
 * omtc_texture_is_new: 1 the first time a surface is seen, 0 afterwards.
 *                      The first call records the surface so later calls
 *                      return 0 -- call it exactly once per SetTexture.
 *                      Surfaces first seen in a frame that is later dropped
 *                      are rolled back, so their TEXTURE_DEF re-emits in a
 *                      subsequent committed frame.
 * omtc_register_texture: SHA-1 the locked pixels and emit texture metadata.
 *                      Hashes h rows of row_bytes each, stepping pitch
 *                      between rows -- pitch padding is excluded so the
 *                      digest is layout-independent. */
uint32_t omtc_texture_id(void *surface);
int      omtc_texture_is_new(void *surface, uint32_t tex_id);
void     omtc_register_texture(uint32_t tex_id, uint16_t w, uint16_t h,
                               uint32_t d3dfmt,
                               uint32_t pf_flags, uint32_t pf_rgb_bit_count,
                               uint32_t pf_r_mask, uint32_t pf_g_mask,
                               uint32_t pf_b_mask, uint32_t pf_a_mask,
                               const void *surface_bits, int32_t pitch,
                               uint32_t row_bytes);
void     omtc_note_surface_colorkey(void *surface, uint32_t flags,
                                    uint32_t low, uint32_t high,
                                    int active, int emit_now);

#endif  /* OMTC_CAPTURE_H */
