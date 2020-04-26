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

from draftutils.utils import filter_objects_for_modifiers
from draftutils.utils import filterObjectsForModifiers

from draftutils.utils import is_closed_edge
from draftutils.utils import isClosedEdge

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
# Draft functions
#---------------------------------------------------------------------------

from draftfunctions.cut import cut

from draftfunctions.downgrade import downgrade

from draftfunctions.draftify import draftify

from draftfunctions.extrude import extrude

from draftfunctions.fuse import fuse

from draftfunctions.heal import heal

from draftfunctions.move import move
from draftfunctions.rotate import rotate

from draftfunctions.scale import scale
from draftfunctions.scale import scale_vertex, scaleVertex
from draftfunctions.scale import scale_edge, scaleEdge
from draftfunctions.scale import copy_scaled_edges, copyScaledEdges

from draftfunctions.join import join_wires
from draftfunctions.join import join_wires as joinWires
from draftfunctions.join import join_two_wires
from draftfunctions.join import join_two_wires as joinTwoWires

from draftfunctions.split import split

from draftfunctions.offset import offset

from draftfunctions.mirror import mirror

from draftfunctions.upgrade import upgrade

#---------------------------------------------------------------------------
# Draft objects
#---------------------------------------------------------------------------

# base object
from draftobjects.base import DraftObject
from draftobjects.base import _DraftObject

# base viewprovider
from draftviewproviders.view_base import ViewProviderDraft
from draftviewproviders.view_base import _ViewProviderDraft
from draftviewproviders.view_base import ViewProviderDraftAlt
from draftviewproviders.view_base import _ViewProviderDraftAlt
from draftviewproviders.view_base import ViewProviderDraftPart
from draftviewproviders.view_base import _ViewProviderDraftPart

# circle
from draftmake.make_circle import make_circle, makeCircle
from draftobjects.circle import Circle, _Circle

# ellipse
from draftmake.make_ellipse import make_ellipse, makeEllipse
from draftobjects.ellipse import Ellipse, _Ellipse

# rectangle
from draftmake.make_rectangle import make_rectangle, makeRectangle
from draftobjects.rectangle import Rectangle, _Rectangle
if FreeCAD.GuiUp:
    from draftviewproviders.view_rectangle import ViewProviderRectangle
    from draftviewproviders.view_rectangle import _ViewProviderRectangle

# polygon
from draftmake.make_polygon import make_polygon, makePolygon
from draftobjects.polygon import Polygon, _Polygon

# wire and line
from draftmake.make_line import make_line, makeLine
from draftmake.make_wire import make_wire, makeWire
from draftobjects.wire import Wire, _Wire
if FreeCAD.GuiUp:
    from draftviewproviders.view_wire import ViewProviderWire
    from draftviewproviders.view_wire import _ViewProviderWire

# bspline
from draftmake.make_bspline import make_bspline, makeBSpline
from draftobjects.bspline import BSpline, _BSpline
if FreeCAD.GuiUp:
    from draftviewproviders.view_bspline import ViewProviderBSpline
    from draftviewproviders.view_bspline import _ViewProviderBSpline

# bezcurve
from draftmake.make_bezcurve import make_bezcurve, makeBezCurve
from draftobjects.bezcurve import BezCurve, _BezCurve
if FreeCAD.GuiUp:
    from draftviewproviders.view_bezcurve import ViewProviderBezCurve
    from draftviewproviders.view_bezcurve import _ViewProviderBezCurve

# copy
from draftmake.make_copy import make_copy
from draftmake.make_copy import make_copy as makeCopy

# clone
from draftmake.make_clone import make_clone, clone
from draftobjects.clone import Clone, _Clone
if FreeCAD.GuiUp:
    from draftviewproviders.view_clone import ViewProviderClone
    from draftviewproviders.view_clone import _ViewProviderClone

# point
from draftmake.make_point import make_point, makePoint
from draftobjects.point import Point, _Point
if FreeCAD.GuiUp:
    from draftviewproviders.view_point import ViewProviderPoint
    from draftviewproviders.view_point import _ViewProviderPoint

