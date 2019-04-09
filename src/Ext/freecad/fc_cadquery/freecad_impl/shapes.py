"""
    Copyright (C) 2011-2015  Parametric Products Intellectual Holdings, LLC

    This file is part of CadQuery.

    CadQuery is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    CadQuery is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; If not, see <http://www.gnu.org/licenses/>

    Wrapper Classes for FreeCAD
    These classes provide a stable interface for 3d objects,
    independent of the FreeCAD interface.

    Future work might include use of pythonOCC, OCC, or even
    another CAD kernel directly, so this interface layer is quite important.

    Funny, in java this is one of those few areas where i'd actually spend the time
    to make an interface and an implementation, but for new these are just rolled together

    This interface layer provides three distinct values:

        1. It allows us to avoid changing key api points if we change underlying implementations.
           It would be a disaster if script and plugin authors had to change models because we
           changed implementations

        2. Allow better documentation.  One of the reasons FreeCAD is no more popular is because
           its docs are terrible.  This allows us to provide good documentation via docstrings
           for each wrapper

        3. Work around bugs. there are a quite a feb bugs in free this layer allows fixing them

        4. allows for enhanced functionality.  Many objects are missing features we need. For example
           we need a 'forConstruction' flag on the Wire object.  this allows adding those kinds of things

        5. allow changing interfaces when we'd like.  there are  few cases where the FreeCAD api is not
           very user friendly: we like to change those when necessary. As an example, in the FreeCAD api,
           all factory methods are on the 'Part' object, but it is very useful to know what kind of
           object each one returns, so these are better grouped by the type of object they return.
           (who would know that Part.makeCircle() returns an Edge, but Part.makePolygon() returns a Wire ?
"""
from cadquery import Vector, BoundBox
import FreeCAD
import Part as FreeCADPart


