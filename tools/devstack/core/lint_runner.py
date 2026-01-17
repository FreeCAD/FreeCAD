from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

from tools.devstack.core.adapters import load_adapter
from tools.devstack.core.git import ensure_commit_exists, git, repo_root
from tools.devstack.core.proc import die, have_cmd, note, run
from tools.devstack.core.stackconf import filtered_mode, read_conf


@dataclass(frozen=True)
class ToolingPaths:
    tools_root: Path
    tools_lint: Path
    tools_label: str


@dataclass(frozen=True)
class Ansi:
    RESET: str = ""
    BOLD: str = ""
    DIM: str = ""
    GREEN: str = ""
    YELLOW: str = ""
    RED: str = ""
    CYAN: str = ""


@dataclass(frozen=True)
class FileSet:
    files: list[str]
    python_files: list[str]
    cpp_files: list[str]
    text_files: list[str]
    diffspec_for_lines: str | None


def _origin_root() -> Path:
    # tools/devstack/core/lint_runner.py -> <repo> is parents[3]
    return Path(__file__).resolve().parents[3]


def _resolve_tools_lint(root: Path, tools_from: str) -> ToolingPaths:
    origin_root = _origin_root()
    tools_from = (tools_from or "origin").strip()
    if tools_from == "local":
        tools_root = root
    elif tools_from == "origin":
        tools_root = origin_root
    else:
        tools_root = Path(tools_from).expanduser().resolve()

    tools_lint = tools_root / "tools" / "lint"
    if not tools_lint.is_dir():
        die(f"missing tools/lint under tools root: {tools_root}")

    tools_label = "origin" if tools_root == origin_root else ("local" if tools_root == root else str(tools_root))
    return ToolingPaths(tools_root=tools_root, tools_lint=tools_lint, tools_label=tools_label)


def _resolve_mode(mode: str) -> str:
    mode = (mode or "auto").strip().lower()
    if mode == "auto":
        return "ci" if (os.environ.get("GITHUB_ACTIONS") or os.environ.get("RUNNER_WORKSPACE")) else "local"
    if mode not in ("local", "ci"):
        die("--mode must be one of: auto|local|ci")
    return mode


def _want_color(*, mode: str, color_mode: str) -> bool:
    color_mode = (color_mode or "auto").strip().lower()
    if color_mode not in ("auto", "always", "never"):
        die("--color must be one of: auto|always|never")
    if color_mode == "never":
        return False
    if os.environ.get("NO_COLOR") is not None:
        return False
    if os.environ.get("CLICOLOR", "").strip() == "0":
        return False
    if color_mode == "always":
        return True
    if mode == "ci":
        return False
    return bool(sys.stdout.isatty())


def _ansi(use_color: bool) -> Ansi:
    if not use_color:
        return Ansi()
    return Ansi(
        RESET="\033[0m",
        BOLD="\033[1m",
        DIM="\033[2m",
        GREEN="\033[32m",
        YELLOW="\033[33m",
        RED="\033[31m",
        CYAN="\033[36m",
    )


def _git_lines(root: Path, argv: list[str]) -> list[str]:
    out = git(argv, cwd=root)
    return [line.strip() for line in out.splitlines() if line.strip()]


def _resolve_existing_base_ref(root: Path, want: str) -> str:
    want = (want or "").strip()
    if not want:
        return ""
    try:
        run(["git", "rev-parse", "--verify", "--quiet", want], cwd=root, capture=True)
        return want
    except subprocess.CalledProcessError:
        return ""


def _fallback_base_ref(root: Path, want: str) -> str:
    # Only used when the default base ref doesn't exist (common when the user's push remote
    # isn't the upstream remote, or the remote wasn't fetched in this worktree).
    want = (want or "").strip()
    branch = want.split("/", 1)[1] if "/" in want else ""
    if not branch:
        branch = "main"

    try:
        remotes = [r.strip() for r in git(["remote"], cwd=root).splitlines() if r.strip()]
    except subprocess.CalledProcessError:
        remotes = []

    ordered: list[str] = []
    for r in ("upstream", "origin"):
        if r in remotes:
            ordered.append(r)
    for r in remotes:
        if r not in ordered:
            ordered.append(r)

    candidates: list[str] = []
    # Prefer the stack base ref if configured.
    try:
        conf = read_conf(root)
        if conf.base_remote_ref and conf.base_remote_ref != want:
            candidates.append(conf.base_remote_ref)
    except SystemExit:
        pass
    for r in ordered:
        candidates.append(f"{r}/{branch}")

    for c in candidates:
        resolved = _resolve_existing_base_ref(root, c)
        if resolved:
            return resolved
    return ""


