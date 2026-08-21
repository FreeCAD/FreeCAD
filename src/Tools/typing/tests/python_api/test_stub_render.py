"""Contract tests for rendering curated Python API functions into stubs."""

from __future__ import annotations

import ast
from pathlib import Path
import tempfile
import unittest

from stubgen.model import BindingMethod, StubSignature
from stubgen.module_merge import (
    merge_api_module_aliases,
    merge_api_module_aliases_into_stubs,
    merge_api_module_attributes,
    merge_api_module_attributes_into_stubs,
)
from stubgen.model import BindingClass
from stubgen.python_api.model import (
    ApiAlias,
    ApiAttribute,
    ApiCallableGroup,
    ApiClass,
    ApiModel,
    ApiModule,
    ApiOrigin,
    ApiSourceLocation,
)
from stubgen.python_api.extract import binding_class_aliases
from stubgen.class_merge import (
    append_api_model_class_stubs,
    merge_api_class_attributes,
    merge_api_class_header,
    merge_api_class_methods,
)
from stubgen.generator import write_public_module_stubs
from stubgen.render import write_stub_file
from stubgen.signature_parser import group_callable_definitions, parse_callable_group


def api_module() -> ApiModule:
    tree = ast.parse("""
from typing import overload

@overload
def open(path: str, /) -> object: ...

@overload
def open(path: str, mode: str, /) -> object: ...
""")
    nodes = group_callable_definitions(tree.body)["open"]
    group = ApiCallableGroup(
        name="open",
        signatures=parse_callable_group(nodes),
        doc="Open a file through the console module.",
        location=ApiSourceLocation("src/Mod/Part/App/Part.module.pyi", 4),
    )
    return ApiModule(name="FreeCAD.Console", functions=(group,))


def binding_method() -> BindingMethod:
    return BindingMethod(
        family="module_stub",
        source="src/Mod/Part/App/Part.module.pyi",
        line=4,
        table=None,
        context_kind="pycxx_module",
        context_name="FreeCAD.Console",
        inferred_module="FreeCAD.Console",
        method_kind="varargs",
        python_name="open",
        cxx_callable="open",
        flags="",
        doc="",
        generated_source=False,
    )


