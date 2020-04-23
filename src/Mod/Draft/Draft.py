# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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
"""Provide the Draft Workbench public programming interface.

The Draft module offers tools to create and manipulate 2D objects.
The functions in this file must be usable without requiring the
graphical user interface.
These functions can be used as the backend for the graphical commands
defined in `DraftTools.py`.
"""
## \addtogroup DRAFT
#  \brief Create and manipulate basic 2D objects
#
#  This module offers tools to create and manipulate basic 2D objects
#
#  The module allows to create 2D geometric objects such as line, rectangle,
#  circle, etc., modify these objects by moving, scaling or rotating them,
#  and offers a couple of other utilities to manipulate further these objects,
#  such as decompose them (downgrade) into smaller elements.
#
#  The functionality of the module is divided into GUI tools, usable from the
#  visual interface, and corresponding python functions, that can perform
#  the same operation programmatically.
#
#  @{

import math
import sys
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD
from FreeCAD import Vector

import DraftVecUtils
import WorkingPlane
from draftutils.translate import translate

if FreeCAD.GuiUp:
    import FreeCADGui
    import Draft_rc
    gui = True
    # To prevent complaints from code checkers (flake8)
    True if Draft_rc.__name__ else False
else:
    gui = False

__title__ = "FreeCAD Draft Workbench"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin, Daniel Falck")
__url__ = "https://www.freecadweb.org"

# ---------------------------------------------------------------------------
# Backwards compatibility
# ---------------------------------------------------------------------------
from DraftLayer import Layer as _VisGroup
from DraftLayer import ViewProviderLayer as _ViewProviderVisGroup
from DraftLayer import makeLayer

# import DraftFillet
# Fillet = DraftFillet.Fillet
# makeFillet = DraftFillet.makeFillet

# ---------------------------------------------------------------------------
# General functions
# ---------------------------------------------------------------------------
from draftutils.utils import ARROW_TYPES as arrowtypes

from draftutils.utils import stringencodecoin
from draftutils.utils import string_encode_coin

from draftutils.utils import typecheck
from draftutils.utils import type_check

from draftutils.utils import getParamType
from draftutils.utils import get_param_type

from draftutils.utils import getParam
from draftutils.utils import get_param

from draftutils.utils import setParam
from draftutils.utils import set_param

from draftutils.utils import precision
from draftutils.utils import tolerance
from draftutils.utils import epsilon

from draftutils.utils import getRealName
from draftutils.utils import get_real_name

from draftutils.utils import getType
from draftutils.utils import get_type

from draftutils.utils import getObjectsOfType
from draftutils.utils import get_objects_of_type

from draftutils.utils import isClone
from draftutils.utils import is_clone

from draftutils.utils import getGroupNames
from draftutils.utils import get_group_names

from draftutils.utils import ungroup

from draftutils.utils import getGroupContents
from draftutils.utils import get_group_contents

from draftutils.utils import printShape
from draftutils.utils import print_shape

from draftutils.utils import compareObjects
from draftutils.utils import compare_objects

from draftutils.utils import shapify

from draftutils.utils import loadSvgPatterns
from draftutils.utils import load_svg_patterns

from draftutils.utils import svgpatterns
from draftutils.utils import svg_patterns

from draftutils.utils import getMovableChildren
from draftutils.utils import get_movable_children

from draftutils.utils import filterObjectsForModifiers
from draftutils.utils import filter_objects_for_modifiers

from draftutils.gui_utils import get3DView
from draftutils.gui_utils import get_3d_view

from draftutils.gui_utils import autogroup

from draftutils.gui_utils import dimSymbol
from draftutils.gui_utils import dim_symbol

from draftutils.gui_utils import dimDash
from draftutils.gui_utils import dim_dash

from draftutils.gui_utils import removeHidden
from draftutils.gui_utils import remove_hidden

from draftutils.gui_utils import formatObject
from draftutils.gui_utils import format_object

from draftutils.gui_utils import getSelection
from draftutils.gui_utils import get_selection

from draftutils.gui_utils import getSelectionEx
from draftutils.gui_utils import get_selection_ex

from draftutils.gui_utils import select

from draftutils.gui_utils import loadTexture
from draftutils.gui_utils import load_texture

#---------------------------------------------------------------------------
# Draft objects and functions
#---------------------------------------------------------------------------

import draftobjects.base
_DraftObject = draftobjects.base.DraftObject

import draftviewproviders.view_base
_ViewProviderDraft = draftviewproviders.view_base.ViewProviderDraft
_ViewProviderDraftAlt = draftviewproviders.view_base.ViewProviderDraftAlt
_ViewProviderDraftPart = draftviewproviders.view_base.ViewProviderDraftPart

#---------------------------------------------------------------------------

from draftmake.make_circle import  make_circle
from draftobjects.circle import Circle

makeCircle = make_circle
_Circle = Circle

# Circle object uses view_base.ViewProviderDraft

#---------------------------------------------------------------------------

from draftmake.make_rectangle import make_rectangle
from draftobjects.rectangle import Rectangle

makeRectangle = make_rectangle
_Rectangle = Rectangle

if gui:
    from draftviewproviders.view_rectangle import ViewProviderRectangle
    _ViewProviderRectangle = ViewProviderRectangle

#---------------------------------------------------------------------------

from draftmake.make_polygon import make_polygon
from draftobjects.polygon import Polygon

makePolygon = make_polygon
_Polygon = Polygon

# Polygon object uses view_base.ViewProviderDraft

#---------------------------------------------------------------------------

from draftmake.make_line import make_line
from draftmake.make_wire import make_wire
from draftobjects.wire import Wire

makeLine = make_line
makeWire = make_wire
_Wire = Wire

if gui:
    from draftviewproviders.view_wire import ViewProviderWire
    # FC v < 0.19: BSpline and BezCurve Objects used _ViewProviderWire
    _ViewProviderWire = ViewProviderWire

#---------------------------------------------------------------------------

from draftmake.make_bezcurve import make_bezcurve
from draftobjects.bezcurve import BezCurve

makeBezCurve = make_bezcurve
_BezCurve = BezCurve

if gui:
    from draftviewproviders.view_bezcurve import ViewProviderBezCurve
    # probably FC v < 0.18 : BezCurve Object used _ViewProviderBezCurve
    _ViewProviderBezCurve = ViewProviderBezCurve

#---------------------------------------------------------------------------

from draftmake.make_bspline import make_bspline
from draftobjects.bspline import BSpline

makeBSpline = make_bspline
_BSpline = BSpline

if gui:
    from draftviewproviders.view_bspline import ViewProviderBSpline
    # probably FC v < 0.18 : BSpline Object used _ViewProviderBSpline
    _ViewProviderBSpline = ViewProviderBSpline 

#---------------------------------------------------------------------------

from draftmake.make_point import make_point
from draftobjects.point import Point

makePoint = make_point
_Point = Point

if gui:
    from draftviewproviders.view_point import ViewProviderPoint
    _ViewProviderPoint = ViewProviderPoint

#---------------------------------------------------------------------------

from draftmake.make_ellipse import  make_ellipse
from draftobjects.ellipse import Ellipse

makeEllipse = make_ellipse
_Ellipse = Ellipse

# Ellipse object uses view_base.ViewProviderDraft

#---------------------------------------------------------------------------

from draftmake.make_clone import make_clone
from draftobjects.clone import Clone

clone = make_clone
_Clone = Clone

if gui:
    from draftviewproviders.view_clone import ViewProviderClone
    _ViewProviderClone = ViewProviderClone

#---------------------------------------------------------------------------

from draftmake.make_block import make_block
from draftobjects.block import Block

makeBlock = make_block
_Block = Block
#---------------------------------------------------------------------------

from draftmake.make_facebinder import make_facebinder
from draftobjects.facebinder import Facebinder

makeFacebinder = make_facebinder

if gui:
    from draftviewproviders.view_facebinder import ViewProviderFacebinder

#---------------------------------------------------------------------------

from draftmake.make_shapestring import make_shapestring
from draftobjects.shapestring import ShapeString

makeShapeString = make_shapestring
_ShapeString = ShapeString

if gui:
    from draftviewproviders.view_facebinder import ViewProviderFacebinder

#---------------------------------------------------------------------------

from draftmake.make_wpproxy import make_working_plane_proxy
from draftobjects.wpproxy import WorkingPlaneProxy

makeWorkingPlaneProxy = make_working_plane_proxy

if gui:
    from draftviewproviders.view_wpproxy import ViewProviderWorkingPlaneProxy

#---------------------------------------------------------------------------

from draftobjects.draftlink import DraftLink
_DraftLink = DraftLink

if gui:
    from draftviewproviders.view_draftlink import ViewProviderDraftLink
    _ViewProviderDraftLink = ViewProviderDraftLink

#---------------------------------------------------------------------------

from draftmake.make_shape2dview import make_shape2dview
from draftobjects.shape2dview import Shape2DView

makeShape2DView = make_shape2dview
_Shape2DView = Shape2DView

#---------------------------------------------------------------------------

from draftmake.make_sketch import make_sketch

makeSketch = make_sketch

#---------------------------------------------------------------------------

from draftmake.make_copy import make_copy
from draftmake.make_copy import makeCopy

#---------------------------------------------------------------------------
# Draft annotation objects
#---------------------------------------------------------------------------

from draftobjects.dimension import make_dimension, make_angular_dimension
from draftobjects.dimension import LinearDimension, AngularDimension

makeDimension = make_dimension
makeAngularDimension = make_angular_dimension
_Dimension = LinearDimension
_AngularDimension = AngularDimension

if gui:
    from draftviewproviders.view_dimension import ViewProviderLinearDimension
    from draftviewproviders.view_dimension import ViewProviderAngularDimension
    _ViewProviderDimension = ViewProviderLinearDimension
    _ViewProviderAngularDimension = ViewProviderAngularDimension

#---------------------------------------------------------------------------

from draftobjects.label import make_label
from draftobjects.label import Label

makeLabel = make_label
DraftLabel = Label

if gui:
    from draftviewproviders.view_label import ViewProviderLabel
    ViewProviderDraftLabel = ViewProviderLabel

#---------------------------------------------------------------------------

from draftobjects.text import make_text
from draftobjects.text import Text

makeText = make_text
DraftText = Text

if gui:
    from draftviewproviders.view_text import ViewProviderText
    ViewProviderDraftText = ViewProviderText

#---------------------------------------------------------------------------

def convertDraftTexts(textslist=[]):
    """
    converts the given Draft texts (or all that is found
    in the active document) to the new object
    This function was already present at splitting time during v 0.19
    """
    if not isinstance(textslist,list):
        textslist = [textslist]
    if not textslist:
        for o in FreeCAD.ActiveDocument.Objects:
            if o.TypeId == "App::Annotation":
                textslist.append(o)
    todelete = []
    for o in textslist:
        l = o.Label
        o.Label = l+".old"
        obj = makeText(o.LabelText,point=o.Position)
        obj.Label = l
        todelete.append(o.Name)
        for p in o.InList:
            if p.isDerivedFrom("App::DocumentObjectGroup"):
                if o in p.Group:
                    g = p.Group
                    g.append(obj)
                    p.Group = g
    for n in todelete:
        FreeCAD.ActiveDocument.removeObject(n)



