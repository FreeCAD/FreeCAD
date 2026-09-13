# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from stubgen.model import BindingClass, BindingMethod
from stubgen.validation import validate_discovered_bindings, validate_public_class_aliases


class ValidationTests(unittest.TestCase):
    def test_invalid_public_alias_symbol_is_rejected(self) -> None:
        binding_class = BindingClass(
            source="src/Demo.pyi",
            line=1,
            class_name="DemoPy",
            export_name="DemoPy",
            python_name=None,
            public_names=("Demo.Valid", "Demo.not-valid"),
            base_class=None,
            explicit_export=True,
        )

        with self.assertRaisesRegex(ValueError, "invalid public symbol"):
            validate_public_class_aliases([binding_class])

    def test_invalid_discovered_public_target_is_rejected(self) -> None:
        with self.assertRaisesRegex(ValueError, "invalid public class symbol"):
            validate_discovered_bindings([], {"DemoPy": ["Demo.not-valid"]})

    def test_invalid_discovered_method_name_is_rejected(self) -> None:
        method = BindingMethod(
            family="pycxx_add_method",
            source="src/Demo.cpp",
            line=12,
            table=None,
            context_kind="pycxx_module",
            context_name="Demo",
            inferred_module="Demo",
            method_kind="noargs",
            python_name="not-valid",
            cxx_callable="Demo::notValid",
            flags="",
            doc="",
            generated_source=True,
        )

        with self.assertRaisesRegex(ValueError, "invalid Python method name"):
            validate_discovered_bindings([method], {})


if __name__ == "__main__":
    unittest.main()
