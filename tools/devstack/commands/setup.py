from __future__ import annotations

import argparse
import os
import re
import shlex
import shutil
import subprocess
import py_compile
from pathlib import Path

from tools.devstack.core.adapters import load_adapter
from tools.devstack.core.cache import devstack_cache_dir
from tools.devstack.core.tools import cmd_info
from tools.devstack.core.env import load_env_from_sh
from tools.devstack.core.git import current_branch, git, repo_root
from tools.devstack.core.proc import die, have_cmd, note, run


def _default_lint_venv_bin() -> Path:
    return devstack_cache_dir("python-lint", "venv", "bin")


def _cached_lint_venv_bin_for_repo(root: Path) -> Path:
    env = os.environ.get("DEVSTACK_LINT_VENV_BIN", "").strip()
    if env:
        return Path(env).expanduser().resolve()

    want = os.environ.get("DEVSTACK_ADAPTER", "auto")
    try:
        adapter = load_adapter(root, want)
    except SystemExit:
        adapter = None

    fn = getattr(adapter, "lint_venv_bin", None) if adapter else None
    if callable(fn):
        try:
            p = fn(root)
            if isinstance(p, Path):
                return p
            return Path(str(p)).expanduser().resolve()
        except Exception:
            pass

    return _default_lint_venv_bin()


def _print_lint_tools(root: Path) -> None:
    print()
    print("Lint tools:")

    pinned_cf = _precommit_clang_format_frozen_version(root)
    pinned_cf_major = ""
    if pinned_cf:
        try:
            pinned_cf_major = _major_version(pinned_cf)
        except ValueError:
            pinned_cf_major = ""

    for name, args_, label in (
        ("clang-tidy", ["--version"], "clang-tidy"),
        ("clang-format", ["--version"], "clang-format"),
        ("clazy-standalone", ["--version"], "clazy-standalone"),
        ("black", ["--version"], "black"),
        ("pylint", ["--version"], "pylint"),
        ("codespell", ["--version"], "codespell"),
        ("cpplint", ["--help"], "cpplint"),
    ):
        ok, origin, exe, out = cmd_info(
            root=root,
            name=name,
            check_args=args_,
            extra_bins=[_cached_lint_venv_bin_for_repo(root)],
        )
        if origin == "extra-bin":
            origin = "lint-venv"
        src = f" ({origin})" if origin else ""
        details: list[str] = []
        if exe:
            details.append(exe)
        if out:
            first = out.splitlines()[0].strip()
            if first:
                details.append(first)

        suffix = ""
        if name == "clang-format" and ok and exe:
            cf_major = ""
            m = re.search(r"\bversion\s+(\d+)(?:\.\d+){0,2}\b", out)
            if m:
                cf_major = m.group(1)

            parse_ok = True
            try:
                proc = run([exe, "--style=file", "-dump-config"], cwd=root, check=False, capture=True)
                parse_ok = proc.returncode == 0
            except Exception:
                parse_ok = False

            notes: list[str] = []
            if pinned_cf and pinned_cf_major and cf_major and cf_major != pinned_cf_major:
                notes.append(f"expected ~{pinned_cf_major}.x (pre-commit v{pinned_cf})")
            if not parse_ok:
                notes.append("incompatible with .clang-format")
            if notes:
                suffix = " [" + "; ".join(notes) + "]"

        extra = f" - {'; '.join(details)}" if details else ""
        print(f"tool {label}: {'yes' if ok else 'no'}{src}{extra}{suffix}")
    print("hint: use `ds provision` (and `ds provision --python-lint`) to set these up")


def _precommit_clang_format_frozen_version(root: Path) -> str | None:
    """Best-effort: extract the pinned clang-format version from .pre-commit-config.yaml.

    We look for the mirrors-clang-format repo and a comment like: "# frozen: v21.1.5".
    """
    path = root / ".pre-commit-config.yaml"
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None

    lines = text.splitlines()
    in_repo = False
    scan_budget = 0
    for line in lines:
        if re.search(r"^\s*-\s+repo:\s+https://github\.com/pre-commit/mirrors-clang-format\s*$", line):
            in_repo = True
            scan_budget = 12
            continue
        if in_repo and re.search(r"^\s*-\s+repo:\s+", line):
            # Next repo entry.
            in_repo = False
            scan_budget = 0
            continue
        if not in_repo:
            continue
        if scan_budget <= 0:
            break
        scan_budget -= 1

        m = re.search(r"frozen:\s*v(\d+(?:\.\d+){0,2})", line, flags=re.IGNORECASE)
        if m:
            return m.group(1)
    return None


