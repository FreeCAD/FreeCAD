# pyright: strict

"""Class-level merge helpers for public stub assembly.

This module owns the class-shaped half of the merge stage:
- map binding classes onto canonical public module/symbol targets
- plan alias exports for multiply-exposed classes
- rewrite binding class ASTs into public import-shaped class definitions
- preserve binding-specific support imports and declarations in ``ApiClass``
- render complete model classes into the final public module tree

In the overall pipeline this sits on top of the generic module merge helpers in
``module_merge``. That module handles package paths and module-body merges;
this one handles class bodies, imports, aliases, and class placement.
"""

from __future__ import annotations

import ast
import copy
from dataclasses import replace
from pathlib import Path
from typing import cast

from .deprecation import literal_keyword_values, structured_deprecation_message
from .model import (
    BindingClass,
    HELPER_PYI_FILES,
    ImportBinding,
    ImportTarget,
    PUBLIC_STUB_DECORATORS,
    PublicClassStub,
)
from .module_merge import (
    api_attribute_source,
    class_body_defined_symbols,
    import_stmt_line,
    module_stub_path,
    public_stub_symbols,
    merged_module_source,
    top_level_symbol_names,
    type_checking_test,
)
from .naming import valid_identifier
from .parsing import decorator_name, parse_python_source
from .python_api.extract import class_from_node
from .python_api.model import ApiClass, ApiModel, ApiModule, ApiOrigin
from .render import render_docstring_lines
from .signature_parser import CallableSignature

DEPRECATED_IMPORT_LINE = "from typing_extensions import deprecated"


