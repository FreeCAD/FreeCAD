# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod, no_args
from Base.Quantity import Quantity
from Base.Vector import Vector
from Base.Axis import Axis
from Part.App.Part2DObject import Part2DObject
from Part.App.Geometry import Geometry
from Sketcher.App.Constraint import Constraint
from typing import List, Tuple, Union, Final, overload

@export(
    Include="Mod/Sketcher/App/SketchObject.h",
    FatherInclude="Mod/Part/App/Part2DObjectPy.h",
)
class SketchObject(Part2DObject):
    """
    Represents a sketch object

    Author: Juergen Riegel
    Licence: LGPL
    """

    MissingPointOnPointConstraints: List = ...
    """Returns a list of (First FirstPos Second SecondPos Type) tuples with all the detected endpoint constraints."""

    MissingVerticalHorizontalConstraints: List = ...
    """Returns a list of (First FirstPos Second SecondPos Type) tuples with all the detected vertical/horizontal constraints."""

    MissingLineEqualityConstraints: List = ...
    """Returns a list of (First FirstPos Second SecondPos) tuples with all the detected line segment equality constraints."""

    MissingRadiusConstraints: List = ...
    """Returns a list of (First FirstPos Second SecondPos) tuples with all the detected radius constraints."""

    OpenVertices: Final[List] = ...
    """Returns a list of vertices positions."""

    ConstraintCount: Final[int] = ...
    """Number of Constraints in this sketch"""

    GeometryCount: Final[int] = ...
    """Number of geometric objects in this sketch"""

    AxisCount: Final[int] = ...
    """Return the number of construction lines in the sketch which can be used as axes"""

    GeometryFacadeList: List = ...
    """Return a list of GeometryFacade objects corresponding to the PropertyGeometryList"""

    DoF: Final[int] = ...
    """Return the DoFs of the current solved sketch"""

    ConflictingConstraints: Final[List] = ...
    """Return a list of integers indicating the constraints detected as conflicting"""

    RedundantConstraints: Final[List] = ...
    """Return a list of integers indicating the constraints detected as redundant"""

    PartiallyRedundantConstraints: Final[List] = ...
    """Return a list of integers indicating the constraints detected as partially redundant"""

    MalformedConstraints: Final[List] = ...
    """Return a list of integers indicating the constraints detected as malformed"""

    def solve(self) -> int:
        """
        Solve the sketch and update the geometry.

        solve()

          Returns:
              0 in case of success, otherwise the following codes in this order of
              priority:
              -4 if over-constrained,
              -3 if conflicting constraints,
              -5 if malformed constraints
              -1 if solver error,
              -2 if redundant constraints.
        """
        ...

    @overload
    def addGeometry(self, geo: Geometry, isConstruction: bool = False, /) -> int: ...
    @overload
    def addGeometry(
        self, geo: List[Geometry], isConstruction: bool = False, /
    ) -> Tuple[int, ...]: ...
    def addGeometry(
        self, geo: Union[Geometry, List[Geometry]], isConstruction: bool = False, /
    ) -> Union[int, Tuple[int, ...]]:
        """
        Add geometric objects to the sketch.

        addGeometry(geo:Geometry, isConstruction=False) -> int
            Add a single geometric object to the sketch.

            Args:
                geo: The geometry to add. e.g. a Part.LineSegement
                isConstruction: Whether the added geometry is a "construction geometry".
                    Defaults to `False`, i.e. by omitting, a regular geometry is added.

            Returns:
                The zero-based index of the newly added geometry.

        addGeometry(geo:List(Geometry), isConstruction=False) -> Tuple(int)
            Add many geometric objects to the sketch.

            Args:
                geo: The geometry to add.
                isConstruction: see above.

            Returns:
                A tuple of zero-based indices of all newly added geometry.
        """
        ...

    def delGeometry(self, geoId: int, noSolve: bool, /) -> None:
        """
        Delete a geometric object from the sketch.

        delGeometry(geoId:int)

            Args:
                geoId: The zero-based index of the geometry to delete.
                    Any internal alignment geometry thereof will be deleted, too.
        """
        ...

    def delGeometries(self, geoIds: List[int], noSolve: bool, /) -> None:
        """
        Delete a list of geometric objects from the sketch.

        delGeometries(geoIds:List(int))

            Args:
                geoId: A list of zero-based indices of the geometry to delete.
                    Any internal alignment geometry thereof will be deleted, too.
        """
        ...

    def deleteAllGeometry(self, noSolve: bool, /) -> None:
        """
        Delete all the geometry objects from the sketch, except external geometry.

        deleteAllGeometry()
        """
        ...

    def detectDegeneratedGeometries(self, tolerance: float, /) -> int:
        """
        Detect degenerated geometries. A curve geometry is considered degenerated
        if the parameter range is less than the tolerance.

        detectDegeneratedGeometries(tolerance:float)

            Args:
                tolerance: The tolerance to check the parameter range of a curve.

            Returns:
                The number of degenerated geometries.
        """
        ...

    def removeDegeneratedGeometries(self, tolerance: float, /) -> int:
        """
        Remove degenerated geometries. A curve geometry is considered degenerated
        if the parameter range is less than the tolerance.

        removeDegeneratedGeometries(tolerance:float)

            Args:
                tolerance: The tolerance to check the parameter range of a curve.

            Returns:
                The number of degenerated geometries.
        """
        ...

    def deleteAllConstraints(self) -> None:
        """
        Delete all the constraints from the sketch.

        deleteAllConstraints()
        """
        ...

    def toggleConstruction(self, geoId: int, /) -> None:
        """
        Toggles a geometry between regular and construction.

        toggleConstruction(geoId:int)

            Args:
                geoId: The zero-based index of the geometry to toggle.
        """
        ...

    def setConstruction(self, geoId: int, state: bool, /) -> None:
        """
        Set construction mode of a geometry.

        setConstruction(geoId:int, state:bool)

            Args:
                geoId: The zero-based index of the geometry to configure.
                state: `True` configures the geometry to "construction geometry",
                    `False` configures it to regular geometry.
        """
        ...

    def getConstruction(self, geoId: int, /) -> bool:
        """
        Determine whether the given geometry is a "construction geometry".

        getConstruction(geoId:int)

            Args:
                geoId: The zero-based index of the geometry to query.

            Returns:
                `True` if the geometry is "construction geometry" and
                `False` if it s a regular geometry.
        """
        ...

    @overload
    def addConstraint(self, constraint: Constraint, /) -> int: ...
    @overload
    def addConstraint(self, constraints: List[Constraint], /) -> Tuple[int, ...]: ...
    def addConstraint(
        self, constraint: Union[Constraint, List[Constraint]], /
    ) -> Union[int, Tuple[int, ...]]:
        """
        Add constraints to the sketch.

        addConstraint(constraint:Constraint) -> int
            Add a single constraint to the sketch and solves it.

            Returns:
                The zero-based index of the newly added constraint.

        addConstraint(constraints:List(Constraint)) -> Tuple(int)
            Add many constraints to the sketch without solving.

            Returns:
                A tuple of zero-based indices of all newly added constraints.
        """
        ...

    def delConstraint(self, constraintIndex: int, noSolve: bool, /) -> None:
        """
        Delete a constraint from the sketch.

        delConstraint(constraintIndex:int)

            Args:
                constraintIndex: The zero-based index of the constraint to delete.
        """
        ...

    def delConstraints(
        self, constraintIndices: List[int], updateGeometry: bool, noSolve: bool, /
    ) -> None:
        """
        Delete multiple constraints from a sketch

        delConstraints(constraintIndices: List[int], updateGeometry: bool)

            Args:
                constraintIndices: The zero-based indices of the constraints to delete
                updateGeometry: Whether to update the geometry after solve
        """
        ...

    def renameConstraint(self, constraintIndex: int, name: str, /) -> None:
        """
        Rename a constraint in the sketch.

        renameConstraint(constraintIndex:int, name:str)

            Args:
                constraintIndex: The zero-based index of the constraint to rename.
                name: The new name for the constraint.
                    An empty string makes the constraint "unnamed" again.
        """
        ...

    @constmethod
    def getIndexByName(self, name: str, /) -> int:
        """
        Get the index of a constraint by name.

        getIndexByName(name:str)

            Args:
                name: The name for the constraint to look up.
                    If there is no such constraint an exception is raised.
        """
        ...

    def carbonCopy(self, objName: str, asConstruction: bool = True, /) -> None:
        """
        Copy another sketch's geometry and constraints into this sketch.

        carbonCopy(objName:str, asConstruction=True)

            Args:
                ObjName: The name of the sketch object to copy from.
                asConstruction: Whether to copy the geometry as "construction geometry".
        """
        ...

    def addExternal(
        self, objName: str, subName: str, defining: bool = False, intersection: bool = False, /
    ) -> None:
        """
        Add a link to an external geometry.

        addExternal(objName:str, subName:str, defining:bool=False, intersection:bool=False)

            Args:
                objName: The name of the document object to reference.
                subName: The name of the sub-element of the object's shape to link as
                    "external geometry".
                defining: Should the external edges be defining or construction?
                intersection: Should the external edges be projections or intersections?
        """
        ...

    def delExternal(self, extGeoId: int, /) -> None:
        """
        Delete an external geometry link from the sketch.

        delExternal(extGeoId:int)

            Args:
                extGeoId: The zero-based index of the external geometry to remove.
        """
        ...

    @overload
    def delConstraintOnPoint(self, vertexId: int, /) -> None: ...
    @overload
    def delConstraintOnPoint(self, geoId: int, pointPos: int, /) -> None: ...
    def delConstraintOnPoint(self, *args: int) -> None:
        """
        Delete coincident constraints associated with a sketch point.

        delConstraintOnPoint(vertexId:int)

            Args:
                vertexId: A zero-based index of the shape's vertices.

        delConstraintOnPoint(geoId:int, pointPos:int)

            Args:
                geoId: The zero-based index of the geometry that contains the point.
                pointPos: Enum denoting which point on the geometry is meant:
                    1: the start of a line or bounded curve.
                    2: the end of a line or bounded curve.
                    3: the center of a circle or ellipse.
        """
        ...

    @no_args
    def delConstraintsToExternal(self) -> None:
        """
        Deletes all constraints referencing an external geometry.
        """
        ...

    def setDatum(self, constraint: Union[int, str], value: Union[float, Quantity], /) -> None:
        """
        Set the value of a datum constraint (e.g. Distance or Angle)

        setDatum(constraint, value)

            Args:
                constraint (int or str): The index or name of the constraint to set.
                value (float or Quantity): The value to set for the constraint. When
                    using floats, values for linear dimensions are interpreted as
                    millimeter, angular ones as radians.
        """
        ...

    @constmethod
    def getDatum(self, constraint: Union[int, str], /) -> Quantity:
        """
        Get the value of a datum constraint (e.g. Distance or Angle)

        getDatum(constraint) -> Quantity

            Args:
                constraint (int or str): The index or name of the constraint to query.

            Returns:
                The value of the constraint.
        """
        ...

    def setDriving(self, constraintIndex: int, state: bool, /) -> None:
        """
        Set the Driving status of a datum constraint.

        setDriving(constraintIndex:int, state:bool)

            Args:
                constraintIndex: The zero-based index of the constraint to configure.
                state: `True` sets the constraint to driving,
                    `False` configures it as non-driving, i.e. reference.
        """
        ...

    def setDatumsDriving(self, state: bool, /) -> None:
        """
        Set the Driving status of all datum constraints.

        setDatumsDriving(state:bool)

            Args:
                state: `True` set all datum constraints to driving,
                    `False` configures them as non-driving, i.e. reference.
        """
        ...

    def moveDatumsToEnd(self) -> None:
        """
        Moves all datum constraints to the end of the constraint list.

        moveDatumsToEnd()

            Warning: This method reorders the constraint indices. Previously held
                numeric references to constraints may reference different constraints
                after this operation.
        """
        ...

    @constmethod
    def getDriving(self, constraintIndex: int, /) -> bool:
        """
        Get the Driving status of a datum constraint.

        getDriving(constraintIndex:int)

            Args:
                constraintIndex: The zero-based index of the constraint to query.

            Returns:
                `True` if the constraint is driving,
                `False` if it is non-driving, i.e. reference.
        """
        ...

    def toggleDriving(self, constraintIndex: int, /) -> None:
        """
        Toggle the Driving status of a datum constraint.

        toggleDriving(constraintIndex:int)

            Args:
                constraintIndex: The zero-based index of the constraint to toggle.
        """
        ...

    def setVirtualSpace(self) -> None:
        """
        Set the VirtualSpace status of a constraint
        """
        ...

    def setVisibility(self) -> None:
        """
        Set the visibility of a constraint
        """
        ...

    def getVirtualSpace(self) -> bool:
        """
        Get the VirtualSpace status of a constraint
        """
        ...

    def toggleVirtualSpace(self) -> None:
        """
        Toggle the VirtualSpace status of a constraint
        """
        ...

    def setActive(self, constraintIndex: int, state: bool, /) -> None:
        """
        Activates or deactivates a constraint (enforce it or not).

        setActive(constraintIndex:int, state:bool)

            Args:
                constraintIndex: The zero-based index of the constraint to configure.
                state: `True` sets the constraint to active i.e. enforced,
                    `False` configures it as inactive, i.e. not enforced.
        """
        ...

    @constmethod
    def getActive(self, constraintIndex: int, /) -> bool:
        """
        Get whether a constraint is active, i.e. enforced, or not.

        getActive(constraintIndex:int)

            Args:
                constraintIndex: The zero-based index of the constraint to query.

            Returns:
                `True` if the constraint is active, i.e. enforced,
                `False` if it is inactive, i.e. not enforced.
        """
        ...

    def toggleActive(self, constraintIndex: int, /) -> None:
        """
        Toggle the constraint between active (enforced) and inactive.

        toggleActive(constraintIndex:int)

            Args:
                constraintIndex: The zero-based index of the constraint to toggle.
        """
        ...

    @constmethod
    def getLabelPosition(self, constraintIndex: int, /) -> float:
        """
        Get label position of the constraint.

        getLabelPosition(constraintIndex:int)

            Args:
                constraintIndex: The zero-based index of the constraint to query.

            Returns:
                float with the current value.
        """
        ...

    def setLabelPosition(self, constraintIndex: int, value: float, /) -> None:
        """
        Set label position of the constraint.

        setLabelPosition(constraintIndex:int, value:float)

            Args:
                constraintIndex: The zero-based index of the constraint to query.
                value: Value of the label position.
        """
        ...

    @constmethod
    def getLabelDistance(self, constraintIndex: int, /) -> float:
        """
        Get label distance of the constraint.

        getLabelDistance(constraintIndex:int)

            Args:
                constraintIndex: The zero-based index of the constraint to query.

            Returns:
                float with the current value.
        """
        ...

    def setLabelDistance(self, constraintIndex: int, value: float, /) -> None:
        """
        Set label distance of the constraint.

        setLabelDistance(constraintIndex:int, value:float)

            Args:
                constraintIndex: The zero-based index of the constraint to query.
                value: Value of the label position.
        """
        ...

    def moveGeometry(
        self, GeoIndex: int, PointPos: int, Vector: Vector, relative: bool = False, /
    ) -> None:
        """
        Move a given point (or curve) to another location.

        moveGeometry(GeoIndex,PointPos,Vector,[relative])

        It moves the specified point (or curve) to the given location by adding some
        temporary weak constraints and solving the sketch.
        This method is mostly used to allow the user to drag some portions of the sketch
        in real time by e.g. the mouse and it works only for underconstrained portions of
        the sketch.
        The argument 'relative', if present, states if the new location is given
        relatively to the current one.
        """
        ...

    def moveGeometries(
        self, Geos: List[Tuple[int, int]], Vector: Vector, relative: bool = False, /
    ) -> None:
        """
        Move given points and curves to another location.

        moveGeometries(Geos,Vector,[relative])

        It moves the specified points and curves to the given location by adding some
        temporary weak constraints and solving the sketch.
        This method is mostly used to allow the user to drag some portions of the sketch
        in real time by e.g. the mouse and it works only for underconstrained portions of
        the sketch.
        The argument 'relative', if present, states if the new location is given
        relatively to the current one. For group dragging this is enforced.
        Geos is a vector of pairs of geoId and posId.
        """
        ...

    @constmethod
    def getPoint(self, GeoIndex: int, PointPos: int, /) -> Vector:
        """
        Retrieve the vector of a point in the sketch.

        getPoint(GeoIndex,PointPos)
        """
        ...

    @constmethod
    def getGeoVertexIndex(self, index: int, /) -> Tuple[int, int]:
        """
        Retrieve the GeoId and PosId of a point in the sketch.

        (geoId, posId) = getGeoVertexIndex(index)
        """
        ...

    @constmethod
    def getAxis(self) -> Axis:
        """
        Return an axis based on the corresponding construction line
        """
        ...

    def fillet(self) -> None:
        """
        Create a fillet between two edges or at a point
        """
        ...

    def trim(self) -> None:
        """
        Trim a curve with a given id at a given reference point
        """
        ...

    def extend(self) -> None:
        """
        Extend a curve to new start and end positions
        """
        ...

    def split(self) -> None:
        """
        Split a curve with a given id at a given reference point
        """
        ...

    def join(self) -> None:
        """
        Join two curves at the given end points
        """
        ...

    def addSymmetric(self) -> None:
        """
        Add symmetric geometric objects to the sketch with respect to a reference point or line
        """
        ...

    def addCopy(self) -> None:
        """
        Add a copy of geometric objects to the sketch displaced by a vector3d
        """
        ...

    def addMove(self) -> None:
        """
        Move the geometric objects in the sketch displaced by a vector3d
        """
        ...

    def addRectangularArray(self) -> None:
        """
        Add an array of size cols by rows where each element is a copy of the selected geometric objects displaced by a vector3d in the cols direction and by a vector perpendicular to it in the rows direction
        """
        ...

    def removeAxesAlignment(self) -> None:
        """
        Modifies constraints so that the shape is not forced to be aligned with axes.
        """
        ...

    def ExposeInternalGeometry(self) -> None:
        """
        Deprecated -- use exposeInternalGeometry
        """
        ...

    def DeleteUnusedInternalGeometry(self) -> None:
        """
        Deprecated -- use deleteUnusedInternalGeometry
        """
        ...

    def exposeInternalGeometry(self) -> None:
        """
        Exposes all internal geometry of an object supporting internal geometry
        """
        ...

    def deleteUnusedInternalGeometry(self) -> None:
        """
        Deletes all unused (not further constrained) internal geometry
        """
        ...

    def convertToNURBS(self) -> None:
        """
        Approximates the given geometry with a B-spline
        """
        ...

    def increaseBSplineDegree(self) -> None:
        """
        Increases the given B-spline Degree by a number of degrees
        """
        ...

    def decreaseBSplineDegree(self) -> None:
        """
        Decreases the given B-spline Degree by a number of degrees by approximating this curve
        """
        ...

    def modifyBSplineKnotMultiplicity(self) -> None:
        """
        Increases or reduces the given BSpline knot multiplicity
        """
        ...

    def insertBSplineKnot(self) -> None:
        """
        Inserts a knot into the BSpline at the given param with given multiplicity. If the knot already exists, this increases the knot multiplicity by the given multiplicity.
        """
        ...

    def calculateAngleViaPoint(self, GeoId1: int, GeoId2: int, px: float, py: float, /) -> float:
        """
        calculateAngleViaPoint(GeoId1, GeoId2, px, py) - calculates angle between
        curves identified by GeoId1 and GeoId2 at point (x,y). The point must be
        on intersection of the curves, otherwise the result may be useless (except
        line-to-line, where (0,0) is OK). Returned value is in radians.
        """
        ...

    def isPointOnCurve(self, GeoIdCurve: int, x: float, y: float, /) -> bool:
        """
        isPointOnCurve(GeoIdCurve, float x, float y) -> bool - tests if the point (x,y)
        geometrically lies on a curve (e.g. ellipse). It treats lines as infinite,
        arcs as full circles/ellipses/etc.
        """
        ...

    def calculateConstraintError(self, index: int, /) -> float:
        """
        calculateConstraintError(index) - calculates the error function of the
        constraint identified by its index and returns the signed error value.
        The error value roughly corresponds to by how much the constraint is
        violated. If the constraint internally has more than one error function,
        the returned value is RMS of all errors (sign is lost in this case).
        """
        ...

    def changeConstraintsLocking(self, bLock: bool, /) -> None:
        """
        changeConstraintsLocking(bLock) - locks or unlocks all tangent and
        perpendicular constraints. (Constraint locking prevents it from
        flipping to another valid configuration, when e.g. external geometry
        is updated from outside.) The sketch solve is not triggered by the
        function, but the SketchObject is touched (a recompute will be
        necessary). The geometry should not be affected by the function.

        The bLock argument specifies, what to do. If true, all constraints
        are unlocked and locked again. If false, all tangent and perp.
        constraints are unlocked.
        """
        ...

    def getGeometryWithDependentParameters(self) -> List[Tuple[int, int]]:
        """
        getGeometryWithDependentParameters - returns a list of geoid posid pairs
        with all the geometry element edges and vertices which the solver regards
        as being dependent on other parameters.
        """
        ...

    def autoconstraint(self) -> None:
        """
        Automatic sketch constraining algorithm.
        """
        ...

    def detectMissingPointOnPointConstraints(self) -> None:
        """
        Detects missing Point On Point Constraints. The detect step just identifies possible missing constraints.
        The result may be retrieved or applied using the corresponding Get / Make methods.
        """
        ...

    def analyseMissingPointOnPointCoincident(self) -> None:
        """
        Analyses the already detected missing Point On Point Constraints to detect endpoint tangency/perpendicular.
        The result may be retrieved or applied using the corresponding Get / Make methods.
        """
        ...

    def detectMissingVerticalHorizontalConstraints(self) -> None:
        """
        Detects missing Horizontal/Vertical Constraints. The detect step just identifies possible missing constraints.
        The result may be retrieved or applied using the corresponding Get / Make methods.
        """
        ...

    def detectMissingEqualityConstraints(self) -> None:
        """
        Detects missing Equality Constraints. The detect step just identifies possible missing constraints.
        The result may be retrieved or applied using the corresponding Get / Make methods.
        """
        ...

    def makeMissingPointOnPointCoincident(self, arg: bool, /) -> None:
        """
        Applies the detected / set Point On Point coincident constraints. If the argument is True, then solving and redundant removal is done after each individual addition.
        """
        ...

    def makeMissingVerticalHorizontal(self, arg: bool, /) -> None:
        """
        Applies the detected / set Vertical/Horizontal constraints. If the argument is True, then solving and redundant removal is done after each individual addition.
        """
        ...

    def makeMissingEquality(self, arg: bool, /) -> None:
        """
        Applies the detected / set Equality constraints. If the argument is True, then solving and redundant removal is done after each individual addition.
        """
        ...

    @constmethod
    @no_args
    def evaluateConstraints(self) -> bool:
        """
        Check for constraints with invalid indexes. Returns True if invalid constraints are found, False otherwise.
        """
        ...

    @no_args
    def validateConstraints(self) -> None:
        """
        Removes constraints with invalid indexes.
        """
        ...

    def autoRemoveRedundants(self, arg: bool, /) -> None:
        """
        Removes constraints currently detected as redundant by the solver. If the argument is True, then the geometry is updated after solving.
        """
        ...

    def toPythonCommands(self) -> None:
        """
        Prints the commands that should be executed to recreate the Geometry and Constraints of the present sketch (excluding any External Geometry).
        """
        ...

    def setGeometryId():
        """
        Sets the GeometryId of the SketchGeometryExtension of the geometry with the provided GeoId
        """
        ...

    def setGeometryIds(GeoIdsToIds: List[Tuple[int, int]], /):
        """
        Sets the GeometryId of the SketchGeometryExtension of the geometries with the provided GeoIds
        Expects a list of pairs (GeoId, id)
        """

    def getGeometryId():
        """
        Gets the GeometryId of the SketchGeometryExtension of the geometry with the provided GeoId
        """
        ...
