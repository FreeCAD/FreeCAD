"""Documentation lint helpers for curated source-adjacent stub inputs.

This module validates the hand-authored stub files that act as the durable
documentation surface for the public Python typing API:

- source-adjacent ``*.module.pyi`` files
- source-adjacent plain ``.pyi`` type stubs that are not binding-spec inputs

The linter is intentionally narrow. It does not inspect the full generated
public stub tree because most of that surface still originates from discovered
C++ registrations rather than curated source files.
"""

from __future__ import annotations

import ast
from dataclasses import dataclass
from pathlib import Path

from .parsing import iter_module_stub_pyi_files, iter_type_stub_pyi_files


@dataclass(frozen=True)
class DocLintIssue:
    path: Path
    line: int
    message: str

    def format(self, root: Path) -> str:
        try:
            display = self.path.relative_to(root)
        except ValueError:
            display = self.path
        return f"{display}:{self.line}: {self.message}"


@dataclass(frozen=True)
class DocLintReport:
    files_checked: int
    issues: tuple[DocLintIssue, ...]

    @property
    def ok(self) -> bool:
        return not self.issues

    def render(self, root: Path) -> str:
        if self.ok:
            return f"Documentation lint passed for {self.files_checked} curated stub files.\n"

        lines = [issue.format(root) for issue in self.issues]
        lines.append(
            f"{len(self.issues)} documentation issues found in {self.files_checked} curated stub files."
        )
        return "\n".join(lines) + "\n"


def selected_stub_path(path: Path, selected: tuple[Path, ...]) -> bool:
    if not selected:
        return True
    return any(path == candidate or candidate in path.parents for candidate in selected)


def public_callable_groups(
    body: list[ast.stmt],
) -> dict[str, list[ast.FunctionDef | ast.AsyncFunctionDef]]:
    groups: dict[str, list[ast.FunctionDef | ast.AsyncFunctionDef]] = {}
    for node in body:
        if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            continue
        groups.setdefault(node.name, []).append(node)
    return groups


def lint_callable_group(
    path: Path,
    owner: str | None,
    name: str,
    group: list[ast.FunctionDef | ast.AsyncFunctionDef],
) -> DocLintIssue | None:
    if any(ast.get_docstring(node, clean=True) for node in group):
        return None

    qualifier = f"{owner}.{name}" if owner else name
    kind = "overload group" if len(group) > 1 else "callable"
    return DocLintIssue(path, group[0].lineno, f"missing docstring for {kind} `{qualifier}`")


def lint_class_body(path: Path, class_node: ast.ClassDef) -> list[DocLintIssue]:
    issues: list[DocLintIssue] = []
    if not ast.get_docstring(class_node, clean=True):
        issues.append(
            DocLintIssue(
                path, class_node.lineno, f"missing docstring for class `{class_node.name}`"
            )
        )

    for method_name, group in sorted(public_callable_groups(class_node.body).items()):
        issue = lint_callable_group(path, class_node.name, method_name, group)
        if issue is not None:
            issues.append(issue)
    return issues


def lint_stub_file(path: Path) -> list[DocLintIssue]:
    source = path.read_text(encoding="utf-8")
    tree = ast.parse(source, filename=str(path))
    issues: list[DocLintIssue] = []

    if not ast.get_docstring(tree, clean=True):
        issues.append(DocLintIssue(path, 1, "missing module docstring"))

    for name, group in sorted(public_callable_groups(tree.body).items()):
        issue = lint_callable_group(path, None, name, group)
        if issue is not None:
            issues.append(issue)

    for node in tree.body:
        if not isinstance(node, ast.ClassDef):
            continue
        issues.extend(lint_class_body(path, node))

    return issues


def lint_curated_stub_docs(
    root: Path,
    source_dir: Path,
    selected_paths: tuple[Path, ...] = (),
) -> DocLintReport:
    selected = tuple(path.resolve() for path in selected_paths)
    files = [
        path
        for path in sorted(iter_module_stub_pyi_files(root, source_dir))
        if selected_stub_path(path.resolve(), selected)
    ]
    files.extend(
        path
        for path in sorted(iter_type_stub_pyi_files(root, source_dir))
        if selected_stub_path(path.resolve(), selected)
    )

    issues: list[DocLintIssue] = []
    for path in files:
        issues.extend(lint_stub_file(path))

    return DocLintReport(len(files), tuple(issues))
