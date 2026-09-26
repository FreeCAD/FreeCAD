# pyright: strict

"""Render the normalized Python API model as public ``.pyi`` modules."""

from __future__ import annotations

import ast
from pathlib import Path

from python_api_model.signatures import (
    ArgumentKind,
    CallableDecoratorFlags,
    CallableSignature,
    SignatureParameter,
)

from python_api_model.model import ApiAttribute, ApiCallableGroup, ApiClass, ApiModule
from .decorators import raw_decorator_name
from .module_merge import generated_stub_header
from .stub_support import StubSupport


def render_stub_signature(signature: CallableSignature) -> str:
    """Render one structured signature for a Python stub."""

    parameters: list[str] = []
    positional_only_end: int | None = None
    keyword_only_marker_added = False
    for parameter in signature.parameters:
        if parameter.kind == ArgumentKind.POSITION_ONLY:
            text = parameter.name
        elif parameter.kind == ArgumentKind.POSITIONAL_OR_KEYWORD:
            text = parameter.name
        elif parameter.kind == ArgumentKind.VAR_POSITIONAL:
            text = f"*{parameter.name}"
            keyword_only_marker_added = True
        elif parameter.kind == ArgumentKind.KEYWORD_ONLY:
            if not keyword_only_marker_added:
                parameters.append("*")
                keyword_only_marker_added = True
            text = parameter.name
        else:
            text = f"**{parameter.name}"

        if parameter.annotation is not None:
            prefix = "*" if parameter.kind == ArgumentKind.VAR_POSITIONAL else ""
            prefix = "**" if parameter.kind == ArgumentKind.VAR_KEYWORD else prefix
            text = f"{prefix}{parameter.name}: {parameter.annotation}"
        if parameter.default is not None:
            text += f" = {parameter.default}"
        parameters.append(text)

        if parameter.kind == ArgumentKind.POSITION_ONLY:
            positional_only_end = len(parameters)

    if positional_only_end is not None:
        parameters.insert(positional_only_end, "/")
    returns = f" -> {signature.return_annotation}" if signature.return_annotation else ""
    prefix = "async " if signature.is_async else ""
    return f"{prefix}def {signature.name}({', '.join(parameters)}){returns}"


def render_docstring_lines(doc: str, indent: str = "") -> list[str]:
    """Render a docstring at the requested indentation level."""

    doc = doc.strip()
    if not doc:
        return []
    if '"""' in doc:
        return [f"{indent}{ast.unparse(ast.Constant(value=doc))}"]
    if "\n" not in doc:
        return [f'{indent}"""{doc}"""']
    lines = [f'{indent}"""']
    lines.extend(f"{indent}{line}" if line else indent for line in doc.splitlines())
    lines.append(f'{indent}"""')
    return lines


def deprecated_decorator_line(message: str) -> str:
    return f"@deprecated({ast.unparse(ast.Constant(value=message))})"


def _raw_decorator_lines(
    decorators: tuple[str, ...],
    *,
    indent: str,
    consumed: set[str] | frozenset[str] = frozenset(),
) -> list[str]:
    lines: list[str] = []
    seen: set[str] = set()
    for decorator in decorators:
        name = raw_decorator_name(decorator)
        text = decorator.removeprefix("@").strip()
        if name in consumed or text in seen:
            continue
        if name.endswith(".setter") and "setter" in consumed:
            continue
        seen.add(text)
        lines.append(f"{indent}@{text}")
    return lines


def _signature_decorators(
    group: ApiCallableGroup,
    signature_index: int,
    *,
    indent: str,
) -> list[str]:
    signature = group.signatures[signature_index]
    flags = signature.flags
    decorators: list[str] = []
    if group.overload:
        decorators.append(f"{indent}@overload")
    if flags.classmethod:
        decorators.append(f"{indent}@classmethod")
    if flags.staticmethod:
        decorators.append(f"{indent}@staticmethod")
    if flags.property_getter:
        decorators.append(f"{indent}@property")
    if flags.property_setter:
        decorators.append(f"{indent}@{group.name}.setter")
    if signature.deprecated_message is not None:
        decorators.append(f"{indent}{deprecated_decorator_line(signature.deprecated_message)}")
    consumed = {
        name
        for name, enabled in (
            ("overload", group.overload),
            ("classmethod", flags.classmethod),
            ("staticmethod", flags.staticmethod),
            ("property", flags.property_getter),
            ("deprecated", signature.deprecated_message is not None),
        )
        if enabled
    }
    if flags.property_setter:
        consumed.add("setter")
    decorators.extend(
        _raw_decorator_lines(
            signature.decorators,
            indent=indent,
            consumed=consumed,
        )
    )
    return decorators


