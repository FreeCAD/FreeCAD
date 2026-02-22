# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

from typing import Any

from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Include="Mod/CAM/App/FeaturePathCompound.h",
    TwinPointer="FeatureCompound",
    Namespace="Path",
)
class FeaturePathCompound(DocumentObject):
    """
    This class handles Path Compound features

    Author: Yorik van Havre (yorik@uncreated.net)
    License: LGPL-2.1-or-later
    """

    def addObject(self) -> Any:
        """Add an object to the group"""
        ...

    def removeObject(self) -> Any:
        """Remove an object from the group"""
        ...
