# pyright: strict

"""Structured diagnostics for Python API model and stub merge validation.

The stub pipeline combines generated binding information with several curated
``.pyi`` layers. This module defines the small diagnostic vocabulary used to
make that merge observable without changing the rendered output policy.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Literal

from .model import BindingClass
from .python_api.model import ApiModel, ApiOrigin, ApiSourceLocation

DiagnosticSeverity = Literal["warning", "error"]


@dataclass(frozen=True)
class MergeDiagnostic:
    """One merge or generated-output validation finding."""

    code: str
    message: str
    severity: DiagnosticSeverity = "error"
    symbol: str | None = None
    location: ApiSourceLocation | None = None


@dataclass(frozen=True)
class MergeDiagnostics:
    """Immutable collection of findings produced by one generation run."""

    items: tuple[MergeDiagnostic, ...] = ()

    @property
    def errors(self) -> tuple[MergeDiagnostic, ...]:
        return tuple(item for item in self.items if item.severity == "error")

    @property
    def warnings(self) -> tuple[MergeDiagnostic, ...]:
        return tuple(item for item in self.items if item.severity == "warning")

    def render(self) -> str:
        """Render findings for command-line output."""

        lines = [f"{item.severity}: {item.code}: {item.message}" for item in self.items]
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


def generated_output_diagnostics(
    target_dir: Path,
    model: ApiModel,
    module_names: set[str],
) -> tuple[MergeDiagnostic, ...]:
    """Verify that public model declarations are present in generated modules."""

    from .module_merge import module_stub_path, public_stub_symbols

    model_symbols: set[str] = set()
    for module in model.modules:
        model_symbols.update(
            f"{module.name}.{symbol}"
            for symbol in (
                [function.name for function in module.functions]
                + [klass.name for klass in module.classes]
                + [attribute.name for attribute in module.attributes]
            )
        )
        model_symbols.update(alias.public_path for alias in module.aliases)

    findings: list[MergeDiagnostic] = []
    for module in model.modules:
        path = module_stub_path(target_dir, module.name, module_names)
        if not path.exists():
            findings.append(
                MergeDiagnostic(
                    code="missing-module-output",
                    message=f"curated module {module.name} has no generated stub",
                    symbol=module.name,
                    location=module.location,
                )
            )
            continue
        symbols = public_stub_symbols(path.read_text(encoding="utf-8"))
        declarations: list[tuple[str, ApiSourceLocation | None]] = [
            *[(function.name, function.location) for function in module.functions],
            *[(klass.name, klass.location) for klass in module.classes],
            *[(attribute.name, attribute.location) for attribute in module.attributes],
            *[(alias.public_path.rsplit(".", 1)[-1], alias.location) for alias in module.aliases],
        ]
        for symbol, location in declarations:
            if symbol in symbols:
                continue
            qualified_name = f"{module.name}.{symbol}"
            findings.append(
                MergeDiagnostic(
                    code="missing-declaration-output",
                    message=f"curated declaration {qualified_name} is absent from generated stub",
                    symbol=qualified_name,
                    location=location,
                )
            )

    for module in model.modules:
        for alias in module.aliases:
            if alias.target_path in model_symbols:
                continue
            findings.append(
                MergeDiagnostic(
                    code="unresolved-alias",
                    message=f"alias {alias.public_path} targets missing {alias.target_path}",
                    symbol=alias.public_path,
                    location=alias.location,
                )
            )
    return tuple(findings)


def discovered_model_diagnostics(
    classes: list[BindingClass],
    model: ApiModel,
) -> tuple[MergeDiagnostic, ...]:
    """Report public binding classes not yet represented by the API model."""

    model_symbols: set[str] = set()
    for module in model.modules:
        model_symbols.update(f"{module.name}.{api_class.name}" for api_class in module.classes)
        model_symbols.update(alias.public_path for alias in module.aliases)

    findings: list[MergeDiagnostic] = []
    for binding_class in classes:
        for public_name in binding_class.public_names:
            if "." not in public_name or public_name in model_symbols:
                continue
            findings.append(
                MergeDiagnostic(
                    code="unmodeled-discovered-symbol",
                    severity="warning",
                    message=f"discovered public class {public_name} is not represented by ApiModel",
                    symbol=public_name,
                    location=ApiSourceLocation(binding_class.source, binding_class.line),
                )
            )
    return tuple(findings)
