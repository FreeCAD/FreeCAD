# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.PyObjectBase import PyObjectBase

@export(
    PythonName="Part.ShapeUpgrade.UnifySameDomain",
    Include="ShapeUpgrade_UnifySameDomain.hxx",
    Twin="ShapeUpgrade_UnifySameDomain",
    TwinPointer="ShapeUpgrade_UnifySameDomain",
    Constructor=True,
    Delete=True,
)
class UnifySameDomain(PyObjectBase):
    """
    This tool tries to unify faces and edges of the shape which lie on the same geometry.

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def initialize(self, **kwargs) -> None:
        """
        Initializes with a shape and necessary flags
        """
        ...

    def allowInternalEdges(self) -> None:
        """
        Sets the flag defining whether it is allowed to create
        internal edges inside merged faces in the case of non-manifold
        topology. Without this flag merging through multi connected edge
        is forbidden. Default value is false.
        """
        ...

    def keepShape(self) -> None:
        """
        Sets the shape for avoid merging of the faces/edges.
        """
        ...

    def keepShapes(self) -> None:
        """
        Sets the map of shapes for avoid merging of the faces/edges.
        """
        ...

    def setSafeInputMode(self) -> None:
        """
        Sets the flag defining the behavior of the algorithm regarding
        modification of input shape.
        If this flag is equal to True then the input (original) shape can't be
        modified during modification process. Default value is true.
        """
        ...

    def setLinearTolerance(self) -> None:
        """
        Sets the linear tolerance
        """
        ...

    def setAngularTolerance(self) -> None:
        """
        Sets the angular tolerance
        """
        ...

    def build(self) -> None:
        """
        Performs unification and builds the resulting shape
        """
        ...

    @constmethod
    def shape(self) -> None:
        """
        Gives the resulting shape
        """
        ...