def makeArray(baseobject,arg1,arg2,arg3,arg4=None,arg5=None,arg6=None,name="Array",use_link=False):
    """makeArray(object,xvector,yvector,xnum,ynum,[name]) for rectangular array, or
    makeArray(object,xvector,yvector,zvector,xnum,ynum,znum,[name]) for rectangular array, or
    makeArray(object,center,totalangle,totalnum,[name]) for polar array, or
    makeArray(object,rdistance,tdistance,axis,center,ncircles,symmetry,[name]) for circular array:
    Creates an array of the given object
    with, in case of rectangular array, xnum of iterations in the x direction
    at xvector distance between iterations, same for y direction with yvector and ynum,
    same for z direction with zvector and znum. In case of polar array, center is a vector, 
    totalangle is the angle to cover (in degrees) and totalnum is the number of objects, 
    including the original. In case of a circular array, rdistance is the distance of the 
    circles, tdistance is the distance within circles, axis the rotation-axes, center the
    center of rotation, ncircles the number of circles and symmetry the number
    of symmetry-axis of the distribution. The result is a parametric Draft Array.
    """

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    if use_link:
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name,_Array(None),None,True)
    else:
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
        _Array(obj)
    obj.Base = baseobject
    if arg6:
        if isinstance(arg1, (int, float, FreeCAD.Units.Quantity)):
            obj.ArrayType = "circular"
            obj.RadialDistance = arg1
            obj.TangentialDistance = arg2
            obj.Axis = arg3
            obj.Center = arg4
            obj.NumberCircles = arg5
            obj.Symmetry = arg6
        else:
            obj.ArrayType = "ortho"
            obj.IntervalX = arg1
            obj.IntervalY = arg2
            obj.IntervalZ = arg3
            obj.NumberX = arg4
            obj.NumberY = arg5
            obj.NumberZ = arg6
    elif arg4:
        obj.ArrayType = "ortho"
        obj.IntervalX = arg1
        obj.IntervalY = arg2
        obj.NumberX = arg3
        obj.NumberY = arg4
    else:
        obj.ArrayType = "polar"
        obj.Center = arg1
        obj.Angle = arg2
        obj.NumberPolar = arg3
    if gui:
        if use_link:
            _ViewProviderDraftLink(obj.ViewObject)
        else:
            _ViewProviderDraftArray(obj.ViewObject)
            formatObject(obj,obj.Base)
            if len(obj.Base.ViewObject.DiffuseColor) > 1:
                obj.ViewObject.Proxy.resetColors(obj.ViewObject)
        baseobject.ViewObject.hide()
        select(obj)
    return obj

def makePathArray(baseobject,pathobject,count,xlate=None,align=False,pathobjsubs=[],use_link=False):
    """makePathArray(docobj,path,count,xlate,align,pathobjsubs,use_link): distribute
    count copies of a document baseobject along a pathobject or subobjects of a
    pathobject. Optionally translates each copy by FreeCAD.Vector xlate direction
    and distance to adjust for difference in shape centre vs shape reference point.
    Optionally aligns baseobject to tangent/normal/binormal of path."""
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    if use_link:
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","PathArray",_PathArray(None),None,True)
    else:
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","PathArray")
        _PathArray(obj)
    obj.Base = baseobject
    obj.PathObj = pathobject
    if pathobjsubs:
        sl = []
        for sub in pathobjsubs:
            sl.append((obj.PathObj,sub))
        obj.PathSubs = list(sl)
    if count > 1:
        obj.Count = count
    if xlate:
        obj.Xlate = xlate
    obj.Align = align
    if gui:
        if use_link:
            _ViewProviderDraftLink(obj.ViewObject)
        else:
            _ViewProviderDraftArray(obj.ViewObject)
            formatObject(obj,obj.Base)
            if len(obj.Base.ViewObject.DiffuseColor) > 1:
                obj.ViewObject.Proxy.resetColors(obj.ViewObject)
        baseobject.ViewObject.hide()
        select(obj)
    return obj

def makePointArray(base, ptlst):
    """makePointArray(base,pointlist):"""
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","PointArray")
    _PointArray(obj, base, ptlst)
    obj.Base = base
    obj.PointList = ptlst
    if gui:
        _ViewProviderDraftArray(obj.ViewObject)
        base.ViewObject.hide()
        formatObject(obj,obj.Base)
        if len(obj.Base.ViewObject.DiffuseColor) > 1:
            obj.ViewObject.Proxy.resetColors(obj.ViewObject)
        select(obj)
    return obj

def extrude(obj,vector,solid=False):
    """makeExtrusion(object,vector): extrudes the given object
    in the direction given by the vector. The original object
    gets hidden."""
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    newobj = FreeCAD.ActiveDocument.addObject("Part::Extrusion","Extrusion")
    newobj.Base = obj
    newobj.Dir = vector
    newobj.Solid = solid
    if gui:
        obj.ViewObject.Visibility = False
        formatObject(newobj,obj)
        select(newobj)

    return newobj

def joinWires(wires, joinAttempts = 0):
    """joinWires(objects): merges a set of wires where possible, if any of those
    wires have a coincident start and end point"""
    if joinAttempts > len(wires):
        return
    joinAttempts += 1
    for wire1Index, wire1 in enumerate(wires):
        for wire2Index, wire2 in enumerate(wires):
            if wire2Index <= wire1Index:
                continue
            if joinTwoWires(wire1, wire2):
                wires.pop(wire2Index)
                break
    joinWires(wires, joinAttempts)

def joinTwoWires(wire1, wire2):
    """joinTwoWires(object, object): joins two wires if they share a common
    point as a start or an end.

    BUG: it occasionally fails to join lines even if the lines
    visually share a point.
    This is a rounding error in the comparison of the shared point;
    a small difference will result in the points being considered different
    and thus the lines not joining.
    Test properly using `DraftVecUtils.equals` because then it will consider
    the precision set in the Draft preferences.
    """
    wire1AbsPoints = [wire1.Placement.multVec(point) for point in wire1.Points]
    wire2AbsPoints = [wire2.Placement.multVec(point) for point in wire2.Points]
    if (wire1AbsPoints[0] == wire2AbsPoints[-1] and wire1AbsPoints[-1] == wire2AbsPoints[0]) \
        or (wire1AbsPoints[0] == wire2AbsPoints[0] and wire1AbsPoints[-1] == wire2AbsPoints[-1]):
        wire2AbsPoints.pop()
        wire1.Closed = True
    elif wire1AbsPoints[0] == wire2AbsPoints[0]:
        wire1AbsPoints = list(reversed(wire1AbsPoints))
    elif wire1AbsPoints[0] == wire2AbsPoints[-1]:
        wire1AbsPoints = list(reversed(wire1AbsPoints))
        wire2AbsPoints = list(reversed(wire2AbsPoints))
    elif wire1AbsPoints[-1] == wire2AbsPoints[-1]:
        wire2AbsPoints = list(reversed(wire2AbsPoints))
    elif wire1AbsPoints[-1] == wire2AbsPoints[0]:
        pass
    else:
        return False
    wire2AbsPoints.pop(0)
    wire1.Points = [wire1.Placement.inverse().multVec(point) for point in wire1AbsPoints] + [wire1.Placement.inverse().multVec(point) for point in wire2AbsPoints]
    FreeCAD.ActiveDocument.removeObject(wire2.Name)
    return True

def split(wire, newPoint, edgeIndex):
    if getType(wire) != "Wire":
        return
    elif wire.Closed:
        splitClosedWire(wire, edgeIndex)
    else:
        splitOpenWire(wire, newPoint, edgeIndex)

def splitClosedWire(wire, edgeIndex):
    wire.Closed = False
    if edgeIndex == len(wire.Points):
        makeWire([wire.Placement.multVec(wire.Points[0]),
            wire.Placement.multVec(wire.Points[-1])], placement=wire.Placement)
    else:
        makeWire([wire.Placement.multVec(wire.Points[edgeIndex-1]),
            wire.Placement.multVec(wire.Points[edgeIndex])], placement=wire.Placement)
        wire.Points = list(reversed(wire.Points[0:edgeIndex])) + list(reversed(wire.Points[edgeIndex:]))

def splitOpenWire(wire, newPoint, edgeIndex):
    wire1Points = []
    wire2Points = []
    for index, point in enumerate(wire.Points):
        if index == edgeIndex:
            wire1Points.append(wire.Placement.inverse().multVec(newPoint))
            wire2Points.append(newPoint)
            wire2Points.append(wire.Placement.multVec(point))
        elif index < edgeIndex:
            wire1Points.append(point)
        elif index > edgeIndex:
            wire2Points.append(wire.Placement.multVec(point))
    wire.Points = wire1Points
    makeWire(wire2Points, placement=wire.Placement)

def fuse(object1,object2):
    """fuse(oject1,object2): returns an object made from
    the union of the 2 given objects. If the objects are
    coplanar, a special Draft Wire is used, otherwise we use
    a standard Part fuse."""
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    import DraftGeomUtils, Part
    # testing if we have holes:
    holes = False
    fshape = object1.Shape.fuse(object2.Shape)
    fshape = fshape.removeSplitter()
    for f in fshape.Faces:
        if len(f.Wires) > 1:
            holes = True
    if DraftGeomUtils.isCoplanar(object1.Shape.fuse(object2.Shape).Faces) and not holes:
        obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Fusion")
        _Wire(obj)
        if gui:
            _ViewProviderWire(obj.ViewObject)
        obj.Base = object1
        obj.Tool = object2
    elif holes:
        # temporary hack, since Part::Fuse objects don't remove splitters
        obj = FreeCAD.ActiveDocument.addObject("Part::Feature","Fusion")
        obj.Shape = fshape
    else:
        obj = FreeCAD.ActiveDocument.addObject("Part::Fuse","Fusion")
        obj.Base = object1
        obj.Tool = object2
    if gui:
        object1.ViewObject.Visibility = False
        object2.ViewObject.Visibility = False
        formatObject(obj,object1)
        select(obj)

    return obj

def cut(object1,object2):
    """cut(oject1,object2): returns a cut object made from
    the difference of the 2 given objects."""
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::Cut","Cut")
    obj.Base = object1
    obj.Tool = object2
    object1.ViewObject.Visibility = False
    object2.ViewObject.Visibility = False
    formatObject(obj,object1)
    select(obj)

    return obj

def moveVertex(object, vertex_index, vector):
    points = object.Points
    points[vertex_index] = points[vertex_index].add(vector)
    object.Points = points

def moveEdge(object, edge_index, vector):
    moveVertex(object, edge_index, vector)
    if isClosedEdge(edge_index, object):
        moveVertex(object, 0, vector)
    else:
        moveVertex(object, edge_index+1, vector)

def copyMovedEdges(arguments):
    copied_edges = []
    for argument in arguments:
        copied_edges.append(copyMovedEdge(argument[0], argument[1], argument[2]))
    joinWires(copied_edges)

def copyMovedEdge(object, edge_index, vector):
    vertex1 = object.Placement.multVec(object.Points[edge_index]).add(vector)
    if isClosedEdge(edge_index, object):
        vertex2 = object.Placement.multVec(object.Points[0]).add(vector)
    else:
        vertex2 = object.Placement.multVec(object.Points[edge_index+1]).add(vector)
    return makeLine(vertex1, vertex2)

def copyRotatedEdges(arguments):
    copied_edges = []
    for argument in arguments:
        copied_edges.append(copyRotatedEdge(argument[0], argument[1],
            argument[2], argument[3], argument[4]))
    joinWires(copied_edges)

def copyRotatedEdge(object, edge_index, angle, center, axis):
    vertex1 = rotateVectorFromCenter(
        object.Placement.multVec(object.Points[edge_index]),
        angle, axis, center)
    if isClosedEdge(edge_index, object):
        vertex2 = rotateVectorFromCenter(
            object.Placement.multVec(object.Points[0]),
            angle, axis, center)
    else:
        vertex2 = rotateVectorFromCenter(
            object.Placement.multVec(object.Points[edge_index+1]),
            angle, axis, center)
    return makeLine(vertex1, vertex2)

def isClosedEdge(edge_index, object):
    return edge_index + 1 >= len(object.Points)

