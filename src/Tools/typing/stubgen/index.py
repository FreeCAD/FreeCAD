"""Package index lookups used to assign the stub package's patch number.

The patch number counts published releases rather than build attempts, so it comes from the
index the package is published to rather than from any CI counter.
"""

from __future__ import annotations

import json
import re
import urllib.error
import urllib.request

DEFAULT_INDEX_URL = "https://pypi.org/simple/"
SIMPLE_JSON_ACCEPT = "application/vnd.pypi.simple.v1+json"
RELEASE_VERSION = re.compile(r"^(\d+)\.(\d+)\.(\d+)$")


def published_versions(
    package_name: str,
    index_url: str = DEFAULT_INDEX_URL,
    *,
    timeout: float = 30.0,
) -> list[str]:
    """Fetch every version of @p package_name the index knows about.

    An unknown package is not an error: it means nothing has been published yet.
    """
    url = f"{index_url.rstrip('/')}/{package_name}/"
    request = urllib.request.Request(
        url,
        headers={"Accept": SIMPLE_JSON_ACCEPT, "Cache-Control": "no-cache"},
    )
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            payload = json.load(response)
    except urllib.error.HTTPError as error:
        if error.code == 404:
            return []
        raise
    return payload.get("versions", [])


def next_patch_number(major: int, minor: int, versions: list[str]) -> int:
    """One past the highest patch published on the given major.minor line, or 0 if it is new.

    Only plain three-part releases count, so a pre-release or dev upload can never advance the
    line it is previewing, and a deleted release leaves a gap that is skipped rather than reused.
    """
    patches = []
    for version in versions:
        match = RELEASE_VERSION.match(version)
        if match and (int(match[1]), int(match[2])) == (major, minor):
            patches.append(int(match[3]))
    return max(patches) + 1 if patches else 0
