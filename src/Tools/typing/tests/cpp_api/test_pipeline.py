"""Contract tests for C++ API pipeline orchestration."""

from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest

from stubgen.cpp_api.doxygen import render_doxygen_config
from stubgen.cpp_api.pipeline import CppDocsOptions, generate_cpp_docs

FIXTURE_DIR = Path(__file__).parent / "fixtures" / "basic"


class CppApiPipelineTests(unittest.TestCase):
    def test_pipeline_generates_docs_from_existing_xml(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            docs_dir = root / "docs"
            sidebar_path = root / "sidebar.ts"
            result = generate_cpp_docs(
                CppDocsOptions(
                    root=root,
                    out_dir=docs_dir,
                    doxygen_out_dir=root / "doxygen",
                    doxygen_xml_dir=FIXTURE_DIR,
                    run_doxygen=False,
                    source_base_url=None,
                    sidebar_out=sidebar_path,
                )
            )

            self.assertEqual(result.page_count, 3)
            self.assertEqual(result.xml_dir, FIXTURE_DIR)
            self.assertTrue((docs_dir / "cpp-api/app/index.mdx").exists())
            self.assertTrue((docs_dir / "cpp-api/app/types/Widget.mdx").exists())
            self.assertTrue(sidebar_path.exists())
            self.assertEqual(result.manifest_path, docs_dir / "cpp-api-manifest.json")
            self.assertEqual(
                json.loads(result.manifest_path.read_text()),
                {
                    "classes": 1,
                    "enums": 0,
                    "functions": 3,
                    "generator": "cpp-api",
                    "namespaces": 1,
                    "pages": 3,
                    "schemaVersion": 1,
                },
            )

    def test_doxygen_config_is_standalone(self) -> None:
        config = render_doxygen_config(Path("/repo"), Path("/repo/build/cpp-api"))

        self.assertIn("PROJECT_NAME = FreeCAD C++ API", config)
        self.assertIn("GENERATE_XML = YES", config)
        self.assertIn("CREATE_SUBDIRS = NO", config)
        self.assertNotIn("BuildDevDoc.cfg", config)


if __name__ == "__main__":
    unittest.main()