def _has_ext(path: str, exts: tuple[str, ...]) -> bool:
    lower = path.lower()
    return any(lower.endswith(e) for e in exts)


def _classify_files(files: list[str], *, include_svg: bool) -> tuple[list[str], list[str], list[str]]:
    py_exts = (".py", ".pyi")
    cpp_exts = (
        ".c",
        ".cc",
        ".cpp",
        ".cxx",
        ".cu",
        ".cuh",
        ".h",
        ".hh",
        ".hpp",
        ".hxx",
        ".h++",
        ".c++",
        ".c++",
    )
    text_exts = py_exts + cpp_exts + (
        ".md",
        ".txt",
        ".rst",
        ".cmake",
        "cmakelists.txt",
        ".yml",
        ".yaml",
        ".json",
        ".toml",
        ".ini",
        ".cfg",
        ".sh",
        ".bat",
        ".ps1",
        ".qml",
        ".ui",
        ".qrc",
    )
    if include_svg:
        text_exts = text_exts + (".svg",)

    python_files = [f for f in files if _has_ext(f, py_exts)]
    cpp_files = [f for f in files if _has_ext(f, cpp_exts)]
    text_files = [f for f in files if _has_ext(f, text_exts)]
    return python_files, cpp_files, text_files


def _select_files(
    *,
    root: Path,
    args: argparse.Namespace,
    base_default: str,
    quiet: bool,
    label: str,
) -> FileSet:
    base_arg = (getattr(args, "base", "") or "").strip()
    base = (base_arg or base_default).strip()

    diffspec_for_lines: str | None = None
    files: list[str]

    if getattr(args, "files", None):
        files = list(args.files)
        # Best-effort: use the same base ref as changed-files mode.
        try:
            run(["git", "rev-parse", "--verify", "--quiet", base], cwd=root, capture=True)
            diffspec_for_lines = f"{base}...HEAD"
        except subprocess.CalledProcessError:
            diffspec_for_lines = None
    elif getattr(args, "layer", None):
        layer = int(args.layer)
        conf = read_conf(root)
        if layer < 1 or layer > len(conf.entries):
            die(f"--layer must be 1..{len(conf.entries)}")
        entry = conf.entries[layer - 1]
        from_ref = conf.base_remote_ref if layer == 1 else (
            conf.entries[layer - 2].branch if filtered_mode(conf) else conf.entries[layer - 2].sha
        )
        to_ref = entry.branch if filtered_mode(conf) else entry.sha
        diffspec_for_lines = f"{from_ref}..{to_ref}"
        if filtered_mode(conf):
            try:
                run(["git", "show-ref", "--verify", "--quiet", f"refs/heads/{entry.branch}"], cwd=root)
            except subprocess.CalledProcessError:
                die(f"missing local branch: {entry.branch} (run: devstack.sh update)")
        else:
            ensure_commit_exists(root, entry.sha)
        files = _git_lines(root, ["diff", "--name-only", "--diff-filter=ACMRT", f"{from_ref}..{to_ref}"])
    elif getattr(args, "all", False):
        files = _git_lines(root, ["ls-files"])
    else:
        if not _resolve_existing_base_ref(root, base):
            if not base_arg:
                fb = _fallback_base_ref(root, base)
                if fb:
                    if not quiet:
                        note(f"{label}: base ref {base!r} not found; using {fb!r}")
                    base = fb
                else:
                    die(f"unknown --base ref: {base} (pass --base <ref> or fetch your remote)")
            else:
                die(f"unknown --base ref: {base} (pass a valid --base <ref>)")
        diffspec_for_lines = f"{base}...HEAD"
        files = _git_lines(root, ["diff", "--name-only", "--diff-filter=ACMRT", f"{base}...HEAD"])

    include_svg = bool(getattr(args, "_devstack_include_svg", True))
    python_files, cpp_files, text_files = _classify_files(files, include_svg=include_svg)
    return FileSet(
        files=files,
        python_files=python_files,
        cpp_files=cpp_files,
        text_files=text_files,
        diffspec_for_lines=diffspec_for_lines,
    )


