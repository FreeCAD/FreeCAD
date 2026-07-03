# pyright: strict

"""Module-level merge helpers for public stub assembly.

This module owns package layout and module-body merge behavior:
- decide which public module files must exist
- map module names onto package-shaped ``.pyi`` output paths
- merge overlays and source-adjacent support nodes into generated modules
- carry support declarations from source-adjacent type stubs into the final
  public modules

In the overall pipeline this is the module-shaped half of the merge stage. The
class-shaped half lives in ``class_merge``.
"""

from __future__ import annotations

import ast
from collections.abc import Callable
import copy
from pathlib import Path

from .discovery import module_names_from_methods, module_names_from_type_methods
from .model import BindingClass
from .parsing import iter_module_stub_pyi_files, iter_type_stub_pyi_files
from .source_inputs import parse_type_stub_target


def module_names_from_classes(classes: list[BindingClass]) -> set[str]:
    return {
        public_name.rsplit(".", 1)[0]
        for klass in classes
        for public_name in klass.public_names
        if "." in public_name
    }


def overlay_module_name(relative: Path) -> str | None:
    if relative.suffix != ".pyi":
        return None
    if relative.name == "__init__.pyi" and relative.parent.parts:
        return ".".join(relative.parent.parts)
    return ".".join(relative.with_suffix("").parts)


def module_names_from_overlays(overlay_dir: Path | None) -> set[str]:
    if not overlay_dir or not overlay_dir.exists():
        return set()
    names: set[str] = set()
    for source in overlay_dir.rglob("*.pyi"):
        module_name = overlay_module_name(source.relative_to(overlay_dir))
        if module_name:
            names.add(module_name)
    return names


def public_module_names(
    methods,
    classes: list[BindingClass],
    type_registrations: dict[str, list[str]],
    overlay_dir: Path | None,
) -> set[str]:
    return (
        module_names_from_methods(methods)
        | module_names_from_classes(classes)
        | module_names_from_type_methods(methods, type_registrations)
        | module_names_from_overlays(overlay_dir)
    )


def has_child_module(module_name: str, module_names: set[str]) -> bool:
    prefix = f"{module_name}."
    return any(name.startswith(prefix) for name in module_names)


def module_stub_path(out_dir: Path, module_name: str, module_names: set[str]) -> Path:
    parts = module_name.split(".")
    if len(parts) == 1 or has_child_module(module_name, module_names):
        return out_dir.joinpath(*parts, "__init__.pyi")
    return out_dir.joinpath(*parts[:-1], f"{parts[-1]}.pyi")


def ensure_parent_package_stubs(out_dir: Path, module_names: set[str]) -> None:
    for module_name in module_names:
        parts = module_name.split(".")
        for index in range(1, len(parts)):
            package_dir = out_dir.joinpath(*parts[:index])
            package_dir.mkdir(parents=True, exist_ok=True)
            init_path = package_dir / "__init__.pyi"
            if not init_path.exists():
                init_path.write_text(
                    "from __future__ import annotations\n",
                    encoding="utf-8",
                )


def public_stub_symbols(source: str) -> set[str]:
    try:
        tree = ast.parse(source)
    except SyntaxError:
        return set()

    symbols: set[str] = set()
    for node in tree.body:
        symbols.update(top_level_symbol_names(node))
    return symbols


def binding_symbol_names(node: ast.stmt) -> set[str]:
    match node:
        case ast.ClassDef():
            return {node.name}
        case ast.FunctionDef():
            return {node.name}
        case ast.AsyncFunctionDef():
            return {node.name}
        case ast.AnnAssign():
            target = node.target
            if isinstance(target, ast.Name):
                return {target.id}
            return set()
        case ast.Assign():
            names: set[str] = set()
            for target in node.targets:
                if isinstance(target, ast.Name):
                    names.add(target.id)
            return names
        case _:
            return set()


def top_level_import_symbol_names(node: ast.stmt) -> set[str]:
    match node:
        case ast.Import():
            exposed: set[str] = set()
            for alias in node.names:
                exposed.add(alias.asname or alias.name.split(".", 1)[0])
            return exposed
        case ast.ImportFrom():
            exposed: set[str] = set()
            for alias in node.names:
                if alias.name == "*":
                    continue
                exposed.add(alias.asname or alias.name)
            return exposed
        case _:
            return set()


def class_body_defined_symbols(body: list[ast.stmt]) -> set[str]:
    symbols: set[str] = set()
    for node in body:
        symbols.update(binding_symbol_names(node))
    return symbols


def type_checking_test(node: ast.expr) -> bool:
    if isinstance(node, ast.Name):
        return node.id == "TYPE_CHECKING"
    if isinstance(node, ast.Attribute) and node.attr == "TYPE_CHECKING":
        return isinstance(node.value, ast.Name) and node.value.id == "typing"
    return False