def _major_version(version: str) -> str:
    v = (version or "").strip()
    m = re.match(r"^(\d+)", v)
    if not m:
        raise ValueError(f"invalid version: {version!r}")
    return m.group(1)


def _default_clang_tools_major(root: Path) -> str:
    frozen = _precommit_clang_format_frozen_version(root)
    if frozen:
        try:
            return _major_version(frozen)
        except ValueError:
            pass
    # Good default: matches the repo's pinned pre-commit clang-format (currently v21.x).
    return "21"


def _ensure_file_block(path: Path, *, begin: str, end: str, block: str) -> None:
    """Insert or replace a marked block inside a text file."""
    try:
        existing = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        existing = ""

    begin_line = f"# >>> {begin} >>>"
    end_line = f"# <<< {end} <<<"
    full_block = f"{begin_line}\n{block.rstrip()}\n{end_line}\n"

    if begin_line in existing and end_line in existing:
        new = re.sub(
            rf"{re.escape(begin_line)}[\s\S]*?{re.escape(end_line)}\n?",
            full_block,
            existing,
        )
    else:
        new = existing.rstrip() + ("\n\n" if existing.strip() else "") + full_block

    if new != existing:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(new, encoding="utf-8")

def cmd_doctor(args: argparse.Namespace) -> None:
    root = repo_root()
    print(f"repo: {root}")
    print(f"branch: {current_branch(root) or '(detached)'}")

    want = getattr(args, "adapter", os.environ.get("DEVSTACK_ADAPTER", "auto"))
    try:
        adapter = load_adapter(root, want)
        print(f"adapter: {adapter.ADAPTER_NAME}")
    except SystemExit:
        print(f"adapter: {want} (unresolved)")

    env_file = os.environ.get("DEVSTACK_ENV_FILE", "").strip()
    candidate = root / ".devstack" / "env.sh"
    global_candidate = Path.home() / ".config" / "devstack" / "env.sh"
    if not env_file and candidate.is_file():
        env_file = str(candidate)
    if not env_file and global_candidate.is_file():
        env_file = str(global_candidate)
    print(f"env-file: {env_file or '(none)'}")

    for tool in ("git", "cmake", "ninja", "gh", "ccache", "distcc"):
        print(f"tool {tool}: {'yes' if have_cmd(tool) else 'no'}")

    if os.environ.get("DISTCC_HOSTS", "").strip():
        print(f"DISTCC_HOSTS: {os.environ.get('DISTCC_HOSTS')}")
    if os.environ.get("CCACHE_PREFIX", "").strip():
        print(f"CCACHE_PREFIX: {os.environ.get('CCACHE_PREFIX')}")

    _print_lint_tools(root)


def cmd_self_check(args: argparse.Namespace) -> None:
    root = repo_root()
    devstack_dir = root / "tools" / "devstack"
    devstack_py = devstack_dir / "devstack.py"
    devstack_sh = devstack_dir / "devstack.sh"

    errors: list[str] = []
    if not devstack_py.is_file():
        errors.append(f"missing: {devstack_py}")
    if not devstack_sh.is_file():
        errors.append(f"missing: {devstack_sh}")

    try:
        if devstack_sh.is_file() and not os.access(devstack_sh, os.X_OK):
            errors.append(f"not executable: {devstack_sh}")
    except OSError:
        pass

    py_files = sorted(
        p
        for p in devstack_dir.rglob("*.py")
        if "__pycache__" not in p.parts
    )
    if not py_files:
        errors.append(f"no python files found under: {devstack_dir}")

    compile_errors: list[str] = []
    for p in py_files:
        try:
            py_compile.compile(str(p), doraise=True)
        except Exception as exc:
            compile_errors.append(f"{p}: {exc}")

    if compile_errors:
        errors.append("python compile failures:\n  " + "\n  ".join(compile_errors[:20]))

    try:
        run(["python3", str(devstack_py), "--help"], cwd=root, check=True, capture=True)
    except subprocess.CalledProcessError as exc:
        errors.append(f"devstack help failed (exit={exc.returncode}): {devstack_py} --help")

    if errors:
        die("self-check failed:\n- " + "\n- ".join(errors))

    if bool(getattr(args, "tests", False)):
        cmd_test(argparse.Namespace())
    print("self-check: OK")


