# pyright: strict

"""Normalized public Python API model shared by output renderers.

The binding inventory and curated ``.pyi`` sources should converge into one
structured public API model before they are rendered into specific outputs.
This module defines that shared semantic model. It contains public declarations
only; stub-only imports and helper declarations belong to the output layer.

Long term, both of these should render from the same data:

- merged public ``.pyi`` stubs
- generated Markdown/MDX documentation

Keeping this model neutral avoids coupling documentation generation to the
legacy binding-generator schema or to the final merged stub text format.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum

from .signatures import CallableSignature


class ApiOrigin(str, Enum):
    """Provenance for authored or synthesized public API entries."""

    BINDING_SPEC = "binding_spec"
    MODULE_STUB = "module_stub"
    TYPE_STUB = "type_stub"
    GENERATED = "generated"
    OVERLAY = "overlay"


@dataclass(frozen=True)
class ApiSourceLocation:
    """Repository location for one API declaration or documentation source."""

    path: str
    line: int | None = None


@dataclass(frozen=True)
class ApiAttribute:
    """One documented module or class attribute."""

    name: str
    annotation: str | None = None
    value: str | None = None
    doc: str | None = None
    origin: ApiOrigin = ApiOrigin.GENERATED
    location: ApiSourceLocation | None = None


@dataclass(frozen=True)
class ApiAlias:
    """One alternate public export path for a canonical API symbol."""

    public_path: str
    target_path: str
    origin: ApiOrigin = ApiOrigin.GENERATED
    location: ApiSourceLocation | None = None


@dataclass(frozen=True)
class ApiCallableGroup:
    """One function or method name with all overloads in source order."""

    name: str
    signatures: tuple[CallableSignature, ...]
    doc: str | None = None
    origin: ApiOrigin = ApiOrigin.GENERATED
    location: ApiSourceLocation | None = None

    @property
    def overload(self) -> bool:
        if any(
            signature.flags.property_getter or signature.flags.property_setter
            for signature in self.signatures
        ):
            return False
        return len(self.signatures) > 1 or any(
            signature.flags.overload for signature in self.signatures
        )


@dataclass(frozen=True)
class ApiClass:
    """One public class in its canonical module placement."""

    module_name: str
    name: str
    doc: str | None = None
    bases: tuple[str, ...] = ()
    methods: tuple[ApiCallableGroup, ...] = ()
    attributes: tuple[ApiAttribute, ...] = ()
    decorators: tuple[str, ...] = ()
    origin: ApiOrigin = ApiOrigin.GENERATED
    location: ApiSourceLocation | None = None

    @property
    def qualified_name(self) -> str:
        return f"{self.module_name}.{self.name}"


@dataclass(frozen=True)
class ApiModule:
    """One public importable module page in the final Python API surface."""

    name: str
    doc: str | None = None
    functions: tuple[ApiCallableGroup, ...] = ()
    classes: tuple[ApiClass, ...] = ()
    attributes: tuple[ApiAttribute, ...] = ()
    aliases: tuple[ApiAlias, ...] = ()
    origin: ApiOrigin = ApiOrigin.GENERATED
    location: ApiSourceLocation | None = None


@dataclass(frozen=True)
class PythonApiModel:
    """Normalized public Python API consumed by output renderers."""

    modules: tuple[ApiModule, ...] = field(default_factory=tuple)
