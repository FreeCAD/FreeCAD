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
#  @{

import FreeCAD as App

if App.GuiUp:
    import Draft_rc
    gui = True
    # To prevent complaints from code checkers (flake8)
    True if Draft_rc.__name__ else False
else:
    gui = False

__title__ = "FreeCAD Draft Workbench"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin, Daniel Falck")
__url__ = "https://www.freecad.org"

# ---------------------------------------------------------------------------
# Utility functions
# ---------------------------------------------------------------------------
from draftutils.utils import ARROW_TYPES as arrowtypes

from draftutils.utils import (type_check,
                              typecheck,
                              get_param_type,
                              getParamType,
                              get_param,
                              getParam,
                              set_param,
                              setParam,
                              precision,
                              tolerance,
                              epsilon)

from draftutils.utils import (get_real_name,
                              getRealName,
                              get_type,
                              getType,
                              get_objects_of_type,
                              getObjectsOfType,
                              is_clone,
                              isClone,
                              get_clone_base,
                              getCloneBase,
                              print_shape,
                              printShape,
                              compare_objects,
                              compareObjects,
                              shapify,
                              filter_objects_for_modifiers,
                              filterObjectsForModifiers,
                              is_closed_edge,
                              isClosedEdge)

from draftutils.utils import (string_encode_coin,
                              stringencodecoin,
                              load_svg_patterns,
                              loadSvgPatterns,
                              svg_patterns,
                              svgpatterns,
                              get_rgb,
                              getrgb,
                              argb_to_rgba,
                              rgba_to_argb)

from draftfunctions.svg import (get_svg,
                                getSVG)

from draftfunctions.dxf import (get_dxf,
                                getDXF)

from draftutils.gui_utils import (get3DView,
                                  get_3d_view,
                                  autogroup,
                                  removeHidden,
                                  remove_hidden,
                                  get_diffuse_color,
                                  formatObject,
                                  format_object,
                                  getSelection,
                                  get_selection,
                                  getSelectionEx,
                                  get_selection_ex,
                                  select,
                                  loadTexture,
                                  load_texture,
                                  get_bbox)

from draftutils.gui_utils import (dim_symbol,
                                  dimSymbol,
                                  dim_dash,
                                  dimDash)

from draftutils.groups import (is_group,
                               get_group_names,
                               getGroupNames,
                               ungroup,
                               get_windows,
                               get_group_contents,
                               getGroupContents,
                               get_movable_children,
                               getMovableChildren)

# ---------------------------------------------------------------------------
# Draft functions
# ---------------------------------------------------------------------------
from draftfunctions.array import array

from draftfunctions.cut import cut

from draftfunctions.downgrade import downgrade

from draftfunctions.draftify import draftify

from draftfunctions.extrude import extrude

from draftfunctions.fuse import fuse

from draftfunctions.heal import heal

from draftfunctions.move import (move,
                                 move_vertex,
                                 moveVertex,
                                 move_edge,
                                 moveEdge,
                                 copy_moved_edges,
                                 copyMovedEdges)

from draftfunctions.rotate import (rotate,
                                   rotate_vertex,
                                   rotateVertex,
                                   rotate_edge,
                                   rotateEdge,
                                   copy_rotated_edges,
                                   copyRotatedEdges)

from draftfunctions.scale import (scale,
                                  scale_vertex,
                                  scaleVertex,
                                  scale_edge,
                                  scaleEdge,
                                  copy_scaled_edges,
                                  copyScaledEdges)

from draftfunctions.join import (join_wires,
                                 joinWires,
                                 join_two_wires,
                                 joinTwoWires)

from draftfunctions.split import split

from draftfunctions.offset import offset

from draftfunctions.mirror import mirror

from draftfunctions.upgrade import upgrade


# ---------------------------------------------------------------------------
# Draft objects
# ---------------------------------------------------------------------------

# base object
from draftobjects.base import (DraftObject,
                               _DraftObject)

# base viewprovider
from draftviewproviders.view_base import (ViewProviderDraft,
                                          _ViewProviderDraft,
                                          ViewProviderDraftAlt,
                                          _ViewProviderDraftAlt,
                                          ViewProviderDraftPart,
                                          _ViewProviderDraftPart)

# App::Link support, used by the arrays
from draftobjects.draftlink import (DraftLink,
                                    _DraftLink)
from draftviewproviders.view_draftlink import (ViewProviderDraftLink,
                                               _ViewProviderDraftLink)

# circle
from draftobjects.circle import (Circle,
                                 _Circle)
from draftmake.make_circle import (make_circle,
                                   makeCircle)

# arcs
from draftmake.make_arc_3points import make_arc_3points



# ellipse
from draftobjects.ellipse import (Ellipse,
                                  _Ellipse)
from draftmake.make_ellipse import (make_ellipse,
                                    makeEllipse)

# rectangle
from draftobjects.rectangle import (Rectangle,
                                    _Rectangle)
from draftmake.make_rectangle import (make_rectangle,
                                      makeRectangle)

if App.GuiUp:
    from draftviewproviders.view_rectangle import (ViewProviderRectangle,
                                                   _ViewProviderRectangle)

# polygon
from draftobjects.polygon import (Polygon,
                                  _Polygon)
from draftmake.make_polygon import (make_polygon,
                                    makePolygon)

# wire and line
from draftobjects.wire import (Wire,
                               _Wire)

from draftmake.make_line import (make_line,
                                 makeLine)
