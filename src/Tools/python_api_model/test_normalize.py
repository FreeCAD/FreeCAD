# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import unittest

from python_api_model.normalize import normalize_source_type


class SourceTypeNormalizationTests(unittest.TestCase):
    def test_aliases_are_rendered_as_forward_references(self) -> None:
        self.assertEqual(normalize_source_type("AxisPy"), "'FreeCAD.Base.Axis'")
        self.assertEqual(
            normalize_source_type("list[AxisPy]"),
            "list['FreeCAD.Base.Axis']",
        )

    def test_string_literals_are_not_rewritten_as_type_names(self) -> None:
        self.assertEqual(
            normalize_source_type('Literal["AxisPy"]'),
            "Literal['AxisPy']",
        )
        self.assertEqual(
            normalize_source_type('Annotated["AxisPy", "AxisPy"]'),
            "Annotated['FreeCAD.Base.Axis', 'AxisPy']",
        )

    def test_forward_reference_strings_are_normalized_in_annotation_context(self) -> None:
        self.assertEqual(
            normalize_source_type('"AxisPy"'),
            "'FreeCAD.Base.Axis'",
        )
        self.assertEqual(
            normalize_source_type('"Part.Shape"', "Part"),
            "'Shape'",
        )

    def test_identifier_substrings_are_not_rewritten(self) -> None:
        self.assertEqual(normalize_source_type("AxisPython"), "AxisPython")


if __name__ == "__main__":
    unittest.main()
