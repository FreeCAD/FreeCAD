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

import FreeCAD as App
import DraftVecUtils

from draftutils.utils import get_param
from draftutils.messages import _msg, _wrn
from draftutils.translate import _tr, translate

from draftobjects.draftlink import DraftLink

class PathArray(DraftLink):
    """The Draft Path Array object - distributes copies of an object along a path.
    Original mode is the historic "Align" for old (v0.18) documents.  It is not 
    really the Fernat alignment. Uses the normal parameter from getNormal (or the
    default) as a constant - it does not calculate curve normal.
    X is curve tangent, Y is normal parameter, Z is (X x Y)

    Tangent mode is similar to Original, but includes a pre-rotation (in execute) to 
    align the Base object's X to the TangentVector, then X follows curve tangent, 
    normal input parameter is the Z component.

    If the ForceVertical option is applied, the normal parameter from getNormal is 
    ignored, and X is curve tangent, Z is VerticalVector, Y is (X x Z)

    Frenet mode orients the copies to a coordinate system along the path.
    X is tangent to curve, Y is curve normal, Z is curve binormal. 
    if normal can not be computed (ex a straight line), the default is used."""
    
    def __init__(self, obj):
        super(PathArray, self).__init__(obj, "PathArray")

    #For PathLinkArray, DraftLink.attach creates the link to the Base object.
    def attach(self,obj):
        self.setProperties(obj)
        super(PathArray, self).attach(obj) 

    def setProperties(self,obj):
        if not obj:
            return
        if hasattr(obj, "PropertiesList"):
            pl = obj.PropertiesList
        else:
            pl = []

        if not "Base" in pl:
            _tip = _tr("The base object that must be duplicated")
            obj.addProperty("App::PropertyLinkGlobal", "Base", "Objects",  _tip)

        if not "PathObj" in pl:
            _tip = _tr("The path object along which to distribute objects")
            obj.addProperty("App::PropertyLinkGlobal", "PathObj", "Objects", _tip)

        if not "PathSubs" in pl:
            _tip = _tr("Selected subobjects (edges) of PathObj")
            obj.addProperty("App::PropertyLinkSubListGlobal", "PathSubs", "Objects", _tip)
            obj.PathSubs = []
            
        if not "Count" in pl:
            _tip = _tr("Number of copies")
            obj.addProperty("App::PropertyInteger", "Count", "Parameters", _tip)
            obj.Count = 2

# copy alignment properties
        if not "Align" in pl:
            _tip = _tr("Orient the copies along path")
            obj.addProperty("App::PropertyBool", "Align", "Alignment", _tip)
            obj.Align = False
            
        if not "AlignMode" in pl:
            _tip = _tr("How to orient copies on path")
            obj.addProperty("App::PropertyEnumeration","AlignMode","Alignment", _tip)
            obj.AlignMode = ['Original','Frenet','Tangent']
            obj.AlignMode = 'Original'
            
        if not "Xlate" in pl:
            _tip = _tr("Optional translation vector")
            obj.addProperty("App::PropertyVectorDistance","Xlate","Alignment", _tip)
            obj.Xlate = App.Vector(0,0,0)
            
        if not "TangentVector" in pl:
            _tip = _tr("Alignment vector for Tangent mode")
            obj.addProperty("App::PropertyVector","TangentVector","Alignment", _tip)
            obj.TangentVector = App.Vector(1,0,0)

        if not "ForceVertical" in pl:
            _tip = _tr("Force Original/Tangent modes to use VerticalVector as Z")
            obj.addProperty("App::PropertyBool","ForceVertical","Alignment", _tip)
            obj.ForceVertical = False

        if not "VerticalVector" in pl:
            _tip = _tr("ForceVertical direction")
            obj.addProperty("App::PropertyVector","VerticalVector","Alignment", _tip)
            obj.VerticalVector = App.Vector(0,0,1)

        if self.use_link and "ExpandArray" not in pl:
            _tip = _tr("Show array element as children object")
            obj.addProperty("App::PropertyBool","ExpandArray", "Parameters", _tip)
            obj.ExpandArray = False
            obj.setPropertyStatus('Shape','Transient')

    def linkSetup(self,obj):
        super(PathArray, self).linkSetup(obj)
        obj.configLinkProperty(ElementCount='Count')

    def execute(self,obj):
        import Part
        import DraftGeomUtils
        if obj.Base and obj.PathObj:
            pl = obj.Placement                  #placement of whole pathArray
            if obj.PathSubs:
                w = self.getWireFromSubs(obj)
            elif (hasattr(obj.PathObj.Shape,'Wires') and obj.PathObj.Shape.Wires):
                w = obj.PathObj.Shape.Wires[0]
            elif obj.PathObj.Shape.Edges:
                w = Part.Wire(obj.PathObj.Shape.Edges)
            else:
                App.Console.PrintLog ("PathArray.execute: path " + obj.PathObj.Name + " has no edges\n")
                return
            if (hasattr(obj, "TangentVector")) and (obj.AlignMode == "Tangent") and (obj.Align):
                basePlacement = obj.Base.Shape.Placement
                baseRotation = basePlacement.Rotation
                stdX = App.Vector(1.0, 0.0, 0.0)                          #default TangentVector
                if (not DraftVecUtils.equals(stdX, obj.TangentVector)):
                    preRotation = App.Rotation(stdX, obj.TangentVector)   #make rotation from X to TangentVector
                    netRotation = baseRotation.multiply(preRotation)
                else:
                    netRotation = baseRotation
                base = calculatePlacementsOnPath(
                        netRotation,w,obj.Count,obj.Xlate,obj.Align, obj.AlignMode, 
                            obj.ForceVertical, obj.VerticalVector)
            else:
                base = calculatePlacementsOnPath(
                        obj.Base.Shape.Placement.Rotation,w,obj.Count,obj.Xlate,obj.Align, obj.AlignMode,
                            obj.ForceVertical, obj.VerticalVector)

            #shape needs to be built before colors assigned
            rc = super(PathArray, self).buildShape(obj, pl, base)
            #match count of DiffuseColor entries to count of copies
            vobdc = obj.Base.ViewObject.DiffuseColor
            dc = vobdc
            for x in range(obj.Count - 1):
                dc =  dc + vobdc
            obj.ViewObject.DiffuseColor = dc

            return rc

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

    def onDocumentRestored(self, obj):
        self.migrate_attributes(obj)
        self.setProperties(obj)

        if self.use_link:
            self.linkSetup(obj)
        else:
            obj.setPropertyStatus('Shape','-Transient')
        if obj.Shape.isNull():
            if getattr(obj,'PlacementList',None):
                self.buildShape(obj,obj.Placement,obj.PlacementList)
            else:
                self.execute(obj)

