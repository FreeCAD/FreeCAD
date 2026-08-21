# pyright: strict

"""Contract tests for adapting discovered bindings into ``ApiModel``."""

from __future__ import annotations

import unittest

from stubgen.model import BindingMethod, ContextKind, MethodKind
from stubgen.python_api.adapters import merge_discovered_bindings
from stubgen.python_api.model import ApiCallableGroup, ApiModel, ApiModule


def discovered_method(
    *,
    context_kind: ContextKind = "pycxx_module",
    context_name: str = "FreeCAD.Console",
    inferred_module: str | None = "FreeCAD.Console",
    python_name: str = "printMessage",
    method_kind: MethodKind = "varargs",
    doc: str = "Write a message.",
) -> BindingMethod:
    return BindingMethod(
        family="pycxx_add_method",
        source="src/App/Console.cpp",
        line=42,
        table=None,
        context_kind=context_kind,
        context_name=context_name,
        inferred_module=inferred_module,
        method_kind=method_kind,
        python_name=python_name,
        cxx_callable="Console::printMessage",
        flags="",
        doc=doc,
        generated_source=True,
    )


class PythonApiAdapterTests(unittest.TestCase):
    def test_discovered_module_functions_become_model_groups(self) -> None:
        model = merge_discovered_bindings(
            ApiModel(),
            [discovered_method()],
            {},
            {},
        )

        self.assertEqual(len(model.modules), 1)
        function = model.modules[0].functions[0]
        self.assertEqual(function.name, "printMessage")
        self.assertEqual(function.doc, "Write a message.")
        self.assertEqual(
            function.signatures[0].display_signature, "printMessage(*args: Any) -> Any"
        )

    def test_discovered_type_methods_become_model_classes(self) -> None:
        model = merge_discovered_bindings(
            ApiModel(),
            [
                discovered_method(
                    context_kind="python_type",
                    context_name="Widget",
                    inferred_module=None,
                    python_name="refresh",
                )
            ],
            {"Widget": ["FreeCAD.Widget"]},
            {},
        )

        api_class = model.modules[0].classes[0]
        self.assertEqual(api_class.qualified_name, "FreeCAD.Widget")
        self.assertEqual(api_class.methods[0].name, "refresh")
        self.assertEqual(
            api_class.methods[0].signatures[0].display_signature,
            "refresh(*args: Any) -> Any",
        )

    def test_curated_functions_take_precedence_over_discovery(self) -> None:
        curated = ApiCallableGroup(
            name="printMessage",
            signatures=(),
            doc="Curated documentation.",
        )
        model = merge_discovered_bindings(
            ApiModel(modules=(ApiModule(name="FreeCAD.Console", functions=(curated,)),)),
            [discovered_method()],
            {},
            {},
        )

        self.assertEqual(model.modules[0].functions, (curated,))


if __name__ == "__main__":
    unittest.main()
