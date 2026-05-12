"""Tests for explicit and legacy deprecation metadata in Python binding stubs."""

from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import textwrap
import unittest


BINDINGS_ROOT = Path(__file__).resolve().parents[1]
if str(BINDINGS_ROOT) not in sys.path:
    sys.path.insert(0, str(BINDINGS_ROOT))


from model.generateModel_Python import parse  # noqa: E402
from templates.templateClassPyExport import TemplateClassPyExport  # noqa: E402


class DeprecationMetadataTest(unittest.TestCase):
    def _write_stub(self, tempdir: str, relative_path: str, contents: str) -> Path:
        path = Path(tempdir, relative_path)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(textwrap.dedent(contents).strip() + "\n", encoding="utf-8")
        return path

    def _parse_export(self, relative_path: str, contents: str):
        with tempfile.TemporaryDirectory() as tempdir:
            stub_path = self._write_stub(tempdir, relative_path, contents)
            model = parse(str(stub_path))
            self.assertEqual(len(model.PythonExport), 1)
            return model.PythonExport[0]

    def _generate_cpp(self, relative_path: str, contents: str) -> str:
        with tempfile.TemporaryDirectory() as tempdir:
            stub_path = self._write_stub(tempdir, relative_path, contents)
            output_dir = Path(tempdir, "generated")
            output_dir.mkdir()

            model = parse(str(stub_path))
            export = model.PythonExport[0]

            template = TemplateClassPyExport()
            template.outputDir = str(output_dir) + "/"
            template.inputDir = str(stub_path.parent) + "/"
            template.export = export
            template.is_python = True
            template.Generate()

            return (output_dir / f"{export.Name}.cpp").read_text(encoding="utf-8")

    def test_parses_method_deprecated_decorator(self) -> None:
        export = self._parse_export(
            "src/Base/TestThing.pyi",
            """
            from __future__ import annotations

            from Metadata import export
            from PyObjectBase import PyObjectBase
            from typing_extensions import deprecated

            @export()
            class TestThing(PyObjectBase):
                @deprecated("Use newName instead.")
                def oldName(self) -> None:
                    \"\"\"Legacy alias for newName.\"\"\"
                    ...
            """,
        )

        methods = {method.Name: method for method in export.Methode}
        self.assertEqual(methods["oldName"].Deprecated, "Use newName instead.")

    def test_explicit_deprecated_attributes_override_docstring_fallback(self) -> None:
        export = self._parse_export(
            "src/Base/TestThing.pyi",
            """
            from __future__ import annotations

            from Metadata import export, deprecated_attributes
            from PyObjectBase import PyObjectBase
            from typing import Final

            @export()
            @deprecated_attributes(oldAttr="Use newAttr instead.")
            class TestThing(PyObjectBase):
                oldAttr: Final[object] = ...
                \"\"\"Deprecated -- old docstring fallback.\"\"\"
            """,
        )

        attributes = {attribute.Name: attribute for attribute in export.Attribute}
        self.assertEqual(attributes["oldAttr"].Deprecated, "Use newAttr instead.")

    def test_parses_legacy_deprecation_docstrings(self) -> None:
        export = self._parse_export(
            "src/Base/TestThing.pyi",
            """
            from __future__ import annotations

            from Metadata import export
            from PyObjectBase import PyObjectBase
            from typing import Final

            @export()
            class TestThing(PyObjectBase):
                legacyAttr: Final[object] = ...
                \"\"\"Deprecated -- use replacementAttr.\"\"\"

                def legacyMethod(self) -> None:
                    \"\"\"Deprecated: use replacementMethod.\"\"\"
                    ...
            """,
        )

        attributes = {attribute.Name: attribute for attribute in export.Attribute}
        methods = {method.Name: method for method in export.Methode}
        self.assertEqual(attributes["legacyAttr"].Deprecated, "use replacementAttr.")
        self.assertEqual(methods["legacyMethod"].Deprecated, "use replacementMethod.")

    def test_unknown_deprecated_attribute_metadata_raises_clean_error(self) -> None:
        with tempfile.TemporaryDirectory() as tempdir:
            stub_path = self._write_stub(
                tempdir,
                "src/Base/TestThing.pyi",
                """
                from __future__ import annotations

                from Metadata import export, deprecated_attributes
                from PyObjectBase import PyObjectBase
                from typing import Final

                @export()
                @deprecated_attributes(Missing="Use Present instead.")
                class TestThing(PyObjectBase):
                    Present: Final[object] = ...
                    \"\"\"A valid attribute.\"\"\"
                """,
            )

            with self.assertRaisesRegex(
                Exception,
                "Unknown deprecated attribute metadata for class 'TestThing': Missing",
            ):
                parse(str(stub_path))

    def test_generation_emits_runtime_warnings_for_methods_and_attributes(self) -> None:
        generated_cpp = self._generate_cpp(
            "src/Base/TestThing.pyi",
            """
            from __future__ import annotations

            from Metadata import export, deprecated_attributes
            from PyObjectBase import PyObjectBase
            from typing import Final
            from typing_extensions import deprecated

            @export()
            @deprecated_attributes(oldAttr="Use newAttr instead.")
            class TestThing(PyObjectBase):
                oldAttr: Final[object] = ...
                \"\"\"Legacy alias for newAttr.\"\"\"

                @deprecated("Use newMethod instead.")
                def oldMethod(self) -> None:
                    \"\"\"Legacy alias for newMethod.\"\"\"
                    ...
            """,
        )

        self.assertIn(
            "#include <Base/Interpreter.h>",
            generated_cpp,
        )
        self.assertIn(
            'Base::warnDeprecatedPythonApi("Method", "Base.TestThing.oldMethod", "Use newMethod instead.")',
            generated_cpp,
        )
        self.assertIn(
            'Base::warnDeprecatedPythonApi("Attribute", "Base.TestThing.oldAttr", "Use newAttr instead.")',
            generated_cpp,
        )

    def test_generation_skips_warning_helper_without_deprecations(self) -> None:
        generated_cpp = self._generate_cpp(
            "src/Base/TestThing.pyi",
            """
            from __future__ import annotations

            from Metadata import export
            from PyObjectBase import PyObjectBase
            from typing import Final

            @export()
            class TestThing(PyObjectBase):
                activeAttr: Final[object] = ...
                \"\"\"Normal attribute.\"\"\"

                def activeMethod(self) -> None:
                    \"\"\"Normal method.\"\"\"
                    ...
            """,
        )

        self.assertNotIn("Base::warnDeprecatedPythonApi(", generated_cpp)
        self.assertNotIn("#include <Base/Interpreter.h>", generated_cpp)


if __name__ == "__main__":
    unittest.main()
