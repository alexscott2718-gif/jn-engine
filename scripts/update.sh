#!/usr/bin/env bash
# Clone the repository if it isn't here yet, or bring an existing checkout up to
# date, then re-stage everything that depends on the tree and report what changed.
#
#   ./scripts/update.sh              from inside a checkout
#   bash update.sh --clone <dir>     from the contributor bundle, with no checkout yet
#
# Safe to run repeatedly. It never discards local work: if the tree is dirty it
# says so and stops before touching anything.
set -euo pipefail

REPO_URL="${JN_REPO_URL:-https://github.com/alexscott2718-gif/jn-engine}"
BRANCH="${JN_BRANCH:-master}"

say()  { printf '\n\033[1m%s\033[0m\n' "$*"; }
note() { printf '  %s\n' "$*"; }
warn() { printf '  ! %s\n' "$*" >&2; }

# ---------------------------------------------------------------- clone mode
if [ "${1:-}" = "--clone" ]; then
    target="${2:-jn-engine}"
    if [ -e "$target" ]; then
        warn "$target already exists - run update.sh from inside it instead"
        exit 1
    fi
    say "Cloning $REPO_URL -> $target"
    git clone --branch "$BRANCH" "$REPO_URL" "$target"
    cd "$target"
else
    cd "$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
fi

command -v git >/dev/null 2>&1 || { warn "git is required"; exit 1; }
git rev-parse --git-dir >/dev/null 2>&1 || { warn "not a git checkout: $PWD"; exit 1; }

# ---------------------------------------------------------------- update
before=$(git rev-parse HEAD)

if [ "${1:-}" != "--clone" ]; then
    if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
        say "Local changes present - not pulling"
        git status --short --untracked-files=no | sed 's/^/  /'
        note ""
        note "Commit, stash, or discard these, then run again."
        exit 1
    fi
    say "Updating from $REPO_URL"
    git fetch --quiet origin "$BRANCH"
    upstream="origin/$BRANCH"
    current=$(git rev-parse --abbrev-ref HEAD)
    behind=$(git rev-list --count "HEAD..$upstream")
    ahead=$(git rev-list --count "$upstream..HEAD")

    if [ "$behind" = "0" ] && [ "$ahead" = "0" ]; then
        note "already up to date at $(git rev-parse --short HEAD)"
    elif [ "$ahead" != "0" ]; then
        # A feature branch, or local commits not yet pushed. Never rewrite either.
        note "on branch '$current', $ahead commit(s) ahead of $upstream${behind:+, $behind behind}"
        if [ "$behind" != "0" ]; then
            note "$behind new upstream commit(s) you don't have:"
            git log --oneline --no-decorate "HEAD..$upstream" | head -10 | sed 's/^/    /'
            note ""
            note "This branch has its own commits, so nothing was merged. When you're ready:"
            note "    git rebase $upstream        # or: git merge $upstream"
        fi
    else
        note "$behind new commit(s):"
        git log --oneline --no-decorate "HEAD..$upstream" | head -20 | sed 's/^/    /'
        if git merge --ff-only "$upstream" >/dev/null 2>&1; then
            note "now at $(git rev-parse --short HEAD)"
        else
            warn "could not fast-forward - resolve by hand:"
            warn "    git merge $upstream"
        fi
    fi
fi

after=$(git rev-parse HEAD)

# ---------------------------------------------------------------- re-stage
changed() { [ "$before" = "$after" ] && return 1; git diff --name-only "$before" "$after" | grep -q "^$1" ; }

if [ "$before" != "$after" ] && changed "scripts/bootstrap.sh"; then
    say "bootstrap.sh changed - you may need to re-run it"
    note "./scripts/bootstrap.sh"
fi

say "Assets"
if [ -d assets/gam ] && [ -d assets/omt ]; then
    note "repo assets present ($(ls assets/gam/*.gam 2>/dev/null | wc -l) .gam, $(ls assets/omt/*.omt 2>/dev/null | wc -l) .omt)"
else
    warn "assets/gam or assets/omt missing - is this a full checkout?"
fi

say "Original executables (optional - see docs/GAME_FILES.md)"
if [ -d assets/exe ] && [ -n "$(find assets/exe -name '*.exe' -o -name '*.dll' 2>/dev/null | head -1)" ]; then
    python3 tools/extract_game_exes.py --verify-only 2>&1 | sed -n '/verification/,$p' | sed 's/^/  /'
else
    note "not present - most tasks do not need them"
    note "to add them from a disc you own:"
    note "    python3 tools/extract_game_exes.py --source <install dir | disc | .iso>"
fi

say "Spec check"
if python3 tools/audit/spec_check.py >/tmp/jn-spec-check.$$ 2>&1; then
    tail -1 /tmp/jn-spec-check.$$ | sed 's/^/  /'
else
    warn "spec check reports findings not in the baseline:"
    sed 's/^/  /' /tmp/jn-spec-check.$$ | tail -20
fi
rm -f /tmp/jn-spec-check.$$

say "Next"
if [ "$before" != "$after" ]; then
    note "run 'make' to rebuild, or 'make check' for the full gate"
elif [ "${behind:-0}" != "0" ]; then
    note "your branch is behind $upstream - merge or rebase when ready"
else
    note "checkout is current; run 'make check' for the full gate"
fi
note "open work: docs/audit/TASKS.md"
note "onboarding: docs/ONBOARDING.md"
