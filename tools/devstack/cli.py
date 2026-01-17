from __future__ import annotations

import argparse
import os
import shlex
import subprocess
import sys
from collections import OrderedDict

from tools.devstack.core.proc import die
from tools.devstack.commands.bodies import cmd_body_context, cmd_body_prune, cmd_body_refresh
from tools.devstack.commands.build import cmd_build
from tools.devstack.commands.github import cmd_gh_sync, cmd_pr_layer, cmd_push
from tools.devstack.commands.lint import cmd_fix, cmd_lint
from tools.devstack.commands.setup import cmd_doctor, cmd_provision, cmd_self_check, cmd_shell_alias, cmd_test
from tools.devstack.commands.stack import (
    cmd_capture,
    cmd_extract,
    cmd_ignore,
    cmd_layer_import,
    cmd_list,
    cmd_log,
    cmd_rebase,
    cmd_update,
)
from tools.devstack.commands.worktrees import cmd_init, cmd_wt_add, cmd_wt_feature, cmd_wt_layer, cmd_wt_sync

class CategorizedArgumentParser(argparse.ArgumentParser):
    command_groups: list[tuple[str, list[tuple[str, str]]]]

    def format_help(self) -> str:
        formatter = self._get_formatter()
        formatter.add_usage(self.usage, self._actions, self._mutually_exclusive_groups)
        formatter.add_text(self.description)

        groups = getattr(self, "command_groups", [])
        if groups:
            formatter.start_section("Commands")
            width = 0
            for _, items in groups:
                for name, _ in items:
                    width = max(width, len(name))
            width = min(max(width, 12), 36)
            for cat, items in groups:
                category_lines = [f"{cat}:"]
                for name, help_text in items:
                    category_lines.append(f"  {name:<{width}}  {help_text}")
                formatter.add_text("\n".join(category_lines))


            formatter.end_section()

        formatter.start_section("Options")
        formatter.add_arguments(self._get_optional_actions())
        formatter.end_section()

        formatter.add_text(self.epilog)
        return formatter.format_help()