def _compile_commands_candidates(base_dir: Path) -> list[Path]:
    candidates: list[Path] = []
    try:
        # Common: build/<preset>/compile_commands.json
        for child in base_dir.iterdir():
            if child.is_dir() and (child / "compile_commands.json").is_file():
                candidates.append(child)
    except OSError:
        return []

    # One more bounded level: build/*/*/compile_commands.json
    if not candidates:
        try:
            for child in base_dir.iterdir():
                if not child.is_dir():
                    continue
                for grand in child.iterdir():
                    if grand.is_dir() and (grand / "compile_commands.json").is_file():
                        candidates.append(grand)
        except OSError:
            return []

    return candidates


def _pick_best_compile_commands_dir(candidates: list[Path]) -> Path:
    non_legacy = [p for p in candidates if "legacy" not in p.name.lower()]
    if non_legacy:
        candidates = non_legacy

    def mtime(p: Path) -> float:
        try:
            return (p / "compile_commands.json").stat().st_mtime
        except OSError:
            return 0.0

    return max(candidates, key=lambda p: (mtime(p), p.name))


def resolve_compile_commands_build_dir(
    *,
    root: Path,
    build_dir_arg: str,
    strict: bool,
    quiet: bool,
) -> Path | None:
    base_dir = Path(build_dir_arg)
    if not base_dir.is_absolute():
        base_dir = (root / base_dir).resolve()
    if not base_dir.is_dir():
        return None
    if (base_dir / "compile_commands.json").is_file():
        return base_dir

    candidates = _compile_commands_candidates(base_dir)
    if not candidates:
        return None
    if len(candidates) == 1:
        return candidates[0]

    if strict:
        shown = ", ".join(str(p) for p in candidates[:5])
        more = f" (+{len(candidates) - 5} more)" if len(candidates) > 5 else ""
        die(
            f"multiple compile_commands.json candidates under {base_dir}: {shown}{more}; "
            f"pass an explicit build dir (e.g. --clang-tidy-build-dir {candidates[0]})"
        )

    chosen = _pick_best_compile_commands_dir(candidates)
    if not quiet:
        def _rel(p: Path) -> str:
            try:
                return str(p.relative_to(root))
            except Exception:
                return str(p)

        shown = ", ".join(_rel(p) for p in candidates[:5])
        more = f" (+{len(candidates) - 5} more)" if len(candidates) > 5 else ""
        note(f"multiple compile_commands.json candidates under {base_dir}: {shown}{more}; using {chosen}")
    return chosen


def _compute_clang_tidy_line_filter(root: Path, diffs: str, paths: list[str]) -> str:
    proc = run(
        ["git", "diff", "--unified=0", diffs, "--", *paths],
        cwd=root,
        check=False,
        capture=True,
    )
    text = (proc.stdout or "")
    current: str | None = None
    ranges: dict[str, list[tuple[int, int]]] = {}

    for raw in text.splitlines():
        if raw.startswith("diff --git "):
            parts = raw.split()
            if len(parts) >= 4:
                b = parts[3]
                current = b[2:] if b.startswith("b/") else b
            continue

        if not current:
            continue

        if raw.startswith("@@"):
            m = re.search(r"\+(\d+)(?:,(\d+))?", raw)
            if not m:
                continue
            start = int(m.group(1))
            count = int(m.group(2) or "1")
            if count <= 0:
                continue
            end = start + count - 1
            ranges.setdefault(current, []).append((start, end))

    wanted = {p for p in paths}
    entries: list[dict] = []
    for name, rs in sorted(ranges.items()):
        if name not in wanted:
            continue
        rs.sort()
        merged: list[list[int]] = []
        for s, e in rs:
            if not merged or s > merged[-1][1] + 1:
                merged.append([s, e])
            else:
                merged[-1][1] = max(merged[-1][1], e)
        entries.append({"name": name, "lines": merged})

    return json.dumps(entries, separators=(",", ":"))


def _print_tail(path: Path, *, count: int) -> None:
    if count <= 0 or not path.is_file():
        return
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return
    tail = lines[-count:] if len(lines) > count else lines
    if not tail:
        return
    print(f"--- {path} (tail {min(len(lines), count)} lines) ---")
    for line in tail:
        print(line)