class PythonApiStubRenderTests(unittest.TestCase):
    def test_api_class_header_replaces_only_the_declaration(self) -> None:
        source = """class Example(OldBase):
    value: int

    def run(self) -> None:
        ...
"""
        api_class = ApiClass(
            name="Example",
            module_name="Example",
            bases=("NewBase",),
        )

        output = merge_api_class_header(source, api_class)

        self.assertIn("class Example(NewBase):", output)
        self.assertIn("value: int", output)
        self.assertIn("def run(self) -> None:", output)

    def test_binding_class_public_names_become_api_aliases(self) -> None:
        aliases = binding_class_aliases(
            [
                BindingClass(
                    source="src/Base/Axis.pyi",
                    line=7,
                    class_name="Axis",
                    export_name="AxisPy",
                    python_name=None,
                    public_names=["FreeCAD.Axis", "FreeCAD.Base.Axis"],
                    base_class=None,
                    explicit_export=False,
                )
            ]
        )

        self.assertEqual(len(aliases), 1)
        self.assertEqual(aliases[0].public_path, "FreeCAD.Axis")
        self.assertEqual(aliases[0].target_path, "FreeCAD.Base.Axis")

    def test_api_aliases_add_missing_reexports_without_duplicates(self) -> None:
        source = """from __future__ import annotations

class Existing:
    ...
"""
        api_module = ApiModule(
            name="FreeCAD",
            aliases=(
                ApiAlias(
                    public_path="FreeCAD.Axis",
                    target_path="FreeCAD.Base.Axis",
                ),
            ),
        )

        output = merge_api_module_aliases(source, api_module)

        self.assertIn("from .Base import Axis as Axis", output)
        self.assertEqual(output.count("from .Base import Axis as Axis"), 1)

    def test_curated_module_attributes_replace_merged_assignments(self) -> None:
        target = """from __future__ import annotations

Original: int = 1
Alias = list[str]

def run() -> None: ...
"""
        api_module = ApiModule(
            name="Example",
            attributes=(
                ApiAttribute(name="Original", annotation="str", value='"value"'),
                ApiAttribute(name="Alias", value="tuple[str, ...]"),
            ),
        )

        output = merge_api_module_attributes(target, api_module)

        self.assertIn("Original: str = 'value'", output)
        self.assertIn("Alias = tuple[str, ...]", output)
        self.assertIn("def run() -> None:\n    ...", output)

    def test_curated_module_attributes_add_missing_assignments(self) -> None:
        target = """from __future__ import annotations

def run() -> None: ...
"""
        api_module = ApiModule(
            name="Example",
            attributes=(ApiAttribute(name="Ready", annotation="bool", value="True"),),
        )

        output = merge_api_module_attributes(target, api_module)

        self.assertIn("Ready: bool = True", output)
        self.assertLess(output.index("Ready"), output.index("def run"))

    def test_curated_class_methods_replace_merged_methods(self) -> None:
        source = '''# Generated header
class Other:
    pass

class Example:
    @property
    def value(self) -> int:
        """Return the current value."""
        return 1

def outside() -> None:
    ...
'''
        tree = ast.parse(source)
        class_node = tree.body[1]
        assert isinstance(class_node, ast.ClassDef)
        method_nodes: list[ast.FunctionDef | ast.AsyncFunctionDef] = [
            node for node in class_node.body if isinstance(node, ast.FunctionDef)
        ]
        group = ApiCallableGroup(
            name="value",
            signatures=parse_callable_group(method_nodes),
            doc="Return the current value.",
            is_method=True,
        )
        api_class = ApiClass(name="Example", module_name="Example", methods=(group,))

        output = merge_api_class_methods(source, api_class)

        self.assertIn("@property", output)
        self.assertIn('"""Return the current value."""', output)
        self.assertIn("def value(self) -> int:", output)
        self.assertIn("# Generated header", output)
        self.assertIn("class Other:\n    pass", output)
        self.assertIn("def outside() -> None:\n    ...", output)

    def test_curated_class_methods_add_missing_methods(self) -> None:
        source = "class Example:\n    ...\n"
        api_class = ApiClass(
            name="Example",
            module_name="Example",
            methods=api_module().functions,
        )

        output = merge_api_class_methods(source, api_class)

        self.assertIn("def open(self, path: str, /) -> object:", output)
        self.assertEqual(output.count("def open("), 2)

    def test_binding_method_merge_preserves_equivalent_docstring_formatting(self) -> None:
        source = '''class Example:
    def run(self, value: int) -> int:
        """
        Keep this existing multiline formatting.

        It is part of the authored stub surface.
        """
        ...
'''
        tree = ast.parse(source)
        class_node = tree.body[0]
        assert isinstance(class_node, ast.ClassDef)
        method_node = next(node for node in class_node.body if isinstance(node, ast.FunctionDef))
        group = ApiCallableGroup(
            name="run",
            signatures=parse_callable_group([method_node, method_node]),
            doc="Keep this existing multiline formatting.\n\nIt is part of the authored stub surface.",
            is_method=True,
            origin=ApiOrigin.BINDING_SPEC,
        )
        api_class = ApiClass(
            name="Example",
            module_name="Example",
            methods=(group,),
            origin=ApiOrigin.BINDING_SPEC,
        )

        output = merge_api_class_methods(source, api_class)

        self.assertIn("    @overload\n    def run", output)
        self.assertIn(
            '        """\n        Keep this existing multiline formatting.\n\n'
            '        It is part of the authored stub surface.\n        """',
            output,
        )

    def test_curated_class_attributes_replace_merged_assignments(self) -> None:
        source = """class Example:
    state: int = 1
    label = "old"
"""
        api_class = ApiClass(
            name="Example",
            module_name="Example",
            attributes=(
                ApiAttribute(name="state", annotation="str", value='"ready"'),
                ApiAttribute(name="label", value='"new"'),
            ),
        )

        output = merge_api_class_attributes(source, api_class)

        self.assertIn("state: str = 'ready'", output)
        self.assertIn("label = 'new'", output)

    def test_curated_class_attributes_add_missing_assignments(self) -> None:
        source = "class Example:\n    ...\n"
        api_class = ApiClass(
            name="Example",
            module_name="Example",
            attributes=(ApiAttribute(name="ready", annotation="bool"),),
        )

        output = merge_api_class_attributes(source, api_class)

        self.assertIn("ready: bool", output)
        self.assertNotIn("class Example:\n    ...", output)

    def test_curated_module_functions_render_with_api_model_signatures(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "FreeCAD" / "Console.pyi"
            write_stub_file(
                path,
                [binding_method()],
                api_module=api_module(),
                module_name="FreeCAD.Console",
            )

            output = path.read_text(encoding="utf-8")

        self.assertIn("from typing import Any, overload", output)
        self.assertEqual(output.count("def open("), 2)
        self.assertIn("def open(path: str, /) -> object:", output)
        self.assertIn("def open(path: str, mode: str, /) -> object:", output)
        self.assertIn('    """Open a file through the console module."""', output)
        self.assertEqual(output.count("    ..."), 2)

    def test_curated_module_functions_render_without_discovered_methods(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "stubs"
            write_public_module_stubs(
                output,
                {"FreeCAD.Standalone"},
                {},
                ApiModel(
                    modules=(
                        ApiModule(
                            name="FreeCAD.Standalone",
                            functions=api_module().functions,
                        ),
                    ),
                ),
            )
            generated = output / "FreeCAD" / "Standalone.pyi"
            stub_source = generated.read_text(encoding="utf-8")

        self.assertEqual(stub_source.count("def open("), 2)
        self.assertIn("def open(path: str, /) -> object:", stub_source)

    def test_curated_module_attributes_render_without_discovered_methods(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "stubs"
            model = ApiModel(
                modules=(
                    ApiModule(
                        name="FreeCAD.Constants",
                        attributes=(ApiAttribute(name="Ready", annotation="bool"),),
                    ),
                ),
            )
            write_public_module_stubs(output, {"FreeCAD.Constants"}, {}, model)
            merge_api_module_attributes_into_stubs(output, model, {"FreeCAD.Constants"})
            generated = output / "FreeCAD" / "Constants.pyi"
            stub_source = generated.read_text(encoding="utf-8")

        self.assertIn("Ready: bool", stub_source)

    def test_curated_module_aliases_render_without_discovered_methods(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "stubs"
            model = ApiModel(
                modules=(
                    ApiModule(
                        name="FreeCAD.Exports",
                        aliases=(
                            ApiAlias(
                                public_path="FreeCAD.Exports.Axis",
                                target_path="FreeCAD.Base.Axis",
                            ),
                        ),
                    ),
                ),
            )
            write_public_module_stubs(output, {"FreeCAD.Exports"}, {}, model)
            merge_api_module_aliases_into_stubs(output, model, {"FreeCAD.Exports"})
            generated = output / "FreeCAD" / "Exports.pyi"
            stub_source = generated.read_text(encoding="utf-8")

        self.assertIn("from FreeCAD.Base import Axis as Axis", stub_source)

    def test_curated_class_renders_without_discovered_binding(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "stubs"
            model = ApiModel(
                modules=(
                    ApiModule(
                        name="FreeCAD.Constants",
                        classes=(
                            ApiClass(
                                name="Settings",
                                module_name="FreeCAD.Constants",
                                doc="Application settings.",
                                bases=("BaseSettings",),
                                methods=api_module().functions,
                            ),
                        ),
                    ),
                ),
            )
            append_api_model_class_stubs(output, model, {"FreeCAD.Constants"})
            generated = output / "FreeCAD" / "Constants.pyi"
            stub_source = generated.read_text(encoding="utf-8")

        self.assertIn("class Settings(BaseSettings):", stub_source)
        self.assertIn('    """Application settings."""', stub_source)
        self.assertIn("from typing import overload", stub_source)
        self.assertIn("def open(self, path: str, /) -> object:", stub_source)

    def test_deprecation_metadata_can_follow_discovered_registration_sources(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "FreeCAD" / "Console.pyi"
            write_stub_file(
                path,
                [binding_method()],
                api_module=api_module(),
                module_name="FreeCAD.Console",
                stub_signature_overrides={
                    (
                        "src/runtime/Console.cpp",
                        "FreeCAD.Console",
                        "open",
                    ): (
                        StubSignature(
                            "path: str, /",
                            "object",
                            deprecated_message="use read instead",
                        ),
                        StubSignature("path: str, mode: str, /", "object"),
                    )
                },
            )

            output = path.read_text(encoding="utf-8")

        self.assertIn("from typing_extensions import deprecated", output)
        self.assertIn("@deprecated('use read instead')", output)


if __name__ == "__main__":
    unittest.main()
