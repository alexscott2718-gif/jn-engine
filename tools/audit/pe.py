#!/usr/bin/env python3
"""Minimal dependency-free PE reader: COFF/Optional header, Rich header,
export table, import table.  Written for Q2/Q3 of the open-question session."""
import struct, sys, os, json, datetime, hashlib


class PE(object):
    def __init__(self, path):
        self.path = path
        with open(path, "rb") as f:
            self.d = f.read()
        d = self.d
        assert d[:2] == b"MZ", "not MZ: %s" % path
        self.e_lfanew = struct.unpack_from("<I", d, 0x3C)[0]
        assert d[self.e_lfanew:self.e_lfanew + 4] == b"PE\0\0", "not PE"
        o = self.e_lfanew + 4
        (self.machine, self.nsec, self.timestamp, self.symtab, self.nsyms,
         self.opthdrsize, self.chars) = struct.unpack_from("<HHIIIHH", d, o)
        oo = o + 20
        self.magic = struct.unpack_from("<H", d, oo)[0]
        self.linker = (d[oo + 2], d[oo + 3])
        (self.sizecode, self.sizeinit, self.sizeuninit, self.entry,
         self.basecode) = struct.unpack_from("<IIIII", d, oo + 4)
        if self.magic == 0x10b:
            self.basedata = struct.unpack_from("<I", d, oo + 24)[0]
            self.imagebase = struct.unpack_from("<I", d, oo + 28)[0]
            nrva_off = oo + 92
            dd_off = oo + 96
        else:
            self.basedata = None
            self.imagebase = struct.unpack_from("<Q", d, oo + 24)[0]
            nrva_off = oo + 108
            dd_off = oo + 112
        (self.secalign, self.filealign) = struct.unpack_from("<II", d, oo + 32)
        (self.osmaj, self.osmin, self.immaj, self.immin,
         self.submaj, self.submin) = struct.unpack_from("<HHHHHH", d, oo + 40)
        self.subsystem = struct.unpack_from("<H", d, oo + (68 if self.magic == 0x10b else 68))[0]
        self.nrva = struct.unpack_from("<I", d, nrva_off)[0]
        self.dd = []
        for i in range(self.nrva):
            self.dd.append(struct.unpack_from("<II", d, dd_off + 8 * i))
        # sections
        so = o + 20 + self.opthdrsize
        self.sections = []
        for i in range(self.nsec):
            b = d[so + 40 * i: so + 40 * (i + 1)]
            name = b[:8].rstrip(b"\0").decode("latin-1")
            vsize, vaddr, rsize, raddr = struct.unpack_from("<IIII", b, 8)
            flags = struct.unpack_from("<I", b, 36)[0]
            self.sections.append({"name": name, "vsize": vsize, "vaddr": vaddr,
                                  "rsize": rsize, "raddr": raddr, "flags": flags})

    # --- helpers ---
    def rva2off(self, rva):
        for s in self.sections:
            if s["vaddr"] <= rva < s["vaddr"] + max(s["vsize"], s["rsize"]):
                return s["raddr"] + (rva - s["vaddr"])
        return None

    def cstr(self, off, maxlen=512):
        if off is None or off >= len(self.d):
            return None
        e = self.d.find(b"\0", off, off + maxlen)
        if e < 0:
            e = off + maxlen
        return self.d[off:e].decode("latin-1")

    # --- Rich header ---
    def rich(self):
        d = self.d
        stub = d[0x80:self.e_lfanew] if self.e_lfanew > 0x80 else d[0x40:self.e_lfanew]
        head = d[:self.e_lfanew]
        ri = head.rfind(b"Rich")
        if ri < 0:
            return None
        key = struct.unpack_from("<I", head, ri + 4)[0]
        # walk backwards to DanS
        i = ri - 4
        vals = []
        while i >= 0:
            v = struct.unpack_from("<I", head, i)[0] ^ key
            if v == 0x536E6144:  # 'DanS'
                break
            vals.append(v)
            i -= 4
        if i < 0:
            return None
        vals.reverse()
        # skip the three padding zeros right after DanS
        while vals and vals[0] == 0:
            vals.pop(0)
        entries = []
        for j in range(0, len(vals) - 1, 2):
            comp = vals[j]
            cnt = vals[j + 1]
            entries.append({"prodid": comp >> 16, "build": comp & 0xFFFF, "count": cnt})
        return {"key": key, "entries": entries}

    # --- exports ---
    def exports(self):
        if len(self.dd) < 1 or self.dd[0][0] == 0:
            return None
        rva, size = self.dd[0]
        off = self.rva2off(rva)
        if off is None:
            return None
        (flags, ts, mj, mn, namerva, ordbase, naddr, nnames,
         addrrva, namesrva, ordsrva) = struct.unpack_from("<IIHHIIIIIII", self.d, off)
        dllname = self.cstr(self.rva2off(namerva))
        addrs = []
        ao = self.rva2off(addrrva)
        for i in range(naddr):
            addrs.append(struct.unpack_from("<I", self.d, ao + 4 * i)[0])
        names = []
        no = self.rva2off(namesrva)
        oo = self.rva2off(ordsrva)
        for i in range(nnames):
            nr = struct.unpack_from("<I", self.d, no + 4 * i)[0]
            ordv = struct.unpack_from("<H", self.d, oo + 2 * i)[0]
            names.append((self.cstr(self.rva2off(nr)), ordv + ordbase, ordv))
        return {"dll": dllname, "timestamp": ts, "ordbase": ordbase,
                "naddr": naddr, "nnames": nnames, "addrs": addrs, "names": names,
                "dir_rva": rva, "dir_size": size}

    # --- imports ---
    def imports(self):
        if len(self.dd) < 2 or self.dd[1][0] == 0:
            return []
        rva, size = self.dd[1]
        off = self.rva2off(rva)
        out = []
        i = 0
        while True:
            ent = self.d[off + 20 * i: off + 20 * (i + 1)]
            if len(ent) < 20:
                break
            oft, ts, fc, namerva, fta = struct.unpack("<IIIII", ent)
            if oft == 0 and namerva == 0 and fta == 0:
                break
            dll = self.cstr(self.rva2off(namerva))
            thunk = oft if oft else fta
            funcs = []
            to = self.rva2off(thunk)
            j = 0
            while to is not None:
                v = struct.unpack_from("<I", self.d, to + 4 * j)[0]
                if v == 0:
                    break
                if v & 0x80000000:
                    funcs.append(("#%d" % (v & 0xFFFF), None))
                else:
                    ho = self.rva2off(v)
                    hint = struct.unpack_from("<H", self.d, ho)[0]
                    funcs.append((self.cstr(ho + 2), hint))
                j += 1
            out.append({"dll": dll, "funcs": funcs, "iat_rva": fta, "oft_rva": oft})
            i += 1
        return out