def move(objectslist,vector,copy=False):
    """move(objects,vector,[copy]): Moves the objects contained
    in objects (that can be an object or a list of objects)
    in the direction and distance indicated by the given
    vector. If copy is True, the actual objects are not moved, but copies
    are created instead. The objects (or their copies) are returned."""
    typecheck([(vector,Vector), (copy,bool)], "move")
    if not isinstance(objectslist,list): objectslist = [objectslist]
    objectslist.extend(getMovableChildren(objectslist))
    newobjlist = []
    newgroups = {}
    objectslist = filterObjectsForModifiers(objectslist, copy)
    for obj in objectslist:
        newobj = None
        # real_vector have been introduced to take into account
        # the possibility that object is inside an App::Part
        if hasattr(obj, "getGlobalPlacement"):
            v_minus_global = obj.getGlobalPlacement().inverse().Rotation.multVec(vector)
            real_vector = obj.Placement.Rotation.multVec(v_minus_global)
        else:
            real_vector = vector
        if getType(obj) == "Point":
            v = Vector(obj.X,obj.Y,obj.Z)
            v = v.add(real_vector)
            if copy:
                newobj = makeCopy(obj)
            else:
                newobj = obj
            newobj.X = v.x
            newobj.Y = v.y
            newobj.Z = v.z
        elif obj.isDerivedFrom("App::DocumentObjectGroup"):
            pass
        elif hasattr(obj,'Shape'):
            if copy:
                newobj = makeCopy(obj)
            else:
                newobj = obj
            pla = newobj.Placement
            pla.move(real_vector)
        elif getType(obj) == "Annotation":
            if copy:
                newobj = FreeCAD.ActiveDocument.addObject("App::Annotation",getRealName(obj.Name))
                newobj.LabelText = obj.LabelText
                if gui:
                    formatObject(newobj,obj)
            else:
                newobj = obj
            newobj.Position = obj.Position.add(real_vector)
        elif getType(obj) == "DraftText":
            if copy:
                newobj = FreeCAD.ActiveDocument.addObject("App::FeaturePython",getRealName(obj.Name))
                DraftText(newobj)
                if gui:
                    ViewProviderDraftText(newobj.ViewObject)
                    formatObject(newobj,obj)
                newobj.Text = obj.Text
                newobj.Placement = obj.Placement
                if gui:
                    formatObject(newobj,obj)
            else:
                newobj = obj
            newobj.Placement.Base = obj.Placement.Base.add(real_vector)
        elif getType(obj) in ["Dimension","LinearDimension"]:
            if copy:
                newobj = FreeCAD.ActiveDocument.addObject("App::FeaturePython",getRealName(obj.Name))
                _Dimension(newobj)
                if gui:
                    _ViewProviderDimension(newobj.ViewObject)
                    formatObject(newobj,obj)
            else:
                newobj = obj
            newobj.Start = obj.Start.add(real_vector)
            newobj.End = obj.End.add(real_vector)
            newobj.Dimline = obj.Dimline.add(real_vector)
        else:
            if copy and obj.isDerivedFrom("Mesh::Feature"):
                print("Mesh copy not supported at the moment") # TODO
            newobj = obj
            if "Placement" in obj.PropertiesList:
                pla = obj.Placement
                pla.move(real_vector)
        if newobj is not None:
            newobjlist.append(newobj)
        if copy:
            for p in obj.InList:
                if p.isDerivedFrom("App::DocumentObjectGroup") and (p in objectslist):
                    g = newgroups.setdefault(p.Name,FreeCAD.ActiveDocument.addObject(p.TypeId,p.Name))
                    g.addObject(newobj)
                    break
                if getType(p) == "Layer":
                    p.Proxy.addObject(p,newobj)
    if copy and getParam("selectBaseObjects",False):
        select(objectslist)
    else:
        select(newobjlist)
    if len(newobjlist) == 1: return newobjlist[0]
    return newobjlist

def array(objectslist,arg1,arg2,arg3,arg4=None,arg5=None,arg6=None):
    """array(objectslist,xvector,yvector,xnum,ynum) for rectangular array,
    array(objectslist,xvector,yvector,zvector,xnum,ynum,znum) for rectangular array,
    or array(objectslist,center,totalangle,totalnum) for polar array: Creates an array
    of the objects contained in list (that can be an object or a list of objects)
    with, in case of rectangular array, xnum of iterations in the x direction
    at xvector distance between iterations, and same for y and z directions with yvector
    and ynum and zvector and znum. In case of polar array, center is a vector, totalangle
    is the angle to cover (in degrees) and totalnum is the number of objects, including
    the original.

    This function creates an array of independent objects. Use makeArray() to create a
    parametric array object."""

    def rectArray(objectslist,xvector,yvector,xnum,ynum):
        typecheck([(xvector,Vector), (yvector,Vector), (xnum,int), (ynum,int)], "rectArray")
        if not isinstance(objectslist,list): objectslist = [objectslist]
        for xcount in range(xnum):
            currentxvector=Vector(xvector).multiply(xcount)
            if not xcount==0:
                move(objectslist,currentxvector,True)
            for ycount in range(ynum):
                currentxvector=FreeCAD.Base.Vector(currentxvector)
                currentyvector=currentxvector.add(Vector(yvector).multiply(ycount))
                if not ycount==0:
                    move(objectslist,currentyvector,True)
    def rectArray2(objectslist,xvector,yvector,zvector,xnum,ynum,znum):
        typecheck([(xvector,Vector), (yvector,Vector), (zvector,Vector),(xnum,int), (ynum,int),(znum,int)], "rectArray2")
        if not isinstance(objectslist,list): objectslist = [objectslist]
        for xcount in range(xnum):
            currentxvector=Vector(xvector).multiply(xcount)
            if not xcount==0:
                move(objectslist,currentxvector,True)
            for ycount in range(ynum):
                currentxvector=FreeCAD.Base.Vector(currentxvector)
                currentyvector=currentxvector.add(Vector(yvector).multiply(ycount))
                if not ycount==0:
                    move(objectslist,currentyvector,True)
                for zcount in range(znum):
                    currentzvector=currentyvector.add(Vector(zvector).multiply(zcount))
                    if not zcount==0:
                        move(objectslist,currentzvector,True)
    def polarArray(objectslist,center,angle,num):
        typecheck([(center,Vector), (num,int)], "polarArray")
        if not isinstance(objectslist,list): objectslist = [objectslist]
        fraction = float(angle)/num
        for i in range(num):
            currangle = fraction + (i*fraction)
            rotate(objectslist,currangle,center,copy=True)
    if arg6:
        rectArray2(objectslist,arg1,arg2,arg3,arg4,arg5,arg6)
    elif arg4:
        rectArray(objectslist,arg1,arg2,arg3,arg4)
    else:
        polarArray(objectslist,arg1,arg2,arg3)

def rotateVertex(object, vertex_index, angle, center, axis):
    points = object.Points
    points[vertex_index] = object.Placement.inverse().multVec(
        rotateVectorFromCenter(
            object.Placement.multVec(points[vertex_index]),
            angle, axis, center))
    object.Points = points

def rotateVectorFromCenter(vector, angle, axis, center):
    rv = vector.sub(center)
    rv = DraftVecUtils.rotate(rv, math.radians(angle), axis)
    return center.add(rv)

def rotateEdge(object, edge_index, angle, center, axis):
    rotateVertex(object, edge_index, angle, center, axis)
    if isClosedEdge(edge_index, object):
        rotateVertex(object, 0, angle, center, axis)
    else:
        rotateVertex(object, edge_index+1, angle, center, axis)

def rotate(objectslist,angle,center=Vector(0,0,0),axis=Vector(0,0,1),copy=False):
    """rotate(objects,angle,[center,axis,copy]): Rotates the objects contained
    in objects (that can be a list of objects or an object) of the given angle
    (in degrees) around the center, using axis as a rotation axis. If axis is
    omitted, the rotation will be around the vertical Z axis.
    If copy is True, the actual objects are not moved, but copies
    are created instead. The objects (or their copies) are returned."""
    import Part
    typecheck([(copy,bool)], "rotate")
    if not isinstance(objectslist,list): objectslist = [objectslist]
    objectslist.extend(getMovableChildren(objectslist))
    newobjlist = []
    newgroups = {}
    objectslist = filterObjectsForModifiers(objectslist, copy)
    for obj in objectslist:
        newobj = None
        # real_center and real_axis are introduced to take into account
        # the possibility that object is inside an App::Part
        if hasattr(obj, "getGlobalPlacement"):
            ci = obj.getGlobalPlacement().inverse().multVec(center)
            real_center = obj.Placement.multVec(ci)
            ai = obj.getGlobalPlacement().inverse().Rotation.multVec(axis)
            real_axis = obj.Placement.Rotation.multVec(ai)
        else:
            real_center = center
            real_axis = axis

        if copy:
            newobj = makeCopy(obj)
        else:
            newobj = obj
        if obj.isDerivedFrom("App::Annotation"):
            if axis.normalize() == Vector(1,0,0):
                newobj.ViewObject.RotationAxis = "X"
                newobj.ViewObject.Rotation = angle
            elif axis.normalize() == Vector(0,1,0):
                newobj.ViewObject.RotationAxis = "Y"
                newobj.ViewObject.Rotation = angle
            elif axis.normalize() == Vector(0,-1,0):
                newobj.ViewObject.RotationAxis = "Y"
                newobj.ViewObject.Rotation = -angle
            elif axis.normalize() == Vector(0,0,1):
                newobj.ViewObject.RotationAxis = "Z"
                newobj.ViewObject.Rotation = angle
            elif axis.normalize() == Vector(0,0,-1):
                newobj.ViewObject.RotationAxis = "Z"
                newobj.ViewObject.Rotation = -angle
        elif getType(obj) == "Point":
            v = Vector(obj.X,obj.Y,obj.Z)
            rv = v.sub(real_center)
            rv = DraftVecUtils.rotate(rv,math.radians(angle),real_axis)
            v = real_center.add(rv)
            newobj.X = v.x
            newobj.Y = v.y
            newobj.Z = v.z
        elif obj.isDerivedFrom("App::DocumentObjectGroup"):
            pass
        elif hasattr(obj,"Placement"):
            #FreeCAD.Console.PrintMessage("placement rotation\n")
            shape = Part.Shape()
            shape.Placement = obj.Placement
            shape.rotate(DraftVecUtils.tup(real_center), DraftVecUtils.tup(real_axis), angle)
            newobj.Placement = shape.Placement
        elif hasattr(obj,'Shape') and (getType(obj) not in ["WorkingPlaneProxy","BuildingPart"]):
            #think it make more sense to try first to rotate placement and later to try with shape. no?
            shape = obj.Shape.copy()
            shape.rotate(DraftVecUtils.tup(real_center), DraftVecUtils.tup(real_axis), angle)
            newobj.Shape = shape
        if copy:
            formatObject(newobj,obj)
        if newobj is not None:
            newobjlist.append(newobj)
        if copy:
            for p in obj.InList:
                if p.isDerivedFrom("App::DocumentObjectGroup") and (p in objectslist):
                    g = newgroups.setdefault(p.Name,FreeCAD.ActiveDocument.addObject(p.TypeId,p.Name))
                    g.addObject(newobj)
                    break
    if copy and getParam("selectBaseObjects",False):
        select(objectslist)
    else:
        select(newobjlist)
    if len(newobjlist) == 1: return newobjlist[0]
    return newobjlist

def scaleVectorFromCenter(vector, scale, center):
    return vector.sub(center).scale(scale.x, scale.y, scale.z).add(center)

def scaleVertex(object, vertex_index, scale, center):
    points = object.Points
    points[vertex_index] = object.Placement.inverse().multVec(
        scaleVectorFromCenter(
            object.Placement.multVec(points[vertex_index]),
            scale, center))
    object.Points = points

def scaleEdge(object, edge_index, scale, center):
    scaleVertex(object, edge_index, scale, center)
    if isClosedEdge(edge_index, object):
        scaleVertex(object, 0, scale, center)
    else:
        scaleVertex(object, edge_index+1, scale, center)

def copyScaledEdges(arguments):
    copied_edges = []
    for argument in arguments:
        copied_edges.append(copyScaledEdge(argument[0], argument[1],
            argument[2], argument[3]))
    joinWires(copied_edges)

def copyScaledEdge(object, edge_index, scale, center):
    vertex1 = scaleVectorFromCenter(
        object.Placement.multVec(object.Points[edge_index]),
        scale, center)
    if isClosedEdge(edge_index, object):
        vertex2 = scaleVectorFromCenter(
            object.Placement.multVec(object.Points[0]),
            scale, center)
    else:
        vertex2 = scaleVectorFromCenter(
            object.Placement.multVec(object.Points[edge_index+1]),
            scale, center)
    return makeLine(vertex1, vertex2)

