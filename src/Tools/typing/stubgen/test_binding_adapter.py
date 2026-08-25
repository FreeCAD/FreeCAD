# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from dataclasses import replace
import unittest

from stubgen.model import BindingMethod, ContextKind, MethodKind, StubSignature
from python_api_model.signatures import ArgumentKind

from stubgen.binding_adapter import adapt_discovered_bindings


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
    def test_discovered_method_is_normalized_without_rendered_stub_text(self) -> None:
        model = adapt_discovered_bindings(
            [discovered_method(method_kind="keyword")],
            {},
            {},
        )

        signature = model.modules[0].functions[0].signatures[0]
        self.assertEqual(
            [parameter.kind for parameter in signature.parameters],
            [ArgumentKind.VAR_POSITIONAL, ArgumentKind.VAR_KEYWORD],
        )

    def test_class_method_gets_self_without_rendering_and_reparsing(self) -> None:
        model = adapt_discovered_bindings(
            [
                discovered_method(
                    context_kind="python_type",
                    context_name="Widget",
                    inferred_module=None,
                    python_name="refresh",
                    method_kind="noargs",
                )
            ],
            {"Widget": ["FreeCAD.Widget"]},
            {},
        )

        signature = model.modules[0].classes[0].methods[0].signatures[0]
        self.assertEqual(signature.parameters[0].name, "self")

    def test_curated_signature_override_is_normalized_into_parameters(self) -> None:
        method = discovered_method(python_name="printMessage")
        model = adapt_discovered_bindings(
            [method],
            {},
            {
                (method.source, method.context_name, method.python_name): (
                    StubSignature(
                        parameters="message: str",
                        returns="bool",
                        doc="Return whether the message was accepted.",
                    ),
                )
            },
        )

        signature = model.modules[0].functions[0].signatures[0]
        self.assertEqual(signature.parameters[0].annotation, "str")
        self.assertEqual(signature.return_annotation, "bool")
        self.assertEqual(signature.docstring, "Return whether the message was accepted.")

    def test_all_overrides_for_one_method_are_preserved_as_overloads(self) -> None:
        method = discovered_method(python_name="printMessage")
        model = adapt_discovered_bindings(
            [method],
            {},
            {
                (method.source, method.context_name, method.python_name): (
                    StubSignature(parameters="message: str", returns="bool"),
                    StubSignature(parameters="message: bytes", returns="int"),
                )
            },
        )

        group = model.modules[0].functions[0]
        self.assertEqual(len(group.signatures), 2)
        self.assertEqual(
            [signature.return_annotation for signature in group.signatures],
            ["bool", "int"],
        )
        self.assertTrue(group.overload)

    def test_overrides_on_later_binding_methods_are_preserved(self) -> None:
        first = discovered_method(python_name="printMessage")
        later = discovered_method(
            python_name="printMessage",
            doc="The later registration.",
        )
        later = replace(later, source="src/App/OtherConsole.cpp", line=43)
        model = adapt_discovered_bindings(
            [first, later],
            {},
            {
                (later.source, later.context_name, later.python_name): (
                    StubSignature(parameters="message: str", returns="bool"),
                    StubSignature(parameters="message: bytes", returns="int"),
                )
            },
        )

        group = model.modules[0].functions[0]
        self.assertEqual(len(group.signatures), 3)
        self.assertEqual(
            [signature.return_annotation for signature in group.signatures],
            ["Any", "bool", "int"],
        )
        self.assertTrue(group.overload)

    def test_source_type_aliases_are_normalized_in_binding_overrides(self) -> None:
        method = discovered_method(
            context_kind="python_type",
            context_name="Placement",
            inferred_module=None,
            python_name="transform",
        )
        model = adapt_discovered_bindings(
            [method],
            {"Placement": ["FreeCAD.Base.Placement"]},
            {
                (method.source, method.context_name, method.python_name): (
                    StubSignature(
                        parameters="matrix: MatrixPy",
                        returns="MatrixPy",
                    ),
                )
            },
        )

        signature = model.modules[0].classes[0].methods[0].signatures[0]
        self.assertEqual(signature.parameters[1].annotation, "'FreeCAD.Base.Matrix'")
        self.assertEqual(signature.return_annotation, "'FreeCAD.Base.Matrix'")


if __name__ == "__main__":
    unittest.main()
