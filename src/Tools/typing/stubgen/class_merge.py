# pyright: strict

"""Class-level merge helpers for public stub assembly.

This module owns the class-shaped half of the merge stage:
- map binding classes onto canonical public module/symbol targets
- plan alias exports for multiply-exposed classes
- rewrite binding class ASTs into public import-shaped class definitions
- append generated class stubs into the final public module tree

In the overall pipeline this sits on top of the generic module merge helpers in
``module_merge``. That module handles package paths and module-body merges;
this one handles class bodies, imports, aliases, and class placement.
"""

from __future__ import annotations

import ast
import copy
from pathlib import Path

from .model import (
    BindingClass,
    HELPER_PYI_FILES,
    ImportBinding,
    ImportTarget,
    PUBLIC_STUB_DECORATORS,
    PublicClassStub,
)
from .module_merge import (
    class_body_defined_symbols,
    import_stmt_line,
    module_names_from_classes,
    module_stub_path,
    public_stub_symbols,
    type_checking_test,
)
from .naming import valid_identifier
from .parsing import decorator_name, parse_python_source

TYPE_CHECKING_IMPORT_LINE = "from typing import TYPE_CHECKING"


def keep_public_stub_decorator(decorator: ast.expr) -> bool:
    name = decorator_name(decorator).split(".", 1)[-1]
    return name in PUBLIC_STUB_DECORATORS


class PublicClassStubTransformer(ast.NodeTransformer):
    def __init__(
        self,
        module_name: str,
        public_symbol: str,
        renames: dict[str, str],
        public_base_names: set[str],
        public_base_modules: set[str],
    ):
        self.module_name = module_name
        self.public_symbol = public_symbol
        self.renames = renames
        self.public_base_names = public_base_names
        self.public_base_modules = public_base_modules
        self.class_depth = 0
        self.shadowed_annotation_names: set[str] = set()
        self.annotation_module_roots_needed: set[str] = set()

    def rewrite_annotation(self, annotation: ast.expr | None) -> ast.expr | None:
        if annotation is None:
            return None
        rewritten = self.visit(annotation)
        shadowed_names = {
            child.id
            for child in ast.walk(rewritten)
            if isinstance(child, ast.Name) and child.id in self.shadowed_annotation_names
        }
        if shadowed_names:
            self.annotation_module_roots_needed.add(self.module_name.split(".", 1)[0])
            qualified = QualifyAnnotationNames(self.module_name, shadowed_names).visit(
                copy.deepcopy(rewritten)
            )
            return ast.Constant(value=ast.unparse(qualified))
        return rewritten

    @staticmethod
    def top_level_class_member_names(body: list[ast.stmt]) -> set[str]:
        return class_body_defined_symbols(body)

    def visit_ClassDef(self, node: ast.ClassDef) -> ast.ClassDef:
        is_public_class = self.class_depth == 0
        if is_public_class:
            node.name = self.public_symbol
        node.decorator_list = [
            decorator for decorator in node.decorator_list if keep_public_stub_decorator(decorator)
        ]
        node.bases = [self.visit(base) for base in node.bases]
        if is_public_class:
            node.bases = [
                base
                for base in node.bases
                if (
                    (isinstance(base, ast.Name) and base.id in self.public_base_names)
                    or (
                        isinstance(base, ast.Attribute)
                        and isinstance(base.value, ast.Name)
                        and base.value.id in self.public_base_modules
                    )
                )
            ]
            node.keywords = []
            self.shadowed_annotation_names = self.top_level_class_member_names(node.body)
        self.class_depth += 1
        try:
            node.body = [self.visit(item) for item in node.body]
            flattened: list[ast.stmt] = []
            for item in node.body:
                if isinstance(item, ast.If) and type_checking_test(item.test) and not item.orelse:
                    flattened.extend(item.body)
                else:
                    flattened.append(item)
            node.body = flattened
        finally:
            self.class_depth -= 1
        return node

    def visit_FunctionDef(self, node: ast.FunctionDef) -> ast.FunctionDef:
        node.decorator_list = [
            decorator for decorator in node.decorator_list if keep_public_stub_decorator(decorator)
        ]
        node.args = self.visit(node.args)
        node.returns = self.rewrite_annotation(node.returns)
        node.body = [self.visit(item) for item in node.body]
        return node

    def visit_AsyncFunctionDef(self, node: ast.AsyncFunctionDef) -> ast.AsyncFunctionDef:
        node.decorator_list = [
            decorator for decorator in node.decorator_list if keep_public_stub_decorator(decorator)
        ]
        node.args = self.visit(node.args)
        node.returns = self.rewrite_annotation(node.returns)
        node.body = [self.visit(item) for item in node.body]
        return node

    def visit_arg(self, node: ast.arg) -> ast.arg:
        node.annotation = self.rewrite_annotation(node.annotation)
        return node

    def visit_AnnAssign(self, node: ast.AnnAssign) -> ast.AnnAssign:
        node.target = self.visit(node.target)
        annotation = self.rewrite_annotation(node.annotation)
        if annotation is None:
            raise ValueError("annotated assignment must keep an annotation")
        node.annotation = annotation
        if isinstance(node.value, ast.Constant) and node.value.value is None:
            node.value = ast.Constant(value=Ellipsis)
        else:
            node.value = self.visit(node.value) if node.value else None
        return node

    def visit_Name(self, node: ast.Name) -> ast.Name:
        if node.id in self.renames:
            node.id = self.renames[node.id]
        return node


