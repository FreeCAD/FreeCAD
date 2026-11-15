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

Record below where each file comes from.

## File sources

`rectangle_lines.dxf`: four lines forming a 10 mm x 5 mm rectangle.
Generated with [ezdxf](https://ezdxf.mozman.at) 1.4.4:

```python
import ezdxf

doc = ezdxf.new("R2000", units=ezdxf.units.MM)
msp = doc.modelspace()
for start, end in [((0, 0), (10, 0)), ((10, 0), (10, 5)), ((10, 5), (0, 5)), ((0, 5), (0, 0))]:
    msp.add_line(start, end, dxfattribs={"layer": "0"})
doc.saveas("rectangle_lines.dxf")
```
