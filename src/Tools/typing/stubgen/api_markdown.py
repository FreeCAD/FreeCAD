# pyright: strict

"""Markdown emission for the neutral public API model.

This renderer turns the structured ``ApiModel`` into package-shaped Markdown
pages that can later be consumed by an Astro Starlight site. The output is
static documentation content, not a site scaffold.
"""

from __future__ import annotations

import os
from pathlib import Path
import shutil

from .api_model import ApiAttribute, ApiCallableGroup, ApiClass, ApiModel, ApiModule


def module_doc_dir(out_dir: Path, module_name: str) -> Path:
    return out_dir.joinpath(*module_name.split("."))


def module_doc_path(out_dir: Path, module_name: str) -> Path:
    return module_doc_dir(out_dir, module_name) / "index.md"


def class_doc_path(out_dir: Path, module_name: str, class_name: str) -> Path:
    return module_doc_dir(out_dir, module_name) / f"{class_name}.md"


def relative_link(from_path: Path, to_path: Path) -> str:
    return Path(os.path.relpath(to_path, from_path.parent)).as_posix()


def fenced_python(lines: list[str]) -> str:
    body = "\n".join(lines)
    return f"```python\n{body}\n```"


def summary_text(text: str | None) -> str | None:
    if not text:
        return None
    first_paragraph = text.strip().split("\n\n", 1)[0]
    return " ".join(first_paragraph.split())


def render_attribute(attribute: ApiAttribute) -> str:
    parts = [attribute.name]
    if attribute.annotation:
        parts.append(f": {attribute.annotation}")
    if attribute.value:
        parts.append(f" = {attribute.value}")
    line = f"- `{''.join(parts)}`"
    if attribute.doc:
        line += f": {attribute.doc}"
    return line


def render_callable_group(group: ApiCallableGroup) -> list[str]:
    lines = [f"### `{group.name}`", ""]
    signature_lines = [
        (
            f"def {signature.display_signature}"
            if not signature.display_signature.startswith(group.name)
            else f"def {signature.display_signature}"
        )
        for signature in group.signatures
    ]
    lines.append(fenced_python(signature_lines))
    lines.append("")
    if group.doc:
        lines.append(group.doc)
        lines.append("")
    return lines


def render_class_summary(class_page: Path, module_page: Path, klass: ApiClass) -> str:
    link = relative_link(module_page, class_page)
    summary = f"- [`{klass.name}`]({link})"
    doc = summary_text(klass.doc)
    if doc:
        summary += f": {doc}"
    return summary


def render_module_summary(child_page: Path, module_page: Path, module: ApiModule) -> str:
    link = relative_link(module_page, child_page)
    summary = f"- [`{module.name}`]({link})"
    doc = summary_text(module.doc)
    if doc:
        summary += f": {doc}"
    return summary


def child_modules(module_name: str, modules: tuple[ApiModule, ...]) -> tuple[ApiModule, ...]:
    prefix = f"{module_name}."
    direct: list[ApiModule] = []
    for module in modules:
        if not module.name.startswith(prefix):
            continue
        remainder = module.name[len(prefix) :]
        if "." in remainder:
            continue
        direct.append(module)
    return tuple(direct)


def frontmatter(title: str, description: str | None = None) -> str:
    lines = ["---", f"title: {title}"]
    if description:
        escaped = summary_text(description).replace('"', '\\"')
        lines.append(f'description: "{escaped}"')
    lines.extend(["---", ""])
    return "\n".join(lines)


def render_module_page(out_dir: Path, module: ApiModule, modules: tuple[ApiModule, ...]) -> str:
    lines = [frontmatter(module.name, module.doc)]
    lines.append(f"# `{module.name}`")
    lines.append("")
    if module.doc:
        lines.append(module.doc)
        lines.append("")

    lines.append("## Import")
    lines.append("")
    lines.append(fenced_python([f"import {module.name}"]))
    lines.append("")

    submodules = child_modules(module.name, modules)
    if submodules:
        lines.append("## Submodules")
        lines.append("")
        module_page = module_doc_path(out_dir, module.name)
        lines.extend(
            render_module_summary(module_doc_path(out_dir, child.name), module_page, child)
            for child in submodules
        )
        lines.append("")

    public_attributes = tuple(
        attribute for attribute in module.attributes if not attribute.name.startswith("_")
    )
    if public_attributes:
        lines.append("## Attributes")
        lines.append("")
        lines.extend(render_attribute(attribute) for attribute in public_attributes)
        lines.append("")

    if module.functions:
        lines.append("## Functions")
        lines.append("")
        for group in module.functions:
            lines.extend(render_callable_group(group))

    if module.classes:
        lines.append("## Classes")
        lines.append("")
        module_page = module_doc_path(out_dir, module.name)
        lines.extend(
            render_class_summary(
                class_doc_path(out_dir, module.name, klass.name), module_page, klass
            )
            for klass in module.classes
        )
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def render_class_page(out_dir: Path, klass: ApiClass) -> str:
    lines = [frontmatter(f"{klass.module_name}.{klass.name}", klass.doc)]
    lines.append(f"# `{klass.module_name}.{klass.name}`")
    lines.append("")
    if klass.doc:
        lines.append(klass.doc)
        lines.append("")

    lines.append("## Import")
    lines.append("")
    lines.append(fenced_python([f"from {klass.module_name} import {klass.name}"]))
    lines.append("")

    if klass.bases:
        lines.append("## Bases")
        lines.append("")
        for base in klass.bases:
            lines.append(f"- `{base}`")
        lines.append("")

    if klass.attributes:
        lines.append("## Attributes")
        lines.append("")
        lines.extend(render_attribute(attribute) for attribute in klass.attributes)
        lines.append("")

    if klass.methods:
        lines.append("## Methods")
        lines.append("")
        for group in klass.methods:
            lines.extend(render_callable_group(group))

    return "\n".join(lines).rstrip() + "\n"


def render_root_index(out_dir: Path, model: ApiModel) -> str:
    lines = [
        frontmatter(
            "Python API", "Generated documentation for the curated FreeCAD Python API stubs."
        )
    ]
    lines.append("# Python API")
    lines.append("")
    lines.append(
        "This reference is generated from the curated source-adjacent `.pyi` files used by the FreeCAD typing pipeline."
    )
    lines.append("")
    lines.append("## Modules")
    lines.append("")
    root_page = out_dir / "index.md"
    for module in model.modules:
        if "." in module.name:
            continue
        lines.append(
            render_module_summary(module_doc_path(out_dir, module.name), root_page, module)
        )
    lines.append("")
    return "\n".join(lines)


def write_api_markdown_docs(out_dir: Path, model: ApiModel) -> int:
    """Write one package-shaped Markdown page tree for the API model."""

    shutil.rmtree(out_dir, ignore_errors=True)
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "index.md").write_text(render_root_index(out_dir, model), encoding="utf-8")

    page_count = 1
    for module in model.modules:
        module_page = module_doc_path(out_dir, module.name)
        module_page.parent.mkdir(parents=True, exist_ok=True)
        module_page.write_text(render_module_page(out_dir, module, model.modules), encoding="utf-8")
        page_count += 1
        for klass in module.classes:
            class_page = class_doc_path(out_dir, module.name, klass.name)
            class_page.parent.mkdir(parents=True, exist_ok=True)
            class_page.write_text(render_class_page(out_dir, klass), encoding="utf-8")
            page_count += 1

    return page_count