def scale(objectslist,scale=Vector(1,1,1),center=Vector(0,0,0),copy=False):
    """scale(objects,vector,[center,copy,legacy]): Scales the objects contained
    in objects (that can be a list of objects or an object) of the given scale
    factors defined by the given vector (in X, Y and Z directions) around
    given center. If copy is True, the actual objects are not moved, but copies
    are created instead. The objects (or their copies) are returned."""
    if not isinstance(objectslist, list):
        objectslist = [objectslist]
    newobjlist = []
    for obj in objectslist:
        if copy:
            newobj = makeCopy(obj)
        else:
            newobj = obj
        if hasattr(obj,'Shape'):
            scaled_shape = obj.Shape.copy()
            m = FreeCAD.Matrix()
            m.move(obj.Placement.Base.negative())
            m.move(center.negative())
            m.scale(scale.x,scale.y,scale.z)
            m.move(center)
            m.move(obj.Placement.Base)
            scaled_shape = scaled_shape.transformGeometry(m)
        if getType(obj) == "Rectangle":
            p = []
            for v in scaled_shape.Vertexes:
                p.append(v.Point)
            pl = obj.Placement.copy()
            pl.Base = p[0]
            diag = p[2].sub(p[0])
            bb = p[1].sub(p[0])
            bh = p[3].sub(p[0])
            nb = DraftVecUtils.project(diag,bb)
            nh = DraftVecUtils.project(diag,bh)
            if obj.Length < 0: l = -nb.Length
            else: l = nb.Length
            if obj.Height < 0: h = -nh.Length
            else: h = nh.Length
            newobj.Length = l
            newobj.Height = h
            tr = p[0].sub(obj.Shape.Vertexes[0].Point)
            newobj.Placement = pl
        elif getType(obj) == "Wire" or getType(obj) == "BSpline":
            for index, point in enumerate(newobj.Points):
                scaleVertex(newobj, index, scale, center)
        elif hasattr(obj,'Shape'):
            newobj.Shape = scaled_shape
        elif (obj.TypeId == "App::Annotation"):
            factor = scale.y * obj.ViewObject.FontSize
            newobj.ViewObject.FontSize = factor
            d = obj.Position.sub(center)
            newobj.Position = center.add(Vector(d.x*scale.x,d.y*scale.y,d.z*scale.z))
        if copy:
            formatObject(newobj,obj)
        newobjlist.append(newobj)
    if copy and getParam("selectBaseObjects",False):
        select(objectslist)
    else:
        select(newobjlist)
    if len(newobjlist) == 1: return newobjlist[0]
    return newobjlist

def offset(obj,delta,copy=False,bind=False,sym=False,occ=False):
    """offset(object,delta,[copymode],[bind]): offsets the given wire by
    applying the given delta Vector to its first vertex. If copymode is
    True, another object is created, otherwise the same object gets
    offset. If bind is True, and provided the wire is open, the original
    and the offset wires will be bound by their endpoints, forming a face
    if sym is True, bind must be true too, and the offset is made on both
    sides, the total width being the given delta length. If offsetting a
    BSpline, the delta must not be a Vector but a list of Vectors, one for
    each node of the spline."""
    import Part, DraftGeomUtils
    newwire = None
    delete = None

    if getType(obj) in ["Sketch","Part"]:
        copy = True
        print("the offset tool is currently unable to offset a non-Draft object directly - Creating a copy")

    def getRect(p,obj):
        """returns length,height,placement"""
        pl = obj.Placement.copy()
        pl.Base = p[0]
        diag = p[2].sub(p[0])
        bb = p[1].sub(p[0])
        bh = p[3].sub(p[0])
        nb = DraftVecUtils.project(diag,bb)
        nh = DraftVecUtils.project(diag,bh)
        if obj.Length.Value < 0: l = -nb.Length
        else: l = nb.Length
        if obj.Height.Value < 0: h = -nh.Length
        else: h = nh.Length
        return l,h,pl

    def getRadius(obj,delta):
        """returns a new radius for a regular polygon"""
        an = math.pi/obj.FacesNumber
        nr = DraftVecUtils.rotate(delta,-an)
        nr.multiply(1/math.cos(an))
        nr = obj.Shape.Vertexes[0].Point.add(nr)
        nr = nr.sub(obj.Placement.Base)
        nr = nr.Length
        if obj.DrawMode == "inscribed":
            return nr
        else:
            return nr * math.cos(math.pi/obj.FacesNumber)

    newwire = None
    if getType(obj) == "Circle":
        pass
    elif getType(obj) == "BSpline":
        pass
    else:
        if sym:
            d1 = Vector(delta).multiply(0.5)
            d2 = d1.negative()
            n1 = DraftGeomUtils.offsetWire(obj.Shape,d1)
            n2 = DraftGeomUtils.offsetWire(obj.Shape,d2)
        else:
            if isinstance(delta,float) and (len(obj.Shape.Edges) == 1):
                # circle
                c = obj.Shape.Edges[0].Curve
                nc = Part.Circle(c.Center,c.Axis,delta)
                if len(obj.Shape.Vertexes) > 1:
                    nc = Part.ArcOfCircle(nc,obj.Shape.Edges[0].FirstParameter,obj.Shape.Edges[0].LastParameter)
                newwire = Part.Wire(nc.toShape())
                p = []
            else:
                newwire = DraftGeomUtils.offsetWire(obj.Shape,delta)
                if DraftGeomUtils.hasCurves(newwire) and copy:
                    p = []
                else:
                    p = DraftGeomUtils.getVerts(newwire)
    if occ:
        newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
        newobj.Shape = DraftGeomUtils.offsetWire(obj.Shape,delta,occ=True)
        formatObject(newobj,obj)
        if not copy:
            delete = obj.Name
    elif bind:
        if not DraftGeomUtils.isReallyClosed(obj.Shape):
            if sym:
                s1 = n1
                s2 = n2
            else:
                s1 = obj.Shape
                s2 = newwire
            if s1 and s2:
                w1 = s1.Edges
                w2 = s2.Edges
                w3 = Part.LineSegment(s1.Vertexes[0].Point,s2.Vertexes[0].Point).toShape()
                w4 = Part.LineSegment(s1.Vertexes[-1].Point,s2.Vertexes[-1].Point).toShape()
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
                newobj.Shape = Part.Face(Part.Wire(w1+[w3]+w2+[w4]))
            else:
                print("Draft.offset: Unable to bind wires")
        else:
            newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
            newobj.Shape = Part.Face(obj.Shape.Wires[0])
        if not copy:
            delete = obj.Name
    elif copy:
        newobj = None
        if sym: return None
        if getType(obj) == "Wire":
            if p:
                newobj = makeWire(p)
                newobj.Closed = obj.Closed
            elif newwire:
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
                newobj.Shape = newwire
            else:
                print("Draft.offset: Unable to duplicate this object")
        elif getType(obj) == "Rectangle":
            if p:
                length,height,plac = getRect(p,obj)
                newobj = makeRectangle(length,height,plac)
            elif newwire:
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
                newobj.Shape = newwire
            else:
                print("Draft.offset: Unable to duplicate this object")
        elif getType(obj) == "Circle":
            pl = obj.Placement
            newobj = makeCircle(delta)
            newobj.FirstAngle = obj.FirstAngle
            newobj.LastAngle = obj.LastAngle
            newobj.Placement = pl
        elif getType(obj) == "Polygon":
            pl = obj.Placement
            newobj = makePolygon(obj.FacesNumber)
            newobj.Radius = getRadius(obj,delta)
            newobj.DrawMode = obj.DrawMode
            newobj.Placement = pl
        elif getType(obj) == "BSpline":
            newobj = makeBSpline(delta)
            newobj.Closed = obj.Closed
        else:
            # try to offset anyway
            try:
                if p:
                    newobj = makeWire(p)
                    newobj.Closed = obj.Shape.isClosed()
            except Part.OCCError:
                pass
            if not(newobj) and newwire:
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
                newobj.Shape = newwire
            else:
                print("Draft.offset: Unable to create an offset")
        if newobj:
            formatObject(newobj,obj)
    else:
        newobj = None
        if sym: return None
        if getType(obj) == "Wire":
            if obj.Base or obj.Tool:
                FreeCAD.Console.PrintWarning("Warning: object history removed\n")
                obj.Base = None
                obj.Tool = None
            obj.Points = p
        elif getType(obj) == "BSpline":
            #print(delta)
            obj.Points = delta
            #print("done")
        elif getType(obj) == "Rectangle":
            length,height,plac = getRect(p,obj)
            obj.Placement = plac
            obj.Length = length
            obj.Height = height
        elif getType(obj) == "Circle":
            obj.Radius = delta
        elif getType(obj) == "Polygon":
            obj.Radius = getRadius(obj,delta)
        elif getType(obj) == 'Part':
            print("unsupported object") # TODO
        newobj = obj
    if copy and getParam("selectBaseObjects",False):
        select(newobj)
    else:
        select(obj)
    if delete:
        FreeCAD.ActiveDocument.removeObject(delete)
    return newobj

def draftify(objectslist,makeblock=False,delete=True):
    """draftify(objectslist,[makeblock],[delete]): turns each object of the given list
    (objectslist can also be a single object) into a Draft parametric
    wire. If makeblock is True, multiple objects will be grouped in a block.
    If delete = False, old objects are not deleted"""
    import DraftGeomUtils, Part

    if not isinstance(objectslist,list):
        objectslist = [objectslist]
    newobjlist = []
    for obj in objectslist:
        if hasattr(obj,'Shape'):
            for cluster in Part.getSortedClusters(obj.Shape.Edges):
                w = Part.Wire(cluster)
                if DraftGeomUtils.hasCurves(w):
                    if (len(w.Edges) == 1) and (DraftGeomUtils.geomType(w.Edges[0]) == "Circle"):
                        nobj = makeCircle(w.Edges[0])
                    else:
                        nobj = FreeCAD.ActiveDocument.addObject("Part::Feature",obj.Name)
                        nobj.Shape = w
                else:
                    nobj = makeWire(w)
                newobjlist.append(nobj)
                formatObject(nobj,obj)
                # sketches are always in wireframe mode. In Draft we don't like that!
                if FreeCAD.GuiUp:
                    nobj.ViewObject.DisplayMode = "Flat Lines"
            if delete:
                FreeCAD.ActiveDocument.removeObject(obj.Name)

    if makeblock:
        return makeBlock(newobjlist)
    else:
        if len(newobjlist) == 1:
            return newobjlist[0]
        return newobjlist

def getDXF(obj,direction=None):
    """getDXF(object,[direction]): returns a DXF entity from the given
    object. If direction is given, the object is projected in 2D."""
    plane = None
    result = ""
    if obj.isDerivedFrom("Drawing::View") or obj.isDerivedFrom("TechDraw::DrawView"):
        if obj.Source.isDerivedFrom("App::DocumentObjectGroup"):
            for o in obj.Source.Group:
                result += getDXF(o,obj.Direction)
        else:
            result += getDXF(obj.Source,obj.Direction)
        return result
    if direction:
        if isinstance(direction,FreeCAD.Vector):
            if direction != Vector(0,0,0):
                plane = WorkingPlane.plane()
                plane.alignToPointAndAxis(Vector(0,0,0),direction)

    def getProj(vec):
        if not plane: return vec
        nx = DraftVecUtils.project(vec,plane.u)
        ny = DraftVecUtils.project(vec,plane.v)
        return Vector(nx.Length,ny.Length,0)

    if getType(obj) in ["Dimension","LinearDimension"]:
        p1 = getProj(obj.Start)
        p2 = getProj(obj.End)
        p3 = getProj(obj.Dimline)
        result += "0\nDIMENSION\n8\n0\n62\n0\n3\nStandard\n70\n1\n"
        result += "10\n"+str(p3.x)+"\n20\n"+str(p3.y)+"\n30\n"+str(p3.z)+"\n"
        result += "13\n"+str(p1.x)+"\n23\n"+str(p1.y)+"\n33\n"+str(p1.z)+"\n"
        result += "14\n"+str(p2.x)+"\n24\n"+str(p2.y)+"\n34\n"+str(p2.z)+"\n"

    elif getType(obj) == "Annotation":
        p = getProj(obj.Position)
        count = 0
        for t in obj.LabeLtext:
            result += "0\nTEXT\n8\n0\n62\n0\n"
            result += "10\n"+str(p.x)+"\n20\n"+str(p.y+count)+"\n30\n"+str(p.z)+"\n"
            result += "40\n1\n"
            result += "1\n"+str(t)+"\n"
            result += "7\nSTANDARD\n"
            count += 1

    elif hasattr(obj,'Shape'):
        # TODO do this the Draft way, for ex. using polylines and rectangles
        import Drawing
        if not direction:
            direction = FreeCAD.Vector(0,0,-1)
        if DraftVecUtils.isNull(direction):
            direction = FreeCAD.Vector(0,0,-1)
        try:
            d = Drawing.projectToDXF(obj.Shape,direction)
        except:
            print("Draft.getDXF: Unable to project ",obj.Label," to ",direction)
        else:
            result += d

    else:
        print("Draft.getDXF: Unsupported object: ",obj.Label)

    return result


