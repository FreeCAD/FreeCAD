import FreeCAD as App
import Part
import Path
import numpy
import math

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def checkForBlindHole(baseshape, selectedFace):
    """
    check for blind holes, returns the bottom face if found, none
    if the hole is a thru-hole
    """
    circularFaces = [
        f
        for f in baseshape.Faces
        if len(f.OuterWire.Edges) == 1
        and type(f.OuterWire.Edges[0].Curve) == Part.Circle
    ]

    circularFaceEdges = [f.OuterWire.Edges[0] for f in circularFaces]
    commonedges = [
        i for i in selectedFace.Edges for x in circularFaceEdges if i.isSame(x)
    ]

    bottomface = None
    for f in circularFaces:
        for e in f.Edges:
            for i in commonedges:
                if e.isSame(i):
                    bottomface = f
                break

    return bottomface


def isDrillableCylinder(obj, candidate, tooldiameter=None, vector=App.Vector(0, 0, 1)):
    """
    checks if a candidate cylindrical face is drillable
    """

    matchToolDiameter = tooldiameter is not None
    matchVector = vector is not None

    Path.Log.debug(
        "\n match tool diameter {} \n match vector {}".format(
            matchToolDiameter, matchVector
        )
    )

    def raisedFeature(obj, candidate):
        # check if the cylindrical 'lids' are inside the base
        # object.  This eliminates extruded circles but allows
        # actual holes.

        startLidCenter = App.Vector(
            candidate.BoundBox.Center.x,
            candidate.BoundBox.Center.y,
            candidate.BoundBox.ZMax,
        )

        endLidCenter = App.Vector(
            candidate.BoundBox.Center.x,
            candidate.BoundBox.Center.y,
            candidate.BoundBox.ZMin,
        )

        return obj.isInside(startLidCenter, 1e-6, False) or obj.isInside(
            endLidCenter, 1e-6, False
        )

    def getSeam(candidate):
        # Finds the vertical seam edge in a cylinder

        for e in candidate.Edges:
            if isinstance(e.Curve, Part.Line):  # found the seam
                return e

    if not candidate.ShapeType == "Face":
        raise TypeError("expected a Face")

    if not isinstance(candidate.Surface, Part.Cylinder):
        raise TypeError("expected a cylinder")

    if len(candidate.Edges) != 3:
        raise TypeError("cylinder does not have 3 edges.  Not supported yet")

    if raisedFeature(obj, candidate):
        Path.Log.debug("The cylindrical face is a raised feature")
        return False

    if not matchToolDiameter and not matchVector:
        return True

    if matchToolDiameter and tooldiameter / 2 > candidate.Surface.Radius:
        Path.Log.debug("The tool is larger than the target")
        return False

    bottomface = checkForBlindHole(obj, candidate)
    Path.Log.track("candidate is a blind hole")

    if (
        bottomface is not None and matchVector
    ):  # blind holes only drillable at exact vector
        result = compareVecs(bottomface.normalAt(0, 0), vector, exact=True)
        Path.Log.track(result)
        return result

    elif matchVector and not (compareVecs(getSeam(candidate).Curve.Direction, vector)):
        Path.Log.debug("The feature is not aligned with the given vector")
        return False
    else:
        return True


def isDrillableFace(obj, candidate, tooldiameter=None, vector=App.Vector(0, 0, 1)):
    """
    checks if a flat face or edge is drillable
    """
    matchToolDiameter = tooldiameter is not None
    matchVector = vector is not None
    Path.Log.debug(
        "\n match tool diameter {} \n match vector {}".format(
            matchToolDiameter, matchVector
        )
    )

    if not type(candidate.Surface) == Part.Plane:
        Path.Log.debug("Drilling on non-planar faces not supported")
        return False

    if (
        len(candidate.Edges) == 1 and type(candidate.Edges[0].Curve) == Part.Circle
    ):  # Regular circular face
        Path.Log.debug("Face is circular - 1 edge")
        edge = candidate.Edges[0]
    elif (
        len(candidate.Edges) == 2
        and type(candidate.Edges[0].Curve) == Part.Circle
        and type(candidate.Edges[1].Curve) == Part.Circle
    ):  # process a donut
        Path.Log.debug("Face is a donut - 2 edges")
        e1 = candidate.Edges[0]
        e2 = candidate.Edges[1]
        edge = e1 if e1.Curve.Radius < e2.Curve.Radius else e2
    else:
        Path.Log.debug(
            "expected a Face with one or two circular edges got a face with {} edges".format(
                len(candidate.Edges)
            )
        )
        return False
    if vector is not None:  # Check for blind hole alignment
        if not compareVecs(candidate.normalAt(0, 0), vector, exact=True):
            Path.Log.debug("Vector not aligned")
            return False
    if matchToolDiameter and edge.Curve.Radius < tooldiameter / 2:
        Path.Log.debug("Failed diameter check")
        return False
    else:
        Path.Log.debug("Face is drillable")
        return True


