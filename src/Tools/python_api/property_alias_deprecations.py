# SPDX-License-Identifier: LGPL-2.1-or-later

"""Discover deprecated C++ property aliases without importing FreeCAD.

The repository inventory already has a Python AST scanner.  C++ aliases need a
different, deliberately small lexer because importing or fully parsing the C++
tree would make this source-only tool much heavier.  This module owns that
C++-specific work and returns the same model as the Python scanner.

The lexer is conservative: comments and literals are masked before searching
for the macro, and malformed-looking invocations become diagnostics instead
of guessed records.  Lifecycle validation is supplied by the coordinator so
both Python declarations and C++ aliases continue to use one schema and one
set of release rules.
"""

from __future__ import annotations

import ast
from collections.abc import Callable, Iterable
from dataclasses import dataclass
from pathlib import Path
import re

from .model import DeprecationRecord, Diagnostic, ScanResult

CPP_SUFFIXES = {".cpp", ".h", ".hpp"}
PROPERTY_ALIAS_MACRO = "ADD_PROPERTY_DEPRECATED_ALIAS"
SKIPPED_PARTS = {"3rdParty", "SCL_output", "generated", "tests", "__pycache__"}
StructuredRecordBuilder = Callable[..., tuple[DeprecationRecord | None, list[Diagnostic]]]


@dataclass(frozen=True)
class _CppMacroCall:
    offset: int
    line: int
    arguments: tuple[str, ...]


def _mask_cpp_noncode(source: str) -> str:
    """Mask non-code regions while preserving offsets and newlines.

    Keeping the original length means macro offsets and line numbers still refer
    to the source file after masking.  Newlines remain visible for diagnostics;
    every other character in comments and literals becomes a space so braces,
    commas, and macro names inside them cannot affect later passes.
    """
    masked = list(source)
    index = 0
    length = len(source)

    def erase(start: int, end: int) -> None:
        for position in range(start, end):
            if masked[position] != "\n":
                masked[position] = " "

    def raw_string_end(start: int) -> int | None:
        # Raw strings may contain anything that resembles C++ syntax, including
        # braces and the deprecated-alias macro itself.  Handle all standard
        # prefixes and custom delimiters before ordinary quote masking.
        prefixes = ("u8R", "uR", "UR", "LR", "R")
        for prefix in prefixes:
            marker = f'{prefix}"'
            if not source.startswith(marker, start):
                continue
            if start and (source[start - 1].isalnum() or source[start - 1] == "_"):
                continue

            delimiter_start = start + len(marker)
            open_paren = source.find("(", delimiter_start, delimiter_start + 17)
            if open_paren < 0:
                # A malformed raw literal should suppress the rest of the file rather
                # than allow macro-looking text in it to become a false record.
                return length

            delimiter = source[delimiter_start:open_paren]
            if len(delimiter) > 16 or any(char in " ()\\\t\n\r" for char in delimiter):
                continue

            close_marker = f'){delimiter}"'
            close = source.find(close_marker, open_paren + 1)
            return length if close < 0 else close + len(close_marker)

        return None

    while index < length:
        if source.startswith("//", index):
            end = source.find("\n", index + 2)
            if end < 0:
                end = length
            erase(index, end)
            index = end
            continue

        if source.startswith("/*", index):
            end = source.find("*/", index + 2)
            end = length if end < 0 else end + 2
            erase(index, end)
            index = end
            continue

        end = raw_string_end(index)
        if end is not None:
            erase(index, end)
            index = end
            continue

        if source[index] in {'"', "'"}:
            quote = source[index]
            start = index
            index += 1
            while index < length:
                if source[index] == "\\":
                    index += 2
                    continue
                if source[index] == quote:
                    index += 1
                    break
                index += 1
            erase(start, min(index, length))
            continue

        index += 1

    return "".join(masked)


def _split_cpp_arguments(
    source: str,
    masked: str,
    start: int,
    end: int,
) -> tuple[str, ...]:
    """Split macro arguments while ignoring commas in nested expressions."""
    arguments: list[str] = []
    argument_start = start
    depth = 0

    for index in range(start, end):
        char = masked[index]
        if char in "([{":
            depth += 1
        elif char in ")]}":
            depth -= 1
        elif char == "," and depth == 0:
            arguments.append(source[argument_start:index].strip())
            argument_start = index + 1

    arguments.append(source[argument_start:end].strip())
    return tuple(arguments)


