"""Contract tests for C++ API output renderers."""

from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from stubgen.cpp_api.markdown import render_class_page, render_enum, write_cpp_api_markdown_docs
from stubgen.cpp_api.model import (
    CppApiClass,
    CppApiEnum,
    CppApiEnumValue,
    CppApiFunction,
    CppApiModel,
    CppApiNamespace,
)
from stubgen.cpp_api.starlight import render_cpp_starlight_sidebar_fragment


def api_class() -> CppApiClass:
    return CppApiClass(
        qualified_name="App::Widget",
        name="Widget",
        display_name="Widget",
        namespace_name="App",
        top_namespace="App",
        kind="class",
        constructors=(CppApiFunction("Widget", "Widget()"),),
        destructor=CppApiFunction("~Widget", "~Widget()"),
        methods=(CppApiFunction("reset", "void reset()"),),
    )


class CppApiRenderTests(unittest.TestCase):
    def test_enum_initializer_is_rendered_once(self) -> None:
        enum = CppApiEnum(
            name="Mode",
            declaration="enum class Mode",
            values=(CppApiEnumValue(name="Active", initializer="State::Active"),),
        )

        page = render_enum(enum, source_base_url=None)

        self.assertIn("`Active = State::Active`", "\n".join(page))
        self.assertNotIn("= =", "\n".join(page))

    def test_class_page_keeps_callable_categories_separate(self) -> None:
        page = render_class_page(api_class(), source_base_url=None)

        self.assertIn("## Constructors", page)
        self.assertIn("## Destructor", page)
        self.assertIn("## Methods", page)

    def test_sidebar_is_rendered_from_the_model(self) -> None:
        namespace = CppApiNamespace(qualified_name="App", name="App")
        model = CppApiModel(namespaces=(namespace,), classes=(api_class(),))

        sidebar = render_cpp_starlight_sidebar_fragment(model)

        self.assertIn('"label": "C++ API"', sidebar)
        self.assertIn("cpp-api/app/types/Widget/", sidebar)

    def test_writer_owns_the_cpp_output_tree(self) -> None:
        namespace = CppApiNamespace(qualified_name="App", name="App")
        model = CppApiModel(namespaces=(namespace,), classes=(api_class(),))
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "docs"
            page_count = write_cpp_api_markdown_docs(output, model, source_base_url=None)

            self.assertEqual(page_count, 3)
            self.assertTrue((output / "cpp-api/index.mdx").exists())
            self.assertTrue((output / "cpp-api/app/types/Widget.mdx").exists())
