from Base.Metadata import export, constmethod, class_declarations
from Base.PyObjectBase import PyObjectBase
from typing import Final

@export(
    Twin="HLRBRep_Algo",
    TwinPointer="HLRBRep_Algo",
    Include="HLRBRep_Algo.hxx",
    Constructor=True,
)
@class_declarations("""
private:
    Handle(HLRBRep_Algo) hAlgo;

public:
    Handle(HLRBRep_Algo) handle() {
        return hAlgo;
    }
""")
class HLRBRep_Algo(PyObjectBase):
    """
    Algo() -> HLRBRep_Algo

    A framework to compute a shape as seen in a projection
    plane. This is done by calculating the visible and the hidden parts
    of the shape. HLRBRep_Algo works with three types of entity:

    - shapes to be visualized
    - edges in these shapes (these edges are the basic entities which will be
      visualized or hidden), and
    - faces in these shapes which hide the edges.

    HLRBRep_Algo is based on the principle of comparing each edge of the shape to
    be visualized with each of its faces, and calculating the visible and the
    hidden parts of each edge. For a given projection, HLRBRep_Algo calculates a
    set of lines characteristic of the object being represented. It is also used in
    conjunction with the HLRBRep_HLRToShape extraction utilities, which reconstruct
    a new, simplified shape from a selection of calculation results. This new shape
    is made up of edges, which represent the shape visualized in the
    projection. HLRBRep_Algo takes the shape itself into account whereas
    HLRBRep_PolyAlgo works with a polyhedral simplification of the shape. When you
    use HLRBRep_Algo, you obtain an exact result, whereas, when you use
    HLRBRep_PolyAlgo, you reduce computation time but obtain polygonal segments. In
    the case of complicated shapes, HLRBRep_Algo may be time-consuming. An
    HLRBRep_Algo object provides a framework for:

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

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def add(self, S, nbIso: int = 0) -> None:
        """
        add(S, nbIso=0)

        Adds the shape S to this framework, and specifies the number of isoparameters
        nbiso desired in visualizing S.  You may add as many shapes as you wish.  Use
        the function add once for each shape.
        """
        ...

    def remove(self, i: int) -> None:
        """
        remove(i)

        Remove the shape of index i from this framework.
        """
        ...

    def index(self, S) -> int:
        """
        index(S) ->  int

        Return the index of the Shape S and return 0 if the Shape S is not found.
        """
        ...

    def outLinedShapeNullify(self) -> None:
        """
        outlinedShapeNullify()

        Nullify all the results of OutLiner from HLRTopoBRep.
        """
        ...

    def setProjector(
        self,
        *,
        Origin: tuple[float, float, float] = (0, 0, 0),
        ZDir: tuple[float, float, float] = (0, 0, 0),
        XDir: tuple[float, float, float] = (0, 0, 0),
        focus: float = float("nan"),
    ) -> None:
        """
        setProjector(Origin=(0, 0, 0), ZDir=(0,0,0), XDir=(0,0,0), focus=NaN)

        Set the projector.  With focus left to NaN, an axonometric projector is
        created.  Otherwise, a perspective projector is created with focus focus.
        """
        ...

    def nbShapes(self) -> int:
        """
        nbShapes()

        Returns the number of shapes in the collection.  It does not modify the
        object's state and is used to retrieve the count of shapes.
        """
        ...

    def showAll(self, i: int = -1) -> None:
        """
        showAll(i=-1)

        If i < 1, then set all the edges to visible.
        Otherwise, set to visible all the edges of the shape of index i.
        """
        ...

    def hide(self, i: int = -1, j: int = -1) -> None:
        """
        hide(i=-1, j=-1)

        If i < 1, hide all of the datastructure.
        Otherwise, if j < 1, hide the shape of index i.
        Otherwise, hide the shape of index i by the shape of index j.
        """
        ...

    def hideAll(self, i: int = -1) -> None:
        """
        hideAll(i=-1)

        If i < 1, hide all the edges.
        Otherwise, hide all the edges of shape of index i.
        """
        ...

    def partialHide(self) -> None:
        """
        partialHide()

        Own hiding of all the shapes of the DataStructure without hiding by each other.
        """
        ...

    def select(self, i: int = -1) -> None:
        """
        select(i=-1)

        If i < 1, select all the DataStructure.
        Otherwise, only select the shape of index i.
        """
        ...

    def selectEdge(self, i: int) -> None:
        """
        selectEdge(i)

        Select only the edges of the shape of index i.
        """
        ...

    def selectFace(self, i: int) -> None:
        """
        selectFace(i)
		
        Select only the faces of the shape of index i.
        """
        ...

    def initEdgeStatus(self) -> None:
        """
        initEdgeStatus()

        Init the status of the selected edges depending of the back faces of a closed
        shell.
        """
        ...

    def update(self) -> None:
        """
        update()
		
        Update the DataStructure.
        """
        ...
