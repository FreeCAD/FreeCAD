# `tools/devstack` (developer helper tooling)

`tools/devstack` is a repo-local developer helper for day-to-day work:

- Build helpers (CMake presets, reduced builds, ccache/distcc).
- Git worktree helpers (create/add/sync worktrees with a per-worktree `.devstack/`).
- Optional stacked-PR workflow helpers (maintain a PR chain while rebasing).

The entrypoint is:

- `./tools/devstack/devstack.sh` (wrapper that runs `tools/devstack/devstack.py`)

## Quick Start

- Sanity-check your environment:

```bash
./tools/devstack/devstack.sh doctor
```

- Provision missing tooling (prints install hints; can preinstall Python lint tools into the cached venv):

```bash
./tools/devstack/devstack.sh provision
./tools/devstack/devstack.sh provision --python-lint
```

- Python lint tooling is installed into:
  - `${DEVSTACK_CACHE_HOME:-${XDG_CACHE_HOME:-$HOME/.cache}/devstack}/python-lint/venv`
  - Example (standalone, without devstack): `python3 tools/lint/provision.py --venv-dir "${DEVSTACK_CACHE_HOME:-${XDG_CACHE_HOME:-$HOME/.cache}/devstack}/python-lint/venv"`

- Provision pinned clang tools via `cpp-linter/clang-tools-pip` (downloads static binaries like `clang-format-<ver>` / `clang-tidy-<ver>` and writes a PATH block to `~/.config/devstack/env.sh`):

```bash
./tools/devstack/devstack.sh provision --clang-tools
```

- Build (uses CMake presets):

```bash
./tools/devstack/devstack.sh build --preset debug
```

- Create a new worktree for a feature branch and bootstrap `.devstack/`:

```bash
./tools/devstack/devstack.sh wt-init my-feature --dir ../FreeCAD-wt-my-feature
```

## How It’s Organized

- `tools/devstack/devstack.py`: thin wrapper entrypoint (keeps sys.path stable for script execution)
- `tools/devstack/cli.py`: CLI parser/dispatch
- `tools/devstack/commands/`: command implementations (`cmd_*` functions, grouped by domain)
- `tools/devstack/core/`: repo-agnostic helpers (git/env/cmake preset helpers, config parsing)
- `tools/devstack/adapters/`: repo adapters (e.g. `freecad.py`)
- `.devstack/`: local, per-worktree artifacts (not committed)

## Common Workflows


### Shell alias (global)

Install a global `devstack` function (and optional `ds` shortcut) in your shell:

```bash
./tools/devstack/devstack.sh shell-alias
```

What it does:

- Writes `~/.config/devstack/alias.sh` (bash/zsh) or `~/.config/devstack/alias.fish` (fish).
- When present, also sources `~/.config/devstack/env.sh` (bash/zsh) or `~/.config/devstack/env.fish` (fish), so provisioned tools on PATH apply to `ds`.
- The alias runs `tools/devstack/devstack.py` by absolute path, so it works even in worktrees that do not have `tools/devstack/` copied in.


### Cache location

Devstack-managed tooling caches live under:

- `DEVSTACK_CACHE_HOME` when set
- otherwise `XDG_CACHE_HOME/devstack` (or `~/.cache/devstack`)

This is used for things like:

- Python lint venv: `<cache>/python-lint/venv/`
- Provisioned clang tools: `<cache>/clang-tools/<major>/bin/`
- clang-tools helper venv: `<cache>/clang-tools/venv/`
- Updates your shell rc file by adding/replacing a managed block:
  - bash: `~/.bashrc`
  - zsh: `~/.zshrc`
  - fish: `~/.config/fish/config.fish`

Disable rc editing (print instructions only):

```bash
./tools/devstack/devstack.sh shell-alias --no-rc
```

Uninstall:

- Remove the managed `# >>> devstack >>>` block from your rc file.
- Delete `~/.config/devstack/alias.sh` / `~/.config/devstack/alias.fish`.


### Worktrees

Create a new worktree for a new branch:

```bash
./tools/devstack/devstack.sh wt-init my-feature --dir ../FreeCAD-wt-gui-refactor
```

