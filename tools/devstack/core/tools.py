from __future__ import annotations

import shutil
from pathlib import Path

from tools.devstack.core.proc import run


def which(name: str, *, extra_bins: list[Path] | None = None) -> tuple[str | None, str]:
    """Return (exe_path, origin) for a tool name.

    origin is one of:
    - "PATH"
    - "extra-bin" (when resolved via `extra_bins`)
    """
    exe = shutil.which(name)
    if exe:
        return exe, "PATH"

    for b in extra_bins or []:
        candidate = b / name
        if candidate.is_file():
            return str(candidate), "extra-bin"
    return None, ""


def cmd_info(
    *,
    root: Path,
    name: str,
    check_args: list[str],
    extra_bins: list[Path] | None = None,
) -> tuple[bool, str, str, str]:
    """Return (ok, origin, exe, first_output_line)."""
    exe, origin = which(name, extra_bins=extra_bins)
    if not exe:
        return False, "", "", ""

    try:
        proc = run([exe, *check_args], cwd=root, check=False, capture=True)
    except Exception:
        return False, origin, exe, ""

    out = ((proc.stdout or "") + "\n" + (proc.stderr or "")).strip()
    if proc.returncode != 0:
        return False, origin, exe, out
    return True, origin, exe, out

