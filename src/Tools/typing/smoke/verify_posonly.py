#!/usr/bin/env python3
'''
Test whether all function definitions declare a positional-only arg.
'''
import ast
import argparse
from pathlib import Path


parser = argparse.ArgumentParser()
parser.add_argument("-v", "--verbose", action="store_true")
parser.add_argument("files", nargs="*", type=Path)
args = parser.parse_args()


if args.files:
    files: list[Path] = [x.resolve() for x in args.files]
    relative_to = Path.cwd()
else:
    scripts_dir = Path(__file__).resolve().parent.parent
    src_dir = scripts_dir.parent.parent  # "src" dir
    files = sorted(
        path for path in src_dir.rglob("*.pyi")
        if scripts_dir not in path.parents  # ignore generated types
    )
    relative_to = src_dir


for path in files:
    if path.name == 'PropertyPythonContracts.pyi':
        print(f'ignored: {path.relative_to(relative_to)}')
        continue  # they are not re-exported, so we dont care

    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))

    for node in sorted(ast.walk(tree), key=lambda n: getattr(n, "lineno", 0)):
        if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            continue

        params = node.args
        if params.posonlyargs:
            continue  # already contains "/"
        if not (params.kwonlyargs or params.vararg or params.kwarg):
            if not params.args:
                continue  # empty static args `def foo()`
            if len(params.args) == 1 and params.args[0].arg in ("self", "cls"):
                continue  # empty instance args `def foo(self)`

        msg = f"{path.relative_to(relative_to)}:{node.lineno}: {node.name}"
        if args.verbose:
            msg += "(" + ast.unparse(node.args) + ")"
        print(msg)
