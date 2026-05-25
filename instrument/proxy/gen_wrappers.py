#!/usr/bin/env python3
"""
gen_wrappers.py -- generate the DX7 COM wrapper thunks for the omtc proxy.

Parses the four DECLARE_INTERFACE_ blocks the proxy wraps from the mingw-w64
ddraw.h / d3d.h headers (the same headers com_wrappers.c #includes, so the
generated vtables are guaranteed to match the struct layout the compiler sees)
and emits com_wrappers_gen.inc:

  * one __stdcall forwarding thunk per method, and
  * one `static const <IF>Vtbl w_<IF>_vtbl` per interface.

Each thunk swaps the wrapper `This` for the real interface and forwards. Two
things are handled automatically from the parsed argument types:
  * an INPUT argument that is a single pointer to one of our wrapped
    interfaces is run through unwrap() (the game hands us wrappers; the real
    method needs the real object);
  * an OUTPUT argument (double pointer to a wrapped interface) has its result
    run through the matching wrap_*() after the call.
QueryInterface and Release forward to the hand-written generics in
com_wrappers.c; everything else is a pure forward.

Usage: gen_wrappers.py <ddraw.h> <d3d.h> <out.inc>
"""
import re
import sys

# interface -> wrap_*() helper name
WRAPPED = {
    "IDirectDraw7":        "wrap_dd7",
    "IDirect3D7":          "wrap_d3d7",
    "IDirect3DDevice7":    "wrap_device",
    "IDirectDrawSurface7": "wrap_surface",
}
# LP-typedefs that are themselves a pointer to one of the wrapped interfaces
LPTYPEDEFS = {
    "LPDIRECTDRAWSURFACE7": "IDirectDrawSurface7",
    "LPDIRECTDRAW7":        "IDirectDraw7",
    "LPDIRECT3DDEVICE7":    "IDirect3DDevice7",
    "LPDIRECT3D7":          "IDirect3D7",
}
# longest interface names first so a substring never shadows a longer match
IFACE_ORDER = ["IDirectDrawSurface7", "IDirect3DDevice7",
               "IDirectDraw7", "IDirect3D7"]

# (interface, method) -> hook spec. Backward-compat string = void(void) hook called
# pre-forward. Dict spec keys (all optional):
#   "hook": name of a side-effect hook called pre-forward with "args" indices.
#   "args": list of arg indices to pass to the hook (default: none).
#   "post_hook": name of a side-effect hook called after forward as
#              fn(This, hr, selected args...). Used for HRESULT methods whose
#              output/state is only trustworthy after the real call succeeds.
#   "post_args": list of arg indices passed after This and hr.
#   "rewrite": list of (arg_index, fn_name, [hook_arg_indices]) tuples; each
#              emits aN = (TYPE)fn(args...); after r = REAL(...) but BEFORE
#              the side-effect hook + forward call -- so both capture and the
#              real driver see the rewritten value.  Used by M7b to left-
#              multiply SetTransform(WORLD) by a commanded camera delta.
# M3: frame markers, draw counts, device-acquisition path (void hooks).
# M4: argument-capturing hooks for SetTransform, SetTexture, DrawPrimitive, etc.
# M7b: SetTransform "rewrite" entry for live camera nudging.
# Hook functions are defined in com_wrappers.c ahead of the #include of this generated file.
HOOKS = {
    ("IDirect3DDevice7", "BeginScene"):           "omtc_hook_frame_begin",
    ("IDirect3DDevice7", "EndScene"):             "omtc_hook_frame_end",
    ("IDirect3DDevice7", "DrawPrimitive"):        {
        "hook": "omtc_capture_draw_primitive",
        "args": [0, 1, 2, 3]  # prim_type, fvf, vtx_count, vertices
    },
    ("IDirect3DDevice7", "DrawIndexedPrimitive"): "omtc_count_draw_dip",
    ("IDirect3DDevice7", "SetTransform"):         {
        "hook": "omtc_capture_set_transform",
        "args": [0, 1],  # xformType, lpD3DMatrix
        # M7b: rewrite a1 (the matrix) in place to D*M when a camera delta is
        # active.  See capture.c omtc_camera_rewrite for the convention.
        "rewrite": [(1, "omtc_camera_rewrite", [0, 1])],
    },
    ("IDirect3DDevice7", "SetTexture"):           {
        "hook": "omtc_capture_set_texture",
        "args": [0, 1]  # dwStage, lpTexture
    },
    ("IDirect3DDevice7", "SetRenderState"):       {
        "hook": "omtc_capture_set_renderstate",
        "args": [0, 1]  # dwRenderStateType, dwRenderState
    },
    ("IDirect3DDevice7", "SetTextureStageState"): {
        "hook": "omtc_capture_set_texstagestate",
        "args": [0, 1, 2]  # dwStage, dwState, dwValue
    },
    ("IDirect3DDevice7", "SetViewport"):          {
        "hook": "omtc_capture_set_viewport",
        "args": [0]  # lpViewport (D3DVIEWPORT7*)
    },
    ("IDirect3DDevice7", "SetLight"):             {
        "hook": "omtc_capture_set_light",
        "args": [0, 1]  # dwLightIndex, lpLight
    },
    ("IDirect3DDevice7", "SetMaterial"):          {
        "hook": "omtc_capture_set_material",
        "args": [0]  # lpMaterial
    },
    ("IDirect3D7",       "CreateDevice"):         "omtc_note_createdevice",
    ("IDirectDrawSurface7", "SetColorKey"):       {
        "post_hook": "omtc_capture_set_colorkey_result",
        "post_args": [0, 1]  # dwFlags, lpDDColorKey
    },
    ("IDirectDrawSurface7", "GetColorKey"):       {
        "post_hook": "omtc_capture_get_colorkey_result",
        "post_args": [0, 1]  # dwFlags, lpDDColorKey
    },
}


