# SPDX-License-Identifier: LGPL-2.1-or-later

"""Discover and validate Python API deprecations without importing FreeCAD."""

from __future__ import annotations

import ast
from dataclasses import asdict
from pathlib import Path
import re
from typing import Iterable
import warnings

from .model import DeprecationRecord, Diagnostic, ScanResult

SCHEMA_VERSION = 1
STRUCTURED_FIELDS = {"deprecated_in", "removed_in", "replacement", "details"}
RELEASE_RE = re.compile(r"^\d+\.\d+(?:\.\d+)?$")
LEGACY_DOC_RE = re.compile(r"(?i)\bdeprecated\b\s*(?::+|--|-)\s*(.*)")
SKIPPED_PARTS = {"3rdParty", "SCL_output", "generated", "tests", "__pycache__"}


def _decorator_name(decorator: ast.expr) -> str:
    target = decorator.func if isinstance(decorator, ast.Call) else decorator
    return ast.unparse(target).rsplit(".", 1)[-1]


def _literal_keywords(call: ast.Call) -> tuple[dict[str, object], list[str]]:
    values: dict[str, object] = {}
    errors: list[str] = []
    for keyword in call.keywords:
        if keyword.arg is None:
            errors.append("keyword unpacking is not supported")
            continue
        try:
            values[keyword.arg] = ast.literal_eval(keyword.value)
        except (ValueError, SyntaxError):
            errors.append(f"'{keyword.arg}' must be a literal value")
    return values, errors


def _release_key(value: str) -> tuple[int, int, int]:
    parts = [int(part) for part in value.split(".")]
    normalized = [*parts, *([0] * (3 - len(parts)))]
    return normalized[0], normalized[1], normalized[2]


def _record_sort_key(record: DeprecationRecord) -> tuple[object, ...]:
    return (
        record.symbol,
        record.kind,
        record.deprecated_in,
        record.removed_in,
        record.source,
        record.line,
    )


def _module_name(path: Path, source_root: Path) -> str:
    if path.name.endswith(".module.pyi"):
        return path.name.removesuffix(".module.pyi")

    rel = path.relative_to(source_root)
    parts = rel.parts
    if parts[0] == "Mod" and len(parts) >= 3:
        module_parts = list(parts[2:])
    else:
        module_parts = list(parts[1:]) if parts[0] in {"App", "Base", "Gui"} else list(parts)

    filename = module_parts.pop()
    if filename not in {"__init__.py", "__init__.pyi"}:
        module_parts.append(filename.rsplit(".", 1)[0])
    return ".".join(module_parts)


def _binding_module(path: Path, source_root: Path) -> str:
    rel = path.relative_to(source_root)
    if rel.parts[0] == "Mod" and len(rel.parts) >= 2:
        return rel.parts[1]
    if rel.parts[0] == "Gui":
        return "FreeCADGui"
    if rel.parts[0] in {"App", "Base"}:
        return "FreeCAD"
    return _module_name(path, source_root)


def _class_public_name(node: ast.ClassDef, path: Path, source_root: Path) -> str:
    for decorator in node.decorator_list:
        if _decorator_name(decorator) != "export" or not isinstance(decorator, ast.Call):
            continue
        values, _ = _literal_keywords(decorator)
        python_name = values.get("PythonName")
        if isinstance(python_name, str) and python_name:
            return python_name
        name = values.get("Name")
        if isinstance(name, str) and name:
            return f"{_binding_module(path, source_root)}.{name}"
    return f"{_binding_module(path, source_root)}.{node.name}"


