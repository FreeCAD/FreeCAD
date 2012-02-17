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
from shipUtils import Math

def Displacement(ship, draft, trim):
    """ Calculate ship displacement.
    @param ship Selected ship instance
    @param draft Draft.
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
            if (Z > z0) and not (Math.isAprox(Z,z0)):
                factor = (Z - z0) / (z1 - z0)
                y = y0 + factor*(y1 - y0)
                points.append(App.Base.Vector(x,y,Z))
        # Convert into array with n elements (Number of points by sections)
        # with m elements into them (Number of points with the same height,
        # typical of multibody)
        section = []
        nPoints = 0
        j = 0
        while j < len(points)-1:
            section.append([points[j]])
            k = j+1
            while(Math.isAprox(points[j].z, points[k].z)):
                section[nPoints].append(points[k])
                k = k+1
            nPoints = nPoints + 1
            j = k
        # Integrate area
        area = 0.0
        for j in range(0, len(section)-1):
            for k in range(0, min(len(section[j])-1, len(section[j+1])-1)):
                # y11,z11 ------- y01,z01
                #    |               |
                #    |               |
                #    |               |
                # y10,z10 ------- y00,z00
                y00 = abs(section[j][k].y)
                z00 = section[j][k].z
                y10 = abs(section[j][k+1].y)
                z10 = section[j][k+1].z
                y01 = abs(section[j+1][k].y)
                z01 = section[j+1][k].z
                y11 = abs(section[j+1][k+1].y)
                z11 = section[j+1][k+1].z
                dy = 0.5*((y00 - y10) + (y01 - y11))
                dz = 0.5*((z01 - z00) + (z11 - z10))
                area = area + dy*dz
            if(len(section[j]) < len(section[j+1])):
                # y01,z01 ------- y11,z11
                #    |        __/
                #    |     __/
                #    |    /
                # y00,z00
                k = len(section[j])-1
                y00 = abs(section[j][k].y)
                z00 = section[j][k].z
                y01 = abs(section[j+1][k].y)
                z01 = section[j+1][k].z
                y11 = abs(section[j+1][k+1].y)
                z11 = section[j+1][k+1].z
                dy = y01 - y11
                dz = z01 - z00
                area = area + 0.5*dy*dz
            elif(len(section[j]) > len(section[j+1])):
                # y01,z01
                #    |    \__
                #    |       \__
                #    |          \
                # y00,z00 ------- y10,z10
                k = len(section[j+1])-1
                y00 = abs(section[j][k].y)
                z00 = section[j][k].z
                y10 = abs(section[j][k+1].y)
                z10 = section[j][k+1].z
                y01 = abs(section[j+1][k].y)
                z01 = section[j+1][k].z
                dy = y00 - y10
                dz = z01 - z00
                area = area + 0.5*dy*dz
            elif(len(section[j]) == 1):
                # y1,z1 ------- 
                #    |          
                #    |          
                #    |          
                # y0,z0 ------- 
                k = 0
                y0 = abs(section[j][k].y)
                z0 = section[j][k].z
                y1 = abs(section[j+1][k].y)
                z1 = section[j+1][k].z
                dy = 0.5 * (y0 + y1)
                dz = z1 - z0
                area = area + dy*dz
        areas.append(2.0*area)	                # 2x because only half ship is represented
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

def WettedArea(ship, draft, trim):
    """ Calculate wetted ship area.
    @param ship Selected ship instance
    @param draft Draft.
    @param trim Trim in degrees.
    @return Wetted ship area.
    """
    angle    = math.radians(trim)
    sections = Instance.sections(ship)
    xCoord   = ship.xSection[:]
    lines    = []
    area     = 0.0
    if not sections:
        return 0.0
    for i in range(0, len(sections)):
        # Get the section
        section = sections[i]
        if len(section) < 2:    # Empty section
            lines.append(0.0)
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
            lines.append(0.0)
            continue
        # Truncate only valid points
        points = section[0:n]
        # Study if additional point is needed
        if n < len(section):
            y0 = section[n-1].y
            z0 = section[n-1].z
            y1 = section[n].y
            z1 = section[n].z
            if (Z > z0) and not (Math.isAprox(Z,z0)):
                factor = (Z - z0) / (z1 - z0)
                y = y0 + factor*(y1 - y0)
                points.append(App.Base.Vector(x,y,Z))
        # Convert into array with n elements (Number of points by sections)
        # with m elements into them (Number of points with the same height,
        # typical of multibody)
        section = []
        nPoints = 0
        j = 0
        while j < len(points)-1:
            section.append([points[j]])
            k = j+1
            while(Math.isAprox(points[j].z, points[k].z)):
                section[nPoints].append(points[k])
                k = k+1
            nPoints = nPoints + 1
            j = k
        # Integrate line area
        line = 0.0
        for j in range(0, len(section)-1):
            for k in range(0, min(len(section[j])-1, len(section[j+1])-1)):
                # y11,z11 ------- y01,z01
                #    |               |
                #    |               |
                #    |               |
                # y10,z10 ------- y00,z00
                y00 = abs(section[j][k].y)
                z00 = section[j][k].z
                y10 = abs(section[j][k+1].y)
                z10 = section[j][k+1].z
                y01 = abs(section[j+1][k].y)
                z01 = section[j+1][k].z
                y11 = abs(section[j+1][k+1].y)
                z11 = section[j+1][k+1].z
                dy = y11 - y10
                dz = z11 - z10
                line = line + math.sqrt(dy*dy + dz*dz)
                dy = y01 - y00
                dz = z01 - z00
                line = line + math.sqrt(dy*dy + dz*dz)
            if(len(section[j]) < len(section[j+1])):
                # y01,z01 ------- y11,z11
                #    |        __/
                #    |     __/
                #    |    /
                # y00,z00
                k = len(section[j])-1
                y00 = abs(section[j][k].y)
                z00 = section[j][k].z
                y01 = abs(section[j+1][k].y)
                z01 = section[j+1][k].z
                y11 = abs(section[j+1][k+1].y)
                z11 = section[j+1][k+1].z
                dy = y11 - y00
                dz = z11 - z00
                line = line + math.sqrt(dy*dy + dz*dz)
                dy = y01 - y00
                dz = z01 - z00
                line = line + math.sqrt(dy*dy + dz*dz)
            elif(len(section[j]) > len(section[j+1])):
                # y01,z01
                #    |    \__
                #    |       \__
                #    |          \
                # y00,z00 ------- y10,z10
                k = len(section[j+1])-1
                y00 = abs(section[j][k].y)
                z00 = section[j][k].z
                y10 = abs(section[j][k+1].y)
                z10 = section[j][k+1].z
                y01 = abs(section[j+1][k].y)
                z01 = section[j+1][k].z
                dy = y01 - y00
                dz = z01 - z00
                line = line + math.sqrt(dy*dy + dz*dz)
                dy = y01 - y10
                dz = z01 - z10
                line = line + math.sqrt(dy*dy + dz*dz)
            elif(len(section[j]) == 1):
                # y1,z1 ------- 
                #    |          
                #    |          
                #    |          
                # y0,z0 ------- 
                k = 0
                y0 = abs(section[j][k].y)
                z0 = section[j][k].z
                y1 = abs(section[j+1][k].y)
                z1 = section[j+1][k].z
                dy = y1 - y0
                dz = z1 - z0
                line = line + math.sqrt(dy*dy + dz*dz)
        lines.append(2.0*line)	                # 2x because only half ship is represented
        # Add area if proceed
        if i > 0:
            dx     = xCoord[i] - xCoord[i-1]
            x      = 0.5*(xCoord[i] + xCoord[i-1])
            line   = 0.5*(lines[i] + lines[i-1])
            area   = area + line*dx
    return area

def Moment(ship, draft, trim, disp, xcb):
    """ Calculate triming 1cm ship moment.
    @param ship Selected ship instance
    @param draft Draft.
    @param trim Trim in degrees.
    @param disp Displacement at selected draft and trim.
    @param xcb Bouyance center at selected draft and trim.
    @return Moment to trim ship 1cm (ton m).
    @note Moment is positive when produce positive trim.
    """
    angle = math.degrees(math.atan2(0.01,0.5*ship.Length))
    newTrim = trim + angle
    data = Displacement(ship,draft,newTrim)
    mom0 = -disp*xcb
    mom1 = -data[1]*data[2]
    return mom1 - mom0

class Point:
    """ Hydrostatics point, that conatins: \n
    draft Ship draft [m]. \n
    trim Ship trim [deg]. \n
    disp Ship displacement [ton]. \n
    xcb Bouyance center X coordinate [m].
    wet Wetted ship area [m2].
    mom triming 1cm ship moment [ton m].
    @note Moment is positive when produce positive trim.
    """
    def __init__(self, ship, draft, trim):
        """ Use all hydrostatics tools to define a hydrostatics 
        point.
        @param ship Selected ship instance
        @param draft Draft.
        @param trim Trim in degrees.
        """
        # Hydrostatics computation
        areasData  = Displacement(ship,draft,trim)
        wettedArea = WettedArea(ship,draft,trim)
        moment     = Moment(ship,draft,trim,areasData[1],areasData[2])
        # Store final data
        self.draft = draft
        self.trim  = trim
        self.disp  = areasData[1]
        self.xcb   = areasData[2]
        self.wet   = wettedArea
        self.mom   = moment
