# SPDX-License-Identifier: LGPL-2.1-or-later

"""Normalization helpers for structured Python API deprecations."""

import ast
import re

DEPRECATION_FIELDS = {"deprecated_in", "removed_in", "replacement", "details"}
RELEASE_RE = re.compile(r"^\d+\.\d+(?:\.\d+)?$")


def release_key(value: str) -> tuple[int, int, int]:
    parts = [int(part) for part in value.split(".")]
    normalized = [*parts, *([0] * (3 - len(parts)))]
    return normalized[0], normalized[1], normalized[2]


def literal_keyword_values(decorator: ast.Call, metadata_name: str) -> dict[str, object]:
    values: dict[str, object] = {}
    for keyword in decorator.keywords:
        if keyword.arg is None:
            raise ValueError(f"{metadata_name} does not support keyword unpacking")
        try:
            values[keyword.arg] = ast.literal_eval(keyword.value)
        except (ValueError, SyntaxError) as error:
            raise ValueError(f"{metadata_name} '{keyword.arg}' must be a literal value") from error
    return values


def structured_deprecation_message(values: dict[str, object]) -> str | None:
    if not values:
        return None

    unknown = values.keys() - DEPRECATION_FIELDS
    if unknown:
        raise ValueError(f"deprecated() got unknown keyword '{sorted(unknown)[0]}'")

    deprecated_in = values.get("deprecated_in")
    removed_in = values.get("removed_in")
    if not isinstance(deprecated_in, str) or not deprecated_in:
        raise ValueError("deprecated() requires a non-empty deprecated_in string")
    if not isinstance(removed_in, str) or not removed_in:
        raise ValueError("deprecated() requires a non-empty removed_in string")
    for field, value in (("deprecated_in", deprecated_in), ("removed_in", removed_in)):
        if not RELEASE_RE.fullmatch(value):
            raise ValueError(f"deprecated() {field} must be a release such as '26.3'")
    if release_key(removed_in) <= release_key(deprecated_in):
        raise ValueError("deprecated() removed_in must be later than deprecated_in")

    message = f"since FreeCAD {deprecated_in} and will be removed in FreeCAD {removed_in}"
    for field in ("replacement", "details"):
        value = values.get(field)
        if value is not None and not isinstance(value, str):
            raise ValueError(f"deprecated() {field} must be a string or None")
    if replacement := values.get("replacement"):
        message += f"; use {replacement} instead"
    if details := values.get("details"):
        message += f"; {details.rstrip()}"
    return message if message.endswith((".", "!", "?")) else message + "."