def getrgb(color,testbw=True):
    """getRGB(color,[testbw]): returns a rgb value #000000 from a freecad color
    if testwb = True (default), pure white will be converted into pure black"""
    r = str(hex(int(color[0]*255)))[2:].zfill(2)
    g = str(hex(int(color[1]*255)))[2:].zfill(2)
    b = str(hex(int(color[2]*255)))[2:].zfill(2)
    col = "#"+r+g+b
    if testbw:
        if col == "#ffffff":
            #print(getParam('SvgLinesBlack'))
            if getParam('SvgLinesBlack',True):
                col = "#000000"
    return col


import getSVG as svg


getSVG = svg.getSVG


def makeDrawingView(obj,page,lwmod=None,tmod=None,otherProjection=None):
    """
    makeDrawingView(object,page,[lwmod,tmod]) - adds a View of the given object to the
    given page. lwmod modifies lineweights (in percent), tmod modifies text heights
    (in percent). The Hint scale, X and Y of the page are used.
    """
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    if getType(obj) == "SectionPlane":
        import ArchSectionPlane
        viewobj = FreeCAD.ActiveDocument.addObject("Drawing::FeatureViewPython","View")
        page.addObject(viewobj)
        ArchSectionPlane._ArchDrawingView(viewobj)
        viewobj.Source = obj
        viewobj.Label = "View of "+obj.Name
    elif getType(obj) == "Panel":
        import ArchPanel
        viewobj = ArchPanel.makePanelView(obj,page)
    else:
        viewobj = FreeCAD.ActiveDocument.addObject("Drawing::FeatureViewPython","View"+obj.Name)
        _DrawingView(viewobj)
        page.addObject(viewobj)
        if (otherProjection):
            if hasattr(otherProjection,"Scale"):
                viewobj.Scale = otherProjection.Scale
            if hasattr(otherProjection,"X"):
                viewobj.X = otherProjection.X
            if hasattr(otherProjection,"Y"):
                viewobj.Y = otherProjection.Y
            if hasattr(otherProjection,"Rotation"):
                viewobj.Rotation = otherProjection.Rotation
            if hasattr(otherProjection,"Direction"):
                viewobj.Direction = otherProjection.Direction
        else:
            if hasattr(page.ViewObject,"HintScale"):
                viewobj.Scale = page.ViewObject.HintScale
            if hasattr(page.ViewObject,"HintOffsetX"):
                viewobj.X = page.ViewObject.HintOffsetX
            if hasattr(page.ViewObject,"HintOffsetY"):
                viewobj.Y = page.ViewObject.HintOffsetY
        viewobj.Source = obj
        if lwmod: viewobj.LineweightModifier = lwmod
        if tmod: viewobj.TextModifier = tmod
        if hasattr(obj.ViewObject,"Pattern"):
            if str(obj.ViewObject.Pattern) in list(svgpatterns().keys()):
                viewobj.FillStyle = str(obj.ViewObject.Pattern)
        if hasattr(obj.ViewObject,"DrawStyle"):
            viewobj.LineStyle = obj.ViewObject.DrawStyle
        if hasattr(obj.ViewObject,"LineColor"):
            viewobj.LineColor = obj.ViewObject.LineColor
        elif hasattr(obj.ViewObject,"TextColor"):
            viewobj.LineColor = obj.ViewObject.TextColor
    return viewobj


def mirror(objlist, p1, p2):
    """mirror(objlist, p1, p2)
    creates a Part::Mirror of the given object(s), along a plane defined
    by the 2 given points and the draft working plane normal.
    """

    if not objlist:
        _err = "No object given"
        FreeCAD.Console.PrintError(translate("draft", _err) + "\n")
        return
    if p1 == p2:
        _err = "The two points are coincident"
        FreeCAD.Console.PrintError(translate("draft", _err) + "\n")
        return
    if not isinstance(objlist,list):
        objlist = [objlist]

    if hasattr(FreeCAD, "DraftWorkingPlane"):
        norm = FreeCAD.DraftWorkingPlane.getNormal()
    elif gui:
        norm = FreeCADGui.ActiveDocument.ActiveView.getViewDirection().negative()
    else:
        norm = FreeCAD.Vector(0,0,1)
    
    pnorm = p2.sub(p1).cross(norm).normalize()

    result = []

    for obj in objlist:
        mir = FreeCAD.ActiveDocument.addObject("Part::Mirroring","mirror")
        mir.Label = "Mirror of " + obj.Label
        mir.Source = obj
        mir.Base = p1
        mir.Normal = pnorm
        formatObject(mir, obj)
        result.append(mir)

    if len(result) == 1:
        result = result[0]
        select(result)

    return result


def heal(objlist=None,delete=True,reparent=True):
    """heal([objlist],[delete],[reparent]) - recreates Draft objects that are damaged,
    for example if created from an earlier version. If delete is True,
    the damaged objects are deleted (default). If ran without arguments, all the objects
    in the document will be healed if they are damaged. If reparent is True (default),
    new objects go at the very same place in the tree than their original."""

    auto = False

    if not objlist:
        objlist = FreeCAD.ActiveDocument.Objects
        print("Automatic mode: Healing whole document...")
        auto = True
    else:
        print("Manual mode: Force-healing selected objects...")

    if not isinstance(objlist,list):
        objlist = [objlist]

    dellist = []
    got = False

    for obj in objlist:
        dtype = getType(obj)
        ftype = obj.TypeId
        if ftype in ["Part::FeaturePython","App::FeaturePython","Part::Part2DObjectPython","Drawing::FeatureViewPython"]:
            proxy = obj.Proxy
            if hasattr(obj,"ViewObject"):
                if hasattr(obj.ViewObject,"Proxy"):
                    proxy = obj.ViewObject.Proxy
            if (proxy == 1) or (dtype in ["Unknown","Part"]) or (not auto):
                got = True
                dellist.append(obj.Name)
                props = obj.PropertiesList
                if ("Dimline" in props) and ("Start" in props):
                    print("Healing " + obj.Name + " of type Dimension")
                    nobj = makeCopy(obj,force="Dimension",reparent=reparent)
                elif ("Height" in props) and ("Length" in props):
                    print("Healing " + obj.Name + " of type Rectangle")
                    nobj = makeCopy(obj,force="Rectangle",reparent=reparent)
                elif ("Points" in props) and ("Closed" in props):
                    if "BSpline" in obj.Name:
                        print("Healing " + obj.Name + " of type BSpline")
                        nobj = makeCopy(obj,force="BSpline",reparent=reparent)
                    else:
                        print("Healing " + obj.Name + " of type Wire")
                        nobj = makeCopy(obj,force="Wire",reparent=reparent)
                elif ("Radius" in props) and ("FirstAngle" in props):
                    print("Healing " + obj.Name + " of type Circle")
                    nobj = makeCopy(obj,force="Circle",reparent=reparent)
                elif ("DrawMode" in props) and ("FacesNumber" in props):
                    print("Healing " + obj.Name + " of type Polygon")
                    nobj = makeCopy(obj,force="Polygon",reparent=reparent)
                elif ("FillStyle" in props) and ("FontSize" in props):
                    nobj = makeCopy(obj,force="DrawingView",reparent=reparent)
                else:
                    dellist.pop()
                    print("Object " + obj.Name + " is not healable")

    if not got:
        print("No object seems to need healing")
    else:
        print("Healed ",len(dellist)," objects")

    if dellist and delete:
        for n in dellist:
            FreeCAD.ActiveDocument.removeObject(n)


