# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import ast
import textwrap
import unittest

from python_api_model.signatures import (
    ArgumentKind,
    parse_callable_signature,
)


class SignatureParserTests(unittest.TestCase):
    def parse(self, source: str):
        node = ast.parse(textwrap.dedent(source).lstrip()).body[0]
        if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            self.fail("expected a function definition")
        return parse_callable_signature(node)

    def test_signature_keeps_structured_semantics_without_rendered_text(self):
        signature = self.parse("""
            @classmethod
            @typing.overload
            @typing.deprecated('use replacement')
            def build(cls, value: int, /, *, name: str = 'x') -> str: ...
            """)

        self.assertEqual(signature.parameters[0].kind, ArgumentKind.POSITION_ONLY)
        self.assertEqual(signature.parameters[1].annotation, "int")
        self.assertEqual(signature.parameters[2].kind, ArgumentKind.KEYWORD_ONLY)
        self.assertTrue(signature.flags.classmethod)
        self.assertTrue(signature.flags.overload)
        self.assertEqual(signature.deprecated_message, "use replacement")

    def test_property_setter_is_semantic(self):
        signature = self.parse("""
            @value.setter
            def value(self, value: int) -> None: ...
            """)

        self.assertTrue(signature.flags.property_setter)
        self.assertFalse(signature.flags.property_getter)

    def test_async_and_variadic_parameters_are_preserved(self):
        signature = self.parse(
            "async def run(self, value: int, *args: str, flag: bool = True, **kwargs: object) -> None: ..."
        )

        self.assertTrue(signature.is_async)
        self.assertEqual(signature.parameters[2].kind, ArgumentKind.VAR_POSITIONAL)
        self.assertEqual(signature.parameters[3].kind, ArgumentKind.KEYWORD_ONLY)
        self.assertEqual(signature.parameters[4].kind, ArgumentKind.VAR_KEYWORD)


if __name__ == "__main__":
    unittest.main()
