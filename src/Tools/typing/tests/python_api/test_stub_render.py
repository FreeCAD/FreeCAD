"""Contract tests for rendering curated Python API functions into stubs."""

from __future__ import annotations

import ast
from pathlib import Path
import tempfile
import unittest

from stubgen.model import BindingMethod, StubSignature
from stubgen.python_api.model import ApiCallableGroup, ApiModule, ApiSourceLocation
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
