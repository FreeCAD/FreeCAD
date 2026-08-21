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
from collections.abc import Callable, Iterable
from dataclasses import dataclass, field
from pathlib import Path

from .model import (
    ApiAttribute,
    ApiAlias,
    ApiCallableGroup,
    ApiClass,
    ApiModel,
    ApiModule,
    ApiOrigin,
    ApiSourceLocation,
)
from ..naming import valid_identifier
from ..model import BindingClass
from ..parsing import (
    iter_module_stub_pyi_files,
    iter_type_stub_pyi_files,
    parse_python_source,
)
from ..signature_parser import (
    CallableSignature,
    group_callable_definitions,
    parse_callable_group,
)
from ..source_inputs import parse_type_stub_target


@dataclass
class ApiModuleBuilder:
    """Mutable aggregation helper before freezing a final ``ApiModule``."""

    name: str
    doc: str | None = None
    origin: ApiOrigin = ApiOrigin.GENERATED
    location: ApiSourceLocation | None = None
    functions: dict[str, ApiCallableGroup] = field(
        default_factory=lambda: dict[str, ApiCallableGroup]()
    )
    classes: dict[str, ApiClass] = field(default_factory=lambda: dict[str, ApiClass]())
    attributes: dict[str, ApiAttribute] = field(default_factory=lambda: dict[str, ApiAttribute]())
    aliases: dict[str, ApiAlias] = field(default_factory=lambda: dict[str, ApiAlias]())


def module_stub_name(path: Path) -> str:
    suffix = ".module.pyi"
    if not path.name.endswith(suffix):
        raise ValueError(f"{path}: invalid module stub filename")
    return path.name.removesuffix(suffix)


def class_is_protocol(node: ast.ClassDef) -> bool:
    return any(ast.unparse(base).split(".", 1)[-1] == "Protocol" for base in node.bases)


def include_module_stub_class(node: ast.ClassDef) -> bool:
    if class_is_protocol(node):
        return False
    return not node.name.startswith("_")


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
    if piece.location and builder.location is None:
        builder.location = piece.location
        builder.origin = piece.origin
    builder.functions.update({group.name: group for group in piece.functions})
    builder.classes.update({klass.name: klass for klass in piece.classes})
    builder.attributes.update({attribute.name: attribute for attribute in piece.attributes})
    builder.aliases.update({alias.public_path: alias for alias in piece.aliases})


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
    path = root / klass.source
    tree = parse_python_source(path)
    node = next(
        (
            node
            for node in (tree.body if tree is not None else [])
            if isinstance(node, ast.ClassDef)
            and node.name == klass.class_name
            and node.lineno == klass.line
        ),
        None,
    )
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
    return ApiClass(
        module_name=module_name,
        name=public_symbol,
        doc=source_class.doc,
        bases=source_class.bases,
        methods=source_class.methods,
        attributes=source_class.attributes,
        origin=source_class.origin,
        location=source_class.location,
    )


def merge_class_piece(builder: ApiModuleBuilder, piece: ApiClass) -> None:
    existing = builder.classes.get(piece.name)
    if existing is None:
        builder.classes[piece.name] = piece
        return
    builder.classes[piece.name] = ApiClass(
        module_name=existing.module_name,
        name=existing.name,
        doc=existing.doc or piece.doc,
        bases=existing.bases or piece.bases,
        methods=existing.methods or piece.methods,
        attributes=existing.attributes or piece.attributes,
        aliases=existing.aliases or piece.aliases,
        origin=existing.origin,
        location=existing.location or piece.location,
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

    classes = module_classes(
        root,
        path,
        module_name,
        tree.body,
        origin=origin,
        include_class=include_class,
    )
    attributes = (
        module_attributes(root, path, tree.body, origin=origin) if include_attributes else ()
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


def extract_curated_api_model(
    root: Path,
    source_dir: Path,
    binding_classes: Iterable[BindingClass] = (),
) -> ApiModel:
    """Build a neutral API model from curated source-adjacent stub inputs."""

    modules: dict[str, ApiModuleBuilder] = {}
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
        builder = modules.setdefault(piece.name, ApiModuleBuilder(name=piece.name))
        merge_module_piece(builder, piece)

    for klass in binding_classes:
        target = binding_class_canonical_target(klass)
        if target is None:
            continue
        module_name, public_symbol = target
        builder = modules.setdefault(module_name, ApiModuleBuilder(name=module_name))
        merge_class_piece(
            builder,
            binding_class_from_source(root, klass, module_name, public_symbol),
        )

    for path in sorted(iter_type_stub_pyi_files(root, source_dir)):
        module_name, class_symbol = parse_type_stub_target(path)
        piece = module_from_stub_file(
            root,
            path,
            module_name,
            origin=ApiOrigin.TYPE_STUB,
            include_module_doc=False,
            include_attributes=False,
            include_class=lambda node, class_symbol=class_symbol: node.name == class_symbol,
        )
        builder = modules.setdefault(piece.name, ApiModuleBuilder(name=piece.name))
        merge_module_piece(builder, piece)

    for alias in binding_class_aliases(binding_classes):
        module_name = alias.public_path.rsplit(".", 1)[0]
        builder = modules.setdefault(module_name, ApiModuleBuilder(name=module_name))
        builder.aliases[alias.public_path] = alias

    return ApiModel(
        modules=tuple(
            ApiModule(
                name=builder.name,
                doc=builder.doc,
                functions=tuple(builder.functions[name] for name in sorted(builder.functions)),
                classes=tuple(builder.classes[name] for name in sorted(builder.classes)),
                attributes=tuple(builder.attributes[name] for name in sorted(builder.attributes)),
                aliases=tuple(builder.aliases[name] for name in sorted(builder.aliases)),
                origin=builder.origin,
                location=builder.location,
            )
            for builder in (modules[name] for name in sorted(modules))
        )
    )
