# -*- coding: utf-8 -*-
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

from draftutils.utils import convert_draft_texts
from draftutils.utils import convertDraftTexts

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

from draftutils.utils import getCloneBase
from draftutils.utils import get_clone_base

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

from draftutils.utils import get_rgb
from draftutils.utils import getrgb

from draftutils.utils import get_DXF
from draftutils.utils import getDXF

import getSVG as svg
getSVG = svg.getSVG

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

from draftfunctions.array import array

from draftfunctions.cut import cut

from draftfunctions.downgrade import downgrade

from draftfunctions.draftify import draftify

from draftfunctions.extrude import extrude

from draftfunctions.fuse import fuse

from draftfunctions.heal import heal

from draftfunctions.move import move
from draftfunctions.move import move_vertex, moveVertex
from draftfunctions.move import move_edge, moveEdge
from draftfunctions.move import copy_moved_edges, copyMovedEdges

from draftfunctions.rotate import rotate
from draftfunctions.rotate import rotate_vertex, rotateVertex
from draftfunctions.rotate import rotate_edge, rotateEdge
from draftfunctions.rotate import copy_rotated_edges, copyRotatedEdges

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

# App::Link support
from draftobjects.draftlink import DraftLink
from draftobjects.draftlink import _DraftLink
from draftviewproviders.view_draftlink import ViewProviderDraftLink
from draftviewproviders.view_draftlink import _ViewProviderDraftLink

# circle
from draftmake.make_circle import make_circle, makeCircle
from draftobjects.circle import Circle, _Circle

# drawing: view NOTE: Obsolete since Drawing was substituted bu TechDraw
from draftmake.make_drawingview import make_drawing_view, makeDrawingView
from draftobjects.drawingview import DrawingView, _DrawingView

# arcs
from draftmake.make_arc_3points import make_arc_3points

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

# arrays
from draftmake.make_patharray import make_path_array, makePathArray
from draftobjects.patharray import PathArray, _PathArray

from draftmake.make_pointarray import make_point_array, makePointArray
from draftobjects.pointarray import PointArray, _PointArray

from draftmake.make_array import make_array, makeArray
from draftobjects.array import Array, _Array

if FreeCAD.GuiUp:
    from draftviewproviders.view_array import ViewProviderDraftArray
    from draftviewproviders.view_array import _ViewProviderDraftArray

from draftmake.make_circulararray import make_circular_array
from draftmake.make_orthoarray import make_ortho_array
from draftmake.make_orthoarray import make_ortho_array2d
from draftmake.make_orthoarray import make_rect_array
from draftmake.make_orthoarray import make_rect_array2d
from draftmake.make_polararray import make_polar_array

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

from draftmake.make_fillet import make_fillet
from draftobjects.fillet import Fillet
if FreeCAD.GuiUp:
    from draftviewproviders.view_fillet import ViewProviderFillet

#---------------------------------------------------------------------------
# Draft annotation objects
#---------------------------------------------------------------------------

from draftobjects.dimension import (LinearDimension,
                                    _Dimension,
                                    AngularDimension,
                                    _AngularDimension)

from draftmake.make_dimension import (make_dimension,
                                      makeDimension,
                                      make_angular_dimension,
                                      makeAngularDimension)

if FreeCAD.GuiUp:
    from draftviewproviders.view_dimension import ViewProviderLinearDimension
    from draftviewproviders.view_dimension import ViewProviderAngularDimension
    _ViewProviderDimension = ViewProviderLinearDimension
    _ViewProviderAngularDimension = ViewProviderAngularDimension

from draftobjects.label import (Label,
                                DraftLabel)

from draftmake.make_label import (make_label,
                                  makeLabel)

if gui:
    from draftviewproviders.view_label import ViewProviderLabel
    ViewProviderDraftLabel = ViewProviderLabel

from draftobjects.text import (Text,
                               DraftText)

from draftmake.make_text import (make_text,
                                 makeText)

if FreeCAD.GuiUp:
    from draftviewproviders.view_text import (ViewProviderText,
                                              ViewProviderDraftText)

## @}