def _cpp_macro_calls(source: str, macro: str) -> Iterable[_CppMacroCall]:
    """Yield balanced invocations of *macro* found in C++ code regions."""
    masked = _mask_cpp_noncode(source)
    pattern = re.compile(rf"\b{re.escape(macro)}\b")

    for match in pattern.finditer(masked):
        line_start = source.rfind("\n", 0, match.start()) + 1
        if source[line_start : match.start()].lstrip().startswith("#"):
            continue
        index = match.end()
        while index < len(masked) and masked[index].isspace():
            index += 1
        if index >= len(masked) or masked[index] != "(":
            continue

        open_paren = index
        depth = 1
        index += 1
        while index < len(masked) and depth:
            if masked[index] == "(":
                depth += 1
            elif masked[index] == ")":
                depth -= 1
            index += 1

        if depth:
            # Let the caller report the malformed invocation using the macro line.
            yield _CppMacroCall(
                offset=match.start(),
                line=source.count("\n", 0, match.start()) + 1,
                arguments=(),
            )
            continue

        close_paren = index - 1
        yield _CppMacroCall(
            offset=match.start(),
            line=source.count("\n", 0, match.start()) + 1,
            arguments=_split_cpp_arguments(
                source,
                masked,
                open_paren + 1,
                close_paren,
            ),
        )


def _cpp_string_literal(value: str) -> str | None:
    if not re.fullmatch(r'"(?:\\.|[^"\\])*"', value):
        return None
    try:
        parsed = ast.literal_eval(value)
    except (SyntaxError, ValueError):
        return None
    return parsed if isinstance(parsed, str) else None


def _cpp_namespace_prefix(source: str, offset: int) -> tuple[str, ...]:
    """Return named namespace blocks enclosing an offset.

    A full C++ parser is unnecessary for this ownership lookup.  The masked
    source gives us enough information to track brace depth and associate each
    namespace opening brace with its matching close.  Anonymous namespaces are
    tracked for nesting but contribute no name to the result.
    """
    namespace_re = re.compile(r"\bnamespace\s*(?P<name>(?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)?\s*\{|[{}]")
    brace_depth = 0
    namespaces: list[tuple[int, tuple[str, ...]]] = []

    for match in namespace_re.finditer(source, 0, offset):
        token = match.group(0)
        if token.lstrip().startswith("namespace"):
            brace_depth += 1
            name = match.group("name")
            parts = tuple(name.split("::")) if name else ()
            namespaces.append((brace_depth, parts))
        elif token == "{":
            brace_depth += 1
        else:
            if namespaces and namespaces[-1][0] == brace_depth:
                namespaces.pop()
            brace_depth -= 1

    return tuple(part for _, parts in namespaces for part in parts)


def _cpp_enclosing_constructor(
    source: str,
    offset: int,
) -> tuple[str, tuple[str, ...]] | None:
    """Return the owning class and namespaces for the constructor containing offset.

    Property macros are normally invoked from constructors, where the property
    storage is initialized.  We identify the nearest qualified ``Type::Type``
    body and then use the namespace tracker to form the public owner symbol.
    """
    masked = _mask_cpp_noncode(source)
    constructor_re = re.compile(
        r"""
        (?P<owner>(?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)
        ::
        (?P<ctor>[A-Za-z_]\w*)
        \s*\([^;{}]*\)
        \s*(?::[^{}]*)?
        \{
        """,
        re.VERBOSE,
    )

    candidates = [
        match
        for match in constructor_re.finditer(masked, 0, offset)
        if match.group("owner").rsplit("::", 1)[-1] == match.group("ctor")
    ]

    for match in reversed(candidates):
        body_start = match.end() - 1
        depth = 0
        for char in masked[body_start:offset]:
            if char == "{":
                depth += 1
            elif char == "}":
                depth -= 1
        if depth > 0:
            return match.group("owner"), _cpp_namespace_prefix(masked, match.start())

    return None


