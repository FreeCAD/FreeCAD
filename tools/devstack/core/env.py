from __future__ import annotations

import subprocess
from pathlib import Path

from .proc import die, have_cmd


def parse_env0(data: bytes) -> dict[str, str]:
    out: dict[str, str] = {}
    for item in data.split(b"\0"):
        if not item:
            continue
        if b"=" not in item:
            continue
        k, v = item.split(b"=", 1)
        out[k.decode("utf-8", errors="replace")] = v.decode("utf-8", errors="replace")
    return out


def load_env_from_sh(path: Path, base_env: dict[str, str]) -> dict[str, str]:
    if not have_cmd("bash"):
        die("bash not found (required for --env-file)")
    if not path.is_file():
        die(f"env file not found: {path}")
    try:
        proc = subprocess.run(
            ["bash", "-lc", 'set -a; source "$1"; env -0', "bash", str(path)],
            check=True,
            env=base_env,
            stdout=subprocess.PIPE,
        )
    except subprocess.CalledProcessError as exc:
        die(f"failed to load env file: {path} (exit {exc.returncode})")
    return parse_env0(proc.stdout or b"")
