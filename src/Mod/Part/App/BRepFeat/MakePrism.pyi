# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.PyObjectBase import PyObjectBase
from typing import List

@export(
    PythonName="Part.BRepFeat.MakePrism",
    Twin="BRepFeat_MakePrism",
    TwinPointer="BRepFeat_MakePrism",
    Include="BRepFeat_MakePrism.hxx",
    Constructor=True,
    Delete=True,
)
class MakePrism(PyObjectBase):
    """
    Describes functions to build prism features.

    Author: Werner Mayer (wmayer[at]users.sourceforge.net)
    Licence: LGPL
    """

    def init(self, **kwargs) -> None:
        """
        Initializes this algorithm for building prisms along surfaces.
        A face Pbase is selected in the shape Sbase
        to serve as the basis for the prism. The orientation
        of the prism will be defined by the vector Direction.

        Fuse offers a choice between:
        -   removing matter with a Boolean cut using the setting 0
        -   adding matter with Boolean fusion using the setting 1.
        The sketch face Skface serves to determine
        the type of operation. If it is inside the basis
        shape, a local operation such as glueing can be performed.
        """
        ...

    def add(self, **kwargs) -> None:
        """
        Indicates that the edge will slide on the face.
        Raises ConstructionError if the  face does not belong to the
        basis shape, or the edge to the prismed shape.
        """
        ...

    def perform(self, **kwargs) -> None:
        """
        Assigns one of the following semantics.
        1. to a height Length
        2. to a face Until
        3. from a face From to a height Until. Reconstructs the feature topologically according to the semantic option chosen.
        """
        ...

    def performUntilEnd(self) -> None:
        """
        Realizes a semi-infinite prism, limited by the
        position of the prism base. All other faces extend infinitely.
        """
        ...

    def performFromEnd(self) -> None:
        """
        Realizes a semi-infinite prism, limited by the face Funtil.
        """
        ...

    def performThruAll(self) -> None:
        """
        Builds an infinite prism. The infinite descendants will not be kept in the result.
        """
        ...

    def performUntilHeight(self) -> None:
        """
        Assigns both a limiting shape, Until from TopoDS_Shape
        and a height, Length at which to stop generation of the prism feature.
        """
        ...

    @constmethod
    def curves(self) -> List:
        """
        Returns the list of curves S parallel to the axis of the prism.
        """
        ...

    @constmethod
    def barycCurve(self) -> object:
        """
        Generates a curve along the center of mass of the primitive.
        """
        ...

    @constmethod
    def shape(self) -> object:
        """
        Returns a shape built by the shape construction algorithm.
        """
        ...
