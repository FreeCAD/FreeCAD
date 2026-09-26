# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import unittest
from dataclasses import replace

from python_api_model.model import (
    ApiAttribute,
    ApiCallableGroup,
    ApiClass,
    ApiModule,
    ApiOrigin,
    ApiSourceLocation,
    PythonApiModel,
)
from python_api_model.resolve import (
    merge_api_class,
    merge_api_models,
    merge_api_module,
    resolve_declaration,
)
from python_api_model.signatures import CallableDecoratorFlags, CallableSignature


def function(return_annotation: str, path: str) -> ApiCallableGroup:
    return ApiCallableGroup(
        name="ping",
        signatures=(
            CallableSignature(
                name="ping",
                parameters=(),
                return_annotation=return_annotation,
                docstring=None,
                flags=CallableDecoratorFlags(),
            ),
        ),
        origin=ApiOrigin.MODULE_STUB,
        location=ApiSourceLocation(path, 1),
    )


class PythonApiResolutionTests(unittest.TestCase):
    def test_semantic_equality_ignores_provenance(self) -> None:
        existing = ApiAttribute(
            name="value",
            annotation="int",
            origin=ApiOrigin.MODULE_STUB,
            location=ApiSourceLocation("src/App/FreeCAD.module.pyi", 4),
        )
        incoming = replace(
            existing,
            origin=ApiOrigin.TYPE_STUB,
            location=ApiSourceLocation("src/Gui/FreeCADGui.Value.pyi", 8),
        )

        result = resolve_declaration(
            existing,
            incoming,
            kind="attribute",
            symbol="FreeCAD.value",
        )

        self.assertEqual(result.value.origin, ApiOrigin.MODULE_STUB)
        self.assertEqual(result.diagnostics, ())

    def test_equal_class_headers_prefer_higher_precedence_origin(self) -> None:
        generated = ApiClass(
            module_name="Demo",
            name="Widget",
            bases=("Base",),
            origin=ApiOrigin.GENERATED,
            location=ApiSourceLocation("z.cpp", 20),
        )
        curated = replace(
            generated,
            origin=ApiOrigin.TYPE_STUB,
            location=ApiSourceLocation("a.pyi", 3),
        )

        result = merge_api_class(generated, curated)

        self.assertEqual(result.value.origin, ApiOrigin.TYPE_STUB)
        self.assertEqual(result.diagnostics, ())

    def test_conflict_diagnostic_is_independent_of_input_order(self) -> None:
        existing = function("int", "b.pyi")
        incoming = replace(
            function("str", "a.pyi"),
            origin=ApiOrigin.TYPE_STUB,
        )

        first = resolve_declaration(
            existing,
            incoming,
            kind="function",
            symbol="Demo.ping",
        )
        second = resolve_declaration(
            incoming,
            existing,
            kind="function",
            symbol="Demo.ping",
        )

        self.assertEqual(first.value, second.value)
        self.assertEqual(first.diagnostics, second.diagnostics)

    def test_higher_precedence_declaration_wins(self) -> None:
        generated = ApiAttribute(name="value", annotation="Any", origin=ApiOrigin.GENERATED)
        curated = ApiAttribute(name="value", annotation="int", origin=ApiOrigin.MODULE_STUB)

        result = resolve_declaration(
            generated,
            curated,
            kind="attribute",
            symbol="Demo.value",
        )

        self.assertEqual(result.value, curated)
        self.assertEqual(result.diagnostics, ())

    def test_same_precedence_resolution_is_independent_of_input_order(self) -> None:
        first = ApiModule(name="Demo", functions=(function("int", "b.pyi"),))
        second = ApiModule(name="Demo", functions=(function("str", "a.pyi"),))

        first_result = merge_api_module(first, second).value
        second_result = merge_api_module(second, first).value

        self.assertEqual(first_result.functions, second_result.functions)
        self.assertEqual(first_result.functions[0].signatures[0].return_annotation, "str")

    def test_class_members_from_non_conflicting_inputs_are_preserved(self) -> None:
        first = ApiModule(
            name="Demo",
            attributes=(ApiAttribute(name="first", annotation="int"),),
        )
        second = ApiModule(
            name="Demo",
            attributes=(ApiAttribute(name="second", annotation="str"),),
        )

        result = merge_api_module(first, second).value

        self.assertEqual(
            [attribute.name for attribute in result.attributes],
            ["first", "second"],
        )

    def test_complementary_class_fragments_do_not_conflict(self) -> None:
        first = ApiClass(
            module_name="Demo",
            name="Widget",
            methods=(function("int", "a.pyi"),),
        )
        second = ApiClass(
            module_name="Demo",
            name="Widget",
            methods=(
                ApiCallableGroup(
                    name="refresh",
                    signatures=(
                        CallableSignature(
                            name="refresh",
                            parameters=(),
                            return_annotation="None",
                            docstring=None,
                            flags=CallableDecoratorFlags(),
                        ),
                    ),
                ),
            ),
        )

        result = merge_api_class(first, second)

        self.assertEqual(result.diagnostics, ())
        self.assertEqual([method.name for method in result.value.methods], ["ping", "refresh"])

    def test_conflicting_class_headers_are_reported(self) -> None:
        first = ApiClass(module_name="Demo", name="Widget", bases=("Base",))
        second = ApiClass(module_name="Demo", name="Widget", bases=("Other",))

        result = merge_api_class(first, second)

        self.assertEqual(len(result.diagnostics), 1)
        self.assertEqual(result.diagnostics[0].code, "conflicting-definition")

    def test_model_merge_applies_member_precedence_once(self) -> None:
        curated = ApiModule(
            name="Demo",
            functions=(function("int", "curated.pyi"),),
            origin=ApiOrigin.MODULE_STUB,
        )
        generated = ApiModule(
            name="Demo",
            functions=(replace(function("str", "generated.cpp"), origin=ApiOrigin.GENERATED),),
            origin=ApiOrigin.GENERATED,
        )

        result = merge_api_models(
            PythonApiModel(modules=(curated,)),
            PythonApiModel(modules=(generated,)),
        )

        self.assertEqual(
            result.value.modules[0].functions[0].signatures[0].return_annotation, "int"
        )
        self.assertEqual(result.diagnostics[0].code, "lower-precedence-definition")


if __name__ == "__main__":
    unittest.main()
