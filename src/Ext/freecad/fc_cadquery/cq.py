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
"""

import time
import math
from . import *
from . import selectors
from . import exporters

from copy import copy, deepcopy


class CQContext(object):
    """
        A shared context for modeling.

        All objects in the same CQ chain share a reference to this same object instance
        which allows for shared state when needed,
    """
    def __init__(self):
        self.pendingWires = []   # a list of wires that have been created and need to be extruded
        self.pendingEdges = []   # a list of created pending edges that need to be joined into wires
        # a reference to the first point for a set of edges.
        # Used to determine how to behave when close() is called
        self.firstPoint = None
        self.tolerance = 0.0001  # user specified tolerance


class CQ(object):
    """
    Provides enhanced functionality for a wrapped CAD primitive.

    Examples include feature selection, feature creation, 2d drawing
    using work planes, and 3d operations like fillets, shells, and splitting
    """

    def __init__(self, obj):
        """
        Construct a new CadQuery (CQ) object that wraps a CAD primitive.

        :param obj: Object to Wrap.
        :type obj: A CAD Primitive ( wire,vertex,face,solid,edge )
        """
        self.objects = []
        self.ctx = CQContext()
        self.parent = None

        if obj:  # guarded because sometimes None for internal use
            self.objects.append(obj)

    def newObject(self, objlist):
        """
        Make a new CQ object.

        :param objlist: The stack of objects to use
        :type objlist: a list of CAD primitives ( wire,face,edge,solid,vertex,etc )

        The parent of the new object will be set to the current object,
        to preserve the chain correctly.

        Custom plugins and subclasses should use this method to create new CQ objects
        correctly.
        """
        r = type(self)(None)  # create a completely blank one
        r.parent = self
        r.ctx = self.ctx  # context solid remains the same
        r.objects = list(objlist)
        return r

    def _collectProperty(self, propName):
        """
        Collects all of the values for propName,
        for all items on the stack.
        FreeCAD objects do not implement id correctly,
        so hashCode is used to ensure we don't add the same
        object multiple times.

        One weird use case is that the stack could have a solid reference object
        on it.  This is meant to be a reference to the most recently modified version
        of the context solid, whatever it is.
        """
        all = {}
        for o in self.objects:

            # tricky-- if an object is a compound of solids,
            # do not return all of the solids underneath-- typically
            # then we'll keep joining to ourself
            if propName == 'Solids' and isinstance(o, Solid) and o.ShapeType() == 'Compound':
                for i in getattr(o, 'Compounds')():
                    all[i.hashCode()] = i
            else:
                if hasattr(o, propName):
                    for i in getattr(o, propName)():
                        all[i.hashCode()] = i

        return list(all.values())

    def split(self, keepTop=False, keepBottom=False):
        """
            Splits a solid on the stack into two parts, optionally keeping the separate parts.

            :param boolean keepTop: True to keep the top, False or None to discard it
            :param boolean keepBottom: True to keep the bottom, False or None to discard it
            :raises: ValueError if keepTop and keepBottom are both false.
            :raises: ValueError if there is not a solid in the current stack or the parent chain
            :returns: CQ object with the desired objects on the stack.

            The most common operation splits a solid and keeps one half. This sample creates
            split bushing::

                #drill a hole in the side
                c = Workplane().box(1,1,1).faces(">Z").workplane().circle(0.25).cutThruAll()F
                #now cut it in half sideways
                c.faces(">Y").workplane(-0.5).split(keepTop=True)
        """

        solid = self.findSolid()

        if (not keepTop) and (not keepBottom):
            raise ValueError("You have to keep at least one half")

        maxDim = solid.BoundingBox().DiagonalLength * 10.0
        topCutBox = self.rect(maxDim, maxDim)._extrude(maxDim)
        bottomCutBox = self.rect(maxDim, maxDim)._extrude(-maxDim)

        top = solid.cut(bottomCutBox)
        bottom = solid.cut(topCutBox)

        if keepTop and keepBottom:
            # Put both on the stack, leave original unchanged.
            return self.newObject([top, bottom])
        else:
            # Put the one we are keeping on the stack, and also update the
            # context solidto the one we kept.
            if keepTop:
                solid.wrapped = top.wrapped
                return self.newObject([top])
            else:
                solid.wrapped = bottom.wrapped
                return self.newObject([bottom])

    def combineSolids(self, otherCQToCombine=None):
        """
        !!!DEPRECATED!!! use union()
        Combines all solids on the current stack, and any context object, together
        into a single object.

        After the operation, the returned solid is also the context solid.

        :param otherCQToCombine: another CadQuery to combine.
        :return: a cQ object with the resulting combined solid on the stack.

        Most of the time, both objects will contain a single solid, which is
        combined and returned on the stack of the new object.
        """
        #loop through current stack objects, and combine them
        #TODO: combine other types of objects as well, like edges and wires
        toCombine = self.solids().vals()

        if otherCQToCombine:
            for obj in otherCQToCombine.solids().vals():
                toCombine.append(obj)

        if len(toCombine) < 1:
            raise ValueError("Cannot Combine: at least one solid required!")

        #get context solid and we don't want to find our own objects
        ctxSolid = self.findSolid(searchStack=False, searchParents=True)

        if ctxSolid is None:
            ctxSolid = toCombine.pop(0)

        #now combine them all. make sure to save a reference to the ctxSolid pointer!
        s = ctxSolid
        for tc in toCombine:
            s = s.fuse(tc)

        ctxSolid.wrapped = s.wrapped
        return self.newObject([s])

    def all(self):
        """
        Return a list of all CQ objects on the stack.

        useful when you need to operate on the elements
        individually.

        Contrast with vals, which returns the underlying
        objects for all of the items on the stack
        """
        return [self.newObject([o]) for o in self.objects]

    def size(self):
        """
         Return the number of objects currently on the stack
        """
        return len(self.objects)

    def vals(self):
        """
        get the values in the current list

        :rtype: list of FreeCAD objects
        :returns: the values of the objects on the stack.

        Contrast with :py:meth:`all`, which returns CQ objects for all of the items on the stack
        """
        return self.objects

    def add(self, obj):
        """
        Adds an object or a list of objects to the stack

        :param obj: an object to add
        :type obj: a CQ object, CAD primitive, or list of CAD primitives
        :return: a CQ object with the requested operation performed

        If an CQ object, the values of that object's stack are added. If a list of cad primitives,
        they are all added. If a single CAD primitive it is added

        Used in rare cases when you need to combine the results of several CQ results
        into a single CQ object. Shelling is one common example
        """
        if type(obj) == list:
            self.objects.extend(obj)
        elif isinstance(obj, CQ):
            self.objects.extend(obj.objects)
        else:
            self.objects.append(obj)
        return self

    def val(self):
        """
        Return the first value on the stack

        :return: the first value on the stack.
        :rtype: A FreeCAD object or a SolidReference
        """
        return self.objects[0]

    def toFreecad(self):
        """
        Directly returns the wrapped FreeCAD object to cut down on the amount of boiler plate code
        needed when rendering a model in FreeCAD's 3D view.
        :return: The wrapped FreeCAD object
        :rtype A FreeCAD object or a SolidReference
        """

        return self.objects[0].wrapped

    def workplane(self, offset=0.0, invert=False, centerOption='CenterOfMass'):
        """
        Creates a new 2-D workplane, located relative to the first face on the stack.

        :param offset:  offset for the work plane in the Z direction. Default
        :param invert:  invert the Z direction from that of the face.
        :type offset: float or None=0.0
        :type invert: boolean or None=False
        :rtype: Workplane object ( which is a subclass of CQ )

        The first element on the stack must be a face, a set of
        co-planar faces or a vertex.  If a vertex, then the parent
        item on the chain immediately before the vertex must be a
        face.

        The result will be a 2-d working plane
        with a new coordinate system set up as follows:

           * The origin will be located in the *center* of the
             face/faces, if a face/faces was selected. If a vertex was
             selected, the origin will be at the vertex, and located
             on the face.
           * The Z direction will be normal to the plane of the face,computed
             at the center point.
           * The X direction will be parallel to the x-y plane. If the workplane is  parallel to
             the global x-y plane, the x direction of the workplane will co-incide with the
             global x direction.

        Most commonly, the selected face will be planar, and the workplane lies in the same plane
        of the face ( IE, offset=0).  Occasionally, it is useful to define a face offset from
        an existing surface, and even more rarely to define a workplane based on a face that is
        not planar.

        To create a workplane without first having a face, use the Workplane() method.

        Future Enhancements:
          * Allow creating workplane from planar wires
          * Allow creating workplane based on an arbitrary point on a face, not just the center.
            For now you can work around by creating a workplane and then offsetting the center
            afterwards.
        """
        def _isCoPlanar(f0, f1):
            """Test if two faces are on the same plane."""
            p0 = f0.Center()
            p1 = f1.Center()
            n0 = f0.normalAt()
            n1 = f1.normalAt()

            # test normals (direction of planes)
            if not ((abs(n0.x-n1.x) < self.ctx.tolerance) or
                    (abs(n0.y-n1.y) < self.ctx.tolerance) or
                    (abs(n0.z-n1.z) < self.ctx.tolerance)):
                return False

            # test if p1 is on the plane of f0 (offset of planes)
            return abs(n0.dot(p0.sub(p1)) < self.ctx.tolerance)

        def _computeXdir(normal):
            """
            Figures out the X direction based on the given normal.
            :param :normal The direction that's normal to the plane.
            :type :normal A Vector
            :return A vector representing the X direction.
            """
            xd = Vector(0, 0, 1).cross(normal)
            if xd.Length < self.ctx.tolerance:
                #this face is parallel with the x-y plane, so choose x to be in global coordinates
                xd = Vector(1, 0, 0)
            return xd

        if len(self.objects) > 1:
            # are all objects 'PLANE'?
            if not all(o.geomType() == 'PLANE' for o in self.objects):
                raise ValueError("If multiple objects selected, they all must be planar faces.")

            # are all faces co-planar with each other?
            if not all(_isCoPlanar(self.objects[0], f) for f in self.objects[1:]):
                raise ValueError("Selected faces must be co-planar.")

            if centerOption == 'CenterOfMass':
                center = Shape.CombinedCenter(self.objects)
            elif centerOption == 'CenterOfBoundBox':
                center = Shape.CombinedCenterOfBoundBox(self.objects)

            normal = self.objects[0].normalAt()
            xDir = _computeXdir(normal)

        else:
            obj = self.objects[0]

            if isinstance(obj, Face):
                if centerOption == 'CenterOfMass':
                        center = obj.Center()
                elif centerOption == 'CenterOfBoundBox':
                        center = obj.CenterOfBoundBox()
                normal = obj.normalAt(center)
                xDir = _computeXdir(normal)
            else:
                if hasattr(obj, 'Center'):
                    if centerOption == 'CenterOfMass':
                        center = obj.Center()
                    elif centerOption == 'CenterOfBoundBox':
                        center = obj.CenterOfBoundBox()
                    normal = self.plane.zDir
                    xDir = self.plane.xDir
                else:
                    raise ValueError("Needs a face or a vertex or point on a work plane")

        #invert if requested
        if invert:
            normal = normal.multiply(-1.0)

        #offset origin if desired
        offsetVector = normal.normalized().multiply(offset)
        offsetCenter = center.add(offsetVector)

        #make the new workplane
        plane = Plane(offsetCenter, xDir, normal)
        s = Workplane(plane)
        s.parent = self
        s.ctx = self.ctx

        #a new workplane has the center of the workplane on the stack
        return s

    def first(self):
        """
        Return the first item on the stack
        :returns: the first item on the stack.
        :rtype: a CQ object
        """
        return self.newObject(self.objects[0:1])

    def item(self, i):
        """

        Return the ith item on the stack.
        :rtype: a CQ object
        """
        return self.newObject([self.objects[i]])

    def last(self):
        """
        Return the last item on the stack.
        :rtype: a CQ object
        """
        return self.newObject([self.objects[-1]])

    def end(self):
        """
        Return the parent of this CQ element
        :rtype: a CQ object
        :raises: ValueError if there are no more parents in the chain.

        For example::

            CQ(obj).faces("+Z").vertices().end()

        will return the same as::

            CQ(obj).faces("+Z")
        """
        if self.parent:
            return self.parent
        else:
            raise ValueError("Cannot End the chain-- no parents!")

    def findSolid(self, searchStack=True, searchParents=True):
        """
        Finds the first solid object in the chain, searching from the current node
        backwards through parents until one is found.

        :param searchStack: should objects on the stack be searched first.
        :param searchParents: should parents be searched?
        :raises: ValueError if no solid is found in the current object or its parents,
            and errorOnEmpty is True

        This function is very important for chains that are modifying a single parent object,
        most often a solid.

        Most of the time, a chain defines or selects a solid, and then modifies it using workplanes
        or other operations.

        Plugin Developers should make use of this method to find the solid that should be modified,
        if the plugin implements a unary operation, or if the operation will automatically merge its
        results with an object already on the stack.
        """
        #notfound = ValueError("Cannot find a Valid Solid to Operate on!")

        if searchStack:
            for s in self.objects:
                if isinstance(s, Solid):
                    return s
                elif isinstance(s, Compound):
                    return s.Solids()

        if searchParents and self.parent is not None:
            return self.parent.findSolid(searchStack=True, searchParents=searchParents)

        return None

    def _selectObjects(self, objType, selector=None):
        """
            Filters objects of the selected type with the specified selector,and returns results

            :param objType: the type of object we are searching for
            :type objType: string: (Vertex|Edge|Wire|Solid|Shell|Compound|CompSolid)
            :return: a CQ object with the selected objects on the stack.

            **Implementation Note**: This is the base implementation of the vertices,edges,faces,
            solids,shells, and other similar selector methods.  It is a useful extension point for
            plugin developers to make other selector methods.
        """
        # A single list of all faces from all objects on the stack
        toReturn = self._collectProperty(objType)

        if selector is not None:
            # if isinstance(selector, str) or isinstance(selector, str):
            try:
                selectorObj = selectors.StringSyntaxSelector(selector)
            except:
                selectorObj = selector
            toReturn = selectorObj.filter(toReturn)

        return self.newObject(toReturn)

    def vertices(self, selector=None):
        """
        Select the vertices of objects on the stack, optionally filtering the selection. If there
        are multiple objects on the stack, the vertices of all objects are collected and a list of
        all the distinct vertices is returned.

        :param selector:
        :type selector:  None, a Selector object, or a string selector expression.
        :return: a CQ object who's stack contains  the *distinct* vertices of *all* objects on the
           current stack, after being filtered by the selector, if provided

        If there are no vertices for any objects on the current stack, an empty CQ object
        is returned

        The typical use is to select the vertices of a single object on the stack. For example::

           Workplane().box(1,1,1).faces("+Z").vertices().size()

        returns 4, because the topmost face of cube will contain four vertices. While this::

           Workplane().box(1,1,1).faces().vertices().size()

        returns 8, because a cube has a total of 8 vertices

        **Note** Circles are peculiar, they have a single vertex at the center!

        :py:class:`StringSyntaxSelector`

        """
        return self._selectObjects('Vertices', selector)

    def faces(self, selector=None):
        """
        Select the faces of objects on the stack, optionally filtering the selection. If there are
        multiple objects on the stack, the faces of all objects are collected and a list of all the
        distinct faces is returned.

        :param selector: A selector
        :type selector:  None, a Selector object, or a string selector expression.
        :return: a CQ object who's stack contains all of the *distinct* faces of *all* objects on
            the current stack, filtered by the provided selector.

        If there are no vertices for any objects on the current stack, an empty CQ object
        is returned.

        The typical use is to select the faces of a single object on the stack. For example::

           CQ(aCube).faces("+Z").size()

        returns 1, because a cube has one face with a normal in the +Z direction. Similarly::

           CQ(aCube).faces().size()

        returns 6, because a cube has a total of 6 faces, And::

            CQ(aCube).faces("|Z").size()

        returns 2, because a cube has 2 faces having normals parallel to the z direction

        See more about selectors HERE
        """
        return self._selectObjects('Faces', selector)

    def edges(self, selector=None):
        """
        Select the edges of objects on the stack, optionally filtering the selection. If there are
        multiple objects on the stack, the edges of all objects are collected and a list of all the
        distinct edges is returned.

        :param selector: A selector
        :type selector:  None, a Selector object, or a string selector expression.
        :return: a CQ object who's stack contains all of the *distinct* edges of *all* objects on
            the current stack, filtered by the provided selector.

        If there are no edges for any objects on the current stack, an empty CQ object is returned

        The typical use is to select the edges of a single object on the stack. For example::

           CQ(aCube).faces("+Z").edges().size()

        returns 4, because a cube has one face with a normal in the +Z direction. Similarly::

           CQ(aCube).edges().size()

        returns 12, because a cube has a total of 12 edges, And::

            CQ(aCube).edges("|Z").size()

        returns 4, because a cube has 4 edges parallel to the z direction

        See more about selectors HERE
        """
        return self._selectObjects('Edges', selector)

    def wires(self, selector=None):
        """
        Select the wires of objects on the stack, optionally filtering the selection. If there are
        multiple objects on the stack, the wires of all objects are collected and a list of all the
        distinct wires is returned.

        :param selector: A selector
        :type selector:  None, a Selector object, or a string selector expression.
        :return: a CQ object who's stack contains all of the *distinct* wires of *all* objects on
            the current stack, filtered by the provided selector.

        If there are no wires for any objects on the current stack, an empty CQ object is returned

        The typical use is to select the wires of a single object on the stack. For example::

           CQ(aCube).faces("+Z").wires().size()

        returns 1, because a face typically only has one outer wire

        See more about selectors HERE
        """
        return self._selectObjects('Wires', selector)

    def solids(self, selector=None):
        """
        Select the solids of objects on the stack, optionally filtering the selection. If there are
        multiple objects on the stack, the solids of all objects are collected and a list of all the
        distinct solids is returned.

        :param selector: A selector
        :type selector:  None, a Selector object, or a string selector expression.
        :return: a CQ object who's stack contains all of the *distinct* solids of *all* objects on
            the current stack, filtered by the provided selector.

        If there are no solids for any objects on the current stack, an empty CQ object is returned

        The typical use is to select the  a single object on the stack. For example::

           CQ(aCube).solids().size()

        returns 1, because a cube consists of one solid.

        It is possible for single CQ object ( or even a single CAD primitive ) to contain
        multiple solids.

        See more about selectors HERE
        """
        return self._selectObjects('Solids', selector)

    def shells(self, selector=None):
        """
        Select the shells of objects on the stack, optionally filtering the selection. If there are
        multiple objects on the stack, the shells of all objects are collected and a list of all the
        distinct shells is returned.

        :param selector: A selector
        :type selector:  None, a Selector object, or a string selector expression.
        :return: a CQ object who's stack contains all of the *distinct* solids of *all* objects on
            the current stack, filtered by the provided selector.

        If there are no shells for any objects on the current stack, an empty CQ object is returned

        Most solids will have a single shell, which represents the outer surface. A shell will
        typically be composed of multiple faces.

        See more about selectors HERE
        """
        return self._selectObjects('Shells', selector)

    def compounds(self, selector=None):
        """
        Select compounds on the stack, optionally filtering the selection. If there are multiple
        objects on the stack, they are collected and a list of all the distinct compounds
        is returned.

        :param selector: A selector
        :type selector:  None, a Selector object, or a string selector expression.
        :return: a CQ object who's stack contains all of the *distinct* solids of *all* objects on
            the current stack, filtered by the provided selector.

        A compound contains multiple CAD primitives that resulted from a single operation, such as
        a union, cut, split, or fillet.  Compounds can contain multiple edges, wires, or solids.

        See more about selectors HERE
        """
        return self._selectObjects('Compounds', selector)

    def toSvg(self, opts=None, view_vector=(-1.75,1.1,5)):
        """
        Returns svg text that represents the first item on the stack.

        for testing purposes.

        :param opts: svg formatting options
        :type opts: dictionary, width and height

        :param view_vector: camera's view direction vector
        :type view_vector: tuple, (x, y, z)
        :return: a string that contains SVG that represents this item.
        """
        return exporters.getSVG(self.val().wrapped, opts=opts, view_vector=view_vector)

    def exportSvg(self, fileName, view_vector=(-1.75,1.1,5)):
        """
        Exports the first item on the stack as an SVG file

        For testing purposes mainly.

        :param fileName: the filename to export

        :param view_vector: camera's view direction vector
        :type view_vector: tuple, (x, y, z)
        :type fileName: String, absolute path to the file
        """
        exporters.exportSVG(self, fileName, view_vector)

    def rotateAboutCenter(self, axisEndPoint, angleDegrees):
        """
        Rotates all items on the stack by the specified angle, about the specified axis

        The center of rotation is a vector starting at the center of the object on the stack,
        and ended at the specified point.

        :param axisEndPoint: the second point of axis of rotation
        :type axisEndPoint: a three-tuple in global coordinates
        :param angleDegrees: the rotation angle, in degrees
        :type angleDegrees: float
        :returns: a CQ object, with all items rotated.

        WARNING: This version returns the same cq object instead of a new one-- the
        old object is not accessible.

        Future Enhancements:
            * A version of this method that returns a transformed copy, rather than modifying
              the originals
            * This method doesn't expose a very good interface, because the axis of rotation
              could be inconsistent between multiple objects.  This is because the beginning
              of the axis is variable, while the end is fixed. This is fine when operating on
              one object, but is not cool for multiple.
        """

        #center point is the first point in the vector
        endVec = Vector(axisEndPoint)

        def _rot(obj):
            startPt = obj.Center()
            endPt = startPt + endVec
            return obj.rotate(startPt, endPt, angleDegrees)

        return self.each(_rot, False)

    def rotate(self, axisStartPoint, axisEndPoint, angleDegrees):
        """
        Returns a copy of all of the items on the stack rotated through and angle around the axis
        of rotation.

        :param axisStartPoint: The first point of the axis of rotation
        :type axisStartPoint: a 3-tuple of floats
        :type axisEndPoint: The second point of the axis of rotation
        :type axisEndPoint: a 3-tuple of floats
        :param angleDegrees: the rotation angle, in degrees
        :type angleDegrees: float
        :returns: a CQ object
        """
        return self.newObject([o.rotate(axisStartPoint, axisEndPoint, angleDegrees)
                               for o in self.objects])

    def mirror(self, mirrorPlane="XY", basePointVector=(0, 0, 0)):
        """
        Mirror a single CQ object. This operation is the same as in the FreeCAD PartWB's mirroring

        :param mirrorPlane: the plane to mirror about
        :type mirrorPlane: string, one of "XY", "YX", "XZ", "ZX", "YZ", "ZY" the planes
        :param basePointVector: the base point to mirror about
        :type basePointVector: tuple
        """
        newS = self.newObject([self.objects[0].mirror(mirrorPlane, basePointVector)])
        return newS.first()


    def translate(self, vec):
        """
        Returns a copy of all of the items on the stack moved by the specified translation vector.

        :param tupleDistance: distance to move, in global coordinates
        :type  tupleDistance: a 3-tuple of float
        :returns: a CQ object
        """
        return self.newObject([o.translate(vec) for o in self.objects])


    def shell(self, thickness):
        """
        Remove the selected faces to create a shell of the specified thickness.

        To shell, first create a solid, and *in the same chain* select the faces you wish to remove.

        :param thickness: a positive float, representing the thickness of the desired shell.
            Negative values shell inwards, positive values shell outwards.
        :raises: ValueError if the current stack contains objects that are not faces of a solid
             further up in the chain.
        :returns: a CQ object with the resulting shelled solid selected.

        This example will create a hollowed out unit cube, where the top most face is open,
        and all other walls are 0.2 units thick::

            Workplane().box(1,1,1).faces("+Z").shell(0.2)

        Shelling is one of the cases where you may need to use the add method to select several
        faces. For example, this example creates a 3-walled corner, by removing three faces
        of a cube::

            s = Workplane().box(1,1,1)
            s1 = s.faces("+Z")
            s1.add(s.faces("+Y")).add(s.faces("+X"))
            self.saveModel(s1.shell(0.2))

        This fairly yucky syntax for selecting multiple faces is planned for improvement

        **Note**:  When sharp edges are shelled inwards, they remain sharp corners, but **outward**
        shells are automatically filleted, because an outward offset from a corner generates
        a radius.


        Future Enhancements:
            Better selectors to make it easier to select multiple faces
        """
        solidRef = self.findSolid()

        for f in self.objects:
            if type(f) != Face:
                raise ValueError("Shelling requires that faces be selected")

        s = solidRef.shell(self.objects, thickness)
        solidRef.wrapped = s.wrapped
        return self.newObject([s])

    def fillet(self, radius):
        """
        Fillets a solid on the selected edges.

        The edges on the stack are filleted. The solid to which the edges belong must be in the
        parent chain of the selected edges.

        :param radius: the radius of the fillet, must be > zero
        :type radius: positive float
        :raises: ValueError if at least one edge is not selected
        :raises: ValueError if the solid containing the edge is not in the chain
        :returns: cq object with the resulting solid selected.

        This example will create a unit cube, with the top edges filleted::

            s = Workplane().box(1,1,1).faces("+Z").edges().fillet(0.1)
        """
        # TODO: we will need much better edge selectors for this to work
        # TODO: ensure that edges selected actually belong to the solid in the chain, otherwise,
        # TODO: we segfault

        solid = self.findSolid()

        edgeList = self.edges().vals()
        if len(edgeList) < 1:
            raise ValueError("Fillets requires that edges be selected")

        s = solid.fillet(radius, edgeList)
        solid.wrapped = s.wrapped
        return self.newObject([s])

    def chamfer(self, length, length2=None):
        """
        Chamfers a solid on the selected edges.

        The edges on the stack are chamfered. The solid to which the
        edges belong must be in the parent chain of the selected
        edges.

        Optional parameter `length2` can be supplied with a different
        value than `length` for a chamfer that is shorter on one side
        longer on the other side.

        :param length: the length of the fillet, must be greater than zero
        :param length2: optional parameter for asymmetrical chamfer
        :type length: positive float
        :type length2: positive float
        :raises: ValueError if at least one edge is not selected
        :raises: ValueError if the solid containing the edge is not in the chain
        :returns: cq object with the resulting solid selected.

        This example will create a unit cube, with the top edges chamfered::

            s = Workplane("XY").box(1,1,1).faces("+Z").chamfer(0.1)

        This example will create chamfers longer on the sides::

            s = Workplane("XY").box(1,1,1).faces("+Z").chamfer(0.2, 0.1)
        """
        solid = self.findSolid()

        edgeList = self.edges().vals()
        if len(edgeList) < 1:
            raise ValueError("Chamfer requires that edges be selected")

        s = solid.chamfer(length, length2, edgeList)

        solid.wrapped = s.wrapped
        return self.newObject([s])

    def __copy__(self):
        return self.newObject(copy(self.objects))

    def __deepcopy__(self, memo):
        return self.newObject(deepcopy(self.objects, memo))


class Workplane(CQ):
    """
    Defines a coordinate system in space, in which 2-d coordinates can be used.

    :param plane: the plane in which the workplane will be done
    :type plane: a Plane object, or a string in (XY|YZ|XZ|front|back|top|bottom|left|right)
    :param origin: the desired origin of the new workplane
    :type origin: a 3-tuple in global coordinates, or None to default to the origin
    :param obj: an object to use initially for the stack
    :type obj: a CAD primitive, or None to use the centerpoint of the plane as the initial
        stack value.
    :raises: ValueError if the provided plane is not a plane, a valid named workplane
    :return: A Workplane object, with coordinate system matching the supplied plane.

    The most common use is::

        s = Workplane("XY")

    After creation, the stack contains a single point, the origin of the underlying plane,
    and the *current point* is on the origin.

    .. note::
        You can also create workplanes on the surface of existing faces using
        :py:meth:`CQ.workplane`
    """

    FOR_CONSTRUCTION = 'ForConstruction'

    def __init__(self, inPlane, origin=(0, 0, 0), obj=None):
        """
        make a workplane from a particular plane

        :param inPlane: the plane in which the workplane will be done
        :type inPlane: a Plane object, or a string in (XY|YZ|XZ|front|back|top|bottom|left|right)
        :param origin: the desired origin of the new workplane
        :type origin: a 3-tuple in global coordinates, or None to default to the origin
        :param obj: an object to use initially for the stack
        :type obj: a CAD primitive, or None to use the centerpoint of the plane as the initial
            stack value.
        :raises: ValueError if the provided plane is not a plane, or one of XY|YZ|XZ
        :return: A Workplane object, with coordinate system matching the supplied plane.

        The most common use is::

            s = Workplane("XY")

        After creation, the stack contains a single point, the origin of the underlying plane, and
        the *current point* is on the origin.
        """

        if inPlane.__class__.__name__ == 'Plane':
            tmpPlane = inPlane
        else:
            try:
                tmpPlane = Plane.named(inPlane, origin)
            except ValueError:
                tmpPlane = None

        if tmpPlane is None:
            raise ValueError(
                'Provided value {} is not a valid work plane'.format(inPlane))

        self.obj = obj
        self.plane = tmpPlane
        self.firstPoint = None
        # Changed so that workplane has the center as the first item on the stack
        self.objects = [self.plane.origin]
        self.parent = None
        self.ctx = CQContext()

    def transformed(self, rotate=(0, 0, 0), offset=(0, 0, 0)):
        """
        Create a new workplane based on the current one.
        The origin of the new plane is located at the existing origin+offset vector, where offset is
        given in coordinates local to the current plane
        The new plane is rotated through the angles specified by the components of the rotation
        vector.
        :param rotate: 3-tuple of angles to rotate, in degrees relative to work plane coordinates
        :param offset: 3-tuple to offset the new plane, in local work plane coordinates
        :return: a new work plane, transformed as requested
        """

        #old api accepted a vector, so we'll check for that.
        if rotate.__class__.__name__ == 'Vector':
            rotate = rotate.toTuple()

        if offset.__class__.__name__ == 'Vector':
            offset = offset.toTuple()

        p = self.plane.rotated(rotate)
        p.origin = self.plane.toWorldCoords(offset)
        ns = self.newObject([p.origin])
        ns.plane = p

        return ns

    def newObject(self, objlist):
        """
        Create a new workplane object from this one.

        Overrides CQ.newObject, and should be used by extensions, plugins, and
        subclasses to create new objects.

        :param objlist: new objects to put on the stack
        :type objlist: a list of CAD primitives
        :return: a new Workplane object with the current workplane as a parent.
        """

        #copy the current state to the new object
        ns = type(self)("XY")
        ns.plane = self.plane
        ns.parent = self
        ns.objects = list(objlist)
        ns.ctx = self.ctx
        return ns

    def _findFromPoint(self, useLocalCoords=False):
        """
        Finds the start point for an operation when an existing point
        is implied.  Examples include 2d operations such as lineTo,
        which allows specifying the end point, and implicitly use the
        end of the previous line as the starting point

        :return: a Vector representing the point to use, or none if
        such a point is not available.

        :param useLocalCoords: selects whether the point is returned
        in local coordinates or global coordinates.

        The algorithm is this:
            * If an Edge is on the stack, its end point is used.yp
            * if a vector is on the stack, it is used

        WARNING: only the last object on the stack is used.

        NOTE:
        """
        obj = self.objects[-1]

        if isinstance(obj, Edge):
            p = obj.endPoint()
        elif isinstance(obj, Vector):
            p = obj
        else:
            raise RuntimeError("Cannot convert object type '%s' to vector " % type(obj))

        if useLocalCoords:
            return self.plane.toLocalCoords(p)
        else:
            return p

    def rarray(self, xSpacing, ySpacing, xCount, yCount, center=True):
        """
        Creates an array of points and pushes them onto the stack.
        If you want to position the array at another point, create another workplane
        that is shifted to the position you would like to use as a reference

        :param xSpacing: spacing between points in the x direction ( must be > 0)
        :param ySpacing: spacing between points in the y direction ( must be > 0)
        :param xCount: number of points ( > 0 )
        :param yCount: number of points ( > 0 )
        :param center: if true, the array will be centered at the center of the workplane. if
            false, the lower left corner will be at the center of the work plane
        """

        if xSpacing <= 0 or ySpacing <= 0 or xCount < 1 or yCount < 1:
            raise ValueError("Spacing and count must be > 0 ")

        lpoints = []  # coordinates relative to bottom left point
        for x in range(xCount):
            for y in range(yCount):
                lpoints.append((xSpacing * x, ySpacing * y))

        #shift points down and left relative to origin if requested
        if center:
            xc = xSpacing*(xCount-1) * 0.5
            yc = ySpacing*(yCount-1) * 0.5
            cpoints = []
            for p in lpoints:
                cpoints.append((p[0] - xc, p[1] - yc))
            lpoints = list(cpoints)

        return self.pushPoints(lpoints)

    def polarArray(self, radius, startAngle, angle, count, fill=True):
        """
        Creates an polar array of points and pushes them onto the stack.
        The 0 degree reference angle is located along the local X-axis.

        :param radius: Radius of the array.
        :param startAngle: Starting angle (degrees) of array. 0 degrees is
            situated along local X-axis.
        :param angle: The angle (degrees) to fill with elements. A positive
            value will fill in the counter-clockwise direction. If fill is
            false, angle is the angle between elements.
        :param count: Number of elements in array. ( > 0 )
        """

        if count <= 0:
            raise ValueError("No elements in array")

        # First element at start angle, convert to cartesian coords
        x = radius * math.cos(math.radians(startAngle))
        y = radius * math.sin(math.radians(startAngle))
        points = [(x, y)]

        # Calculate angle between elements
        if fill:
            if angle % 360 == 0:
                angle = angle / count
            elif count > 1:
                # Inclusive start and end
                angle = angle / (count - 1)

        # Add additional elements
        for i in range(1, count):
            phi = math.radians(startAngle + (angle * i))
            x = radius * math.cos(phi)
            y = radius * math.sin(phi)
            points.append((x, y))

        return self.pushPoints(points)

    def pushPoints(self, pntList):
        """
        Pushes a list of points onto the stack as vertices.
        The points are in the 2-d coordinate space of the workplane face

        :param pntList: a list of points to push onto the stack
        :type pntList: list of 2-tuples, in *local* coordinates
        :return: a new workplane with the desired points on the stack.

        A common use is to provide a list of points for a subsequent operation, such as creating
        circles or holes. This example creates a cube, and then drills three holes through it,
        based on three points::

            s = Workplane().box(1,1,1).faces(">Z").workplane().\
                pushPoints([(-0.3,0.3),(0.3,0.3),(0,0)])
            body = s.circle(0.05).cutThruAll()

        Here the circle function operates on all three points, and is then extruded to create three
        holes. See :py:meth:`circle` for how it works.
        """
        vecs = []
        for pnt in pntList:
            vec = self.plane.toWorldCoords(pnt)
            vecs.append(vec)

        return self.newObject(vecs)

    def center(self, x, y):
        """
        Shift local coordinates to the specified location.

        The location is specified in terms of local coordinates.

        :param float x: the new x location
        :param float y: the new y location
        :returns: the workplane object, with the center adjusted.

        The current point is set to the new center.
        This method is useful to adjust the center point after it has been created automatically on
        a face, but not where you'd like it to be.

        In this example, we adjust the workplane center to be at the corner of a cube, instead of
        the center of a face, which is the default::

            #this workplane is centered at x=0.5,y=0.5, the center of the upper face
            s = Workplane().box(1,1,1).faces(">Z").workplane()

            s.center(-0.5,-0.5) # move the center to the corner
            t = s.circle(0.25).extrude(0.2)
            assert ( t.faces().size() == 9 ) # a cube with a cylindrical nub at the top right corner

        The result is a cube with a round boss on the corner
        """
        "Shift local coordinates to the specified location, according to current coordinates"
        self.plane.setOrigin2d(x, y)
        n = self.newObject([self.plane.origin])
        return n

    def lineTo(self, x, y, forConstruction=False):
        """
        Make a line from the current point to the provided point

        :param float x: the x point, in workplane plane coordinates
        :param float y: the y point, in workplane plane coordinates
        :return: the Workplane object with the current point at the end of the new line

        see :py:meth:`line` if you want to use relative dimensions to make a line instead.
        """
        startPoint = self._findFromPoint(False)

        endPoint = self.plane.toWorldCoords((x, y))

        p = Edge.makeLine(startPoint, endPoint)

        if not forConstruction:
            self._addPendingEdge(p)

        return self.newObject([p])

    # line a specified incremental amount from current point
    def line(self, xDist, yDist, forConstruction=False):
        """
        Make a line from the current point to the provided point, using
        dimensions relative to the current point

        :param float xDist: x distance from current point
        :param float yDist: y distance from current point
        :return: the workplane object with the current point at the end of the new line

        see :py:meth:`lineTo` if you want to use absolute coordinates to make a line instead.
        """
        p = self._findFromPoint(True)  # return local coordinates
        return self.lineTo(p.x + xDist, yDist + p.y, forConstruction)

    def vLine(self, distance, forConstruction=False):
        """
        Make a vertical line from the current point the provided distance

        :param float distance: (y) distance from current point
        :return: the workplane object with the current point at the end of the new line
        """
        return self.line(0, distance, forConstruction)

    def hLine(self, distance, forConstruction=False):
        """
        Make a horizontal line from the current point the provided distance

        :param float distance: (x) distance from current point
        :return: the Workplane object with the current point at the end of the new line
        """
        return self.line(distance, 0, forConstruction)

    def vLineTo(self, yCoord, forConstruction=False):
        """
        Make a vertical line from the current point to the provided y coordinate.

        Useful if it is more convenient to specify the end location rather than distance,
        as in :py:meth:`vLine`

        :param float yCoord: y coordinate for the end of the line
        :return: the Workplane object with the current point at the end of the new line
        """
        p = self._findFromPoint(True)
        return self.lineTo(p.x, yCoord, forConstruction)

    def hLineTo(self, xCoord, forConstruction=False):
        """
        Make a horizontal line from the current point to the provided x coordinate.

        Useful if it is more convenient to specify the end location rather than distance,
        as in :py:meth:`hLine`

        :param float xCoord: x coordinate for the end of the line
        :return: the Workplane object with the current point at the end of the new line
        """
        p = self._findFromPoint(True)
        return self.lineTo(xCoord, p.y, forConstruction)

    def polarLine(self, distance, angle, forConstruction=False):
        """
        Make a line of the given length, at the given angle from the current point

        :param float distance: distance of the end of the line from the current point
        :param float angle: angle of the vector to the end of the line with the x-axis
        :return: the Workplane object with the current point at the end of the new line
       """
        x = math.cos(math.radians(angle)) * distance
        y = math.sin(math.radians(angle)) * distance

        return self.line(x, y, forConstruction)

    def polarLineTo(self, distance, angle, forConstruction=False):
        """
        Make a line from the current point to the given polar co-ordinates

        Useful if it is more convenient to specify the end location rather than
        the distance and angle from the current point

        :param float distance: distance of the end of the line from the origin
        :param float angle: angle of the vector to the end of the line with the x-axis
        :return: the Workplane object with the current point at the end of the new line
        """
        x = math.cos(math.radians(angle)) * distance
        y = math.sin(math.radians(angle)) * distance

        return self.lineTo(x, y, forConstruction)

    #absolute move in current plane, not drawing
    def moveTo(self, x=0, y=0):
        """
        Move to the specified point, without drawing.

        :param x: desired x location, in local coordinates
        :type x: float, or none for zero
        :param y: desired y location, in local coordinates
        :type y: float, or none for zero.

        Not to be confused with :py:meth:`center`, which moves the center of the entire
        workplane, this method only moves the current point ( and therefore does not affect objects
        already drawn ).

        See :py:meth:`move` to do the same thing but using relative dimensions
        """
        newCenter = Vector(x, y, 0)
        return self.newObject([self.plane.toWorldCoords(newCenter)])

    #relative move in current plane, not drawing
    def move(self, xDist=0, yDist=0):
        """
        Move the specified distance from the current point, without drawing.

        :param xDist: desired x distance, in local coordinates
        :type xDist: float, or none for zero
        :param yDist: desired y distance, in local coordinates
        :type yDist: float, or none for zero.

        Not to be confused with :py:meth:`center`, which moves the center of the entire
        workplane, this method only moves the current point ( and therefore does not affect objects
        already drawn ).

        See :py:meth:`moveTo` to do the same thing but using absolute coordinates
        """
        p = self._findFromPoint(True)
        newCenter = p + Vector(xDist, yDist, 0)
        return self.newObject([self.plane.toWorldCoords(newCenter)])

    def spline(self, listOfXYTuple, forConstruction=False):
        """
        Create a spline interpolated through the provided points.

        :param listOfXYTuple: points to interpolate through
        :type listOfXYTuple: list of 2-tuple
        :return: a Workplane object with the current point at the end of the spline

        The spline will begin at the current point, and
        end with the last point in the XY tuple list

        This example creates a block with a spline for one side::

            s = Workplane(Plane.XY())
            sPnts = [
                (2.75,1.5),
                (2.5,1.75),
                (2.0,1.5),
                (1.5,1.0),
                (1.0,1.25),
                (0.5,1.0),
                (0,1.0)
            ]
            r = s.lineTo(3.0,0).lineTo(3.0,1.0).spline(sPnts).close()
            r = r.extrude(0.5)

        *WARNING*  It is fairly easy to create a list of points
        that cannot be correctly interpreted as a spline.

        Future Enhancements:
          * provide access to control points
        """
        gstartPoint = self._findFromPoint(False)
        gEndPoint = self.plane.toWorldCoords(listOfXYTuple[-1])

        vecs = [self.plane.toWorldCoords(p) for p in listOfXYTuple]
        allPoints = [gstartPoint] + vecs

        e = Edge.makeSpline(allPoints)

        if not forConstruction:
            self._addPendingEdge(e)

        return self.newObject([e])

    def threePointArc(self, point1, point2, forConstruction=False):
        """
        Draw an arc from the current point, through point1, and ending at point2

        :param point1: point to draw through
        :type point1: 2-tuple, in workplane coordinates
        :param point2: end point for the arc
        :type point2: 2-tuple, in workplane coordinates
        :return: a workplane with the current point at the end of the arc

        Future Enhancements:
            provide a version that allows an arc using relative measures
            provide a centerpoint arc
            provide tangent arcs
        """

        startPoint = self._findFromPoint(False)
        point1 = self.plane.toWorldCoords(point1)
        point2 = self.plane.toWorldCoords(point2)

        arc = Edge.makeThreePointArc(startPoint, point1, point2)

        if not forConstruction:
            self._addPendingEdge(arc)

        return self.newObject([arc])

    def sagittaArc(self, endPoint, sag, forConstruction=False):
        """
        Draw an arc from the current point to endPoint with an arc defined by the sag (sagitta).

        :param endPoint: end point for the arc
        :type endPoint: 2-tuple, in workplane coordinates
        :param sag: the sagitta of the arc
        :type sag: float, perpendicular distance from arc center to arc baseline.
        :return: a workplane with the current point at the end of the arc

        The sagitta is the distance from the center of the arc to the arc base.
        Given that a closed contour is drawn clockwise;
        A positive sagitta means convex arc and negative sagitta means concave arc.
        See "https://en.wikipedia.org/wiki/Sagitta_(geometry)" for more information.
        """

        startPoint = self._findFromPoint(useLocalCoords=True)
        endPoint = Vector(endPoint)
        midPoint = endPoint.add(startPoint).multiply(0.5)

        sagVector = endPoint.sub(startPoint).normalized().multiply(abs(sag))
        if(sag > 0):
            sagVector.x, sagVector.y = -sagVector.y, sagVector.x # Rotate sagVector +90 deg
        else:
            sagVector.x, sagVector.y = sagVector.y, -sagVector.x # Rotate sagVector -90 deg

        sagPoint = midPoint.add(sagVector)

        return self.threePointArc(sagPoint, endPoint, forConstruction)

    def radiusArc(self, endPoint, radius, forConstruction=False):
        """
        Draw an arc from the current point to endPoint with an arc defined by the sag (sagitta).

        :param endPoint: end point for the arc
        :type endPoint: 2-tuple, in workplane coordinates
        :param radius: the radius of the arc
        :type radius: float, the radius of the arc between start point and end point.
        :return: a workplane with the current point at the end of the arc

        Given that a closed contour is drawn clockwise;
        A positive radius means convex arc and negative radius means concave arc.
        """

        startPoint = self._findFromPoint(useLocalCoords=True)
        endPoint = Vector(endPoint)

        # Calculate the sagitta from the radius
        length = endPoint.sub(startPoint).Length / 2.0
        try:
            sag = abs(radius) - math.sqrt(radius**2 - length**2)
        except ValueError:
            raise ValueError("Arc radius is not large enough to reach the end point.")

        # Return a sagittaArc
        if radius > 0:
            return self.sagittaArc(endPoint, sag, forConstruction)
        else:
            return self.sagittaArc(endPoint, -sag, forConstruction)

    def rotateAndCopy(self, matrix):
        """
        Makes a copy of all edges on the stack, rotates them according to the
        provided matrix, and then attempts to consolidate them into a single wire.

        :param matrix: a 4xr transformation matrix, in global coordinates
        :type matrix: a FreeCAD Base.Matrix object
        :return: a CadQuery object  with consolidated wires, and any originals on the stack.

        The most common use case is to create a set of open edges, and then mirror them
        around either the X or Y axis to complete a closed shape.

        see :py:meth:`mirrorX` and :py:meth:`mirrorY` to mirror about the global X and Y axes
        see :py:meth:`mirrorX` and for an example

        Future Enhancements:
            faster implementation: this one transforms 3 times to accomplish the result
        """

        #convert edges to a wire, if there are pending edges
        n = self.wire(forConstruction=False)

        #attempt to consolidate wires together.
        consolidated = n.consolidateWires()

        rotatedWires = self.plane.rotateShapes(consolidated.wires().vals(), matrix)

        for w in rotatedWires:
            consolidated.objects.append(w)
            consolidated._addPendingWire(w)

        #attempt again to consolidate all of the wires
        c = consolidated.consolidateWires()

        return c

    def mirrorY(self):
        """
        Mirror entities around the y axis of the workplane plane.

        :return: a new object with any free edges consolidated into as few wires as possible.

        All free edges are collected into a wire, and then the wire is mirrored,
        and finally joined into a new wire

        Typically used to make creating wires with symmetry easier. This line of code::

             s = Workplane().lineTo(2,2).threePointArc((3,1),(2,0)).mirrorX().extrude(0.25)

        Produces a flat, heart shaped object

        Future Enhancements:
            mirrorX().mirrorY() should work but doesn't, due to some FreeCAD weirdness
        """
        tm = Matrix()
        tm.rotateY(math.pi)
        return self.rotateAndCopy(tm)

    def mirrorX(self):
        """
        Mirror entities around the x axis of the workplane plane.

        :return: a new object with any free edges consolidated into as few wires as possible.

        All free edges are collected into a wire, and then the wire is mirrored,
        and finally joined into a new wire

        Typically used to make creating wires with symmetry easier.

        Future Enhancements:
            mirrorX().mirrorY() should work but doesn't, due to some FreeCAD weirdness
        """
        tm = Matrix()
        tm.rotateX(math.pi)
        return self.rotateAndCopy(tm)

    def _addPendingEdge(self, edge):
        """
        Queues an edge for later combination into a wire.

        :param edge:
        :return:
        """
        self.ctx.pendingEdges.append(edge)

        if self.ctx.firstPoint is None:
            self.ctx.firstPoint = self.plane.toLocalCoords(edge.startPoint())

    def _addPendingWire(self, wire):
        """
        Queue a Wire for later extrusion

        Internal Processing Note.  In FreeCAD, edges-->wires-->faces-->solids.

        but users do not normally care about these distinctions.  Users 'think' in terms
        of edges, and solids.

        CadQuery tracks edges as they are drawn, and automatically combines them into wires
        when the user does an operation that needs it.

        Similarly, cadQuery tracks pending wires, and automatically combines them into faces
        when necessary to make a solid.
        """
        self.ctx.pendingWires.append(wire)

    def consolidateWires(self):
        """
        Attempt to consolidate wires on the stack into a single.
        If possible, a new object with the results are returned.
        if not possible, the wires remain separated

        FreeCAD has a bug in Part.Wire([]) which does not create wires/edges properly sometimes
        Additionally, it has a bug where a profile composed of two wires ( rather than one )
        also does not work properly. Together these are a real problem.
        """
        wires = self.wires().vals()
        if len(wires) < 2:
            return self

        #TODO: this makes the assumption that either all wires could be combined, or none.
        #in reality trying each combination of wires is probably not reasonable anyway
        w = Wire.combine(wires)

        #ok this is a little tricky. if we consolidate wires, we have to actually
        #modify the pendingWires collection to remove the original ones, and replace them
        #with the consolidate done
        #since we are already assuming that all wires could be consolidated, its easy, we just
        #clear the pending wire list
        r = self.newObject([w])
        r.ctx.pendingWires = []
        r._addPendingWire(w)
        return r

    def wire(self, forConstruction=False):
        """
        Returns a CQ object with all pending edges connected into a wire.

        All edges on the stack that can be combined will be combined into a single wire object,
        and other objects will remain on the stack unmodified

        :param forConstruction: whether the wire should be used to make a solid, or if it is just
            for reference
        :type forConstruction: boolean. true if the object is only for reference

        This method is primarily of use to plugin developers making utilities for 2-d construction.
        This method should be called when a user operation implies that 2-d construction is
        finished, and we are ready to begin working in 3d

        SEE '2-d construction concepts' for a more detailed explanation of how CadQuery handles
        edges, wires, etc

        Any non edges will still remain.
        """

        edges = self.ctx.pendingEdges

        #do not consolidate if there are no free edges
        if len(edges) == 0:
            return self

        self.ctx.pendingEdges = []

        others = []
        for e in self.objects:
            if type(e) != Edge:
                others.append(e)


        w = Wire.assembleEdges(edges)
        if not forConstruction:
            self._addPendingWire(w)

        return self.newObject(others + [w])

    def each(self, callBackFunction, useLocalCoordinates=False):
        """
        Runs the provided function on each value in the stack, and collects the return values into
        a new CQ object.

        Special note: a newly created workplane always has its center point as its only stack item

        :param callBackFunction: the function to call for each item on the current stack.
        :param useLocalCoordinates: should  values be converted from local coordinates first?
        :type useLocalCoordinates: boolean

        The callback function must accept one argument, which is the item on the stack, and return
        one object, which is collected. If the function returns None, nothing is added to the stack.
        The object passed into the callBackFunction is potentially transformed to local coordinates,
        if useLocalCoordinates is true

        useLocalCoordinates is very useful for plugin developers.

        If false, the callback function is assumed to be working in global coordinates.  Objects
        created are added as-is, and objects passed into the function are sent in using global
        coordinates

        If true, the calling function is assumed to be  working in local coordinates.  Objects are
        transformed to local coordinates before they are passed into the callback method, and result
        objects are transformed to global coordinates after they are returned.

        This allows plugin developers to create objects in local coordinates, without worrying
        about the fact that the working plane is different than the global coordinate system.


        TODO: wrapper object for Wire will clean up forConstruction flag everywhere
        """
        results = []
        for obj in self.objects:

            if useLocalCoordinates:
                #TODO: this needs to work for all types of objects, not just vectors!
                r = callBackFunction(self.plane.toLocalCoords(obj))
                r = r.transformShape(self.plane.rG)
            else:
                r = callBackFunction(obj)

            if type(r) == Wire:
                if not r.forConstruction:
                    self._addPendingWire(r)

            results.append(r)

        return self.newObject(results)

    def eachpoint(self, callbackFunction, useLocalCoordinates=False):
        """
        Same as each(), except each item on the stack is converted into a point before it
        is passed into the callback function.

        :return: CadQuery object which contains a list of  vectors (points ) on its stack.

        :param useLocalCoordinates: should points be in local or global coordinates
        :type useLocalCoordinates: boolean

        The resulting object has a point on the stack for each object on the original stack.
        Vertices and points remain a point.  Faces, Wires, Solids, Edges, and Shells are converted
        to a point by using their center of mass.

        If the stack has zero length, a single point is returned, which is the center of the current
        workplane/coordinate system
        """
        #convert stack to a list of points
        pnts = []
        if len(self.objects) == 0:
            #nothing on the stack. here, we'll assume we should operate with the
            #origin as the context point
            pnts.append(self.plane.origin)
        else:

            for v in self.objects:
                pnts.append(v.Center())

        return self.newObject(pnts).each(callbackFunction, useLocalCoordinates)

    def rect(self, xLen, yLen, centered=True, forConstruction=False):
        """
        Make a rectangle for each item on the stack.

        :param xLen: length in xDirection ( in workplane coordinates )
        :type xLen: float > 0
        :param yLen: length in yDirection ( in workplane coordinates )
        :type yLen: float > 0
        :param boolean centered: true if the rect is centered on the reference point, false if the
            lower-left is on the reference point
        :param forConstruction: should the new wires be reference geometry only?
        :type forConstruction: true if the wires are for reference, false if they are creating part
            geometry
        :return: a new CQ object with the created wires on the stack

        A common use case is to use a for-construction rectangle to define the centers of a hole
        pattern::

            s = Workplane().rect(4.0,4.0,forConstruction=True).vertices().circle(0.25)

        Creates 4 circles at the corners of a square centered on the origin.

        Future Enhancements:
            better way to handle forConstruction
            project points not in the workplane plane onto the workplane plane
        """
        def makeRectangleWire(pnt):
            # Here pnt is in local coordinates due to useLocalCoords=True
            # (xc,yc,zc) = pnt.toTuple()
            if centered:
                p1 = pnt.add(Vector(xLen/-2.0, yLen/-2.0, 0))
                p2 = pnt.add(Vector(xLen/2.0, yLen/-2.0, 0))
                p3 = pnt.add(Vector(xLen/2.0, yLen/2.0, 0))
                p4 = pnt.add(Vector(xLen/-2.0, yLen/2.0, 0))
            else:
                p1 = pnt
                p2 = pnt.add(Vector(xLen, 0, 0))
                p3 = pnt.add(Vector(xLen, yLen, 0))
                p4 = pnt.add(Vector(0, yLen, 0))

            w = Wire.makePolygon([p1, p2, p3, p4, p1], forConstruction)
            return w
            #return Part.makePolygon([p1,p2,p3,p4,p1])

        return self.eachpoint(makeRectangleWire, True)

    #circle from current point
    def circle(self, radius, forConstruction=False):
        """
        Make a circle for each item on the stack.

        :param radius: radius of the circle
        :type radius: float > 0
        :param forConstruction: should the new wires be reference geometry only?
        :type forConstruction: true if the wires are for reference, false if they are creating
            part geometry
        :return: a new CQ object with the created wires on the stack

        A common use case is to use a for-construction rectangle to define the centers of a
        hole pattern::

            s = Workplane().rect(4.0,4.0,forConstruction=True).vertices().circle(0.25)

        Creates 4 circles at the corners of a square centered on the origin. Another common case is
        to use successive circle() calls to create concentric circles.  This works because the
        center of a circle is its reference point::

            s = Workplane().circle(2.0).circle(1.0)

        Creates two concentric circles, which when extruded will form a ring.

        Future Enhancements:
            better way to handle forConstruction
            project points not in the workplane plane onto the workplane plane

        """
        def makeCircleWire(obj):
            cir = Wire.makeCircle(radius, obj, Vector(0, 0, 1))
            cir.forConstruction = forConstruction
            return cir

        return self.eachpoint(makeCircleWire, useLocalCoordinates=True)

    def polygon(self, nSides, diameter, forConstruction=False):
        """
        Creates a polygon inscribed in a circle of the specified diameter for each point on
        the stack

        The first vertex is always oriented in the x direction.

        :param nSides: number of sides, must be > 3
        :param diameter: the size of the circle the polygon is inscribed into
        :return: a polygon wire
        """
        def _makePolygon(center):
            #pnt is a vector in local coordinates
            angle = 2.0 * math.pi / nSides
            pnts = []
            for i in range(nSides+1):
                pnts.append(center + Vector((diameter / 2.0 * math.cos(angle*i)),
                                            (diameter / 2.0 * math.sin(angle*i)), 0))
            return Wire.makePolygon(pnts, forConstruction)

        return self.eachpoint(_makePolygon, True)

    def polyline(self, listOfXYTuple, forConstruction=False):
        """
        Create a polyline from a list of points

        :param listOfXYTuple: a list of points in Workplane coordinates
        :type listOfXYTuple: list of 2-tuples
        :param forConstruction: whether or not the edges are used for reference
        :type forConstruction: true if the edges are for reference, false if they are for creating geometry
            part geometry
        :return: a new CQ object with a list of edges on the stack

        *NOTE* most commonly, the resulting wire should be closed.
        """

        # Our list of new edges that will go into a new CQ object
        edges = []

        # The very first startPoint comes from our original object, but not after that
        startPoint = self._findFromPoint(False)

        # Draw a line for each set of points, starting from the from-point of the original CQ object
        for curTuple in listOfXYTuple:
            endPoint = self.plane.toWorldCoords(curTuple)

            edges.append(Edge.makeLine(startPoint, endPoint))

            # We need to move the start point for the next line that we draw or we get stuck at the same startPoint
            startPoint = endPoint

            if not forConstruction:
                self._addPendingEdge(edges[-1])

        return self.newObject(edges)

    def close(self):
        """
        End 2-d construction, and attempt to build a closed wire.

        :return: a CQ object with a completed wire on the stack, if possible.

        After 2-d drafting with lineTo,threePointArc, and polyline, it is necessary
        to convert the edges produced by these into one or more wires.

        When a set of edges is closed, cadQuery assumes it is safe to build the group of edges
        into a wire.  This example builds a simple triangular prism::

            s = Workplane().lineTo(1,0).lineTo(1,1).close().extrude(0.2)
        """
        endPoint = self._findFromPoint(True)
        startPoint = self.ctx.firstPoint

        # Check if there is a distance between startPoint and endPoint
        # that is larger than what is considered a numerical error.
        # If so; add a line segment between endPoint and startPoint
        if endPoint.sub(startPoint).Length > 1e-6:
            self.lineTo(self.ctx.firstPoint.x, self.ctx.firstPoint.y)

        # Need to reset the first point after closing a wire
        self.ctx.firstPoint=None

        return self.wire()

    def largestDimension(self):
        """
        Finds the largest dimension in the stack.
        Used internally to create thru features, this is how you can compute
        how long or wide a feature must be to make sure to cut through all of the material
        :return: A value representing the largest dimension of the first solid on the stack
        """
        #TODO: this implementation is naive and returns the dims of the first solid... most of
        #TODO: the time this works. but a stronger implementation would be to search all solids.
        s = self.findSolid()
        if s:
            return s.BoundingBox().DiagonalLength * 5.0
        else:
            return -1

    def cutEach(self, fcn, useLocalCoords=False, clean=True):
        """
        Evaluates the provided function at each point on the stack (ie, eachpoint)
        and then cuts the result from the context solid.
        :param fcn: a function suitable for use in the eachpoint method: ie, that accepts a vector
        :param useLocalCoords: same as for :py:meth:`eachpoint`
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape
        :return: a CQ object that contains the resulting solid
        :raises: an error if there is not a context solid to cut from
        """
        ctxSolid = self.findSolid()
        if ctxSolid is None:
            raise ValueError("Must have a solid in the chain to cut from!")

        #will contain all of the counterbores as a single compound
        results = self.eachpoint(fcn, useLocalCoords).vals()
        s = ctxSolid
        for cb in results:
            s = s.cut(cb)

        if clean: s = s.clean()

        ctxSolid.wrapped = s.wrapped
        return self.newObject([s])

    #but parameter list is different so a simple function pointer won't work
    def cboreHole(self, diameter, cboreDiameter, cboreDepth, depth=None, clean=True):
        """
        Makes a counterbored hole for each item on the stack.

        :param diameter: the diameter of the hole
        :type diameter: float > 0
        :param cboreDiameter: the diameter of the cbore
        :type cboreDiameter: float > 0 and > diameter
        :param cboreDepth: depth of the counterbore
        :type cboreDepth: float > 0
        :param depth: the depth of the hole
        :type depth: float > 0 or None to drill thru the entire part.
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape

        The surface of the hole is at the current workplane plane.

        One hole is created for each item on the stack.  A very common use case is to use a
        construction rectangle to define the centers of a set of holes, like so::

                s = Workplane(Plane.XY()).box(2,4,0.5).faces(">Z").workplane()\
                    .rect(1.5,3.5,forConstruction=True)\
                    .vertices().cboreHole(0.125, 0.25,0.125,depth=None)

        This sample creates a plate with a set of holes at the corners.

        **Plugin Note**: this is one example of the power of plugins. Counterbored holes are quite
        time consuming to create, but are quite easily defined by users.

        see :py:meth:`cskHole` to make countersinks instead of counterbores
        """
        if depth is None:
            depth = self.largestDimension()

        def _makeCbore(center):
            """
            Makes a single hole with counterbore at the supplied point
            returns a solid suitable for subtraction
            pnt is in local coordinates
            """
            boreDir = Vector(0, 0, -1)
            #first make the hole
            hole = Solid.makeCylinder(diameter/2.0, depth, center, boreDir)  # local coordianates!

            #add the counter bore
            cbore = Solid.makeCylinder(cboreDiameter / 2.0, cboreDepth, center, boreDir)
            r = hole.fuse(cbore)
            return r

        return self.cutEach(_makeCbore, True, clean)

    #TODO: almost all code duplicated!
    #but parameter list is different so a simple function pointer won't work
    def cskHole(self, diameter, cskDiameter, cskAngle, depth=None, clean=True):
        """
        Makes a countersunk hole for each item on the stack.

        :param diameter: the diameter of the hole
        :type diameter: float > 0
        :param cskDiameter: the diameter of the countersink
        :type cskDiameter: float > 0 and > diameter
        :param cskAngle: angle of the countersink, in degrees ( 82 is common )
        :type cskAngle: float > 0
        :param depth: the depth of the hole
        :type depth: float > 0 or None to drill thru the entire part.
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape

        The surface of the hole is at the current workplane.

        One hole is created for each item on the stack.  A very common use case is to use a
        construction rectangle to define the centers of a set of holes, like so::

                s = Workplane(Plane.XY()).box(2,4,0.5).faces(">Z").workplane()\
                    .rect(1.5,3.5,forConstruction=True)\
                    .vertices().cskHole(0.125, 0.25,82,depth=None)

        This sample creates a plate with a set of holes at the corners.

        **Plugin Note**: this is one example of the power of plugins. CounterSunk holes are quite
        time consuming to create, but are quite easily defined by users.

        see :py:meth:`cboreHole` to make counterbores instead of countersinks
        """

        if depth is None:
            depth = self.largestDimension()

        def _makeCsk(center):
            #center is in local coordinates

            boreDir = Vector(0, 0, -1)

            #first make the hole
            hole = Solid.makeCylinder(diameter/2.0, depth, center, boreDir)  # local coords!
            r = cskDiameter / 2.0
            h = r / math.tan(math.radians(cskAngle / 2.0))
            csk = Solid.makeCone(r, 0.0, h, center, boreDir)
            r = hole.fuse(csk)
            return r

        return self.cutEach(_makeCsk, True, clean)

    #TODO: almost all code duplicated!
    #but parameter list is different so a simple function pointer won't work
    def hole(self, diameter, depth=None, clean=True):
        """
        Makes a hole for each item on the stack.

        :param diameter: the diameter of the hole
        :type diameter: float > 0
        :param depth: the depth of the hole
        :type depth: float > 0 or None to drill thru the entire part.
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape

        The surface of the hole is at the current workplane.

        One hole is created for each item on the stack.  A very common use case is to use a
        construction rectangle to define the centers of a set of holes, like so::

                s = Workplane(Plane.XY()).box(2,4,0.5).faces(">Z").workplane()\
                    .rect(1.5,3.5,forConstruction=True)\
                    .vertices().hole(0.125, 0.25,82,depth=None)

        This sample creates a plate with a set of holes at the corners.

        **Plugin Note**: this is one example of the power of plugins. CounterSunk holes are quite
        time consuming to create, but are quite easily defined by users.

        see :py:meth:`cboreHole` and :py:meth:`cskHole` to make counterbores or countersinks
        """
        if depth is None:
            depth = self.largestDimension()

        def _makeHole(center):
            """
            Makes a single hole with counterbore at the supplied point
            returns a solid suitable for subtraction
            pnt is in local coordinates
            """
            boreDir = Vector(0, 0, -1)
            #first make the hole
            hole = Solid.makeCylinder(diameter / 2.0, depth, center, boreDir)  # local coordinates!
            return hole

        return self.cutEach(_makeHole, True, clean)

    #TODO: duplicated code with _extrude and extrude
    def twistExtrude(self, distance, angleDegrees, combine=True, clean=True):
        """
        Extrudes a wire in the direction normal to the plane, but also twists by the specified
        angle over the length of the extrusion

        The center point of the rotation will be the center of the workplane

        See extrude for more details, since this method is the same except for the the addition
        of the angle. In fact, if angle=0, the result is the same as a linear extrude.

        **NOTE**  This method can create complex calculations, so be careful using it with
        complex geometries

        :param distance: the distance to extrude normal to the workplane
        :param angle: angline ( in degrees) to rotate through the extrusion
        :param boolean combine: True to combine the resulting solid with parent solids if found.
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape
        :return: a CQ object with the resulting solid selected.
        """
        #group wires together into faces based on which ones are inside the others
        #result is a list of lists
        wireSets = sortWiresByBuildOrder(list(self.ctx.pendingWires), self.plane, [])

        self.ctx.pendingWires = []  # now all of the wires have been used to create an extrusion

        #compute extrusion vector and extrude
        eDir = self.plane.zDir.multiply(distance)

        #one would think that fusing faces into a compound and then extruding would work,
        #but it doesnt-- the resulting compound appears to look right, ( right number of faces, etc)
        #but then cutting it from the main solid fails with BRep_NotDone.
        #the work around is to extrude each and then join the resulting solids, which seems to work

        #underlying cad kernel can only handle simple bosses-- we'll aggregate them if there
        # are multiple sets
        r = None
        for ws in wireSets:
            thisObj = Solid.extrudeLinearWithRotation(ws[0], ws[1:], self.plane.origin,
                                                      eDir, angleDegrees)
            if r is None:
                r = thisObj
            else:
                r = r.fuse(thisObj)

        if combine:
            newS = self._combineWithBase(r)
        else:
            newS = self.newObject([r])
        if clean: newS = newS.clean()
        return newS

    def extrude(self, distance, combine=True, clean=True, both=False):
        """
        Use all un-extruded wires in the parent chain to create a prismatic solid.

        :param distance: the distance to extrude, normal to the workplane plane
        :type distance: float, negative means opposite the normal direction
        :param boolean combine: True to combine the resulting solid with parent solids if found.
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape
        :param boolean both: extrude in both directions symmetrically
        :return: a CQ object with the resulting solid selected.

        extrude always *adds* material to a part.

        The returned object is always a CQ object, and depends on wither combine is True, and
        whether a context solid is already defined:

        *  if combine is False, the new value is pushed onto the stack.
        *  if combine is true, the value is combined with the context solid if it exists,
           and the resulting solid becomes the new context solid.

        FutureEnhancement:
            Support for non-prismatic extrusion ( IE, sweeping along a profile, not just
            perpendicular to the plane extrude to surface. this is quite tricky since the surface
            selected may not be planar
        """
        r = self._extrude(distance,both=both)  # returns a Solid (or a compound if there were multiple)

        if combine:
            newS = self._combineWithBase(r)
        else:
            newS = self.newObject([r])
        if clean: newS = newS.clean()
        return newS

    def revolve(self, angleDegrees=360.0, axisStart=None, axisEnd=None, combine=True, clean=True):
        """
        Use all un-revolved wires in the parent chain to create a solid.

        :param angleDegrees: the angle to revolve through.
        :type angleDegrees: float, anything less than 360 degrees will leave the shape open
        :param axisStart: the start point of the axis of rotation
        :type axisStart: tuple, a two tuple
        :param axisEnd: the end point of the axis of rotation
        :type axisEnd: tuple, a two tuple
        :param combine: True to combine the resulting solid with parent solids if found.
        :type combine: boolean, combine with parent solid
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape
        :return: a CQ object with the resulting solid selected.

        The returned object is always a CQ object, and depends on wither combine is True, and
        whether a context solid is already defined:

        *  if combine is False, the new value is pushed onto the stack.
        *  if combine is true, the value is combined with the context solid if it exists,
           and the resulting solid becomes the new context solid.
        """
        #Make sure we account for users specifying angles larger than 360 degrees
        angleDegrees %= 360.0

        #Compensate for FreeCAD not assuming that a 0 degree revolve means a 360 degree revolve
        angleDegrees = 360.0 if angleDegrees == 0 else angleDegrees

        # The default start point of the vector defining the axis of rotation will be the origin
        # of the workplane
        if axisStart is None:
            axisStart = self.plane.toWorldCoords((0, 0)).toTuple()
        else:
            axisStart = self.plane.toWorldCoords(axisStart).toTuple()

        # The default end point of the vector defining the axis of rotation should be along the
        # normal from the plane
        if axisEnd is None:
            # Make sure we match the user's assumed axis of rotation if they specified an start
            # but not an end
            if axisStart[1] != 0:
                axisEnd = self.plane.toWorldCoords((0, axisStart[1])).toTuple()
            else:
                axisEnd = self.plane.toWorldCoords((0, 1)).toTuple()
        else:
            axisEnd = self.plane.toWorldCoords(axisEnd).toTuple()

        # returns a Solid (or a compound if there were multiple)
        r = self._revolve(angleDegrees, axisStart, axisEnd)
        if combine:
            newS = self._combineWithBase(r)
        else:
            newS = self.newObject([r])
        if clean: newS = newS.clean()
        return newS

    def sweep(self, path, sweepAlongWires=False, makeSolid=True, isFrenet=False, combine=True, clean=True):
        """
        Use all un-extruded wires in the parent chain to create a swept solid.

        :param path: A wire along which the pending wires will be swept
        :param boolean sweepAlongWires:
            False to create multiple swept from wires on the chain along path
            True to create only one solid swept along path with shape following the list of wires on the chain
        :param boolean combine: True to combine the resulting solid with parent solids if found.
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape
        :return: a CQ object with the resulting solid selected.
        """

        r = self._sweep(path.wire(), sweepAlongWires, makeSolid, isFrenet)  # returns a Solid (or a compound if there were multiple)
        if combine:
            newS = self._combineWithBase(r)
        else:
            newS = self.newObject([r])
        if clean: newS = newS.clean()
        return newS

    def _combineWithBase(self, obj):
        """
        Combines the provided object with the base solid, if one can be found.
        :param obj:
        :return: a new object that represents the result of combining the base object with obj,
           or obj if one could not be found
        """
        baseSolid = self.findSolid(searchParents=True)
        r = obj
        if baseSolid is not None:
            r = baseSolid.fuse(obj)
            baseSolid.wrapped = r.wrapped

        return self.newObject([r])

    def combine(self, clean=True):
        """
        Attempts to combine all of the items on the stack into a single item.
        WARNING: all of the items must be of the same type!

        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape
        :raises: ValueError if there are no items on the stack, or if they cannot be combined
        :return: a CQ object with the resulting object selected
        """
        items = list(self.objects)
        s = items.pop(0)
        for ss in items:
            s = s.fuse(ss)

        if clean: s = s.clean()

        return self.newObject([s])

    def union(self, toUnion=None, combine=True, clean=True):
        """
        Unions all of the items on the stack of toUnion with the current solid.
        If there is no current solid, the items in toUnion are unioned together.
        if combine=True, the result and the original are updated to point to the new object
        if combine=False, the result will be on the stack, but the original is unmodified

        :param toUnion:
        :type toUnion: a solid object, or a CQ object having a solid,
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape
        :raises: ValueError if there is no solid to add to in the chain
        :return: a CQ object with the resulting object selected
        """

        #first collect all of the items together
        if isinstance(toUnion, CQ):
            solids = toUnion.solids().vals()
            if len(solids) < 1:
                raise ValueError("CQ object  must have at least one solid on the stack to union!")
            newS = solids.pop(0)
            for s in solids:
                newS = newS.fuse(s)
        elif type(toUnion) == Solid:
            newS = toUnion
        else:
            raise ValueError("Cannot union type '{}'".format(type(toUnion)))

        #now combine with existing solid, if there is one
        # look for parents to cut from
        solidRef = self.findSolid(searchStack=True, searchParents=True)
        if combine and solidRef is not None:
            r = solidRef.fuse(newS)
            solidRef.wrapped = newS.wrapped
        else:
            r = newS

        if clean: r = r.clean()

        return self.newObject([r])

    def cut(self, toCut, combine=True, clean=True):
        """
        Cuts the provided solid from the current solid, IE, perform a solid subtraction

        if combine=True, the result and the original are updated to point to the new object
        if combine=False, the result will be on the stack, but the original is unmodified

        :param toCut: object to cut
        :type toCut: a solid object, or a CQ object having a solid,
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape
        :raises: ValueError if there is no solid to subtract from in the chain
        :return: a CQ object with the resulting object selected
        """

        # look for parents to cut from
        solidRef = self.findSolid(searchStack=True, searchParents=True)

        if solidRef is None:
            raise ValueError("Cannot find solid to cut from")
        solidToCut = None
        if type(toCut) == CQ or type(toCut) == Workplane:
            solidToCut = toCut.val()
        elif type(toCut) == Solid:
            solidToCut = toCut
        else:
            raise ValueError("Cannot cut type '{}'".format(type(toCut)))

        newS = solidRef.cut(solidToCut)

        if clean: newS = newS.clean()

        if combine:
            solidRef.wrapped = newS.wrapped

        return self.newObject([newS])

    def intersect(self, toIntersect, combine=True, clean=True):
        """
        Intersects the provided solid from the current solid.

        if combine=True, the result and the original are updated to point to the new object
        if combine=False, the result will be on the stack, but the original is unmodified

        :param toIntersect: object to intersect
        :type toIntersect: a solid object, or a CQ object having a solid,
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape
        :raises: ValueError if there is no solid to intersect with in the chain
        :return: a CQ object with the resulting object selected
        """

        # look for parents to intersect with
        solidRef = self.findSolid(searchStack=True, searchParents=True)

        if solidRef is None:
            raise ValueError("Cannot find solid to intersect with")
        solidToIntersect = None

        if isinstance(toIntersect, CQ):
            solidToIntersect = toIntersect.val()
        elif isinstance(toIntersect, Solid):
            solidToIntersect = toIntersect
        else:
            raise ValueError("Cannot intersect type '{}'".format(type(toIntersect)))

        newS = solidRef.intersect(solidToIntersect)

        if clean: newS = newS.clean()

        if combine:
            solidRef.wrapped = newS.wrapped

        return self.newObject([newS])


    def cutBlind(self, distanceToCut, clean=True):
        """
        Use all un-extruded wires in the parent chain to create a prismatic cut from existing solid.

        Similar to extrude, except that a solid in the parent chain is required to remove material
        from. cutBlind always removes material from a part.

        :param distanceToCut: distance to extrude before cutting
        :type distanceToCut: float, >0 means in the positive direction of the workplane normal,
            <0 means in the negative direction
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape
        :raises: ValueError if there is no solid to subtract from in the chain
        :return: a CQ object with the resulting object selected

        see :py:meth:`cutThruAll` to cut material from the entire part

        Future Enhancements:
            Cut Up to Surface
        """
        #first, make the object
        toCut = self._extrude(distanceToCut)

        #now find a solid in the chain

        solidRef = self.findSolid()

        s = solidRef.cut(toCut)

        if clean: s = s.clean()

        solidRef.wrapped = s.wrapped
        return self.newObject([s])

    def cutThruAll(self, positive=False, clean=True):
        """
        Use all un-extruded wires in the parent chain to create a prismatic cut from existing solid.

        Similar to extrude, except that a solid in the parent chain is required to remove material
        from. cutThruAll always removes material from a part.

        :param boolean positive: True to cut in the positive direction, false to cut in the
            negative direction
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape
        :raises: ValueError if there is no solid to subtract from in the chain
        :return: a CQ object with the resulting object selected

        see :py:meth:`cutBlind` to cut material to a limited depth
        """
        maxDim = self.largestDimension()
        if not positive:
            maxDim *= (-1.0)

        return self.cutBlind(maxDim, clean)

    def loft(self, filled=True, ruled=False, combine=True):
        """
        Make a lofted solid, through the set of wires.
        :return: a CQ object containing the created loft
        """
        wiresToLoft = self.ctx.pendingWires
        self.ctx.pendingWires = []

        r = Solid.makeLoft(wiresToLoft, ruled)

        if combine:
            parentSolid = self.findSolid(searchStack=False, searchParents=True)
            if parentSolid is not None:
                r = parentSolid.fuse(r)
                parentSolid.wrapped = r.wrapped

        return self.newObject([r])

    def _extrude(self, distance, both=False):
        """
        Make a prismatic solid from the existing set of pending wires.

        :param distance: distance to extrude
        :param boolean both: extrude in both directions symmetrically
        :return: a FreeCAD solid, suitable for boolean operations.

        This method is a utility method, primarily for plugin and internal use.
        It is the basis for cutBlind,extrude,cutThruAll, and all similar methods.

        Future Enhancements:
            extrude along a profile (sweep)
        """

        #group wires together into faces based on which ones are inside the others
        #result is a list of lists
        s = time.time()
        wireSets = sortWiresByBuildOrder(list(self.ctx.pendingWires), self.plane, [])
        #print "sorted wires in %d sec" % ( time.time() - s )
        self.ctx.pendingWires = []  # now all of the wires have been used to create an extrusion

        #compute extrusion vector and extrude
        eDir = self.plane.zDir.multiply(distance)


        #one would think that fusing faces into a compound and then extruding would work,
        #but it doesnt-- the resulting compound appears to look right, ( right number of faces, etc)
        #but then cutting it from the main solid fails with BRep_NotDone.
        #the work around is to extrude each and then join the resulting solids, which seems to work

        # underlying cad kernel can only handle simple bosses-- we'll aggregate them if there are
        # multiple sets

        # IMPORTANT NOTE: OCC is slow slow slow in boolean operations.  So you do NOT want to fuse
        # each item to another and save the result-- instead, you want to combine all of the new
        # items into a compound, and fuse them together!!!
        # r = None
        # for ws in wireSets:
        #     thisObj = Solid.extrudeLinear(ws[0], ws[1:], eDir)
        #     if r is None:
        #         r = thisObj
        #     else:
        #         s = time.time()
        #         r = r.fuse(thisObj)
        #         print "Fused in %0.3f sec" % ( time.time() - s )
        # return r

        toFuse = []
        for ws in wireSets:
            thisObj = Solid.extrudeLinear(ws[0], ws[1:], eDir)
            toFuse.append(thisObj)

            if both:
                thisObj = Solid.extrudeLinear(ws[0], ws[1:], eDir.multiply(-1.))
                toFuse.append(thisObj)

        return Compound.makeCompound(toFuse)

    def _revolve(self, angleDegrees, axisStart, axisEnd):
        """
        Make a solid from the existing set of pending wires.

        :param angleDegrees: the angle to revolve through.
        :type angleDegrees: float, anything less than 360 degrees will leave the shape open
        :param axisStart: the start point of the axis of rotation
        :type axisStart: tuple, a two tuple
        :param axisEnd: the end point of the axis of rotation
        :type axisEnd: tuple, a two tuple
        :return: a FreeCAD solid, suitable for boolean operations.

        This method is a utility method, primarily for plugin and internal use.
        """
        #We have to gather the wires to be revolved
        wireSets = sortWiresByBuildOrder(list(self.ctx.pendingWires), self.plane, [])

        #Mark that all of the wires have been used to create a revolution
        self.ctx.pendingWires = []

        #Revolve the wires, make a compound out of them and then fuse them
        toFuse = []
        for ws in wireSets:
            thisObj = Solid.revolve(ws[0], ws[1:], angleDegrees, axisStart, axisEnd)
            toFuse.append(thisObj)

        return Compound.makeCompound(toFuse)

    def _sweep(self, path, sweepAlongWires=False, makeSolid=True, isFrenet=False):
        """
        Makes a swept solid from an existing set of pending wires.

        :param path: A wire along which the pending wires will be swept
        :param boolean sweepAlongWires:
            False to create multiple swept from wires on the chain along path
            True to create only one solid swept along path with shape following the list of wires on the chain
        :return:a FreeCAD solid, suitable for boolean operations
        """

        # group wires together into faces based on which ones are inside the others
        # result is a list of lists
        s = time.time()
        wireSets = sortWiresByBuildOrder(list(self.ctx.pendingWires), self.plane, [])
        # print "sorted wires in %d sec" % ( time.time() - s )
        self.ctx.pendingWires = []  # now all of the wires have been used to create an extrusion

        toFuse = []
        if not sweepAlongWires:
            for ws in wireSets:
                thisObj = Solid.sweep(ws[0], ws[1:], path.val(), makeSolid, isFrenet)
                toFuse.append(thisObj)
        else:
            section = []
            for ws in wireSets:
                for i in range(0, len(ws)):
                    section.append(ws[i])

            # implementation
            outW = Wire(section[0].wrapped)
            inW = section[1:]
            thisObj = Solid.sweep(outW, inW, path.val(), makeSolid, isFrenet)
            toFuse.append(thisObj)

        return Compound.makeCompound(toFuse)

    def box(self, length, width, height, centered=(True, True, True), combine=True, clean=True):
        """
        Return a 3d box with specified dimensions for each object on the stack.

        :param length: box size in X direction
        :type length: float > 0
        :param width: box size in Y direction
        :type width: float > 0
        :param height: box size in Z direction
        :type height: float > 0
        :param centered: should the box be centered, or should reference point be at the lower
            bound of the range?
        :param combine: should the results be combined with other solids on the stack
            (and each other)?
        :type combine: true to combine shapes, false otherwise.
        :param boolean clean: call :py:meth:`clean` afterwards to have a clean shape

        Centered is a tuple that describes whether the box should be centered on the x,y, and
        z axes.  If true, the box is centered on the respective axis relative to the workplane
        origin, if false, the workplane center will represent the lower bound of the resulting box

        one box is created for each item on the current stack. If no items are on the stack, one box
        using the current workplane center is created.

        If combine is true, the result will be a single object on the stack:
            if a solid was found in the chain, the result is that solid with all boxes produced
            fused onto it otherwise, the result is the combination of all the produced boxes

        if combine is false, the result will be a list of the boxes produced

        Most often boxes form the basis for a part::

            #make a single box with lower left corner at origin
            s = Workplane().box(1,2,3,centered=(False,False,False)

        But sometimes it is useful to create an array of them:

            #create 4 small square bumps on a larger base plate:
            s = Workplane().box(4,4,0.5).faces(">Z").workplane()\
                .rect(3,3,forConstruction=True).vertices().box(0.25,0.25,0.25,combine=True)

        """

        def _makebox(pnt):

            #(xp,yp,zp) = self.plane.toLocalCoords(pnt)
            (xp, yp, zp) = pnt.toTuple()
            if centered[0]:
                xp -= (length / 2.0)
            if centered[1]:
                yp -= (width / 2.0)
            if centered[2]:
                zp -= (height / 2.0)

            return Solid.makeBox(length, width, height, Vector(xp, yp, zp))

        boxes = self.eachpoint(_makebox, True)

        #if combination is not desired, just return the created boxes
        if not combine:
            return boxes
        else:
            #combine everything
            return self.union(boxes, clean=clean)

    def sphere(self, radius, direct=(0, 0, 1), angle1=-90, angle2=90, angle3=360,
               centered=(True, True, True), combine=True, clean=True):
        """
        Returns a 3D sphere with the specified radius for each point on the stack

        :param radius: The radius of the sphere
        :type radius: float > 0
        :param direct: The direction axis for the creation of the sphere
        :type direct: A three-tuple
        :param angle1: The first angle to sweep the sphere arc through
        :type angle1: float > 0
        :param angle2: The second angle to sweep the sphere arc through
        :type angle2: float > 0
        :param angle3: The third angle to sweep the sphere arc through
        :type angle3: float > 0
        :param centered: A three-tuple of booleans that determines whether the sphere is centered
            on each axis origin
        :param combine: Whether the results should be combined with other solids on the stack
            (and each other)
        :type combine: true to combine shapes, false otherwise
        :return: A sphere object for each point on the stack

        Centered is a tuple that describes whether the sphere should be centered on the x,y, and
        z axes.  If true, the sphere is centered on the respective axis relative to the workplane
        origin, if false, the workplane center will represent the lower bound of the resulting
        sphere.

        One sphere is created for each item on the current stack. If no items are on the stack, one
        box using the current workplane center is created.

        If combine is true, the result will be a single object on the stack:
            If a solid was found in the chain, the result is that solid with all spheres produced
            fused onto it otherwise, the result is the combination of all the produced boxes

        If combine is false, the result will be a list of the spheres produced
        """

        # Convert the direction tuple to a vector, if needed
        if isinstance(direct, tuple):
            direct = Vector(direct)

        def _makesphere(pnt):
            """
            Inner function that is used to create a sphere for each point/object on the workplane
            :param pnt: The center point for the sphere
            :return: A CQ Solid object representing a sphere
            """
            (xp, yp, zp) = pnt.toTuple()

            if not centered[0]:
                xp += radius

            if not centered[1]:
                yp += radius

            if not centered[2]:
                zp += radius

            return Solid.makeSphere(radius, Vector(xp, yp, zp), direct, angle1, angle2, angle3)

        # We want a sphere for each point on the workplane
        spheres = self.eachpoint(_makesphere, True)

        # If we don't need to combine everything, just return the created spheres
        if not combine:
            return spheres
        else:
            return self.union(spheres, clean=clean)

    def clean(self):
        """
        Cleans the current solid by removing unwanted edges from the
        faces.

        Normally you don't have to call this function. It is
        automatically called after each related operation. You can
        disable this behavior with `clean=False` parameter if method
        has any. In some cases this can improve performance
        drastically but is generally dis-advised since it may break
        some operations such as fillet.

        Note that in some cases where lots of solid operations are
        chained, `clean()` may actually improve performance since
        the shape is 'simplified' at each step and thus next operation
        is easier.

        Also note that, due to limitation of the underlying engine,
        `clean` may fail to produce a clean output in some cases such as
        spherical faces.
        """
        try:
            cleanObjects = [obj.clean() for obj in self.objects]
        except AttributeError:
            raise AttributeError("%s object doesn't support `clean()` method!" % obj.ShapeType())
        return self.newObject(cleanObjects)
