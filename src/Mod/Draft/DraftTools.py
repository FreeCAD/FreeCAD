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

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False
True if DraftGui.__name__ else False

__title__ = "FreeCAD Draft Workbench GUI Tools"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin")
__url__ = "https://www.freecad.org"

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
defaultWP = Draft.getParam("defaultWP",0)
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
from draftguitools.gui_hatch import Draft_Hatch

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
from draftguitools.gui_wire2spline import WireToBSpline
from draftguitools.gui_shape2dview import Shape2DView
from draftguitools.gui_draft2sketch import Draft2Sketch
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
