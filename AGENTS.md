# FreeCAD worktree instructions (Codex)

## Build workflow (use `tools/devstack`)

Prefer the repo-local devstack wrapper for configuring/building FreeCAD (jobs default to auto):

- Sanity check devstack itself: `./tools/devstack/devstack.sh self-check --tests`
- Environment summary: `./tools/devstack/devstack.sh doctor`
- Configure+build: `./tools/devstack/devstack.sh build --preset debug`
- Build only: `./tools/devstack/devstack.sh build --preset debug --build-only`
- Configure only: `./tools/devstack/devstack.sh build --preset debug --configure-only`
- Reduced build (GUI + Material + Part only): `./tools/devstack/devstack.sh build --preset debug --core`

Parallelism:
- Default is auto (DEVSTACK_JOBS, distcc slots, or local CPU count).
- Override with `-j N` or set `DEVSTACK_JOBS=N`.

## Lint and fix (use `tools/devstack`)

- Provision Python lint tools into the cached venv: `./tools/devstack/devstack.sh provision --python-lint`
- Lint changed files: `./tools/devstack/devstack.sh lint`
- Auto-fix formatting: `./tools/devstack/devstack.sh fix`
- Target a specific stack layer: `./tools/devstack/devstack.sh lint --layer 1` / `./tools/devstack/devstack.sh fix --layer 1`
- Lint everything: `./tools/devstack/devstack.sh lint --all`

Notes:
- clang-based tools (clang-tidy/clazy) require a configured build with `compile_commands.json` (enable via CMake preset or `CMAKE_EXPORT_COMPILE_COMMANDS=ON`).
- Devstack-managed caches live under `DEVSTACK_CACHE_HOME` (default: `XDG_CACHE_HOME/devstack` or `~/.cache/devstack`).

## Worktrees (devstack)

- Bootstrap `.devstack/` in an existing worktree: `./tools/devstack/devstack.sh init --symlink`
- Create a new worktree and bootstrap its `.devstack/`: `./tools/devstack/devstack.sh wt-init <name> --branch <branch>`
- Add a worktree for an existing branch: `./tools/devstack/devstack.sh wt-add <branch>`
- Sync scripts/config across worktrees: `./tools/devstack/devstack.sh wt-sync --symlink`
- Create a worktree at a stack layer: `./tools/devstack/devstack.sh wt-layer 1`
- Bring commits back onto your stack branch: `./tools/devstack/devstack.sh layer-import 1` (dry-run) / `./tools/devstack/devstack.sh layer-import 1 --apply`

## Stacked PR workflow (devstack)

The tool lives under `tools/devstack/`, and per-worktree stack state lives under `.devstack/`:
- Stack config: `.devstack/stack.conf`
- Key commands:
  - `./tools/devstack/devstack.sh update`: regenerate `pr/...` branches from `.devstack/stack.conf` (applies ignore rules; does not rewrite history).
  - `./tools/devstack/devstack.sh rebase`: interactive rebase your stack branch (`git rebase -i --update-refs <base>`) and refresh stack state.
  - `./tools/devstack/devstack.sh capture`: write the current cut-point SHAs back into `.devstack/stack.conf` (use after manual rebases).
  - `./tools/devstack/devstack.sh push`: push `pr/...` branches to the configured remote (force-with-lease).
  - `./tools/devstack/devstack.sh gh-sync`: create/edit GitHub PRs (dry-run by default; pass `--apply` to perform changes).
  - `./tools/devstack/devstack.sh body-refresh`: refresh PR body templates (updates AUTOGEN blocks; safe to re-run).
  - `./tools/devstack/devstack.sh body-context`: print churn/commit context for a layer (useful for drafting PR text).
- Independent commits can be excluded from PR branches: `./tools/devstack/devstack.sh ignore <sha>` / `./tools/devstack/devstack.sh extract <sha> --to topic/<name>`

Notes:
- Commands that are dry-run by default and require `--apply`: `gh-sync`, `body-prune`, `layer-import` (and `pr-layer` when used).
- To create new PRs as drafts, pass `--draft` to `gh-sync` / `pr-layer` (or set `DEVSTACK_GH_DRAFT=1`).
- Destructive-ish flags:
  - `wt-layer --force` deletes the target layer branch (refuses if checked out in a worktree).
  - `push` updates remote `pr/...` branches with `--force-with-lease`.
- `.devstack/stack.conf` is local-only. Override config selection with `DEVSTACK_STACK_CONF=/path/to/stack.conf`.
- GitHub PR sync requires `gh` to be authenticated and a default repo set (`gh repo set-default`), or set `DEVSTACK_GH_REPO=OWNER/REPO`.
