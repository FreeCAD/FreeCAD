"""Contract tests for Python API documentation orchestration."""

from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from stubgen.python_api.pipeline import PythonDocsOptions, generate_python_docs

REPOSITORY_ROOT = Path(__file__).resolve().parents[5]


class PythonApiPipelineTests(unittest.TestCase):
    def test_pipeline_writes_the_existing_python_api_surface(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "docs"
            sidebar_path = Path(directory) / "sidebar.ts"
            result = generate_python_docs(
                PythonDocsOptions(
                    root=REPOSITORY_ROOT,
                    source_dir=REPOSITORY_ROOT / "src",
                    out_dir=output,
                    source_base_url=None,
                    sidebar_out=sidebar_path,
                )
            )

            self.assertGreater(result.page_count, 0)
            self.assertTrue((output / "python-api/index.mdx").exists())
            freecad_page = output / "python-api/freecad/index.mdx"
            self.assertTrue(freecad_page.exists())
            self.assertIn("`FreeCAD.Axis` -> `FreeCAD.Base.Axis`", freecad_page.read_text())
            self.assertTrue(
                (output / "python-api/freecad/types/ApplicationDirectories.mdx").exists()
            )
            self.assertTrue(sidebar_path.exists())


if __name__ == "__main__":
    unittest.main()
