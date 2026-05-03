# pyright: strict

"""Extract a neutral public API model from curated source-adjacent stub files.

This module is the first bridge between authored ``.pyi`` sources and future
documentation renderers. It reads:

- source-adjacent ``*.module.pyi`` files
- source-adjacent plain ``.pyi`` type stubs

and merges them into one ``ApiModel``. The resulting model is intended to be
shared by both documentation and stub renderers so neither has to parse the
curated stub sources independently.
"""

from __future__ import annotations

import ast
from dataclasses import dataclass, field
from pathlib import Path

from .api_model import (
    ApiAttribute,
    ApiCallableGroup,
    ApiClass,
    ApiModel,
    ApiModule,
    ApiOrigin,
    ApiSourceLocation,
)
from .parsing import iter_module_stub_pyi_files, iter_type_stub_pyi_files
from .signature_parser import (
    CallableSignature,
    group_callable_definitions,
    parse_callable_group,
)
from .source_inputs import parse_type_stub_target


@dataclass
class ApiModuleBuilder:
    """Mutable aggregation helper before freezing a final ``ApiModule``."""

    name: str
    doc: str | None = None
    functions: dict[str, ApiCallableGroup] = field(default_factory=dict)
    classes: dict[str, ApiClass] = field(default_factory=dict)
    attributes: dict[str, ApiAttribute] = field(default_factory=dict)


def module_stub_name(path: Path) -> str:
    suffix = ".module.pyi"
    if not path.name.endswith(suffix):
        raise ValueError(f"{path}: invalid module stub filename")
    return path.name.removesuffix(suffix)


def source_location(root: Path, path: Path, line: int | None = None) -> ApiSourceLocation:
    return ApiSourceLocation(path=path.relative_to(root).as_posix(), line=line)


def public_signatures(signatures: tuple[CallableSignature, ...]) -> tuple[CallableSignature, ...]:
    return tuple(signature for signature in signatures if not signature.flags.typing_only)


def callable_group_from_nodes(
    root: Path,
    path: Path,
    owner: str,
    nodes: list[ast.FunctionDef | ast.AsyncFunctionDef],
    *,
    is_method: bool,
    origin: ApiOrigin,
) -> ApiCallableGroup | None:
    signatures = public_signatures(parse_callable_group(nodes))
    if not signatures:
        return None
    doc = next((signature.docstring for signature in signatures if signature.docstring), None)
    return ApiCallableGroup(
        name=nodes[0].name,
        signatures=signatures,
        doc=doc,
        is_method=is_method,
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
    elif node.value is not None:
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
    origin: ApiOrigin,
) -> ApiAttribute | None:
    name = assignment_name(node)
    if name is None:
        return None
    annotation, value = annotation_value_text(node)
    return ApiAttribute(
        name=name,
        annotation=annotation,
        value=value,
        origin=origin,
        location=source_location(root, path, node.lineno),
    )


def class_attributes(
    root: Path,
    path: Path,
    body: list[ast.stmt],
    *,
    origin: ApiOrigin,
) -> tuple[ApiAttribute, ...]:
    attributes: list[ApiAttribute] = []
    for node in body:
        if isinstance(node, (ast.Assign, ast.AnnAssign)):
            attribute = attribute_from_assignment(root, path, node, origin=origin)
            if attribute is not None:
                attributes.append(attribute)
    return tuple(attributes)


def class_methods(
    root: Path,
    path: Path,
    class_name: str,
    body: list[ast.stmt],
    *,
    origin: ApiOrigin,
) -> tuple[ApiCallableGroup, ...]:
    methods: list[ApiCallableGroup] = []
    for _, group in sorted(group_callable_definitions(body).items()):
        callable_group = callable_group_from_nodes(
            root,
            path,
            class_name,
            group,
            is_method=True,
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
    return ApiClass(
        module_name=module_name,
        name=node.name,
        doc=ast.get_docstring(node, clean=True),
        bases=tuple(ast.unparse(base) for base in node.bases),
        methods=class_methods(root, path, node.name, node.body, origin=origin),
        attributes=class_attributes(root, path, node.body, origin=origin),
        origin=origin,
        location=source_location(root, path, node.lineno),
    )


def module_attributes(
    root: Path,
    path: Path,
    body: list[ast.stmt],
    *,
    origin: ApiOrigin,
) -> tuple[ApiAttribute, ...]:
    attributes: list[ApiAttribute] = []
    for node in body:
        if isinstance(node, (ast.Assign, ast.AnnAssign)):
            attribute = attribute_from_assignment(root, path, node, origin=origin)
            if attribute is not None:
                attributes.append(attribute)
    return tuple(attributes)


def merge_module_piece(builder: ApiModuleBuilder, piece: ApiModule) -> None:
    if piece.doc and not builder.doc:
        builder.doc = piece.doc
    builder.functions.update({group.name: group for group in piece.functions})
    builder.classes.update({klass.name: klass for klass in piece.classes})
    builder.attributes.update({attribute.name: attribute for attribute in piece.attributes})


def module_from_stub_file(
    root: Path,
    path: Path,
    module_name: str,
    *,
    origin: ApiOrigin,
    include_module_doc: bool,
) -> ApiModule:
    source = path.read_text(encoding="utf-8")
    tree = ast.parse(source, filename=str(path))

    functions: list[ApiCallableGroup] = []
    for _, group in sorted(group_callable_definitions(tree.body).items()):
        callable_group = callable_group_from_nodes(
            root,
            path,
            module_name,
            group,
            is_method=False,
            origin=origin,
        )
        if callable_group is not None:
            functions.append(callable_group)

    classes = tuple(
        class_from_node(root, path, module_name, node, origin=origin)
        for node in tree.body
        if isinstance(node, ast.ClassDef)
    )
    attributes = module_attributes(root, path, tree.body, origin=origin)

    return ApiModule(
        name=module_name,
        doc=ast.get_docstring(tree, clean=True) if include_module_doc else None,
        functions=tuple(functions),
        classes=classes,
        attributes=attributes,
        origin=origin,
        location=source_location(root, path, 1),
    )


def extract_curated_api_model(root: Path, source_dir: Path) -> ApiModel:
    """Build a neutral API model from curated source-adjacent stub inputs."""

    modules: dict[str, ApiModuleBuilder] = {}

    for path in sorted(iter_module_stub_pyi_files(root, source_dir)):
        piece = module_from_stub_file(
            root,
            path,
            module_stub_name(path),
            origin=ApiOrigin.MODULE_STUB,
            include_module_doc=True,
        )
        builder = modules.setdefault(piece.name, ApiModuleBuilder(name=piece.name))
        merge_module_piece(builder, piece)

    for path in sorted(iter_type_stub_pyi_files(root, source_dir)):
        module_name, _ = parse_type_stub_target(path)
        piece = module_from_stub_file(
            root,
            path,
            module_name,
            origin=ApiOrigin.TYPE_STUB,
            include_module_doc=False,
        )
        builder = modules.setdefault(piece.name, ApiModuleBuilder(name=piece.name))
        merge_module_piece(builder, piece)

    return ApiModel(
        modules=tuple(
            ApiModule(
                name=builder.name,
                doc=builder.doc,
                functions=tuple(builder.functions[name] for name in sorted(builder.functions)),
                classes=tuple(builder.classes[name] for name in sorted(builder.classes)),
                attributes=tuple(builder.attributes[name] for name in sorted(builder.attributes)),
            )
            for builder in (modules[name] for name in sorted(modules))
        )
    )