class Shape(object):
    """
        Represents a shape in the system.
        Wrappers the FreeCAD api
    """

    def __init__(self, obj):
        self.wrapped = obj
        self.forConstruction = False

        # Helps identify this solid through the use of an ID
        self.label = ""

    @classmethod
    def cast(cls, obj, forConstruction=False):
        "Returns the right type of wrapper, given a FreeCAD object"
        s = obj.ShapeType
        if type(obj) == FreeCAD.Base.Vector:
            return Vector(obj)
        tr = None

        # TODO: there is a clever way to do this i'm sure with a lookup
        # but it is not a perfect mapping, because we are trying to hide
        # a bit of the complexity of Compounds in FreeCAD.
        if s == 'Vertex':
            tr = Vertex(obj)
        elif s == 'Edge':
            tr = Edge(obj)
        elif s == 'Wire':
            tr = Wire(obj)
        elif s == 'Face':
            tr = Face(obj)
        elif s == 'Shell':
            tr = Shell(obj)
        elif s == 'Solid':
            tr = Solid(obj)
        elif s == 'Compound':
            #compound of solids, lets return a solid instead
            if len(obj.Solids) > 1:
                tr = Solid(obj)
            elif len(obj.Solids) == 1:
                tr = Solid(obj.Solids[0])
            elif len(obj.Wires) > 0:
                tr = Wire(obj)
            else:
                tr = Compound(obj)
        else:
            raise ValueError("cast:unknown shape type %s" % s)

        tr.forConstruction = forConstruction
        return tr

    # TODO: all these should move into the exporters folder.
    # we don't need a bunch of exporting code stored in here!
    #
    def exportStl(self, fileName, tolerance=0.1):
        self.wrapped.exportStl(fileName, tolerance)

    def exportStep(self, fileName):
        self.wrapped.exportStep(fileName)

    def exportShape(self, fileName, fileFormat, tolerance=0.1):
        if fileFormat == ExportFormats.STL:
            self.wrapped.exportStl(fileName, tolerance)
        elif fileFormat == ExportFormats.BREP:
            self.wrapped.exportBrep(fileName)
        elif fileFormat == ExportFormats.STEP:
            self.wrapped.exportStep(fileName)
        elif fileFormat == ExportFormats.AMF:
            # not built into FreeCAD
            #TODO: user selected tolerance
            tess = self.wrapped.tessellate(0.1)
            aw = amfUtils.AMFWriter(tess)
            aw.writeAmf(fileName)
        elif fileFormat == ExportFormats.IGES:
            self.wrapped.exportIges(fileName)
        else:
            raise ValueError("Unknown export format: %s" % format)

    def geomType(self):
        """
            Gets the underlying geometry type
            :return: a string according to the geometry type.

            Implementations can return any values desired, but the
            values the user uses in type filters should correspond to these.

            As an example, if a user does::

                CQ(object).faces("%mytype")

            The expectation is that the geomType attribute will return 'mytype'

            The return values depend on the type of the shape:

            Vertex:  always 'Vertex'
            Edge:   LINE, ARC, CIRCLE, SPLINE
            Face:   PLANE, SPHERE, CONE
            Solid:  'Solid'
            Shell:  'Shell'
            Compound: 'Compound'
            Wire:   'Wire'
        """
        return self.wrapped.ShapeType

    def isType(self, obj, strType):
        """
            Returns True if the shape is the specified type, false otherwise

            contrast with ShapeType, which will raise an exception
            if the provide object is not a shape at all
        """
        if hasattr(obj, 'ShapeType'):
            return obj.ShapeType() == strType
        else:
            return False

    def hashCode(self):
        return self.wrapped.hashCode()

    def isNull(self):
        return self.wrapped.isNull()

    def isSame(self, other):
        return self.wrapped.isSame(other.wrapped)

    def isEqual(self, other):
        return self.wrapped.isEqual(other.wrapped)

    def isValid(self):
        return self.wrapped.isValid()

    def BoundingBox(self, tolerance=0.1):
        self.wrapped.tessellate(tolerance)
        return BoundBox(self.wrapped.BoundBox)

    def mirror(self, mirrorPlane="XY", basePointVector=(0, 0, 0)):
        if mirrorPlane == "XY" or mirrorPlane== "YX":
            mirrorPlaneNormalVector = FreeCAD.Base.Vector(0, 0, 1)
        elif mirrorPlane == "XZ" or mirrorPlane == "ZX":
            mirrorPlaneNormalVector = FreeCAD.Base.Vector(0, 1, 0)
        elif mirrorPlane == "YZ" or mirrorPlane == "ZY":
            mirrorPlaneNormalVector = FreeCAD.Base.Vector(1, 0, 0)

        if type(basePointVector) == tuple:
            basePointVector = Vector(basePointVector)

        return Shape.cast(self.wrapped.mirror(basePointVector.wrapped, mirrorPlaneNormalVector))

    def Center(self):
        # A Part.Shape object doesn't have the CenterOfMass function, but it's wrapped Solid(s) does
        if isinstance(self.wrapped, FreeCADPart.Shape):
            # If there are no Solids, we're probably dealing with a Face or something similar
            if len(self.Solids()) == 0:
                return Vector(self.wrapped.CenterOfMass)
            elif len(self.Solids()) == 1:
                return Vector(self.Solids()[0].wrapped.CenterOfMass)
            elif len(self.Solids()) > 1:
                return self.CombinedCenter(self.Solids())
        elif isinstance(self.wrapped, FreeCADPart.Solid):
            return Vector(self.wrapped.CenterOfMass)
        else:
            raise ValueError("Cannot find the center of %s object type" % str(type(self.Solids()[0].wrapped)))

    def CenterOfBoundBox(self, tolerance = 0.1):
        self.wrapped.tessellate(tolerance)
        if isinstance(self.wrapped, FreeCADPart.Shape):
            # If there are no Solids, we're probably dealing with a Face or something similar
            if len(self.Solids()) == 0:
                return Vector(self.wrapped.BoundBox.Center)
            elif len(self.Solids()) == 1:
                return Vector(self.Solids()[0].wrapped.BoundBox.Center)
            elif len(self.Solids()) > 1:
                return self.CombinedCenterOfBoundBox(self.Solids())
        elif isinstance(self.wrapped, FreeCADPart.Solid):
            return Vector(self.wrapped.BoundBox.Center)
        else:
            raise ValueError("Cannot find the center(BoundBox's) of %s object type" % str(type(self.Solids()[0].wrapped)))

    @staticmethod
    def CombinedCenter(objects):
        """
        Calculates the center of mass of multiple objects.

        :param objects: a list of objects with mass
        """
        total_mass = sum(Shape.computeMass(o) for o in objects)
        weighted_centers = [o.wrapped.CenterOfMass.multiply(Shape.computeMass(o)) for o in objects]

        sum_wc = weighted_centers[0]
        for wc in weighted_centers[1:] :
            sum_wc = sum_wc.add(wc)

        return Vector(sum_wc.multiply(1./total_mass))

    @staticmethod
    def computeMass(object):
        """
        Calculates the 'mass' of an object. in FreeCAD < 15, all objects had a mass.
        in FreeCAD >=15, faces no longer have mass, but instead have area.
        """
        if object.wrapped.ShapeType == 'Face':
          return object.wrapped.Area
        else:
          return object.wrapped.Mass

    @staticmethod
    def CombinedCenterOfBoundBox(objects, tolerance = 0.1):
        """
        Calculates the center of BoundBox of multiple objects.

        :param objects: a list of objects with mass 1
        """
        total_mass = len(objects)

        weighted_centers = []
        for o in objects:
            o.wrapped.tessellate(tolerance)
            weighted_centers.append(o.wrapped.BoundBox.Center.multiply(1.0))

        sum_wc = weighted_centers[0]
        for wc in weighted_centers[1:] :
            sum_wc = sum_wc.add(wc)

        return Vector(sum_wc.multiply(1./total_mass))

    def Closed(self):
        return self.wrapped.Closed

    def ShapeType(self):
        return self.wrapped.ShapeType

    def Vertices(self):
        return [Vertex(i) for i in self.wrapped.Vertexes]

    def Edges(self):
        return [Edge(i) for i in self.wrapped.Edges]

    def Compounds(self):
        return [Compound(i) for i in self.wrapped.Compounds]

    def Wires(self):
        return [Wire(i) for i in self.wrapped.Wires]

    def Faces(self):
        return [Face(i) for i in self.wrapped.Faces]

    def Shells(self):
        return [Shell(i) for i in self.wrapped.Shells]

    def Solids(self):
        return [Solid(i) for i in self.wrapped.Solids]

    def Area(self):
        """
        Returns the area of a shape, but only if it is a face
        """
        if self.wrapped.ShapeType == 'Face':
          return self.wrapped.Area
        else:
          raise ValueError("shape type must be 'Face' to calculate the area")

    def Length(self):
        return self.wrapped.Length

    def rotate(self, startVector, endVector, angleDegrees):
        """
        Rotates a shape around an axis
        :param startVector: start point of rotation axis  either a 3-tuple or a Vector
        :param endVector:  end point of rotation axis, either a 3-tuple or a Vector
        :param angleDegrees:  angle to rotate, in degrees
        :return: a copy of the shape, rotated
        """
        if type(startVector) == tuple:
            startVector = Vector(startVector)

        if type(endVector) == tuple:
            endVector = Vector(endVector)

        tmp = self.wrapped.copy()
        tmp.rotate(startVector.wrapped, endVector.wrapped, angleDegrees)
        return Shape.cast(tmp)

    def translate(self, vector):

        if type(vector) == tuple:
            vector = Vector(vector)
        tmp = self.wrapped.copy()
        tmp.translate(vector.wrapped)
        return Shape.cast(tmp)

    def scale(self, factor):
        tmp = self.wrapped.copy()
        tmp.scale(factor)
        return Shape.cast(tmp)

    def copy(self):
        return Shape.cast(self.wrapped.copy())

    def transformShape(self, tMatrix):
        """
            tMatrix is a matrix object.
            returns a copy of the object, transformed by the provided matrix,
            with all objects keeping their type
        """
        tmp = self.wrapped.copy()
        tmp.transformShape(tMatrix)
        r = Shape.cast(tmp)
        r.forConstruction = self.forConstruction
        return r

    def transformGeometry(self, tMatrix):
        """
            tMatrix is a matrix object.

            returns a copy of the object, but with geometry transformed instead of just
            rotated.

            WARNING: transformGeometry will sometimes convert lines and circles to splines,
            but it also has the ability to handle skew and stretching transformations.

            If your transformation is only translation and rotation, it is safer to use transformShape,
            which doesn't change the underlying type of the geometry, but cannot handle skew transformations
        """
        tmp = self.wrapped.copy()
        tmp = tmp.transformGeometry(tMatrix)
        return Shape.cast(tmp)

    def __hash__(self):
        return self.wrapped.hashCode()


