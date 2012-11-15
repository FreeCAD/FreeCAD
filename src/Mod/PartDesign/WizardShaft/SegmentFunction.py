#/******************************************************************************
# *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
# *                                                                            *
# *   This file is part of the FreeCAD CAx development system.                 *
# *                                                                            *
# *   This library is free software; you can redistribute it and/or            *
# *   modify it under the terms of the GNU Library General Public              *
# *   License as published by the Free Software Foundation; either             *
# *   version 2 of the License, or (at your option) any later version.         *
# *                                                                            *
# *   This library  is distributed in the hope that it will be useful,         *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
# *   GNU Library General Public License for more details.                     *
# *                                                                            *
# *   You should have received a copy of the GNU Library General Public        *
# *   License along with this library; see the file COPYING.LIB. If not,       *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
# *   Suite 330, Boston, MA  02111-1307, USA                                   *
# *                                                                            *
# ******************************************************************************/

import FreeCAD # just for debug printing to console...

class SegmentFunctionSegment:
    "One segment of a segment function"
    start = 0
    variable = "x"
    coefficient = 0
    exponent = 0

    def __init__(self, st, var, coeff, exp):
        self.start = st
        self.variable = var
        self.coefficient = coeff
        self.exponent = exp

    def hasStart(self, xval):
        "Return true if the start of this segment is xval"
        #FIXME: 1E-9 is arbitrary here. But since units are in meters, 1E-9 is a nanometer...
        return abs(self.start - xval) < 1E-9

    def value(self, xval):
        if xval < self.start:
            return 0
        else:
            return self.coefficient * pow(xval - self.start, self.exponent)

    def clone(self):
        return SegmentFunctionSegment(self.start, self.variable, self.coefficient, self.exponent)

    def negate(self):
        self.coefficient *= -1

    def integrate(self):
        self.exponent = self.exponent + 1
        self.coefficient = self.coefficient * 1 / self.exponent

    def asString(self):
        return "%f * {%s - %f}^%i" % (self.coefficient, self.variable, self.start, self.exponent)

class SegmentFunction:
    "Function that is defined segment-wise"
    variable = "x"
    segments = []
    name = "f(x)"

    def __init__(self, name = "f(x)"):
        self.variable = "x"
        self.segments = []
        self.name = name

    def negate(self):
        for s in self.segments:
            s.negate()
        return self

    def index(self, xval):
        "Find insert position for start value xval"
        lastStart = 0.0
        for i in range(len(self.segments)):
            newStart = self.segments[i].start
            if (xval >= lastStart) and (xval < newStart):
                return i
            lastStart = newStart
        return len(self.segments)

    def buildFromDict(self, var, dict):
        self.variable = var
        for key in sorted(dict.iterkeys()):
            #if abs(dict[key]) > 1E-9:
            self.segments.append(SegmentFunctionSegment(key, var, dict[key], 0))

    def addSegments(self, dict):
        for key in sorted(dict.iterkeys()):
            if abs(dict[key]) > 1E-9:
                self.segments.insert(self.index(key), SegmentFunctionSegment(key, self.variable, dict[key], 0))

    def setMaxX(self, mx):
        self.maxX = mx

    def value(self, xval):
        "Return the value of the function at the specified x value"
        result = 0
        for s in self.segments:
            result = result + s.value(xval)
        return result

    def lowervalue(self, xval):
        "Return the value of the previous segment at the specified x value"
        result = 0
        for s in self.segments:
            result = result + s.value(xval - 1E-8)
        return result

    def clone(self):
        result = SegmentFunction()
        result.variable = self.variable
        for s in self.segments:
            result.segments.append(s.clone())
        return result

    def integrate(self):
        "Integrate all segments with respect to the variable"
        for s in self.segments:
            s.integrate()

    def integrated(self):
        "Return a copy of self integrated with respect to the variable"
        result = self.clone()
        result.integrate()
        return result

    def evaluate(self, maxX, pointsX):
        # Note: This usually creates a few more points than specified in pointsX
        offset = (maxX - self.segments[0].start) / (pointsX - 1)
        xvals = set([self.segments[0].start + s * offset for s in range(pointsX)])
        starts = set([self.segments[i].start for i in range(len(self.segments))])
        xvals = xvals.union(starts) # Make sure we have a point on each segment start
        xresult = []
        yresult = []
        for xval in sorted(xvals):
            if xval in starts:
                # create double point at segment border
                xresult.append(xval)
                yresult.append(self.lowervalue(xval))
            xresult.append(xval)
            yresult.append(self.value(xval))
        return (xresult, yresult)

    def output(self):
        FreeCAD.Console.PrintMessage(self.name + " = ")
        for i in range(len(self.segments)):
            FreeCAD.Console.PrintMessage(self.segments[i].asString())
            if i < len(self.segments) - 1:
                FreeCAD.Console.PrintMessage(" + ")
        FreeCAD.Console.PrintMessage("\n")