def build_parser() -> argparse.ArgumentParser:
    p = CategorizedArgumentParser(
        prog="devstack.py",
        add_help=True,
        formatter_class=argparse.RawTextHelpFormatter,
        description="Developer helper tooling: worktrees, builds (CMake presets, ccache/distcc), and optional stacked PR management.\n\nLocal config lives in .devstack/stack.conf (per worktree).\n\nTip: run `devstack.py <command> --help` for command-specific flags.",
    )
    p.add_argument("--debug", action="store_true", help="Show Python tracebacks for failures.")
    sub = p.add_subparsers(dest="cmd", required=True)

    groups: "OrderedDict[str, list[tuple[str, str]]]" = OrderedDict(
        [
            ("Worktrees", []),
            ("Build", []),
            ("Lint", []),
            ("Stack", []),
            ("GitHub", []),
            ("PR Bodies", []),
            ("Setup", []),
            ("Diagnostics", []),
        ]
    )

    def cmd(name: str, help_text: str, *, category: str) -> argparse.ArgumentParser:
        if category not in groups:
            groups[category] = []
        groups[category].append((name, help_text))
        return sub.add_parser(
            name,
            help=help_text,
            description=help_text,
            formatter_class=argparse.RawTextHelpFormatter,
        )

    cmd("list", "List stack entries from .devstack/stack.conf.", category="Stack")
    cmd("log", "Show git log per stack layer.", category="Stack")
    cmd(
        "update",
        "Move PR branch refs to configured SHAs; regenerate filtered PR branches when ignore rules exist.",
        category="Stack",
    )

    ps = cmd("push", "Push PR branches to the remote (supports --only N).", category="GitHub")
    ps.add_argument("--only", type=int, help="Only push layer N (1-based).")

    ghs = cmd("gh-sync", "Create/update PRs via gh (supports --only N).", category="GitHub")
    ghs.add_argument("--apply", action="store_true", help="Apply changes (default is dry-run).")
    ghs.add_argument("--only", type=int, help="Only create/edit the PR for layer N (1-based).")
    pl = cmd("pr-layer", "Update + push + gh-sync for a single PR layer.", category="GitHub")
    pl.add_argument("layer", type=int, help="Layer number (1-based).")
    pl.add_argument("--apply", action="store_true", help="Actually create/edit the PR (default is dry-run).")

    cmd("capture", "Capture current cut-point SHAs into .devstack/stack.conf.", category="Stack")
    cmd(
        "rebase",
        "Interactive rebase the stack branch (uses --update-refs) and refresh stack config/branches.",
        category="Stack",
    )

    cmd(
        "body-refresh",
        "Refresh PR body markdown files with an AUTOGEN block (commits/depends-on metadata).",
        category="PR Bodies",
    )
    bc = cmd("body-context", "Print review context (commits/churn) for a layer.", category="PR Bodies")
    bp = cmd("body-prune", "Delete unreferenced PR body files under the configured body dir (dry-run by default).", category="PR Bodies")
    bp.add_argument("--apply", action="store_true", help="Delete files (default is dry-run).")
    bc.add_argument("branch", nargs="?", help="Branch name or key (defaults to all).")

    ini = cmd("init", "Bootstrap .devstack/ (local devstack.sh/devstack.py wrappers + stack.conf) in this worktree.", category="Worktrees")
    ini.add_argument("--copy", dest="mode", action="store_const", const="copy", default="copy")
    ini.add_argument("--symlink", dest="mode", action="store_const", const="symlink")
    ini.add_argument("--force", action="store_true")
    ini.add_argument("--force-script", action="store_true")
    ini.add_argument("--force-conf", action="store_true")

    wti = cmd("wt-init", "Create a new worktree and bootstrap its .devstack/.", category="Worktrees")
    wti.add_argument("name")
    wti.add_argument("--dir")
    wti.add_argument("--base")
    wti.add_argument("--branch")
    wti.add_argument("--symlink", action="store_true")
    wti.add_argument("--build", action="store_true")
    wti.add_argument("--preset", default="debug")
    wti.add_argument("--adapter", default=os.environ.get("DEVSTACK_ADAPTER", "auto"))
    wti.add_argument("--build-mode", default="full")
    wti.add_argument("--core", action="store_true")
    wti.add_argument("--toolchain", default="auto")
    wti.add_argument("--clang-mold", action="store_true")
    wti.add_argument("--env-file")
    wti.add_argument("--no-env-file", action="store_true")
    wti.add_argument("--ccache-launcher", action="store_true")
    wti.add_argument("--distcc", action="store_true")
    wti.add_argument("--no-distcc", action="store_true")
    wti.add_argument("--distcc-hosts")
    wti.add_argument("--distcc-verbose", action="store_true")
    wti.add_argument("--jobs", "-j", type=int, default=8)
    wti.add_argument("--target")
    wti.add_argument("--clean", action="store_true")

    wta = cmd(
        "wt-add",
        "Add a worktree for an existing branch and optionally import stack config/PR bodies.",
        category="Worktrees",
    )
    wta.add_argument("branch")
    wta.add_argument("--dir")
    wta.add_argument("--no-import", action="store_true")
    wta.add_argument("--import-from")
    wta.add_argument("--symlink", action="store_true")
    wta.add_argument("--build", action="store_true")
    wta.add_argument("--preset", default="debug")
    wta.add_argument("--adapter", default=os.environ.get("DEVSTACK_ADAPTER", "auto"))
    wta.add_argument("--build-mode", default="full")
    wta.add_argument("--core", action="store_true")
    wta.add_argument("--toolchain", default="auto")
    wta.add_argument("--clang-mold", action="store_true")
    wta.add_argument("--env-file")
    wta.add_argument("--no-env-file", action="store_true")
    wta.add_argument("--ccache-launcher", action="store_true")
    wta.add_argument("--distcc", action="store_true")
    wta.add_argument("--no-distcc", action="store_true")
    wta.add_argument("--distcc-hosts")
    wta.add_argument("--distcc-verbose", action="store_true")
    wta.add_argument("--jobs", "-j", type=int, default=8)
    wta.add_argument("--target")
    wta.add_argument("--clean", action="store_true")

    wts = cmd("wt-sync", "Sync devstack scripts/config across existing worktrees.", category="Worktrees")
    wtl = cmd("wt-layer", "Create a worktree checked out at stack layer N.", category="Worktrees")
    wtl.add_argument("layer", type=int, help="Layer number (1-based) from .devstack/stack.conf")
    wtl.add_argument("--view", default="source", help="source|pr (default: source; source is safer for making changes)")
    wtl.add_argument("--dir", help="Worktree directory (default: ../<repo>-wt-layer-<key>)")
    wtl.add_argument("--branch", help="Branch name to create in the worktree (default: layer/<stack>/<key> for source view)")
    wtl.add_argument("--force", action="store_true", help="Delete the target --branch if it already exists (refuses if checked out in a worktree)")

    li = cmd("layer-import", "Cherry-pick commits from a wt-layer branch onto the current branch (dry-run by default).", category="Stack")
    li.add_argument("layer", type=int, help="Layer number (1-based) from .devstack/stack.conf")
    li.add_argument("--from", dest="from_branch", help="Source branch (default: layer/<stack>/<key>)")
    li.add_argument("--onto", help="Switch to this branch before applying (optional)")
    li.add_argument("--apply", action="store_true", help="Apply the cherry-pick (default is dry-run)")
    li.add_argument("--replace", action="store_true", help="Replace the current layer range by rebasing the remainder onto the layer branch (rewrites history)")
    li.add_argument("--squash", action="store_true", help="Squash all imported commits into one commit")
    li.add_argument("--message", help="Commit message for --squash (default: 'Import layer N changes')")
    li.add_argument("--update", action="store_true", help="Run `ds update` after applying (useful in filtered mode)")
    wts.add_argument("--symlink", dest="mode", action="store_const", const="symlink", default="copy")
    wts.add_argument("--copy", dest="mode", action="store_const", const="copy")
    wts.add_argument("--no-import", action="store_true")
    wts.add_argument("--import-from")
    wts.add_argument("--no-force-script", action="store_true")
    wts.add_argument("--dry-run", action="store_true")

    b = cmd("build", "Configure/build the current worktree using CMake presets.", category="Build")
    b.add_argument("--preset", default="debug")
    b.add_argument("--adapter", default=os.environ.get("DEVSTACK_ADAPTER", "auto"))
    b.add_argument("--build-dir")
    b.add_argument("--jobs", "-j", type=int, default=8)
    b.add_argument("--target")
    b.add_argument("--build-mode", default="full")
    b.add_argument("--core", action="store_true")
    b.add_argument("--toolchain", default="auto")
    b.add_argument("--clang-mold", action="store_true")
    b.add_argument("--env-file")
    b.add_argument("--no-env-file", action="store_true")
    b.add_argument("--ccache-launcher", action="store_true")
    b.add_argument("--ccache-dir", help="Override CCACHE_DIR for this build (useful for timing runs)")
    b.add_argument("--distcc", action="store_true")
    b.add_argument("--no-distcc", action="store_true")
    b.add_argument("--distcc-mode", default="ccache", help="ccache|direct (default: ccache; direct sets compiler launcher to distcc)")
    b.add_argument("--distcc-hosts")
    b.add_argument("--distcc-verbose", action="store_true")
    b.add_argument("--submodules", default="auto")
    b.add_argument("--no-submodules", action="store_true")
    b.add_argument("--configure-only", action="store_true")
    b.add_argument("--build-only", action="store_true")
    b.add_argument("--clean", action="store_true")

    l = cmd("lint", "Run repo lint checks (wrapping tools/lint scripts).", category="Lint")
    l.add_argument("--base", help="Git base ref for changed-files mode (default: adapter-defined, usually origin/main)")
    l.add_argument("--layer", type=int, help="Lint files in stack layer N (1-based; uses .devstack/stack.conf; ignored if --files is provided).")
    l.add_argument("--all", action="store_true", help="Lint all tracked files (git ls-files)")
    l.add_argument("--files", nargs="+", help="Explicit file list or glob patterns (overrides --layer/--base/--all)")
    l.add_argument("--tools-from", default="origin", help="Where to run lint tooling from: origin|local|<path> (default: origin, i.e. the repo containing this devstack.py).")
    l.add_argument("--log-dir", help="Directory for lint logs (default: .devstack/lint/logs)")
    l.add_argument("--report-file", help="Markdown report path (default: .devstack/lint/report.md)")
    l.add_argument("--mode", default="auto", help="Output mode: auto|local|ci (default: auto; ci when running in GitHub Actions)")
    l.add_argument("--verbose", action="store_true", help="Verbose checker output (sets DEVSTACK_LINT_LOG_LEVEL=DEBUG for tools/lint scripts)")
    l.add_argument("--quiet", action="store_true", help="Minimal output (only final report path)")
    l.add_argument("--color", default="auto", help="Color output: auto|always|never (default: auto; disabled in CI)")
    l.add_argument("--log-tail", type=int, default=40, help="On failure, print last N lines of checker logs (default: 40)")
    l.add_argument("--fail-fast", action="store_true", help="Stop on first failing checker")
    l.add_argument("--no-generic", action="store_true", help="Skip generic whitespace/tabs/line endings checks")
    l.add_argument("--no-case-check", action="store_true", help="Skip case-insensitive filename collision check")
    l.add_argument("--no-black", action="store_true", help="Skip Black (Python)")
    l.add_argument("--no-pylint", action="store_true", help="Skip Pylint (Python)")
    l.add_argument("--no-cpplint", action="store_true", help="Skip cpplint (C/C++)")
    l.add_argument("--no-codespell", action="store_true", help="Skip codespell")
    l.add_argument("--no-qt-connections", action="store_true", help="Skip Qt string-based SIGNAL/SLOT connection check")
    ct = l.add_mutually_exclusive_group()
    ct.add_argument("--clang-tidy", action="store_true", help="Force clang-tidy to run (fails if unavailable)")
    ct.add_argument("--no-clang-tidy", action="store_true", help="Disable clang-tidy (default is auto)")
    l.add_argument("--clang-format", action="store_true", help="Run clang-format (requires clang-format in PATH)")
    l.add_argument("--clazy", action="store_true", help="Run clazy (requires clazy-standalone + compile_commands.json)")
    l.add_argument("--clazy-checks", default=None, help="Comma-separated list of clazy checks (default: tools/lint defaults)")
    l.add_argument("--clazy-qt6", action="store_true", help="Run clazy QT6 porting checks (requires clazy-standalone + compile_commands.json)")
    l.add_argument("--clazy-qt6-checks", default=None, help="Comma-separated list of clazy QT6 checks (default: tools/lint defaults)")
    l.add_argument("--clang-style", default="file", help="Style passed to clang tools (default: file)")
    l.add_argument("--clang-tidy-build-dir", default="build", help="Build dir for clang-tidy (-p <dir>) (default: build)")
    l.add_argument("--clang-tidy-line-filter", help="Pass-through JSON for clang-tidy --line-filter")
    l.add_argument("--no-clang-tidy-auto-line-filter", action="store_true", help="Do not auto-generate a clang-tidy --line-filter from git diff")
    l.add_argument("--clang-build-dir", default="build", help="Build dir for clang tools requiring compile_commands.json (default: build)")

    fx = cmd("fix", "Apply auto-fixers (Black/clang-format, optional codespell) for the selected file set.", category="Lint")
    fx.add_argument("--base", help="Git base ref for changed-files mode (default: adapter-defined, usually origin/main)")
    fx.add_argument("--layer", type=int, help="Fix files in stack layer N (1-based; uses .devstack/stack.conf; ignored if --files is provided).")
    fx.add_argument("--all", action="store_true", help="Fix all tracked files (git ls-files)")
    fx.add_argument("--files", nargs="+", help="Explicit file list or glob patterns (overrides --layer/--base/--all)")
    fx.add_argument("--tools-from", default="origin", help="Where to run fix tooling from: origin|local|<path> (default: origin)")
    fx.add_argument("--log-dir", help="Directory for fix logs (default: .devstack/lint/logs)")
    fx.add_argument("--report-file", help="Markdown report path (default: .devstack/lint/fix-report.md)")
    fx.add_argument("--mode", default="auto", help="Output mode: auto|local|ci (default: auto; ci when running in GitHub Actions)")
    fx.add_argument("--verbose", action="store_true", help="Verbose fixer output (sets DEVSTACK_LINT_LOG_LEVEL=DEBUG for tools/lint scripts)")
    fx.add_argument("--quiet", action="store_true", help="Minimal output (only final report path)")
    fx.add_argument("--color", default="auto", help="Color output: auto|always|never (default: auto; disabled in CI)")
    fx.add_argument("--no-black", action="store_true", help="Skip Black formatter")
    fx.add_argument("--no-clang-format", action="store_true", help="Skip clang-format formatter")
    fx.add_argument("--codespell", action="store_true", help="Apply codespell fixes (writes to files)")
    fx.add_argument("--clang-style", default="file", help="Style passed to clang-format (default: file)")

    ign = cmd("ignore", "Ignore a commit and regenerate PR branches (filtered mode).", category="Stack")
    ign.add_argument("commitish")
    ign.add_argument("--name")
    ign.add_argument("--no-update", action="store_true")

    ext = cmd("extract", "Extract a commit onto a PR branch (optionally add ignore rule).", category="Stack")
    ext.add_argument("commitish")
    ext.add_argument("--to", dest="to_branch", required=True)
    ext.add_argument("--base")
    ext.add_argument("--no-ignore", action="store_true")
    ext.add_argument("--ignore-name")
    ext.add_argument("--force", action="store_true")
    ext.add_argument("--no-update", action="store_true")

    d = cmd("doctor", "Print environment sanity info (adapter, env-file, required tools).", category="Diagnostics")
    sc = cmd("self-check", "Sanity-check devstack itself (py_compile + wrapper smoke).", category="Diagnostics")
    sc.add_argument("--tests", action="store_true", help="Also run devstack unit tests")
    cmd("test", "Run devstack unit tests.", category="Diagnostics")
    pv = cmd("provision", "Show missing tooling and optionally preinstall Python lint tools.", category="Setup")
    pv.add_argument("--adapter", default=os.environ.get("DEVSTACK_ADAPTER", "auto"))
    pv.add_argument("--tools-from", default="origin", help="Where to run provisioning scripts from: origin|local|<path> (default: origin)")
    pv.add_argument("--python-lint", action="store_true", help="Install Python lint tools into cached venv (black, pylint, codespell, cpplint)")
    pv.add_argument("--clang-tools", action="store_true", help="Install clang tools via cpp-linter/clang-tools-pip (clang-format/clang-tidy)")
    pv.add_argument("--clang-tools-version", default="auto", help="Clang major version to install (auto uses .pre-commit-config.yaml when available)")
    pv.add_argument("--clang-tools-dir", help="Install directory for clang tools (default: ~/.cache/devstack/clang-tools/<ver>/bin)")
    pv.add_argument("--clang-tools-tool", nargs="*", help="Tool(s) to install (default: clang-format clang-tidy clangd)")
    pv.add_argument("--env-file", help="Env file to update with PATH (default: ~/.config/devstack/env.sh)")
    pv.add_argument("--verbose", action="store_true", help="Verbose provisioning output")
    sa = cmd("shell-alias", "Install a global shell alias/function for devstack (writes ~/.config/devstack/alias.* and updates your rc file).", category="Setup")
    sa.add_argument("--shell", default="auto", help="auto|bash|zsh|fish")
    sa.add_argument("--name", default="devstack", help="Function name to install (default: devstack)")
    sa.add_argument("--no-short", action="store_true", help="Do not create the ds shortcut")
    sa.add_argument("--no-rc", action="store_true", help="Do not edit your shell rc file; just print instructions")
    sa.add_argument("--rc-file", help="Override rc file path")
    d.add_argument("--adapter", default=os.environ.get("DEVSTACK_ADAPTER", "auto"))

    # Populate categorized help list.
    p.command_groups = [(k, v) for k, v in groups.items() if v]

    return p


