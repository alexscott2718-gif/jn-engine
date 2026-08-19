# Contributor onboarding

A clean-room reimplementation of the **Open Media Toolkit 2.x** engine behind
*Jimmy Neutron: Boy Genius*, built against a Direct3D 7 command stream captured off
period hardware. The same engine also runs the sequel and *Hot Wheels: Mechanix*.

If you read the [audit report](https://exentt.com/jn/audit.html) and want to pick
something up, jump to `docs/audit/TASKS.md`. This page gets you from zero to a working
checkout.

---

## 1. Credit where it's due

**awefan4524** is the project's main contributor and triager, and maintains the
*Hot Wheels: Mechanix* and *Operation Krabby Patty* efforts. The 3DSP parsers
(v0–v5, endianness auto-detect, extra-pass-set support), the bidirectional Canvas RLE,
and the independent decoder that settled the 32-bit canvas byte order and the 8-bit
palette layout are theirs — that last one caught this project having both wrong. Their
parsers are vendored in `tools/contrib_awefan/` with credit. Improvements that
generalise should go upstream to their repos, not just here. See
`docs/CONTRIBUTING_AWEFAN.md`.

## 2. Get a checkout that builds

```bash
git clone https://github.com/alexscott2718-gif/jn-engine
cd jn-engine
./scripts/bootstrap.sh          # apt packages: SDL2, GL, zlib, xvfb
make                            # native build
make check                      # the full gate — see §4
```

The repository already contains the level data (`assets/gam`, `assets/omt`) and every
derived catalog. **You do not need the game to build, run, or work on most tasks.**

### Staying current

```bash
./scripts/update.sh
```

Pulls, then re-checks everything that depends on the tree: assets present, your
`assets/exe/` copies still matching their checksums, and the spec gate. It refuses to
touch a dirty tree, and on a feature branch it tells you what's upstream rather than
merging behind your back.

From the contributor bundle, with no checkout yet:

```bash
bash update.sh --clone jn-engine
```

## 3. If you want the original binaries

Only needed for binary-backed tooling. You supply them from a disc you own — they are
not ours to distribute and `assets/exe/` is gitignored.

```bash
python3 tools/extract_game_exes.py --source <install dir | disc | .iso>
```

Windows: drag a folder or `.iso` onto `tools\extract-game-files.cmd`. Copies are
checksum-verified against `docs/binaries.sha256`. Full detail: `docs/GAME_FILES.md`.

## 4. How the gates work

The audit's finding was that this project verifies *code* well and *prose* not at all.
Both halves now have a gate.

**Linkage certificates** — the un-fakeable one. A class aspect reaches `linked` only
when a headless oracle proves the ported behaviour matches the recovered decompiled
body. No green oracle, no `linked`. `linked-blocked` is a first-class outcome and
means "we tried honestly and cannot certify yet" — it is not a failure. A review pass
once mutation-tested every certified oracle and all eight went red under a native-side
perturbation, which is what makes them worth trusting. Current state: 31 class-aspects,
15 linked, 16 linked-blocked.

**Determinism and goldens** — byte-identical output across runs; regenerate
deliberately, never to make a diff go away.

**The faithfulness sweep** — `tools/audit_faithfulness.py` turns every past QA root
cause into a permanent assertion across all 35 levels.

**The spec check** — new, and the audit's own P1 item. `tools/audit/spec_check.py`
compares all 208 class specs against three independent generated sources and fails on
any disagreement not in `docs/audit/spec_check_baseline.json`:

```bash
python3 tools/audit/spec_check.py          # green unless you introduced something new
python3 tools/audit/spec_check.py --all    # everything, baseline included
```

The baseline exists so the check could land green against 58 pre-existing findings and
ratchet from there. **Fixing a spec means removing its entry from the baseline** — that
is how a fix gets locked in. `--update-baseline` re-records the accepted set; use it
only for a deliberate change you can justify in the PR.

## 5. The evidence discipline

This is the part that matters most, and it is not bureaucracy — every rule below was
paid for by a specific bug.

- **Tag every claim.** `CONFIRMED` (observed in a file or binary — cite path and line,
  or a byte offset), `REPORTED` (a log or a person said so — name them), `INFERRED`
  (give the reasoning *and* what would falsify it), `ASSUMED`, `CONTRADICTED`.
  **Never promote a tag** because an earlier session sounded sure.
- **Decompiler output is a hypothesis; the raw disassembly is the evidence.** A
  committed evidence document once assigned the wrong expressions to three outputs and
  silently dropped a term, because the decompiler missed a stack argument. Anyone
  implementing from it in good faith would have shipped a subtly wrong transform.
- **A number in prose must match what the code prints.** The audit's headline finding
  is a fabricated 94% statistic that reached two entry-point documents and is still
  live on `master`. If you quote a figure, quote the command that produced it.
- **"We don't know" is a deliverable.** Use the ground-truth queue. The best sessions
  in the corpus are the ones that refused to certify without evidence.

## 6. Where to start

| You want to | Start at |
|---|---|
| Fix something concrete today | `docs/audit/TASKS.md` — A-01 and A-02 are good first tasks |
| Understand the architecture | `docs/ARCHITECTURE.md` |
| Understand how it got here | `docs/PROJECT_HISTORY.md` |
| Work on asset formats | `docs/omt_3dsp_format.md`, `tools/contrib_awefan/` |
| Check a claim before trusting it | `docs/audit/06-open-questions.md` |
| Use an agent on the repo | the contributor MCP — bounded, commit-pinned, `claim_task` |

Two caveats on the docs above, both from the audit: `ARCHITECTURE.md` and
`PROJECT_HISTORY.md` still carry the retracted 94% figure and an under-qualified
`canvas_id` invariant. They are otherwise the right places to start — and fixing those
two lines is task A-01.

## 7. Legal

Non-commercial reverse-engineering and preservation. *Jimmy Neutron: Boy Genius*,
*Jimmy Neutron vs. Jimmy Negatron*, *Hot Wheels: Mechanix* and
*SpongeBob SquarePants: Operation Krabby Patty* are the property of their respective
rights holders. No original game executables are distributed here, and please don't
redistribute them through the Discord either. The Open Media Toolkit is
© Yves Schmid / GarageCube, LGPL 2.1.
