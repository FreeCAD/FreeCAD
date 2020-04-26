#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
#*   Copyright (c) 2020 Carlo Pavan                                        *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************
"""Provide the object code for Arch Wall object."""
## @package wall
# \ingroup ARCH
# \brief Provide the object code for Arch Wall.

import FreeCAD as App


def find_near_endings(w1, w2):
    pass


def get_walls_intersection(w1, w2):
    w1_core_axis = w1.Proxy.get_core_axis(w1)#.toShape()
    w2_core_axis = w2.Proxy.get_core_axis(w2)#.toShape()
    if w1_core_axis is None or w2_core_axis is None:
        print("Failed to get wall core axis")
        return False

    int_pts = w1_core_axis.intersect(w2_core_axis)
    if len(int_pts) == 1:
        int_p = int_pts[0]        
        intersection = App.Vector(int_p.X,int_p.Y,int_p.Z)
    else:
        print("No intersection point found, or too many intersection points found")
        return False
    return intersection


def join_walls(w1, w2, join_type="T"):
    """
    joinwalls(w1, w2)

    Join two walls

    Parameters
    ----------
    w1 : First wall object

    w2 : Second wall object

    join_type : string
        Join type of the wall, can be:
        "L" corner joint
        "T" extend first wall to the second
        "X" not implemented yet
    """
    intersection = get_walls_intersection(w1, w2)
    if intersection == False:
        return

    if (w1.Proxy.get_first_point(w1).distanceToPoint(intersection) <
        w1.Proxy.get_last_point(w1).distanceToPoint(intersection)):
        w1_end = 0
    else:    
        w1_end = 1

    if (w2.Proxy.get_first_point(w2).distanceToPoint(intersection) <
        w2.Proxy.get_last_point(w2).distanceToPoint(intersection)):
        w2_end = 0
    else:    
        w2_end = 1

    if join_type == "T":
        if w1_end == 0:
            w1.JoinFirstEndTo = w2.Name
        elif w1_end == 1:
            w1.JoinLastEndTo = w2.Name
        return True

    elif join_type == "L":
        raise NotImplementedError

    elif join_type == "X":
        raise NotImplementedError

