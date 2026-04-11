# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.Persistence import Persistence
from App.Extension import Extension
from Base.Vector import Vector
from Base.Matrix import Matrix
from typing import Final, List, Optional

@export(
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
    Delete=True,
)
class Geometry(Persistence):
    """
    The abstract class Geometry for 3D space is the root class of all geometric objects.
    It describes the common behavior of these objects when:
    - applying geometric transformations to objects, and
    - constructing objects by geometric transformation (including copying).
    """

    Tag: Final[str]
    """Gives the tag of the geometry as string."""

    def mirror(self, geometry: "Geometry", /) -> None:
        """
        Performs the symmetrical transformation of this geometric object
        """
        ...

    def rotate(self, angle: float, axis: Vector, /) -> None:
        """
        Rotates this geometric object at angle Ang (in radians) about axis
        """
        ...

    def scale(self, center: Vector, factor: float, /) -> None:
        """
        Applies a scaling transformation on this geometric object with a center and scaling factor
        """
        ...

    def transform(self, transformation: Matrix, /) -> None:
        """
        Applies a transformation to this geometric object
        """
        ...

    def translate(self, vector: Vector, /) -> None:
        """
        Translates this geometric object
        """
        ...

    @constmethod
    def copy(self) -> "Geometry":
        """
        Create a copy of this geometry
        """
        ...

    @constmethod
    def clone(self) -> "Geometry":
        """
        Create a clone of this geometry with the same Tag
        """
        ...

    @constmethod
    def isSame(self, geom: "Geometry", tol: float, angulartol: float, /) -> bool:
        """
        isSame(geom, tol, angulartol) -> boolean

        Compare this geometry to another one
        """
        ...

    @constmethod
    def hasExtensionOfType(self, type_name: str, /) -> bool:
        """
        Returns a boolean indicating whether a geometry extension of the type indicated as a string exists.
        """
        ...

    @constmethod
    def hasExtensionOfName(self, name: str, /) -> bool:
        """
        Returns a boolean indicating whether a geometry extension with the name indicated as a string exists.
        """
        ...

    @constmethod
    def getExtensionOfType(self, type_name: str, /) -> Optional[Extension]:
        """
        Gets the first geometry extension of the type indicated by the string.
        """
        ...

    @constmethod
    def getExtensionOfName(self, name: str, /) -> Optional[Extension]:
        """
        Gets the first geometry extension of the name indicated by the string.
        """
        ...

    def setExtension(self, extension: Extension, /) -> None:
        """
        Sets a geometry extension of the indicated type.
        """
        ...

    def deleteExtensionOfType(self, type_name: str, /) -> None:
        """
        Deletes all extensions of the indicated type.
        """
        ...

    def deleteExtensionOfName(self, name: str, /) -> None:
        """
        Deletes all extensions of the indicated name.
        """
        ...

    @constmethod
    def getExtensions(self) -> List[Extension]:
        """
        Returns a list with information about the geometry extensions.
        """
        ...
