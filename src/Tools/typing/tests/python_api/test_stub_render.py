"""Contract tests for rendering curated Python API functions into stubs."""

from __future__ import annotations

import ast
from pathlib import Path
import tempfile
import unittest

from stubgen.model import StubSignature
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
    ApiSourceLocation,
)
from stubgen.python_api.extract import binding_class_aliases
from stubgen.class_merge import (
    append_api_model_class_stubs,
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


class PythonApiStubRenderTests(unittest.TestCase):
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

    def test_curated_module_functions_render_with_api_model_signatures(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "FreeCAD" / "Console.pyi"
            write_stub_file(
                path,
                stub_signature_overrides={},
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
                                decorators=("final",),
                                support_imports=("from typing import Final",),
                                support_body=("class Mode:\n    Read = 1",),
                                attributes=(ApiAttribute(name="ready", annotation="Final[bool]"),),
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
        self.assertIn("@final\nclass Settings", stub_source)
        self.assertIn('    """Application settings."""', stub_source)
        self.assertIn("from typing import Final", stub_source)
        self.assertIn("    ready: Final[bool]", stub_source)
        self.assertIn("from typing import overload", stub_source)
        self.assertIn("def open(self, path: str, /) -> object:", stub_source)
        self.assertIn("    class Mode:\n        Read = 1", stub_source)

    def test_deprecation_metadata_can_follow_discovered_registration_sources(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "FreeCAD" / "Console.pyi"
            write_stub_file(
                path,
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
