# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import ast
from pathlib import Path
import unittest


class PythonApiArchitectureTests(unittest.TestCase):
    def test_shared_package_does_not_import_stubgen(self) -> None:
        package_dir = Path(__file__).parent
        for path in sorted(package_dir.glob("*.py")):
            if path.name.startswith("test_"):
                continue
            tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
            for node in ast.walk(tree):
                if isinstance(node, ast.Import):
                    modules = [alias.name for alias in node.names]
                elif isinstance(node, ast.ImportFrom):
                    modules = [node.module or ""]
                else:
                    continue
                self.assertFalse(
                    any(module == "stubgen" or module.startswith("stubgen.") for module in modules),
                    f"{path.name} imports StubGen directly",
                )


if __name__ == "__main__":
    unittest.main()
