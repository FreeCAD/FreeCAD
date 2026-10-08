# SPDX-License-Identifier: LGPL-2.1-or-later
"""Small native assembly exercising nested placements through STEP roundtrips."""

import FreeCAD as App


def build(params):
    doc = App.newDocument("AssemblyExample")
    root = doc.addObject("App::Part", "Assembly")
    root.Placement = App.Placement(App.Vector(10, 20, 30), App.Rotation(App.Vector(0, 0, 1), 90))
    group = doc.addObject("App::Part", "Bracket")
    group.Placement.Base = App.Vector(4, 0, 0)
    root.addObject(group)
    box = doc.addObject("Part::Box", "Block")
    box.Length, box.Width, box.Height = 2, 3, 4
    group.addObject(box)
    pin = doc.addObject("Part::Cylinder", "Pin")
    pin.Radius, pin.Height = 1, 2
    pin.Placement.Base = App.Vector(0, 10, 0)
    root.addObject(pin)
    root.addProperty("App::PropertyBool", "HeadlessAssembly", "Import")
    root.HeadlessAssembly = True
    return root
