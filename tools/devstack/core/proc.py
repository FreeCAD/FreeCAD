from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional


def die(message: str) -> "None":
    print(f"error: {message}", file=sys.stderr)
    raise SystemExit(1)


def note(message: str) -> None:
    print(f"note: {message}", file=sys.stderr)


def have_cmd(name: str) -> bool:
    return shutil.which(name) is not None


def run(
    args: list[str],
    *,
    cwd: Optional[Path] = None,
    check: bool = True,
    capture: bool = False,
    env: Optional[dict[str, str]] = None,
) -> subprocess.CompletedProcess[str]:
    try:
        return subprocess.run(
            args,
            cwd=str(cwd) if cwd else None,
            check=check,
            text=True,
            env=env,
            stdout=subprocess.PIPE if capture else None,
            stderr=subprocess.PIPE if capture else None,
        )
    except subprocess.CalledProcessError as exc:
        if capture:
            if exc.stdout:
                sys.stdout.write(exc.stdout)
            if exc.stderr:
                sys.stderr.write(exc.stderr)
        raise