def _structured_record(
    *,
    symbol: str,
    kind: str,
    values: dict[str, object],
    source: str,
    line: int,
) -> tuple[DeprecationRecord | None, list[Diagnostic]]:
    diagnostics: list[Diagnostic] = []
    unknown = sorted(values.keys() - STRUCTURED_FIELDS)
    if unknown:
        diagnostics.append(
            Diagnostic(source, line, "error", f"unknown deprecation field '{unknown[0]}'")
        )

    deprecated_in = values.get("deprecated_in")
    removed_in = values.get("removed_in")
    replacement = values.get("replacement")
    details = values.get("details")
    for field, value in (("deprecated_in", deprecated_in), ("removed_in", removed_in)):
        if not isinstance(value, str) or not value:
            diagnostics.append(
                Diagnostic(source, line, "error", f"{field} must be a non-empty string")
            )
    for field, value in (("replacement", replacement), ("details", details)):
        if value is not None and not isinstance(value, str):
            diagnostics.append(
                Diagnostic(source, line, "error", f"{field} must be a string or null")
            )

    if diagnostics:
        return None, diagnostics
    assert isinstance(deprecated_in, str)
    assert isinstance(removed_in, str)
    assert replacement is None or isinstance(replacement, str)
    assert details is None or isinstance(details, str)
    for field, value in (("deprecated_in", deprecated_in), ("removed_in", removed_in)):
        if not RELEASE_RE.fullmatch(value):
            diagnostics.append(
                Diagnostic(source, line, "error", f"{field} '{value}' is not a valid release")
            )
    if not diagnostics and _release_key(removed_in) <= _release_key(deprecated_in):
        diagnostics.append(
            Diagnostic(source, line, "error", "removed_in must be later than deprecated_in")
        )

    return (
        DeprecationRecord(
            symbol=symbol,
            kind=kind,
            deprecated_in=deprecated_in,
            removed_in=removed_in,
            replacement=replacement,
            details=details,
            source=source,
            line=line,
        ),
        diagnostics,
    )


class _FileScanner(ast.NodeVisitor):
    def __init__(
        self,
        path: Path,
        source_root: Path,
    ):
        self.path = path
        self.source_root = source_root
        self.source = path.relative_to(source_root.parent).as_posix()
        self.module = _module_name(path, source_root)
        self.scope: list[str] = []
        self.records: list[DeprecationRecord] = []
        self.diagnostics: list[Diagnostic] = []

    def _symbol(self, name: str) -> str:
        return ".".join(part for part in (self.module, *self.scope, name) if part)

    def _scan_decorators(
        self,
        node: ast.FunctionDef | ast.AsyncFunctionDef | ast.ClassDef,
        symbol_override: str | None = None,
    ) -> bool:
        symbol = symbol_override or self._symbol(node.name)
        kind = "class" if isinstance(node, ast.ClassDef) else "method" if self.scope else "function"
        for decorator in node.decorator_list:
            if _decorator_name(decorator) != "deprecated":
                continue
            if not isinstance(decorator, ast.Call):
                self.diagnostics.append(
                    Diagnostic(self.source, decorator.lineno, "error", "deprecated must be called")
                )
                return True
            if decorator.args:
                self.diagnostics.append(
                    Diagnostic(
                        self.source,
                        decorator.lineno,
                        "error",
                        "deprecated() requires structured keyword-only lifecycle metadata",
                    )
                )
                return True
            values, errors = _literal_keywords(decorator)
            self.diagnostics.extend(
                Diagnostic(self.source, decorator.lineno, "error", error) for error in errors
            )
            record, diagnostics = _structured_record(
                symbol=symbol,
                kind=kind,
                values=values,
                source=self.source,
                line=decorator.lineno,
            )
            if record:
                self.records.append(record)
            self.diagnostics.extend(diagnostics)
            return True
        return False

    def _scan_docstring(
        self,
        node: ast.FunctionDef | ast.AsyncFunctionDef | ast.ClassDef,
        symbol_override: str | None = None,
    ) -> None:
        doc = ast.get_docstring(node, clean=False) or ""
        if LEGACY_DOC_RE.search(doc):
            symbol = symbol_override or self._symbol(node.name)
            self.diagnostics.append(
                Diagnostic(
                    self.source,
                    node.lineno,
                    "error",
                    f"{symbol} uses docstring-only deprecation metadata",
                )
            )

    def _scan_attributes(self, node: ast.ClassDef, class_symbol: str) -> None:
        for decorator in node.decorator_list:
            if _decorator_name(decorator) != "deprecated_attributes" or not isinstance(
                decorator, ast.Call
            ):
                continue
            values, errors = _literal_keywords(decorator)
            self.diagnostics.extend(
                Diagnostic(self.source, decorator.lineno, "error", error) for error in errors
            )
            for name, value in values.items():
                symbol = f"{class_symbol}.{name}"
                if isinstance(value, dict):
                    record, diagnostics = _structured_record(
                        symbol=symbol,
                        kind="attribute",
                        values=value,
                        source=self.source,
                        line=decorator.lineno,
                    )
                    if record:
                        self.records.append(record)
                    self.diagnostics.extend(diagnostics)
                else:
                    self.diagnostics.append(
                        Diagnostic(
                            self.source,
                            decorator.lineno,
                            "error",
                            f"{symbol} metadata must be a structured mapping",
                        )
                    )

    def visit_ClassDef(self, node: ast.ClassDef) -> None:
        if (
            not self.scope
            and self.path.suffix == ".pyi"
            and not self.path.name.endswith(".module.pyi")
        ):
            public_name = _class_public_name(node, self.path, self.source_root)
            previous_module = self.module
            self.module, _, class_name = public_name.rpartition(".")
            deprecated = self._scan_decorators(node, public_name)
            if not deprecated:
                self._scan_docstring(node, public_name)
            self._scan_attributes(node, public_name)
            self.scope.append(class_name)
            for child in node.body:
                self.visit(child)
            self.scope.pop()
            self.module = previous_module
            return

        deprecated = self._scan_decorators(node)
        if not deprecated:
            self._scan_docstring(node)
        class_symbol = self._symbol(node.name)
        self._scan_attributes(node, class_symbol)
        self.scope.append(node.name)
        for child in node.body:
            self.visit(child)
        self.scope.pop()

    def visit_FunctionDef(self, node: ast.FunctionDef) -> None:
        if not self._scan_decorators(node):
            self._scan_docstring(node)

    def visit_AsyncFunctionDef(self, node: ast.AsyncFunctionDef) -> None:
        if not self._scan_decorators(node):
            self._scan_docstring(node)