# facebinder
from draftmake.make_facebinder import make_facebinder, makeFacebinder
from draftobjects.facebinder import Facebinder, _Facebinder
if FreeCAD.GuiUp:
    from draftviewproviders.view_facebinder import ViewProviderFacebinder
    from draftviewproviders.view_facebinder import _ViewProviderFacebinder

# shapestring
from draftmake.make_block import make_block, makeBlock
from draftobjects.block import Block, _Block

# shapestring
from draftmake.make_shapestring import make_shapestring, makeShapeString
from draftobjects.shapestring import ShapeString, _ShapeString

# shape 2d view
from draftmake.make_shape2dview import make_shape2dview, makeShape2DView
from draftobjects.shape2dview import Shape2DView, _Shape2DView

# sketch
from draftmake.make_sketch import make_sketch, makeSketch

# working plane proxy
from draftmake.make_wpproxy import make_workingplaneproxy
from draftmake.make_wpproxy import makeWorkingPlaneProxy
from draftobjects.wpproxy import WorkingPlaneProxy
if FreeCAD.GuiUp:
    from draftviewproviders.view_wpproxy import ViewProviderWorkingPlaneProxy

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


from draftobjects.label import make_label
from draftobjects.label import Label

makeLabel = make_label
DraftLabel = Label

if gui:
    from draftviewproviders.view_label import ViewProviderLabel
    ViewProviderDraftLabel = ViewProviderLabel


from draftobjects.text import make_text
from draftobjects.text import Text
makeText = make_text
DraftText = Text

if gui:
    from draftviewproviders.view_text import ViewProviderText
    ViewProviderDraftText = ViewProviderText

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

    nullv = FreeCAD.Vector(0, 0, 0)

    # get a local coord system (tangent, normal, binormal) at parameter offset (normally length)
    t = edge.tangentAt(getParameterFromV0(edge, offset))
    t.normalize()

    try:
        n = edge.normalAt(getParameterFromV0(edge, offset))
        n.normalize()
        b = (t.cross(n))
        b.normalize()
    # no normal defined here
    except FreeCAD.Base.FreeCADError:
        n = nullv
        b = nullv
        FreeCAD.Console.PrintMessage(
            "Draft PathArray.orientShape - Cannot calculate Path normal.\n")

    priority = "ZXY"        #the default. Doesn't seem to affect results.
    newRot = FreeCAD.Rotation(t, n, b, priority);
    newGRot = newRot.multiply(globalRotation)

    placement.Rotation = newGRot
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
import draftobjects.base
_DraftObject = draftobjects.base.DraftObject

class _ViewProviderDraftLink:
    "a view provider for link type object"

    def __init__(self,vobj):
        self.Object = vobj.Object
        vobj.Proxy = self

    def attach(self,vobj):
        self.Object = vobj.Object

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def getIcon(self):
        tp = self.Object.Proxy.Type
        if tp == 'Array':
            if self.Object.ArrayType == 'ortho':
                return ":/icons/Draft_LinkArray.svg"
            elif self.Object.ArrayType == 'polar':
                return ":/icons/Draft_PolarLinkArray.svg"
            elif self.Object.ArrayType == 'circular':
                return ":/icons/Draft_CircularLinkArray.svg"
        elif tp == 'PathArray':
            return ":/icons/Draft_PathLinkArray.svg"

    def claimChildren(self):
        obj = self.Object
        if hasattr(obj,'ExpandArray'):
            expand = obj.ExpandArray
        else:
            expand = obj.ShowElement
        if not expand:
            return [obj.Base]
        else:
            return obj.ElementList


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


