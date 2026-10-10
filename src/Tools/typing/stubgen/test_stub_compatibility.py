# SPDX-License-Identifier: LGPL-2.1-or-later

"""Independent semantic comparisons for generated stub trees."""

from __future__ import annotations

import ast
from pathlib import Path
import tempfile
import unittest

from python_api_model.model import ApiCallableGroup, ApiClass, ApiModule
from stubgen.render import render_module
from python_api_model.signatures import CallableDecoratorFlags, CallableSignature


def _module_name(root: Path, path: Path) -> str:
    parts = list(path.relative_to(root).parts)
    if parts[-1] == "__init__.pyi":
        parts.pop()
    else:
        parts[-1] = parts[-1].removesuffix(".pyi")
    return ".".join(parts)


def _target_names(node: ast.stmt) -> set[str]:
    if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
        return {node.name} if not node.name.startswith("_") else set()
    if isinstance(node, (ast.Assign, ast.AnnAssign)):
        targets = node.targets if isinstance(node, ast.Assign) else [node.target]
        return {
            target.id
            for target in targets
            if isinstance(target, ast.Name) and not target.id.startswith("_")
        }
    if isinstance(node, (ast.Import, ast.ImportFrom)):
        if isinstance(node, ast.ImportFrom) and node.module in {
            "__future__",
            "typing",
            "typing_extensions",
        }:
            return set()
        return {
            alias.asname
            or (alias.name.split(".", 1)[0] if isinstance(node, ast.Import) else alias.name)
            for alias in node.names
            if alias.name != "*"
            and not (alias.asname or alias.name.split(".", 1)[0]).startswith("_")
        }
    return set()


def _callable_shape(node: ast.FunctionDef | ast.AsyncFunctionDef) -> tuple[str, str, str]:
    return (
        node.name,
        ast.dump(node.args, include_attributes=False),
        ast.unparse(node.returns) if node.returns else "",
    )


def stub_tree_semantics(root: Path) -> dict[str, object]:
    """Extract comparison data without using production model classes."""

    result: dict[str, object] = {}
    for path in sorted(root.rglob("*.pyi")):
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        functions = tuple(
            _callable_shape(node)
            for node in tree.body
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef))
        )
        classes: dict[str, object] = {}
        for node in tree.body:
            if not isinstance(node, ast.ClassDef) or node.name.startswith("_"):
                continue
            classes[node.name] = {
                "bases": tuple(ast.unparse(base) for base in node.bases),
                "members": tuple(
                    sorted(
                        next(iter(names))
                        for member in node.body
                        if len(names := _target_names(member)) == 1
                    )
                ),
                "methods": tuple(
                    _callable_shape(member)
                    for member in node.body
                    if isinstance(member, (ast.FunctionDef, ast.AsyncFunctionDef))
                ),
            }
        result[_module_name(root, path)] = {
            "symbols": tuple(sorted(name for node in tree.body for name in _target_names(node))),
            "functions": functions,
            "classes": classes,
        }
    return result


class StubCompatibilityTests(unittest.TestCase):
    def test_semantic_comparator_ignores_comments_and_docstrings(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            baseline = root / "baseline"
            candidate = root / "candidate"
            baseline.mkdir()
            candidate.mkdir()
            (baseline / "Demo.pyi").write_text(
                "# old\ndef ping(value: int) -> bool: ...\n",
                encoding="utf-8",
            )
            (candidate / "Demo.pyi").write_text(
                '"""new docs"""\n\ndef ping(value: int) -> bool: ...\n',
                encoding="utf-8",
            )

            self.assertEqual(stub_tree_semantics(baseline), stub_tree_semantics(candidate))

    def test_model_output_conserves_public_declarations(self) -> None:
        module = ApiModule(
            name="Demo",
            functions=(
                ApiCallableGroup(
                    name="ping",
                    signatures=(
                        CallableSignature(
                            name="ping",
                            parameters=(),
                            return_annotation="bool",
                            docstring=None,
                            flags=CallableDecoratorFlags(),
                        ),
                    ),
                ),
            ),
            classes=(ApiClass(module_name="Demo", name="Widget"),),
        )

        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            output = root / "Demo.pyi"
            output.write_text(render_module(module), encoding="utf-8")
            semantics = stub_tree_semantics(root)

        symbols = semantics["Demo"]
        assert isinstance(symbols, dict)
        self.assertEqual(symbols["symbols"], ("Widget", "ping"))


if __name__ == "__main__":
    unittest.main()
