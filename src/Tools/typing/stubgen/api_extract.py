# pyright: strict

"""Extract a neutral public API model from curated source-adjacent stub files.

This module is the first bridge between authored ``.pyi`` sources and future
documentation renderers. It reads:

- source-adjacent ``*.module.pyi`` files
- source-adjacent plain ``.pyi`` type stubs

and merges them into one ``PythonApiModel``. The resulting model is intended to
be shared by stub and documentation renderers so neither has to parse the
curated stub sources independently.
"""

from __future__ import annotations

import ast
from collections.abc import Callable, Iterable, Mapping
from dataclasses import replace
from pathlib import Path

from python_api_model.model import (
    ApiAttribute,
    ApiAlias,
    ApiCallableGroup,
    ApiClass,
    PythonApiModel,
    ApiModule,
    ApiOrigin,
    ApiSourceLocation,
)
from python_api_model.diagnostics import MergeDiagnostic
from python_api_model.normalize import normalize_signature_types, normalize_source_type
from python_api_model.resolve import merge_api_module
from python_api_model.signatures import (
    CallableSignature,
    group_callable_definitions,
    parse_callable_group,
)

from .model import BindingClass
from .decorators import public_decorators, strip_binding_decorators
from .module_merge import overlay_module_name
from .naming import valid_identifier
from .parsing import (
    iter_module_stub_pyi_files,
    iter_type_stub_pyi_files,
)
from .source_inputs import binding_class_source, parse_type_stub_target
from .source_inputs import deprecated_attribute_messages, deprecated_message_from_function_node


def module_stub_name(path: Path) -> str:
    suffix = ".module.pyi"
    if not path.name.endswith(suffix):
        raise ValueError(f"{path}: invalid module stub filename")
    return path.name.removesuffix(suffix)


def class_is_protocol(node: ast.ClassDef) -> bool:
    base_names = [ast.unparse(base).split(".", 1)[-1] for base in node.bases]
    return "Protocol" in base_names


def include_module_stub_class(node: ast.ClassDef) -> bool:
    if class_is_protocol(node):
        return False
    return not node.name.startswith("_")


def source_location(root: Path, path: Path, line: int | None = None) -> ApiSourceLocation:
    return ApiSourceLocation(path=path.relative_to(root).as_posix(), line=line)


def callable_group_from_nodes(
    root: Path,
    path: Path,
    nodes: list[ast.FunctionDef | ast.AsyncFunctionDef],
    *,
    module_name: str,
    origin: ApiOrigin,
) -> ApiCallableGroup | None:
    signature_list: list[CallableSignature] = []
    for node, signature in zip(nodes, parse_callable_group(nodes)):
        signature = strip_binding_decorators(signature)
        deprecated_message = signature.deprecated_message
        if deprecated_message is None:
            deprecated_message = deprecated_message_from_function_node(node)
        if deprecated_message is not None:
            signature = replace(signature, deprecated_message=deprecated_message)
        signature_list.append(normalize_signature_types(signature, module_name))
    signatures = tuple(signature_list)
    if not signatures:
        return None
    doc = next((signature.docstring for signature in signatures if signature.docstring), None)
    return ApiCallableGroup(
        name=nodes[0].name,
        signatures=signatures,
        doc=doc,
        origin=origin,
        location=source_location(root, path, nodes[0].lineno),
    )


def annotation_value_text(node: ast.Assign | ast.AnnAssign) -> tuple[str | None, str | None]:
    annotation: str | None = None
    value: str | None = None
    if isinstance(node, ast.AnnAssign):
        annotation = ast.unparse(node.annotation)
        if node.value is not None:
            value = ast.unparse(node.value)
    else:
        value = ast.unparse(node.value)
    return annotation, value


def assignment_name(node: ast.Assign | ast.AnnAssign) -> str | None:
    if isinstance(node, ast.AnnAssign):
        return node.target.id if isinstance(node.target, ast.Name) else None
    if len(node.targets) != 1:
        return None
    target = node.targets[0]
    return target.id if isinstance(target, ast.Name) else None