Add a worktree for an existing branch:

```bash
./tools/devstack/devstack.sh wt-add my-existing-branch --dir ../FreeCAD-wt-shell
```

Create a worktree at a stack layer (good for “edit layer N and then merge back”):

```bash
./tools/devstack/devstack.sh wt-layer 2
```

Bring those commits back onto your stack branch (dry-run first):

```bash
./tools/devstack/devstack.sh layer-import 2
./tools/devstack/devstack.sh layer-import 2 --apply
```

Sync the latest script/config into all existing worktrees:

```bash
./tools/devstack/devstack.sh wt-sync --symlink
```

### Building

Build the current worktree using CMake presets:

```bash
./tools/devstack/devstack.sh build --preset debug
```

Reduced “core” build (FreeCAD adapter):

```bash
./tools/devstack/devstack.sh build --preset debug --core
```

Prefer clang+mold when available:

```bash
./tools/devstack/devstack.sh build --preset debug --toolchain auto
```

Clean and rebuild:

```bash
./tools/devstack/devstack.sh build --preset debug --clean
```

### Lint

Run repo lint checks (wraps the existing `tools/lint/*.py` scripts) against files changed vs `origin/main`:

```bash
./tools/devstack/devstack.sh lint
```

If you see `unknown --base ref: ...`, pass an explicit base (or fetch the remote):

```bash
./tools/devstack/devstack.sh lint --base upstream/main
./tools/devstack/devstack.sh fix --base upstream/main
```

Auto-fix common issues (formats Python with Black; formats C/C++ with clang-format) on the same default file set:

```bash
./tools/devstack/devstack.sh fix
```

Fix a specific stack layer:

```bash
./tools/devstack/devstack.sh fix --layer 1
```

Optionally apply codespell fixes too (writes to files):

```bash
./tools/devstack/devstack.sh fix --codespell
```

Note: by default, `devstack lint` runs the `tools/lint/` scripts from the same repo as `devstack.py` (origin), so your worktrees automatically share the same lint tooling. Use `--tools-from local` to use the current repo's `tools/lint/` instead.

Lint defaults (codespell ignore/skip, clazy check sets, etc.) live under `tools/lint/` (see `tools/lint/defaults.py`).

Local vs CI output:

- Local runs default to compact summaries.
- In GitHub Actions, `--mode auto` selects CI output automatically.
- Override manually: `./tools/devstack/devstack.sh lint --mode local|ci`
- More/less output: `--verbose` / `--quiet`
- Color: `--color auto|always|never` (default: auto; disabled in CI)
- `clang-tidy` line filtering: when `--clang-tidy` is enabled, devstack auto-generates a `--line-filter` from `git diff` (disable with `--no-clang-tidy-auto-line-filter`)
- `clang-tidy` runs in auto mode by default (skips if `clang-tidy` or `compile_commands.json` is missing). Disable explicitly with `--no-clang-tidy`.
- If your `build/` contains multiple preset subdirs (e.g. `build/debug`, `build/core-debug`), `ds lint` auto-detects a subdir with `compile_commands.json` when `--clang-tidy-build-dir build` or `--clang-build-dir build` is used (prefers non-legacy dirs, then newest `compile_commands.json`); pass an explicit `--clang-tidy-build-dir <dir>` / `--clang-build-dir <dir>` to override.


Lint files in a specific stack layer (uses `.devstack/stack.conf`):
Requires `.devstack/stack.conf` in the current worktree (run `devstack init` first if needed).


```bash
./tools/devstack/devstack.sh lint --layer 1
```

Lint all tracked files:

```bash
./tools/devstack/devstack.sh lint --all
```

Use local tooling explicitly:

```bash
./tools/devstack/devstack.sh lint --tools-from local
```

Run clang-tidy (opt-in; requires a configured build dir with `compile_commands.json`):

```bash
./tools/devstack/devstack.sh lint --clang-tidy --clang-tidy-build-dir build
```

Other opt-in C++ linters:

- clang-format:

```bash
./tools/devstack/devstack.sh lint --clang-format --clang-style file
```

- clazy (Qt best-practices):

```bash
./tools/devstack/devstack.sh lint --clazy --clang-build-dir build
```