def normalized_deprecated_decorator(decorator: ast.expr) -> ast.expr:
    if decorator_name(decorator).split(".", 1)[-1] != "deprecated":
        return decorator
    if not isinstance(decorator, ast.Call):
        raise ValueError("deprecated must be called with structured lifecycle metadata")
    if decorator.args:
        raise ValueError("structured deprecated() metadata accepts only keyword arguments")

    kwargs = literal_keyword_values(decorator, "deprecated() metadata")
    message = structured_deprecation_message(kwargs)
    if message is None:
        raise ValueError("deprecated() requires structured lifecycle metadata")
    return ast.Call(
        func=ast.Name(id="deprecated", ctx=ast.Load()),
        args=[ast.Constant(value=message)],
        keywords=[],
    )


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
        self.current_deprecated_attributes: dict[str, str] = {}
        self.needs_deprecated_import = False

    def public_decorators(self, decorators: list[ast.expr]) -> list[ast.expr]:
        normalized = [
            normalized_deprecated_decorator(decorator)
            for decorator in decorators
            if keep_public_stub_decorator(decorator)
        ]
        if any(
            decorator_name(decorator).split(".", 1)[-1] == "deprecated" for decorator in normalized
        ):
            self.needs_deprecated_import = True
        return normalized

    @staticmethod
    def deprecated_attributes(node: ast.ClassDef) -> dict[str, str]:
        deprecated_attributes: dict[str, str] = {}
        for decorator in node.decorator_list:
            if decorator_name(decorator).split(".", 1)[-1] != "deprecated_attributes":
                continue
            if not isinstance(decorator, ast.Call):
                continue
            values = literal_keyword_values(decorator, "deprecated_attributes() metadata")
            for name, value in values.items():
                if not isinstance(value, dict):
                    raise ValueError(
                        f"deprecated attribute '{name}' metadata must be a structured mapping"
                    )
                message = structured_deprecation_message(cast(dict[str, object], value))
                if message is None:
                    raise ValueError(
                        f"deprecated attribute '{name}' metadata requires lifecycle fields"
                    )
                deprecated_attributes[name] = message
        return deprecated_attributes

    @staticmethod
    def docstring_expr(node: ast.stmt) -> str | None:
        if (
            isinstance(node, ast.Expr)
            and isinstance(node.value, ast.Constant)
            and isinstance(node.value.value, str)
        ):
            return node.value.value
        return None

    @staticmethod
    def is_final_annotation(annotation: ast.expr) -> bool:
        if isinstance(annotation, ast.Name):
            return annotation.id == "Final"
        if isinstance(annotation, ast.Attribute):
            return annotation.attr == "Final"
        if isinstance(annotation, ast.Subscript):
            return PublicClassStubTransformer.is_final_annotation(annotation.value)
        return False

    @staticmethod
    def property_annotation(annotation: ast.expr) -> ast.expr:
        if isinstance(annotation, ast.Subscript) and PublicClassStubTransformer.is_final_annotation(
            annotation
        ):
            return copy.deepcopy(annotation.slice)
        return copy.deepcopy(annotation)

    @staticmethod
    def ellipsis_body(doc: str | None = None) -> list[ast.stmt]:
        body: list[ast.stmt] = []
        if doc:
            body.append(ast.Expr(value=ast.Constant(value=doc)))
        body.append(ast.Expr(value=ast.Constant(value=Ellipsis)))
        return body

    def deprecated_property_nodes(
        self,
        node: ast.AnnAssign,
        doc: str | None,
    ) -> list[ast.stmt] | None:
        if not isinstance(node.target, ast.Name):
            return None
        message = self.current_deprecated_attributes.get(node.target.id)
        if message is None:
            return None

        annotation = self.rewrite_annotation(node.annotation)
        if annotation is None:
            raise ValueError("annotated assignment must keep an annotation")
        property_annotation = self.property_annotation(annotation)
        self.needs_deprecated_import = True

        getter = ast.FunctionDef(
            name=node.target.id,
            args=ast.arguments(
                posonlyargs=[],
                args=[ast.arg(arg="self")],
                vararg=None,
                kwonlyargs=[],
                kw_defaults=[],
                kwarg=None,
                defaults=[],
            ),
            body=self.ellipsis_body(doc),
            decorator_list=[
                ast.Name(id="property", ctx=ast.Load()),
                ast.Call(
                    func=ast.Name(id="deprecated", ctx=ast.Load()),
                    args=[ast.Constant(value=message)],
                    keywords=[],
                ),
            ],
            returns=property_annotation,
            type_comment=None,
            type_params=[],
        )
        nodes: list[ast.stmt] = [getter]

        if self.is_final_annotation(annotation):
            return nodes

        setter = ast.FunctionDef(
            name=node.target.id,
            args=ast.arguments(
                posonlyargs=[],
                args=[
                    ast.arg(arg="self"),
                    ast.arg(arg="value", annotation=copy.deepcopy(property_annotation)),
                ],
                vararg=None,
                kwonlyargs=[],
                kw_defaults=[],
                kwarg=None,
                defaults=[],
            ),
            body=self.ellipsis_body(),
            decorator_list=[
                ast.Attribute(
                    value=ast.Name(id=node.target.id, ctx=ast.Load()),
                    attr="setter",
                    ctx=ast.Load(),
                ),
                ast.Call(
                    func=ast.Name(id="deprecated", ctx=ast.Load()),
                    args=[ast.Constant(value=message)],
                    keywords=[],
                ),
            ],
            returns=ast.Constant(value=None),
            type_comment=None,
            type_params=[],
        )
        nodes.append(setter)
        return nodes

    def transform_class_body(self, body: list[ast.stmt]) -> list[ast.stmt]:
        transformed: list[ast.stmt] = []
        index = 0
        while index < len(body):
            item = body[index]
            attribute_doc: str | None = None
            doc_index: int | None = None
            if isinstance(item, ast.AnnAssign) and index + 1 < len(body):
                attribute_doc = self.docstring_expr(body[index + 1])
                if attribute_doc is not None:
                    doc_index = index + 1

            if isinstance(item, ast.AnnAssign):
                deprecated_nodes = self.deprecated_property_nodes(item, attribute_doc)
                if deprecated_nodes is not None:
                    transformed.extend(deprecated_nodes)
                    index += 2 if doc_index is not None else 1
                    continue

            visited = self.visit(item)
            if isinstance(visited, list):
                children = cast(list[object], visited)
                for child in children:
                    if not isinstance(child, ast.stmt):
                        raise TypeError("class transformer produced a non-statement node")
                    transformed.append(child)
            else:
                if not isinstance(visited, ast.stmt):
                    raise TypeError("class transformer produced a non-statement node")
                transformed.append(visited)
            index += 1
        return transformed

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
        deprecated_attributes = self.deprecated_attributes(node) if is_public_class else {}
        if is_public_class:
            node.name = self.public_symbol
        node.decorator_list = self.public_decorators(node.decorator_list)
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
        previous_deprecated_attributes = self.current_deprecated_attributes
        self.current_deprecated_attributes = deprecated_attributes
        self.class_depth += 1
        try:
            node.body = self.transform_class_body(node.body)
            flattened: list[ast.stmt] = []
            for item in node.body:
                if isinstance(item, ast.If) and type_checking_test(item.test) and not item.orelse:
                    flattened.extend(item.body)
                else:
                    flattened.append(item)
            node.body = flattened
        finally:
            self.class_depth -= 1
            self.current_deprecated_attributes = previous_deprecated_attributes
        return node

    def visit_FunctionDef(self, node: ast.FunctionDef) -> ast.FunctionDef:
        node.decorator_list = self.public_decorators(node.decorator_list)
        node.args = self.visit(node.args)
        node.returns = self.rewrite_annotation(node.returns)
        node.body = [self.visit(item) for item in node.body]
        return node

    def visit_AsyncFunctionDef(self, node: ast.AsyncFunctionDef) -> ast.AsyncFunctionDef:
        node.decorator_list = self.public_decorators(node.decorator_list)
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
    if transformer.needs_deprecated_import and DEPRECATED_IMPORT_LINE not in import_lines:
        import_lines.append(DEPRECATED_IMPORT_LINE)
    for root_name in sorted(transformer.annotation_module_roots_needed):
        line = f"import {root_name}"
        if line not in import_lines:
            import_lines.insert(0, line)
    return PublicClassStub(
        source=ast.unparse(transformed),
        import_lines=tuple(import_lines),
    )


