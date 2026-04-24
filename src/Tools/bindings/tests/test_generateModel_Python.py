# SPDX-License-Identifier: LGPL-2.1-or-later

import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

BINDINGS_DIR = Path(__file__).resolve().parents[1]
SRC_DIR = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(BINDINGS_DIR))

from model.generateModel_Python import parse_python_code


class GenerateModelPythonTests(unittest.TestCase):
    def test_overload_only_constructor_docs_are_merged_into_class_doc(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from typing import overload

            from Base.Metadata import export
            from Base.PyObjectBase import PyObjectBase


            @export(Constructor=True)
            class Example(PyObjectBase):
                \"\"\"Example class doc.\"\"\"

                @overload
                def __init__(self) -> None: ...

                @overload
                def __init__(self, value: int) -> None:
                    \"\"\"
                    Create example objects.
                    \"\"\"
                    ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR) as temp_dir:
            path = Path(temp_dir) / "Example.pyi"
            path.write_text(source, encoding="utf-8")
            model = parse_python_code(str(path))

        export = model.PythonExport[0]
        self.assertEqual([method.Name for method in export.Methode], [])
        self.assertIn("Example class doc.", export.Documentation.UserDocu)
        self.assertIn("Example()", export.Documentation.UserDocu)
        self.assertIn("Example(value)", export.Documentation.UserDocu)
        self.assertIn("Example(value: int) -> None", export.Documentation.UserDocu)
        self.assertIn("Create example objects.", export.Documentation.UserDocu)

    def test_existing_class_constructor_docs_are_not_duplicated(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from typing import overload

            from Base.Metadata import export
            from Base.PyObjectBase import PyObjectBase


            @export(Constructor=True)
            class Example(PyObjectBase):
                \"\"\"
                Example class doc.

                The following constructors are supported:

                Example()
                Empty constructor.

                Example(value)
                Value constructor.
                \"\"\"

                @overload
                def __init__(self) -> None: ...

                @overload
                def __init__(self, value: int) -> None: ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR) as temp_dir:
            path = Path(temp_dir) / "Example.pyi"
            path.write_text(source, encoding="utf-8")
            model = parse_python_code(str(path))

        export = model.PythonExport[0]
        user_doc = export.Documentation.UserDocu
        self.assertIn("The following constructors are supported:", user_doc)
        self.assertEqual(user_doc.count("Example()"), 1)
        self.assertEqual(user_doc.count("Example(value)"), 1)
        self.assertNotIn("--\n", user_doc)


if __name__ == "__main__":
    unittest.main()
