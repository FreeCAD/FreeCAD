# pyright: strict

"""Neutral C++ API model extracted from Doxygen XML.

The Python API docs are authored from curated ``.pyi`` files. The C++ API docs
need a different upstream source, and Doxygen XML is the most stable one
already present in the repository.

This module defines a small semantic model for that XML so the documentation
renderer does not have to depend on Doxygen's raw element layout.
"""

from __future__ import annotations

from dataclasses import dataclass, field


@dataclass(frozen=True)
class CppSourceLocation:
    """Repository location for one documented C++ declaration."""

    path: str
    line: int | None = None


@dataclass(frozen=True)
class CppApiFunction:
    """One documented C++ function or method signature."""

    name: str
    declaration: str
    doc: str | None = None
    location: CppSourceLocation | None = None


@dataclass(frozen=True)
class CppApiEnumValue:
    """One enum constant in declaration order."""

    name: str
    initializer: str | None = None
    doc: str | None = None


@dataclass(frozen=True)
class CppApiEnum:
    """One documented C++ enum."""

    name: str
    declaration: str
    doc: str | None = None
    values: tuple[CppApiEnumValue, ...] = ()
    location: CppSourceLocation | None = None


@dataclass(frozen=True)
class CppApiNamespace:
    """One documented namespace page."""

    qualified_name: str
    name: str
    doc: str | None = None
    functions: tuple[CppApiFunction, ...] = ()
    enums: tuple[CppApiEnum, ...] = ()
    location: CppSourceLocation | None = None

    @property
    def parent_name(self) -> str | None:
        if "::" not in self.qualified_name:
            return None
        return self.qualified_name.rsplit("::", 1)[0]


@dataclass(frozen=True)
class CppApiClass:
    """One documented class or struct page."""

    qualified_name: str
    name: str
    display_name: str
    namespace_name: str
    top_namespace: str
    kind: str
    doc: str | None = None
    bases: tuple[str, ...] = ()
    constructors: tuple[CppApiFunction, ...] = ()
    destructor: CppApiFunction | None = None
    methods: tuple[CppApiFunction, ...] = ()
    enums: tuple[CppApiEnum, ...] = ()
    location: CppSourceLocation | None = None


@dataclass(frozen=True)
class CppApiModel:
    """The complete extracted C++ API model for one generated docs run."""

    namespaces: tuple[CppApiNamespace, ...] = field(default_factory=tuple)
    classes: tuple[CppApiClass, ...] = field(default_factory=tuple)
