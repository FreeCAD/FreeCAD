# pyright: strict

"""Render Starlight-shaped MDX pages for the extracted C++ API model."""

from __future__ import annotations

import hashlib
from pathlib import Path
import re
import shutil

from .cpp_api_model import (
    CppApiClass,
    CppApiEnum,
    CppApiFunction,
    CppApiModel,
    CppApiNamespace,
    CppSourceLocation,
)

CPP_API_ROOT = "cpp-api"
TYPE_GROUP_DIR = "types"
TOP_LEVEL_NAMESPACE_ORDER = {
    "App": 1,
    "Base": 2,
    "Gui": 3,
    "Part": 4,
    "Data": 5,
    "Attacher": 6,
}


def content_root_dir(out_dir: Path) -> Path:
    return out_dir / CPP_API_ROOT


def summary_text(text: str | None) -> str | None:
    if not text:
        return None
    first_paragraph = text.strip().split("\n\n", 1)[0]
    return " ".join(first_paragraph.split())


def mdx_text(text: str) -> str:
    return (
        text.replace("\\", "\\\\")
        .replace("{", "\\{")
        .replace("}", "\\}")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
    )


def namespace_slug_parts(qualified_name: str) -> tuple[str, ...]:
    return tuple(part.lower() for part in qualified_name.split("::"))


def namespace_slug(qualified_name: str) -> str:
    return "/".join((CPP_API_ROOT, *namespace_slug_parts(qualified_name)))


def class_slug_token(klass: CppApiClass) -> str:
    raw_token = klass.display_name.replace("::", "-")
    token = raw_token
    token = re.sub(r"[^0-9A-Za-z_-]+", "-", token)
    token = re.sub(r"-{2,}", "-", token)
    token = token.strip("-")
    if token != klass.display_name:
        suffix = hashlib.sha256(klass.qualified_name.encode("utf-8")).hexdigest()[:8]
        token = f"{token or klass.name}-{suffix}"
    return token


def class_slug(klass: CppApiClass) -> str:
    return "/".join((namespace_slug(klass.namespace_name), TYPE_GROUP_DIR, class_slug_token(klass)))


def namespace_doc_dir(out_dir: Path, qualified_name: str) -> Path:
    return content_root_dir(out_dir).joinpath(*namespace_slug_parts(qualified_name))


def namespace_doc_path(out_dir: Path, qualified_name: str) -> Path:
    return namespace_doc_dir(out_dir, qualified_name) / "index.mdx"


def class_doc_filename(klass: CppApiClass) -> str:
    token = class_slug_token(klass)
    if token.startswith("_"):
        return f"cls-{token}.mdx"
    return f"{token}.mdx"


def class_doc_path(out_dir: Path, klass: CppApiClass) -> Path:
    return (
        namespace_doc_dir(out_dir, klass.namespace_name)
        / TYPE_GROUP_DIR
        / class_doc_filename(klass)
    )


def page_link(slug: str) -> str:
    return f"/{slug}/"


def source_text(location: CppSourceLocation) -> str:
    if location.line is None:
        return location.path
    return f"{location.path}:{location.line}"


def source_link(location: CppSourceLocation, source_base_url: str | None) -> str:
    if source_base_url is None:
        return f"`{source_text(location)}`"
    path = location.path
    if location.line is not None:
        return f"[`{source_text(location)}`]({source_base_url.rstrip('/')}/{path}#L{location.line})"
    return f"[`{path}`]({source_base_url.rstrip('/')}/{path})"


def frontmatter(
    title: str,
    description: str | None,
    *,
    slug: str,
    sidebar_label: str | None = None,
    sidebar_order: int | None = None,
) -> str:
    lines = ["---", f"title: {title!r}"]
    summary = summary_text(description)
    if summary:
        lines.append(f"description: {summary!r}")
    lines.append(f"slug: {slug!r}")
    if sidebar_label or sidebar_order is not None:
        lines.append("sidebar:")
        if sidebar_label:
            lines.append(f"  label: {sidebar_label!r}")
        if sidebar_order is not None:
            lines.append(f"  order: {sidebar_order}")
    lines.extend(["---", ""])
    return "\n".join(lines)