def upgrade(objects,delete=False,force=None):
    """upgrade(objects,delete=False,force=None): Upgrades the given object(s) (can be
    an object or a list of objects). If delete is True, old objects are deleted.
    The force attribute can be used to
    force a certain way of upgrading. It can be: makeCompound, closeGroupWires,
    makeSolid, closeWire, turnToParts, makeFusion, makeShell, makeFaces, draftify,
    joinFaces, makeSketchFace, makeWires
    Returns a dictionary containing two lists, a list of new objects and a list
    of objects to be deleted"""

    import Part, DraftGeomUtils

    if not isinstance(objects,list):
        objects = [objects]

    global deleteList, newList
    deleteList = []
    addList = []

    # definitions of actions to perform

    def turnToLine(obj):
        """turns an edge into a Draft line"""
        p1 = obj.Shape.Vertexes[0].Point
        p2 = obj.Shape.Vertexes[-1].Point
        newobj = makeLine(p1,p2)
        addList.append(newobj)
        deleteList.append(obj)
        return newobj

    def makeCompound(objectslist):
        """returns a compound object made from the given objects"""
        newobj = makeBlock(objectslist)
        addList.append(newobj)
        return newobj

    def closeGroupWires(groupslist):
        """closes every open wire in the given groups"""
        result = False
        for grp in groupslist:
            for obj in grp.Group:
                    newobj = closeWire(obj)
                    # add new objects to their respective groups
                    if newobj:
                        result = True
                        grp.addObject(newobj)
        return result

    def makeSolid(obj):
        """turns an object into a solid, if possible"""
        if obj.Shape.Solids:
            return None
        sol = None
        try:
            sol = Part.makeSolid(obj.Shape)
        except Part.OCCError:
            return None
        else:
            if sol:
                if sol.isClosed():
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Solid")
                    newobj.Shape = sol
                    addList.append(newobj)
                    deleteList.append(obj)
            return newobj

    def closeWire(obj):
        """closes a wire object, if possible"""
        if obj.Shape.Faces:
            return None
        if len(obj.Shape.Wires) != 1:
            return None
        if len(obj.Shape.Edges) == 1:
            return None
        if getType(obj) == "Wire":
            obj.Closed = True
            return True
        else:
            w = obj.Shape.Wires[0]
            if not w.isClosed():
                edges = w.Edges
                p0 = w.Vertexes[0].Point
                p1 = w.Vertexes[-1].Point
                if p0 == p1:
                    # sometimes an open wire can have its start and end points identical (OCC bug)
                    # in that case, although it is not closed, face works...
                    f = Part.Face(w)
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Face")
                    newobj.Shape = f
                else:
                    edges.append(Part.LineSegment(p1,p0).toShape())
                    w = Part.Wire(Part.__sortEdges__(edges))
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Wire")
                    newobj.Shape = w
                addList.append(newobj)
                deleteList.append(obj)
                return newobj
            else:
                return None

    def turnToParts(meshes):
        """turn given meshes to parts"""
        result = False
        import Arch
        for mesh in meshes:
            sh = Arch.getShapeFromMesh(mesh.Mesh)
            if sh:
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Shell")
                newobj.Shape = sh
                addList.append(newobj)
                deleteList.append(mesh)
                result = True
        return result

    def makeFusion(obj1,obj2):
        """makes a Draft or Part fusion between 2 given objects"""
        newobj = fuse(obj1,obj2)
        if newobj:
            addList.append(newobj)
            return newobj
        return None

    def makeShell(objectslist):
        """makes a shell with the given objects"""
        params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        preserveFaceColor = params.GetBool("preserveFaceColor") # True
        preserveFaceNames = params.GetBool("preserveFaceNames") # True
        faces = []
        facecolors = [[], []] if (preserveFaceColor) else None
        for obj in objectslist:
            faces.extend(obj.Shape.Faces)
            if (preserveFaceColor):
                """ at this point, obj.Shape.Faces are not in same order as the
                original faces we might have gotten as a result of downgrade, nor do they
                have the same hashCode(); but they still keep reference to their original
                colors - capture that in facecolors.
                Also, cannot w/ .ShapeColor here, need a whole array matching the colors
                of the array of faces per object, only DiffuseColor has that """
                facecolors[0].extend(obj.ViewObject.DiffuseColor)
                facecolors[1] = faces
        sh = Part.makeShell(faces)
        if sh:
            if sh.Faces:
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Shell")
                newobj.Shape = sh
                if (preserveFaceNames):
                    import re
                    firstName = objectslist[0].Label
                    nameNoTrailNumbers = re.sub("\d+$", "", firstName)
                    newobj.Label = "{} {}".format(newobj.Label, nameNoTrailNumbers)
                if (preserveFaceColor):
                    """ At this point, sh.Faces are completely new, with different hashCodes
                    and different ordering from obj.Shape.Faces; since we cannot compare
                    via hashCode(), we have to iterate and use a different criteria to find
                    the original matching color """
                    colarray = []
                    for ind, face in enumerate(newobj.Shape.Faces):
                        for fcind, fcface in enumerate(facecolors[1]):
                            if ((face.Area == fcface.Area) and (face.CenterOfMass == fcface.CenterOfMass)):
                                colarray.append(facecolors[0][fcind])
                                break
                    newobj.ViewObject.DiffuseColor = colarray;
                addList.append(newobj)
                deleteList.extend(objectslist)
                return newobj
        return None

    def joinFaces(objectslist):
        """makes one big face from selected objects, if possible"""
        faces = []
        for obj in objectslist:
            faces.extend(obj.Shape.Faces)
        u = faces.pop(0)
        for f in faces:
            u = u.fuse(f)
        if DraftGeomUtils.isCoplanar(faces):
            u = DraftGeomUtils.concatenate(u)
            if not DraftGeomUtils.hasCurves(u):
                # several coplanar and non-curved faces: they can become a Draft wire
                newobj = makeWire(u.Wires[0],closed=True,face=True)
            else:
                # if not possible, we do a non-parametric union
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Union")
                newobj.Shape = u
            addList.append(newobj)
            deleteList.extend(objectslist)
            return newobj
        return None

    def makeSketchFace(obj):
        """Makes a Draft face out of a sketch"""
        newobj = makeWire(obj.Shape,closed=True)
        if newobj:
            newobj.Base = obj
            obj.ViewObject.Visibility = False
            addList.append(newobj)
            return newobj
        return None

    def makeFaces(objectslist):
        """make a face from every closed wire in the list"""
        result = False
        for o in objectslist:
            for w in o.Shape.Wires:
                try:
                    f = Part.Face(w)
                except Part.OCCError:
                    pass
                else:
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Face")
                    newobj.Shape = f
                    addList.append(newobj)
                    result = True
                    if not o in deleteList:
                        deleteList.append(o)
        return result

    def makeWires(objectslist):
        """joins edges in the given objects list into wires"""
        edges = []
        for o in objectslist:
            for e in o.Shape.Edges:
                edges.append(e)
        try:
            nedges = Part.__sortEdges__(edges[:])
            # for e in nedges: print("debug: ",e.Curve,e.Vertexes[0].Point,e.Vertexes[-1].Point)
            w = Part.Wire(nedges)
        except Part.OCCError:
            return None
        else:
            if len(w.Edges) == len(edges):
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Wire")
                newobj.Shape = w
                addList.append(newobj)
                deleteList.extend(objectslist)
                return True
        return None

    # analyzing what we have in our selection

    edges = []
    wires = []
    openwires = []
    faces = []
    groups = []
    parts = []
    curves = []
    facewires = []
    loneedges = []
    meshes = []
    for ob in objects:
        if ob.TypeId == "App::DocumentObjectGroup":
            groups.append(ob)
        elif hasattr(ob,'Shape'):
            parts.append(ob)
            faces.extend(ob.Shape.Faces)
            wires.extend(ob.Shape.Wires)
            edges.extend(ob.Shape.Edges)
            for f in ob.Shape.Faces:
                facewires.extend(f.Wires)
            wirededges = []
            for w in ob.Shape.Wires:
                if len(w.Edges) > 1:
                    for e in w.Edges:
                        wirededges.append(e.hashCode())
                if not w.isClosed():
                    openwires.append(w)
            for e in ob.Shape.Edges:
                if DraftGeomUtils.geomType(e) != "Line":
                    curves.append(e)
                if not e.hashCode() in wirededges:
                    loneedges.append(e)
        elif ob.isDerivedFrom("Mesh::Feature"):
            meshes.append(ob)
    objects = parts

    #print("objects:",objects," edges:",edges," wires:",wires," openwires:",openwires," faces:",faces)
    #print("groups:",groups," curves:",curves," facewires:",facewires, "loneedges:", loneedges)

    if force:
        if force in ["makeCompound","closeGroupWires","makeSolid","closeWire","turnToParts","makeFusion",
                     "makeShell","makeFaces","draftify","joinFaces","makeSketchFace","makeWires","turnToLine"]:
            result = eval(force)(objects)
        else:
            FreeCAD.Console.PrintMessage(translate("Draft","Upgrade: Unknown force method:")+" "+force)
            result = None

    else:

        # applying transformations automatically

        result = None

        # if we have a group: turn each closed wire inside into a face
        if groups:
            result = closeGroupWires(groups)
            if result:
                FreeCAD.Console.PrintMessage(translate("draft", "Found groups: closing each open object inside")+"\n")

        # if we have meshes, we try to turn them into shapes
        elif meshes:
            result = turnToParts(meshes)
            if result:
                FreeCAD.Console.PrintMessage(translate("draft", "Found mesh(es): turning into Part shapes")+"\n")

        # we have only faces here, no lone edges
        elif faces and (len(wires) + len(openwires) == len(facewires)):

            # we have one shell: we try to make a solid
            if (len(objects) == 1) and (len(faces) > 3):
                result = makeSolid(objects[0])
                if result:
                    FreeCAD.Console.PrintMessage(translate("draft", "Found 1 solidifiable object: solidifying it")+"\n")

            # we have exactly 2 objects: we fuse them
            elif (len(objects) == 2) and (not curves):
                result = makeFusion(objects[0],objects[1])
                if result:
                    FreeCAD.Console.PrintMessage(translate("draft", "Found 2 objects: fusing them")+"\n")

            # we have many separate faces: we try to make a shell
            elif (len(objects) > 2) and (len(faces) > 1) and (not loneedges):
                result = makeShell(objects)
                if result:
                    FreeCAD.Console.PrintMessage(translate("draft", "Found several objects: creating a shell")+"\n")

            # we have faces: we try to join them if they are coplanar
            elif len(faces) > 1:
                result = joinFaces(objects)
                if result:
                    FreeCAD.Console.PrintMessage(translate("draft", "Found several coplanar objects or faces: creating one face")+"\n")

            # only one object: if not parametric, we "draftify" it
            elif len(objects) == 1 and (not objects[0].isDerivedFrom("Part::Part2DObjectPython")):
                result = draftify(objects[0])
                if result:
                    FreeCAD.Console.PrintMessage(translate("draft", "Found 1 non-parametric objects: draftifying it")+"\n")

        # we have only one object that contains one edge
        elif (not faces) and (len(objects) == 1) and (len(edges) == 1):
            # we have a closed sketch: Extract a face
            if objects[0].isDerivedFrom("Sketcher::SketchObject") and (len(edges[0].Vertexes) == 1):
                result = makeSketchFace(objects[0])
                if result:
                    FreeCAD.Console.PrintMessage(translate("draft", "Found 1 closed sketch object: creating a face from it")+"\n")
            else:
                # turn to Draft line
                e = objects[0].Shape.Edges[0]
                if isinstance(e.Curve,(Part.LineSegment,Part.Line)):
                    result = turnToLine(objects[0])
                    if result:
                        FreeCAD.Console.PrintMessage(translate("draft", "Found 1 linear object: converting to line")+"\n")

        # we have only closed wires, no faces
        elif wires and (not faces) and (not openwires):

            # we have a sketch: Extract a face
            if (len(objects) == 1) and objects[0].isDerivedFrom("Sketcher::SketchObject"):
                result = makeSketchFace(objects[0])
                if result:
                    FreeCAD.Console.PrintMessage(translate("draft", "Found 1 closed sketch object: creating a face from it")+"\n")

            # only closed wires
            else:
                result = makeFaces(objects)
                if result:
                    FreeCAD.Console.PrintMessage(translate("draft", "Found closed wires: creating faces")+"\n")

        # special case, we have only one open wire. We close it, unless it has only 1 edge!"
        elif (len(openwires) == 1) and (not faces) and (not loneedges):
            result = closeWire(objects[0])
            if result:
                FreeCAD.Console.PrintMessage(translate("draft", "Found 1 open wire: closing it")+"\n")

        # only open wires and edges: we try to join their edges
        elif openwires and (not wires) and (not faces):
            result = makeWires(objects)
            if result:
                FreeCAD.Console.PrintMessage(translate("draft", "Found several open wires: joining them")+"\n")

        # only loneedges: we try to join them
        elif loneedges and (not facewires):
            result = makeWires(objects)
            if result:
                FreeCAD.Console.PrintMessage(translate("draft", "Found several edges: wiring them")+"\n")

        # all other cases, if more than 1 object, make a compound
        elif (len(objects) > 1):
            result = makeCompound(objects)
            if result:
                FreeCAD.Console.PrintMessage(translate("draft", "Found several non-treatable objects: creating compound")+"\n")

        # no result has been obtained
        if not result:
            FreeCAD.Console.PrintMessage(translate("draft", "Unable to upgrade these objects.")+"\n")

    if delete:
        names = []
        for o in deleteList:
            names.append(o.Name)
        deleteList = []
        for n in names:
            FreeCAD.ActiveDocument.removeObject(n)
    select(addList)
    return [addList,deleteList]