class QualifyAnnotationNames(ast.NodeTransformer):
    def __init__(self, module_name: str, names: set[str]):
        self.module_name = module_name
        self.names = names

    def qualified_name_expr(self, name: str) -> ast.expr:
        head, *tail = self.module_name.split(".")
        expr: ast.expr = ast.Name(id=head, ctx=ast.Load())
        for part in tail:
            expr = ast.Attribute(value=expr, attr=part, ctx=ast.Load())
        return ast.Attribute(value=expr, attr=name, ctx=ast.Load())

    def visit_Name(self, node: ast.Name) -> ast.expr:
        if node.id in self.names:
            return ast.copy_location(self.qualified_name_expr(node.id), node)
        return node


def group_classes_by_module(classes: list[BindingClass]) -> dict[str, list[BindingClass]]:
    grouped: dict[str, list[BindingClass]] = {}
    seen: set[tuple[str, str]] = set()
    for klass in classes:
        for public_name in klass.public_names:
            if "." not in public_name:
                continue
            module_name = public_name.rsplit(".", 1)[0]
            symbol = public_name.rsplit(".", 1)[1]
            key = (module_name, symbol)
            if key in seen:
                continue
            seen.add(key)
            grouped.setdefault(module_name, []).append(klass)
    return grouped


def class_node(root: Path, klass: BindingClass) -> ast.ClassDef | None:
    tree = parse_python_source(root / klass.source)
    if not tree:
        return None

    for node in tree.body:
        if (
            isinstance(node, ast.ClassDef)
            and node.name == klass.class_name
            and node.lineno == klass.line
        ):
            return node
    return None


def source_import_bindings(root: Path, source: str) -> dict[str, ImportBinding]:
    tree = parse_python_source(root / source)
    if not tree:
        return {}

    bindings: dict[str, ImportBinding] = {}
    for node in tree.body:
        if isinstance(node, ast.Import):
            for alias in node.names:
                exposed_name = alias.asname or alias.name.split(".", 1)[0]
                bindings[exposed_name] = ImportBinding(module=alias.name)
        elif isinstance(node, ast.ImportFrom):
            if node.level:
                continue
            module = node.module or ""
            if not module:
                continue
            for alias in node.names:
                if alias.name == "*":
                    continue
                exposed_name = alias.asname or alias.name
                bindings[exposed_name] = ImportBinding(module=module, name=alias.name)

    return bindings


def module_prefixes(parts: tuple[str, ...]) -> set[str]:
    return {".".join(parts[:index]) for index in range(1, len(parts) + 1)}


