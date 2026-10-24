# SPDX-License-Identifier: LGPL-2.1-or-later

from pathlib import Path
import tempfile
import unittest

from stubgen.api_extract import extract_curated_api_model_with_diagnostics


class CuratedApiExtractionTests(unittest.TestCase):
    def test_module_stub_declarations_become_public_model_entries(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "Demo.module.pyi").write_text(
                """\
class Widget:
    \"\"\"A documented widget.\"\"\"

    def refresh(self, value: int, /) -> None: ...

def ping(message: str) -> bool: ...
""",
                encoding="utf-8",
            )

            model, diagnostics = extract_curated_api_model_with_diagnostics(
                root,
                source_dir,
            )

        self.assertEqual(diagnostics, ())
        self.assertEqual(len(model.modules), 1)
        module = model.modules[0]
        self.assertEqual(module.name, "Demo")
        self.assertEqual(module.functions[0].name, "ping")
        self.assertEqual(module.classes[0].qualified_name, "Demo.Widget")
        self.assertEqual(module.classes[0].methods[0].name, "refresh")

    def test_conflicting_same_precedence_declarations_are_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_dir = root / "src"
            (source_dir / "one").mkdir(parents=True)
            (source_dir / "two").mkdir()
            for directory, annotation in (("one", "int"), ("two", "str")):
                (source_dir / directory / "Demo.module.pyi").write_text(
                    f"def ping() -> {annotation}: ...\n",
                    encoding="utf-8",
                )

            _, diagnostics = extract_curated_api_model_with_diagnostics(root, source_dir)

        self.assertEqual(len(diagnostics), 1)
        self.assertEqual(diagnostics[0].code, "conflicting-definition")
        self.assertEqual(diagnostics[0].symbol, "Demo.ping")

    def test_overlay_public_declarations_enter_the_same_model(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_dir = root / "src"
            source_dir.mkdir()
            overlay_dir = root / "overlays" / "PySide"
            overlay_dir.mkdir(parents=True)
            (overlay_dir / "QtCore.pyi").write_text(
                "def QT_TRANSLATE_NOOP(context: str, text: str, /) -> str: ...\n",
                encoding="utf-8",
            )

            model, diagnostics = extract_curated_api_model_with_diagnostics(
                root,
                source_dir,
                overlay_dir=overlay_dir.parent,
            )

        self.assertEqual(diagnostics, ())
        self.assertEqual(model.modules[0].name, "PySide.QtCore")
        self.assertEqual(model.modules[0].origin.value, "overlay")
        self.assertEqual(model.modules[0].functions[0].name, "QT_TRANSLATE_NOOP")

    def test_source_type_names_are_normalized_in_module_context(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "Part.module.pyi").write_text(
                """\
class Shape:
    pass

class Feature:
    Shape: "Part.Shape" = ...

    def transform(self, matrix: MatrixPy, /) -> "Part.Shape": ...
""",
                encoding="utf-8",
            )

            model, diagnostics = extract_curated_api_model_with_diagnostics(
                root,
                source_dir,
            )

        self.assertEqual(diagnostics, ())
        feature = next(klass for klass in model.modules[0].classes if klass.name == "Feature")
        self.assertEqual(feature.attributes[0].annotation, "'Shape'")
        signature = feature.methods[0].signatures[0]
        self.assertEqual(signature.parameters[1].annotation, "'FreeCAD.Base.Matrix'")
        self.assertEqual(signature.return_annotation, "'Shape'")

    def test_binding_decorators_are_removed_before_model_rendering(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "Demo.module.pyi").write_text(
                "@export\n@final\nclass Widget:\n    pass\n",
                encoding="utf-8",
            )

            model, diagnostics = extract_curated_api_model_with_diagnostics(root, source_dir)

        self.assertEqual(diagnostics, ())
        self.assertEqual(model.modules[0].classes[0].decorators, ("final",))


if __name__ == "__main__":
    unittest.main()
