# Asset Resolution

The engine currently ships its redistributable reconstruction assets in the
repository's `assets/` directory. That location is provisional: it makes a
fresh clone immediately testable, but it is not a permanent decision about
hosting, packaging, or extracting assets.

## Runtime resolution

Set `JN_ASSET_ROOT` to relocate the complete tree. Every logical path beginning
with `assets/` is resolved below that root by the native asset loaders. Level
metadata uses the same layer:

| Purpose | Default below `JN_ASSET_ROOT` | Compatibility override |
|---|---|---|
| serialized levels and task files | `gam/` | `JN_GAM_ROOT` |
| placement manifests and OMT models | `glb/omt/` | `JN_PLACEMENTS_ROOT`, then legacy `JN_PLB_ROOT` |
| native overrides and camera descriptors | `native/` | `JN_NATIVE_ROOT` |

The specific compatibility variables take precedence over `JN_ASSET_ROOT`.
They remain supported so existing capture and comparison commands do not need
an immediate migration. Absolute paths passed through explicit debugging
variables remain absolute.

For example:

```bash
cp -a assets /tmp/jn-assets
JN_ASSET_ROOT=/tmp/jn-assets make check-assets
```

Repository tools that consume the GAM corpus or accepted capture fixture use
the same environment contract.

## Fetch interface

`scripts/fetch_assets.sh` is the stable preparation entry point. Its shipped
`repo` backend validates the checked-in tree and can copy it to another root:

```bash
./scripts/fetch_assets.sh
./scripts/fetch_assets.sh --backend repo --destination /tmp/jn-assets
```

Two backend names are reserved but intentionally return an error today:

- `http`: download a versioned, integrity-checked redistributable bundle.
- `extract-from-disc`: derive assets from a user-supplied legitimate game disc.

Neither hook silently falls back to repository data. Implementing one requires
a manifest with hashes, an explicit license/provenance review, and the same
`make check-assets` result as the repository backend.

## When the default should move

Move assets out of Git only when at least one of these becomes true:

- repository size or clone time measurably obstructs contributors;
- redistribution rights require separating generated and source-owned data;
- release packaging needs independently versioned asset bundles; or
- a reproducible disc extractor can provide the full tested tree.

Until then, `backend=repo` preserves the no-secrets, no-private-machine
north-star while the indirection keeps a later move reversible.
