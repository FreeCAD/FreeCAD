# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from App.DocumentObject import DocumentObject
from Part.App.Part2DObject import Part2DObject
from typing import Final

@export(
    Include="Mod/Text/App/ShapeText.h",
    FatherInclude="Mod/Part/App/Part2DObjectPy.h",
    Constructor=True,
    Delete=True,
)
class ShapeText(Part2DObject):
    """
    Represents a text object

    Author: Martin Rodriguez Reboredo yakoyoku@gmail.com
    Licence: LGPL
    """

    String: str = ""
    """Text string to be drawn"""

    FontSource: str = "Name"
    """
    Whether the font comes from its family name, a file path or a document
    object (only font objects are accepted)
    """

    FontName: str = ""
    """Font family name"""

    FontFile: str = ""
    """Font file location in the filesystem"""

    FontObject: DocumentObject = None
    """Font document object"""

    Size: float = 5.0
    """Text height, more specifically, the vertical bearing of glyphs"""

    Aspect: str = "Normal"
    """Font face aspect, either normal, bold, italic or bold and italic"""

    Justification: str = "BottomLeft"
    """Horizontal and vertical alignment"""

    HeightReference: str = "CapHeight"
    """
    Height reference, either the height of the glyph caps or the text by itself
    """

    Direction: str = "LeftToRight"
    """Text direction from-to"""

    KeepLeftMargin: bool = False
    """Keep left margin, i.e. whitespace used as indentation"""

    ScaleToSize: bool = False
    """Scale text to size instead of its glyphs' vertical bearing"""