def downgrade(objects,delete=False,force=None):
    """downgrade(objects,delete=False,force=None): Downgrades the given object(s) (can be
    an object or a list of objects). If delete is True, old objects are deleted.
    The force attribute can be used to
    force a certain way of downgrading. It can be: explode, shapify, subtr,
    splitFaces, cut2, getWire, splitWires, splitCompounds.
    Returns a dictionary containing two lists, a list of new objects and a list
    of objects to be deleted"""

    import Part, DraftGeomUtils

    if not isinstance(objects,list):
        objects = [objects]

    global deleteList, addList
    deleteList = []
    addList = []

    # actions definitions

    def explode(obj):
        """explodes a Draft block"""
        pl = obj.Placement
        newobj = []
        for o in obj.Components:
            o.ViewObject.Visibility = True
            o.Placement = o.Placement.multiply(pl)
        if newobj:
            deleteList(obj)
            return newobj
        return None

    def cut2(objects):
        """cuts first object from the last one"""
        newobj = cut(objects[0],objects[1])
        if newobj:
            addList.append(newobj)
            return newobj
        return None

    def splitCompounds(objects):
        """split solids contained in compound objects into new objects"""
        result = False
        for o in objects:
            if o.Shape.Solids:
                for s in o.Shape.Solids:
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Solid")
                    newobj.Shape = s
                    addList.append(newobj)
                result = True
                deleteList.append(o)
        return result

    def splitFaces(objects):
        """split faces contained in objects into new objects"""
        result = False
        params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        preserveFaceColor = params.GetBool("preserveFaceColor") # True
        preserveFaceNames = params.GetBool("preserveFaceNames") # True
        for o in objects:
            voDColors = o.ViewObject.DiffuseColor if (preserveFaceColor and hasattr(o,'ViewObject')) else None
            oLabel = o.Label if hasattr(o,'Label') else ""
            if o.Shape.Faces:
                for ind, f in enumerate(o.Shape.Faces):
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Face")
                    newobj.Shape = f
                    if preserveFaceNames:
                        newobj.Label = "{} {}".format(oLabel, newobj.Label)
                    if preserveFaceColor:
                        """ At this point, some single-color objects might have
                        just a single entry in voDColors for all their faces; handle that"""
                        tcolor = voDColors[ind] if ind<len(voDColors) else voDColors[0]
                        newobj.ViewObject.DiffuseColor[0] = tcolor # does is not applied visually on its own; left just in case
                        newobj.ViewObject.ShapeColor = tcolor # this gets applied, works by itself too
                    addList.append(newobj)
                result = True
                deleteList.append(o)
        return result

    def subtr(objects):
        """subtracts objects from the first one"""
        faces = []
        for o in objects:
            if o.Shape.Faces:
                faces.extend(o.Shape.Faces)
                deleteList.append(o)
        u = faces.pop(0)
        for f in faces:
            u = u.cut(f)
        if not u.isNull():
            newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Subtraction")
            newobj.Shape = u
            addList.append(newobj)
            return newobj
        return None

    def getWire(obj):
        """gets the wire from a face object"""
        result = False
        for w in obj.Shape.Faces[0].Wires:
            newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Wire")
            newobj.Shape = w
            addList.append(newobj)
            result = True
        deleteList.append(obj)
        return result

    def splitWires(objects):
        """splits the wires contained in objects into edges"""
        result = False
        for o in objects:
            if o.Shape.Edges:
                for e in o.Shape.Edges:
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Edge")
                    newobj.Shape = e
                    addList.append(newobj)
                deleteList.append(o)
                result = True
        return result

    # analyzing objects

    faces = []
    edges = []
    onlyedges = True
    parts = []
    solids = []
    result = None

    for o in objects:
        if hasattr(o, 'Shape'):
            for s in o.Shape.Solids:
                solids.append(s)
            for f in o.Shape.Faces:
                faces.append(f)
            for e in o.Shape.Edges:
                edges.append(e)
            if o.Shape.ShapeType != "Edge":
                onlyedges = False
            parts.append(o)
    objects = parts

    if force:
        if force in ["explode","shapify","subtr","splitFaces","cut2","getWire","splitWires"]:
            result = eval(force)(objects)
        else:
            FreeCAD.Console.PrintMessage(translate("Draft","Upgrade: Unknown force method:")+" "+force)
            result = None

    else:

        # applying transformation automatically

        # we have a block, we explode it
        if (len(objects) == 1) and (getType(objects[0]) == "Block"):
            result = explode(objects[0])
            if result:
                FreeCAD.Console.PrintMessage(translate("draft", "Found 1 block: exploding it")+"\n")

        # we have one multi-solids compound object: extract its solids
        elif (len(objects) == 1) and hasattr(objects[0],'Shape') and (len(solids) > 1):
            result = splitCompounds(objects)
            #print(result)
            if result:
                FreeCAD.Console.PrintMessage(translate("draft", "Found 1 multi-solids compound: exploding it")+"\n")

        # special case, we have one parametric object: we "de-parametrize" it
        elif (len(objects) == 1) and hasattr(objects[0],'Shape') and hasattr(objects[0], 'Base'):
            result = shapify(objects[0])
            if result:
                FreeCAD.Console.PrintMessage(translate("draft", "Found 1 parametric object: breaking its dependencies")+"\n")
                addList.append(result)
                #deleteList.append(objects[0])

        # we have only 2 objects: cut 2nd from 1st
        elif len(objects) == 2:
            result = cut2(objects)
            if result:
                FreeCAD.Console.PrintMessage(translate("draft", "Found 2 objects: subtracting them")+"\n")

        elif (len(faces) > 1):

            # one object with several faces: split it
            if len(objects) == 1:
                result = splitFaces(objects)
                if result:
                    FreeCAD.Console.PrintMessage(translate("draft", "Found several faces: splitting them")+"\n")

            # several objects: remove all the faces from the first one
            else:
                result = subtr(objects)
                if result:
                    FreeCAD.Console.PrintMessage(translate("draft", "Found several objects: subtracting them from the first one")+"\n")

        # only one face: we extract its wires
        elif (len(faces) > 0):
            result = getWire(objects[0])
            if result:
                FreeCAD.Console.PrintMessage(translate("draft", "Found 1 face: extracting its wires")+"\n")

        # no faces: split wire into single edges
        elif not onlyedges:
            result = splitWires(objects)
            if result:
                FreeCAD.Console.PrintMessage(translate("draft", "Found only wires: extracting their edges")+"\n")

        # no result has been obtained
        if not result:
            FreeCAD.Console.PrintMessage(translate("draft", "No more downgrade possible")+"\n")

    if delete:
        names = []
        for o in deleteList:
            names.append(o.Name)
        deleteList = []
        for n in names:
            FreeCAD.ActiveDocument.removeObject(n)
    select(addList)
    return [addList,deleteList]


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


def calculatePlacement(globalRotation, edge, offset, RefPt, xlate, align, normal=None):
    """Orient shape to tangent at parm offset along edge."""
    import functools
    # http://en.wikipedia.org/wiki/Euler_angles
    # start with null Placement point so translate goes to right place.
    placement = FreeCAD.Placement()
    # preserve global orientation
    placement.Rotation = globalRotation

    placement.move(RefPt + xlate)

    if not align:
        return placement

    # unit +Z  Probably defined elsewhere?
    z = FreeCAD.Vector(0, 0, 1)
    # y = FreeCAD.Vector(0, 1, 0)               # unit +Y
    x = FreeCAD.Vector(1, 0, 0)                 # unit +X
    nullv = FreeCAD.Vector(0, 0, 0)

    # get local coord system - tangent, normal, binormal, if possible
    t = edge.tangentAt(getParameterFromV0(edge, offset))
    t.normalize()

    try:
        if normal:
            n = normal
        else:
            n = edge.normalAt(getParameterFromV0(edge, offset))
            n.normalize()
        b = (t.cross(n))
        b.normalize()
    # no normal defined here
    except FreeCAD.Base.FreeCADError:
        n = nullv
        b = nullv
        FreeCAD.Console.PrintLog(
            "Draft PathArray.orientShape - Cannot calculate Path normal.\n")

    lnodes = z.cross(b)

    try:
        # Can't normalize null vector.
        lnodes.normalize()
    except:
        # pathological cases:
        pass
    # 1) can't determine normal, don't align.
    if n == nullv:
        psi = 0.0
        theta = 0.0
        phi = 0.0
        FreeCAD.Console.PrintWarning(
            "Draft PathArray.orientShape - Path normal is Null. Cannot align.\n")
    elif abs(b.dot(z)) == 1.0:                                    # 2) binormal is || z
        # align shape to tangent only
        psi = math.degrees(DraftVecUtils.angle(x, t, z))
        theta = 0.0
        phi = 0.0
        FreeCAD.Console.PrintWarning(
            "Draft PathArray.orientShape - Gimbal lock. Infinite lnodes. Change Path or Base.\n")
    else:                                                        # regular case
        psi = math.degrees(DraftVecUtils.angle(x, lnodes, z))
        theta = math.degrees(DraftVecUtils.angle(z, b, lnodes))
        phi = math.degrees(DraftVecUtils.angle(lnodes, t, b))

    rotations = [placement.Rotation]

    if psi != 0.0:
        rotations.insert(0, FreeCAD.Rotation(z, psi))
    if theta != 0.0:
        rotations.insert(0, FreeCAD.Rotation(lnodes, theta))
    if phi != 0.0:
        rotations.insert(0, FreeCAD.Rotation(b, phi))

    if len(rotations) == 1:
        finalRotation = rotations[0]
    else:
        finalRotation = functools.reduce(
            lambda rot1, rot2: rot1.multiply(rot2), rotations)

    placement.Rotation = finalRotation

    return placement


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

#---------------------------------------------------------------------------
# Python Features definitions
#---------------------------------------------------------------------------

class _DrawingView(_DraftObject):
    """The Draft DrawingView object"""
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"DrawingView")
        obj.addProperty("App::PropertyVector","Direction","Shape View",QT_TRANSLATE_NOOP("App::Property","Projection direction"))
        obj.addProperty("App::PropertyFloat","LineWidth","View Style",QT_TRANSLATE_NOOP("App::Property","The width of the lines inside this object"))
        obj.addProperty("App::PropertyLength","FontSize","View Style",QT_TRANSLATE_NOOP("App::Property","The size of the texts inside this object"))
        obj.addProperty("App::PropertyLength","LineSpacing","View Style",QT_TRANSLATE_NOOP("App::Property","The spacing between lines of text"))
        obj.addProperty("App::PropertyColor","LineColor","View Style",QT_TRANSLATE_NOOP("App::Property","The color of the projected objects"))
        obj.addProperty("App::PropertyLink","Source","Base",QT_TRANSLATE_NOOP("App::Property","The linked object"))
        obj.addProperty("App::PropertyEnumeration","FillStyle","View Style",QT_TRANSLATE_NOOP("App::Property","Shape Fill Style"))
        obj.addProperty("App::PropertyEnumeration","LineStyle","View Style",QT_TRANSLATE_NOOP("App::Property","Line Style"))
        obj.addProperty("App::PropertyBool","AlwaysOn","View Style",QT_TRANSLATE_NOOP("App::Property","If checked, source objects are displayed regardless of being visible in the 3D model"))
        obj.FillStyle = ['shape color'] + list(svgpatterns().keys())
        obj.LineStyle = ['Solid','Dashed','Dotted','Dashdot']
        obj.LineWidth = 0.35
        obj.FontSize = 12

    def execute(self, obj):
        result = ""
        if hasattr(obj,"Source"):
            if obj.Source:
                if hasattr(obj,"LineStyle"):
                    ls = obj.LineStyle
                else:
                    ls = None
                if hasattr(obj,"LineColor"):
                    lc = obj.LineColor
                else:
                    lc = None
                if hasattr(obj,"LineSpacing"):
                    lp = obj.LineSpacing
                else:
                    lp = None
                if obj.Source.isDerivedFrom("App::DocumentObjectGroup"):
                    svg = ""
                    shapes = []
                    others = []
                    objs = getGroupContents([obj.Source])
                    for o in objs:
                        v = o.ViewObject.isVisible()
                        if hasattr(obj,"AlwaysOn"):
                            if obj.AlwaysOn:
                                v = True
                        if v:
                            svg += getSVG(o,obj.Scale,obj.LineWidth,obj.FontSize.Value,obj.FillStyle,obj.Direction,ls,lc,lp)
                else:
                    svg = getSVG(obj.Source,obj.Scale,obj.LineWidth,obj.FontSize.Value,obj.FillStyle,obj.Direction,ls,lc,lp)
                result += '<g id="' + obj.Name + '"'
                result += ' transform="'
                result += 'rotate('+str(obj.Rotation)+','+str(obj.X)+','+str(obj.Y)+') '
                result += 'translate('+str(obj.X)+','+str(obj.Y)+') '
                result += 'scale('+str(obj.Scale)+','+str(-obj.Scale)+')'
                result += '">'
                result += svg
                result += '</g>'
        obj.ViewResult = result

    def getDXF(self,obj):
        "returns a DXF fragment"
        return getDXF(obj)