def cmd_test(args: argparse.Namespace) -> None:
    root = repo_root()
    run(
        ["python3", "-m", "unittest", "discover", "-s", "tools/devstack/tests", "-p", "test_*.py"],
        cwd=root,
        check=True,
    )


def cmd_provision(args: argparse.Namespace) -> None:
    root = repo_root()
    origin_root = Path(__file__).resolve().parents[3]

    tools_from = (getattr(args, "tools_from", None) or "origin").strip()
    if tools_from == "local":
        tools_root = root
    elif tools_from == "origin":
        tools_root = origin_root
    else:
        tools_root = Path(tools_from).expanduser().resolve()

    tools_lint = tools_root / "tools" / "lint"
    if not tools_lint.is_dir():
        die(f"missing tools/lint under tools root: {tools_root}")

    want = getattr(args, "adapter", os.environ.get("DEVSTACK_ADAPTER", "auto"))
    try:
        adapter = load_adapter(root, want)
        adapter_name = adapter.ADAPTER_NAME
    except SystemExit:
        adapter_name = want

    def detect_pkg_manager() -> str:
        for name in ("apt-get", "dnf", "pacman", "zypper", "brew"):
            if have_cmd(name):
                return name
        return ""

    def tool_status(name: str) -> str:
        return "yes" if have_cmd(name) else "no"

    pkg_mgr = detect_pkg_manager()
    print(f"repo: {root}")
    print(f"adapter: {adapter_name}")
    print(f"package-manager: {pkg_mgr or '(unknown)'}")
    print()
    print("Tools (lint-related):")
    for t in ("clang-tidy", "clang-format", "clazy-standalone"):
        print(f"- {t}: {tool_status(t)}")
    frozen = _precommit_clang_format_frozen_version(root)
    if frozen:
        print(f"- clang-format (pre-commit pinned): {frozen}")
    print()

    if not have_cmd("clang-tidy") or not have_cmd("clang-format") or not have_cmd("clazy-standalone"):
        print("Install hints (system packages):")
        missing = [t for t in ("clang-tidy", "clang-format", "clazy") if not have_cmd(t if t != "clazy" else "clazy-standalone")]
        if pkg_mgr == "apt-get":
            pkgs = []
            if "clang-tidy" in missing:
                pkgs.append("clang-tidy")
            if "clang-format" in missing:
                pkgs.append("clang-format")
            if "clazy" in missing:
                pkgs.append("clazy")
            if pkgs:
                print(f"- sudo apt-get update && sudo apt-get install -y {' '.join(pkgs)}")
            else:
                print("- (none)")
        else:
            print("- Install the following tools via your package manager:")
            print(f"  - {', '.join(missing) if missing else '(none)'}")
            print("  - (For clazy, ensure `clazy-standalone` ends up on PATH)")
        print()

    if getattr(args, "clang_tools", False):
        major = (getattr(args, "clang_tools_version", "") or "auto").strip()
        if major == "auto":
            major = _default_clang_tools_major(root)
        try:
            major = _major_version(major)
        except ValueError as exc:
            die(str(exc))

        venv_dir = devstack_cache_dir("clang-tools", "venv")
        install_dir = (
            Path(getattr(args, "clang_tools_dir", "")).expanduser()
            if getattr(args, "clang_tools_dir", "")
            else devstack_cache_dir("clang-tools", major, "bin")
        )
        if not install_dir.is_absolute():
            install_dir = (root / install_dir).resolve()
        install_dir.mkdir(parents=True, exist_ok=True)

        tool_list = list(getattr(args, "clang_tools_tool", None) or [])
        if not tool_list:
            # clang-tools-static-binaries does not currently ship clangd; keep the default to formatting/linting tools.
            tool_list = ["clang-format", "clang-tidy"]

        if "clangd" in tool_list:
            note("clang-tools: clangd is not available from cpp-linter/clang-tools-static-binaries; skipping")
            tool_list = [t for t in tool_list if t != "clangd"]
        if not tool_list:
            die("clang-tools: no supported tools requested (try: --clang-tools-tool clang-format clang-tidy)")

        env_path = (getattr(args, "env_file", "") or "").strip()
        if not env_path:
            env_path = str(Path.home() / ".config" / "devstack" / "env.sh")
        env_file = Path(env_path).expanduser()

        print("Installing clang tools (cpp-linter/clang-tools-pip)...")
        print(f"- version: {major} (from {'pre-commit' if frozen else 'default'}; override with --clang-tools-version)")
        print(f"- install-dir: {install_dir}")
        print(f"- env-file: {env_file}")

        if not venv_dir.exists():
            print(f"- creating venv: {venv_dir}")
            run(["python3", "-m", "venv", str(venv_dir)], cwd=root, check=True)

        venv_py = venv_dir / "bin" / "python"
        clang_tools = venv_dir / "bin" / "clang-tools"

        run([str(venv_py), "-m", "pip", "install", "-q", "--upgrade", "pip"], cwd=root, check=True)
        run([str(venv_py), "-m", "pip", "install", "-q", "--upgrade", "clang-tools"], cwd=root, check=True)

        argv = [str(clang_tools), "--install", major, "--directory", str(install_dir), "--tool", *tool_list]
        try:
            run(argv, cwd=root, check=True)
        except subprocess.CalledProcessError:
            note("clang-tools: install failed")
            note("hint: try limiting tools to clang-format/clang-tidy:")
            note("  ds provision --clang-tools --clang-tools-tool clang-format clang-tidy")
            raise

        # Create unsuffixed shims (clang-format -> clang-format-<major>, etc).
        for t in tool_list:
            target = install_dir / f"{t}-{major}"
            link = install_dir / t
            if not target.exists():
                note(f"clang-tools: expected binary missing: {target}")
                continue
            try:
                if link.exists() or link.is_symlink():
                    link.unlink()
                link.symlink_to(target.name)
            except OSError:
                # Fall back to a tiny shell wrapper if symlinks aren't supported.
                link.write_text(f"#!/bin/sh\nexec \"{target}\" \"$@\"\n", encoding="utf-8")
                link.chmod(0o755)

        _ensure_file_block(
            env_file,
            begin="devstack clang-tools",
            end="devstack clang-tools",
            block="\n".join(
                [
                    f"export DEVSTACK_CLANG_TOOLS_VERSION={shlex_quote(major)}",
                    f"export DEVSTACK_CLANG_TOOLS_DIR={shlex_quote(str(install_dir))}",
                    'export PATH="$DEVSTACK_CLANG_TOOLS_DIR:$PATH"',
                ]
            ),
        )

        print("done")
        print(f"note: for interactive shells, run: source {env_file}")

    if getattr(args, "python_lint", False):
        prov = tools_lint / "provision.py"
        print("Installing Python lint tools (cached venv)...")
        lint_venv_bin = _cached_lint_venv_bin_for_repo(root)
        lint_venv_dir = lint_venv_bin.parent
        argv = ["python3", str(prov), "--venv-dir", str(lint_venv_dir)]
        if getattr(args, "verbose", False):
            argv.append("--verbose")
        run(argv, cwd=root, check=True)
        print("done")


