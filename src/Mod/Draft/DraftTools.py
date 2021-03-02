# -*- coding: utf8 -*-
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
"""Provide GUI commands of the Draft Workbench.

This module loads all graphical commands of the Draft Workbench,
that is, those actions that can be called from menus and buttons.
This module must be imported only when the graphical user interface
is available, for example, during the workbench definition in `IntiGui.py`.
"""
## @package DraftTools
#  \ingroup DRAFT
#  \brief Provide GUI commands of the Draft workbench.
#
#  This module contains all the graphical commands of the Draft workbench,
#  that is, those actions that can be called from menus and buttons.

# ---------------------------------------------------------------------------
# Generic stuff
# ---------------------------------------------------------------------------
import math
import sys
from PySide import QtCore, QtGui
from pivy import coin

import FreeCAD
import FreeCADGui
from FreeCAD import Vector

import Draft
import Draft_rc
import DraftGui  # Initializes the DraftToolBar class
import DraftVecUtils
import WorkingPlane
from draftutils.todo import ToDo
from draftutils.translate import translate
import draftguitools.gui_snapper as gui_snapper
import draftguitools.gui_trackers as trackers

__title__ = "FreeCAD Draft Workbench GUI Tools"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin")
__url__ = "https://www.freecadweb.org"

if not hasattr(FreeCADGui, "Snapper"):
    FreeCADGui.Snapper = gui_snapper.Snapper()

if not hasattr(FreeCAD, "DraftWorkingPlane"):
    FreeCAD.DraftWorkingPlane = WorkingPlane.plane()

# ---------------------------------------------------------------------------
# Commands that have been migrated to their own modules
# ---------------------------------------------------------------------------
import draftguitools.gui_edit
import draftguitools.gui_selectplane
import draftguitools.gui_setstyle
import draftguitools.gui_planeproxy
from draftguitools.gui_lineops import FinishLine
from draftguitools.gui_lineops import CloseLine
from draftguitools.gui_lineops import UndoLine
from draftguitools.gui_togglemodes import ToggleConstructionMode
from draftguitools.gui_togglemodes import ToggleContinueMode
from draftguitools.gui_togglemodes import ToggleDisplayMode
from draftguitools.gui_groups import AddToGroup
from draftguitools.gui_groups import SelectGroup
from draftguitools.gui_groups import SetAutoGroup
from draftguitools.gui_groups import Draft_AddConstruction
from draftguitools.gui_grid import ToggleGrid
from draftguitools.gui_heal import Heal
from draftguitools.gui_dimension_ops import Draft_FlipDimension
from draftguitools.gui_lineslope import Draft_Slope
import draftguitools.gui_arrays
import draftguitools.gui_annotationstyleeditor
from draftguitools.gui_layers import Layer

# ---------------------------------------------------------------------------
# Preflight stuff
# ---------------------------------------------------------------------------
# update the translation engine
FreeCADGui.updateLocale()

# sets the default working plane
plane = WorkingPlane.plane()
FreeCAD.DraftWorkingPlane = plane
defaultWP = Draft.getParam("defaultWP",1)
if defaultWP == 1: plane.alignToPointAndAxis(Vector(0,0,0), Vector(0,0,1), 0)
elif defaultWP == 2: plane.alignToPointAndAxis(Vector(0,0,0), Vector(0,1,0), 0)
elif defaultWP == 3: plane.alignToPointAndAxis(Vector(0,0,0), Vector(1,0,0), 0)

# last snapped objects, for quick intersection calculation
lastObj = [0,0]

# Set modifier keys
from draftguitools.gui_tool_utils import MODCONSTRAIN
from draftguitools.gui_tool_utils import MODSNAP
from draftguitools.gui_tool_utils import MODALT

# ---------------------------------------------------------------------------
# General functions
# ---------------------------------------------------------------------------
from draftguitools.gui_tool_utils import formatUnit

from draftguitools.gui_tool_utils import selectObject

from draftguitools.gui_tool_utils import getPoint

from draftguitools.gui_tool_utils import getSupport

from draftguitools.gui_tool_utils import setWorkingPlaneToObjectUnderCursor

from draftguitools.gui_tool_utils import setWorkingPlaneToSelectedObject

from draftguitools.gui_tool_utils import hasMod

from draftguitools.gui_tool_utils import setMod

# ---------------------------------------------------------------------------
# Base Class
# ---------------------------------------------------------------------------
from draftguitools.gui_base_original import DraftTool

# ---------------------------------------------------------------------------
# Geometry constructors
# ---------------------------------------------------------------------------
from draftguitools.gui_tool_utils import redraw3DView

from draftguitools.gui_base_original import Creator

