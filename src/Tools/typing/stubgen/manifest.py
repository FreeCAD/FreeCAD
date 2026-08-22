# pyright: strict

"""Write deterministic API-generation manifests for CI observability."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Mapping

MANIFEST_SCHEMA_VERSION = 1


def write_api_manifest(
    path: Path,
    *,
    generator: str,
    pages: int,
    counts: Mapping[str, int],
) -> None:
    """Write one stable summary of a generated API surface."""

    manifest: dict[str, int | str] = {
        "generator": generator,
        "schemaVersion": MANIFEST_SCHEMA_VERSION,
        "pages": pages,
    }
    manifest.update(dict(sorted(counts.items())))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
