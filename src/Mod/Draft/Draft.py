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
__url__ = "https://www.freecadweb.org"

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
                              getrgb)

from draftfunctions.svg import (get_svg,
                                getSVG)

from draftfunctions.dxf import (get_dxf,
                                getDXF)

from draftutils.gui_utils import (get3DView,
                                  get_3d_view,
                                  autogroup,
                                  removeHidden,
                                  remove_hidden,
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

from draftutils.groups import (get_group_names,
                               getGroupNames,
                               ungroup,
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

# drawing: obsolete since Drawing was replaced by TechDraw
from draftobjects.drawingview import (DrawingView,
                                      _DrawingView)
from draftmake.make_drawingview import (make_drawing_view,
                                        makeDrawingView)

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

## @}


# Silence complaints from static analyzers about the several hundred unused imports
# by "using" each of them in turn.
True if arrowtypes else False #pylint: disable=W0104
True if type_check.__name__ else False #pylint: disable=W0104
True if typecheck.__name__ else False #pylint: disable=W0104
True if get_param_type.__name__ else False #pylint: disable=W0104
True if getParamType.__name__ else False #pylint: disable=W0104
True if get_param.__name__ else False #pylint: disable=W0104
True if getParam.__name__ else False #pylint: disable=W0104
True if set_param.__name__ else False #pylint: disable=W0104
True if setParam.__name__ else False #pylint: disable=W0104
True if precision.__name__ else False #pylint: disable=W0104
True if tolerance.__name__ else False #pylint: disable=W0104
True if epsilon.__name__ else False #pylint: disable=W0104
True if get_real_name.__name__ else False #pylint: disable=W0104
True if getRealName.__name__ else False #pylint: disable=W0104
True if get_type.__name__ else False #pylint: disable=W0104
True if getType.__name__ else False #pylint: disable=W0104
True if get_objects_of_type.__name__ else False #pylint: disable=W0104
True if getObjectsOfType.__name__ else False #pylint: disable=W0104
True if is_clone.__name__ else False #pylint: disable=W0104
True if isClone.__name__ else False #pylint: disable=W0104
True if get_clone_base.__name__ else False #pylint: disable=W0104
True if getCloneBase.__name__ else False #pylint: disable=W0104
True if print_shape.__name__ else False #pylint: disable=W0104
True if printShape.__name__ else False #pylint: disable=W0104
True if compare_objects.__name__ else False #pylint: disable=W0104
True if compareObjects.__name__ else False #pylint: disable=W0104
True if shapify.__name__ else False #pylint: disable=W0104
True if filter_objects_for_modifiers.__name__ else False #pylint: disable=W0104
True if filterObjectsForModifiers.__name__ else False #pylint: disable=W0104
True if is_closed_edge.__name__ else False #pylint: disable=W0104
True if isClosedEdge.__name__ else False #pylint: disable=W0104
True if string_encode_coin.__name__ else False #pylint: disable=W0104
True if stringencodecoin.__name__ else False #pylint: disable=W0104
True if load_svg_patterns.__name__ else False #pylint: disable=W0104
True if loadSvgPatterns.__name__ else False #pylint: disable=W0104
True if svg_patterns.__name__ else False #pylint: disable=W0104
True if svgpatterns.__name__ else False #pylint: disable=W0104
True if get_rgb.__name__ else False #pylint: disable=W0104
True if getrgb.__name__ else False #pylint: disable=W0104
True if get_svg.__name__ else False #pylint: disable=W0104
True if getSVG.__name__ else False #pylint: disable=W0104
True if get_dxf.__name__ else False #pylint: disable=W0104
True if getDXF.__name__ else False #pylint: disable=W0104
True if get3DView.__name__ else False #pylint: disable=W0104
True if get_3d_view.__name__ else False #pylint: disable=W0104
True if autogroup.__name__ else False #pylint: disable=W0104
True if removeHidden.__name__ else False #pylint: disable=W0104
True if remove_hidden.__name__ else False #pylint: disable=W0104
True if formatObject.__name__ else False #pylint: disable=W0104
True if format_object.__name__ else False #pylint: disable=W0104
True if getSelection.__name__ else False #pylint: disable=W0104
True if get_selection.__name__ else False #pylint: disable=W0104
True if getSelectionEx.__name__ else False #pylint: disable=W0104
True if get_selection_ex.__name__ else False #pylint: disable=W0104
True if select.__name__ else False #pylint: disable=W0104
True if loadTexture.__name__ else False #pylint: disable=W0104
True if load_texture.__name__ else False #pylint: disable=W0104
True if get_bbox.__name__ else False #pylint: disable=W0104
True if dim_symbol.__name__ else False #pylint: disable=W0104
True if dimSymbol.__name__ else False #pylint: disable=W0104
True if dim_dash.__name__ else False #pylint: disable=W0104
True if dimDash.__name__ else False #pylint: disable=W0104
True if get_group_names.__name__ else False #pylint: disable=W0104
True if getGroupNames.__name__ else False #pylint: disable=W0104
True if ungroup.__name__ else False #pylint: disable=W0104
True if get_group_contents.__name__ else False #pylint: disable=W0104
True if getGroupContents.__name__ else False #pylint: disable=W0104
True if get_movable_children.__name__ else False #pylint: disable=W0104
True if getMovableChildren.__name__ else False #pylint: disable=W0104
True if array.__name__ else False #pylint: disable=W0104
True if cut.__name__ else False #pylint: disable=W0104
True if downgrade.__name__ else False #pylint: disable=W0104
True if draftify.__name__ else False #pylint: disable=W0104
True if extrude.__name__ else False #pylint: disable=W0104
True if fuse.__name__ else False #pylint: disable=W0104
True if heal.__name__ else False #pylint: disable=W0104
True if move.__name__ else False #pylint: disable=W0104
True if move_vertex.__name__ else False #pylint: disable=W0104
True if moveVertex.__name__ else False #pylint: disable=W0104
True if move_edge.__name__ else False #pylint: disable=W0104
True if moveEdge.__name__ else False #pylint: disable=W0104
True if copy_moved_edges.__name__ else False #pylint: disable=W0104
True if copyMovedEdges.__name__ else False #pylint: disable=W0104
True if rotate.__name__ else False #pylint: disable=W0104
True if rotate_vertex.__name__ else False #pylint: disable=W0104
True if rotateVertex.__name__ else False #pylint: disable=W0104
True if rotate_edge.__name__ else False #pylint: disable=W0104
True if rotateEdge.__name__ else False #pylint: disable=W0104
True if copy_rotated_edges.__name__ else False #pylint: disable=W0104
True if copyRotatedEdges.__name__ else False #pylint: disable=W0104
True if scale.__name__ else False #pylint: disable=W0104
True if scale_vertex.__name__ else False #pylint: disable=W0104
True if scaleVertex.__name__ else False #pylint: disable=W0104
True if scale_edge.__name__ else False #pylint: disable=W0104
True if scaleEdge.__name__ else False #pylint: disable=W0104
True if copy_scaled_edges.__name__ else False #pylint: disable=W0104
True if copyScaledEdges.__name__ else False #pylint: disable=W0104
True if join_wires.__name__ else False #pylint: disable=W0104
True if joinWires.__name__ else False #pylint: disable=W0104
True if join_two_wires.__name__ else False #pylint: disable=W0104
True if joinTwoWires.__name__ else False #pylint: disable=W0104
True if split.__name__ else False #pylint: disable=W0104
True if offset.__name__ else False #pylint: disable=W0104
True if mirror.__name__ else False #pylint: disable=W0104
True if upgrade.__name__ else False #pylint: disable=W0104
True if DraftObject.__name__ else False #pylint: disable=W0104
True if _DraftObject.__name__ else False #pylint: disable=W0104
True if ViewProviderDraft.__name__ else False #pylint: disable=W0104
True if _ViewProviderDraft.__name__ else False #pylint: disable=W0104
True if ViewProviderDraftAlt.__name__ else False #pylint: disable=W0104
True if _ViewProviderDraftAlt.__name__ else False #pylint: disable=W0104
True if ViewProviderDraftPart.__name__ else False #pylint: disable=W0104
True if _ViewProviderDraftPart.__name__ else False #pylint: disable=W0104
True if DraftLink.__name__ else False #pylint: disable=W0104
True if _DraftLink.__name__ else False #pylint: disable=W0104
True if ViewProviderDraftLink.__name__ else False #pylint: disable=W0104
True if _ViewProviderDraftLink.__name__ else False #pylint: disable=W0104
True if Circle.__name__ else False #pylint: disable=W0104
True if _Circle.__name__ else False #pylint: disable=W0104
True if make_circle.__name__ else False #pylint: disable=W0104
True if makeCircle.__name__ else False #pylint: disable=W0104
True if make_arc_3points.__name__ else False #pylint: disable=W0104
True if DrawingView.__name__ else False #pylint: disable=W0104
True if _DrawingView.__name__ else False #pylint: disable=W0104
True if make_drawing_view.__name__ else False #pylint: disable=W0104
True if makeDrawingView.__name__ else False #pylint: disable=W0104
True if Ellipse.__name__ else False #pylint: disable=W0104
True if _Ellipse.__name__ else False #pylint: disable=W0104
True if make_ellipse.__name__ else False #pylint: disable=W0104
True if makeEllipse.__name__ else False #pylint: disable=W0104
True if Rectangle.__name__ else False #pylint: disable=W0104
True if _Rectangle.__name__ else False #pylint: disable=W0104
True if make_rectangle.__name__ else False #pylint: disable=W0104
True if makeRectangle.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderRectangle.__name__ else False #pylint: disable=W0104
    True if _ViewProviderRectangle.__name__ else False #pylint: disable=W0104
True if Polygon.__name__ else False #pylint: disable=W0104
True if _Polygon.__name__ else False #pylint: disable=W0104
True if make_polygon.__name__ else False #pylint: disable=W0104
True if makePolygon.__name__ else False #pylint: disable=W0104
True if Wire.__name__ else False #pylint: disable=W0104
True if _Wire.__name__ else False #pylint: disable=W0104
True if make_line.__name__ else False #pylint: disable=W0104
True if makeLine.__name__ else False #pylint: disable=W0104
True if make_wire.__name__ else False #pylint: disable=W0104
True if makeWire.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderWire.__name__ else False #pylint: disable=W0104
    True if _ViewProviderWire.__name__ else False #pylint: disable=W0104
True if BSpline.__name__ else False #pylint: disable=W0104
True if _BSpline.__name__ else False #pylint: disable=W0104
True if make_bspline.__name__ else False #pylint: disable=W0104
True if makeBSpline.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderBSpline.__name__ else False #pylint: disable=W0104
    True if _ViewProviderBSpline.__name__ else False #pylint: disable=W0104
True if BezCurve.__name__ else False #pylint: disable=W0104
True if _BezCurve.__name__ else False #pylint: disable=W0104
True if make_bezcurve.__name__ else False #pylint: disable=W0104
True if makeBezCurve.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderBezCurve.__name__ else False #pylint: disable=W0104
    True if _ViewProviderBezCurve.__name__ else False #pylint: disable=W0104
True if make_copy.__name__ else False #pylint: disable=W0104
True if makeCopy.__name__ else False #pylint: disable=W0104
True if Clone.__name__ else False #pylint: disable=W0104
True if _Clone.__name__ else False #pylint: disable=W0104
True if make_clone.__name__ else False #pylint: disable=W0104
True if clone.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderClone.__name__ else False #pylint: disable=W0104
    True if _ViewProviderClone.__name__ else False #pylint: disable=W0104
True if Point.__name__ else False #pylint: disable=W0104
True if _Point.__name__ else False #pylint: disable=W0104
True if make_point.__name__ else False #pylint: disable=W0104
True if makePoint.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderPoint.__name__ else False #pylint: disable=W0104
    True if _ViewProviderPoint.__name__ else False #pylint: disable=W0104
True if Array.__name__ else False #pylint: disable=W0104
True if _Array.__name__ else False #pylint: disable=W0104
True if make_array.__name__ else False #pylint: disable=W0104
True if makeArray.__name__ else False #pylint: disable=W0104
True if make_ortho_array.__name__ else False #pylint: disable=W0104
True if make_ortho_array2d.__name__ else False #pylint: disable=W0104
True if make_rect_array.__name__ else False #pylint: disable=W0104
True if make_rect_array2d.__name__ else False #pylint: disable=W0104
True if make_polar_array.__name__ else False #pylint: disable=W0104
True if make_circular_array.__name__ else False #pylint: disable=W0104
True if PathArray.__name__ else False #pylint: disable=W0104
True if _PathArray.__name__ else False #pylint: disable=W0104
True if make_path_array.__name__ else False #pylint: disable=W0104
True if makePathArray.__name__ else False #pylint: disable=W0104
True if make_path_twisted_array.__name__ else False #pylint: disable=W0104
True if PointArray.__name__ else False #pylint: disable=W0104
True if _PointArray.__name__ else False #pylint: disable=W0104
True if make_point_array.__name__ else False #pylint: disable=W0104
True if makePointArray.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderDraftArray.__name__ else False #pylint: disable=W0104
    True if _ViewProviderDraftArray.__name__ else False #pylint: disable=W0104
True if Facebinder.__name__ else False #pylint: disable=W0104
True if _Facebinder.__name__ else False #pylint: disable=W0104
True if make_facebinder.__name__ else False #pylint: disable=W0104
True if makeFacebinder.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderFacebinder.__name__ else False #pylint: disable=W0104
    True if _ViewProviderFacebinder.__name__ else False #pylint: disable=W0104
True if Block.__name__ else False #pylint: disable=W0104
True if _Block.__name__ else False #pylint: disable=W0104
True if make_block.__name__ else False #pylint: disable=W0104
True if makeBlock.__name__ else False #pylint: disable=W0104
True if ShapeString.__name__ else False #pylint: disable=W0104
True if _ShapeString.__name__ else False #pylint: disable=W0104
True if make_shapestring.__name__ else False #pylint: disable=W0104
True if makeShapeString.__name__ else False #pylint: disable=W0104
True if Shape2DView.__name__ else False #pylint: disable=W0104
True if _Shape2DView.__name__ else False #pylint: disable=W0104
True if make_shape2dview.__name__ else False #pylint: disable=W0104
True if makeShape2DView.__name__ else False #pylint: disable=W0104
True if make_sketch.__name__ else False #pylint: disable=W0104
True if makeSketch.__name__ else False #pylint: disable=W0104
True if WorkingPlaneProxy.__name__ else False #pylint: disable=W0104
True if make_workingplaneproxy.__name__ else False #pylint: disable=W0104
True if makeWorkingPlaneProxy.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderWorkingPlaneProxy.__name__ else False #pylint: disable=W0104
True if Fillet.__name__ else False #pylint: disable=W0104
True if make_fillet.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderFillet.__name__ else False #pylint: disable=W0104
True if Layer.__name__ else False #pylint: disable=W0104
True if _VisGroup.__name__ else False #pylint: disable=W0104
True if make_layer.__name__ else False #pylint: disable=W0104
True if makeLayer.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderLayer.__name__ else False #pylint: disable=W0104
    True if _ViewProviderVisGroup.__name__ else False #pylint: disable=W0104
True if LinearDimension.__name__ else False #pylint: disable=W0104
True if _Dimension.__name__ else False #pylint: disable=W0104
True if AngularDimension.__name__ else False #pylint: disable=W0104
True if _AngularDimension.__name__ else False #pylint: disable=W0104
True if make_dimension.__name__ else False #pylint: disable=W0104
True if makeDimension.__name__ else False #pylint: disable=W0104
True if make_linear_dimension.__name__ else False #pylint: disable=W0104
True if make_linear_dimension_obj.__name__ else False #pylint: disable=W0104
True if make_radial_dimension_obj.__name__ else False #pylint: disable=W0104
True if make_angular_dimension.__name__ else False #pylint: disable=W0104
True if makeAngularDimension.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderLinearDimension.__name__ else False #pylint: disable=W0104
    True if _ViewProviderDimension.__name__ else False #pylint: disable=W0104
    True if ViewProviderAngularDimension.__name__ else False #pylint: disable=W0104
    True if _ViewProviderAngularDimension.__name__ else False #pylint: disable=W0104
True if Label.__name__ else False #pylint: disable=W0104
True if DraftLabel.__name__ else False #pylint: disable=W0104
True if make_label.__name__ else False #pylint: disable=W0104
True if makeLabel.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderLabel.__name__ else False #pylint: disable=W0104
    True if ViewProviderDraftLabel.__name__ else False #pylint: disable=W0104
True if Text.__name__ else False #pylint: disable=W0104
True if DraftText.__name__ else False #pylint: disable=W0104
True if make_text.__name__ else False #pylint: disable=W0104
True if makeText.__name__ else False #pylint: disable=W0104
True if convert_draft_texts.__name__ else False #pylint: disable=W0104
True if convertDraftTexts.__name__ else False #pylint: disable=W0104
if App.GuiUp:
    True if ViewProviderText.__name__ else False #pylint: disable=W0104
    True if ViewProviderDraftText.__name__ else False #pylint: disable=W0104
