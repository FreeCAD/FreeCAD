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

from .model import BindingMethod, StubSignatureGroup, StubSignatureOverrides
from .python_api.model import ApiCallableGroup, ApiModule
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


def deprecated_decorator_line(message: str) -> str:
    return f"@deprecated({ast.unparse(ast.Constant(value=message))})"


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
            if known_signature.deprecated_message is not None:
                rendered.append(deprecated_decorator_line(known_signature.deprecated_message))
            if known_signature.doc:
                rendered.append(f"def {method.python_name}{signature_text} -> {returns}:")
                rendered.extend(render_docstring_lines(known_signature.doc))
                rendered.append("    ...")
            else:
                rendered.append(f"def {method.python_name}{signature_text} -> {returns}: ...")
        return tuple(rendered)

    return (f"def {method.python_name}{signature(method, class_method)} -> Any: ...",)


def api_groups_need_overload_import(api_module: ApiModule | None) -> bool:
    return bool(api_module and any(group.overload for group in api_module.functions))


def api_groups_need_deprecated_import(
    api_module: ApiModule | None,
    module_name: str | None,
    stub_signature_overrides: StubSignatureOverrides | None,
) -> bool:
    if api_module is None or module_name is None:
        return False
    overrides = stub_signature_overrides or {}
    return any(
        message is not None
        for group in api_module.functions
        for message in api_group_deprecated_messages(group, module_name, overrides)
    )


def typing_import_lines(
    api_module: ApiModule | None,
    module_name: str,
    stub_signature_overrides: StubSignatureOverrides,
) -> list[str]:
    lines: list[str] = []
    if api_groups_need_overload_import(api_module):
        lines.append("from typing import Any, overload")
    else:
        lines.append("from typing import Any")
    if api_groups_need_deprecated_import(api_module, module_name, stub_signature_overrides):
        lines.append("from typing_extensions import deprecated")
    return lines


def api_group_stub_signatures(
    group: ApiCallableGroup,
    module_name: str,
    stub_signature_overrides: StubSignatureOverrides,
) -> StubSignatureGroup | None:
    if group.location is None:
        return None
    overrides = stub_signature_overrides.get(
        (group.location.path, module_name, group.name),
    )
    if overrides is None:
        candidates = [
            candidate
            for (
                _source,
                context_name,
                function_name,
            ), candidate in stub_signature_overrides.items()
            if context_name == module_name and function_name == group.name
        ]
        if candidates and all(candidate == candidates[0] for candidate in candidates[1:]):
            overrides = candidates[0]
    if overrides is not None and len(overrides) == len(group.signatures):
        return overrides
    return None


def api_group_deprecated_messages(
    group: ApiCallableGroup,
    module_name: str,
    stub_signature_overrides: StubSignatureOverrides,
) -> tuple[str | None, ...]:
    overrides = api_group_stub_signatures(group, module_name, stub_signature_overrides)
    if overrides is None:
        return (None,) * len(group.signatures)
    return tuple(signature.deprecated_message for signature in overrides)


def rendered_api_callable_group(
    group: ApiCallableGroup,
    module_name: str,
    stub_signature_overrides: StubSignatureOverrides,
    *,
    indent: str = "",
) -> list[str]:
    """Render a module-level callable from the canonical public API model."""

    rendered: list[str] = []
    source_signatures = api_group_stub_signatures(
        group,
        module_name,
        stub_signature_overrides,
    )
    use_overload = group.overload if source_signatures is None else len(source_signatures) > 1
    for index, signature_data in enumerate(group.signatures):
        if use_overload:
            rendered.append(f"{indent}@overload")
        source_signature = source_signatures[index] if source_signatures is not None else None
        deprecated_message = (
            source_signature.deprecated_message if source_signature is not None else None
        )
        if deprecated_message is not None:
            rendered.append(f"{indent}{deprecated_decorator_line(deprecated_message)}")
        if source_signature is not None:
            display_signature = (
                f"{group.name}({source_signature.parameters}) -> {source_signature.returns}"
            )
            doc = source_signature.doc
        else:
            display_signature = signature_data.display_signature
            doc = signature_data.docstring or group.doc
        rendered.append(f"{indent}def {display_signature}:")
        if doc:
            rendered.extend(f"{indent}{line}" for line in render_docstring_lines(doc))
            rendered.append(f"{indent}    ...")
        else:
            rendered[-1] += " ..."
    return rendered


def write_stub_file(
    path: Path,
    *,
    stub_signature_overrides: StubSignatureOverrides,
    api_module: ApiModule | None = None,
    module_name: str,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    logical_module_name = module_name
    lines = [
        "from __future__ import annotations",
        *typing_import_lines(
            api_module,
            logical_module_name,
            stub_signature_overrides,
        ),
        "",
    ]

    for group in api_module.functions if api_module else ():
        lines.extend(
            rendered_api_callable_group(
                group,
                logical_module_name,
                stub_signature_overrides,
            )
        )

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
