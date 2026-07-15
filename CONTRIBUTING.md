# Contributing

## North-star test

A fresh Ubuntu 24 environment with no private-machine access or secrets must be able to run:

```bash
git clone https://github.com/alexscott2718-gif/jn-engine.git
cd jn-engine
./scripts/bootstrap.sh
make check
```

and get a green exit without human intervention. Preserve that property in every change.

## Start a change

Read `AGENTS.md`, `docs/decomp/_next_session.md`, and the relevant class specification before
editing behavior. Branch from current `master`; historical branches are not work targets.

```bash
git switch master
git pull --ff-only
git switch -c <name>
```

For asset-backed work, keep the checked-in `assets/` tree or point `JN_ASSET_ROOT` at an external
copy. `docs/ASSETS.md` documents the complete resolution contract.

## Validate

```bash
make check
make check-assets   # when assets are available
```

`make check` is asset-free: it builds gameplay code with warnings as errors, proves fixed-step
determinism, and compares Docker-generated fixture goldens. `make check-assets` adds Level 1
goldens, the accepted capture/native oracle, and linkage certificates. Never regenerate or accept
goldens on a physical GPU; use the repository Docker image and `make regen-goldens`.

If your conclusion depends on a new original-game capture, exact physical-GPU output, input feel,
or audio judgment, report it as blocked for a dev-machine owner. The current ownership tiers are
in `docs/decomp/_next_session.md`.

## Submit

Open one focused pull request per phase or behavior. Include:

- the source/decomp evidence for behavior claims;
- exact validation commands and outcomes;
- state traces or PNGs for visible/motion changes; and
- an update to `docs/PROJECT_HISTORY.md` plus the live handoff when the frontier changes.

Required `core` and `assets` checks must pass. On failure, download the visual artifact: golden
failures contain both actual and expected PNG directories for direct inspection.