def import_stmt_line(node: ast.Import | ast.ImportFrom) -> str:
    if isinstance(node, ast.Import):
        names = ", ".join(
            alias.name + (f" as {alias.asname}" if alias.asname else "") for alias in node.names
        )
        return f"import {names}"

    module = "." * node.level + (node.module or "")
    names = ", ".join(
        alias.name + (f" as {alias.asname}" if alias.asname else "") for alias in node.names
    )
    return f"from {module} import {names}"


def top_level_defined_symbols(body: list[ast.stmt]) -> set[str]:
    symbols: set[str] = set()
    for node in body:
        symbols.update(top_level_symbol_names(node))
    return symbols


def unparse_module_body(body: list[ast.stmt]) -> str:
    if not body:
        return ""
    return ast.unparse(ast.Module(body=body, type_ignores=[])).rstrip() + "\n"


def leading_comment_block(source: str) -> str:
    lines = source.splitlines()
    leading: list[str] = []
    for line in lines:
        if line.startswith("#") or not line.strip():
            leading.append(line)
            continue
        break
    return "\n".join(leading).rstrip()


def top_level_symbol_names(node: ast.stmt) -> set[str]:
    return binding_symbol_names(node) | top_level_import_symbol_names(node)


def overlay_symbol_groups(body: list[ast.stmt]) -> list[tuple[set[str], list[ast.stmt]]]:
    groups: list[tuple[set[str], list[ast.stmt]]] = []
    for node in body:
        names = top_level_symbol_names(node)
        if groups and names and groups[-1][0] == names:
            groups[-1][1].append(node)
            continue
        groups.append((names, [node]))
    return groups


def support_definition_group(group: list[ast.stmt]) -> bool:
    if not group:
        return False
    return all(
        isinstance(node, (ast.ClassDef, ast.FunctionDef, ast.AsyncFunctionDef)) for node in group
    )


def overlay_insertion_index(body: list[ast.stmt]) -> int:
    insertion_index = 0
    while insertion_index < len(body):
        current = body[insertion_index]
        if (
            isinstance(current, ast.Expr)
            and isinstance(current.value, ast.Constant)
            and isinstance(current.value.value, str)
        ):
            insertion_index += 1
            continue
        if isinstance(current, (ast.Import, ast.ImportFrom, ast.Assign, ast.AnnAssign)):
            insertion_index += 1
            continue
        break
    return insertion_index


def class_support_insertion_index(body: list[ast.stmt]) -> int:
    insertion_index = 0
    if (
        body
        and isinstance(body[0], ast.Expr)
        and isinstance(body[0].value, ast.Constant)
        and isinstance(body[0].value.value, str)
    ):
        insertion_index = 1
    while insertion_index < len(body):
        current = body[insertion_index]
        if isinstance(current, (ast.Assign, ast.AnnAssign)):
            insertion_index += 1
            continue
        break
    return insertion_index


def is_future_import(node: ast.stmt) -> bool:
    return isinstance(node, ast.ImportFrom) and node.module == "__future__"


def normalize_future_imports(body: list[ast.stmt]) -> list[ast.stmt]:
    if not body:
        return body

    docstring_end = 0
    if (
        isinstance(body[0], ast.Expr)
        and isinstance(body[0].value, ast.Constant)
        and isinstance(body[0].value.value, str)
    ):
        docstring_end = 1

    future_imports = [node for node in body[docstring_end:] if is_future_import(node)]
    if not future_imports:
        return body

    rest = [node for node in body[docstring_end:] if not is_future_import(node)]
    return body[:docstring_end] + future_imports + rest


def copied_named_definition(
    node: ast.ClassDef | ast.FunctionDef | ast.AsyncFunctionDef,
    existing_symbols: set[str],
) -> ast.stmt | None:
    if node.name in existing_symbols:
        return None
    existing_symbols.add(node.name)
    return copy.deepcopy(node)


def copied_assignment_support_node(
    node: ast.Assign | ast.AnnAssign,
    existing_symbols: set[str],
    symbol_names: set[str],
) -> ast.stmt | None:
    if not symbol_names or symbol_names.issubset(existing_symbols):
        return None
    existing_symbols.update(symbol_names)
    return copy.deepcopy(node)


def filtered_import_aliases(
    names: list[ast.alias],
    existing_symbols: set[str],
    *,
    from_import: bool,
) -> list[ast.alias]:
    kept: list[ast.alias] = []
    for alias in names:
        if from_import and alias.name == "*":
            continue
        exposed = alias.asname or (alias.name if from_import else alias.name.split(".", 1)[0])
        if exposed in existing_symbols:
            continue
        kept.append(copy.deepcopy(alias))
        existing_symbols.add(exposed)
    return kept


