from Base.Metadata import export, constmethod
from Part.TopoShapePy import TopoShape
from Base.PyObjectBase import PyObjectBase
from typing import Optional, overload

@export(
    PythonName="Part.HLRToShapePy",
    Twin="HLRBRep_HLRToShape",
    TwinPointer="HLRBRep_HLRToShape",
    Include="HLRBRep_HLRToShape.hxx",
    Constructor=True,
    Delete=True,
)
class HLRToShape(PyObjectBase):
    """
    HLRToShape(algo: HLRBRep_Algo) -> HLRBRep_HLRToShape

    A framework for filtering the computation results of an HLRBRep_Algo algorithm
    by extraction. From the results calculated by the algorithm on a shape, a
    filter returns the type of edge you want to identify. You can choose any of the
    following types of output:
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
    new computation of the algorithm, but only reads its internal results. The
    methods of this shape are almost identic to those of the HLRBrep_PolyHLRToShape
    class.
    """

    def vCompound(self, Shape: Optional[TopoShape] = None) -> TopoShape:
        """
        vCompound(Shape=None) -> TopoShape

        Sets the extraction filter for visible sharp edges for either shape Shape or
        for all added shapes (Shape=None).
        """
        ...

    def Rg1LineVCompound(self, Shape: Optional[TopoShape] = None) -> TopoShape:
        """
        Rg1LineVCompound(Shape=None) -> TopoShape

        Sets the extraction filter for visible smooth edges for either shape Shape or
        for all added shapes (Shape=None).
        """
        ...

    def RgNLineVCompound(self, Shape: Optional[TopoShape] = None) -> TopoShape:
        """
        RgNLineVCompound(Shape=None) -> TopoShape

        Sets the extraction filter for visible sewn edges for either shape Shape or for
        all added shapes (Shape=None).
        """
        ...

    def outLineVCompound(self, Shape: Optional[TopoShape] = None) -> TopoShape:
        """
        outLineVCompound(Shape=None) -> TopoShape

        Sets the extraction filter for visible outline edges for either shape Shape or
        for all added shapes (Shape=None).
        """
        ...

    def outLineVCompound3d(self, Shape: Optional[TopoShape] = None) -> TopoShape:
        """
        outLineVCompound(Shape=None) -> TopoShape

        Sets the extraction filter for visible outline edges in 3D for either shape
        Shape or for all added shapes (Shape=None).
        """
        ...

    def isoLineVCompound(self, Shape: Optional[TopoShape] = None) -> TopoShape:
        """
        isoLineVCompound(Shape=None) -> TopoShape

        Sets the extraction filter for visible isoparameters for either shape Shape or
        for all added shapes (Shape=None).
        """
        ...

    def hCompound(self, Shape: Optional[TopoShape] = None) -> TopoShape:
        """
        hCompound(Shape=None) -> TopoShape

        Sets the extraction filter for hidden sharp edges for either shape Shape or for
        all added shapes (Shape=None).
        """
        ...

    def Rg1LineHCompound(self, Shape: Optional[TopoShape] = None) -> TopoShape:
        """
        Rg1LineHCompound(Shape=None) -> TopoShape

        Sets the extraction filter for hidden smooth edges for either shape Shape or
        for all added shapes (Shape=None).
        """
        ...

    def RgNLineHCompound(self, Shape: Optional[TopoShape] = None) -> TopoShape:
        """
        RgNLineHCompound(Shape=None) -> TopoShape

        Sets the extraction filter for hidden sewn edges for either shape Shape or for
        all added shapes (Shape=None).
        """
        ...

    def outLineHCompound(self, Shape: Optional[TopoShape] = None) -> TopoShape:
        """
        outLineHCompound(Shape=None) -> TopoShape

        Sets the extraction filter for hidden outline edges for either shape Shape or
        for all added shapes (Shape=None).
        """
        ...

    def isoLineHCompound(self, Shape: Optional[TopoShape] = None) -> TopoShape:
        """
        isoLineHCompound(Shape=None) -> TopoShape

        Sets the extraction filter for hidden isoparameters for either shape Shape or
        for all added shapes (Shape=None).
        """
        ...

    def compoundOfEdges(self, Type: int, Visible: bool, In3D: bool, *, Shape: Optional[TopoShape] = None) -> TopoShape:
        """
        compoundOfEdges(Type: int, Visible: bool, In3D: bool, Shape=None) -> TopoShape

        Returns compound of resulting edges of required type and visibility, taking
        into account the kind of space (2d or 3d).  If Shape=None, return it for all
        added shapes, otherwise return it for shape Shape.
        """
        ...