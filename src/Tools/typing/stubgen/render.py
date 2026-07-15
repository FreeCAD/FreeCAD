# pyright: strict

"""Stub rendering helpers for the public-output pipeline.

This module owns the text-level rendering of discovered bindings and curated
signatures. It turns normalized binding records into stub lines, inventory
files, and small generated fragments that later stages merge into the public
package tree.

In the overall pipeline this sits after discovery and source-input parsing:
- ``discovery`` and ``source_inputs`` decide what should be rendered
- ``render`` decides how an individual stub fragment is written
- ``generator`` and merge helpers decide where the rendered output lands
"""

from __future__ import annotations

import ast
from pathlib import Path
import re

from .model import BindingMethod, PublicTypeGroup, StubSignatureOverrides
from .naming import valid_identifier


def signature(method: BindingMethod, class_method: bool = False) -> str:
    self_arg = "self, " if class_method else ""
    if method.method_kind == "noargs":
        return "(self)" if class_method else "()"
    if method.method_kind == "keyword":
        return f"({self_arg}*args: Any, **kwargs: Any)"
    return f"({self_arg}*args: Any)"


def known_stub_signatures(
    method: BindingMethod,
    stub_signature_overrides: StubSignatureOverrides,
):
    return stub_signature_overrides.get((method.source, method.context_name, method.python_name))


def resolve_signature_placeholders(
    text: str,
    class_symbol: str | None,
    source_class_symbol: str | None = None,
) -> str:
    if not class_symbol:
        return text
    text = text.replace("{class}", class_symbol)
    if source_class_symbol and source_class_symbol != class_symbol:
        return re.sub(rf"\b{re.escape(source_class_symbol)}\b", class_symbol, text)
    return text


def format_signature(parameters: str, class_method: bool) -> str:
    if class_method:
        if parameters:
            return f"(self, {parameters})"
        return "(self)"
    return f"({parameters})" if parameters else "()"


def render_docstring_lines(doc: str) -> tuple[str, ...]:
    doc = doc.strip()
    if not doc:
        return ()
    if '"""' in doc:
        return (f"    {ast.unparse(ast.Constant(value=doc))}",)
    if "\n" not in doc:
        return (f'    """{doc}"""',)

    lines = ['    """']
    lines.extend(f"    {line}" if line else "    " for line in doc.splitlines())
    lines.append('    """')
    return tuple(lines)


def render_stub_lines(
    method: BindingMethod,
    class_method: bool = False,
    class_symbol: str | None = None,
    stub_signature_overrides: StubSignatureOverrides | None = None,
) -> tuple[str, ...]:
    if not valid_identifier(method.python_name):
        return (f"# TODO: invalid Python identifier from binding table: {method.python_name!r}",)

    known_signatures = known_stub_signatures(method, stub_signature_overrides or {})
    if known_signatures:
        rendered: list[str] = []
        use_overload = len(known_signatures) > 1
        for known_signature in known_signatures:
            parameters = resolve_signature_placeholders(
                known_signature.parameters,
                class_symbol,
                known_signature.class_symbol,
            )
            returns = resolve_signature_placeholders(
                known_signature.returns,
                class_symbol,
                known_signature.class_symbol,
            )
            signature_text = format_signature(parameters, class_method)
            if use_overload:
                rendered.append("@overload")
            if known_signature.doc:
                rendered.append(f"def {method.python_name}{signature_text} -> {returns}:")
                rendered.extend(render_docstring_lines(known_signature.doc))
                rendered.append("    ...")
            else:
                rendered.append(f"def {method.python_name}{signature_text} -> {returns}: ...")
        return tuple(rendered)

    return (f"def {method.python_name}{signature(method, class_method)} -> Any: ...",)


def methods_need_overload_import(
    methods: list[BindingMethod],
    stub_signature_overrides: StubSignatureOverrides | None,
) -> bool:
    overrides = stub_signature_overrides or {}
    return any(
        signatures is not None and len(signatures) > 1
        for method in methods
        for signatures in (known_stub_signatures(method, overrides),)
    )


def typing_import_line(
    methods: list[BindingMethod],
    stub_signature_overrides: StubSignatureOverrides | None,
) -> str:
    if methods_need_overload_import(methods, stub_signature_overrides):
        return "from typing import Any, overload"
    return "from typing import Any"


def rendered_method_blocks(
    methods: list[BindingMethod],
    *,
    class_method: bool = False,
    class_symbol: str | None = None,
    stub_signature_overrides: StubSignatureOverrides | None = None,
    indent: str = "",
) -> list[str]:
    rendered_lines: list[str] = []
    seen: set[tuple[str, ...]] = set()
    for method in methods:
        rendered = tuple(
            indent + line
            for line in render_stub_lines(
                method,
                class_method=class_method,
                class_symbol=class_symbol,
                stub_signature_overrides=stub_signature_overrides,
            )
        )
        if rendered in seen:
            continue
        rendered_lines.extend(rendered)
        seen.add(rendered)
    return rendered_lines


def write_stub_file(
    path: Path,
    methods: list[BindingMethod],
    class_name: str | None = None,
    stub_signature_overrides: StubSignatureOverrides | None = None,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "from __future__ import annotations",
        typing_import_line(methods, stub_signature_overrides),
        "",
    ]

    if class_name:
        safe_class_name = class_name.rsplit(".", 1)[-1]
        if not valid_identifier(safe_class_name):
            safe_class_name = "BindingType"
        lines.append(f"class {safe_class_name}:")
        class_lines = rendered_method_blocks(
            methods,
            class_method=True,
            class_symbol=safe_class_name,
            stub_signature_overrides=stub_signature_overrides,
            indent="    ",
        )
        if class_lines:
            lines.extend(class_lines)
        else:
            lines.append("    pass")
    else:
        lines.extend(
            rendered_method_blocks(
                methods,
                stub_signature_overrides=stub_signature_overrides,
            )
        )

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def type_stub_lines(
    type_groups: list[PublicTypeGroup],
    stub_signature_overrides: StubSignatureOverrides,
    include_future_import: bool = True,
) -> list[str]:
    methods = [method for type_group in type_groups for method in type_group.methods]
    lines = [
        "# Generated public type stubs from PyCXX binding method tables.",
    ]
    if include_future_import:
        lines.append("from __future__ import annotations")
    lines.extend([typing_import_line(methods, stub_signature_overrides), ""])

    for type_group in type_groups:
        base_clause = f"({', '.join(type_group.base_symbols)})" if type_group.base_symbols else ""
        lines.append(f"class {type_group.class_symbol}{base_clause}:")
        class_lines = rendered_method_blocks(
            type_group.methods,
            class_method=True,
            class_symbol=type_group.class_symbol,
            stub_signature_overrides=stub_signature_overrides,
            indent="    ",
        )
        if class_lines:
            lines.extend(class_lines)
        else:
            lines.append("    pass")
        if type_group.variable_symbol:
            lines.extend(["", f"{type_group.variable_symbol}: {type_group.class_symbol}"])
        lines.append("")

    return lines