def _source_paths(source_root: Path) -> Iterable[Path]:
    for path in source_root.rglob("*"):
        if not path.is_file() or not (path.suffix == ".py" or path.suffix == ".pyi"):
            continue
        rel = path.relative_to(source_root)
        if SKIPPED_PARTS.intersection(rel.parts) or rel.parts[0] == "Tools":
            continue
        yield path


def scan_repository(root: Path) -> ScanResult:
    source_root = root / "src"
    records: list[DeprecationRecord] = []
    diagnostics: list[Diagnostic] = []
    for path in sorted(_source_paths(source_root)):
        try:
            with warnings.catch_warnings():
                warnings.simplefilter("ignore", SyntaxWarning)
                tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        except (OSError, SyntaxError) as error:
            diagnostics.append(
                Diagnostic(
                    path.relative_to(root).as_posix(),
                    getattr(error, "lineno", 1) or 1,
                    "error",
                    f"cannot parse source: {error}",
                )
            )
            continue
        scanner = _FileScanner(path, source_root)
        scanner.visit(tree)
        records.extend(scanner.records)
        diagnostics.extend(scanner.diagnostics)

    unique: dict[tuple[object, ...], DeprecationRecord] = {}
    by_symbol: dict[str, set[tuple[object, ...]]] = {}
    for record in records:
        identity = (
            record.symbol,
            record.kind,
            record.deprecated_in,
            record.removed_in,
            record.replacement,
            record.details,
            record.source,
        )
        existing = unique.get(identity)
        if existing is None or record.line < existing.line:
            unique[identity] = record
        lifecycle = (
            record.deprecated_in,
            record.removed_in,
            record.replacement,
            record.details,
        )
        by_symbol.setdefault(record.symbol, set()).add(lifecycle)
    for symbol, lifecycles in by_symbol.items():
        if len(lifecycles) > 1:
            first = min(
                (record for record in records if record.symbol == symbol),
                key=lambda record: (record.source, record.line),
            )
            diagnostics.append(
                Diagnostic(first.source, first.line, "error", f"conflicting metadata for {symbol}")
            )

    return ScanResult(
        records=tuple(sorted(unique.values(), key=_record_sort_key)),
        diagnostics=tuple(sorted(set(diagnostics))),
    )


def manifest(result: ScanResult) -> dict[str, object]:
    return {
        "schema_version": SCHEMA_VERSION,
        "deprecations": [asdict(record) for record in result.records],
    }
