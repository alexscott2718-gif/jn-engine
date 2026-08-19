# Game files you supply yourself

**Short version:** clone the repo and you already have everything you need to build
and run. You only need this page if you want to work on the parts of the tooling that
read the *original executables*.

---

## What ships, and what does not

| | In the repo? | Why |
|---|---|---|
| `assets/gam/` — 35 shipped level property bags | **yes** | project-processed level data |
| `assets/omt/` — canvas containers | **yes** | |
| `assets/ase/`, `glb/`, `native/`, `hud/`, `parsed/` — derived catalogs | **yes** | this project's own output |
| `docs/decomp/` — 208 class specs | **yes** | our analysis, not their code |
| **`assets/exe/`** — `Neutron.exe`, `NeutronSW.exe`, `OMT2.dll` | **no** | not ours to distribute |

`assets/exe/` is gitignored on purpose. Those three files are THQ / Nickelodeon
property. We do not host them, mirror them, or pass them around the Discord — please
don't either. Everything in this repository is either our own work or data derived
from a disc each contributor owns.

## Do I actually need them?

Most work does not. You need `assets/exe/` only for:

- `tools/audit/spec_check.py --binary …` (binary-backed confirmation; the default
  run needs no binary)
- `tools/audit/pe.py` — PE headers, imports, exports, the Rich header
- anything that disassembles a registrar or checks a vtable address
- reproducing the Q1–Q3 findings in `docs/audit/`

Building the engine, running the test suite, working on any of the 208 specs from the
generated data, and the whole native/web port need none of it.

## Getting them

You need a copy of the game you own. Then:

```bash
python3 tools/extract_game_exes.py --source <where your copy is>
```

On Windows you can drag a folder or a `.iso` onto `tools\extract-game-files.cmd`.

Three source forms are accepted, best first:

**1. An installed game directory — recommended, zero dependencies.**
Install from your disc, then point at the install:

```bash
python3 tools/extract_game_exes.py \
    --source "/mnt/c/Program Files/THQ/Jimmy Neutron/Program Executable Files"
```

**2. A mounted disc.** The retail discs are InstallShield, so the payload is inside
`data1.cab` and `unshield` is required (`apt install unshield`, `brew install unshield`).

```bash
python3 tools/extract_game_exes.py --source /media/cdrom
```

**3. A `.iso` image.** The script walks ISO 9660 itself — no mounting, no
dependencies — pulls `data1.cab` out, and hands it to `unshield`. Without `unshield`
installed it will tell you so and stop.

```bash
python3 tools/extract_game_exes.py --source ~/discs/jn.iso
```

Or through the asset pipeline, which does the same thing and then stages the repo's
data assets alongside:

```bash
JN_DISC_SOURCE=/media/cdrom ./scripts/fetch_assets.sh --backend extract-from-disc
```

### Other titles

`--title jnvsjn` and `--title mechanix` work the same way for the sequel and for
*Hot Wheels: Mechanix*, which runs on the same engine.

## Checking you got the right build

Every copy is verified against `docs/binaries.sha256`:

```
$ python3 tools/extract_game_exes.py --verify-only
  Neutron.exe      OK
  NeutronSW.exe    OK
  OMT2.dll         OK
```

The manifest holds checksums, not content. If yours mismatch you probably have a
different regional release or a patched copy — the tooling will still mostly work, but
**every offset quoted in `docs/decomp/` and `docs/audit/` is relative to the build in
the manifest**, so a mismatch means addresses may not line up. Say so when you file
anything address-specific.

For reference, the manifest build is the retail PC release whose binaries were linked
**2001-09-30 01:07:40 UTC** (`Neutron.exe`) — see `docs/audit/06-open-questions.md`
§Q3 for how that was established.

## If you don't have a disc

You can still do most of the open work. `docs/audit/TASKS.md` marks which tasks need
binaries; the majority do not.
