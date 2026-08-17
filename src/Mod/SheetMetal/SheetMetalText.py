# ######################################################################
#
#  SheetMetalText.py
# - Text handling for sheet metal sketches, including bend line labels and dimensions.
#
#  Copyright 2026 Shai Seger <shaise at gmail dot com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
# ######################################################################

import math

import FreeCAD
import Part
from FreeCAD import Base, Vector
import SheetMetalTools

smEpsilon = SheetMetalTools.smEpsilon


# Technical font for dimensions and labels. Made from lines and arcs
# Codes:
# s: start point (absolute)
# l: line to (relative)
# q: quarter arc clockwise, with center point (relative)
# h: half arc clockwise, with center point (relative)
# c: full circle clockwise, with center point (relative)

smSketchFont = {
    "0": "s0,2 l0,6 q2,0 l2,0 q0,-2 l0,-6 q-2,0 l-2,0 q0,2",
    "1": "s0,10 l3,0 l0,-10 s0,0 l6,0",
    "2": "s0,8 q2,0 l2,0 h0,-2 l-2,0 s6,0 l-6,0 l0,4 q2,0",
    "3": "s0,8 q2,0 l2,0 h0,-2 l-2,0 s4,6 q0,-2 l0,-2 q-2,0 l-2,0 q0,2",
    "4": "s2,10 l-2,-8 l6,0 s4,0 l0,6",
    "5": "s6,10 l-6,0 l0,-4 l4,0 q0,-2 l0,-2 q-2,0 l-4,0",
    "6": "s0,6 l4,0 q0,-2 l0,-2 q-2,0 l-2,0 q0,2 l0,6 q2,0 l2,0 q0,-2",
    "7": "s0,8 l0,2 l6,0 l-4,-10",
    "8": "s2,6 h0,2 l2,0 h0,-2 l-2,0 s4,6 q0,-2 l0,-2 q-2,0 l-2,0 q0,2 l0,2 q2,0",
    "9": "s6,6 l-4,0 h0,2 l2,0 q0,-2 l0,-6 q-2,0 l-2,0 q0,2",
    "-": "s0,5 l6,0",
    ".": "s2,1 c1,0",
    "^": "s0,8 c2,0", # used for degrees symbol
}

smArcAngles = {
    'q': 90,
    'h': 180,
    'c': 360,
}

def smtGenerateDigit(digit: str, basePos: Vector) -> list[Part.Edge] | None:
    if digit not in smSketchFont:
        return None
    code = smSketchFont[digit]
    edges = []
    curpos = basePos
    planeNormal = Vector(0, 0, 1)
    for cmd in code.split():
        op = cmd[0]
        x, y = map(float, cmd[1:].split(','))
        if op == 's':
            curpos = basePos + Vector(x, y, 0)
            continue
        edge = None
        relpos = curpos + Vector(x, y, 0)
        if op == 'l':
            edge = Part.makeLine(curpos, relpos)
            endpos = relpos
        elif op in smArcAngles:
            radius = (relpos - curpos).Length
            endAngle = math.degrees(math.atan2(-y, -x))
            startAngle = endAngle - smArcAngles[op]
            edge = Part.makeCircle(radius, relpos, planeNormal, startAngle, endAngle)
            if op == 'c':
                endpos = curpos
            elif op == 'h':
                endpos = relpos + Vector(x, y, 0)
            else:  # 'q'
                endpos = relpos + Vector(-y, x, 0)
        if edge:
            edges.append(edge)
        curpos = endpos        
    return edges

def smtGenerateText(text: str, 
                    basePos: Vector, 
                    fontSize: float = 1.0,
                    direction: Vector = Vector(0, 1, 0)
                    ) -> Part.Compound | None:
    allEdges = []
    textPos = Vector(-len(text) * 4, 0, 0)  # center text horizontally around basePos
    for i, char in enumerate(text):
        digitEdges = smtGenerateDigit(char, textPos + Vector(i * 8, 0, 0))
        if digitEdges is None:
            continue
        allEdges.extend(digitEdges)
    if not allEdges:
        return None
    comp = Part.Compound(allEdges)
    # Scale and rotate the compound to match the desired font size and direction
    comp.scale(fontSize / 10.0)
    angle = math.atan2(direction.y, direction.x)
    comp.rotate(Base.Vector(0, 0, 0), Base.Vector(0, 0, 1), math.degrees(angle))
    comp.translate(basePos)
    return comp