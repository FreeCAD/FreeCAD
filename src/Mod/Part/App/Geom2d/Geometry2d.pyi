# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.PyObjectBase import PyObjectBase

@export(
    Twin="Geometry2d",
    TwinPointer="Geometry2d",
    PythonName="Part.Geom2d.Geometry2d",
    Include="Mod/Part/App/Geometry2d.h",
    Constructor=True,
    Delete=True,
)
class Geometry2d(PyObjectBase):
    """
    The abstract class Geometry for 2D space is the root class of all geometric objects.
    It describes the common behavior of these objects when:
    - applying geometric transformations to objects, and
    - constructing objects by geometric transformation (including copying).

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def mirror(self) -> None:
        """
        Performs the symmetrical transformation of this geometric object.
        """
        ...

    def rotate(self) -> None:
        """
        Rotates this geometric object at angle Ang (in radians) around a point.
        """
        ...

    def scale(self) -> None:
        """
        Applies a scaling transformation on this geometric object with a center and scaling factor.
        """
        ...

    def transform(self) -> None:
        """
        Applies a transformation to this geometric object.
        """
        ...

    def translate(self) -> None:
        """
        Translates this geometric object.
        """
        ...

    @constmethod
    def copy(self) -> "Geometry2d":
        """
        Create a copy of this geometry.
        """
        ...