def render_page_metadata(
    lines: list[str],
    *,
    qualified_name: str,
    kind: str,
    location: CppSourceLocation | None,
    source_base_url: str | None,
) -> None:
    lines.append(f"- **Qualified name:** `{qualified_name}`")
    lines.append(f"- **Kind:** `{kind}`")
    if location is not None:
        lines.append(f"- **Source:** {source_link(location, source_base_url)}")
    lines.append("")


def fenced_cpp(lines: list[str]) -> str:
    body = "\n".join(lines)
    return f"```cpp\n{body}\n```"


def render_function(function: CppApiFunction, *, source_base_url: str | None) -> list[str]:
    lines = [f"### `{function.name}`", "", fenced_cpp([function.declaration])]
    if function.doc:
        lines.extend(["", mdx_text(function.doc)])
    if function.location is not None:
        lines.extend(["", f"_Source:_ {source_link(function.location, source_base_url)}"])
    lines.extend(["", ""])
    return lines


def render_enum(enum: CppApiEnum, *, source_base_url: str | None) -> list[str]:
    lines = [f"### `{enum.name}`", "", fenced_cpp([enum.declaration])]
    if enum.doc:
        lines.extend(["", mdx_text(enum.doc)])
    if enum.values:
        lines.extend(["", "Values", ""])
        for value in enum.values:
            initializer = f" = {value.initializer}" if value.initializer else ""
            line = f"- `{value.name}{initializer}`"
            if value.doc:
                line += f": {mdx_text(value.doc)}"
            lines.append(line)
    if enum.location is not None:
        lines.extend(["", f"_Source:_ {source_link(enum.location, source_base_url)}"])
    lines.extend(["", ""])
    return lines


def child_namespaces(
    namespace: CppApiNamespace,
    namespaces: tuple[CppApiNamespace, ...],
) -> tuple[CppApiNamespace, ...]:
    return tuple(child for child in namespaces if child.parent_name == namespace.qualified_name)


def namespace_classes(
    namespace: CppApiNamespace,
    classes: tuple[CppApiClass, ...],
) -> tuple[CppApiClass, ...]:
    return tuple(klass for klass in classes if klass.namespace_name == namespace.qualified_name)


def validate_class_paths(out_dir: Path, classes: tuple[CppApiClass, ...]) -> None:
    paths: dict[Path, str] = {}
    for klass in classes:
        path = class_doc_path(out_dir, klass)
        previous = paths.get(path)
        if previous is not None:
            raise ValueError(
                f"C++ API class pages collide at {path}: {previous} and {klass.qualified_name}"
            )
        paths[path] = klass.qualified_name


def render_namespace_summary(namespace: CppApiNamespace) -> str:
    summary = summary_text(namespace.doc)
    line = f"- [`{namespace.name}`]({page_link(namespace_slug(namespace.qualified_name))})"
    if summary:
        line += f": {mdx_text(summary)}"
    return line


def render_class_summary(klass: CppApiClass) -> str:
    summary = summary_text(klass.doc)
    line = f"- [`{klass.display_name}`]({page_link(class_slug(klass))})"
    if summary:
        line += f": {mdx_text(summary)}"
    return line


