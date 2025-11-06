<!--
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2025 Furgo
SPDX-FileNotice: Part of the FreeCAD project.
-->

# Import module test fixtures

Test data files used by `importtests`, such as CAD files for the importers.

These files are copied into the build tree and installed under
`Mod/Import/importtests/fixtures` so tests can reference them at runtime.
Add each new file to `importtests_FIXTURES` in `src/Mod/Import/CMakeLists.txt`,
and get its path in a test with:

```python
from importtests.fixtures import fixture_path

path = fixture_path("example.dxf")
```