class _DraftLink(_DraftObject):

    def __init__(self,obj,tp):
        self.use_link = False if obj else True
        _DraftObject.__init__(self,obj,tp)
        if obj:
            self.attach(obj)

    def __getstate__(self):
        return self.__dict__

    def __setstate__(self,state):
        if isinstance(state,dict):
            self.__dict__ = state
        else:
            self.use_link = False
            _DraftObject.__setstate__(self,state)

    def attach(self,obj):
        if self.use_link:
            obj.addExtension('App::LinkExtensionPython', None)
            self.linkSetup(obj)

    def canLinkProperties(self,_obj):
        return False

    def linkSetup(self,obj):
        obj.configLinkProperty('Placement',LinkedObject='Base')
        if hasattr(obj,'ShowElement'):
            # rename 'ShowElement' property to 'ExpandArray' to avoid conflict
            # with native App::Link
            obj.configLinkProperty('ShowElement')
            showElement = obj.ShowElement
            obj.addProperty("App::PropertyBool","ExpandArray","Draft",
                    QT_TRANSLATE_NOOP("App::Property","Show array element as children object"))
            obj.ExpandArray = showElement
            obj.configLinkProperty(ShowElement='ExpandArray')
            obj.removeProperty('ShowElement')
        else:
            obj.configLinkProperty(ShowElement='ExpandArray')
        if getattr(obj,'ExpandArray',False):
            obj.setPropertyStatus('PlacementList','Immutable')
        else:
            obj.setPropertyStatus('PlacementList','-Immutable')
        if not hasattr(obj,'LinkTransform'):
            obj.addProperty('App::PropertyBool','LinkTransform',' Link')
        if not hasattr(obj,'ColoredElements'):
            obj.addProperty('App::PropertyLinkSubHidden','ColoredElements',' Link')
            obj.setPropertyStatus('ColoredElements','Hidden')
        obj.configLinkProperty('LinkTransform','ColoredElements')

    def getViewProviderName(self,_obj):
        if self.use_link:
            return 'Gui::ViewProviderLinkPython'
        return ''

    def migrate_attributes(self, obj):
        """Migrate old attribute names to new names if they exist.

        This is done to comply with Python guidelines or fix small issues
        in older code.
        """
        if hasattr(self, "useLink"):
            # This is only needed for some models created in 0.19
            # while it was in development. Afterwards,
            # all models should use 'use_link' by default
            # and this won't be run.
            self.use_link = bool(self.useLink)
            FreeCAD.Console.PrintWarning("Migrating 'useLink' to 'use_link', "
                                         "{} ({})\n".format(obj.Label,
                                                            obj.TypeId))
            del self.useLink

    def onDocumentRestored(self, obj):
        self.migrate_attributes(obj)
        if self.use_link:
            self.linkSetup(obj)
        else:
            obj.setPropertyStatus('Shape','-Transient')
        if obj.Shape.isNull():
            if getattr(obj,'PlacementList',None):
                self.buildShape(obj,obj.Placement,obj.PlacementList)
            else:
                self.execute(obj)

    def buildShape(self,obj,pl,pls):
        import Part
        import DraftGeomUtils

        if self.use_link:
            if not getattr(obj,'ExpandArray',True) or obj.Count != len(pls):
                obj.setPropertyStatus('PlacementList','-Immutable')
                obj.PlacementList = pls
                obj.setPropertyStatus('PlacementList','Immutable')
                obj.Count = len(pls)

        if obj.Base:
            shape = Part.getShape(obj.Base)
            if shape.isNull():
                raise RuntimeError("'{}' cannot build shape of '{}'\n".format(
                        obj.Name,obj.Base.Name))
            else:
                shape = shape.copy()
                shape.Placement = FreeCAD.Placement()
                base = []
                for i,pla in enumerate(pls):
                    vis = getattr(obj,'VisibilityList',[])
                    if len(vis)>i and not vis[i]:
                        continue;
                    # 'I' is a prefix for disambiguation when mapping element names
                    base.append(shape.transformed(pla.toMatrix(),op='I{}'.format(i)))
                if getattr(obj,'Fuse',False) and len(base) > 1:
                    obj.Shape = base[0].multiFuse(base[1:]).removeSplitter()
                else:
                    obj.Shape = Part.makeCompound(base)

                if not DraftGeomUtils.isNull(pl):
                    obj.Placement = pl

        if self.use_link:
            return False # return False to call LinkExtension::execute()

    def onChanged(self, obj, prop):
        if not getattr(self,'use_link',False):
            return
        if prop == 'Fuse':
            if obj.Fuse:
                obj.setPropertyStatus('Shape','-Transient')
            else:
                obj.setPropertyStatus('Shape','Transient')
        elif prop == 'ExpandArray':
            if hasattr(obj,'PlacementList'):
                obj.setPropertyStatus('PlacementList',
                        '-Immutable' if obj.ExpandArray else 'Immutable')