def extract_interface(text, iface):
    """Return the brace body of DECLARE_INTERFACE_(iface,...) { ... }."""
    m = re.search(r"DECLARE_INTERFACE_\(\s*" + iface +
                  r"\s*,[^)]*\)\s*\{([^}]*)\}", text)
    if not m:
        sys.exit("gen_wrappers: interface not found: " + iface)
    return m.group(1)


def parse_methods(body):
    """Yield (ret_type, name, [arg_type, ...]) for each STDMETHOD in order."""
    flat = re.sub(r"\s+", " ", body)
    for m in re.finditer(
            r"(STDMETHOD_?)\(([^)]*)\)\s*\(([^)]*)\)\s*PURE", flat):
        kind, head, params = m.group(1), m.group(2), m.group(3)
        if kind == "STDMETHOD_":
            ret, name = head.split(",", 1)
            ret, name = ret.strip(), name.strip()
        else:
            ret, name = "HRESULT", head.strip()

        params = params.strip()
        if params.startswith("THIS_"):
            params = params[len("THIS_"):]
        elif params == "THIS":
            params = ""
        args = []
        for raw in params.split(","):
            raw = raw.strip()
            if not raw:
                continue
            # the trailing identifier is the parameter name; drop it, keep type
            im = re.search(r"[A-Za-z_]\w*\s*$", raw)
            atype = raw[:im.start()].strip() if im else ""
            if not atype:                       # arg was type-only
                atype = raw.strip()
            args.append(atype)
        yield ret, name, args


def classify(atype):
    """(wrapped-interface-name or None, pointer-depth) for an argument type."""
    stars = atype.count("*")
    for lp, iface in LPTYPEDEFS.items():
        if re.search(r"\b" + lp + r"\b", atype):
            return iface, stars + 1
    for iface in IFACE_ORDER:
        if re.search(r"\b" + iface + r"\b", atype):
            return iface, stars
    return None, stars


