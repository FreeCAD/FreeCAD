#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *  
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *  
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

import math
# FreeCAD modules
import FreeCAD as App
import FreeCADGui as Gui
# Module
import Instance

def Displacement(ship, draft, trim):
    """ Calculate ship displacement.
    @param ship Selected ship instance
    @param traft Draft.
    @param trim Trim in degrees.
    @return [areas,disp,xcb]: \n
    areas : Area of each section \n
    disp: Ship displacement \n
    xcb: X bouyance center coordinate
    """
    angle    = math.radians(trim)
    sections = Instance.sections(ship)
    xCoord   = ship.xSection[:]
    areas    = []
    vol      = 0.0
    moment   = 0.0
    if not sections:
        return [[],0.0,0.0]
    for i in range(0, len(sections)):
        # Get the section
        section = sections[i]
        if len(section) < 2:    # Empty section
            areas.append(0.0)
            continue
        # Get the position of the section
        x = xCoord[i]
        # Get the maximum Z value
        Z = draft - x*math.tan(angle)
        # Count the number of valid points
        n = 0
        for j in range(0,len(section)):
            z = section[j].z
            if z > Z:
                break
            n = n+1
        # Discard invalid sections
        if n == 0:
            areas.append(0.0)
            continue
        # Truncate only valid points
        points = section[0:n]
        # Study if additional point is needed
        if n < len(section):
            y0 = section[n-1].y
            z0 = section[n-1].z
            y1 = section[n].y
            z1 = section[n].z
            factor = (Z - z0) / (z1 - z0)
            y = y0 + factor*(y1 - y0)
            points.append(App.Base.Vector(x,y,Z))
        # Integrate area
        area = 0.0
        for j in range(0, len(points)-1):
            y0 = abs(points[j].y)
            z0 = points[j].z
            y1 = abs(points[j+1].y)
            z1 = points[j+1].z
            y  = 0.5 * (y0 + y1)
            dz = z1 - z0
            area = area + 2.0*y*dz	# 2x because only half ship is represented
        areas.append(area)
        # Add volume & moment if proceed
        if i > 0:
            dx     = xCoord[i] - xCoord[i-1]
            x      = 0.5*(xCoord[i] + xCoord[i-1])
            area   = 0.5*(areas[i] + areas[i-1])
            vol    = vol + area*dx
            moment = moment + area*dx*x
    # Compute displacement and xcb
    disp = vol / 1.025  # rho = 1.025 ton/m3 (salt water density)
    xcb  = 0.0
    if vol > 0.0:
        xcb  = moment / vol
    return [areas,disp,xcb]