def class_source_module_aliases(klass: BindingClass) -> set[str]:
    path = Path(klass.source).with_suffix("")
    parts = path.parts
    aliases: set[str] = {path.name}
    if len(parts) < 3 or parts[0] != "src":
        return aliases

    if parts[1] in {"Base", "App", "Gui"}:
        aliases |= module_prefixes(parts[1:])
    elif parts[1] == "Mod" and len(parts) >= 5:
        workbench = parts[2]
        impl_parts = parts[3:]
        public_parts = (workbench, *parts[4:])
        aliases |= module_prefixes(public_parts)
        aliases |= module_prefixes((workbench, *impl_parts))

    aliases |= {module_name for module_name, _ in class_public_targets(klass)}
    return aliases


def public_import_target_index(classes: list[BindingClass]) -> dict[ImportTarget, ImportTarget]:
    index: dict[ImportTarget, ImportTarget] = {}
    ambiguous: set[ImportTarget] = set()
    for klass in classes:
        target = canonical_class_public_target(klass)
        if not target:
            continue
        candidate_names = {
            klass.class_name,
            klass.export_name,
            *(symbol for _, symbol in class_public_targets(klass)),
        }
        for module_name in class_source_module_aliases(klass):
            for name in candidate_names:
                key = (module_name, name)
                existing = index.get(key)
                if existing and existing != target:
                    ambiguous.add(key)
                    continue
                index[key] = target

    for key in ambiguous:
        index.pop(key, None)
    return index


def known_stub_module_roots(classes: list[BindingClass]) -> set[str]:
    roots = {"App", "Base", "Data", "Gui"}
    for klass in classes:
        for module_name in class_source_module_aliases(klass):
            roots.add(module_name.split(".", 1)[0])
    for helper_source in HELPER_PYI_FILES:
        roots.add(Path(helper_source).with_suffix("").name)
    return roots


def transformed_import_bindings(
    import_bindings: dict[str, ImportBinding],
    renames: dict[str, str],
) -> dict[str, ImportBinding]:
    transformed: dict[str, ImportBinding] = {}
    for name, binding in import_bindings.items():
        transformed.setdefault(renames.get(name, name), binding)
    return transformed


def renamed_source_import_bindings(
    root: Path,
    source: str,
    renames: dict[str, str],
) -> dict[str, ImportBinding]:
    return transformed_import_bindings(source_import_bindings(root, source), renames)


def binding_import_target(
    binding: ImportBinding,
    import_targets: dict[ImportTarget, ImportTarget],
) -> ImportTarget | None:
    if binding.name is None:
        return None
    return import_targets.get((binding.module, binding.name))


def binding_import_line(
    binding: ImportBinding,
    symbol_name: str,
    module_name: str,
    import_targets: dict[ImportTarget, ImportTarget],
    internal_roots: set[str],
) -> str | None:
    if binding.name is not None:
        if target := binding_import_target(binding, import_targets):
            target_module, target_symbol = target
            if target_module == module_name:
                return None
            if symbol_name == target_symbol:
                return f"from {target_module} import {target_symbol}"
            return f"from {target_module} import {target_symbol} as {symbol_name}"

        if binding.module.split(".", 1)[0] in internal_roots:
            return None
        if symbol_name == binding.name:
            return f"from {binding.module} import {binding.name}"
        return f"from {binding.module} import {binding.name} as {symbol_name}"

    root_name = binding.module.split(".", 1)[0]
    if root_name in internal_roots:
        return None
    if symbol_name == root_name:
        return f"import {binding.module}"
    return f"import {binding.module} as {symbol_name}"


def binding_available_in_module(
    binding: ImportBinding,
    symbol_name: str,
    module_name: str,
    import_targets: dict[ImportTarget, ImportTarget],
    internal_roots: set[str],
) -> bool:
    if binding_import_line(binding, symbol_name, module_name, import_targets, internal_roots):
        return True
    target = binding_import_target(binding, import_targets)
    return target is not None and target[0] == module_name


