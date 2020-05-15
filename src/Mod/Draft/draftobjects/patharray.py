# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""This module provides the object code for the Draft PathArray object.
"""
## @package patharray
# \ingroup DRAFT
# \brief This module provides the object code for the Draft PathArray object.

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftVecUtils

from draftutils.utils import get_param

from draftobjects.draftlink import DraftLink


class PathArray(DraftLink):
    """The Draft Path Array object"""

    def __init__(self,obj):
        super(PathArray, self).__init__(obj, "PathArray")

    def attach(self,obj):
        _tip = "The base object that must be duplicated"
        obj.addProperty("App::PropertyLinkGlobal", "Base", 
                        "Objects", QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "The path object along which to distribute objects"
        obj.addProperty("App::PropertyLinkGlobal", "PathObj",
                        "Objects", QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Selected subobjects (edges) of PathObj"
        obj.addProperty("App::PropertyLinkSubListGlobal", "PathSubs",
                        "Objects", QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Number of copies"
        obj.addProperty("App::PropertyInteger", "Count",
                        "Parameters", QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Orientation of Base along path"
        obj.addProperty("App::PropertyBool", "Align",
                        "Parameters", QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Alignment of copies"
        obj.addProperty("App::PropertyVector", "TangentVector",
                        "Parameters", QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Optional translation vector" # why? placement is not enough?
        obj.addProperty("App::PropertyVectorDistance", "Xlate",
                        "Parameters", QT_TRANSLATE_NOOP("App::Property", _tip))

        obj.Count = 2
        obj.PathSubs = []
        obj.Xlate = App.Vector(0,0,0)
        obj.Align = False
        obj.TangentVector = App.Vector(1.0, 0.0, 0.0)

        if self.use_link:
            _tip = "Show array element as children object"
            obj.addProperty("App::PropertyBool","ExpandArray",
                            "Parameters", QT_TRANSLATE_NOOP("App::Property", _tip))

            obj.ExpandArray = False
            obj.setPropertyStatus('Shape','Transient')

        super(PathArray, self).attach(obj)

    def linkSetup(self,obj):
        super(PathArray, self).linkSetup(obj)
        obj.configLinkProperty(ElementCount='Count')

    def execute(self,obj):
        import Part
        import DraftGeomUtils
        if obj.Base and obj.PathObj:
            pl = obj.Placement
            if obj.PathSubs:
                w = self.getWireFromSubs(obj)
            elif (hasattr(obj.PathObj.Shape,'Wires') and obj.PathObj.Shape.Wires):
                w = obj.PathObj.Shape.Wires[0]
            elif obj.PathObj.Shape.Edges:
                w = Part.Wire(obj.PathObj.Shape.Edges)
            else:
                App.Console.PrintLog ("_PathArray.createGeometry: path " + obj.PathObj.Name + " has no edges\n")
                return
            if (hasattr(obj, "TangentVector")) and (obj.Align):
                basePlacement = obj.Base.Shape.Placement
                baseRotation = basePlacement.Rotation
                stdX = App.Vector(1.0, 0.0, 0.0)
                preRotation = App.Rotation(stdX, obj.TangentVector)   #make rotation from X to TangentVector
                netRotation = baseRotation.multiply(preRotation)
                base = calculatePlacementsOnPath(
                        netRotation,w,obj.Count,obj.Xlate,obj.Align)
            else:
                base = calculatePlacementsOnPath(
                        obj.Base.Shape.Placement.Rotation,w,obj.Count,obj.Xlate,obj.Align)
            return super(PathArray, self).buildShape(obj, pl, base)

    def getWireFromSubs(self,obj):
        '''Make a wire from PathObj subelements'''
        import Part
        sl = []
        for sub in obj.PathSubs:
            edgeNames = sub[1]
            for n in edgeNames:
                e = sub[0].Shape.getElement(n)
                sl.append(e)
        return Part.Wire(sl)

    def pathArray(self,shape,pathwire,count,xlate,align):
        '''Distribute shapes along a path.'''
        import Part

        placements = calculatePlacementsOnPath(
            shape.Placement.Rotation, pathwire, count, xlate, align)

        base = []

        for placement in placements:
            ns = shape.copy()
            ns.Placement = placement

            base.append(ns)

        return (Part.makeCompound(base))


_PathArray = PathArray


def calculatePlacementsOnPath(shapeRotation, pathwire, count, xlate, align):
    """Calculates the placements of a shape along a given path so that each copy will be distributed evenly"""
    import Part
    import DraftGeomUtils

    closedpath = DraftGeomUtils.isReallyClosed(pathwire)
    normal = DraftGeomUtils.getNormal(pathwire)
    path = Part.__sortEdges__(pathwire.Edges)
    ends = []
    cdist = 0

    for e in path:                                                 # find cumulative edge end distance
        cdist += e.Length
        ends.append(cdist)

    placements = []

    # place the start shape
    pt = path[0].Vertexes[0].Point
    placements.append(calculatePlacement(
        shapeRotation, path[0], 0, pt, xlate, align, normal))

    # closed path doesn't need shape on last vertex
    if not(closedpath):
        # place the end shape
        pt = path[-1].Vertexes[-1].Point
        placements.append(calculatePlacement(
            shapeRotation, path[-1], path[-1].Length, pt, xlate, align, normal))

    if count < 3:
        return placements

    # place the middle shapes
    if closedpath:
        stop = count
    else:
        stop = count - 1
    step = float(cdist) / stop
    remains = 0
    travel = step
    for i in range(1, stop):
        # which edge in path should contain this shape?
        # avoids problems with float math travel > ends[-1]
        iend = len(ends) - 1

        for j in range(0, len(ends)):
            if travel <= ends[j]:
                iend = j
                break

        # place shape at proper spot on proper edge
        remains = ends[iend] - travel
        offset = path[iend].Length - remains
        pt = path[iend].valueAt(getParameterFromV0(path[iend], offset))

        placements.append(calculatePlacement(
            shapeRotation, path[iend], offset, pt, xlate, align, normal))

        travel += step

    return placements



def calculatePlacement(globalRotation, edge, offset, RefPt, xlate, align, normal=None):
    """Orient shape to tangent at parm offset along edge."""
    import functools
    # http://en.wikipedia.org/wiki/Euler_angles
    # start with null Placement point so translate goes to right place.
    placement = App.Placement()
    # preserve global orientation
    placement.Rotation = globalRotation

    placement.move(RefPt + xlate)

    if not align:
        return placement

    nullv = App.Vector(0, 0, 0)

    # get a local coord system (tangent, normal, binormal) at parameter offset (normally length)
    t = edge.tangentAt(getParameterFromV0(edge, offset))
    t.normalize()

    try:
        n = edge.normalAt(getParameterFromV0(edge, offset))
        n.normalize()
        b = (t.cross(n))
        b.normalize()
    # no normal defined here
    except App.Base.FreeCADError:
        n = nullv
        b = nullv
        App.Console.PrintMessage(
            "Draft PathArray.orientShape - Cannot calculate Path normal.\n")

    priority = "ZXY"        #the default. Doesn't seem to affect results.
    newRot = App.Rotation(t, n, b, priority)
    newGRot = newRot.multiply(globalRotation)

    placement.Rotation = newGRot
    return placement



def getParameterFromV0(edge, offset):
    """return parameter at distance offset from edge.Vertexes[0]
    sb method in Part.TopoShapeEdge???"""

    lpt = edge.valueAt(edge.getParameterByLength(0))
    vpt = edge.Vertexes[0].Point

    if not DraftVecUtils.equals(vpt, lpt):
        # this edge is flipped
        length = edge.Length - offset
    else:
        # this edge is right way around
        length = offset

    return (edge.getParameterByLength(length))
