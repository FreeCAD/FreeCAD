"""Regression tests for the Doxygen-based C++ API documentation pipeline."""

from __future__ import annotations

from pathlib import Path
import tempfile
import unittest
from xml.etree import ElementTree as ET

from stubgen.cpp_api.extract import extract_class, extract_enum, function_declaration
from stubgen.cpp_api.markdown import (
    class_slug,
    class_slug_token,
    namespace_classes,
    validate_class_paths,
)
from stubgen.cpp_api.model import CppApiClass, CppApiNamespace
from stubgen.cpp_api.starlight import namespace_classes as sidebar_namespace_classes
from stubgen.cli import display_path


def api_class(
    qualified_name: str,
    *,
    display_name: str,
    namespace_name: str,
) -> CppApiClass:
    return CppApiClass(
        qualified_name=qualified_name,
        name=display_name.rsplit("::", 1)[-1],
        display_name=display_name,
        namespace_name=namespace_name,
        top_namespace=namespace_name.split("::", 1)[0],
        kind="class",
    )


class CppApiDocsTests(unittest.TestCase):
    def test_mixed_content_xml_is_preserved(self) -> None:
        member = ET.fromstring("""
            <memberdef kind="function">
              <type><ref>Document</ref> *</type>
              <definition><ref>Document</ref> * Ns::open</definition>
              <argsstring>(<ref>Thing</ref> value)</argsstring>
              <exceptions>noexcept(<ref>safe</ref>)</exceptions>
              <name>open</name>
            </memberdef>
            """)

        declaration = function_declaration(member)

        self.assertIn("Document *", declaration)
        self.assertIn("Thing value", declaration)
        self.assertIn("noexcept(safe)", declaration)

    def test_mixed_content_enum_initializer_is_preserved(self) -> None:
        enum = extract_enum(
            ET.fromstring("""
                <memberdef kind="enum" strong="yes">
                  <name>Mode</name>
                  <enumvalue>
                    <name>Active</name>
                    <initializer>= <ref>State::Active</ref></initializer>
                  </enumvalue>
                </memberdef>
                """),
            Path("/repo"),
        )

        self.assertEqual(enum.values[0].initializer, "= State::Active")

    def test_templated_class_lifecycle_members_are_classified(self) -> None:
        klass = extract_class(
            Path("/repo"),
            ET.fromstring("""
                <compounddef kind="class">
                  <compoundname>App::Widget&lt;T&gt;</compoundname>
                  <location file="src/App/Widget.h" line="10"/>
                  <sectiondef kind="public-func">
                    <memberdef kind="function"><name>Widget</name><argsstring>()</argsstring></memberdef>
                    <memberdef kind="function"><name>~Widget</name><argsstring>()</argsstring></memberdef>
                    <memberdef kind="function"><name>reset</name><argsstring>()</argsstring></memberdef>
                  </sectiondef>
                </compounddef>
                """),
        )

        assert klass is not None
        self.assertEqual([method.name for method in klass.constructors], ["Widget"])
        self.assertEqual(klass.destructor.name if klass.destructor else None, "~Widget")
        self.assertEqual([method.name for method in klass.methods], ["reset"])

    def test_nested_classes_use_the_immediate_namespace(self) -> None:
        klass = api_class(
            "App::Extension::Widget",
            display_name="Extension::Widget",
            namespace_name="App::Extension",
        )
        namespace = CppApiNamespace(
            qualified_name="App::Extension",
            name="Extension",
        )

        self.assertEqual(namespace_classes(namespace, (klass,)), (klass,))
        self.assertEqual(sidebar_namespace_classes(namespace, (klass,)), (klass,))
        self.assertIn("cpp-api/app/extension/types/", class_slug(klass))

    def test_absolute_paths_have_safe_cli_display(self) -> None:
        root = Path("/repo")
        self.assertEqual(display_path(root, root / "docs/sidebar.ts"), "docs/sidebar.ts")
        self.assertEqual(display_path(root, Path("/tmp/sidebar.ts")), "/tmp/sidebar.ts")

    def test_sanitized_template_slugs_do_not_collide(self) -> None:
        first = api_class(
            "Base::Foo<A*>",
            display_name="Foo<A*>",
            namespace_name="Base",
        )
        second = api_class(
            "Base::Foo<A&>",
            display_name="Foo<A&>",
            namespace_name="Base",
        )

        self.assertNotEqual(class_slug_token(first), class_slug_token(second))
        with tempfile.TemporaryDirectory() as directory:
            validate_class_paths(Path(directory), (first, second))


if __name__ == "__main__":
    unittest.main()