class Vertex(Shape):
    """
    A Single Point in Space
    """

    def __init__(self, obj, forConstruction=False):
        """
            Create a vertex from a FreeCAD Vertex
        """
        self.wrapped = obj
        self.forConstruction = forConstruction
        self.X = obj.X
        self.Y = obj.Y
        self.Z = obj.Z

         # Helps identify this solid through the use of an ID
        self.label = ""

    def toTuple(self):
        return (self.X, self.Y, self.Z)

    def Center(self):
        """
            The center of a vertex is itself!
        """
        return Vector(self.wrapped.Point)


class Edge(Shape):
    """
    A trimmed curve that represents the border of a face
    """

    def __init__(self, obj):
        """
            An Edge
        """
        self.wrapped = obj
        # self.startPoint = None
        # self.endPoint = None

        self.edgetypes = {
            FreeCADPart.ArcOfCircle: 'ARC',
            FreeCADPart.Circle: 'CIRCLE'
        }

        if hasattr(FreeCADPart,"Line"):
            self.edgetypes[FreeCADPart.Line] = 'LINE'

        if hasattr(FreeCADPart,"LineSegment"):
            self.edgetypes[FreeCADPart.LineSegment] = 'LINE'

         # Helps identify this solid through the use of an ID
        self.label = ""

    def geomType(self):
        t = type(self.wrapped.Curve)
        if t in self.edgetypes:
            return self.edgetypes[t]
        else:
            return "Unknown Edge Curve Type: %s" % str(t)

    def startPoint(self):
        """

            :return: a vector representing the start point of this edge

            Note, circles may have the start and end points the same
        """
        # work around freecad bug where valueAt is unreliable
        curve = self.wrapped.Curve
        return Vector(curve.value(self.wrapped.ParameterRange[0]))

    def endPoint(self):
        """

            :return: a vector representing the end point of this edge.

            Note, circles may have the start and end points the same

        """
        # warning: easier syntax in freecad of <Edge>.valueAt(<Edge>.ParameterRange[1]) has
        # a bug with curves other than arcs, but using the underlying curve directly seems to work
        # that's the solution i'm using below
        curve = self.wrapped.Curve
        v = Vector(curve.value(self.wrapped.ParameterRange[1]))
        return v

    def tangentAt(self, locationVector=None):
        """
        Compute tangent vector at the specified location.
        :param locationVector: location to use. Use the center point if None
        :return: tangent vector
        """
        if locationVector is None:
            locationVector = self.Center()

        p = self.wrapped.Curve.parameter(locationVector.wrapped)
        return Vector(self.wrapped.tangentAt(p))

    @classmethod
    def makeCircle(cls, radius, pnt=(0, 0, 0), dir=(0, 0, 1), angle1=360.0, angle2=360):
        center = Vector(pnt)
        normal = Vector(dir)
        return Edge(FreeCADPart.makeCircle(radius, center.wrapped, normal.wrapped, angle1, angle2))

    @classmethod
    def makeSpline(cls, listOfVector):
        """
        Interpolate a spline through the provided points.
        :param cls:
        :param listOfVector: a list of Vectors that represent the points
        :return: an Edge
        """
        vecs = [v.wrapped for v in listOfVector]

        spline = FreeCADPart.BSplineCurve()
        spline.interpolate(vecs, False)
        return Edge(spline.toShape())

    @classmethod
    def makeThreePointArc(cls, v1, v2, v3):
        """
        Makes a three point arc through the provided points
        :param cls:
        :param v1: start vector
        :param v2: middle vector
        :param v3: end vector
        :return: an edge object through the three points
        """
        arc = FreeCADPart.Arc(v1.wrapped, v2.wrapped, v3.wrapped)
        e = Edge(arc.toShape())
        return e  # arcane and undocumented, this creates an Edge object

    @classmethod
    def makeLine(cls, v1, v2):
        """
            Create a line between two points
            :param v1: Vector that represents the first point
            :param v2: Vector that represents the second point
            :return: A linear edge between the two provided points
        """
        return Edge(FreeCADPart.makeLine(v1.toTuple(), v2.toTuple()))