def run_lint(args: argparse.Namespace) -> None:
    root = repo_root()
    tools = _resolve_tools_lint(root, getattr(args, "tools_from", "origin"))
    mode = _resolve_mode(getattr(args, "mode", "auto"))
    quiet = bool(getattr(args, "quiet", False))
    verbose = bool(getattr(args, "verbose", False))
    log_tail = int(getattr(args, "log_tail", 40) or 0)
    ansi = _ansi(_want_color(mode=mode, color_mode=getattr(args, "color", "auto")))

    try:
        adapter = load_adapter(root, os.environ.get("DEVSTACK_ADAPTER", "auto"))
        base_default = getattr(adapter, "lint_default_base_ref", lambda _root: "origin/main")(root)
    except SystemExit:
        base_default = "origin/main"

    setattr(args, "_devstack_include_svg", True)
    fileset = _select_files(root=root, args=args, base_default=base_default, quiet=quiet, label="lint")
    if not fileset.files:
        print("lint: no files")
        return

    lint_root = root / ".devstack" / "lint"
    log_dir = Path(getattr(args, "log_dir", "")).expanduser() if getattr(args, "log_dir", None) else (lint_root / "logs")
    report_file = (
        Path(getattr(args, "report_file", "")).expanduser()
        if getattr(args, "report_file", None)
        else (lint_root / "report.md")
    )
    if not log_dir.is_absolute():
        log_dir = (root / log_dir).resolve()
    if not report_file.is_absolute():
        report_file = (root / report_file).resolve()
    log_dir.mkdir(parents=True, exist_ok=True)
    report_file.parent.mkdir(parents=True, exist_ok=True)
    report_file.write_text("# Lint report\n\n", encoding="utf-8")

    lint_env = os.environ.copy()
    lint_env["DEVSTACK_LINT_MODE"] = mode
    if verbose:
        lint_env["DEVSTACK_LINT_LOG_LEVEL"] = "DEBUG"
    elif mode == "ci":
        lint_env["DEVSTACK_LINT_LOG_LEVEL"] = "INFO"
    else:
        lint_env["DEVSTACK_LINT_LOG_LEVEL"] = "WARNING"

    if not quiet:
        print(
            f"lint: mode={mode} tools={tools.tools_label} files={len(fileset.files)} "
            f"(text={len(fileset.text_files)} py={len(fileset.python_files)} cpp={len(fileset.cpp_files)})"
        )

    fail_fast = bool(getattr(args, "fail_fast", False))
    ok = True

    def print_skip(name: str, reason: str) -> None:
        if quiet:
            return
        print(f"{ansi.BOLD}{name:<12}{ansi.RESET} {ansi.DIM}...{ansi.RESET} {ansi.YELLOW}SKIP{ansi.RESET}{ansi.DIM} ({reason}){ansi.RESET}")

    def run_checker(
        name: str,
        argv: list[str],
        *,
        logs: list[Path] | None = None,
        allow_failure: bool = False,
    ) -> None:
        nonlocal ok
        if not quiet:
            print(f"{ansi.BOLD}{name:<12}{ansi.RESET} {ansi.DIM}...{ansi.RESET}", end="", flush=True)
        try:
            run(argv, cwd=root, check=True, env=lint_env)
            if not quiet:
                print(f" {ansi.GREEN}OK{ansi.RESET}")
        except subprocess.CalledProcessError:
            if allow_failure:
                if not quiet:
                    print(f" {ansi.YELLOW}WARN{ansi.RESET}")
                    for p in (logs or []):
                        _print_tail(p, count=log_tail)
                return

            ok = False
            if not quiet:
                print(f" {ansi.RED}FAIL{ansi.RESET}")
                for p in (logs or []):
                    _print_tail(p, count=log_tail)
            if fail_fast:
                raise

    if not getattr(args, "no_generic", False) and fileset.text_files:
        run_checker(
            "generic",
            [
                "python3",
                str(tools.tools_lint / "generic_checks.py"),
                "--whitespace-check",
                "--tabs-check",
                "--lineendings-check",
                "--files",
                *fileset.text_files,
                "--log-dir",
                str(log_dir),
                "--report-file",
                str(report_file),
            ],
            logs=[log_dir / "whitespace.log", log_dir / "tabs.log", log_dir / "lineendings.log"],
        )

    if not getattr(args, "no_case_check", False) and fileset.files:
        run_checker(
            "case-check",
            [
                "python3",
                str(tools.tools_lint / "case_collisions.py"),
                "--files",
                *fileset.files,
                "--log-dir",
                str(log_dir),
                "--report-file",
                str(report_file),
            ],
            logs=[log_dir / "case-collisions.log"],
        )

    if not getattr(args, "no_black", False) and fileset.python_files:
        run_checker(
            "black",
            [
                "python3",
                str(tools.tools_lint / "black.py"),
                "--files",
                *fileset.python_files,
                "--log-dir",
                str(log_dir),
                "--report-file",
                str(report_file),
            ],
            logs=[log_dir / "black.log"],
        )

    if not getattr(args, "no_pylint", False) and fileset.python_files:
        run_checker(
            "pylint",
            [
                "python3",
                str(tools.tools_lint / "pylint.py"),
                "--files",
                *fileset.python_files,
                "--log-dir",
                str(log_dir),
                "--report-file",
                str(report_file),
            ],
            logs=[log_dir / "pylint.log"],
        )

    if not getattr(args, "no_cpplint", False) and fileset.cpp_files:
        run_checker(
            "cpplint",
            [
                "python3",
                str(tools.tools_lint / "cpplint.py"),
                "--files",
                *fileset.cpp_files,
                "--log-dir",
                str(log_dir),
                "--report-file",
                str(report_file),
            ],
            logs=[log_dir / "cpplint.log"],
        )

    if not getattr(args, "no_qt_connections", False) and fileset.cpp_files:
        run_checker(
            "qt-conns",
            [
                "python3",
                str(tools.tools_lint / "qt_connections.py"),
                "--files",
                *fileset.cpp_files,
                "--log-dir",
                str(log_dir),
                "--report-file",
                str(report_file),
            ],
            logs=[log_dir / "qtConnections.log"],
            allow_failure=True,
        )

    if getattr(args, "clang_format", False) and fileset.cpp_files:
        run_checker(
            "clang-format",
            [
                "python3",
                str(tools.tools_lint / "clang_format.py"),
                "--files",
                *fileset.cpp_files,
                "--clang-style",
                getattr(args, "clang_style", "file"),
                "--log-dir",
                str(log_dir),
                "--report-file",
                str(report_file),
            ],
            logs=[log_dir / "clang-format.log"],
        )

    clang_tidy_mode = (
        "off"
        if getattr(args, "no_clang_tidy", False)
        else ("on" if getattr(args, "clang_tidy", False) else "auto")
    )
    if clang_tidy_mode != "off" and fileset.cpp_files:
        strict = clang_tidy_mode == "on"
        effective_build_dir = resolve_compile_commands_build_dir(
            root=root,
            build_dir_arg=getattr(args, "clang_tidy_build_dir", "build"),
            strict=strict,
            quiet=quiet,
        )
        have_compile_commands = bool(effective_build_dir)
        have_clang_tidy = have_cmd("clang-tidy")

        if clang_tidy_mode == "auto" and not (have_clang_tidy and have_compile_commands):
            reason = []
            if not have_clang_tidy:
                reason.append("clang-tidy missing")
            if not have_compile_commands:
                reason.append(f"no compile_commands.json in {getattr(args,'clang_tidy_build_dir','build')} (or subdirs)")
            print_skip("clang-tidy", ", ".join(reason) if reason else "unavailable")
        else:
            clang_tidy_build_dir_arg = str(effective_build_dir) if effective_build_dir else getattr(args, "clang_tidy_build_dir", "build")

            if (
                not getattr(args, "clang_tidy_line_filter", None)
                and not getattr(args, "no_clang_tidy_auto_line_filter", False)
                and fileset.diffspec_for_lines
            ):
                try:
                    args.clang_tidy_line_filter = _compute_clang_tidy_line_filter(
                        root,
                        fileset.diffspec_for_lines,
                        fileset.cpp_files,
                    )
                except Exception:
                    pass

            export_fixes = log_dir / "clang-tidy.yaml"
            argv = [
                "python3",
                str(tools.tools_lint / "clang_tidy.py"),
                "--files",
                *fileset.cpp_files,
                "--clang-style",
                getattr(args, "clang_style", "file"),
                "--build-dir",
                clang_tidy_build_dir_arg,
                "--export-fixes",
                str(export_fixes),
                "--log-dir",
                str(log_dir),
                "--report-file",
                str(report_file),
            ]
            if getattr(args, "clang_tidy_line_filter", None):
                argv += ["--line-filter", args.clang_tidy_line_filter]
            run_checker("clang-tidy", argv, logs=[log_dir / "clang-tidy.log"])

    if getattr(args, "clazy", False) and fileset.cpp_files:
        export_fixes = log_dir / "clazy.yaml"
        effective_build_dir = resolve_compile_commands_build_dir(
            root=root,
            build_dir_arg=getattr(args, "clang_build_dir", "build"),
            strict=False,
            quiet=quiet,
        )
        clazy_build_dir_arg = str(effective_build_dir) if effective_build_dir else getattr(args, "clang_build_dir", "build")
        argv = [
            "python3",
            str(tools.tools_lint / "clazy.py"),
            "--files",
            *fileset.cpp_files,
            "--build-dir",
            clazy_build_dir_arg,
            "--export-fixes",
            str(export_fixes),
            "--log-dir",
            str(log_dir),
            "--report-file",
            str(report_file),
        ]
        if getattr(args, "clazy_checks", None):
            argv += ["--clazy-checks", args.clazy_checks]
        run_checker("clazy", argv, logs=[log_dir / "clazy.log"])

    if getattr(args, "clazy_qt6", False) and fileset.cpp_files:
        export_fixes = log_dir / "clazyQT6.yaml"
        effective_build_dir = resolve_compile_commands_build_dir(
            root=root,
            build_dir_arg=getattr(args, "clang_build_dir", "build"),
            strict=False,
            quiet=quiet,
        )
        clazy_qt6_build_dir_arg = str(effective_build_dir) if effective_build_dir else getattr(args, "clang_build_dir", "build")
        argv = [
            "python3",
            str(tools.tools_lint / "clazy_qt6.py"),
            "--files",
            *fileset.cpp_files,
            "--build-dir",
            clazy_qt6_build_dir_arg,
            "--export-fixes",
            str(export_fixes),
            "--log-dir",
            str(log_dir),
            "--report-file",
            str(report_file),
        ]
        if getattr(args, "clazy_qt6_checks", None):
            argv += ["--clazy-qt6-checks", args.clazy_qt6_checks]
        run_checker("clazy-qt6", argv, logs=[log_dir / "clazyQT6.log"])

    if not getattr(args, "no_codespell", False) and fileset.text_files:
        run_checker(
            "codespell",
            [
                "python3",
                str(tools.tools_lint / "codespell.py"),
                "--files",
                *fileset.text_files,
                "--log-dir",
                str(log_dir),
                "--report-file",
                str(report_file),
            ],
            logs=[log_dir / "codespell.log"],
        )

    if ansi.CYAN and not quiet:
        print(f"lint report: {ansi.CYAN}{report_file}{ansi.RESET}")
    else:
        print(f"lint report: {report_file}")
    if not ok:
        raise SystemExit(1)