def _detect_shell() -> str:
    shell = os.environ.get("SHELL", "").strip()
    name = Path(shell).name if shell else ""
    if name in ("bash", "zsh", "fish"):
        return name
    return "bash"


def _rc_path_for_shell(shell: str) -> Path:
    home = Path.home()
    if shell == "zsh":
        return home / ".zshrc"
    if shell == "fish":
        return home / ".config" / "fish" / "config.fish"
    return home / ".bashrc"


def _snippet_path_for_shell(shell: str) -> Path:
    cfg = Path.home() / ".config" / "devstack"
    if shell == "fish":
        return cfg / "alias.fish"
    return cfg / "alias.sh"


def _write_file(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _ensure_sourced(rc_path: Path, source_line: str) -> None:
    begin = "# >>> devstack >>>"
    end = "# <<< devstack <<<"

    try:
        existing = rc_path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        existing = ""

    # IMPORTANT: these are real newlines at runtime.
    block = f"{begin}\n{source_line}\n{end}\n"

    if begin in existing and end in existing:
        new = re.sub(r"# >>> devstack >>>[\s\S]*?# <<< devstack <<<\n?", block, existing)
        # Remove stray literal "\n" lines that may remain from older buggy installs.
        new = re.sub(r"^(\\n)+$", "", new, flags=re.MULTILINE)

        # Repair older versions that accidentally wrote literal "\\n" sequences into the rc file.
        new = new.replace("\\n\\n" + begin, "\n\n" + begin).replace("\\n" + begin, "\n" + begin)

        if new != existing:
            rc_path.parent.mkdir(parents=True, exist_ok=True)
            rc_path.write_text(new, encoding="utf-8")
        return

    if source_line in existing:
        return

    new = existing.rstrip() + ("\n\n" if existing.strip() else "") + block
    rc_path.parent.mkdir(parents=True, exist_ok=True)
    rc_path.write_text(new, encoding="utf-8")



def cmd_shell_alias(args: argparse.Namespace) -> None:
    shell = (args.shell or "auto").strip()
    if shell == "auto":
        shell = _detect_shell()
    if shell not in ("bash", "zsh", "fish"):
        die("--shell must be one of: auto|bash|zsh|fish")

    fn_name = (args.name or "devstack").strip()
    if not fn_name:
        die("--name must be non-empty")

    add_short = not bool(args.no_short)

    snippet_path = _snippet_path_for_shell(shell)
    devstack_py = (Path(__file__).resolve().parents[1] / "devstack.py").resolve()

    if shell == "fish":
        lines = [
            "# Generated by tools/devstack/devstack.py shell-alias",
            f"set -g DEVSTACK_PY {shlex_quote(str(devstack_py))}",
            "",
            "set -l _ds_env (set -q DEVSTACK_ENV_FILE_FISH; and echo $DEVSTACK_ENV_FILE_FISH; or echo ~/.config/devstack/env.fish)",
            "if test -f $_ds_env",
            "  source $_ds_env",
            "end",
            "",
            f"function {fn_name}",
            "  if test (count $argv) -eq 0",
            "    command python3 \"$DEVSTACK_PY\" --help",
            "  else",
            "    command python3 \"$DEVSTACK_PY\" $argv",
            "  end",
            "end",
        ]
        if add_short and fn_name != "ds":
            lines += ["", f"functions -c {fn_name} ds"]
        snippet_text = "\n".join(lines) + "\n"
        source_line = f"source {snippet_path}"
    else:
        lines = [
            "# Generated by tools/devstack/devstack.py shell-alias",
            f"DEVSTACK_PY={shlex_quote(str(devstack_py))}",
            "",
            'DEVSTACK_ENV_FILE="${DEVSTACK_ENV_FILE:-$HOME/.config/devstack/env.sh}"',
            'if [[ -f "$DEVSTACK_ENV_FILE" ]]; then source "$DEVSTACK_ENV_FILE"; fi',
            "",
            f"{fn_name}() {{",
            "  if [[ $# -eq 0 ]]; then",
            "    command python3 \"$DEVSTACK_PY\" --help",
            "  else",
            "    command python3 \"$DEVSTACK_PY\" \"$@\"",
            "  fi",
            "}",
        ]
        if add_short and fn_name != "ds":
            lines += ["", f"alias ds={fn_name}"]
        snippet_text = "\n".join(lines) + "\n"
        source_line = f'source "{snippet_path}"'

    _write_file(snippet_path, snippet_text)
    print(f"wrote: {snippet_path}")

    if args.no_rc:
        print(f"\nAdd this to your rc file to enable it:\n  {source_line}")
        return

    rc_path = Path(args.rc_file).expanduser() if args.rc_file else _rc_path_for_shell(shell)
    _ensure_sourced(rc_path, source_line)
    print(f"updated: {rc_path}")
    print("\nRestart your shell (or source your rc file) to pick up the alias.")

# CLI entrypoint is implemented in tools.devstack.cli.