PRODID = {
    0x00: "unknown/linker-pre-VC", 0x01: "Import0", 0x02: "Linker510",
    0x03: "Cvtomf510", 0x04: "Linker600", 0x05: "Cvtomf600", 0x06: "Cvtres500",
    0x07: "Utc11_Basic", 0x08: "Utc11_C", 0x09: "Utc12_Basic", 0x0a: "Utc12_C",
    0x0b: "Utc12_CPP", 0x0c: "AliasObj60", 0x0d: "VisualBasic60", 0x0e: "Masm613",
    0x0f: "Masm710", 0x10: "Linker511", 0x11: "Cvtomf511", 0x12: "Masm614",
    0x13: "Linker512", 0x14: "Cvtomf512", 0x15: "Utc12_C_Std", 0x16: "Utc12_CPP_Std",
    0x17: "Utc12_C_Book", 0x18: "Utc12_CPP_Book", 0x19: "Implib700",
    0x1a: "Cvtomf700", 0x1b: "Utc13_Basic", 0x1c: "Utc13_C", 0x1d: "Utc13_CPP",
    0x1e: "Linker610", 0x1f: "Cvtomf610", 0x20: "Linker601", 0x21: "Cvtomf601",
    0x22: "Utc12_2_Basic", 0x23: "Utc12_2_C", 0x24: "Utc12_2_CPP",
    0x25: "Utc12_2_C_Std", 0x26: "Utc12_2_CPP_Std", 0x27: "Utc12_2_C_Book",
    0x28: "Utc12_2_CPP_Book", 0x29: "Implib622", 0x2a: "Cvtomf622",
    0x2b: "Cvtres501", 0x2c: "Utc13_C_Std", 0x2d: "Utc13_CPP_Std",
    0x2e: "Cvtpgd1300", 0x2f: "Linker620", 0x30: "Cvtomf620", 0x31: "AliasObj70",
    0x32: "Linker621", 0x33: "Cvtomf621", 0x34: "Masm615", 0x35: "Utc13_LTCG_C",
    0x36: "Utc13_LTCG_CPP", 0x37: "Masm620", 0x38: "ILAsm100",
    0x5e: "Utc1400_C", 0x5f: "Utc1400_CPP",
}


def fmt(p, label=None):
    pe = PE(p)
    print("=" * 78)
    print("%s" % (label or p))
    print("  size            : %d bytes" % len(pe.d))
    print("  md5             : %s" % hashlib.md5(pe.d).hexdigest())
    print("  machine         : 0x%04x   sections: %d" % (pe.machine, pe.nsec))
    print("  TimeDateStamp   : 0x%08x  = %s UTC" %
          (pe.timestamp, datetime.datetime.utcfromtimestamp(pe.timestamp).isoformat()))
    print("  linker version  : %d.%d" % pe.linker)
    print("  OS/image/subsys : %d.%d / %d.%d / %d.%d  subsystem=%d" %
          (pe.osmaj, pe.osmin, pe.immaj, pe.immin, pe.submaj, pe.submin, pe.subsystem))
    print("  ImageBase       : 0x%08x  entry 0x%08x" % (pe.imagebase, pe.entry))
    print("  sections        : %s" % ", ".join(
        "%s(v=0x%x r=0x%x)" % (s["name"], s["vsize"], s["rsize"]) for s in pe.sections))
    r = pe.rich()
    if r:
        print("  Rich header     : key=0x%08x, %d entries" % (r["key"], len(r["entries"])))
        for e in r["entries"]:
            print("      prodid=0x%02x %-18s build=%-6d count=%d" %
                  (e["prodid"], PRODID.get(e["prodid"], "?"), e["build"], e["count"]))
    else:
        print("  Rich header     : (none)")
    ex = pe.exports()
    if ex:
        print("  exports         : dll=%s  names=%d  addrs=%d  ordbase=%d  ts=0x%08x" %
              (ex["dll"], ex["nnames"], ex["naddr"], ex["ordbase"], ex["timestamp"]))
    imps = pe.imports()
    print("  imports         : %d DLLs" % len(imps))
    for im in imps:
        print("      %-16s %d funcs" % (im["dll"], len(im["funcs"])))
    return pe


if __name__ == "__main__":
    for a in sys.argv[1:]:
        fmt(a)