def append_binding_import_line(
    lines: list[str],
    seen: set[str],
    binding: ImportBinding,
    symbol_name: str,
    module_name: str,
    import_targets: dict[ImportTarget, ImportTarget],
    internal_roots: set[str],
) -> None:
    line = binding_import_line(
        binding,
        symbol_name,
        module_name,
        import_targets,
        internal_roots,
    )
    if line and line not in seen:
        seen.add(line)
        lines.append(line)


def referenced_import_lines(
    node: ast.ClassDef,
    import_bindings: dict[str, ImportBinding],
    module_symbols: set[str],
    module_name: str,
    import_targets: dict[ImportTarget, ImportTarget],
    internal_roots: set[str],
) -> tuple[str, ...]:
    lines: list[str] = []
    seen: set[str] = set()
    for child in ast.walk(node):
        if isinstance(child, ast.Name) and isinstance(child.ctx, ast.Load):
            if child.id in module_symbols or child.id == "object":
                continue
            binding = import_bindings.get(child.id)
            if not binding:
                continue
            append_binding_import_line(
                lines,
                seen,
                binding,
                child.id,
                module_name,
                import_targets,
                internal_roots,
            )
        elif isinstance(child, ast.Attribute) and isinstance(child.value, ast.Name):
            binding = import_bindings.get(child.value.id)
            if not binding or binding.name is not None:
                continue
            append_binding_import_line(
                lines,
                seen,
                binding,
                child.value.id,
                module_name,
                import_targets,
                internal_roots,
            )

    return tuple(lines)


def type_checking_import_lines(
    root: Path,
    classes: list[BindingClass],
    existing_source: str = "",
) -> list[str]:
    lines: list[str] = []
    for source in sorted({klass.source for klass in classes}):
        tree = parse_python_source(root / source)
        if not tree:
            continue
        for node in tree.body:
            if not isinstance(node, ast.If) or not type_checking_test(node.test):
                continue
            for item in node.body:
                if isinstance(item, (ast.Import, ast.ImportFrom)):
                    line = import_stmt_line(item)
                    if line not in existing_source and line not in lines:
                        lines.append(line)

    if not lines:
        return []
    return ["if TYPE_CHECKING:", *(f"    {line}" for line in lines), ""]


def module_symbol_renames(classes: list[BindingClass], module_name: str) -> dict[str, str]:
    renames: dict[str, str] = {}
    for klass in classes:
        symbol = class_public_symbol(klass, module_name)
        if symbol:
            renames.setdefault(klass.class_name, symbol)
            renames.setdefault(klass.export_name, symbol)
    return renames


def public_name_target(public_name: str) -> tuple[str, str] | None:
    if "." not in public_name:
        return None
    module_name, symbol = public_name.rsplit(".", 1)
    if not module_name or not valid_identifier(symbol):
        return None
    return module_name, symbol


def class_public_targets(klass: BindingClass) -> list[tuple[str, str]]:
    targets: list[tuple[str, str]] = []
    seen: set[tuple[str, str]] = set()
    for public_name in klass.public_names:
        target = public_name_target(public_name)
        if not target or target in seen:
            continue
        seen.add(target)
        targets.append(target)
    return targets


def canonical_target_from_targets(
    klass: BindingClass,
    targets: list[tuple[str, str]],
) -> tuple[str, str] | None:
    if not targets:
        return None
    if klass.source.startswith("src/Base/"):
        for target in targets:
            if target[0] == "FreeCAD.Base":
                return target
    return targets[0]


def canonical_class_public_target(klass: BindingClass) -> tuple[str, str] | None:
    return canonical_target_from_targets(klass, class_public_targets(klass))


def class_public_symbol(klass: BindingClass, module_name: str) -> str | None:
    return next(
        (
            symbol
            for public_module_name, symbol in class_public_targets(klass)
            if public_module_name == module_name
        ),
        None,
    )


