"""Low-level parsing helpers for the stub generation pipeline.

This module deals with syntax, not policy. It provides the small building
blocks used by the generator to read C++ source, CMake fragments, and binding
``.pyi`` files without deciding which symbols should become public stubs.

Typical responsibilities here are:
- stripping comments while preserving source offsets
- extracting balanced C++ initializer bodies
- splitting top-level comma-separated expressions safely
- scanning the repository for relevant source and helper ``.pyi`` files
- reading Python AST fragments from binding specs

The manual scanners here exist for raw C++ and CMake text that Python's
``ast`` cannot parse. Python inputs already use ``ast`` through
``parse_python_source`` and the helper functions below.

If a change affects what gets published or how names are mapped, it usually
belongs in ``generator`` instead of this module.
"""

from __future__ import annotations

import ast
from functools import lru_cache
from pathlib import Path, PurePosixPath
import re
from typing import Iterable

from .model import (
    GENERATE_FROM_PY_CALL_RE,
    HELPER_PYI_FILES,
    MODULE_STUB_PYI_SUFFIX,
    SKIPPED_SOURCE_PREFIXES,
    SOURCE_EXTENSIONS,
    STRING_LITERAL_RE,
    ScannerState,
)


def strip_comments(source: str) -> str:
    """Return source with comments blanked while preserving string literals."""

    out: list[str] = []
    i = 0
    state = ScannerState.CODE
    while i < len(source):
        char = source[i]
        nxt = source[i + 1] if i + 1 < len(source) else ""

        match state:
            case ScannerState.CODE:
                if char == "/" and nxt == "/":
                    out.extend("  ")
                    i += 2
                    state = ScannerState.LINE_COMMENT
                    continue
                if char == "/" and nxt == "*":
                    out.extend("  ")
                    i += 2
                    state = ScannerState.BLOCK_COMMENT
                    continue
                out.append(char)
                if char == '"':
                    state = ScannerState.STRING
                elif char == "'":
                    state = ScannerState.CHAR
                i += 1
                continue
            case ScannerState.LINE_COMMENT:
                out.append("\n" if char == "\n" else " ")
                if char == "\n":
                    state = ScannerState.CODE
                i += 1
                continue
            case ScannerState.BLOCK_COMMENT:
                if char == "*" and nxt == "/":
                    out.extend("  ")
                    i += 2
                    state = ScannerState.CODE
                    continue
                out.append("\n" if char == "\n" else " ")
                i += 1
                continue
            case ScannerState.STRING | ScannerState.CHAR:
                out.append(char)
                if char == "\\":
                    if i + 1 < len(source):
                        out.append(source[i + 1])
                        i += 2
                        continue
                elif (state is ScannerState.STRING and char == '"') or (
                    state is ScannerState.CHAR and char == "'"
                ):
                    state = ScannerState.CODE
        i += 1

    return "".join(out)


def line_number(source: str, index: int) -> int:
    return source.count("\n", 0, index) + 1


def extract_balanced(source: str, start: int, opener: str, closer: str) -> tuple[str, int]:
    depth = 0
    state = ScannerState.CODE
    i = start
    while i < len(source):
        char = source[i]
        match state:
            case ScannerState.CODE:
                if char == '"':
                    state = ScannerState.STRING
                elif char == "'":
                    state = ScannerState.CHAR
                elif char == opener:
                    depth += 1
                elif char == closer:
                    depth -= 1
                    if depth == 0:
                        return source[start + 1 : i], i + 1
            case ScannerState.STRING | ScannerState.CHAR:
                if char == "\\":
                    i += 2
                    continue
                if (state is ScannerState.STRING and char == '"') or (
                    state is ScannerState.CHAR and char == "'"
                ):
                    state = ScannerState.CODE
            case _:
                raise AssertionError(f"unexpected scanner state: {state}")
        i += 1

    raise ValueError(f"unterminated {opener}{closer} block at byte offset {start}")


def split_top_level(expression: str) -> list[str]:
    parts: list[str] = []
    start = 0
    paren = brace = bracket = 0
    state = ScannerState.CODE
    i = 0
    while i < len(expression):
        char = expression[i]
        match state:
            case ScannerState.CODE:
                if char == '"':
                    state = ScannerState.STRING
                elif char == "'":
                    state = ScannerState.CHAR
                elif char == "(":
                    paren += 1
                elif char == ")":
                    paren -= 1
                elif char == "{":
                    brace += 1
                elif char == "}":
                    brace -= 1
                elif char == "[":
                    bracket += 1
                elif char == "]":
                    bracket -= 1
                elif char == "," and paren == 0 and brace == 0 and bracket == 0:
                    parts.append(expression[start:i].strip())
                    start = i + 1
            case ScannerState.STRING | ScannerState.CHAR:
                if char == "\\":
                    i += 2
                    continue
                if (state is ScannerState.STRING and char == '"') or (
                    state is ScannerState.CHAR and char == "'"
                ):
                    state = ScannerState.CODE
            case _:
                raise AssertionError(f"unexpected scanner state: {state}")
        i += 1

    tail = expression[start:].strip()
    if tail:
        parts.append(tail)
    return parts


