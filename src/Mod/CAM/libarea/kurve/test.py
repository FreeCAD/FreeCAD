# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileNotice: Part of the FreeCAD project.

import area

p = area.Point(0, 0)

m = area.Matrix([1, 0, 0, 12, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1])

p.Transform(m)

print(p.x, p.y)