class _Array(_DraftLink):
    "The Draft Array object"

    def __init__(self,obj):
        _DraftLink.__init__(self,obj,"Array")

    def attach(self, obj):
        obj.addProperty("App::PropertyLink","Base","Draft",QT_TRANSLATE_NOOP("App::Property","The base object that must be duplicated"))
        obj.addProperty("App::PropertyEnumeration","ArrayType","Draft",QT_TRANSLATE_NOOP("App::Property","The type of array to create"))
        obj.addProperty("App::PropertyVector","Axis","Draft",QT_TRANSLATE_NOOP("App::Property","The axis direction"))
        obj.addProperty("App::PropertyInteger","NumberX","Draft",QT_TRANSLATE_NOOP("App::Property","Number of copies in X direction"))
        obj.addProperty("App::PropertyInteger","NumberY","Draft",QT_TRANSLATE_NOOP("App::Property","Number of copies in Y direction"))
        obj.addProperty("App::PropertyInteger","NumberZ","Draft",QT_TRANSLATE_NOOP("App::Property","Number of copies in Z direction"))
        obj.addProperty("App::PropertyInteger","NumberPolar","Draft",QT_TRANSLATE_NOOP("App::Property","Number of copies"))
        obj.addProperty("App::PropertyVectorDistance","IntervalX","Draft",QT_TRANSLATE_NOOP("App::Property","Distance and orientation of intervals in X direction"))
        obj.addProperty("App::PropertyVectorDistance","IntervalY","Draft",QT_TRANSLATE_NOOP("App::Property","Distance and orientation of intervals in Y direction"))
        obj.addProperty("App::PropertyVectorDistance","IntervalZ","Draft",QT_TRANSLATE_NOOP("App::Property","Distance and orientation of intervals in Z direction"))
        obj.addProperty("App::PropertyVectorDistance","IntervalAxis","Draft",QT_TRANSLATE_NOOP("App::Property","Distance and orientation of intervals in Axis direction"))
        obj.addProperty("App::PropertyVectorDistance","Center","Draft",QT_TRANSLATE_NOOP("App::Property","Center point"))
        obj.addProperty("App::PropertyAngle","Angle","Draft",QT_TRANSLATE_NOOP("App::Property","Angle to cover with copies"))
        obj.addProperty("App::PropertyDistance","RadialDistance","Draft",QT_TRANSLATE_NOOP("App::Property","Distance between copies in a circle"))
        obj.addProperty("App::PropertyDistance","TangentialDistance","Draft",QT_TRANSLATE_NOOP("App::Property","Distance between circles"))
        obj.addProperty("App::PropertyInteger","NumberCircles","Draft",QT_TRANSLATE_NOOP("App::Property","number of circles"))
        obj.addProperty("App::PropertyInteger","Symmetry","Draft",QT_TRANSLATE_NOOP("App::Property","number of circles"))
        obj.addProperty("App::PropertyBool","Fuse","Draft",QT_TRANSLATE_NOOP("App::Property","Specifies if copies must be fused (slower)"))
        obj.Fuse = False
        if self.use_link:
            obj.addProperty("App::PropertyInteger","Count","Draft",'')
            obj.addProperty("App::PropertyBool","ExpandArray","Draft",
                    QT_TRANSLATE_NOOP("App::Property","Show array element as children object"))
            obj.ExpandArray = False

        obj.ArrayType = ['ortho','polar','circular']
        obj.NumberX = 1
        obj.NumberY = 1
        obj.NumberZ = 1
        obj.NumberPolar = 1
        obj.IntervalX = Vector(1,0,0)
        obj.IntervalY = Vector(0,1,0)
        obj.IntervalZ = Vector(0,0,1)
        obj.Angle = 360
        obj.Axis = Vector(0,0,1)
        obj.RadialDistance = 1.0
        obj.TangentialDistance = 1.0
        obj.NumberCircles = 2
        obj.Symmetry = 1

        _DraftLink.attach(self,obj)

    def linkSetup(self,obj):
        _DraftLink.linkSetup(self,obj)
        obj.configLinkProperty(ElementCount='Count')
        obj.setPropertyStatus('Count','Hidden')

    def execute(self,obj):
        if obj.Base:
            pl = obj.Placement
            if obj.ArrayType == "ortho":
                pls = self.rectArray(obj.Base.Placement,obj.IntervalX,obj.IntervalY,
                                    obj.IntervalZ,obj.NumberX,obj.NumberY,obj.NumberZ)
            elif obj.ArrayType == "circular":
                pls = self.circArray(obj.Base.Placement,obj.RadialDistance,obj.TangentialDistance,
                                     obj.Axis,obj.Center,obj.NumberCircles,obj.Symmetry)
            else:
                av = obj.IntervalAxis if hasattr(obj,"IntervalAxis") else None
                pls = self.polarArray(obj.Base.Placement,obj.Center,obj.Angle.Value,obj.NumberPolar,obj.Axis,av)

            return _DraftLink.buildShape(self,obj,pl,pls)

    def rectArray(self,pl,xvector,yvector,zvector,xnum,ynum,znum):
        import Part
        base = [pl.copy()]
        for xcount in range(xnum):
            currentxvector=Vector(xvector).multiply(xcount)
            if not xcount==0:
                npl = pl.copy()
                npl.translate(currentxvector)
                base.append(npl)
            for ycount in range(ynum):
                currentyvector=FreeCAD.Vector(currentxvector)
                currentyvector=currentyvector.add(Vector(yvector).multiply(ycount))
                if not ycount==0:
                    npl = pl.copy()
                    npl.translate(currentyvector)
                    base.append(npl)
                for zcount in range(znum):
                    currentzvector=FreeCAD.Vector(currentyvector)
                    currentzvector=currentzvector.add(Vector(zvector).multiply(zcount))
                    if not zcount==0:
                        npl = pl.copy()
                        npl.translate(currentzvector)
                        base.append(npl)
        return base

    def circArray(self,pl,rdist,tdist,axis,center,cnum,sym):
        import Part
        sym = max(1, sym)
        lead = (0,1,0)
        if axis.x == 0 and axis.z == 0: lead = (1,0,0)
        direction = axis.cross(Vector(lead)).normalize()
        base = [pl.copy()]
        for xcount in range(1, cnum):
            rc = xcount*rdist
            c = 2*rc*math.pi
            n = math.floor(c/tdist)
            n = int(math.floor(n/sym)*sym)
            if n == 0: continue
            angle = 360.0/n
            for ycount in range(0, n):
                npl = pl.copy()
                trans = FreeCAD.Vector(direction).multiply(rc)
                npl.translate(trans)
                npl.rotate(npl.Rotation.inverted().multVec(center-trans), axis, ycount*angle)
                base.append(npl)
        return base

    def polarArray(self,spl,center,angle,num,axis,axisvector):
        #print("angle ",angle," num ",num)
        import Part
        spin = FreeCAD.Placement(Vector(), spl.Rotation)
        pl = FreeCAD.Placement(spl.Base, FreeCAD.Rotation())
        center = center.sub(spl.Base)
        base = [spl.copy()]
        if angle == 360:
            fraction = float(angle)/num
        else:
            if num == 0:
                return base
            fraction = float(angle)/(num-1)
        ctr = DraftVecUtils.tup(center)
        axs = DraftVecUtils.tup(axis)
        for i in range(num-1):
            currangle = fraction + (i*fraction)
            npl = pl.copy()
            npl.rotate(ctr, axs, currangle)
            npl = npl.multiply(spin)
            if axisvector:
                if not DraftVecUtils.isNull(axisvector):
                    npl.translate(FreeCAD.Vector(axisvector).multiply(i+1))
            base.append(npl)
        return base

class _PathArray(_DraftLink):
    """The Draft Path Array object"""

    def __init__(self,obj):
        _DraftLink.__init__(self,obj,"PathArray")

    def attach(self,obj):
        obj.addProperty("App::PropertyLinkGlobal","Base","Draft",QT_TRANSLATE_NOOP("App::Property","The base object that must be duplicated"))
        obj.addProperty("App::PropertyLinkGlobal","PathObj","Draft",QT_TRANSLATE_NOOP("App::Property","The path object along which to distribute objects"))
        obj.addProperty("App::PropertyLinkSubListGlobal","PathSubs",QT_TRANSLATE_NOOP("App::Property","Selected subobjects (edges) of PathObj"))
        obj.addProperty("App::PropertyInteger","Count","Draft",QT_TRANSLATE_NOOP("App::Property","Number of copies"))
        obj.addProperty("App::PropertyVectorDistance","Xlate","Draft",QT_TRANSLATE_NOOP("App::Property","Optional translation vector"))
        obj.addProperty("App::PropertyBool","Align","Draft",QT_TRANSLATE_NOOP("App::Property","Orientation of Base along path"))
        obj.Count = 2
        obj.PathSubs = []
        obj.Xlate = FreeCAD.Vector(0,0,0)
        obj.Align = False

        if self.use_link:
            obj.addProperty("App::PropertyBool","ExpandArray","Draft",
                    QT_TRANSLATE_NOOP("App::Property","Show array element as children object"))
            obj.ExpandArray = False
            obj.setPropertyStatus('Shape','Transient')

        _DraftLink.attach(self,obj)

    def linkSetup(self,obj):
        _DraftLink.linkSetup(self,obj)
        obj.configLinkProperty(ElementCount='Count')

    def execute(self,obj):
        import FreeCAD
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
                FreeCAD.Console.PrintLog ("_PathArray.createGeometry: path " + obj.PathObj.Name + " has no edges\n")
                return
            base = calculatePlacementsOnPath(
                    obj.Base.Shape.Placement.Rotation,w,obj.Count,obj.Xlate,obj.Align)
            return _DraftLink.buildShape(self,obj,pl,base)

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

class _PointArray(_DraftObject):
    """The Draft Point Array object"""
    def __init__(self, obj, bobj, ptlst):
        _DraftObject.__init__(self,obj,"PointArray")
        obj.addProperty("App::PropertyLink","Base","Draft",QT_TRANSLATE_NOOP("App::Property","Base")).Base = bobj
        obj.addProperty("App::PropertyLink","PointList","Draft",QT_TRANSLATE_NOOP("App::Property","PointList")).PointList = ptlst
        obj.addProperty("App::PropertyInteger","Count","Draft",QT_TRANSLATE_NOOP("App::Property","Count")).Count = 0
        obj.setEditorMode("Count", 1)

    def execute(self, obj):
        import Part
        from FreeCAD import Base, Vector
        pls = []
        opl = obj.PointList
        while getType(opl) == 'Clone':
            opl = opl.Objects[0]
        if hasattr(opl, 'Geometry'):
            place = opl.Placement
            for pts in opl.Geometry:
                if hasattr(pts, 'X') and hasattr(pts, 'Y') and hasattr(pts, 'Z'):
                    pn = pts.copy()
                    pn.translate(place.Base)
                    pn.rotate(place)
                    pls.append(pn)
        elif hasattr(opl, 'Links'):
            pls = opl.Links
        elif hasattr(opl, 'Components'):
            pls = opl.Components

        base = []
        i = 0
        if hasattr(obj.Base, 'Shape'):
            for pts in pls:
                #print pts # inspect the objects
                if hasattr(pts, 'X') and hasattr(pts, 'Y') and hasattr(pts, 'Z'):
                    nshape = obj.Base.Shape.copy()
                    if hasattr(pts, 'Placement'):
                        place = pts.Placement
                        nshape.translate(place.Base)
                        nshape.rotate(place.Base, place.Rotation.Axis, place.Rotation.Angle * 180 /  math.pi )
                    else:
                        nshape.translate(Base.Vector(pts.X,pts.Y,pts.Z))
                    i += 1
                    base.append(nshape)
        obj.Count = i
        if i > 0:
            obj.Shape = Part.makeCompound(base)
        else:
            FreeCAD.Console.PrintError(translate("draft","No point found\n"))
            obj.Shape = obj.Base.Shape.copy()


class _ViewProviderDraftArray(_ViewProviderDraft):
    """a view provider that displays a Array icon instead of a Draft icon"""

    def __init__(self,vobj):
        _ViewProviderDraft.__init__(self,vobj)

    def getIcon(self):
        if hasattr(self.Object,"ArrayType"):
            return ":/icons/Draft_Array.svg"
        elif hasattr(self.Object,"PointList"):
            return ":/icons/Draft_PointArray.svg"
        return ":/icons/Draft_PathArray.svg"

    def resetColors(self, vobj):
        colors = []
        if vobj.Object.Base:
            if vobj.Object.Base.isDerivedFrom("Part::Feature"):
                if len(vobj.Object.Base.ViewObject.DiffuseColor) > 1:
                    colors = vobj.Object.Base.ViewObject.DiffuseColor
                else:
                    c = vobj.Object.Base.ViewObject.ShapeColor
                    c = (c[0],c[1],c[2],vobj.Object.Base.ViewObject.Transparency/100.0)
                    for f in vobj.Object.Base.Shape.Faces:
                        colors.append(c)
        if colors:
            n = 1
            if hasattr(vobj.Object,"ArrayType"):
                if vobj.Object.ArrayType == "ortho":
                    n = vobj.Object.NumberX * vobj.Object.NumberY * vobj.Object.NumberZ
                else:
                    n = vobj.Object.NumberPolar
            elif hasattr(vobj.Object,"Count"):
                n = vobj.Object.Count
            colors = colors * n
            vobj.DiffuseColor = colors




## @}
