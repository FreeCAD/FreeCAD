# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase
from Part.HLRBRep_PolyAlgo import HLRBRep_PolyAlgo
from Part.TopoShapePy import TopoShape
from typing import Optional

@export(
    PythonName="Part.PolyHLRToShapePy",
    Twin="HLRBRep_PolyHLRToShape",
    TwinPointer="HLRBRep_PolyHLRToShape",
    Include="HLRBRep_PolyHLRToShape.hxx",
    Constructor=True,
    Delete=True,
)
class PolyHLRToShape(PyObjectBase):
    """
    PolyHLRToShape(algo: HLRBRep_PolyAlgo) -> HLRBRep_PolyHLRToShape

    A framework for filtering the computation results of an HLRBRep_PolyAlgo
    algorithm by extraction.  From the results calculated by the algorithm on a
    shape, a filter returns the type of edge you want to identify.  You can choose
    any of the following types of output:
    - visible sharp edges
    - hidden sharp edges
    - visible smooth edges
    - hidden smooth edges
    - visible sewn edges
    - hidden sewn edges
    - visible outline edges
    - hidden outline edges
    - visible isoparameters and
    - hidden isoparameters.

    Sharp edges present a C0 continuity (non G1). Smooth edges present a G1
    continuity (non G2). Sewn edges present a C2 continuity. The result is composed
    of 2D edges in the projection plane of the view which the algorithm has worked
    with. These 2D edges are not included in the data structure of the visualized
    shape. In order to obtain a complete image, you must combine the shapes given
    by each of the chosen filters. The construction of the shape does not call a
    new computation of the algorithm, but only reads its internal results.
    """

    def update(self, algo: HLRBRep_PolyAlgo, /) -> None:
        """
        update(algo: HLRBRep_PolyAlgo)
        """
        ...

    def show(self) -> None:
        """
        show()
        """
        ...

    def hide(self) -> None:
        """
        hide()
        """
        ...

    def vCompound(self, Shape: Optional[TopoShape] = None, /) -> TopoShape:
        """
        vCompound(Shape=None) -> TopoShape

        Sets the extraction filter for visible sharp edges for either shape Shape or
        for all added shapes (Shape=None).
        """
        ...

    def Rg1LineVCompound(self, Shape: Optional[TopoShape] = None, /) -> TopoShape:
        """
        Rg1LineVCompound(Shape=None) -> TopoShape

        Sets the extraction filter for visible smooth edges for either shape Shape or
        for all added shapes (Shape=None).
        """
        ...

    def RgNLineVCompound(self, Shape: Optional[TopoShape] = None, /) -> TopoShape:
        """
        RgNLineVCompound(Shape=None) -> TopoShape

        Sets the extraction filter for visible sewn edges for either shape Shape or for
        all added shapes (Shape=None).
        """
        ...

    def outLineVCompound(self, Shape: Optional[TopoShape] = None, /) -> TopoShape:
        """
        outLineVCompound(Shape=None) -> TopoShape

        Sets the extraction filter for visible outline edges for either shape Shape or
        for all added shapes (Shape=None).
        """
        ...

    def hCompound(self, Shape: Optional[TopoShape] = None, /) -> TopoShape:
        """
        hCompound(Shape=None) -> TopoShape

        Sets the extraction filter for hidden sharp edges for either shape Shape or for
        all added shapes (Shape=None).
        """
        ...

    def Rg1LineHCompound(self, Shape: Optional[TopoShape] = None, /) -> TopoShape:
        """
        Rg1LineHCompound(Shape=None) -> TopoShape

        Sets the extraction filter for hidden smooth edges for either shape Shape or
        for all added shapes (Shape=None).
        """
        ...

    def RgNLineHCompound(self, Shape: Optional[TopoShape] = None, /) -> TopoShape:
        """
        RgNLineHCompound(Shape=None) -> TopoShape

        Sets the extraction filter for hidden sewn edges for either shape Shape or for
        all added shapes (Shape=None).
        """
        ...

    def outLineHCompound(self, Shape: Optional[TopoShape] = None, /) -> TopoShape:
        """
        outLineHCompound(Shape=None) -> TopoShape

        Sets the extraction filter for hidden outline edges for either shape Shape or
        for all added shapes (Shape=None).
        """
        ...