def attribute_from_assignment(
    root: Path,
    path: Path,
    node: ast.Assign | ast.AnnAssign,
    *,
    module_name: str,
    origin: ApiOrigin,
    doc: str | None = None,
    deprecated_message: str | None = None,
) -> ApiAttribute | None:
    name = assignment_name(node)
    if name is None or name.startswith("_"):
        return None
    annotation, value = annotation_value_text(node)
    return ApiAttribute(
        name=name,
        annotation=normalize_source_type(annotation, module_name),
        value=value,
        doc=doc,
        deprecated_message=deprecated_message,
        origin=origin,
        location=source_location(root, path, node.lineno),
    )


def attributes_from_body(
    root: Path,
    path: Path,
    body: list[ast.stmt],
    *,
    module_name: str,
    origin: ApiOrigin,
    deprecated_messages: Mapping[str, str] | None = None,
) -> tuple[ApiAttribute, ...]:
    """Extract public assignments from either a module or class body."""

    deprecated_messages = deprecated_messages or {}
    attributes: list[ApiAttribute] = []
    for index, node in enumerate(body):
        if not isinstance(node, (ast.Assign, ast.AnnAssign)):
            continue
        doc = None
        if index + 1 < len(body):
            following = body[index + 1]
            if (
                isinstance(following, ast.Expr)
                and isinstance(following.value, ast.Constant)
                and isinstance(following.value.value, str)
            ):
                doc = following.value.value
        attribute = attribute_from_assignment(
            root,
            path,
            node,
            module_name=module_name,
            origin=origin,
            doc=doc,
            deprecated_message=deprecated_messages.get(assignment_name(node) or ""),
        )
        if attribute is not None:
            attributes.append(attribute)
    return tuple(attributes)


def class_methods(
    root: Path,
    path: Path,
    body: list[ast.stmt],
    *,
    module_name: str,
    origin: ApiOrigin,
) -> tuple[ApiCallableGroup, ...]:
    methods: list[ApiCallableGroup] = []
    for _, group in sorted(group_callable_definitions(body).items()):
        callable_group = callable_group_from_nodes(
            root,
            path,
            group,
            module_name=module_name,
            origin=origin,
        )
        if callable_group is not None:
            methods.append(callable_group)
    return tuple(methods)


def class_from_node(
    root: Path,
    path: Path,
    module_name: str,
    node: ast.ClassDef,
    *,
    origin: ApiOrigin,
) -> ApiClass:
    deprecated_messages = deprecated_attribute_messages(node)
    attributes = attributes_from_body(
        root,
        path,
        node.body,
        module_name=module_name,
        origin=origin,
        deprecated_messages=deprecated_messages,
    )
    declared_attributes = {attribute.name for attribute in attributes}
    unknown_attributes = sorted(set(deprecated_messages) - declared_attributes)
    if unknown_attributes:
        joined = ", ".join(unknown_attributes)
        raise ValueError(f"Unknown deprecated attribute metadata for class '{node.name}': {joined}")
    bases: list[str] = []
    for base in node.bases:
        base_text = ast.unparse(base)
        if base_text.split(".", 1)[-1] == "PyObjectBase":
            continue
        bases.append(normalize_source_type(base_text, module_name) or base_text)
    return ApiClass(
        module_name=module_name,
        name=node.name,
        doc=ast.get_docstring(node, clean=True),
        bases=tuple(bases),
        methods=class_methods(
            root,
            path,
            node.body,
            module_name=module_name,
            origin=origin,
        ),
        attributes=attributes,
        decorators=public_decorators(
            tuple(ast.unparse(decorator) for decorator in node.decorator_list)
        ),
        origin=origin,
        location=source_location(root, path, node.lineno),
    )