def add_type_calls(source: str) -> Iterable[tuple[int, str, str, str]]:
    """Yield parsed ``addType(...)`` registrations from stripped C++ source.

    Example source shape:
        addType(SomeNamespace::Thing::Type, module, "Thing");

    The return values are ``(byte_offset, type_expr, module_var, export_name)``.
    Parsing is balanced rather than regex-based because the first argument can
    contain nested calls or template syntax.
    """

    start = 0
    marker = "addType"
    while True:
        index = source.find(marker, start)
        if index == -1:
            return

        prefix = source[index - 1] if index > 0 else ""
        suffix_index = index + len(marker)
        suffix = source[suffix_index] if suffix_index < len(source) else ""
        if (prefix.isalnum() or prefix == "_") or (suffix.isalnum() or suffix == "_"):
            start = suffix_index
            continue

        paren_index = suffix_index
        while paren_index < len(source) and source[paren_index].isspace():
            paren_index += 1
        if paren_index >= len(source) or source[paren_index] != "(":
            start = suffix_index
            continue

        try:
            body, end = extract_balanced(source, paren_index, "(", ")")
        except ValueError:
            start = paren_index + 1
            continue

        fields = split_top_level(body)
        if len(fields) >= 3:
            name = first_string_literal(fields[2])
            if name:
                yield index, fields[0], fields[1], name
        start = end


def string_literals(expression: str) -> list[str]:
    values: list[str] = []
    for match in STRING_LITERAL_RE.finditer(expression):
        try:
            value = ast.literal_eval(match.group(1))
        except (SyntaxError, ValueError):
            value = match.group(1).strip('"')
        values.append(value)
    return values


def first_string_literal(expression: str) -> str | None:
    values = string_literals(expression)
    return values[0] if values else None


def normalize_doc(expression: str) -> str:
    return "".join(string_literals(expression)).strip()


def normalize_expr(expression: str) -> str:
    return " ".join(expression.strip().split())


def decorator_kwargs(decorator: ast.expr) -> dict[str, object]:
    if not isinstance(decorator, ast.Call):
        return {}
    result: dict[str, object] = {}
    for keyword_node in decorator.keywords:
        if keyword_node.arg is None:
            continue
        try:
            result[keyword_node.arg] = ast.literal_eval(keyword_node.value)
        except (ValueError, SyntaxError):
            continue
    return result


def decorator_name(decorator: ast.expr) -> str:
    return (
        ast.unparse(decorator.func) if isinstance(decorator, ast.Call) else ast.unparse(decorator)
    )


def skipped_source_path(rel_path: str) -> bool:
    parts = PurePosixPath(rel_path).parts
    return any(parts[: len(prefix)] == prefix for prefix in SKIPPED_SOURCE_PREFIXES)


def iter_source_files(root: Path, source_dir: Path) -> Iterable[Path]:
    for path in source_dir.rglob("*"):
        if path.suffix not in SOURCE_EXTENSIONS or not path.is_file():
            continue
        rel = path.relative_to(root).as_posix()
        if skipped_source_path(rel):
            continue
        yield path


@lru_cache(maxsize=None)
def cmake_registered_binding_pyi_files(root: Path, source_dir: Path) -> tuple[Path, ...]:
    registered: set[Path] = set()
    for cmake_file in source_dir.rglob("CMakeLists.txt"):
        try:
            source = cmake_file.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        source = re.sub(r"#.*", "", source)
        for match in GENERATE_FROM_PY_CALL_RE.finditer(source):
            candidate = (cmake_file.parent / f"{match.group('base')}.pyi").resolve()
            if candidate.is_file():
                registered.add(candidate)
    return tuple(sorted(registered))


def iter_binding_pyi_files(root: Path, source_dir: Path) -> Iterable[Path]:
    for path in cmake_registered_binding_pyi_files(root, source_dir):
        rel = path.relative_to(root).as_posix()
        if rel in HELPER_PYI_FILES:
            continue
        if skipped_source_path(rel):
            continue
        yield path


def iter_module_stub_pyi_files(root: Path, source_dir: Path) -> Iterable[Path]:
    for path in source_dir.rglob(f"*{MODULE_STUB_PYI_SUFFIX}"):
        if not path.is_file():
            continue
        rel = path.relative_to(root).as_posix()
        if skipped_source_path(rel):
            continue
        yield path


def iter_type_stub_pyi_files(root: Path, source_dir: Path) -> Iterable[Path]:
    registered = set(cmake_registered_binding_pyi_files(root, source_dir))
    for path in source_dir.rglob("*.pyi"):
        if not path.is_file():
            continue
        if path in registered:
            continue
        if path.name.endswith(MODULE_STUB_PYI_SUFFIX):
            continue
        rel = path.relative_to(root).as_posix()
        if rel in HELPER_PYI_FILES:
            continue
        if skipped_source_path(rel):
            continue
        yield path


def generated_source(rel_path: str) -> bool:
    path = Path(rel_path)
    return path.name.endswith("PyImp.cpp")


def parse_python_source(path: Path) -> ast.Module | None:
    try:
        source = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None
    try:
        tree = ast.parse(source, filename=str(path))
    except SyntaxError:
        return None
    return tree