def filtered_type_checking_block(
    test: ast.expr,
    body: list[ast.stmt],
    existing_symbols: set[str],
    body_filter: Callable[[list[ast.stmt], set[str]], list[ast.stmt]],
) -> ast.If | None:
    if not type_checking_test(test):
        return None
    filtered_body = body_filter(body, existing_symbols)
    if not filtered_body:
        return None
    return ast.If(test=copy.deepcopy(test), body=filtered_body, orelse=[])


def filtered_module_support_node(
    node: ast.stmt,
    existing_symbols: set[str],
) -> ast.stmt | None:
    match node:
        case ast.ClassDef():
            return copied_named_definition(node, existing_symbols)
        case ast.FunctionDef():
            return copied_named_definition(node, existing_symbols)
        case ast.AsyncFunctionDef():
            return copied_named_definition(node, existing_symbols)
        case ast.Import():
            kept = filtered_import_aliases(node.names, existing_symbols, from_import=False)
            return ast.Import(names=kept) if kept else None
        case ast.ImportFrom(module="__future__"):
            return None
        case ast.ImportFrom():
            kept = filtered_import_aliases(node.names, existing_symbols, from_import=True)
            if not kept:
                return None
            return ast.ImportFrom(module=node.module, names=kept, level=node.level)
        case ast.Assign() | ast.AnnAssign():
            return copied_assignment_support_node(
                node,
                existing_symbols,
                top_level_symbol_names(node),
            )
        case ast.If(orelse=[]):
            return filtered_type_checking_block(
                node.test,
                node.body,
                existing_symbols,
                filtered_module_support_nodes,
            )
        case _:
            return None


def filtered_module_support_nodes(
    body: list[ast.stmt],
    existing_symbols: set[str],
) -> list[ast.stmt]:
    filtered_nodes: list[ast.stmt] = []
    for names, group in overlay_symbol_groups(body):
        if names and support_definition_group(group):
            if names.issubset(existing_symbols):
                continue
            existing_symbols.update(names)
            filtered_nodes.extend(copy.deepcopy(node) for node in group)
            continue
        for node in group:
            filtered = filtered_module_support_node(node, existing_symbols)
            if filtered is not None:
                filtered_nodes.append(filtered)
    return filtered_nodes


def filtered_type_class_support_node(
    node: ast.stmt,
    existing_symbols: set[str],
) -> ast.stmt | None:
    match node:
        case ast.FunctionDef() | ast.AsyncFunctionDef():
            return None
        case ast.ClassDef():
            return copied_named_definition(node, existing_symbols)
        case ast.Assign() | ast.AnnAssign():
            return copied_assignment_support_node(
                node,
                existing_symbols,
                class_body_defined_symbols([node]),
            )
        case ast.If(orelse=[]):
            return filtered_type_checking_block(
                node.test,
                node.body,
                existing_symbols,
                filtered_type_class_support_nodes,
            )
        case _:
            return None


def filtered_type_class_support_nodes(
    body: list[ast.stmt],
    existing_symbols: set[str],
) -> list[ast.stmt]:
    filtered_nodes: list[ast.stmt] = []
    for node in body:
        filtered = filtered_type_class_support_node(node, existing_symbols)
        if filtered is not None:
            filtered_nodes.append(filtered)
    return filtered_nodes


def module_support_source(source: str) -> str:
    tree = ast.parse(source)
    support_nodes = filtered_module_support_nodes(tree.body, set())
    return unparse_module_body(support_nodes)


def type_stub_support_sources(source: str, class_symbol: str) -> tuple[str, str]:
    tree = ast.parse(source)
    target_class: ast.ClassDef | None = None
    module_body: list[ast.stmt] = []
    for node in tree.body:
        if isinstance(node, ast.ClassDef) and node.name == class_symbol and target_class is None:
            target_class = node
            continue
        module_body.append(node)

    module_support_nodes = filtered_module_support_nodes(module_body, set())
    module_support_source_text = unparse_module_body(module_support_nodes)

    if target_class is None:
        return module_support_source_text, ""

    class_support_nodes = filtered_type_class_support_nodes(
        target_class.body,
        set(),
    )
    class_support_source = unparse_module_body(class_support_nodes)
    return module_support_source_text, class_support_source


def merged_module_source(target_source: str, target_tree: ast.Module) -> str:
    merged = ast.unparse(target_tree).rstrip() + "\n"
    preamble = leading_comment_block(target_source)
    if preamble:
        return f"{preamble}\n\n{merged}"
    return merged


