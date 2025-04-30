import pathlib
from typing import Mapping, Optional
import xml.etree.ElementTree as ET
import FreeCAD


_svg_ns = {"s": "http://www.w3.org/2000/svg"}


def file_is_newer(reference: pathlib.Path, file: pathlib.Path):
    return reference.stat().st_mtime > file.stat().st_mtime


def create_thumbnail(filepath: pathlib.Path, w: int = 200, h: int = 200) -> Optional[pathlib.Path]:
    if not FreeCAD.GuiUp:
        return None

    try:
        import FreeCADGui
    except ImportError:
        raise RuntimeError("Error: Could not load UI - is it up?")

    doc = FreeCAD.openDocument(str(filepath))
    view = FreeCADGui.activeDocument().ActiveView
    out_filepath = filepath.with_suffix(".png")
    if not view:
        print("No view active, cannot make thumbnail for {}".format(filepath))
        return

    view.viewFront()
    view.fitAll()
    view.setAxisCross(False)
    view.saveImage(str(out_filepath), w, h, "Transparent")

    FreeCAD.closeDocument(doc.Name)
    return out_filepath


def get_abbreviations_from_svg(svg: bytes) -> Mapping[str, str]:
    try:
        tree = ET.fromstring(svg)
    except ET.ParseError:
        return {}

    result = {}
    for text_elem in tree.findall(".//s:text", _svg_ns):
        id = text_elem.attrib.get("id", _svg_ns)
        if id is None or not isinstance(id, str):
            continue

        abbr = text_elem.text
        if abbr is not None:
            result[id.lower()] = abbr

        span_elem = text_elem.find(".//s:tspan", _svg_ns)
        if span_elem is None:
            continue
        abbr = span_elem.text
        result[id.lower()] = abbr

    return result


if __name__ == "__main__":
    import sys

    filename = sys.argv[1]
    with open(filename, "b") as fp:
        svg = fp.read()
    print(get_abbreviations_from_svg(svg))
