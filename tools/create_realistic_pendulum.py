"""Create a simple realistic pendulum FreeCAD document.

Run from FreeCAD's Python console, for example:

    exec(open(r"tools/create_realistic_pendulum.py").read())

The script creates ``data/examples/RealisticPendulum.FCStd`` by default.
Pass an output path as the first argument to write somewhere else.
"""

from __future__ import annotations

import os
import shutil
import sys
import zipfile
from xml.sax.saxutils import escape

import FreeCAD
import Part
from FreeCAD import Base


DOC_NAME = "RealisticPendulum"


def compound_all(shapes):
    """Group primitives into one document shape without expensive booleans."""
    return Part.makeCompound(shapes)


def make_ceiling_bracket():
    """Return a ceiling-mounted support bracket for the hinge pin."""
    ceiling_plate = Part.makeBox(130, 90, 12, Base.Vector(-65, -45, 230))
    left_cheek = Part.makeBox(36, 12, 66, Base.Vector(-18, -36, 164))
    right_cheek = Part.makeBox(36, 12, 66, Base.Vector(-18, 24, 164))

    pin_clearance = Part.makeCylinder(
        10.5,
        90,
        Base.Vector(0, -45, 192),
        Base.Vector(0, 1, 0),
    )
    left_cheek = left_cheek.cut(pin_clearance)
    right_cheek = right_cheek.cut(pin_clearance)

    return compound_all([ceiling_plate, left_cheek, right_cheek])


def make_pin():
    """Return the horizontal hinge pin shape."""
    pin = Part.makeCylinder(
        8,
        88,
        Base.Vector(0, -44, 192),
        Base.Vector(0, 1, 0),
    )
    left_cap = Part.makeCylinder(
        11,
        4,
        Base.Vector(0, -48, 192),
        Base.Vector(0, 1, 0),
    )
    right_cap = Part.makeCylinder(
        11,
        4,
        Base.Vector(0, 44, 192),
        Base.Vector(0, 1, 0),
    )

    return compound_all([pin, left_cap, right_cap])


def make_pendulum():
    """Return one long pendulum shape with a through-hole for the pin."""
    hub = Part.makeCylinder(
        20,
        10,
        Base.Vector(0, -5, 192),
        Base.Vector(0, 1, 0),
    )
    pin_hole = Part.makeCylinder(
        9.4,
        14,
        Base.Vector(0, -7, 192),
        Base.Vector(0, 1, 0),
    )
    hub = hub.cut(pin_hole)

    rod = Part.makeBox(10, 6, 132, Base.Vector(-5, -3, 48))
    bob = Part.makeCylinder(
        28,
        10,
        Base.Vector(0, -5, 48),
        Base.Vector(0, 1, 0),
    )

    return compound_all([hub, rod, bob])


def add_part(doc, name, shape, color):
    obj = doc.addObject("Part::Feature", name)
    obj.Shape = shape
    if obj.ViewObject is not None:
        obj.ViewObject.ShapeColor = color
        obj.ViewObject.DisplayMode = "Shaded"
    return obj


def default_output_path():
    script_path = globals().get("__file__", os.path.join(os.getcwd(), "tools", "create_realistic_pendulum.py"))
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(script_path)))
    return os.path.join(repo_root, "data", "examples", "RealisticPendulum.FCStd")


def requested_output_path():
    for arg in sys.argv[1:]:
        if arg.startswith("-"):
            continue
        if arg.lower().endswith(".fcstd"):
            return os.path.abspath(arg)
    return default_output_path()


def set_isometric_fit_view():
    """Store an isometric fit-all view when the GUI is available."""
    try:
        import FreeCADGui
    except ImportError:
        return False

    gui_doc = None
    if hasattr(FreeCADGui, "getDocument"):
        gui_doc = FreeCADGui.getDocument(DOC_NAME)
    if gui_doc is None and hasattr(FreeCADGui, "ActiveDocument"):
        gui_doc = FreeCADGui.ActiveDocument
    if gui_doc is None:
        return False

    view = gui_doc.ActiveView
    if view is None:
        return False

    view.viewIsometric()
    view.fitAll()
    return True


def make_camera_settings(doc):
    bbox = None
    for obj in doc.Objects:
        shape = getattr(obj, "Shape", None)
        if shape is None or shape.isNull():
            continue
        if bbox is None:
            bbox = shape.BoundBox
        else:
            bbox.add(shape.BoundBox)

    if bbox is None:
        return ""

    center = bbox.Center
    diagonal = bbox.DiagonalLength
    distance = max(diagonal * 1.7, 250.0)
    position = Base.Vector(center.x + distance, center.y - distance, center.z + distance)
    height = max(diagonal * 1.2, bbox.ZLength * 1.25, 250.0)

    # Matches Gui::Camera::isometric(), converted from quaternion to axis-angle.
    axis = (0.74290526076291, 0.307721760535265, 0.594472192130913)
    angle = 1.21711684054496

    return (
        "OrthographicCamera {\n"
        "  viewportMapping ADJUST_CAMERA\n"
        f"  position {position.x:.6f} {position.y:.6f} {position.z:.6f}\n"
        f"  orientation {axis[0]:.14f} {axis[1]:.14f} {axis[2]:.14f}  {angle:.14f}\n"
        f"  nearDistance {-distance:.6f}\n"
        f"  farDistance {distance * 3.0:.6f}\n"
        "  aspectRatio 1\n"
        f"  focalDistance {distance:.6f}\n"
        f"  height {height:.6f}\n\n"
        "}\n"
    )


def write_gui_document(output_path, camera_settings):
    if not camera_settings:
        return

    escaped_camera = escape(camera_settings, {'"': "&quot;", "\n": "&#10;"})
    gui_document = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        "<!--\n"
        " FreeCAD Document, see https://www.freecad.org for more information\n"
        "-->\n"
        '<Document SchemaVersion="1" HasExpansion="1">\n'
        "    <Expand />\n"
        '    <ViewProviderData Count="0">\n'
        "    </ViewProviderData>\n"
        f'    <Camera settings="{escaped_camera}"/>\n'
        "</Document>\n"
    )

    temp_path = output_path + ".tmp"
    with zipfile.ZipFile(output_path, "r") as source:
        with zipfile.ZipFile(temp_path, "w", compression=zipfile.ZIP_DEFLATED) as target:
            for item in source.infolist():
                if item.filename == "GuiDocument.xml":
                    continue
                target.writestr(item, source.read(item.filename))
            target.writestr("GuiDocument.xml", gui_document)

    shutil.move(temp_path, output_path)


def main():
    output_path = requested_output_path()

    try:
        existing = FreeCAD.getDocument(DOC_NAME)
    except NameError:
        existing = None

    if existing is not None:
        FreeCAD.closeDocument(DOC_NAME)

    doc = FreeCAD.newDocument(DOC_NAME)

    bracket = add_part(doc, "CeilingBracket", make_ceiling_bracket(), (0.38, 0.38, 0.36))
    pin = add_part(doc, "Pin", make_pin(), (0.85, 0.82, 0.72))
    pendulum = add_part(doc, "LongPendulum", make_pendulum(), (0.60, 0.16, 0.10))

    for obj in (bracket, pin, pendulum):
        if obj.ViewObject is not None:
            obj.ViewObject.LineColor = (0.08, 0.08, 0.08)

    doc.recompute()
    set_isometric_fit_view()
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    doc.saveAs(output_path)
    write_gui_document(output_path, make_camera_settings(doc))
    print(f"Created {output_path}")


if __name__ == "__main__":
    main()