def render_namespace_page(
    namespace: CppApiNamespace,
    model: CppApiModel,
    *,
    source_base_url: str | None,
) -> str:
    lines = [
        frontmatter(
            namespace.name,
            namespace.doc,
            slug=namespace_slug(namespace.qualified_name),
            sidebar_label=namespace.name,
            sidebar_order=TOP_LEVEL_NAMESPACE_ORDER.get(namespace.qualified_name),
        )
    ]
    lines.extend([f"# {namespace.qualified_name}", ""])
    if namespace.doc:
        lines.extend([mdx_text(namespace.doc), ""])
    render_page_metadata(
        lines,
        qualified_name=namespace.qualified_name,
        kind="namespace",
        location=namespace.location,
        source_base_url=source_base_url,
    )

    children = child_namespaces(namespace, model.namespaces)
    if children:
        lines.extend(["## Namespaces", ""])
        lines.extend(render_namespace_summary(child) for child in children)
        lines.append("")

    classes = namespace_classes(namespace, model.classes)
    if classes:
        lines.extend(["## Types", ""])
        lines.extend(render_class_summary(klass) for klass in classes)
        lines.append("")

    if namespace.functions:
        lines.extend(["## Functions", ""])
        for function in namespace.functions:
            lines.extend(render_function(function, source_base_url=source_base_url))

    if namespace.enums:
        lines.extend(["## Enums", ""])
        for enum in namespace.enums:
            lines.extend(render_enum(enum, source_base_url=source_base_url))

    return "\n".join(lines).rstrip() + "\n"


def render_class_page(
    klass: CppApiClass,
    *,
    source_base_url: str | None,
) -> str:
    lines = [
        frontmatter(
            klass.display_name,
            klass.doc,
            slug=class_slug(klass),
            sidebar_label=klass.display_name,
        )
    ]
    lines.extend([f"# {klass.display_name}", ""])
    if klass.doc:
        lines.extend([mdx_text(klass.doc), ""])
    render_page_metadata(
        lines,
        qualified_name=klass.qualified_name,
        kind=klass.kind,
        location=klass.location,
        source_base_url=source_base_url,
    )

    if klass.bases:
        lines.extend(["## Bases", ""])
        lines.extend(f"- `{base}`" for base in klass.bases)
        lines.append("")

    if klass.constructors:
        lines.extend(["## Constructors", ""])
        for constructor in klass.constructors:
            lines.extend(render_function(constructor, source_base_url=source_base_url))

    if klass.destructor is not None:
        lines.extend(["## Destructor", ""])
        lines.extend(render_function(klass.destructor, source_base_url=source_base_url))

    if klass.methods:
        lines.extend(["## Methods", ""])
        for method in klass.methods:
            lines.extend(render_function(method, source_base_url=source_base_url))

    if klass.enums:
        lines.extend(["## Enums", ""])
        for enum in klass.enums:
            lines.extend(render_enum(enum, source_base_url=source_base_url))

    return "\n".join(lines).rstrip() + "\n"


def render_root_index(model: CppApiModel) -> str:
    lines = [
        frontmatter(
            "C++ API",
            "Generated reference pages from FreeCAD Doxygen XML.",
            slug=CPP_API_ROOT,
            sidebar_label="C++ API",
        ),
        "# C++ API",
        "",
        "Generated reference pages derived from FreeCAD's Doxygen XML output.",
        "",
        "## Namespaces",
        "",
    ]
    top_level = tuple(namespace for namespace in model.namespaces if namespace.parent_name is None)
    lines.extend(render_namespace_summary(namespace) for namespace in top_level)
    lines.append("")
    return "\n".join(lines)


def write_cpp_api_markdown_docs(
    out_dir: Path,
    model: CppApiModel,
    *,
    source_base_url: str | None,
) -> int:
    """Write generated Starlight-ready MDX pages for the C++ API model."""

    cpp_root = content_root_dir(out_dir)
    shutil.rmtree(cpp_root, ignore_errors=True)
    cpp_root.mkdir(parents=True, exist_ok=True)
    validate_class_paths(out_dir, model.classes)

    page_count = 0
    root_path = cpp_root / "index.mdx"
    root_path.write_text(render_root_index(model), encoding="utf-8")
    page_count += 1

    for namespace in model.namespaces:
        path = namespace_doc_path(out_dir, namespace.qualified_name)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(
            render_namespace_page(namespace, model, source_base_url=source_base_url),
            encoding="utf-8",
        )
        page_count += 1

    for klass in model.classes:
        path = class_doc_path(out_dir, klass)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(
            render_class_page(klass, source_base_url=source_base_url),
            encoding="utf-8",
        )
        page_count += 1

    return page_count