def normalize_api_model_binding_class_headers(
    root: Path,
    classes: list[BindingClass],
    api_model: ApiModel,
) -> ApiModel:
    """Normalize binding-class bases using the public class transformation rules."""

    grouped = group_classes_by_module(classes)
    import_targets = public_import_target_index(classes)
    internal_roots = known_stub_module_roots(classes)
    model_modules: list[ApiModule] = []
    for module in api_model.modules:
        api_classes: list[ApiClass] = []
        module_classes = grouped.get(module.name, [])
        module_symbols = {
            symbol
            for klass in module_classes
            if (symbol := class_public_symbol(klass, module.name)) is not None
        }
        renames = module_symbol_renames(module_classes, module.name)
        for api_class in module.classes:
            if api_class.origin != ApiOrigin.BINDING_SPEC:
                api_classes.append(api_class)
                continue
            binding_class = next(
                (
                    klass
                    for klass in module_classes
                    if class_public_symbol(klass, module.name) == api_class.name
                ),
                None,
            )
            if binding_class is None:
                api_classes.append(api_class)
                continue
            stub = public_class_stub_source(
                root,
                binding_class,
                module.name,
                renames,
                module_symbols,
                import_targets,
                internal_roots,
            )
            if stub is None:
                api_classes.append(api_class)
                continue
            tree = ast.parse(stub.source)
            class_node = next(
                (node for node in tree.body if isinstance(node, ast.ClassDef)),
                None,
            )
            if class_node is None:
                api_classes.append(api_class)
                continue
            normalized_class = ast_class_api_model(
                root,
                binding_class,
                module.name,
                class_node,
                api_class,
                stub.import_lines,
            )
            api_classes.append(normalized_class)
        model_modules.append(replace(module, classes=tuple(api_classes)))
    return replace(api_model, modules=tuple(model_modules))