def module_classes(
    root: Path,
    path: Path,
    module_name: str,
    body: list[ast.stmt],
    *,
    origin: ApiOrigin,
    include_class: Callable[[ast.ClassDef], bool] | None = None,
) -> tuple[ApiClass, ...]:
    classes: list[ApiClass] = []
    for node in body:
        if not isinstance(node, ast.ClassDef):
            continue
        if include_class is not None and not include_class(node):
            continue
        classes.append(class_from_node(root, path, module_name, node, origin=origin))
    return tuple(classes)


def merge_piece(
    modules: dict[str, ApiModule],
    piece: ApiModule,
    diagnostics: list[MergeDiagnostic],
) -> None:
    """Merge one source piece through the shared API resolution policy."""

    current = modules.get(piece.name)
    if current is None:
        modules[piece.name] = piece
        return

    result = merge_api_module(current, piece)
    modules[piece.name] = result.value
    diagnostics.extend(result.diagnostics)


def binding_class_aliases(classes: Iterable[BindingClass]) -> tuple[ApiAlias, ...]:
    """Convert multi-public binding classes into semantic API re-exports."""

    aliases: dict[str, ApiAlias] = {}
    for klass in classes:
        targets = binding_class_targets(klass)
        if len(targets) < 2:
            continue

        canonical = binding_class_canonical_target(klass)
        if canonical is None:
            continue
        target_path = ".".join(canonical)
        for public_module, public_symbol in targets:
            public_path = f"{public_module}.{public_symbol}"
            if (public_module, public_symbol) == canonical:
                continue
            aliases[public_path] = ApiAlias(
                public_path=public_path,
                target_path=target_path,
                origin=ApiOrigin.BINDING_SPEC,
                location=ApiSourceLocation(klass.source, klass.line),
            )
    return tuple(aliases[name] for name in sorted(aliases))


def binding_class_targets(klass: BindingClass) -> list[tuple[str, str]]:
    targets: list[tuple[str, str]] = []
    for public_name in klass.public_names:
        if "." not in public_name:
            continue
        module_name, symbol = public_name.rsplit(".", 1)
        if not module_name or not valid_identifier(symbol):
            continue
        target = (module_name, symbol)
        if target not in targets:
            targets.append(target)
    return targets


def binding_class_canonical_target(klass: BindingClass) -> tuple[str, str] | None:
    targets = binding_class_targets(klass)
    if not targets:
        return None
    if klass.source.startswith("src/Base/"):
        return next(
            (target for target in targets if target[0] == "FreeCAD.Base"),
            targets[0],
        )
    return targets[0]


def binding_class_from_source(
    root: Path,
    klass: BindingClass,
    module_name: str,
    public_symbol: str,
) -> ApiClass:
    source = binding_class_source(root, klass)
    path = source.path
    node = source.class_node
    if node is None:
        return ApiClass(
            module_name=module_name,
            name=public_symbol,
            bases=(klass.base_class,) if klass.base_class else (),
            origin=ApiOrigin.BINDING_SPEC,
            location=ApiSourceLocation(klass.source, klass.line),
        )

    source_class = class_from_node(
        root,
        path,
        module_name,
        node,
        origin=ApiOrigin.BINDING_SPEC,
    )
    return replace(source_class, module_name=module_name, name=public_symbol)


def module_from_source(
    root: Path,
    path: Path,
    source: str,
    module_name: str,
    *,
    origin: ApiOrigin,
    include_module_doc: bool,
    include_attributes: bool = True,
    include_class: Callable[[ast.ClassDef], bool] | None = None,
) -> ApiModule:
    tree = ast.parse(source, filename=str(path))

    functions: list[ApiCallableGroup] = []
    for _, group in sorted(group_callable_definitions(tree.body).items()):
        callable_group = callable_group_from_nodes(
            root,
            path,
            group,
            module_name=module_name,
            origin=origin,
        )
        if callable_group is not None:
            functions.append(callable_group)

    classes = module_classes(
        root,
        path,
        module_name,
        tree.body,
        origin=origin,
        include_class=include_class,
    )
    attributes = (
        attributes_from_body(
            root,
            path,
            tree.body,
            module_name=module_name,
            origin=origin,
        )
        if include_attributes
        else ()
    )

    return ApiModule(
        name=module_name,
        doc=ast.get_docstring(tree, clean=True) if include_module_doc else None,
        functions=tuple(functions),
        classes=classes,
        attributes=attributes,
        origin=origin,
        location=source_location(root, path, 1),
    )