def render_callable_group(
    group: ApiCallableGroup,
    *,
    indent: str = "",
) -> list[str]:
    lines: list[str] = []
    for index, signature in enumerate(group.signatures):
        lines.extend(_signature_decorators(group, index, indent=indent))
        lines.append(f"{indent}{render_stub_signature(signature)}:")
        doc = signature.docstring or group.doc
        if doc:
            lines.extend(render_docstring_lines(doc, f"{indent}    "))
        lines.append(f"{indent}    ...")
    return lines


def _support_lines(source: str, indent: str = "") -> list[str]:
    return [f"{indent}{line}" if line else "" for line in source.splitlines()]


def _is_final_annotation(annotation: str | None) -> bool:
    if not annotation:
        return False
    try:
        node = ast.parse(annotation, mode="eval").body
    except SyntaxError:
        return False
    if isinstance(node, ast.Subscript):
        node = node.value
    return (
        isinstance(node, (ast.Name, ast.Attribute))
        and (node.id if isinstance(node, ast.Name) else node.attr) == "Final"
    )


def _property_annotation(annotation: str) -> str:
    try:
        node = ast.parse(annotation, mode="eval").body
    except SyntaxError:
        return annotation
    if isinstance(node, ast.Subscript):
        value = node.value
        if (
            isinstance(value, (ast.Name, ast.Attribute))
            and (value.id if isinstance(value, ast.Name) else value.attr) == "Final"
        ):
            return ast.unparse(node.slice)
    return annotation


def _deprecated_attribute_methods(attribute: ApiAttribute) -> ApiCallableGroup:
    """Render one deprecated attribute as a property getter and optional setter."""

    assert attribute.deprecated_message is not None
    annotation = attribute.annotation or "Any"
    property_annotation = _property_annotation(annotation)
    getter = CallableSignature(
        name=attribute.name,
        parameters=(
            SignatureParameter(
                name="self",
                annotation=None,
                kind=ArgumentKind.POSITIONAL_OR_KEYWORD,
            ),
        ),
        return_annotation=property_annotation,
        docstring=attribute.doc,
        flags=CallableDecoratorFlags(property_getter=True),
        deprecated_message=attribute.deprecated_message,
    )
    signatures = [getter]
    if not _is_final_annotation(annotation):
        setter = CallableSignature(
            name=attribute.name,
            parameters=(
                SignatureParameter(
                    name="self",
                    annotation=None,
                    kind=ArgumentKind.POSITIONAL_OR_KEYWORD,
                ),
                SignatureParameter(
                    name="value",
                    annotation=property_annotation,
                    kind=ArgumentKind.POSITIONAL_OR_KEYWORD,
                ),
            ),
            return_annotation="None",
            docstring=None,
            flags=CallableDecoratorFlags(property_setter=True),
            deprecated_message=attribute.deprecated_message,
        )
        signatures.append(setter)
    return ApiCallableGroup(name=attribute.name, signatures=tuple(signatures))


def _render_attribute(attribute: ApiAttribute, indent: str = "") -> list[str]:
    """Render an attribute declaration together with its adjacent documentation."""

    annotation = f": {attribute.annotation}" if attribute.annotation else ""
    value = f" = {attribute.value}" if attribute.value is not None else ""
    lines = [f"{indent}{attribute.name}{annotation}{value}"]
    if attribute.doc:
        lines.extend(render_docstring_lines(attribute.doc, indent))
    return lines


def _module_support_lines(source: str, generated_names: set[str]) -> list[str]:
    """Remove support imports already provided by the renderer preamble."""

    lines: list[str] = []
    for node in ast.parse(source).body:
        if isinstance(node, ast.ImportFrom) and node.module in {"typing", "typing_extensions"}:
            node.names = [alias for alias in node.names if alias.name not in generated_names]
            if not node.names:
                continue
        lines.append(ast.unparse(node))
    return lines


