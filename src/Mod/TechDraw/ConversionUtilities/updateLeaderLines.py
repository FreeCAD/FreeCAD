#!/usr/bin/env python
# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2024 Wanderer Fan <wandererfan@gmail.com>               *
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
#
# updates any leader lines in the current document from pre-v0.22 coordinates
# to v0.22+ coordinates.

# usage: open the document to be converted in FreeCAD then run this macro and
#        save the result.


import TechDraw

RezFactor = 10.0

for obj in FreeCAD.ActiveDocument.Objects:
    print("obj: {}".format(obj.Name))
    if obj.isDerivedFrom("TechDraw::DrawLeaderLine"):
        pointsAll = obj.WayPoints
        newPoints = list()
        for point in pointsAll:
            point = point / RezFactor
            newPoints.append(point)
        obj.WayPoints = newPoints

print("conversion complete")