from draftmake.make_wire import (make_wire,
                                 makeWire)

if App.GuiUp:
    from draftviewproviders.view_wire import (ViewProviderWire,
                                              _ViewProviderWire)

# bspline
from draftobjects.bspline import (BSpline,
                                  _BSpline)
from draftmake.make_bspline import (make_bspline,
                                    makeBSpline)

if App.GuiUp:
    from draftviewproviders.view_bspline import (ViewProviderBSpline,
                                                 _ViewProviderBSpline)

# bezcurve
from draftobjects.bezcurve import (BezCurve,
                                   _BezCurve)
from draftmake.make_bezcurve import (make_bezcurve,
                                     makeBezCurve)

if App.GuiUp:
    from draftviewproviders.view_bezcurve import (ViewProviderBezCurve,
                                                  _ViewProviderBezCurve)

# copy
from draftmake.make_copy import make_copy
from draftmake.make_copy import make_copy as makeCopy

# clone
from draftobjects.clone import (Clone,
                                _Clone)
from draftmake.make_clone import (make_clone,
                                  clone)

if App.GuiUp:
    from draftviewproviders.view_clone import (ViewProviderClone,
                                               _ViewProviderClone)

# point
from draftobjects.point import (Point,
                                _Point)
from draftmake.make_point import (make_point,
                                  makePoint)

if App.GuiUp:
    from draftviewproviders.view_point import (ViewProviderPoint,
                                               _ViewProviderPoint)

# arrays
from draftobjects.array import (Array,
                                _Array)
from draftmake.make_array import (make_array,
                                  makeArray)
from draftmake.make_orthoarray import (make_ortho_array,
                                       make_ortho_array2d,
                                       make_rect_array,
                                       make_rect_array2d)
from draftmake.make_polararray import make_polar_array
from draftmake.make_circulararray import make_circular_array

from draftobjects.patharray import (PathArray,
                                    _PathArray)
from draftmake.make_patharray import (make_path_array,
                                      makePathArray,
                                      make_path_twisted_array)

from draftobjects.pointarray import (PointArray,
                                     _PointArray)
from draftmake.make_pointarray import (make_point_array,
                                       makePointArray)

if App.GuiUp:
    from draftviewproviders.view_array import (ViewProviderDraftArray,
                                               _ViewProviderDraftArray)

# facebinder
from draftobjects.facebinder import (Facebinder,
                                     _Facebinder)
from draftmake.make_facebinder import (make_facebinder,
                                       makeFacebinder)

if App.GuiUp:
    from draftviewproviders.view_facebinder import (ViewProviderFacebinder,
                                                    _ViewProviderFacebinder)

# shapestring
from draftobjects.block import (Block,
                                _Block)
from draftmake.make_block import (make_block,
                                  makeBlock)

# shapestring
from draftobjects.shapestring import (ShapeString,
                                      _ShapeString)
from draftmake.make_shapestring import (make_shapestring,
                                        makeShapeString)
if App.GuiUp:
    from draftviewproviders.view_shapestring import ViewProviderShapeString

# shape 2d view
from draftobjects.shape2dview import (Shape2DView,
                                      _Shape2DView)
from draftmake.make_shape2dview import (make_shape2dview,
                                        makeShape2DView)

# sketch
from draftmake.make_sketch import (make_sketch,
                                   makeSketch)

# working plane proxy
from draftobjects.wpproxy import WorkingPlaneProxy
from draftmake.make_wpproxy import (make_workingplaneproxy,
                                    makeWorkingPlaneProxy)

if App.GuiUp:
    from draftviewproviders.view_wpproxy import ViewProviderWorkingPlaneProxy

from draftobjects.fillet import Fillet
from draftmake.make_fillet import make_fillet

if App.GuiUp:
    from draftviewproviders.view_fillet import ViewProviderFillet

from draftobjects.layer import (Layer,
                                _VisGroup)

from draftmake.make_layer import (make_layer,
                                  makeLayer)

if App.GuiUp:
    from draftviewproviders.view_layer import (ViewProviderLayer,
                                               _ViewProviderVisGroup)

# Annotation objects
from draftobjects.dimension import (LinearDimension,
                                    _Dimension,
                                    AngularDimension,
                                    _AngularDimension)
from draftmake.make_dimension import (make_dimension,
                                      makeDimension,
                                      make_linear_dimension,
                                      make_linear_dimension_obj,
                                      make_radial_dimension_obj,
                                      make_angular_dimension,
                                      makeAngularDimension)

if App.GuiUp:
    from draftviewproviders.view_dimension \
        import (ViewProviderLinearDimension,
                _ViewProviderDimension,
                ViewProviderAngularDimension,
                _ViewProviderAngularDimension)

from draftobjects.label import (Label,
                                DraftLabel)
from draftmake.make_label import (make_label,
                                  makeLabel)

if App.GuiUp:
    from draftviewproviders.view_label import (ViewProviderLabel,
                                               ViewProviderDraftLabel)

from draftobjects.text import (Text,
                               DraftText)
from draftmake.make_text import (make_text,
                                 makeText,
                                 convert_draft_texts,
                                 convertDraftTexts)

if App.GuiUp:
    from draftviewproviders.view_text import (ViewProviderText,
                                              ViewProviderDraftText)

from draftobjects.hatch import (Hatch)
from draftmake.make_hatch import (make_hatch)
if App.GuiUp:
    from draftviewproviders.view_hatch import (ViewProviderDraftHatch)

## @}
