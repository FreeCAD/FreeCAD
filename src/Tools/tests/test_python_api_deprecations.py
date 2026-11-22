# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import tempfile
import textwrap
import unittest

from src.Tools.python_api.cli import main
from src.Tools.python_api.deprecations import manifest, scan_repository


class PythonApiDeprecationsTest(unittest.TestCase):
    def _write(self, root: Path, relative: str, source: str) -> None:
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(textwrap.dedent(source).lstrip(), encoding="utf-8")

    def _fixture(self, root: Path) -> None:
        self._write(
            root,
            "src/App/Example.py",
            """
            from freecad.deprecation import deprecated

            @deprecated(
                deprecated_in="26.3",
                removed_in="27.2",
                replacement="new_api()",
            )
            def old_api(): ...

            @deprecated(deprecated_in="26.3", removed_in="27.2")
            def older_api():
                \"\"\"Deprecated -- use old_api instead.\"\"\"
                ...
            """,
        )
        self._write(
            root,
            "src/Mod/Part/App/Thing.pyi",
            """
            from Base.Metadata import deprecated, deprecated_attributes, export

            @export(PythonName="Part.Thing")
            @deprecated_attributes(
                old_attr={
                    "deprecated_in": "26.3",
                    "removed_in": "27.2",
                    "replacement": "new_attr",
                },
            )
            @deprecated(
                deprecated_in="26.3",
                removed_in="28.1",
                replacement="BetterThing",
            )
            class ThingSpec:
                old_attr: int

                @deprecated(
                    deprecated_in="26.3",
                    removed_in="27.2",
                    replacement="new_method()",
                )
                def old_method(self) -> None: ...
            """,
        )
        self._write(
            root,
            "src/Gui/FreeCADGui.module.pyi",
            """
            from Base.Metadata import deprecated

            @deprecated(deprecated_in="26.3", removed_in="27.2", replacement="hideObject")
            def hide() -> None: ...
            """,
        )

    def test_scans_python_bindings_modules_and_attributes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._fixture(root)
            result = scan_repository(root)

        records = {record.symbol: record for record in result.records}
        self.assertEqual(
            set(records),
            {
                "Example.old_api",
                "Example.older_api",
                "FreeCADGui.hide",
                "Part.Thing.old_attr",
                "Part.Thing.old_method",
                "Part.Thing",
            },
        )
        self.assertEqual(records["Example.old_api"].removed_in, "27.2")
        self.assertEqual(records["Part.Thing.old_attr"].kind, "attribute")
        self.assertEqual(records["Part.Thing"].kind, "class")
        self.assertEqual(records["Part.Thing.old_method"].replacement, "new_method()")
        self.assertFalse(any(item.severity == "error" for item in result.diagnostics))

    def test_reports_invalid_lifecycle_metadata(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write(
                root,
                "src/App/Invalid.py",
                """
                from freecad.deprecation import deprecated

                @deprecated(
                    deprecated_in="27.2",
                    removed_in="26.3",
                    deadline="28.1",
                )
                def invalid(): ...

                @deprecated(deprecated_in="27.2", removed_in="26.3")
                def backwards(): ...
                """,
            )
            result = scan_repository(root)

        messages = [item.message for item in result.diagnostics]
        self.assertIn("unknown deprecation field 'deadline'", messages)
        self.assertIn("removed_in must be later than deprecated_in", messages)

    def test_rejects_legacy_deprecation_metadata(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write(
                root,
                "src/App/Legacy.py",
                """
                from typing_extensions import deprecated

                @deprecated("Use replacement instead.")
                def old_function(): ...

                def old_docstring():
                    \"\"\"Deprecated -- use replacement instead.\"\"\"
                    ...
                """,
            )
            result = scan_repository(root)

        messages = [item.message for item in result.diagnostics]
        self.assertIn(
            "deprecated() requires structured keyword-only lifecycle metadata",
            messages,
        )
        self.assertIn(
            "Legacy.old_docstring uses docstring-only deprecation metadata",
            messages,
        )

    def test_matching_overload_metadata_collapses_to_one_record(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write(
                root,
                "src/App/Example.module.pyi",
                """
                from typing import overload
                from Base.Metadata import deprecated

                @overload
                @deprecated(deprecated_in="26.3", removed_in="27.2")
                def old_api(value: int) -> int: ...

                @overload
                @deprecated(deprecated_in="26.3", removed_in="27.2")
                def old_api(value: str) -> str: ...
                """,
            )
            result = scan_repository(root)

        self.assertEqual(len(result.records), 1)
        self.assertEqual(result.records[0].symbol, "Example.old_api")

    def test_workbench_module_stubs_use_public_module_names(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for relative_path in (
                "src/Mod/CAM/App/PathApp.module.pyi",
                "src/Mod/Part/App/Part.module.pyi",
            ):
                self._write(
                    root,
                    relative_path,
                    """
                    from Base.Metadata import deprecated

                    @deprecated(deprecated_in="26.3", removed_in="27.2")
                    def old_api() -> None: ...
                    """,
                )
            result = scan_repository(root)

        self.assertEqual(
            {record.symbol for record in result.records},
            {"Part.old_api", "PathApp.old_api"},
        )

    def test_manifest_is_deterministic(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._fixture(root)
            first_manifest = manifest(scan_repository(root))
            first = json.dumps(first_manifest, sort_keys=True)
            second = json.dumps(manifest(scan_repository(root)), sort_keys=True)

        self.assertEqual(first, second)
        self.assertNotIn("metadata_kind", first_manifest["deprecations"][0])
        self.assertNotIn("message", first_manifest["deprecations"][0])

    def test_manifest_creates_parent_directories(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._fixture(root)
            output = root / "reports" / "deprecations.json"
            exit_code = main(["--root", str(root), "manifest", "--output", str(output)])

            self.assertEqual(exit_code, 0)
            self.assertTrue(output.is_file())

    def test_list_filters_by_removal_release(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._fixture(root)
            output = io.StringIO()
            with redirect_stdout(output):
                exit_code = main(["--root", str(root), "list", "--remove-by", "27.1"])
            self.assertEqual(exit_code, 0)
            self.assertIn("No matching deprecations.", output.getvalue())

            output = io.StringIO()
            with redirect_stdout(output):
                exit_code = main(["--root", str(root), "list", "--remove-by", "27.2"])

        self.assertEqual(exit_code, 0)
        self.assertIn("Example.old_api", output.getvalue())
        self.assertIn("FreeCADGui.hide", output.getvalue())

    def test_repository_scan_has_no_errors(self) -> None:
        root = Path(__file__).resolve().parents[3]
        result = scan_repository(root)
        errors = [item for item in result.diagnostics if item.severity == "error"]
        self.assertEqual(errors, [])
        self.assertTrue(
            any(record.symbol == "DraftVecUtils.precision" for record in result.records)
        )
        self.assertTrue(any(record.symbol == "Part.Face.Wire" for record in result.records))
        expected_runtime_deprecations = {
            "DraftUtils.readDXF",
            "FreeCADGui._MDIView.message",
            "Part.insert",
            "Part.open",
            "PathApp.fromShape",
        }
        self.assertTrue(expected_runtime_deprecations.issubset({r.symbol for r in result.records}))


if __name__ == "__main__":
    unittest.main()