def run_fix(args: argparse.Namespace) -> None:
    root = repo_root()
    tools = _resolve_tools_lint(root, getattr(args, "tools_from", "origin"))
    mode = _resolve_mode(getattr(args, "mode", "auto"))
    quiet = bool(getattr(args, "quiet", False))
    verbose = bool(getattr(args, "verbose", False))
    ansi = _ansi(_want_color(mode=mode, color_mode=getattr(args, "color", "auto")))

    try:
        adapter = load_adapter(root, os.environ.get("DEVSTACK_ADAPTER", "auto"))
        base_default = getattr(adapter, "lint_default_base_ref", lambda _root: "origin/main")(root)
    except SystemExit:
        base_default = "origin/main"

    setattr(args, "_devstack_include_svg", False)
    fileset = _select_files(root=root, args=args, base_default=base_default, quiet=quiet, label="fix")
    if not fileset.files:
        print("fix: no files")
        return

    fix_root = root / ".devstack" / "lint"
    log_dir = Path(getattr(args, "log_dir", "")).expanduser() if getattr(args, "log_dir", None) else (fix_root / "logs")
    report_file = (
        Path(getattr(args, "report_file", "")).expanduser()
        if getattr(args, "report_file", None)
        else (fix_root / "fix-report.md")
    )
    if not log_dir.is_absolute():
        log_dir = (root / log_dir).resolve()
    if not report_file.is_absolute():
        report_file = (root / report_file).resolve()
    log_dir.mkdir(parents=True, exist_ok=True)
    report_file.parent.mkdir(parents=True, exist_ok=True)
    report_file.write_text("# Fix report\n\n", encoding="utf-8")

    fix_env = os.environ.copy()
    fix_env["DEVSTACK_LINT_MODE"] = mode
    fix_env["DEVSTACK_LINT_LOG_LEVEL"] = "DEBUG" if verbose else ("INFO" if mode == "ci" else "WARNING")

    if not quiet:
        print(
            f"fix: mode={mode} tools={tools.tools_label} files={len(fileset.files)} "
            f"(text={len(fileset.text_files)} py={len(fileset.python_files)} cpp={len(fileset.cpp_files)})"
        )

    def step(name: str, argv: list[str], *, log_file: Path | None = None) -> None:
        if not quiet:
            print(f"{ansi.BOLD}{name:<12}{ansi.RESET} {ansi.DIM}...{ansi.RESET}", end="", flush=True)
        cp = run(argv, cwd=root, check=False, env=fix_env)
        if cp.returncode == 0:
            if not quiet:
                print(f" {ansi.GREEN}OK{ansi.RESET}")
            return

        if not quiet:
            print(f" {ansi.RED}FAIL{ansi.RESET}")
            cmd_display = " ".join(shlex.quote(str(x)) for x in argv)
            print(f"{ansi.DIM}command:{ansi.RESET} {cmd_display}")
            if log_file:
                print(f"{ansi.DIM}log:{ansi.RESET} {log_file}")
                try:
                    log_text = log_file.read_text(encoding="utf-8", errors="replace")
                except OSError:
                    log_text = ""
                if "error: unknown key" in log_text and "Error reading" in log_text and ".clang-format" in log_text:
                    print(
                        f"{ansi.YELLOW}hint:{ansi.RESET} clang-format failed to parse .clang-format (version mismatch). "
                        f"Try `devstack fix --no-clang-format`, or pass `--clang-style LLVM`, or update `.clang-format`."
                    )
            print(f"{ansi.DIM}report:{ansi.RESET} {report_file}")

        raise SystemExit(cp.returncode or 1)

    def skip(name: str, reason: str) -> None:
        if quiet:
            return
        print(f"{ansi.BOLD}{name:<12}{ansi.RESET} {ansi.DIM}...{ansi.RESET} {ansi.YELLOW}SKIP{ansi.RESET}{ansi.DIM} ({reason}){ansi.RESET}")

    if not getattr(args, "no_black", False) and fileset.python_files:
        step(
            "black",
            [
                "python3",
                str(tools.tools_lint / "black.py"),
                "--apply",
                "--files",
                *fileset.python_files,
                "--log-dir",
                str(log_dir),
                "--report-file",
                str(report_file),
            ],
            log_file=log_dir / "black.log",
        )

    if not getattr(args, "no_clang_format", False) and fileset.cpp_files:
        if not have_cmd("clang-format"):
            skip("clang-format", "clang-format missing")
        else:
            step(
                "clang-format",
                [
                    "python3",
                    str(tools.tools_lint / "clang_format.py"),
                    "--apply",
                    "--files",
                    *fileset.cpp_files,
                    "--clang-style",
                    getattr(args, "clang_style", "file"),
                    "--log-dir",
                    str(log_dir),
                    "--report-file",
                    str(report_file),
                ],
                log_file=log_dir / "clang-format.log",
            )

    if getattr(args, "codespell", False) and fileset.text_files:
        step(
            "codespell",
            [
                "python3",
                str(tools.tools_lint / "codespell.py"),
                "--write",
                "--files",
                *fileset.text_files,
                "--log-dir",
                str(log_dir),
                "--report-file",
                str(report_file),
            ],
            log_file=log_dir / "codespell.log",
        )

    if ansi.CYAN and not quiet:
        print(f"fix report: {ansi.CYAN}{report_file}{ansi.RESET}")
    else:
        print(f"fix report: {report_file}")