class Wire(Shape):
    """
    A series of connected, ordered Edges, that typically bounds a Face
    """

    def __init__(self, obj):
        """
            A Wire
        """
        self.wrapped = obj

         # Helps identify this solid through the use of an ID
        self.label = ""

    @classmethod
    def combine(cls, listOfWires):
        """
        Attempt to combine a list of wires into a new wire.
        the wires are returned in a list.
        :param cls:
        :param listOfWires:
        :return:
        """
        return Shape.cast(FreeCADPart.Wire([w.wrapped for w in listOfWires]))

    @classmethod
    def assembleEdges(cls, listOfEdges):
        """
            Attempts to build a wire that consists of the edges in the provided list
            :param cls:
            :param listOfEdges: a list of Edge objects
            :return: a wire with the edges assembled
        """
        fCEdges = [a.wrapped for a in listOfEdges]

        wa = Wire(FreeCADPart.Wire(fCEdges))
        return wa

    @classmethod
    def makeCircle(cls, radius, center, normal):
        """
            Makes a Circle centered at the provided point, having normal in the provided direction
            :param radius: floating point radius of the circle, must be > 0
            :param center: vector representing the center of the circle
            :param normal: vector representing the direction of the plane the circle should lie in
            :return:
        """
        w = Wire(FreeCADPart.Wire([FreeCADPart.makeCircle(radius, center.wrapped, normal.wrapped)]))
        return w

    @classmethod
    def makePolygon(cls, listOfVertices, forConstruction=False):
        # convert list of tuples into Vectors.
        w = Wire(FreeCADPart.makePolygon([i.wrapped for i in listOfVertices]))
        w.forConstruction = forConstruction
        return w

    @classmethod
    def makeHelix(cls, pitch, height, radius, angle=0, lefthand=False, heightstyle=False):
        """
        Make a helix along +z axis
        :param pitch: displacement of 1 turn measured along surface.
        :param height: full length of helix surface (measured sraight along surface's face)
        :param radius: starting radius of helix
        :param angle: if > 0, conical surface is used instead of a cylindrical. (angle < 0 not supported)
        :param lefthand: if True, helix direction is reversed
        :param heightstyle: if True, pitch and height are measured parallel to z-axis
        """
        # FreeCAD doc: https://www.freecadweb.org/wiki/Part_API (search for makeHelix)
        return Wire(FreeCADPart.makeHelix(pitch, height, radius, angle, lefthand, heightstyle))

    def clean(self):
        """This method is not implemented yet."""
        return self

