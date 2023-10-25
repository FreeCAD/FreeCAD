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
"""Define geometry functions for manipulating shapes in the Draft Workbench.

These functions are used by different object creation functions
of the Draft Workbench, both in `Draft.py` and `DraftTools.py`.
They operate on the internal shapes (`Part::TopoShape`) of different objects
and on their subelements, that is, vertices, edges, and faces.
"""
## \defgroup DRAFTGEOMUTILS DraftGeomUtils
#  \ingroup UTILITIES
#  \brief Shape manipulation utilities for the Draft workbench
#
# Shapes manipulation utilities

## \addtogroup DRAFTGEOMUTILS
#  @{

__title__ = "FreeCAD Draft Workbench - Geometry library"
__author__ = "Yorik van Havre, Jacques-Antoine Gaudin, Ken Cline"
__url__ = ["https://www.freecad.org"]

# Doesn't need requisites
from draftgeoutils.linear_algebra import (linearFromPoints,
                                          determinant)

# Needs math, Part, and vector tools
from draftgeoutils.general import PARAMGRP as params
from draftgeoutils.general import NORM

from draftgeoutils.general import (precision,
                                   vec,
                                   edg,
                                   getVerts,
                                   v1,
                                   isNull,
                                   isPtOnEdge,
                                   hasCurves,
                                   isAligned,
                                   getQuad,
                                   areColinear,
                                   hasOnlyWires,
                                   geomType,
                                   isValidPath,
                                   findClosest,
                                   getBoundaryAngles)

# Needs general functions
from draftgeoutils.geometry import (findPerpendicular,
                                    findDistance,
                                    getSplineNormal,
                                    get_spline_normal,
                                    getNormal,
                                    get_normal,
                                    getRotation,
                                    isPlanar,
                                    is_planar,
                                    calculatePlacement,
                                    mirror,
                                    are_coplanar,
                                    is_straight_line,
                                    mirror_matrix,
                                    uv_vectors_from_face,
                                    placement_from_face,
                                    placement_from_points,
                                    distance_to_plane,
                                    project_point_on_plane)

from draftgeoutils.edges import (findEdge,
                                 orientEdge,
                                 isSameLine,
                                 isLine,
                                 is_line,
                                 invert,
                                 findMidpoint,
                                 getTangent,
                                 get_referenced_edges)

from draftgeoutils.faces import (concatenate,
                                 getBoundary,
                                 isCoplanar,
                                 is_coplanar,
                                 bind,
                                 cleanFaces,
                                 removeSplitter)

from draftgeoutils.arcs import (isClockwise,
                                isWideAngle,
                                arcFrom2Pts,
                                arcFromSpline)

from draftgeoutils.cuboids import (isCubic,
                                   getCubicDimensions)

# Needs geometry functions
from draftgeoutils.circle_inversion import (pointInversion,
                                            polarInversion,
                                            circleInversion)

# Needs edges functions
from draftgeoutils.sort_edges import (sortEdges,
                                      sortEdgesOld)

from draftgeoutils.intersections import (findIntersection,
                                         wiresIntersect,
                                         connect,
                                         angleBisection)

from draftgeoutils.wires import (findWires,
                                 findWiresOld,
                                 findWiresOld2,
                                 flattenWire,
                                 superWire,
                                 isReallyClosed,
                                 curvetowire,
                                 curvetosegment,
                                 rebaseWire,
                                 removeInterVertices,
                                 cleanProjection,
                                 tessellateProjection,
                                 get_placement_perpendicular_to_wire,
                                 get_extended_wire)

# Needs wires functions
from draftgeoutils.fillets import (fillet,
                                   filletWire)

# Needs intersections functions
from draftgeoutils.offsets import (pocket2d,
                                   offset,
                                   offsetWire)

from draftgeoutils.circles import (findClosestCircle,
                                   getCircleFromSpline,
                                   circlefrom1Line2Points,
                                   circlefrom2Lines1Point,
                                   circleFrom2LinesRadius,
                                   circleFrom3LineTangents,
                                   circleFromPointLineRadius,
                                   circleFrom2PointsRadius,
                                   findHomotheticCenterOfCircles,
                                   findRadicalAxis,
                                   findRadicalCenter)

from draftgeoutils.circles_apollonius import (outerSoddyCircle,
                                              innerSoddyCircle,
                                              circleFrom3CircleTangents)

# Needs circles_apollonius functions
# These functions are not imported because they are incomplete;
# they require pre-requisite functions that haven't been written
# from draftgeoutils.circles_incomplete import (circleFrom2tan1pt,
#                                               circleFrom2tan1rad,
#                                               circleFrom1tan2pt,
#                                               circleFrom1tan1pt1rad,
#                                               circleFrom3tan)

##  @}