def isDrillableEdge(
    obj, candidate, tooldiameter=None, vector=App.Vector(0, 0, 1), allowPartial=False
):
    """
    checks if an edge is drillable
    """

    matchToolDiameter = tooldiameter is not None
    matchVector = vector is not None
    Path.Log.debug(
        "\n match tool diameter {} \n match vector {}".format(
            matchToolDiameter, matchVector
        )
    )

    edge = candidate
    if not (isinstance(edge.Curve, Part.Circle)):
        Path.Log.debug("expected a circular edge")
        return False

    if isinstance(edge.Curve, Part.Circle):
        if not (allowPartial or edge.isClosed()):
            Path.Log.debug("expected a closed circular edge or allow partial")
            return False

    if not hasattr(edge.Curve, "Radius"):
        Path.Log.debug("The Feature edge has no radius - Ellipse.")
        return False

    if not matchToolDiameter and not matchVector:
        return True

    if matchToolDiameter and tooldiameter / 2 > edge.Curve.Radius:
        Path.Log.debug("The tool is larger than the target")
        return False

    if matchVector and not (compareVecs(edge.Curve.Axis, vector)):
        Path.Log.debug("The feature is not aligned with the given vector")
        return False
    else:
        return True


def isDrillable(
    obj, candidate, tooldiameter=None, vector=App.Vector(0, 0, 1), allowPartial=False
):
    """
    Checks candidates to see if they can be drilled at the given vector.
    Candidates can be either faces - circular or cylindrical or circular edges.
    The tooldiameter can be optionally passed.  if passed, the check will return
    False for any holes smaller than the tooldiameter.

    vector defaults to (0,0,1) which aligns with the Z axis.  By default will return False
    for any candidate not drillable in this orientation.  Pass 'None' to vector to test whether
    the hole is drillable at any orientation.

    allowPartial will permit selecting partial circular arcs manually.

    obj=Shape
    candidate = Face or Edge
    tooldiameter=float
    vector=App.Vector or None
    allowPartial boolean

    """
    Path.Log.debug(
        "obj: {} candidate: {} tooldiameter {} vector {}".format(
            obj, candidate, tooldiameter, vector
        )
    )

    if list == type(obj):
        for shape in obj:
            if isDrillable(shape, candidate, tooldiameter, vector):
                return (True, shape)
        return (False, None)

    if candidate.ShapeType not in ["Face", "Edge"]:
        raise TypeError("expected a Face or Edge. Got a {}".format(candidate.ShapeType))

    try:
        if candidate.ShapeType == "Face":
            if isinstance(candidate.Surface, Part.Cylinder):
                return isDrillableCylinder(obj, candidate, tooldiameter, vector)
            else:
                return isDrillableFace(obj, candidate, tooldiameter, vector)
        if candidate.ShapeType == "Edge":
            return isDrillableEdge(obj, candidate, tooldiameter, vector, allowPartial)
        else:
            return False

    except TypeError as e:
        Path.Log.debug(e)
        return False
        # raise TypeError("{}".format(e))


def compareVecs(vec1, vec2, exact=False):
    """
    compare the two vectors to see if they are aligned for drilling.
    if exact is True, vectors must match direction. Otherwise,
    alignment can indicate the vectors are the same or exactly opposite
    """

    angle = vec1.getAngle(vec2)
    angle = 0 if math.isnan(angle) else math.degrees(angle)
    Path.Log.debug("vector angle: {}".format(angle))
    if exact:
        return numpy.isclose(angle, 0, rtol=1e-05, atol=1e-06)
    else:
        return numpy.isclose(angle, 0, rtol=1e-05, atol=1e-06) or numpy.isclose(
            angle, 180, rtol=1e-05, atol=1e-06
        )


def getDrillableTargets(obj, ToolDiameter=None, vector=App.Vector(0, 0, 1)):
    """
    Returns a list of tuples for drillable subelements from the given object
    [(obj,'Face1'),(obj,'Face3')]

    Finds cylindrical faces that are larger than the tool diameter (if provided) and
    oriented with the vector.  If vector is None, all drillables are returned

    """

    shp = obj.Shape

    results = []
    for i in range(1, len(shp.Faces)+1):
        fname = "Face{}".format(i)
        Path.Log.debug(fname)
        candidate = obj.getSubObject(fname)

        if not isinstance(candidate.Surface, Part.Cylinder):
            continue

        try:
            drillable = isDrillable(
                shp, candidate, tooldiameter=ToolDiameter, vector=vector
            )
            Path.Log.debug("fname: {} : drillable {}".format(fname, drillable))
        except Exception as e:
            Path.Log.debug(e)
            continue

        if drillable:
            results.append((obj, fname))

    return results