_PathArray = PathArray

def calculatePlacementsOnPath(shapeRotation, pathwire, count, xlate, align,
        mode = 'Original', forceNormal=False, normalOverride=None):
    """Calculates the placements of a shape along a given path so that each copy will be distributed evenly"""
    import Part
    import DraftGeomUtils
    closedpath = DraftGeomUtils.isReallyClosed(pathwire)

    normal = DraftGeomUtils.getNormal(pathwire)
    if forceNormal and normalOverride:
            normal = normalOverride

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
        shapeRotation, path[0], 0, pt, xlate, align, normal, mode, forceNormal))

    # closed path doesn't need shape on last vertex
    if not(closedpath):
        # place the end shape
        pt = path[-1].Vertexes[-1].Point
        placements.append(calculatePlacement(
            shapeRotation, path[-1], path[-1].Length, pt, xlate, align, normal, mode, forceNormal))

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
            shapeRotation, path[iend], offset, pt, xlate, align, normal, mode, forceNormal))

        travel += step

    return placements

def calculatePlacement(globalRotation, edge, offset, RefPt, xlate, align, normal=None, 
        mode = 'Original', overrideNormal=False):
    """Orient shape to a local coord system (tangent, normal, binormal) at parameter offset (normally length)"""
    import functools
    # http://en.wikipedia.org/wiki/Euler_angles  (previous version)
    # http://en.wikipedia.org/wiki/Quaternions
    # start with null Placement point so _tr goes to right place.
    placement = App.Placement()
    # preserve global orientation
    placement.Rotation = globalRotation

    placement.move(RefPt + xlate)
    if not align:
        return placement

    nullv = App.Vector(0, 0, 0)
    defNormal = App.Vector(0.0, 0.0, 1.0)
    if not normal is None:
        defNormal = normal

    try:
        t = edge.tangentAt(getParameterFromV0(edge, offset))
        t.normalize()
    except:
        _msg("Draft CalculatePlacement - Cannot calculate Path tangent. Copy not aligned\n")
        return placement

    if (mode == 'Original') or (mode == 'Tangent'):
        if normal is None:
            n = defNormal 
        else:
            n = normal
            n.normalize()
        try:
            b = t.cross(n)
            b.normalize()
        except:                             # weird special case. tangent & normal parallel
            b = nullv
            _msg("PathArray computePlacement - parallel tangent, normal. Copy not aligned\n")
            return placement
        if overrideNormal:
            priority = "XZY"
            newRot = App.Rotation(t, b, n, priority);    #t/x, b/y, n/z
        else:        
            priority = "XZY"    #must follow X, try to follow Z, Y is what it is
            newRot = App.Rotation(t, n, b, priority);
    elif mode == 'Frenet':
        try:
            n = edge.normalAt(getParameterFromV0(edge, offset))
            n.normalize()
        except App.Base.FreeCADError:   # no/infinite normals here
            n = defNormal
            _msg("PathArray computePlacement - Cannot calculate Path normal, using default\n")
        try:
            b = t.cross(n)
            b.normalize()
        except:
            b = nullv
            _msg("Draft PathArray.orientShape - Cannot calculate Path biNormal. Copy not aligned\n")
            return placement
        priority = "XZY"                            
        newRot = App.Rotation(t, n, b, priority);    #t/x, n/y, b/z
    else:
        _msg(_tr("AlignMode {} is not implemented".format(mode)))
        return placement
        
    #have valid t, n, b
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