class Face(Shape):
    """
    a bounded surface that represents part of the boundary of a solid
    """
    def __init__(self, obj):

        self.wrapped = obj

        self.facetypes = {
            # TODO: bezier,bspline etc
            FreeCADPart.Plane: 'PLANE',
            FreeCADPart.Sphere: 'SPHERE',
            FreeCADPart.Cone: 'CONE'
        }

         # Helps identify this solid through the use of an ID
        self.label = ""

    def geomType(self):
        t = type(self.wrapped.Surface)
        if t in self.facetypes:
            return self.facetypes[t]
        else:
            return "Unknown Face Surface Type: %s" % str(t)

    def normalAt(self, locationVector=None):
        """
            Computes the normal vector at the desired location on the face.

            :returns: a  vector representing the direction
            :param locationVector: the location to compute the normal at. If none, the center of the face is used.
            :type locationVector: a vector that lies on the surface.
        """
        if locationVector == None:
            locationVector = self.Center()
        (u, v) = self.wrapped.Surface.parameter(locationVector.wrapped)

        return Vector(self.wrapped.normalAt(u, v).normalize())

    @classmethod
    def makePlane(cls, length, width, basePnt=(0, 0, 0), dir=(0, 0, 1)):
        basePnt = Vector(basePnt)
        dir = Vector(dir)
        return Face(FreeCADPart.makePlane(length, width, basePnt.wrapped, dir.wrapped))

    @classmethod
    def makeRuledSurface(cls, edgeOrWire1, edgeOrWire2, dist=None):
        """
        'makeRuledSurface(Edge|Wire,Edge|Wire) -- Make a ruled surface
        Create a ruled surface out of two edges or wires. If wires are used then
        these must have the same
        """
        return Shape.cast(FreeCADPart.makeRuledSurface(edgeOrWire1.wrapped, edgeOrWire2.wrapped))

    def cut(self, faceToCut):
        "Remove a face from another one"
        return Shape.cast(self.wrapped.cut(faceToCut.wrapped))

    def fuse(self, faceToJoin):
        return Shape.cast(self.wrapped.fuse(faceToJoin.wrapped))

    def intersect(self, faceToIntersect):
        """
        computes the intersection between the face and the supplied one.
        The result could be a face or a compound of faces
        """
        return Shape.cast(self.wrapped.common(faceToIntersect.wrapped))


class Shell(Shape):
    """
    the outer boundary of a surface
    """
    def __init__(self, wrapped):
        """
            A Shell
        """
        self.wrapped = wrapped

         # Helps identify this solid through the use of an ID
        self.label = ""

    @classmethod
    def makeShell(cls, listOfFaces):
        return Shell(FreeCADPart.makeShell([i.obj for i in listOfFaces]))


