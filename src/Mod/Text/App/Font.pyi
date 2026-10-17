# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from App.DocumentObject import DocumentObject

@export(
    Include="Mod/Text/App/Font.h",
    Constructor=True,
    Delete=True,
)
class Font(DocumentObject):
    """
    Represents a font object in a document

    Author: Martin Rodriguez Reboredo yakoyoku@gmail.com
    Licence: LGPL
    """

    Name: str = ""
    """Font family name"""

    Source: str = ""
    """
    Whether the font comes from its family name or a file path either in the
    filesystem or in the document
    """

    File: str = ""
    """Font file location in the filesystem"""

    Included: str = ""
    """Font file location in document"""