- clazy QT6 porting checks:

```bash
./tools/devstack/devstack.sh lint --clazy-qt6 --clang-build-dir build
```

Other checks:

- Case-collision check runs by default (skip with `--no-case-check`).
- Qt string-based `SIGNAL`/`SLOT` connection check runs by default and is advisory (skip with `--no-qt-connections`).


### Machine-local environment (`.devstack/env.sh` / `~/.config/devstack/env.sh`)

If an env file exists, `build` will source it (via `bash`) and apply those environment variables to configure/build.

Env-file precedence (first match wins):

- `--env-file <path>`
- `DEVSTACK_ENV_FILE=<path>`
- `.devstack/env.sh`
- `~/.config/devstack/env.sh`

Disable env loading:

```bash
./tools/devstack/devstack.sh build --preset debug --no-env-file

Parallelism:
- Default is auto (DEVSTACK_JOBS, distcc slots, or local CPU count).
- Override with `-j N` or set `DEVSTACK_JOBS=N`.
```

### Distributed builds (distcc + ccache)

`tools/devstack` supports a simple distcc workflow where **ccache stays local** and **cache misses are forwarded to distcc**.

Note: when using clang with distcc+ccache, clang can spam `-Wgnu-line-marker` warnings under `-Wpedantic` (because distcc often compiles preprocessed sources containing GNU-style `# <n> "<file>" ...` markers). `devstack build --distcc` automatically adds `-Wno-gnu-line-marker` at configure time to keep logs readable.

Install and enable `distccd` on a remote machine (systemd) via SSH:

```bash
tools/devstack/distcc/setup-distccd-remote.sh --host remote-machine-hostname
```

Then build with distcc enabled:

```bash
./tools/devstack/devstack.sh build --preset debug --distcc -j12
```

To use distcc without ccache (for timing/experiments), configure with the distcc launcher instead:

```bash
./tools/devstack/devstack.sh build --preset debug --distcc --distcc-mode direct -j12
```

To force an empty ccache for timing runs (still using distcc+ccache mode), point ccache at a fresh directory:

```bash
./tools/devstack/devstack.sh build --preset debug --distcc --ccache-dir /tmp/ccache-empty -j12
```

If `DISTCC_HOSTS` is set and `DEVSTACK_AUTO_DISTCC=1` (the default in the env file generated by `setup-distccd-remote.sh`), distcc is enabled automatically for `devstack build`.

Force-disable distcc (even if auto-enabled):

```bash
./tools/devstack/devstack.sh build --preset debug --no-distcc -j12
```

## Stacked PR Management (Optional)

If you like working as a sequence of smaller PRs while frequently rebasing/reordering commits, devstack can manage a PR “stack”.

Key idea:

- `.devstack/stack.conf` defines a base branch plus a list of PR layer branches and their cut-point SHAs.

Common commands:

- Refresh local PR branches to match `.devstack/stack.conf`:
  - `./tools/devstack/devstack.sh update`
- Record current cut-point SHAs into `.devstack/stack.conf`:
  - `./tools/devstack/devstack.sh capture`
- Rebase your stack branch and keep PR refs in sync:
  - `./tools/devstack/devstack.sh rebase`

### PR titles (draftable)

By default, `gh-sync` uses the subject of the layer’s cut-point commit as the PR title (and injects `[NNN]` if the key starts with digits like `001-...`).

New PR body files created by `body-refresh` include a frontmatter `title:` by default; edit it any time.

To override the title, add a YAML frontmatter block at the top of the PR body file:

```md
---
title: "Gui: [001] My title"
---

...rest of body...
```

Publish just a single layer (serial workflow, wait for merge):

```bash
./tools/devstack/devstack.sh pr-layer 1 --apply
```

## Adapter Selection

Adapters provide repo-specific behavior (e.g. FreeCAD build modes/presets). Adapters are discovered automatically by scanning `tools/devstack/adapters/*.py`.

- Default: `--adapter auto`
- Force: `--adapter freecad` or `DEVSTACK_ADAPTER=freecad`

## Notes

- `.devstack/` is intentionally local/per-worktree. Remove it at any time to “reset” local devstack state.
