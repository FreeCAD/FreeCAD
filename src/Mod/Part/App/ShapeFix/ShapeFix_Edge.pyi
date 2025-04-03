from Base.Metadata import export, constmethod, class_declarations
from Base.PyObjectBase import PyObjectBase
from typing import Final

@export(
    PythonName="Part.ShapeFix.Edge",
    Include="ShapeFix_Edge.hxx",
    Constructor=True,
)
@class_declarations("""
private:
    Handle(ShapeFix_Edge) hEdge;

public:
    void setHandle(Handle(ShapeFix_Edge) handle) {
        setTwinPointer(handle.get());
        hEdge = handle;
    }
""")
class ShapeFix_Edge(PyObjectBase):
    """
    Fixing invalid edge

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def fixRemovePCurve(self) -> bool:
        """
        Removes the pcurve(s) of the edge if it does not match the
        vertices
        Check is done
        Use    : It is to be called when pcurve of an edge can be wrong
        (e.g., after import from IGES)
        Returns: True, if does not match, removed (status DONE)
        False, (status OK) if matches or (status FAIL) if no pcurve,
        nothing done.
        """
        ...

    def fixRemoveCurve3d(self) -> bool:
        """
        Removes 3d curve of the edge if it does not match the vertices
        Returns: True,  if does not match, removed (status DONE)
        False, (status OK) if matches or (status FAIL) if no 3d curve,
        nothing done.
        """
        ...

    def fixAddPCurve(self) -> bool:
        """
        Adds pcurve(s) of the edge if missing (by projecting 3d curve)
        Parameter isSeam indicates if the edge is a seam.
        The parameter 'prec' defines the precision for calculations.
        If it is 0 (default), the tolerance of the edge is taken.
        Remark : This method is rather for internal use since it accepts parameter
        'surfana' for optimization of computations
        Use    : It is to be called after FixRemovePCurve (if removed) or in any
        case when edge can have no pcurve
        Returns: True if pcurve was added, else False
        Status :
        OK   : Pcurve exists
        FAIL1: No 3d curve
        FAIL2: fail during projecting
        DONE1: Pcurve was added
        DONE2: specific case of pcurve going through degenerated point on
        sphere encountered during projection (see class
        ShapeConstruct_ProjectCurveOnSurface for more info).
        """
        ...

    def fixAddCurve3d(self) -> bool:
        """
        Tries to build 3d curve of the edge if missing
        Use    : It is to be called after FixRemoveCurve3d (if removed) or in any
        case when edge can have no 3d curve
        Returns: True if 3d curve was added, else False
        Status :
        OK   : 3d curve exists
        FAIL1: BRepLib::BuildCurve3d() has failed
        DONE1: 3d curve was added.
        """
        ...

    def fixVertexTolerance(self) -> bool:
        """
        Increases the tolerances of the edge vertices to comprise
        the ends of 3d curve and pcurve on the given face
        (first method) or all pcurves stored in an edge (second one)
        Returns: True, if tolerances have been increased, otherwise False
        Status:
        OK   : the original tolerances have not been changed
        DONE1: the tolerance of first vertex has been increased
        DONE2: the tolerance of last  vertex has been increased.
        """
        ...

    def fixReversed2d(self) -> bool:
        """
        Fixes edge if pcurve is directed opposite to 3d curve
        Check is done by call to the function
        ShapeAnalysis_Edge::CheckCurve3dWithPCurve()
        Warning: For seam edge this method will check and fix the pcurve in only
        one direction. Hence, it should be called twice for seam edge:
        once with edge orientation FORWARD and once with REVERSED.
        Returns: False if nothing done, True if reversed (status DONE)
        Status:  OK    - pcurve OK, nothing done
        FAIL1 - no pcurve
        FAIL2 - no 3d curve
        DONE1 - pcurve was reversed.
        """
        ...

    def fixSameParameter(self) -> bool:
        """
        Tries to make edge SameParameter and sets corresponding
        tolerance and SameParameter flag.
        First, it makes edge same range if SameRange flag is not set.
        If flag SameParameter is set, this method calls the
        function ShapeAnalysis_Edge::CheckSameParameter() that
        calculates the maximal deviation of pcurves of the edge from
        its 3d curve. If deviation > tolerance, the tolerance of edge
        is increased to a value of deviation. If deviation < tolerance
        nothing happens.

        If flag SameParameter is not set, this method chooses the best
        variant (one that has minimal tolerance), either
        a. only after computing deviation (as above) or
        b. after calling standard procedure BRepLib::SameParameter
        and computing deviation (as above). If 'tolerance' > 0, it is
        used as parameter for BRepLib::SameParameter, otherwise,
        tolerance of the edge is used.

        Use    : Is to be called after all pcurves and 3d curve of the edge are
        correctly computed
        Remark : SameParameter flag is always set to True after this method
        Returns: True, if something done, else False
        Status : OK    - edge was initially SameParameter, nothing is done
        FAIL1 - computation of deviation of pcurves from 3d curve has failed
        FAIL2 - BRepLib::SameParameter() has failed
        DONE1 - tolerance of the edge was increased
        DONE2 - flag SameParameter was set to True (only if
        BRepLib::SameParameter() did not set it)
        DONE3 - edge was modified by BRepLib::SameParameter() to SameParameter
        DONE4 - not used anymore
        DONE5 - if the edge resulting from BRepLib has been chosen, i.e. variant b. above
        (only for edges with not set SameParameter).
        """
        ...