def render_class(
    klass: ApiClass,
    *,
    support: StubSupport | None = None,
) -> list[str]:
    bases = f"({', '.join(klass.bases)})" if klass.bases else ""
    lines = _raw_decorator_lines(klass.decorators, indent="")
    lines.append(f"class {klass.name}{bases}:")
    has_body = bool(klass.doc or klass.attributes or klass.methods)
    class_support = support.class_source(klass.module_name, klass.name) if support else ""
    has_body = has_body or bool(class_support.strip())
    if not has_body:
        return [*lines, "    pass"]
    if klass.doc:
        lines.extend(render_docstring_lines(klass.doc, "    "))
    deprecated_attributes = [
        _deprecated_attribute_methods(attribute)
        for attribute in klass.attributes
        if attribute.deprecated_message is not None
    ]
    for attribute in klass.attributes:
        if attribute.deprecated_message is not None:
            continue
        lines.extend(_render_attribute(attribute, "    "))
    if class_support.strip():
        lines.extend(_support_lines(class_support, "    "))
    for method in (*deprecated_attributes, *klass.methods):
        if lines[-1] != "    ":
            lines.append("")
        lines.extend(render_callable_group(method, indent="    "))
    return lines


def _needs_overload(module: ApiModule) -> bool:
    return any(group.overload for group in module.functions) or any(
        group.overload for klass in module.classes for group in klass.methods
    )


def _needs_deprecated(module: ApiModule) -> bool:
    return (
        any(
            raw_decorator_name(decorator) == "deprecated"
            for klass in module.classes
            for decorator in klass.decorators
        )
        or any(
            attribute.deprecated_message is not None
            for klass in module.classes
            for attribute in klass.attributes
        )
        or any(
            signature.deprecated_message is not None
            for group in module.functions
            for signature in group.signatures
        )
        or any(
            signature.deprecated_message is not None
            for klass in module.classes
            for group in klass.methods
            for signature in group.signatures
        )
    )


def _render_alias(module_name: str, public_path: str, target_path: str) -> str:
    name = public_path.rsplit(".", 1)[-1]
    target_module, target = target_path.rsplit(".", 1)
    if target_module == module_name:
        return f"{name} = {target}"
    return f"from {target_module} import {target} as {name}"


def _uses_qualified_freecad_types(
    module: ApiModule,
    module_support: str,
) -> bool:
    texts = [module_support]
    texts.extend(base for klass in module.classes for base in klass.bases)
    texts.extend(attribute.annotation or "" for attribute in module.attributes)
    for function in module.functions:
        texts.extend([signature.return_annotation or "" for signature in function.signatures])
        texts.extend(
            parameter.annotation or ""
            for signature in function.signatures
            for parameter in signature.parameters
        )
    for klass in module.classes:
        for attribute in klass.attributes:
            texts.append(attribute.annotation or "")
        for method in klass.methods:
            for signature in method.signatures:
                texts.append(signature.return_annotation or "")
                texts.extend(parameter.annotation or "" for parameter in signature.parameters)
    return any("FreeCAD." in text for text in texts)


def render_module(
    module: ApiModule,
    *,
    support: StubSupport | None = None,
) -> str:
    needs_overload = _needs_overload(module)
    needs_deprecated = _needs_deprecated(module)
    lines = [
        *generated_stub_header().splitlines(),
        "",
        "from __future__ import annotations",
        "from typing import Any",
    ]
    if needs_overload:
        lines[-1] += ", overload"
    if needs_deprecated:
        lines.append("from typing_extensions import deprecated")
    lines.append("")

    module_support = support.module_source(module.name) if support else ""
    if module.name != "FreeCAD" and _uses_qualified_freecad_types(module, module_support):
        lines.append("import FreeCAD")
        lines.append("")
    if module_support.strip():
        generated_names = {"Any"}
        if needs_overload:
            generated_names.add("overload")
        if needs_deprecated:
            generated_names.add("deprecated")
        lines.extend(_module_support_lines(module_support, generated_names))
        lines.append("")
    for attribute in module.attributes:
        lines.extend(_render_attribute(attribute))
    if module.attributes:
        lines.append("")
    for function in module.functions:
        lines.extend(render_callable_group(function))
        lines.append("")
    for klass in module.classes:
        lines.extend(render_class(klass, support=support))
        lines.append("")
    for alias in module.aliases:
        lines.append(_render_alias(module.name, alias.public_path, alias.target_path))
    return "\n".join(lines).rstrip() + "\n"


def write_stub_file(
    path: Path,
    *,
    module: ApiModule,
    support: StubSupport | None = None,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(render_module(module, support=support), encoding="utf-8")