def merge_module_support_nodes(target_source: str, support_source: str) -> str:
    if not support_source.strip():
        return target_source

    target_tree = ast.parse(target_source)
    support_tree = ast.parse(support_source)
    target_body = target_tree.body
    existing_symbols = top_level_defined_symbols(target_body)
    support_nodes = filtered_module_support_nodes(support_tree.body, existing_symbols)
    if not support_nodes:
        return target_source

    insertion_index = overlay_insertion_index(target_body)
    target_tree.body = normalize_future_imports(
        target_body[:insertion_index] + support_nodes + target_body[insertion_index:]
    )
    return merged_module_source(target_source, target_tree)


def merge_type_class_support_nodes(
    target_source: str,
    class_symbol: str,
    support_source: str,
) -> str:
    if not support_source.strip():
        return target_source

    target_tree = ast.parse(target_source)
    support_tree = ast.parse(support_source)
    target_class = next(
        (
            node
            for node in target_tree.body
            if isinstance(node, ast.ClassDef) and node.name == class_symbol
        ),
        None,
    )
    if target_class is None:
        return target_source

    existing_symbols = class_body_defined_symbols(target_class.body)
    support_nodes = filtered_type_class_support_nodes(support_tree.body, existing_symbols)
    if not support_nodes:
        return target_source

    insertion_index = class_support_insertion_index(target_class.body)
    target_class.body = (
        target_class.body[:insertion_index] + support_nodes + target_class.body[insertion_index:]
    )
    return merged_module_source(target_source, target_tree)


def merge_overlay_module(target_source: str, overlay_source: str) -> str:
    target_tree = ast.parse(target_source)
    overlay_tree = ast.parse(overlay_source)

    target_body = target_tree.body
    overlay_nodes: list[ast.stmt] = []
    for names, group in overlay_symbol_groups(overlay_tree.body):
        if names:
            target_body = [
                existing
                for existing in target_body
                if not top_level_symbol_names(existing).intersection(names)
            ]
        overlay_nodes.extend(copy.deepcopy(node) for node in group)

    insertion_index = overlay_insertion_index(target_body)
    target_tree.body = normalize_future_imports(
        target_body[:insertion_index] + overlay_nodes + target_body[insertion_index:]
    )
    return merged_module_source(target_source, target_tree)


def copy_overlay_stubs(
    overlay_dir: Path,
    target_dir: Path,
    module_names: set[str] | None = None,
) -> int:
    count = 0
    if not overlay_dir.exists():
        return count

    for source in sorted(overlay_dir.rglob("*.pyi")):
        relative = source.relative_to(overlay_dir)
        module_name = overlay_module_name(relative)
        target = (
            module_stub_path(target_dir, module_name, module_names)
            if module_names and module_name
            else target_dir / relative
        )
        target.parent.mkdir(parents=True, exist_ok=True)
        overlay_source = source.read_text(encoding="utf-8")
        if target.exists():
            merged = merge_overlay_module(target.read_text(encoding="utf-8"), overlay_source)
            target.write_text(merged, encoding="utf-8")
        else:
            target.write_text(overlay_source, encoding="utf-8")
        count += 1

    return count


def copy_module_support_stubs(
    root: Path,
    source_dir: Path,
    target_dir: Path,
    module_names: set[str] | None = None,
) -> int:
    count = 0
    for source in sorted(iter_module_stub_pyi_files(root, source_dir)):
        module_name = source.name.removesuffix(".module.pyi")
        target = (
            module_stub_path(target_dir, module_name, module_names)
            if module_names and module_name
            else target_dir / source.name
        )
        if not target.exists():
            continue
        support_source = module_support_source(source.read_text(encoding="utf-8"))
        if not support_source.strip():
            continue
        merged = merge_module_support_nodes(target.read_text(encoding="utf-8"), support_source)
        target.write_text(merged, encoding="utf-8")
        count += 1
    return count


def copy_type_support_stubs(
    root: Path,
    source_dir: Path,
    target_dir: Path,
    module_names: set[str] | None = None,
) -> int:
    count = 0
    for source in sorted(iter_type_stub_pyi_files(root, source_dir)):
        module_name, class_symbol = parse_type_stub_target(source)
        target = (
            module_stub_path(target_dir, module_name, module_names)
            if module_names and module_name
            else target_dir / source.name
        )
        if not target.exists():
            continue
        module_support_source_text, class_support_source = type_stub_support_sources(
            source.read_text(encoding="utf-8"),
            class_symbol,
        )
        original = target.read_text(encoding="utf-8")
        merged = original
        if module_support_source_text.strip():
            merged = merge_module_support_nodes(merged, module_support_source_text)
        if class_support_source.strip():
            merged = merge_type_class_support_nodes(merged, class_symbol, class_support_source)
        if merged == original:
            continue
        target.write_text(merged, encoding="utf-8")
        count += 1
    return count
