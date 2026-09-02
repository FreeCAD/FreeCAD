# SPDX-License-Identifier: LGPL-2.1-or-later

"""IfcOpenShell loading and capability detection for FreeCAD BIM.

This module intentionally has no FreeCAD dependency. It is the runtime
boundary between BIM code and the native IfcOpenShell Python package and can
therefore be exercised in isolation, including when IfcOpenShell is absent.
"""

from dataclasses import dataclass
import importlib
from types import ModuleType

REQUIRED_MODULES = (
    "ifcopenshell",
    "ifcopenshell.api",
    "ifcopenshell.geom",
    "ifcopenshell.entity_instance",
    "ifcopenshell.util.attribute",
    "ifcopenshell.util.element",
    "ifcopenshell.util.placement",
    "ifcopenshell.util.schema",
    "ifcopenshell.util.unit",
)


class IfcOpenShellUnavailable(ImportError):
    """Raised when FreeCAD's required IfcOpenShell surface cannot be loaded."""


@dataclass(frozen=True)
class BackendStatus:
    """Result of validating the installed IfcOpenShell backend."""

    available: bool
    version: str = ""
    geometry_iterator: bool = False
    serialized_brep: bool = False
    schema_names: bool = False
    error: str = ""


_module: ModuleType | None = None
_status: BackendStatus | None = None


def _version(module: ModuleType) -> str:
    """Return a normalized display version across IfcOpenShell releases."""

    return str(getattr(module, "version", getattr(module, "__version__", "")))


def _load() -> tuple[ModuleType, BackendStatus]:
    modules = {name: importlib.import_module(name) for name in REQUIRED_MODULES}
    module = modules["ifcopenshell"]
    geom = modules["ifcopenshell.geom"]
    api = modules["ifcopenshell.api"]
    wrapper = getattr(module, "ifcopenshell_wrapper", None)

    required_callables = {
        "ifcopenshell.open": getattr(module, "open", None),
        "ifcopenshell.file": getattr(module, "file", None),
        "ifcopenshell.api.run": getattr(api, "run", None),
        "ifcopenshell.geom.iterator": getattr(geom, "iterator", None),
    }
    missing = [name for name, value in required_callables.items() if not callable(value)]
    if missing:
        raise IfcOpenShellUnavailable("missing required API: " + ", ".join(missing))

    status = BackendStatus(
        available=True,
        version=_version(module),
        geometry_iterator=True,
        serialized_brep=wrapper is not None and hasattr(wrapper, "SERIALIZED"),
        schema_names=wrapper is not None and callable(getattr(wrapper, "schema_names", None)),
    )
    return module, status


def get_status(refresh: bool = False) -> BackendStatus:
    """Validate and describe the backend without raising import errors."""

    global _module, _status
    if refresh:
        invalidate()
    if _status is None:
        try:
            _module, _status = _load()
        except Exception as exc:
            _module = None
            _status = BackendStatus(
                available=False,
                error=f"{type(exc).__name__}: {exc}",
            )
    return _status


def get_backend(refresh: bool = False) -> ModuleType:
    """Return a validated IfcOpenShell module or raise a stable exception."""

    status = get_status(refresh=refresh)
    if not status.available or _module is None:
        detail = f" ({status.error})" if status.error else ""
        raise IfcOpenShellUnavailable(f"IfcOpenShell is unavailable{detail}")
    return _module


def invalidate() -> None:
    """Forget cached validation after the Python package environment changes."""

    global _module, _status
    _module = None
    _status = None