class Solid(Shape):
    """
    a single solid
    """
    def __init__(self, obj):
        """
            A Solid
        """
        self.wrapped = obj

         # Helps identify this solid through the use of an ID
        self.label = ""

    @classmethod
    def isSolid(cls, obj):
        """
            Returns true if the object is a FreeCAD solid, false otherwise
        """
        if hasattr(obj, 'ShapeType'):
            if obj.ShapeType == 'Solid' or \
                    (obj.ShapeType == 'Compound' and len(obj.Solids) > 0):
                return True
        return False

    @classmethod
    def makeBox(cls, length, width, height, pnt=Vector(0, 0, 0), dir=Vector(0, 0, 1)):
        """
        makeBox(length,width,height,[pnt,dir]) -- Make a box located in pnt with the dimensions (length,width,height)
        By default pnt=Vector(0,0,0) and dir=Vector(0,0,1)'
        """
        return Shape.cast(FreeCADPart.makeBox(length, width, height, pnt.wrapped, dir.wrapped))

    @classmethod
    def makeCone(cls, radius1, radius2, height, pnt=Vector(0, 0, 0), dir=Vector(0, 0, 1), angleDegrees=360):
        """
        Make a cone with given radii and height
        By default pnt=Vector(0,0,0),
        dir=Vector(0,0,1) and angle=360'
        """
        return Shape.cast(FreeCADPart.makeCone(radius1, radius2, height, pnt.wrapped, dir.wrapped, angleDegrees))

    @classmethod
    def makeCylinder(cls, radius, height, pnt=Vector(0, 0, 0), dir=Vector(0, 0, 1), angleDegrees=360):
        """
        makeCylinder(radius,height,[pnt,dir,angle]) --
        Make a cylinder with a given radius and height
        By default pnt=Vector(0,0,0),dir=Vector(0,0,1) and angle=360'
        """
        return Shape.cast(FreeCADPart.makeCylinder(radius, height, pnt.wrapped, dir.wrapped, angleDegrees))

    @classmethod
    def makeTorus(cls, radius1, radius2, pnt=None, dir=None, angleDegrees1=None, angleDegrees2=None):
        """
        makeTorus(radius1,radius2,[pnt,dir,angle1,angle2,angle]) --
        Make a torus with agiven radii and angles
        By default pnt=Vector(0,0,0),dir=Vector(0,0,1),angle1=0
        ,angle1=360 and angle=360'
        """
        return Shape.cast(FreeCADPart.makeTorus(radius1, radius2, pnt, dir, angleDegrees1, angleDegrees2))

    @classmethod
    def makeLoft(cls, listOfWire, ruled=False):
        """
            makes a loft from a list of wires
            The wires will be converted into faces when possible-- it is presumed that nobody ever actually
            wants to make an infinitely thin shell for a real FreeCADPart.
        """
        # the True flag requests building a solid instead of a shell.

        return Shape.cast(FreeCADPart.makeLoft([i.wrapped for i in listOfWire], True, ruled))

    @classmethod
    def makeWedge(cls, xmin, ymin, zmin, z2min, x2min, xmax, ymax, zmax, z2max, x2max, pnt=None, dir=None):
        """
        Make a wedge located in pnt
        By default pnt=Vector(0,0,0) and dir=Vector(0,0,1)
        """
        return Shape.cast(
            FreeCADPart.makeWedge(xmin, ymin, zmin, z2min, x2min, xmax, ymax, zmax, z2max, x2max, pnt, dir))

    @classmethod
    def makeSphere(cls, radius, pnt=None, dir=None, angleDegrees1=None, angleDegrees2=None, angleDegrees3=None):
        """
        Make a sphere with a given radius
        By default pnt=Vector(0,0,0), dir=Vector(0,0,1), angle1=0, angle2=90 and angle3=360
        """
        return Shape.cast(FreeCADPart.makeSphere(radius, pnt.wrapped, dir.wrapped, angleDegrees1, angleDegrees2, angleDegrees3))

    @classmethod
    def extrudeLinearWithRotation(cls, outerWire, innerWires, vecCenter, vecNormal, angleDegrees):
        """
            Creates a 'twisted prism' by extruding, while simultaneously rotating around the extrusion vector.

            Though the signature may appear to be similar enough to extrudeLinear to merit combining them, the
            construction methods used here are different enough that they should be separate.

            At a high level, the steps followed are:
            (1) accept a set of wires
            (2) create another set of wires like this one, but which are transformed and rotated
            (3) create a ruledSurface between the sets of wires
            (4) create a shell and compute the resulting object

            :param outerWire: the outermost wire, a cad.Wire
            :param innerWires: a list of inner wires, a list of cad.Wire
            :param vecCenter: the center point about which to rotate.  the axis of rotation is defined by
                   vecNormal, located at vecCenter. ( a cad.Vector )
            :param vecNormal: a vector along which to extrude the wires ( a cad.Vector )
            :param angleDegrees: the angle to rotate through while extruding
            :return: a cad.Solid object
        """

        # from this point down we are dealing with FreeCAD wires not cad.wires
        startWires = [outerWire.wrapped] + [i.wrapped for i in innerWires]
        endWires = []
        p1 = vecCenter.wrapped
        p2 = vecCenter.add(vecNormal).wrapped

        # make translated and rotated copy of each wire
        for w in startWires:
            w2 = w.copy()
            w2.translate(vecNormal.wrapped)
            w2.rotate(p1, p2, angleDegrees)
            endWires.append(w2)

        # make a ruled surface for each set of wires
        sides = []
        for w1, w2 in zip(startWires, endWires):
            rs = FreeCADPart.makeRuledSurface(w1, w2)
            sides.append(rs)

        #make faces for the top and bottom
        startFace = FreeCADPart.Face(startWires)
        endFace = FreeCADPart.Face(endWires)
        startFace.validate()
        endFace.validate()

        #collect all the faces from the sides
        faceList = [startFace]
        for s in sides:
            faceList.extend(s.Faces)
        faceList.append(endFace)

        shell = FreeCADPart.makeShell(faceList)
        solid = FreeCADPart.makeSolid(shell)
        return Shape.cast(solid)

    @classmethod
    def extrudeLinear(cls, outerWire, innerWires, vecNormal):
        """
            Attempt to extrude the list of wires  into a prismatic solid in the provided direction

            :param outerWire: the outermost wire
            :param innerWires: a list of inner wires
            :param vecNormal: a vector along which to extrude the wires
            :return: a Solid object

            The wires must not intersect

            Extruding wires is very non-trivial.  Nested wires imply very different geometry, and
            there are many geometries that are invalid. In general, the following conditions must be met:

            * all wires must be closed
            * there cannot be any intersecting or self-intersecting wires
            * wires must be listed from outside in
            * more than one levels of nesting is not supported reliably

            This method will attempt to sort the wires, but there is much work remaining to make this method
            reliable.
        """

        # one would think that fusing faces into a compound and then extruding would work,
        # but it doesnt-- the resulting compound appears to look right, ( right number of faces, etc),
        # but then cutting it from the main solid fails with BRep_NotDone.
        #the work around is to extrude each and then join the resulting solids, which seems to work

        #FreeCAD allows this in one operation, but others might not
        freeCADWires = [outerWire.wrapped]
        for w in innerWires:
            freeCADWires.append(w.wrapped)

        f = FreeCADPart.Face(freeCADWires)
        f.validate()
        result = f.extrude(vecNormal.wrapped)

        return Shape.cast(result)

    @classmethod
    def revolve(cls, outerWire, innerWires, angleDegrees, axisStart, axisEnd):
        """
        Attempt to revolve the list of wires into a solid in the provided direction

        :param outerWire: the outermost wire
        :param innerWires: a list of inner wires
        :param angleDegrees: the angle to revolve through.
        :type angleDegrees: float, anything less than 360 degrees will leave the shape open
        :param axisStart: the start point of the axis of rotation
        :type axisStart: tuple, a two tuple
        :param axisEnd: the end point of the axis of rotation
        :type axisEnd: tuple, a two tuple
        :return: a Solid object

        The wires must not intersect

        * all wires must be closed
        * there cannot be any intersecting or self-intersecting wires
        * wires must be listed from outside in
        * more than one levels of nesting is not supported reliably
        * the wire(s) that you're revolving cannot be centered

        This method will attempt to sort the wires, but there is much work remaining to make this method
        reliable.
        """
        freeCADWires = [outerWire.wrapped]

        for w in innerWires:
            freeCADWires.append(w.wrapped)

        f = FreeCADPart.Face(freeCADWires)
        f.validate()

        rotateCenter = FreeCAD.Base.Vector(axisStart)
        rotateAxis = FreeCAD.Base.Vector(axisEnd)

        #Convert our axis end vector into to something FreeCAD will understand (an axis specification vector)
        rotateAxis = rotateCenter.sub(rotateAxis)

        #FreeCAD wants a rotation center and then an axis to rotate around rather than an axis of rotation
        result = f.revolve(rotateCenter, rotateAxis, angleDegrees)

        return Shape.cast(result)

    @classmethod
    def sweep(cls, outerWire, innerWires, path, makeSolid=True, isFrenet=False):
        """
        Attempt to sweep the list of wires  into a prismatic solid along the provided path

        :param outerWire: the outermost wire
        :param innerWires: a list of inner wires
        :param path: The wire to sweep the face resulting from the wires over
        :return: a Solid object
        """

        # FreeCAD allows this in one operation, but others might not
        freeCADWires = [outerWire.wrapped]
        for w in innerWires:
            freeCADWires.append(w.wrapped)

        # f = FreeCADPart.Face(freeCADWires)
        wire = FreeCADPart.Wire([path.wrapped])
        result = wire.makePipeShell(freeCADWires, makeSolid, isFrenet)

        return Shape.cast(result)

    def tessellate(self, tolerance):
        return self.wrapped.tessellate(tolerance)

    def intersect(self, toIntersect):
        """
        computes the intersection between this solid and the supplied one
        The result could be a face or a compound of faces
        """
        return Shape.cast(self.wrapped.common(toIntersect.wrapped))

    def cut(self, solidToCut):
        "Remove a solid from another one"
        return Shape.cast(self.wrapped.cut(solidToCut.wrapped))

    def fuse(self, solidToJoin):
        return Shape.cast(self.wrapped.fuse(solidToJoin.wrapped))

    def clean(self):
        """Clean faces by removing splitter edges."""
        r = self.wrapped.removeSplitter()
        # removeSplitter() returns a generic Shape type, cast to actual type of object
        r = FreeCADPart.cast_to_shape(r)
        return Shape.cast(r)

    def fillet(self, radius, edgeList):
        """
        Fillets the specified edges of this solid.
        :param radius: float > 0, the radius of the fillet
        :param edgeList:  a list of Edge objects, which must belong to this solid
        :return: Filleted solid
        """
        nativeEdges = [e.wrapped for e in edgeList]
        return Shape.cast(self.wrapped.makeFillet(radius, nativeEdges))

    def chamfer(self, length, length2, edgeList):
        """
        Chamfers the specified edges of this solid.
        :param length: length > 0, the length (length) of the chamfer
        :param length2: length2 > 0, optional parameter for asymmetrical chamfer. Should be `None` if not required.
        :param edgeList:  a list of Edge objects, which must belong to this solid
        :return: Chamfered solid
        """
        nativeEdges = [e.wrapped for e in edgeList]
        # note: we prefer 'length' word to 'radius' as opposed to FreeCAD's API
        if length2:
            return Shape.cast(self.wrapped.makeChamfer(length, length2, nativeEdges))
        else:
            return Shape.cast(self.wrapped.makeChamfer(length, nativeEdges))

    def shell(self, faceList, thickness, tolerance=0.0001):
        """
            make a shelled solid of given  by removing the list of faces

        :param faceList: list of face objects, which must be part of the solid.
        :param thickness: floating point thickness. positive shells outwards, negative shells inwards
        :param tolerance: modelling tolerance of the method, default=0.0001
        :return: a shelled solid

            **WARNING**  The underlying FreeCAD implementation can very frequently have problems
            with shelling complex geometries!
        """
        nativeFaces = [f.wrapped for f in faceList]
        return Shape.cast(self.wrapped.makeThickness(nativeFaces, thickness, tolerance))


class Compound(Shape):
    """
    a collection of disconnected solids
    """

    def __init__(self, obj):
        """
            An Edge
        """
        self.wrapped = obj

         # Helps identify this solid through the use of an ID
        self.label = ""

    def Center(self):
        return self.Center()

    @classmethod
    def makeCompound(cls, listOfShapes):
        """
        Create a compound out of a list of shapes
        """
        solids = [s.wrapped for s in listOfShapes]
        c = FreeCADPart.Compound(solids)
        return Shape.cast(c)

    def fuse(self, toJoin):
        return Shape.cast(self.wrapped.fuse(toJoin.wrapped))

    def tessellate(self, tolerance):
        return self.wrapped.tessellate(tolerance)

    def clean(self):
        """This method is not implemented yet."""
        return self