def _cpp_property_alias_scan(
    path: Path,
    root: Path,
    record_builder: StructuredRecordBuilder,
) -> ScanResult:
    """Turn deprecated-alias macro invocations in one source file into records."""
    source_name = path.relative_to(root).as_posix()
    try:
        source = path.read_text(encoding="utf-8")
    except OSError as error:
        return ScanResult(
            records=(),
            diagnostics=(Diagnostic(source_name, 1, "error", f"cannot read source: {error}"),),
        )

    records: list[DeprecationRecord] = []
    diagnostics: list[Diagnostic] = []

    for call in _cpp_macro_calls(source, PROPERTY_ALIAS_MACRO):
        if len(call.arguments) != 4:
            diagnostics.append(
                Diagnostic(
                    source_name,
                    call.line,
                    "error",
                    f"{PROPERTY_ALIAS_MACRO} requires four literal arguments",
                )
            )
            continue

        canonical, alias_arg, deprecated_arg, removed_arg = call.arguments
        if not re.fullmatch(r"[A-Za-z_]\w*", canonical):
            diagnostics.append(
                Diagnostic(
                    source_name,
                    call.line,
                    "error",
                    f"{PROPERTY_ALIAS_MACRO} canonical property must be an identifier",
                )
            )
            continue

        alias = _cpp_string_literal(alias_arg)
        deprecated_in = _cpp_string_literal(deprecated_arg)
        removed_in = _cpp_string_literal(removed_arg)
        if alias is None or deprecated_in is None or removed_in is None:
            diagnostics.append(
                Diagnostic(
                    source_name,
                    call.line,
                    "error",
                    f"{PROPERTY_ALIAS_MACRO} lifecycle metadata must use string literals",
                )
            )
            continue

        owner_info = _cpp_enclosing_constructor(source, call.offset)
        if owner_info is None:
            diagnostics.append(
                Diagnostic(
                    source_name,
                    call.line,
                    "error",
                    f"cannot determine owner of {PROPERTY_ALIAS_MACRO}",
                )
            )
            continue

        owner, namespaces = owner_info
        relative = path.relative_to(root / "src")
        if "::" not in owner:
            if namespaces:
                owner = "::".join((*namespaces, owner))
            # Constructors in src/App and src/Gui commonly omit their namespace because
            # the implementation file is already inside namespace App/Gui.
            elif relative.parts[0] in {"App", "Gui"}:
                owner = f"{relative.parts[0]}::{owner}"

        record, record_diagnostics = record_builder(
            symbol=f"{owner}.{alias}",
            kind="property_alias",
            values={
                "deprecated_in": deprecated_in,
                "removed_in": removed_in,
                "replacement": canonical,
            },
            source=source_name,
            line=call.line,
        )
        if record:
            records.append(record)
        diagnostics.extend(record_diagnostics)

    return ScanResult(records=tuple(records), diagnostics=tuple(diagnostics))


def _cpp_source_paths(source_root: Path) -> Iterable[Path]:
    for path in source_root.rglob("*"):
        if not path.is_file() or path.suffix not in CPP_SUFFIXES:
            continue
        rel = path.relative_to(source_root)
        if SKIPPED_PARTS.intersection(rel.parts) or rel.parts[0] == "Tools":
            continue
        yield path


def scan_property_alias_deprecations(
    root: Path,
    record_builder: StructuredRecordBuilder,
) -> ScanResult:
    """Scan C++ sources for deprecated property aliases.

    ``record_builder`` is the coordinator's shared lifecycle validator.  The
    C++ scanner only extracts source facts—owner, alias, replacement, and the
    two release literals—and deliberately does not duplicate record semantics.
    """
    source_root = root / "src"
    records: list[DeprecationRecord] = []
    diagnostics: list[Diagnostic] = []
    for path in sorted(_cpp_source_paths(source_root)):
        result = _cpp_property_alias_scan(path, root, record_builder)
        records.extend(result.records)
        diagnostics.extend(result.diagnostics)
    return ScanResult(records=tuple(records), diagnostics=tuple(diagnostics))