def gen_thunk(iface, ret, name, args):
    """Emit one thunk function. Returns (code, thunk_symbol)."""
    sym = "w_%s_%s" % (iface, name)
    params = ["%s *This" % iface]
    for i, atype in enumerate(args):
        params.append("%s a%d" % (atype, i))
    sig = "static %s STDMETHODCALLTYPE %s(%s)" % (
        ret, sym, ", ".join(params))

    # QueryInterface / Release -> hand-written generics
    if name == "QueryInterface":
        body = "    return generic_QueryInterface((void*)This, a0, a1);"
        return "%s\n{\n%s\n}\n" % (sig, body), sym
    if name == "Release":
        body = "    return generic_Release((void*)This);"
        return "%s\n{\n%s\n}\n" % (sig, body), sym

    fwd_args = ["r"]
    post = []
    for i, atype in enumerate(args):
        iface_arg, depth = classify(atype)
        if iface_arg and iface_arg in WRAPPED and depth == 1:
            # input: game handed us a wrapper -> pass the real object
            fwd_args.append("(%s)unwrap((void*)a%d)" % (atype, i))
        elif iface_arg and iface_arg in WRAPPED and depth == 2:
            # output: real fills a real pointer -> wrap it for the game
            fwd_args.append("a%d" % i)
            pointee = atype.rstrip()
            pointee = pointee[:pointee.rfind("*")].strip()  # drop one '*'
            post.append(
                "    if (a%d && *a%d) *a%d = (%s)%s((void*)*a%d);" % (
                    i, i, i, pointee, WRAPPED[iface_arg], i))
        else:
            fwd_args.append("a%d" % i)

    lines = ["    %s *r = REAL(%s, This);" % (iface, iface)]
    hook = HOOKS.get((iface, name))
    if hook:
        if isinstance(hook, dict):
            # M7b: argument rewrites first (so both the capture hook and the
            # real driver below see the rewritten values).
            for entry in hook.get("rewrite", []):
                arg_idx, fn_name, rw_arg_idxs = entry
                if arg_idx >= len(args):
                    continue
                rw_args = ", ".join(
                    "a%d" % i for i in rw_arg_idxs if i < len(args))
                cast = "(%s)" % args[arg_idx]
                lines.append("    a%d = %s%s(%s);" % (
                    arg_idx, cast, fn_name, rw_args))
            # Argument-aware side-effect hook with specified arg indices
            if "hook" in hook:
                hook_name = hook["hook"]
                hook_args = []
                for arg_idx in hook.get("args", []):
                    if arg_idx < len(args):
                        hook_args.append("a%d" % arg_idx)
                lines.append("    %s(%s);" % (hook_name, ", ".join(hook_args)))
        else:
            # Backward-compat: void(void) hook
            lines.append("    %s();" % hook)
    call = "r->lpVtbl->%s(%s)" % (name, ", ".join(fwd_args))
    post_hook = hook if isinstance(hook, dict) and "post_hook" in hook else None
    if post or post_hook:
        lines.append("    %s hr = %s;" % (ret, call))
        lines.extend(post)
        if post_hook:
            hook_args = ["This", "hr"]
            for arg_idx in post_hook.get("post_args", []):
                if arg_idx < len(args):
                    hook_args.append("a%d" % arg_idx)
            lines.append("    %s(%s);" % (
                post_hook["post_hook"], ", ".join(hook_args)))
        lines.append("    return hr;")
    else:
        lines.append("    return %s;" % call)
    return "%s\n{\n%s\n}\n" % (sig, "\n".join(lines)), sym


def main():
    if len(sys.argv) != 4:
        sys.exit("usage: gen_wrappers.py <ddraw.h> <d3d.h> <out.inc>")
    ddraw = open(sys.argv[1]).read()
    d3d = open(sys.argv[2]).read()
    src = {"IDirectDraw7": ddraw, "IDirectDrawSurface7": ddraw,
           "IDirect3D7": d3d, "IDirect3DDevice7": d3d}

    out = ["/* AUTOGENERATED by gen_wrappers.py -- do not edit. */\n"]
    total = 0
    for iface in ["IDirectDraw7", "IDirect3D7",
                  "IDirect3DDevice7", "IDirectDrawSurface7"]:
        methods = list(parse_methods(extract_interface(src[iface], iface)))
        out.append("/* ===== %s : %d methods ===== */"
                   % (iface, len(methods)))
        slots = []
        for ret, name, args in methods:
            code, sym = gen_thunk(iface, ret, name, args)
            out.append(code)
            slots.append(sym)
        out.append("static const %sVtbl w_%s_vtbl = {" % (iface, iface))
        out.append("    " + ",\n    ".join(slots))
        out.append("};\n")
        total += len(methods)

    open(sys.argv[3], "w").write("\n".join(out))
    print("[gen_wrappers] %d thunks across 4 interfaces -> %s"
          % (total, sys.argv[3]))


if __name__ == "__main__":
    main()
