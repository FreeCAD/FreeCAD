# pyright: strict

"""Generate a Starlight sidebar fragment for the C++ API docs tree."""

from __future__ import annotations

import json
from pathlib import Path

from .markdown import CPP_API_ROOT, class_slug, namespace_slug, page_link
from .model import CppApiClass, CppApiModel, CppApiNamespace


def namespace_children(
    namespace: CppApiNamespace, namespaces: tuple[CppApiNamespace, ...]
) -> tuple[CppApiNamespace, ...]:
    return tuple(child for child in namespaces if child.parent_name == namespace.qualified_name)


def namespace_classes(
    namespace: CppApiNamespace, classes: tuple[CppApiClass, ...]
) -> tuple[CppApiClass, ...]:
    return tuple(klass for klass in classes if klass.namespace_name == namespace.qualified_name)


def namespace_sidebar_group(
    namespace: CppApiNamespace,
    model: CppApiModel,
) -> dict[str, object]:
    items: list[dict[str, object]] = [
        {"label": "Overview", "link": page_link(namespace_slug(namespace.qualified_name))}
    ]
    children = namespace_children(namespace, model.namespaces)
    if children:
        items.append(
            {
                "label": "Namespaces",
                "items": [namespace_sidebar_group(child, model) for child in children],
            }
        )
    classes = namespace_classes(namespace, model.classes)
    if classes:
        items.append(
            {
                "label": "Types",
                "items": [
                    {"label": klass.display_name, "link": page_link(class_slug(klass))}
                    for klass in classes
                ],
            }
        )
    return {"label": namespace.name, "items": items}


def sidebar_items(model: CppApiModel) -> list[dict[str, object]]:
    items: list[dict[str, object]] = [{"label": "Overview", "link": page_link(CPP_API_ROOT)}]
    top_level = tuple(namespace for namespace in model.namespaces if namespace.parent_name is None)
    items.extend(namespace_sidebar_group(namespace, model) for namespace in top_level)
    return [{"label": "C++ API", "items": items}]


def render_cpp_starlight_sidebar_fragment(model: CppApiModel) -> str:
    items_json = json.dumps(sidebar_items(model), indent=2)
    return (
        "import type { StarlightSidebarUserConfig } from '@astrojs/starlight/types';\n\n"
        f"export const cppApiSidebar: StarlightSidebarUserConfig = {items_json};\n"
    )


def write_cpp_starlight_sidebar_fragment(path: Path, model: CppApiModel) -> Path:
    """Write the generated Starlight sidebar fragment for C++ docs."""

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(render_cpp_starlight_sidebar_fragment(model), encoding="utf-8")
    return path
