# pyright: strict

"""Tests for structured Python API merge diagnostics."""

from pathlib import Path
import tempfile
import unittest

from stubgen.diagnostics import generated_output_diagnostics
from stubgen.python_api.extract import extract_curated_api_model_with_diagnostics
from stubgen.python_api.model import ApiAlias, ApiModel, ApiModule


class MergeDiagnosticsTest(unittest.TestCase):
    def test_conflicting_module_pieces_are_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_dir = root / "src"
            (source_dir / "one").mkdir(parents=True)
            (source_dir / "two").mkdir()
            (source_dir / "one" / "FreeCAD.module.pyi").write_text(
                "def ping() -> int: ...\n",
                encoding="utf-8",
            )
            (source_dir / "two" / "FreeCAD.module.pyi").write_text(
                "def ping() -> str: ...\n",
                encoding="utf-8",
            )

            _, diagnostics = extract_curated_api_model_with_diagnostics(root, source_dir)

        self.assertEqual(len(diagnostics), 1)
        self.assertEqual(diagnostics[0].code, "conflicting-definition")
        self.assertEqual(diagnostics[0].symbol, "FreeCAD.ping")

    def test_missing_curated_declaration_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "Demo.module.pyi").write_text(
                "def ping() -> int: ...\n",
                encoding="utf-8",
            )
            model, extraction_diagnostics = extract_curated_api_model_with_diagnostics(
                root,
                source_dir,
            )
            output_dir = root / "stubs"
            (output_dir / "Demo").mkdir(parents=True)
            (output_dir / "Demo" / "__init__.pyi").write_text(
                "from __future__ import annotations\n",
                encoding="utf-8",
            )
            diagnostics = generated_output_diagnostics(
                output_dir,
                model,
                {"Demo"},
            )

        self.assertEqual(extraction_diagnostics, ())
        self.assertEqual(len(diagnostics), 1)
        self.assertEqual(diagnostics[0].code, "missing-declaration-output")
        self.assertEqual(diagnostics[0].symbol, "Demo.ping")

    def test_unresolved_alias_is_reported(self) -> None:
        model = ApiModel(
            modules=(
                ApiModule(
                    name="Demo",
                    aliases=(
                        ApiAlias(
                            public_path="Demo.PublicName",
                            target_path="Demo.MissingName",
                        ),
                    ),
                ),
            ),
        )

        with tempfile.TemporaryDirectory() as temporary_directory:
            output_dir = Path(temporary_directory)
            (output_dir / "Demo").mkdir()
            (output_dir / "Demo" / "__init__.pyi").write_text(
                "PublicName = MissingName\n",
                encoding="utf-8",
            )
            diagnostics = generated_output_diagnostics(output_dir, model, {"Demo"})

        self.assertEqual(len(diagnostics), 1)
        self.assertEqual(diagnostics[0].code, "unresolved-alias")


if __name__ == "__main__":
    unittest.main()
