# pyright: strict

"""Markdown emission for the neutral public API model.

This renderer turns the structured ``ApiModel`` into package-shaped MDX pages
that match Starlight's content-collection layout. The output is still static
content, not a site scaffold, so the generator stays focused on durable API
page structure and leaves Astro configuration to the docs site.
"""

from __future__ import annotations

import os
from pathlib import Path
import shutil

from .api_model import (
    ApiAttribute,
    ApiCallableGroup,
    ApiClass,
    ApiModel,
    ApiModule,
    ApiSourceLocation,
)

PYTHON_API_ROOT = "python-api"
TYPE_GROUP_DIR = "types"
TOP_LEVEL_MODULE_ORDER = {
    "FreeCAD": 1,
    "FreeCADGui": 2,
    "Part": 3,
    "QtUnitGui": 4,
}
MODULE_ORDER = {
    "FreeCAD.Console": 1,
    "FreeCAD.Qt": 2,
    "FreeCAD.Units": 3,
    "FreeCADGui.Selection": 1,
}


def content_root_dir(out_dir: Path) -> Path:
    return out_dir / PYTHON_API_ROOT


def module_slug_parts(module_name: str) -> tuple[str, ...]:
    return tuple(part.lower() for part in module_name.split("."))


def module_slug(module_name: str) -> str:
    return "/".join((PYTHON_API_ROOT, *module_slug_parts(module_name)))


def class_slug(klass: ApiClass) -> str:
    return "/".join((module_slug(klass.module_name), TYPE_GROUP_DIR, klass.name))


def module_doc_dir(out_dir: Path, module_name: str) -> Path:
    return content_root_dir(out_dir).joinpath(*module_slug_parts(module_name))


def module_doc_path(out_dir: Path, module_name: str) -> Path:
    return module_doc_dir(out_dir, module_name) / "index.mdx"


def class_doc_path(out_dir: Path, module_name: str, class_name: str) -> Path:
    return module_doc_dir(out_dir, module_name) / TYPE_GROUP_DIR / f"{class_name}.mdx"


def module_title(module_name: str) -> str:
    return module_name.rsplit(".", 1)[-1]


def class_title(klass: ApiClass) -> str:
    return klass.name


def page_link(slug: str) -> str:
    return f"/{slug}/"


def fenced_python(lines: list[str]) -> str:
    body = "\n".join(lines)
    return f"```python\n{body}\n```"


def summary_text(text: str | None) -> str | None:
    if not text:
        return None
    first_paragraph = text.strip().split("\n\n", 1)[0]
    return " ".join(first_paragraph.split())


def source_text(location: ApiSourceLocation) -> str:
    if location.line is None:
        return location.path
    return f"{location.path}:{location.line}"


def source_link(location: ApiSourceLocation, source_base_url: str | None) -> str:
    if source_base_url is None:
        return f"`{source_text(location)}`"
    path = location.path
    if location.line is not None:
        return f"[`{source_text(location)}`]({source_base_url.rstrip('/')}/{path}#L{location.line})"
    return f"[`{path}`]({source_base_url.rstrip('/')}/{path})"


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


def render_aliases(aliases: tuple[str, ...]) -> list[str]:
    return [f"- `{alias}`" for alias in aliases]


def render_page_metadata(
    lines: list[str],
    *,
    import_line: str,
    location: ApiSourceLocation | None,
    source_base_url: str | None,
) -> None:
    lines.append(f"- **Import:** `{import_line}`")
    if location is not None:
        lines.append(f"- **Source:** {source_link(location, source_base_url)}")
    lines.append("")


def render_callable_group(
    group: ApiCallableGroup,
    *,
    source_base_url: str | None,
) -> list[str]:
    lines = [f"#### `{group.name}`", ""]
    if group.doc:
        lines.append(group.doc)
        lines.append("")
    if group.overload:
        for signature in group.signatures:
            lines.append(fenced_python([f"def {signature.display_signature}: ..."]))
            lines.append("")
    else:
        lines.append(fenced_python([f"def {group.signatures[0].display_signature}: ..."]))
        lines.append("")
    if group.location is not None:
        lines.append(f"- **Source:** {source_link(group.location, source_base_url)}")
        lines.append("")
    return lines


def render_class_summary(klass: ApiClass) -> str:
    link = page_link(class_slug(klass))
    summary = f"- [`{klass.name}`]({link})"
    doc = summary_text(klass.doc)
    if doc:
        summary += f": {doc}"
    return summary


def render_module_summary(module: ApiModule) -> str:
    link = page_link(module_slug(module.name))
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


