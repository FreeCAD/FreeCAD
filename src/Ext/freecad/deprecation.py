# SPDX-License-Identifier: LGPL-2.1-or-later

"""Python API deprecation helpers."""

from __future__ import annotations

from collections.abc import Callable
import re
from types import CodeType, FrameType
from typing import TypeVar

from typing_extensions import deprecated as _pep702_deprecated

_T = TypeVar("_T")
_RELEASE_RE = re.compile(r"^\d+\.\d+(?:\.\d+)?$")
_WRAPPER_CODES: set[CodeType] = set()


def _unwrap_deprecated_frame(frame: FrameType | None) -> FrameType | None:
    """Return the first frame outside FreeCAD's deprecation wrappers."""
    while frame is not None and frame.f_code in _WRAPPER_CODES:
        frame = frame.f_back
    return frame


def _release_key(value: str) -> tuple[int, int, int]:
    parts = [int(part) for part in value.split(".")]
    normalized = [*parts, *([0] * (3 - len(parts)))]
    return normalized[0], normalized[1], normalized[2]


def _validate_lifecycle(deprecated_in: str, removed_in: str) -> None:
    for field, value in (("deprecated_in", deprecated_in), ("removed_in", removed_in)):
        if not isinstance(value, str) or not value:
            raise ValueError(f"{field} must be a non-empty string")
        if not _RELEASE_RE.fullmatch(value):
            raise ValueError(f"{field} must be a release such as '26.3'")
    if _release_key(removed_in) <= _release_key(deprecated_in):
        raise ValueError("removed_in must be later than deprecated_in")


def _message(
    qualified_name: str,
    deprecated_in: str,
    removed_in: str,
    replacement: str | None,
    details: str | None,
) -> str:
    message = (
        f"{qualified_name} is deprecated since FreeCAD {deprecated_in} "
        f"and will be removed in FreeCAD {removed_in}"
    )
    if replacement:
        message += f"; use {replacement} instead"
    if details:
        message += f"; {details.rstrip()}"
    return message if message.endswith((".", "!", "?")) else message + "."


def deprecated(
    *,
    deprecated_in: str,
    removed_in: str,
    replacement: str | None = None,
    details: str | None = None,
) -> Callable[[_T], _T]:
    """Mark a Python API for removal in a future FreeCAD release."""

    _validate_lifecycle(deprecated_in, removed_in)
    for field, value in (("replacement", replacement), ("details", details)):
        if value is not None and not isinstance(value, str):
            raise TypeError(f"{field} must be a string or None")

    def decorate(obj: _T) -> _T:
        module = getattr(obj, "__module__", "")
        name = getattr(obj, "__qualname__", getattr(obj, "__name__", "API"))
        qualified_name = f"{module}.{name}" if module else name
        message = _message(qualified_name, deprecated_in, removed_in, replacement, details)
        decorated = _pep702_deprecated(message)(obj)
        wrapper_code = getattr(decorated, "__code__", None)
        if isinstance(wrapper_code, CodeType):
            _WRAPPER_CODES.add(wrapper_code)
        return decorated

    return decorate