def class_alias_stub_line(
    module_name: str,
    symbol: str,
    target_module_name: str,
    target_symbol: str,
) -> str:
    if module_name == target_module_name:
        return f"{symbol} = {target_symbol}"
    if target_module_name.startswith(f"{module_name}."):
        relative_module = target_module_name.removeprefix(module_name)
        return f"from {relative_module} import {target_symbol} as {symbol}"
    return f"from {target_module_name} import {target_symbol} as {symbol}"


def class_public_alias_targets(
    klass: BindingClass,
) -> list[tuple[str, str, str, str]]:
    targets = class_public_targets(klass)
    canonical_target = canonical_target_from_targets(klass, targets)
    if not canonical_target:
        return []
    target_module_name, target_symbol = canonical_target
    return [
        (module_name, symbol, target_module_name, target_symbol)
        for module_name, symbol in targets
        if (module_name, symbol) != canonical_target
    ]


def class_public_alias_line(klass: BindingClass, module_name: str) -> tuple[str, str] | None:
    for public_module_name, symbol, target_module_name, target_symbol in class_public_alias_targets(
        klass
    ):
        if public_module_name == module_name:
            return (
                symbol,
                class_alias_stub_line(module_name, symbol, target_module_name, target_symbol),
            )
    return None


def validate_public_class_aliases(classes: list[BindingClass]) -> None:
    errors: list[str] = []
    for klass in classes:
        public_names = list(dict.fromkeys(klass.public_names))
        if len(public_names) < 2:
            continue
        targets = class_public_targets(klass)
        canonical_target = canonical_target_from_targets(klass, targets)
        if len(targets) != len(public_names):
            errors.append(
                f"{klass.source}:{klass.line} {klass.class_name} has unsupported public names: "
                + ", ".join(public_names)
            )
            continue
        if not canonical_target:
            errors.append(f"{klass.source}:{klass.line} {klass.class_name} has no canonical target")
            continue
        target_module_name, target_symbol = canonical_target
        aliases = [
            (module_name, symbol, target_module_name, target_symbol)
            for module_name, symbol in targets
            if (module_name, symbol) != canonical_target
        ]
        if len(aliases) != len(targets) - 1:
            errors.append(
                f"{klass.source}:{klass.line} {klass.class_name} has {len(targets)} public "
                f"targets but only {len(aliases)} generated aliases"
            )
    if errors:
        raise ValueError("invalid multi-public class alias plan:\n  " + "\n  ".join(errors))


def public_class_stub_source(
    root: Path,
    klass: BindingClass,
    module_name: str,
    renames: dict[str, str],
    module_symbols: set[str],
    import_targets: dict[ImportTarget, ImportTarget],
    internal_roots: set[str],
) -> PublicClassStub | None:
    symbol = class_public_symbol(klass, module_name)
    if not symbol:
        return None
    node = class_node(root, klass)
    if not node:
        return None
    node = copy.deepcopy(node)
    import_bindings = renamed_source_import_bindings(root, klass.source, renames)
    public_base_names = set(module_symbols)
    public_base_modules: set[str] = set()
    for base in node.bases:
        match base:
            case ast.Name(id=base_name):
                transformed_name = renames.get(base_name, base_name)
                if transformed_name == "object":
                    public_base_names.add("object")
                    continue
                binding = import_bindings.get(transformed_name)
                if not binding:
                    continue
                if binding_available_in_module(
                    binding,
                    transformed_name,
                    module_name,
                    import_targets,
                    internal_roots,
                ):
                    public_base_names.add(transformed_name)
            case ast.Attribute(value=ast.Name(id=module_alias)):
                binding = import_bindings.get(module_alias)
                if not binding or binding.name is not None:
                    continue
                if binding_available_in_module(
                    binding,
                    module_alias,
                    module_name,
                    import_targets,
                    internal_roots,
                ):
                    public_base_modules.add(module_alias)
            case _:
                continue

    transformer = PublicClassStubTransformer(
        module_name,
        symbol,
        renames,
        public_base_names,
        public_base_modules,
    )
    transformed = transformer.visit(node)
    ast.fix_missing_locations(transformed)
    import_lines = list(
        referenced_import_lines(
            transformed,
            import_bindings,
            module_symbols,
            module_name,
            import_targets,
            internal_roots,
        )
    )
    for root_name in sorted(transformer.annotation_module_roots_needed):
        line = f"import {root_name}"
        if line not in import_lines:
            import_lines.insert(0, line)
    return PublicClassStub(
        source=ast.unparse(transformed),
        import_lines=tuple(import_lines),
    )


