# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import inspect
from pathlib import Path
import sys
import unittest
import warnings

EXT_ROOT = Path(__file__).resolve().parents[2] / "Ext"
if str(EXT_ROOT) not in sys.path:
    sys.path.insert(0, str(EXT_ROOT))

from freecad.deprecation import deprecated, _unwrap_deprecated_frame  # noqa: E402


class DeprecationTest(unittest.TestCase):
    def test_decorator_warns_with_lifecycle_and_replacement(self) -> None:
        @deprecated(
            deprecated_in="26.3",
            removed_in="27.2",
            replacement="new_api()",
        )
        def old_api() -> int:
            return 42

        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            self.assertEqual(old_api(), 42)

        self.assertEqual(len(caught), 1)
        self.assertEqual(caught[0].category, DeprecationWarning)
        self.assertIn("DeprecationTest.test_decorator_warns_with_lifecycle", str(caught[0].message))
        self.assertIn("deprecated since FreeCAD 26.3", str(caught[0].message))
        self.assertIn("will be removed in FreeCAD 27.2", str(caught[0].message))
        self.assertIn("use new_api() instead", str(caught[0].message))
        self.assertEqual(old_api.__deprecated__, str(caught[0].message))

    def test_required_releases_must_not_be_empty(self) -> None:
        with self.assertRaisesRegex(ValueError, "deprecated_in"):
            deprecated(deprecated_in="", removed_in="27.2")
        with self.assertRaisesRegex(ValueError, "removed_in"):
            deprecated(deprecated_in="26.3", removed_in="")

    def test_releases_must_be_ordered_and_valid(self) -> None:
        with self.assertRaisesRegex(ValueError, "later"):
            deprecated(deprecated_in="27.2", removed_in="26.3")
        with self.assertRaisesRegex(ValueError, "release"):
            deprecated(deprecated_in="next", removed_in="27.2")

    def test_optional_fields_must_be_strings(self) -> None:
        with self.assertRaisesRegex(TypeError, "replacement"):
            deprecated(deprecated_in="26.3", removed_in="27.2", replacement=42)  # type: ignore[arg-type]
        with self.assertRaisesRegex(TypeError, "details"):
            deprecated(deprecated_in="26.3", removed_in="27.2", details=[])  # type: ignore[arg-type]

    def test_message_does_not_duplicate_terminal_punctuation(self) -> None:
        @deprecated(
            deprecated_in="26.3",
            removed_in="27.2",
            details="Legacy compatibility API.",
        )
        def old_api() -> None:
            return None

        self.assertTrue(old_api.__deprecated__.endswith("Legacy compatibility API."))
        self.assertFalse(old_api.__deprecated__.endswith(".."))

    def test_wrapper_frame_can_be_unwrapped(self) -> None:
        @deprecated(deprecated_in="26.3", removed_in="27.2")
        def caller() -> object:
            frame = inspect.currentframe()
            return _unwrap_deprecated_frame(frame.f_back if frame else None)

        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            frame = caller()
        self.assertIsNotNone(frame)
        self.assertEqual(frame.f_code.co_name, "test_wrapper_frame_can_be_unwrapped")


if __name__ == "__main__":
    unittest.main()