from draftguitools.gui_lines import Line
from draftguitools.gui_lines import Wire
from draftguitools.gui_fillets import Fillet
from draftguitools.gui_splines import BSpline
from draftguitools.gui_beziers import BezCurve
from draftguitools.gui_beziers import CubicBezCurve
from draftguitools.gui_beziers import BezierGroup
from draftguitools.gui_rectangles import Rectangle
from draftguitools.gui_arcs import Arc
from draftguitools.gui_arcs import Draft_Arc_3Points
from draftguitools.gui_circles import Circle
from draftguitools.gui_polygons import Polygon
from draftguitools.gui_ellipses import Ellipse
from draftguitools.gui_texts import Text
from draftguitools.gui_dimensions import Dimension
from draftguitools.gui_shapestrings import ShapeString
from draftguitools.gui_points import Point
from draftguitools.gui_facebinders import Draft_Facebinder
from draftguitools.gui_labels import Draft_Label

# ---------------------------------------------------------------------------
# Modifier functions
# ---------------------------------------------------------------------------
from draftguitools.gui_base_original import Modifier

from draftguitools.gui_subelements import SubelementHighlight
from draftguitools.gui_move import Move
from draftguitools.gui_styles import ApplyStyle
from draftguitools.gui_rotate import Rotate
from draftguitools.gui_offset import Offset
from draftguitools.gui_stretch import Stretch
from draftguitools.gui_join import Join
from draftguitools.gui_split import Split
from draftguitools.gui_upgrade import Upgrade
from draftguitools.gui_downgrade import Downgrade
from draftguitools.gui_trimex import Trimex
from draftguitools.gui_scale import Scale
from draftguitools.gui_drawing import Drawing
from draftguitools.gui_wire2spline import WireToBSpline
from draftguitools.gui_shape2dview import Shape2DView
from draftguitools.gui_draft2sketch import Draft2Sketch
from draftguitools.gui_array_simple import Array
from draftguitools.gui_array_simple import LinkArray
from draftguitools.gui_patharray import PathArray
from draftguitools.gui_patharray import PathLinkArray
from draftguitools.gui_pointarray import PointArray
import draftguitools.gui_arrays
from draftguitools.gui_clone import Draft_Clone
from draftguitools.gui_mirror import Mirror

# ---------------------------------------------------------------------------
# Snap tools
# ---------------------------------------------------------------------------
from draftguitools.gui_snaps import Draft_Snap_Lock
from draftguitools.gui_snaps import Draft_Snap_Midpoint
from draftguitools.gui_snaps import Draft_Snap_Perpendicular
from draftguitools.gui_snaps import Draft_Snap_Grid
from draftguitools.gui_snaps import Draft_Snap_Intersection
from draftguitools.gui_snaps import Draft_Snap_Parallel
from draftguitools.gui_snaps import Draft_Snap_Endpoint
from draftguitools.gui_snaps import Draft_Snap_Angle
from draftguitools.gui_snaps import Draft_Snap_Center
from draftguitools.gui_snaps import Draft_Snap_Extension
from draftguitools.gui_snaps import Draft_Snap_Near
from draftguitools.gui_snaps import Draft_Snap_Ortho
from draftguitools.gui_snaps import Draft_Snap_Special
from draftguitools.gui_snaps import Draft_Snap_Dimensions
from draftguitools.gui_snaps import Draft_Snap_WorkingPlane
from draftguitools.gui_snaps import ShowSnapBar

# ---------------------------------------------------------------------------
# Adds the icons & commands to the FreeCAD command manager, and sets defaults
# ---------------------------------------------------------------------------

# drawing commands

# modification commands

# context commands

# a global place to look for active draft Command
FreeCAD.activeDraftCommand = None