def class_stub_lines(
    root: Path,
    module_classes: list[BindingClass],
    module_name: str,
    all_classes: list[BindingClass] | None = None,
    include_future_import: bool = True,
    skip_symbols: set[str] | None = None,
    existing_source: str = "",
) -> list[str]:
    all_classes = all_classes or module_classes
    header = [
        "# Generated public class stubs from binding .pyi specs.",
    ]
    if include_future_import:
        header.append("from __future__ import annotations")
    body: list[str] = []
    extra_import_lines: list[str] = []
    seen: set[str] = set()
    skip_symbols = skip_symbols or set()
    renames = module_symbol_renames(module_classes, module_name)
    module_symbols = {
        symbol
        for klass in module_classes
        if (symbol := class_public_symbol(klass, module_name)) is not None
    }
    import_targets = public_import_target_index(all_classes)
    internal_roots = known_stub_module_roots(all_classes)
    alias_lines: list[str] = []
    for klass in module_classes:
        alias = class_public_alias_line(klass, module_name)
        if not alias:
            continue
        symbol, line = alias
        if symbol in seen or symbol in skip_symbols:
            continue
        alias_lines.append(line)
        seen.add(symbol)
    body.extend(alias_lines)
    if alias_lines:
        body.append("")
    for klass in module_classes:
        symbol = class_public_symbol(klass, module_name)
        if not symbol or symbol in seen or symbol in skip_symbols:
            continue
        stub = public_class_stub_source(
            root,
            klass,
            module_name,
            renames,
            module_symbols,
            import_targets,
            internal_roots,
        )
        if stub:
            for line in stub.import_lines:
                if line not in existing_source and line not in extra_import_lines:
                    extra_import_lines.append(line)
            body.append(f"# {klass.source}:{klass.line}")
            body.append(stub.source)
        else:
            body.append(f"class {symbol}:  # {klass.source}:{klass.line}")
            body.append("    ...")
        body.append("")
        seen.add(symbol)
    if not body:
        return []
    type_checking_lines = type_checking_import_lines(root, module_classes, existing_source)
    if type_checking_lines and TYPE_CHECKING_IMPORT_LINE not in existing_source:
        if TYPE_CHECKING_IMPORT_LINE not in extra_import_lines:
            extra_import_lines.append(TYPE_CHECKING_IMPORT_LINE)
    header.extend(extra_import_lines)
    if extra_import_lines:
        header.append("")
    header.extend(type_checking_lines)
    return header + body


def append_class_stubs(
    out_dir: Path,
    root: Path,
    classes: list[BindingClass],
    module_names: set[str] | None = None,
) -> int:
    module_names = module_names or module_names_from_classes(classes)
    suppressed = 0
    for module_name, group in sorted(group_classes_by_module(classes).items()):
        path = module_stub_path(out_dir, module_name, module_names)
        path.parent.mkdir(parents=True, exist_ok=True)
        existing = path.read_text(encoding="utf-8") if path.exists() else ""
        existing_symbols = public_stub_symbols(existing)
        suppressed += sum(
            1
            for klass in group
            if (symbol := class_public_symbol(klass, module_name)) and symbol in existing_symbols
        )
        class_lines = "\n".join(
            class_stub_lines(
                root,
                group,
                module_name,
                all_classes=classes,
                include_future_import=not existing.strip(),
                skip_symbols=existing_symbols,
                existing_source=existing,
            )
        ).rstrip()
        if not class_lines:
            continue
        separator = "\n\n" if existing else ""
        path.write_text(existing.rstrip() + separator + class_lines + "\n", encoding="utf-8")
    return suppressed