def frontmatter(
    title: str,
    description: str | None = None,
    *,
    slug: str | None = None,
    sidebar_label: str | None = None,
    sidebar_order: int | None = None,
    sidebar_hidden: bool = False,
) -> str:
    lines = ["---", f"title: {title}"]
    if description:
        escaped = summary_text(description).replace('"', '\\"')
        lines.append(f'description: "{escaped}"')
    if slug is not None:
        lines.append(f"slug: {slug}")
    if sidebar_label is not None or sidebar_order is not None or sidebar_hidden:
        lines.append("sidebar:")
        if sidebar_label is not None:
            lines.append(f"  label: {sidebar_label}")
        if sidebar_order is not None:
            lines.append(f"  order: {sidebar_order}")
        if sidebar_hidden:
            lines.append("  hidden: true")
    lines.extend(["---", ""])
    return "\n".join(lines)


def render_module_page(
    out_dir: Path,
    module: ApiModule,
    modules: tuple[ApiModule, ...],
    *,
    source_base_url: str | None,
) -> str:
    lines = [
        frontmatter(
            module_title(module.name),
            module.doc,
            slug=module_slug(module.name),
            sidebar_label=module_title(module.name),
            sidebar_order=MODULE_ORDER.get(module.name, TOP_LEVEL_MODULE_ORDER.get(module.name)),
        )
    ]
    lines.append(f"# `{module.name}`")
    lines.append("")
    if module.doc:
        lines.append(module.doc)
        lines.append("")

    render_page_metadata(
        lines,
        import_line=f"import {module.name}",
        location=module.location,
        source_base_url=source_base_url,
    )

    if module.aliases:
        lines.append("## Aliases")
        lines.append("")
        lines.extend(
            f"- `{alias.public_path}` -> `{alias.target_path}`" for alias in module.aliases
        )
        lines.append("")

    submodules = child_modules(module.name, modules)
    if submodules:
        lines.append("## Submodules")
        lines.append("")
        lines.extend(render_module_summary(child) for child in submodules)
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
            lines.extend(render_callable_group(group, source_base_url=source_base_url))

    if module.classes:
        lines.append("## Classes")
        lines.append("")
        lines.extend(render_class_summary(klass) for klass in module.classes)
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def render_class_page(
    out_dir: Path,
    klass: ApiClass,
    *,
    source_base_url: str | None,
) -> str:
    lines = [
        frontmatter(
            class_title(klass),
            klass.doc,
            slug=class_slug(klass),
            sidebar_label=klass.name,
        )
    ]
    lines.append(f"# `{klass.module_name}.{klass.name}`")
    lines.append("")
    if klass.doc:
        lines.append(klass.doc)
        lines.append("")

    render_page_metadata(
        lines,
        import_line=f"from {klass.module_name} import {klass.name}",
        location=klass.location,
        source_base_url=source_base_url,
    )

    if klass.aliases:
        lines.append("## Aliases")
        lines.append("")
        lines.extend(render_aliases(klass.aliases))
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
            lines.extend(render_callable_group(group, source_base_url=source_base_url))

    return "\n".join(lines).rstrip() + "\n"


def render_root_index(out_dir: Path, model: ApiModel) -> str:
    lines = [
        frontmatter(
            "Python API",
            "Generated documentation for the curated FreeCAD Python API stubs.",
            slug=PYTHON_API_ROOT,
            sidebar_label="Python API",
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
    for module in model.modules:
        if "." in module.name:
            continue
        lines.append(render_module_summary(module))
    lines.append("")
    return "\n".join(lines)


def write_api_markdown_docs(
    out_dir: Path,
    model: ApiModel,
    *,
    source_base_url: str | None = None,
) -> int:
    """Write one Starlight-ready MDX page tree for the API model."""

    out_dir.mkdir(parents=True, exist_ok=True)
    content_root = content_root_dir(out_dir)
    shutil.rmtree(content_root, ignore_errors=True)
    content_root.mkdir(parents=True, exist_ok=True)
    (content_root / "index.mdx").write_text(render_root_index(out_dir, model), encoding="utf-8")

    page_count = 1
    for module in model.modules:
        module_page = module_doc_path(out_dir, module.name)
        module_page.parent.mkdir(parents=True, exist_ok=True)
        module_page.write_text(
            render_module_page(
                out_dir,
                module,
                model.modules,
                source_base_url=source_base_url,
            ),
            encoding="utf-8",
        )
        page_count += 1
        for klass in module.classes:
            class_page = class_doc_path(out_dir, module.name, klass.name)
            class_page.parent.mkdir(parents=True, exist_ok=True)
            class_page.write_text(
                render_class_page(
                    out_dir,
                    klass,
                    source_base_url=source_base_url,
                ),
                encoding="utf-8",
            )
            page_count += 1

    return page_count