def main(argv: list[str]) -> None:
    parser = build_parser()
    ns = parser.parse_args(argv)

    try:
        cmd = ns.cmd
        if cmd == "list":
            cmd_list(ns)
        elif cmd == "log":
            cmd_log(ns)
        elif cmd == "update":
            cmd_update(ns)
        elif cmd == "push":
            cmd_push(ns)
        elif cmd == "pr-layer":
            cmd_pr_layer(ns)
        elif cmd == "gh-sync":
            cmd_gh_sync(ns)
        elif cmd == "capture":
            cmd_capture(ns)
        elif cmd == "doctor":
            cmd_doctor(ns)
        elif cmd == "self-check":
            cmd_self_check(ns)
        elif cmd == "test":
            cmd_test(ns)
        elif cmd == "provision":
            cmd_provision(ns)
        elif cmd == "shell-alias":
            cmd_shell_alias(ns)
        elif cmd == "rebase":
            cmd_rebase(ns)
        elif cmd == "body-refresh":
            cmd_body_refresh(ns)
        elif cmd == "body-context":
            cmd_body_context(ns)
        elif cmd == "body-prune":
            cmd_body_prune(ns)
        elif cmd == "init":
            cmd_init(ns)
        elif cmd == "wt-init":
            cmd_wt_feature(ns)
        elif cmd == "wt-add":
            cmd_wt_add(ns)
        elif cmd == "wt-sync":
            cmd_wt_sync(ns)
        elif cmd == "wt-layer":
            cmd_wt_layer(ns)
        elif cmd == "layer-import":
            cmd_layer_import(ns)
        elif cmd == "build":
            if ns.no_submodules:
                ns.submodules = "off"
            cmd_build(ns)
        elif cmd == "lint":
            cmd_lint(ns)
        elif cmd == "fix":
            cmd_fix(ns)
        elif cmd == "ignore":
            cmd_ignore(ns)
        elif cmd == "extract":
            cmd_extract(ns)
        else:
            die(f"unknown command: {cmd}")
    except subprocess.CalledProcessError as exc:
        if getattr(ns, "debug", False):
            raise
        cmd_str = exc.cmd
        if isinstance(cmd_str, list):
            cmd_str = " ".join(shlex.quote(str(x)) for x in cmd_str)
        die(f"command failed (exit={exc.returncode}): {cmd_str}")
    except Exception as exc:
        if getattr(ns, "debug", False):
            raise
        die(f"unexpected failure: {exc}")


if __name__ == "__main__":
    main(sys.argv[1:])