def module_from_stub_file(
    root: Path,
    path: Path,
    module_name: str,
    *,
    origin: ApiOrigin,
    include_module_doc: bool,
    include_attributes: bool = True,
    include_class: Callable[[ast.ClassDef], bool] | None = None,
) -> ApiModule:
    return module_from_source(
        root,
        path,
        path.read_text(encoding="utf-8"),
        module_name,
        origin=origin,
        include_module_doc=include_module_doc,
        include_attributes=include_attributes,
        include_class=include_class,
    )


def extract_curated_api_model_with_diagnostics(
    root: Path,
    source_dir: Path,
    binding_classes: Iterable[BindingClass] = (),
    overlay_dir: Path | None = None,
) -> tuple[PythonApiModel, tuple[MergeDiagnostic, ...]]:
    """Build an API model and report conflicts between its input layers."""

    modules: dict[str, ApiModule] = {}
    diagnostics: list[MergeDiagnostic] = []
    binding_classes = tuple(binding_classes)

    for path in sorted(iter_module_stub_pyi_files(root, source_dir)):
        piece = module_from_stub_file(
            root,
            path,
            module_stub_name(path),
            origin=ApiOrigin.MODULE_STUB,
            include_module_doc=True,
            include_class=include_module_stub_class,
        )
        merge_piece(modules, piece, diagnostics)

    for klass in binding_classes:
        target = binding_class_canonical_target(klass)
        if target is None:
            continue
        module_name, public_symbol = target
        merge_piece(
            modules,
            ApiModule(
                name=module_name,
                classes=(binding_class_from_source(root, klass, module_name, public_symbol),),
                origin=ApiOrigin.BINDING_SPEC,
                location=ApiSourceLocation(klass.source, klass.line),
            ),
            diagnostics,
        )

    for path in sorted(iter_type_stub_pyi_files(root, source_dir)):
        target = parse_type_stub_target(path)
        module_name = target.module_name
        class_symbol = target.class_name
        piece = module_from_stub_file(
            root,
            path,
            module_name,
            origin=ApiOrigin.TYPE_STUB,
            include_module_doc=False,
            include_attributes=False,
            include_class=lambda node, class_symbol=class_symbol: node.name == class_symbol,
        )
        merge_piece(modules, piece, diagnostics)

    if overlay_dir is not None and overlay_dir.exists():
        for path in sorted(overlay_dir.rglob("*.pyi")):
            module_name = overlay_module_name(path.relative_to(overlay_dir))
            if module_name is None:
                continue
            piece = module_from_stub_file(
                root,
                path,
                module_name,
                origin=ApiOrigin.OVERLAY,
                include_module_doc=True,
            )
            merge_piece(modules, piece, diagnostics)

    for alias in binding_class_aliases(binding_classes):
        module_name = alias.public_path.rsplit(".", 1)[0]
        merge_piece(
            modules,
            ApiModule(
                name=module_name, aliases=(alias,), origin=alias.origin, location=alias.location
            ),
            diagnostics,
        )

    return PythonApiModel(modules=tuple(modules[name] for name in sorted(modules))), tuple(
        diagnostics
    )


def extract_curated_api_model(
    root: Path,
    source_dir: Path,
    binding_classes: Iterable[BindingClass] = (),
) -> PythonApiModel:
    """Build a neutral API model from curated source-adjacent stub inputs."""

    model, _ = extract_curated_api_model_with_diagnostics(
        root,
        source_dir,
        binding_classes=binding_classes,
    )
    return model
