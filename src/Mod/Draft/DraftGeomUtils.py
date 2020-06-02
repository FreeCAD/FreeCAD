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
import cmath
import math

import FreeCAD
import Part
import DraftVecUtils
from FreeCAD import Vector

__title__ = "FreeCAD Draft Workbench - Geometry library"
__author__ = "Yorik van Havre, Jacques-Antoine Gaudin, Ken Cline"
__url__ = ["https://www.freecadweb.org"]

from draftgeoutils.general import PARAMGRP as params

from draftgeoutils.general import NORM

# Generic functions *********************************************************


from draftgeoutils.general import precision


from draftgeoutils.general import vec


from draftgeoutils.general import edg


from draftgeoutils.general import getVerts


from draftgeoutils.general import v1


from draftgeoutils.general import isNull


from draftgeoutils.general import isPtOnEdge


from draftgeoutils.general import hasCurves


from draftgeoutils.general import isAligned


from draftgeoutils.general import getQuad


from draftgeoutils.general import areColinear


from draftgeoutils.general import hasOnlyWires


from draftgeoutils.general import geomType


from draftgeoutils.general import isValidPath


# edge functions *************************************************************


from draftgeoutils.edges import findEdge


from draftgeoutils.intersections import findIntersection


from draftgeoutils.intersections import wiresIntersect


from draftgeoutils.offsets import pocket2d


from draftgeoutils.edges import orientEdge


from draftgeoutils.geometry import mirror


from draftgeoutils.arcs import isClockwise


from draftgeoutils.edges import isSameLine


from draftgeoutils.arcs import isWideAngle


from draftgeoutils.general import findClosest


from draftgeoutils.faces import concatenate


from draftgeoutils.faces import getBoundary


from draftgeoutils.edges import isLine


from draftgeoutils.sort_edges import sortEdges


from draftgeoutils.sort_edges import sortEdgesOld


from draftgeoutils.edges import invert


from draftgeoutils.wires import flattenWire


from draftgeoutils.wires import findWires


from draftgeoutils.wires import findWiresOld2


from draftgeoutils.wires import superWire


from draftgeoutils.edges import findMidpoint


from draftgeoutils.geometry import findPerpendicular


from draftgeoutils.offsets import offset


from draftgeoutils.wires import isReallyClosed


from draftgeoutils.geometry import getSplineNormal


from draftgeoutils.geometry import getNormal


from draftgeoutils.geometry import getRotation


from draftgeoutils.geometry import calculatePlacement


from draftgeoutils.offsets import offsetWire


from draftgeoutils.intersections import connect


from draftgeoutils.geometry import findDistance


from draftgeoutils.intersections import angleBisection


from draftgeoutils.circles import findClosestCircle


from draftgeoutils.faces import isCoplanar


from draftgeoutils.geometry import isPlanar


from draftgeoutils.wires import findWiresOld


from draftgeoutils.edges import getTangent


from draftgeoutils.faces import bind


from draftgeoutils.faces import cleanFaces


from draftgeoutils.cuboids import isCubic


from draftgeoutils.cuboids import getCubicDimensions


from draftgeoutils.wires import removeInterVertices


from draftgeoutils.arcs import arcFromSpline


from draftgeoutils.fillets import fillet


from draftgeoutils.fillets import filletWire


from draftgeoutils.circles import getCircleFromSpline


from draftgeoutils.wires import curvetowire


from draftgeoutils.wires import cleanProjection


from draftgeoutils.wires import curvetosegment


from draftgeoutils.wires import tessellateProjection


from draftgeoutils.wires import rebaseWire


from draftgeoutils.faces import removeSplitter


# circle functions *********************************************************


from draftgeoutils.general import getBoundaryAngles


# These functions are not imported because they are incomplete;
# they require pre-requisite functions that haven't been written
# from draftgeoutils.circles_incomplete import circleFrom2tan1pt
# from draftgeoutils.circles_incomplete import circleFrom2tan1rad
# from draftgeoutils.circles_incomplete import circleFrom1tan2pt
# from draftgeoutils.circles_incomplete import circleFrom1tan1pt1rad
# from draftgeoutils.circles_incomplete import circleFrom3tan


from draftgeoutils.circles import circlefrom2Lines1Point


from draftgeoutils.circles import circlefrom1Line2Points


from draftgeoutils.circles import circleFrom2LinesRadius


from draftgeoutils.circles import circleFrom3LineTangents


from draftgeoutils.circles import circleFromPointLineRadius


from draftgeoutils.circles import circleFrom2PointsRadius


from draftgeoutils.arcs import arcFrom2Pts


from draftgeoutils.circles_apollonius import outerSoddyCircle


from draftgeoutils.circles_apollonius import innerSoddyCircle


from draftgeoutils.circles_apollonius import circleFrom3CircleTangents


from draftgeoutils.linear_algebra import linearFromPoints


from draftgeoutils.linear_algebra import determinant


from draftgeoutils.circles import findHomotheticCenterOfCircles


from draftgeoutils.circles import findRadicalAxis


from draftgeoutils.circles import findRadicalCenter


from draftgeoutils.circle_inversion import pointInversion


from draftgeoutils.circle_inversion import polarInversion


from draftgeoutils.circle_inversion import circleInversion

##  @}
