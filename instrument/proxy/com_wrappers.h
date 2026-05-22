/*
 * com_wrappers.h -- shared interface between ddraw_proxy.c and com_wrappers.c
 * (omtc instrumentation proxy, milestone M2).
 */
#ifndef OMTC_COM_WRAPPERS_H
#define OMTC_COM_WRAPPERS_H

/* Timestamped line to C:\omtc.log. Defined in ddraw_proxy.c. */
void omtc_log(const char *msg);

/* --- com_wrappers.c ----------------------------------------------------- */

/* Logs the IDirect3DDevice7::DrawPrimitive vtable offset (must be 0x64).
 * Call once before the first wrap. */
void omtc_com_selfcheck(void);

/* Wrap a freshly-created real IDirectDraw7 so the game drives our vtable.
 * Returns a wrapper iff `iid` is IID_IDirectDraw7; otherwise returns
 * `real_dd` untouched (any other interface is passed through unwrapped). */
void *omtc_wrap_dd7_byiid(const void *iid, void *real_dd);

#endif