# Silence complaints from static analyzers about the several hundred unused imports
# by "using" each of them in turn.
True if math.__name__ else False #pylint: disable=W0104
True if sys.__name__ else False #pylint: disable=W0104
True if QtCore.__name__ else False #pylint: disable=W0104
True if QtGui.__name__ else False #pylint: disable=W0104
True if coin.__name__ else False #pylint: disable=W0104
True if FreeCAD.__name__ else False #pylint: disable=W0104
True if FreeCADGui.__name__ else False #pylint: disable=W0104
True if Vector.__name__ else False #pylint: disable=W0104
True if Draft.__name__ else False #pylint: disable=W0104
True if Draft_rc.__name__ else False #pylint: disable=W0104
True if DraftGui.__name__ else False #pylint: disable=W0104
True if DraftVecUtils.__name__ else False #pylint: disable=W0104
True if WorkingPlane.__name__ else False #pylint: disable=W0104
True if ToDo.__name__ else False #pylint: disable=W0104
True if translate.__name__ else False #pylint: disable=W0104
True if gui_snapper.__name__ else False #pylint: disable=W0104
True if trackers.__name__ else False #pylint: disable=W0104
True if draftguitools.gui_edit.__name__ else False #pylint: disable=W0104
True if draftguitools.gui_selectplane.__name__ else False #pylint: disable=W0104
True if draftguitools.gui_setstyle.__name__ else False #pylint: disable=W0104
True if draftguitools.gui_planeproxy.__name__ else False #pylint: disable=W0104
True if FinishLine.__name__ else False #pylint: disable=W0104
True if CloseLine.__name__ else False #pylint: disable=W0104
True if UndoLine.__name__ else False #pylint: disable=W0104
True if ToggleConstructionMode.__name__ else False #pylint: disable=W0104
True if ToggleContinueMode.__name__ else False #pylint: disable=W0104
True if ToggleDisplayMode.__name__ else False #pylint: disable=W0104
True if AddToGroup.__name__ else False #pylint: disable=W0104
True if SelectGroup.__name__ else False #pylint: disable=W0104
True if SetAutoGroup.__name__ else False #pylint: disable=W0104
True if Draft_AddConstruction.__name__ else False #pylint: disable=W0104
True if ToggleGrid.__name__ else False #pylint: disable=W0104
True if Heal.__name__ else False #pylint: disable=W0104
True if Draft_FlipDimension.__name__ else False #pylint: disable=W0104
True if Draft_Slope.__name__ else False #pylint: disable=W0104
True if draftguitools.gui_arrays.__name__ else False #pylint: disable=W0104
True if draftguitools.gui_annotationstyleeditor.__name__ else False #pylint: disable=W0104
True if Layer.__name__ else False #pylint: disable=W0104
True if MODCONSTRAIN else False #pylint: disable=W0104
True if MODSNAP else False #pylint: disable=W0104
True if MODALT else False #pylint: disable=W0104
True if formatUnit.__name__ else False #pylint: disable=W0104
True if selectObject.__name__ else False #pylint: disable=W0104
True if getPoint.__name__ else False #pylint: disable=W0104
True if getSupport.__name__ else False #pylint: disable=W0104
True if setWorkingPlaneToObjectUnderCursor.__name__ else False #pylint: disable=W0104
True if setWorkingPlaneToSelectedObject.__name__ else False #pylint: disable=W0104
True if hasMod.__name__ else False #pylint: disable=W0104
True if setMod.__name__ else False #pylint: disable=W0104
True if DraftTool.__name__ else False #pylint: disable=W0104
True if redraw3DView.__name__ else False #pylint: disable=W0104
True if Creator.__name__ else False #pylint: disable=W0104
True if Line.__name__ else False #pylint: disable=W0104
True if Wire.__name__ else False #pylint: disable=W0104
True if Fillet.__name__ else False #pylint: disable=W0104
True if BSpline.__name__ else False #pylint: disable=W0104
True if BezCurve.__name__ else False #pylint: disable=W0104
True if CubicBezCurve.__name__ else False #pylint: disable=W0104
True if BezierGroup.__name__ else False #pylint: disable=W0104
True if Rectangle.__name__ else False #pylint: disable=W0104
True if Arc.__name__ else False #pylint: disable=W0104
True if Draft_Arc_3Points.__name__ else False #pylint: disable=W0104
True if Circle.__name__ else False #pylint: disable=W0104
True if Polygon.__name__ else False #pylint: disable=W0104
True if Ellipse.__name__ else False #pylint: disable=W0104
True if Text.__name__ else False #pylint: disable=W0104
True if Dimension.__name__ else False #pylint: disable=W0104
True if ShapeString.__name__ else False #pylint: disable=W0104
True if Point.__name__ else False #pylint: disable=W0104
True if Draft_Facebinder.__name__ else False #pylint: disable=W0104
True if Draft_Label.__name__ else False #pylint: disable=W0104
True if Modifier.__name__ else False #pylint: disable=W0104
True if SubelementHighlight.__name__ else False #pylint: disable=W0104
True if Move.__name__ else False #pylint: disable=W0104
True if ApplyStyle.__name__ else False #pylint: disable=W0104
True if Rotate.__name__ else False #pylint: disable=W0104
True if Offset.__name__ else False #pylint: disable=W0104
True if Stretch.__name__ else False #pylint: disable=W0104
True if Join.__name__ else False #pylint: disable=W0104
True if Split.__name__ else False #pylint: disable=W0104
True if Upgrade.__name__ else False #pylint: disable=W0104
True if Downgrade.__name__ else False #pylint: disable=W0104
True if Trimex.__name__ else False #pylint: disable=W0104
True if Scale.__name__ else False #pylint: disable=W0104
True if Drawing.__name__ else False #pylint: disable=W0104
True if WireToBSpline.__name__ else False #pylint: disable=W0104
True if Shape2DView.__name__ else False #pylint: disable=W0104
True if Draft2Sketch.__name__ else False #pylint: disable=W0104
True if Array.__name__ else False #pylint: disable=W0104
True if LinkArray.__name__ else False #pylint: disable=W0104
True if PathArray.__name__ else False #pylint: disable=W0104
True if PathLinkArray.__name__ else False #pylint: disable=W0104
True if PointArray.__name__ else False #pylint: disable=W0104
True if draftguitools.gui_arrays.__name__ else False #pylint: disable=W0104
True if Draft_Clone.__name__ else False #pylint: disable=W0104
True if Mirror.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Lock.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Midpoint.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Perpendicular.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Grid.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Intersection.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Parallel.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Endpoint.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Angle.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Center.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Extension.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Near.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Ortho.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Special.__name__ else False #pylint: disable=W0104
True if Draft_Snap_Dimensions.__name__ else False #pylint: disable=W0104
True if Draft_Snap_WorkingPlane.__name__ else False #pylint: disable=W0104
True if ShowSnapBar.__name__ else False #pylint: disable=W0104
