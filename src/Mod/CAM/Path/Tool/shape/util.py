# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
import pathlib
from typing import Optional
import FreeCAD
import tempfile
import os
from .doc import ShapeDocFromBytes


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


def create_thumbnail_from_data(shape_data: bytes, w: int = 200, h: int = 200) -> Optional[bytes]:
    """
    Create a thumbnail icon from shape data bytes using a temporary document.

    Args:
        shape_data (bytes): The raw bytes of the shape file (.FCStd).
        w (int): Width of the thumbnail.
        h (int): Height of the thumbnail.

    Returns:
        Optional[bytes]: PNG image bytes, or None if generation fails.
    """
    if not FreeCAD.GuiUp:
        return None

    try:
        import FreeCADGui
    except ImportError:
        raise RuntimeError("Error: Could not load UI - is it up?")

    temp_png_path = None
    try:
        with ShapeDocFromBytes(shape_data) as doc:
            view = FreeCADGui.activeDocument().ActiveView

            if not view:
                print("No view active, cannot make thumbnail from data")
                return None

            view.viewFront()
            view.fitAll()
            view.setAxisCross(False)

            # Create a temporary file path for the output PNG
            with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as temp_file:
                temp_png_path = pathlib.Path(temp_file.name)

            view.saveImage(str(temp_png_path), w, h, "Transparent")

            # Read the PNG bytes
            with open(temp_png_path, "rb") as f:
                png_bytes = f.read()

            return png_bytes

    except Exception as e:
        print(f"Error creating thumbnail from data: {e}")
        return None

    finally:
        # Clean up temporary PNG file
        if temp_png_path and temp_png_path.exists():
            os.remove(temp_png_path)
