from Base.Metadata import export, constmethod, class_declarations
from Base.PyObjectBase import PyObjectBase
from Part.TopoShapePy import TopoShape
from typing import Final, overload

@export(
    PythonName="Part.HLRBRep_PolyAlgo",
    Twin="HLRBRep_PolyAlgo",
    TwinPointer="HLRBRep_PolyAlgo",
    Include="HLRBRep_PolyAlgo.hxx",
    Constructor=True,
)
@class_declarations("""
private:
    Handle(HLRBRep_PolyAlgo) hAlgo;

public:
    Handle(HLRBRep_PolyAlgo) handle() {
        return hAlgo;
    }
""")
class HLRBRep_PolyAlgo(PyObjectBase):
    """
    PolyAlgo() -> HLRBRep_PolyAlgo

    A framework to compute the shape as seen in a projection
    plane. This is done by calculating the visible and the hidden parts of the
    shape. HLRBRep_PolyAlgo works with three types of entity:

    - shapes to be visualized (these shapes must have already been triangulated.)
    - edges in these shapes (these edges are defined as polygonal lines on the
      triangulation of the shape, and are the basic entities which will be visualized
      or hidden), and
    - triangles in these shapes which hide the edges.

    HLRBRep_PolyAlgo is based on the principle of comparing each edge of the shape
    to be visualized with each of the triangles produced by the triangulation of
    the shape, and calculating the visible and the hidden parts of each edge. For a
    given projection, HLRBRep_PolyAlgo calculates a set of lines characteristic of
    the object being represented. It is also used in conjunction with the
    HLRBRep_PolyHLRToShape extraction utilities, which reconstruct a new,
    simplified shape from a selection of calculation results. This new shape is
    made up of edges, which represent the shape visualized in the
    projection. HLRBRep_PolyAlgo works with a polyhedral simplification of the
    shape whereas HLRBRep_Algo takes the shape itself into account. When you use
    HLRBRep_Algo, you obtain an exact result, whereas, when you use
    HLRBRep_PolyAlgo, you reduce computation time but obtain polygonal segments. An
    HLRBRep_PolyAlgo object provides a framework for:

    - defining the point of view
    - identifying the shape or shapes to be visualized
    - calculating the outlines
    - calculating the visible and hidden lines of the shape. Warning
    - Superimposed lines are not eliminated by this algorithm.
    - There must be no unfinished objects inside the shape you wish to visualize.
    - Points are not treated.
    - Note that this is not the sort of algorithm used in generating shading, which
      calculates the visible and hidden parts of each face in a shape to be
      visualized by comparing each face in the shape with every other face in the
      same shape.
    """

    def load(self, S: TopoShape) -> None:
        """
        load(S)

        Loads the shape S into this framework. Warning S must have already been triangulated.
        """
        ...

    def remove(self, i: int) -> None:
        """
        remove(i)

        Remove the shape of index i from this framework.
	    """
        ...

    def nbShapes(self) -> int:
        """
        nbShapes()

        Returns the number of shapes in the collection.  It does not modify the
        object's state and is used to retrieve the count of shapes.
	    """
        ...

    def shape(self, i: int) -> TopoShape:
        """
        shape(i) -> TopoShape

        Return the shape of index i.
	    """
        ...

    def index(self, S: TopoShape) -> int:
        """
        index(S) ->  int

        Return the index of the Shape S.
	    """
        ...

    def setProjector(self, *, Origin: tuple[float, float, float] = (0.0, 0.0, 0.0),
                     ZDir: tuple[float, float, float] = (0.0, 0.0, 0.0),
                     XDir: tuple[float, float, float] = (0.0, 0.0, 0.0),
                     focus: float = float("nan")) -> None:
        """
        setProjector(Origin=(0, 0, 0), ZDir=(0,0,0), XDir=(0,0,0), focus=NaN)

        Set the projector.  With focus left to NaN, an axonometric projector is
        created.  Otherwise, a perspective projector is created with focus focus.
        """
        ...

    def update(self) -> None:
        """
        update()

        Launches calculation of outlines of the shape visualized by this
        framework. Used after setting the point of view and defining the shape or
        shapes to be visualized.
	    """
        ...

    def initHide(self) -> None:
        """
        initHide()
        """
        ...

    def moreHide(self) -> None:
        """
        moreHide()
        """
        ...

    def nextHide(self) -> None:
        """
        nextHide()
        """
        ...

    def initShow(self) -> None:
        """
        initShow()
        """
        ...

    def moreShow(self) -> None:
        """
        moreShow()
        """
        ...

    def nextShow(self) -> None:
        """
        nextShow()
        """
        ...

    def outLinedShape(self, S: TopoShape) -> TopoShape:
        """
        outLinedShape(S) -> TopoShape

        Make a shape with the internal outlines in each face of shape S.
        """
        ...

    TolAngular: float = ...

    TolCoef: float = ...