def ast_class_api_model(
    root: Path,
    binding_class: BindingClass,
    module_name: str,
    node: ast.ClassDef,
    existing: ApiClass,
    transformed_imports: tuple[str, ...],
) -> ApiClass:
    """Build normalized callable and attribute data from a transformed class AST."""

    path = root / binding_class.source
    transformed = class_from_node(
        root,
        path,
        module_name,
        node,
        origin=ApiOrigin.BINDING_SPEC,
    )
    raw_methods = {group.name: group for group in existing.methods}
    methods = tuple(
        replace(group, location=raw_methods.get(group.name, group).location)
        for group in transformed.methods
    )
    raw_attributes = {attribute.name: attribute for attribute in existing.attributes}
    attributes = tuple(
        replace(attribute, location=raw_attributes.get(attribute.name, attribute).location)
        for attribute in transformed.attributes
    )
    support_imports = list(transformed_imports)
    type_checking_lines = type_checking_import_lines(root, [binding_class])
    if type_checking_lines:
        support_imports.append("\n".join(type_checking_lines).rstrip())
    return replace(
        existing,
        doc=transformed.doc,
        bases=tuple(ast.unparse(base) for base in node.bases),
        methods=methods,
        attributes=attributes,
        decorators=transformed.decorators,
        support_imports=tuple(support_imports),
        support_body=transformed.support_body,
    )


def render_api_class_header(api_class: ApiClass) -> str:
    if not api_class.bases:
        return f"class {api_class.name}:"
    return f"class {api_class.name}({', '.join(api_class.bases)}):"


def render_api_model_class(api_class: ApiClass) -> str:
    """Render one complete public class from the semantic API model."""

    lines = [f"@{decorator}" for decorator in api_class.decorators]
    lines.append(render_api_class_header(api_class))
    body: list[str] = []
    if api_class.doc:
        body.extend(render_docstring_lines(api_class.doc))
    body.extend(f"    {api_attribute_source(attribute)}" for attribute in api_class.attributes)
    for group in api_class.methods:
        for index, signature in enumerate(group.signatures):
            decorators = list(signature.decorators)
            decorator_names = {
                decorator.split("(", 1)[0].split(".", 1)[-1] for decorator in decorators
            }
            if group.overload and "overload" not in decorator_names:
                decorators.insert(0, "overload")
            if signature.flags.classmethod and "classmethod" not in decorator_names:
                decorators.insert(0, "classmethod")
            elif signature.flags.staticmethod and "staticmethod" not in decorator_names:
                decorators.insert(0, "staticmethod")
            body.extend(f"    @{decorator}" for decorator in decorators)
            display = class_method_display_signature(signature)
            body.append(f"    def {display}:")
            doc = signature.docstring or (group.doc if index == 0 else None)
            if doc:
                body.extend(f"    {line}" for line in render_docstring_lines(doc))
            body.append("        ...")
    for support in api_class.support_body:
        body.extend(f"    {line}" for line in support.splitlines())
    if not body:
        body.append("    ...")
    lines.extend(body)
    return "\n".join(lines)


def append_api_model_class_stubs(
    target_dir: Path,
    api_model: ApiModel,
    module_names: set[str],
) -> int:
    """Materialize model classes absent from the existing generated output."""

    count = 0
    for module in api_model.modules:
        if not module.classes:
            continue
        path = module_stub_path(target_dir, module.name, module_names)
        path.parent.mkdir(parents=True, exist_ok=True)
        existing = path.read_text(encoding="utf-8") if path.exists() else ""
        existing_symbols = public_stub_symbols(existing)
        missing = [
            api_class for api_class in module.classes if api_class.name not in existing_symbols
        ]
        if not missing:
            continue
        prefix = existing.rstrip()
        if not prefix:
            prefix = "from __future__ import annotations"
        class_source = "\n\n".join(render_api_model_class(api_class) for api_class in missing)
        required_imports = {
            import_line
            for api_class in missing
            for import_line in api_class.support_imports
            if import_line.strip()
        }
        if "Any" in class_source:
            required_imports.add("from typing import Any")
        if any(group.overload for api_class in missing for group in api_class.methods):
            required_imports.add("from typing import overload")
        for import_line in sorted(required_imports):
            if import_line not in prefix:
                prefix = f"{prefix}\n{import_line}"
        path.write_text(f"{prefix}\n\n{class_source}\n", encoding="utf-8")
        count += len(missing)
    return count


def class_method_display_signature(
    signature: CallableSignature,
) -> str:
    display = signature.display_signature
    if signature.flags.staticmethod:
        return display

    receiver = "cls" if signature.flags.classmethod else "self"
    opening = display.index("(") + 1
    if display[opening] == ")":
        return f"{display[:opening]}{receiver}{display[opening:]}"
    return f"{display[:opening]}{receiver}, {display[opening:]}"