class _Array(_DraftLink):
    "The Draft Array object"

    def __init__(self,obj):
        _DraftLink.__init__(self,obj,"Array")

    def attach(self, obj):
        obj.addProperty("App::PropertyLink","Base","Draft",QT_TRANSLATE_NOOP("App::Property","The base object that must be duplicated"))
        obj.addProperty("App::PropertyEnumeration","ArrayType","Draft",QT_TRANSLATE_NOOP("App::Property","The type of array to create"))
        obj.addProperty("App::PropertyLinkGlobal","AxisReference","Draft",QT_TRANSLATE_NOOP("App::Property","The axis (e.g. DatumLine) overriding Axis/Center"))
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

    def onChanged(self,obj,prop):
        _DraftLink.onChanged(self,obj,prop)
        if prop == "AxisReference":
            if obj.AxisReference:
                obj.setEditorMode("Center", 1)
                obj.setEditorMode("Axis", 1)
            else:
                obj.setEditorMode("Center", 0)
                obj.setEditorMode("Axis", 0)

    def execute(self,obj):
        if obj.Base:
            pl = obj.Placement
            axis = obj.Axis
            center = obj.Center
            if hasattr(obj,"AxisReference") and obj.AxisReference:
                if hasattr(obj.AxisReference,"Placement"):
                    axis = obj.AxisReference.Placement.Rotation * Vector(0,0,1)
                    center = obj.AxisReference.Placement.Base
                else:
                    raise TypeError("AxisReference has no Placement attribute. Please select a different AxisReference.")
            if obj.ArrayType == "ortho":
                pls = self.rectArray(obj.Base.Placement,obj.IntervalX,obj.IntervalY,
                                    obj.IntervalZ,obj.NumberX,obj.NumberY,obj.NumberZ)
            elif obj.ArrayType == "circular":
                pls = self.circArray(obj.Base.Placement,obj.RadialDistance,obj.TangentialDistance,
                                     axis,center,obj.NumberCircles,obj.Symmetry)
            else:
                av = obj.IntervalAxis if hasattr(obj,"IntervalAxis") else None
                pls = self.polarArray(obj.Base.Placement,center,obj.Angle.Value,obj.NumberPolar,axis,av)

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
        obj.addProperty("App::PropertyVector","TangentVector","Draft",QT_TRANSLATE_NOOP("App::Property","Alignment of copies"))

        obj.Count = 2
        obj.PathSubs = []
        obj.Xlate = FreeCAD.Vector(0,0,0)
        obj.Align = False
        obj.TangentVector = FreeCAD.Vector(1.0, 0.0, 0.0)

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
            if (hasattr(obj, "TangentVector")) and (obj.Align):
                basePlacement = obj.Base.Shape.Placement
                baseRotation = basePlacement.Rotation
                stdX = FreeCAD.Vector(1.0, 0.0, 0.0)
                preRotation = FreeCAD.Rotation(stdX, obj.TangentVector)   #make rotation from X to TangentVector
                netRotation = baseRotation.multiply(preRotation)
                base = calculatePlacementsOnPath(
                        netRotation,w,obj.Count,obj.Xlate,obj.Align)
            else:
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
        if hasattr(self.Object, "ArrayType"):
            if self.Object.ArrayType == 'ortho':
                return ":/icons/Draft_Array.svg"
            elif self.Object.ArrayType == 'polar':
                return ":/icons/Draft_PolarArray.svg"
            elif self.Object.ArrayType == 'circular':
                return ":/icons/Draft_CircularArray.svg"
        elif hasattr(self.Object, "PointList"):
            return ":/icons/Draft_PointArray.svg"
        else:
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
