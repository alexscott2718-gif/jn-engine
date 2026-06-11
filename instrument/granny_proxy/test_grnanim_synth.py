#!/usr/bin/env python3
"""Local end-to-end validation of the M3a animation export path WITHOUT the game.

Mirrors how the v3 ddraw pipeline was validated by injection: we synthesize a
.grnanim (the exact binary the M3a proxy emits) from a real captured .grnmesh by
waving the position/normal streams over N frames, then bake it to .glb and assert
the result is a valid morph-target animation.
"""
import math
import struct
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import pygltflib as gl
from grnmesh_to_obj import read_records, floats, classify
from grnanim_to_glb import convert, read_grnanim


def synth_grnanim(grnmesh: Path, out: Path, nframes=90, amp=4.0):
    """Write a .grnanim that waves the position (and normal) streams of a base
    mesh, matching the proxy's on-disk format byte-for-byte."""
    meshid, descp, name, desc, xfrm, streams = read_records(grnmesh.read_bytes())

    # float streams in stored order; classify to find pos/nrm ordinals
    fstreams, ordinal = [], 0
    fmap = []  # (full_idx, ordinal, dtype, ncomp, count, rows)
    for full_idx, (dtype, ncomp, count, blob) in enumerate(streams):
        if dtype == 6:
            rows = floats(blob, ncomp)
            fstreams.append((ordinal, ncomp, rows))
            fmap.append((ordinal, ncomp, count, rows))
            ordinal += 1
    roles = classify(fstreams)
    pos_ord = roles["pos"][0]
    nrm_ord = roles["nrm"][0] if roles.get("nrm") else None

    xf = xfrm if xfrm else [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]

    buf = bytearray()
    buf += b"GRNA" + struct.pack("<2I", 1, descp)
    src = name or "grn\\jimmybase.grn"
    buf += b"SRCN" + struct.pack("<I", len(src)) + src.encode("latin1")

    for f in range(nframes):
        phase = 2 * math.pi * f / nframes
        buf += b"FRAM" + struct.pack("<2I", f, f * 33)  # ~30fps timestamps
        buf += b"XFRM" + struct.pack("<16f", *xf)
        buf += struct.pack("<I", len(fmap))  # nstreams (all float streams)
        for o, ncomp, count, rows in fmap:
            buf += b"STRM" + struct.pack("<3I", 6, ncomp, count)
            flat = []
            for vi, r in enumerate(rows):
                comps = list(r[:ncomp])
                if o == pos_ord:
                    # wave along Y by a per-vertex-phased sine -> visible deform
                    comps[1] += amp * math.sin(phase + vi * 0.05)
                elif o == nrm_ord:
                    comps[0] += 0.1 * math.sin(phase)  # nudge normals too
                flat.extend(float(c) for c in comps)
            buf += struct.pack(f"<{len(flat)}f", *flat)
    out.write_bytes(buf)
    return descp, nframes, pos_ord, nrm_ord


def main():
    caps = sorted(Path.home().glob(
        "jnvsjn-runtime/grn_capture_m2d_run1/m01310230.grnmesh"))
    if not caps:
        caps = sorted(Path.home().glob(
            "jnvsjn-runtime/grn_capture_m2d_run1/*.grnmesh"))
    if not caps:
        print("FAIL: no .grnmesh captures found to drive the test"); return 1
    base = caps[0]
    print(f"base mesh: {base.name}")

    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        anim = td / "synth.grnanim"
        descp, nframes, pos_ord, nrm_ord = synth_grnanim(base, anim, nframes=90)
        print(f"synth .grnanim: descp={descp:08x} frames={nframes} "
              f"pos_ord={pos_ord} nrm_ord={nrm_ord} size={anim.stat().st_size}")

        # round-trip the parser
        ver, ad, asrc, samples = read_grnanim(anim.read_bytes())
        assert ad == descp, "descp mismatch on round-trip"
        assert len(samples) == nframes, f"parsed {len(samples)} != {nframes}"
        print(f"parser round-trip: version={ver} src={asrc!r} samples={len(samples)}")

        out = td / "out.anim.glb"
        res = convert(base, anim, out, max_frames=60, fps=30)
        print(f"baked: V={res['verts']} T={res['tris']} frames={res['frames']} "
              f"span={res['span']:.3f} nrm={res['normal_morph']} "
              f"dur={res['duration_s']:.2f}s size={out.stat().st_size}")

        # ---- assertions on the produced glb ----
        g = gl.GLTF2().load_binary(str(out))
        prim = g.meshes[0].primitives[0]
        assert prim.targets, "no morph targets emitted"
        assert len(prim.targets) == res["frames"], "morph target count != frames"

        def tgt_pos(t):  # pygltflib reloads targets as plain dicts
            return t.get("POSITION") if isinstance(t, dict) else t.POSITION
        assert all(tgt_pos(t) is not None for t in prim.targets), "target missing POSITION"
        assert g.animations, "no animation emitted"
        ch = g.animations[0].channels[0]
        assert ch.target.path == "weights", "channel does not target weights"
        smp = g.animations[0].samplers[ch.sampler]
        in_acc = g.accessors[smp.input]
        out_acc = g.accessors[smp.output]
        nf = res["frames"]
        assert in_acc.count == nf, f"time keys {in_acc.count} != {nf}"
        assert out_acc.count == nf * nf, f"weight outputs {out_acc.count} != {nf*nf}"
        assert g.meshes[0].weights and len(g.meshes[0].weights) == nf
        assert res["span"] > 0.5, "deformation span too small -- morph not captured"
        # extensions
        assert "KHR_materials_unlit" in (g.extensionsUsed or [])

        print("\nPASS: valid glTF morph-target animation")
        print(f"  morph targets : {len(prim.targets)}")
        print(f"  anim channels : {len(g.animations[0].channels)} (weights)")
        print(f"  time keys     : {in_acc.count}  span {in_acc.min}..{in_acc.max}")
        print(f"  weight outputs: {out_acc.count} (= {nf} x {nf})")
        print(f"  deform span   : {res['span']:.3f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
