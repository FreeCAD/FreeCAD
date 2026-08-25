# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import unittest

from python_api_model.model import (
    ApiAttribute,
    ApiCallableGroup,
    ApiClass,
    ApiModule,
)
from stubgen.render import render_module
from python_api_model.signatures import (
    ArgumentKind,
    CallableDecoratorFlags,
    CallableSignature,
    SignatureParameter,
)
from stubgen.stub_support import StubSupport


def signature(
    name: str,
    return_annotation: str,
    *,
    parameters: tuple[SignatureParameter, ...] = (),
    flags: CallableDecoratorFlags = CallableDecoratorFlags(),
    decorators: tuple[str, ...] = (),
    deprecated_message: str | None = None,
) -> CallableSignature:
    return CallableSignature(
        name=name,
        parameters=parameters,
        return_annotation=return_annotation,
        docstring=None,
        flags=flags,
        decorators=decorators,
        deprecated_message=deprecated_message,
    )


class StubRenderTests(unittest.TestCase):
    def test_model_and_support_render_to_one_stub_module(self) -> None:
        module = ApiModule(
            name="Demo",
            functions=(
                ApiCallableGroup(
                    name="ping",
                    signatures=(signature("ping", "bool"),),
                ),
            ),
            classes=(
                ApiClass(
                    module_name="Demo",
                    name="Widget",
                    attributes=(ApiAttribute(name="value", annotation="int"),),
                    methods=(
                        ApiCallableGroup(
                            name="refresh",
                            signatures=(
                                signature(
                                    "refresh",
                                    "None",
                                    parameters=(
                                        SignatureParameter(
                                            name="self",
                                            annotation=None,
                                            kind=ArgumentKind.POSITIONAL_OR_KEYWORD,
                                        ),
                                    ),
                                ),
                            ),
                        ),
                    ),
                ),
            ),
        )
        support = StubSupport(
            module_fragments=(("Demo", "_T = TypeVar('_T')\n"),),
            class_fragments=(("Demo.Widget", "_token: object\n"),),
        )

        rendered = render_module(module, support=support)

        self.assertIn("_T = TypeVar('_T')", rendered)
        self.assertIn("def ping() -> bool:", rendered)
        self.assertIn("class Widget:", rendered)
        self.assertIn("value: int", rendered)
        self.assertIn("_token: object", rendered)
        self.assertIn("def refresh(self) -> None:", rendered)

    def test_public_decorators_survive_model_rendering(self) -> None:
        module = ApiModule(
            name="Demo",
            classes=(
                ApiClass(
                    module_name="Demo",
                    name="Widget",
                    decorators=("final",),
                    methods=(
                        ApiCallableGroup(
                            name="refresh",
                            signatures=(
                                signature(
                                    "refresh",
                                    "None",
                                    decorators=("override",),
                                ),
                            ),
                        ),
                    ),
                ),
            ),
        )

        rendered = render_module(module)

        self.assertIn("@final\nclass Widget:", rendered)
        self.assertIn("    @override\n    def refresh() -> None:", rendered)

    def test_semantic_decorators_are_not_duplicated(self) -> None:
        module = ApiModule(
            name="Demo",
            classes=(
                ApiClass(
                    module_name="Demo",
                    name="Widget",
                    methods=(
                        ApiCallableGroup(
                            name="build",
                            signatures=(
                                signature(
                                    "build",
                                    "Widget",
                                    flags=CallableDecoratorFlags(classmethod=True),
                                    decorators=("classmethod", "runtime_checkable"),
                                ),
                            ),
                        ),
                    ),
                ),
            ),
        )

        rendered = render_module(module)

        self.assertEqual(rendered.count("@classmethod"), 1)
        self.assertIn("    @runtime_checkable\n    def build() -> Widget:", rendered)

    def test_qualified_semantic_decorators_are_not_duplicated(self) -> None:
        module = ApiModule(
            name="Demo",
            functions=(
                ApiCallableGroup(
                    name="build",
                    signatures=(
                        signature(
                            "build",
                            "Widget",
                            flags=CallableDecoratorFlags(overload=True),
                            decorators=("typing.overload", "typing.deprecated('old')"),
                            deprecated_message="old",
                        ),
                    ),
                ),
            ),
        )

        rendered = render_module(module)

        self.assertEqual(rendered.count("@overload"), 1)
        self.assertEqual(rendered.count("@deprecated('old')"), 1)
        self.assertNotIn("typing.overload", rendered)
        self.assertNotIn("typing.deprecated", rendered)


if __name__ == "__main__":
    unittest.main()
