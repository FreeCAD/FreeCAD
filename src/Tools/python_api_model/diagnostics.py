# pyright: strict

"""Structured diagnostics for Python API model construction and resolution.

This module deliberately contains no StubGen, filesystem, or output-specific
validation. It defines the diagnostic vocabulary used while combining public
API declarations from multiple input layers.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Literal

from .model import ApiOrigin, ApiSourceLocation

DiagnosticSeverity = Literal["warning", "error"]
DiagnosticCode = Literal[
    "conflicting-definition",
    "lower-precedence-definition",
]


@dataclass(frozen=True)
class MergeDiagnostic:
    """One API model construction or resolution finding."""

    code: DiagnosticCode
    message: str
    severity: DiagnosticSeverity = "error"
    symbol: str | None = None
    location: ApiSourceLocation | None = None


@dataclass(frozen=True)
class MergeDiagnostics:
    """Immutable collection of API model construction or resolution findings."""

    items: tuple[MergeDiagnostic, ...] = ()

    @property
    def errors(self) -> tuple[MergeDiagnostic, ...]:
        return tuple(item for item in self.items if item.severity == "error")

    @property
    def warnings(self) -> tuple[MergeDiagnostic, ...]:
        return tuple(item for item in self.items if item.severity == "warning")

    def render(self) -> str:
        """Render findings for command-line output."""

        lines: list[str] = []
        for item in self.items:
            location = ""
            if item.location is not None:
                line = f":{item.location.line}" if item.location.line is not None else ""
                location = f" [{item.location.path}{line}]"
            lines.append(f"{item.severity}: {item.code}: {item.message}{location}")
        return "\n".join(lines)


# Curated declarations intentionally refine the generated binding inventory.
ORIGIN_PRECEDENCE: dict[ApiOrigin, int] = {
    ApiOrigin.GENERATED: 10,
    ApiOrigin.BINDING_SPEC: 20,
    ApiOrigin.MODULE_STUB: 30,
    ApiOrigin.TYPE_STUB: 30,
    ApiOrigin.OVERLAY: 40,
}


def origin_precedence(origin: ApiOrigin) -> int:
    """Return the merge precedence for one API declaration origin."""

    return ORIGIN_PRECEDENCE[origin]
