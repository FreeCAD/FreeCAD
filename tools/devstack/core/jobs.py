from __future__ import annotations

import os
from collections.abc import Mapping


def _parse_positive_int(value: str) -> int | None:
    value = value.strip()
    if not value:
        return None
    try:
        parsed = int(value, 10)
    except ValueError:
        return None
    if parsed <= 0:
        return None
    return parsed


def distcc_slots(distcc_hosts: str) -> int:
    """Best-effort sum of /N slots in DISTCC_HOSTS.

    DISTCC_HOSTS is a whitespace-separated list of:
      host[/N][,options] ...

    We only sum explicitly provided /N slots (ignores hosts without /N).
    """
    total = 0
    for token in (distcc_hosts or "").split():
        token = token.strip()
        if not token:
            continue
        if token.startswith("-"):
            # e.g. --randomize / --localslots=N
            continue

        host = token.split(",", 1)[0].lstrip("@").strip()
        if not host:
            continue
        if "/" not in host:
            continue

        _, _, slots_s = host.rpartition("/")
        slots = _parse_positive_int(slots_s)
        if slots:
            total += slots
    return total


def resolve_jobs(
    jobs_arg: int | None,
    env: Mapping[str, str] | None = None,
    *,
    distcc_enabled: bool = False,
) -> tuple[int, str]:
    """Resolve build parallelism.

    Precedence:
      1) explicit CLI -j/--jobs
      2) DEVSTACK_JOBS
      3) DISTCC_HOSTS slot sum (when distcc_enabled)
      4) os.cpu_count()

    Returns: (jobs, source) where source is one of: cli|env|distcc|cpu.
    """
    if jobs_arg is not None:
        if jobs_arg <= 0:
            raise ValueError("--jobs/-j must be a positive integer")
        return jobs_arg, "cli"

    env_map: Mapping[str, str] = env or os.environ
    override = _parse_positive_int(env_map.get("DEVSTACK_JOBS", ""))
    if override:
        return override, "env"

    if distcc_enabled:
        slots = distcc_slots(env_map.get("DISTCC_HOSTS", ""))
        if slots > 0:
            return slots, "distcc"

    cpu = os.cpu_count() or 1
    return max(1, int(cpu)), "cpu"

