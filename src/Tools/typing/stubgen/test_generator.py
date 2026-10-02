# SPDX-License-Identifier: LGPL-2.1-or-later

from pathlib import Path
import tempfile
import unittest

from python_api_model.diagnostics import MergeDiagnostics
from stubgen.generator import write_outputs


class StubGenerationTests(unittest.TestCase):
    def test_generation_stops_before_replacing_output_on_merge_error(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_dir = root / "src"
            first = source_dir / "one"
            second = source_dir / "two"
            first.mkdir(parents=True)
            second.mkdir()
            (first / "Demo.module.pyi").write_text(
                "def ping() -> int: ...\n",
                encoding="utf-8",
            )
            (second / "Demo.module.pyi").write_text(
                "def ping() -> str: ...\n",
                encoding="utf-8",
            )

            output_dir = root / "generated"
            stale_output = output_dir / "stubs" / "Demo.pyi"
            stale_output.parent.mkdir(parents=True)
            stale_output.write_text("stale\n", encoding="utf-8")

            result = write_outputs(
                output_dir,
                root,
                source_dir,
                [],
                [],
                {},
                {},
            )

            self.assertEqual(len(result.errors), 1)
            self.assertEqual(result.errors[0].code, "conflicting-definition")
            location = result.errors[0].location
            assert location is not None
            self.assertEqual(location.path, "src/two/Demo.module.pyi")
            self.assertEqual(location.line, 1)
            self.assertIn(
                "[src/two/Demo.module.pyi:1]",
                MergeDiagnostics(result.diagnostics).render(),
            )
            self.assertEqual(stale_output.read_text(encoding="utf-8"), "stale\n")


if __name__ == "__main__":
    unittest.main()
