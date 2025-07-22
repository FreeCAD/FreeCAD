# -*- coding: utf-8 -*-
#/******************************************************************************
# *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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
import numpy as np

class SegmentFunctionSegment:
    "One segment of a segment function"

    def __init__(self, st, var, coeff, exp):
        self.start = st
        self.variable = var
        self.coefficient = coeff
        self.exponent = exp

    def hasStart(self, xval):
        "Return true if the start of this segment is xval"
        #FIXME: 1E-9 is arbitrary here. But since units are in meters, 1E-9 is a nanometer...
        return abs(self.start - xval) < 1E-9

    def isZero(self):
        #FIXME: 1E-9 is arbitrary here. But since units are in meters, 1E-9 is a nanometer...
        return abs(self.coefficient) < 1E-5

    def value(self, xval):
        if xval < self.start:
            return 0
        else:
            return self.coefficient * pow(xval - self.start, self.exponent)

    def clone(self):
        return SegmentFunctionSegment(self.start, self.variable, self.coefficient, self.exponent)

    def negate(self):
        self.coefficient *= -1
        return self

    def negated(self):
        return SegmentFunctionSegment(self.start, self.variable, self.coefficient * -1.0, self.exponent)

    def __mul__(self,  value):
        return SegmentFunctionSegment(self.start, self.variable, self.coefficient * value, self.exponent)

    def integrate(self):
        self.exponent = self.exponent + 1
        self.coefficient = self.coefficient * 1 / self.exponent
        return self

    def asString(self):
        return "%f * {%s - %f}^%i" % (self.coefficient, self.variable, self.start, self.exponent)

class SegmentFunction:
    "Function that is defined segment-wise"

    def __init__(self, name = "f(x)"):
        self.variable = "x"
        self.segments = []
        self.name = name

    def findSegment(self,  xval):
        "Find segment valid for the given xval"
        for s in self.segments:
            if s.start <= xval:
                return s
        return self.segments[len(self.segments)]

    def isZero(self):
        for s in self.segments:
            if not s.isZero():
                return False
        return True

    def negate(self):
        for s in self.segments:
            s.negate()
        return self

    def negated(self):
        result = SegmentFunction()
        result.variable = self.variable
        for s in self.segments:
            result.segments.append(s.negated())
        return result

    def __mul__(self,  value):
        result = SegmentFunction()
        result.variable = self.variable
        for s in self.segments:
            result.segments.append(s * value)
        return result

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
        for key in sorted(dict):
            #if abs(dict[key]) > 1E-9:
            self.segments.append(SegmentFunctionSegment(key, var, dict[key], 0))

    def addSegment(self, st,  coeff,  exp = 0.0):
        if abs(coeff) > 1E-9:
            self.segments.insert(self.index(st), SegmentFunctionSegment(st, self.variable, coeff, exp))

    def addSegments(self, dict):
        for key in sorted(dict):
            self.addSegment(key,  dict[key])

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
        return self

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

class IntervalFunction:
    "Function defined in intervals"

    def __init__(self):
        self.intervals = []
        self.values = []

    def addInterval(self,  begin,  length,  value):
        self.intervals.append((begin,  length))
        self.values.append(value)

    def value(self,  xval):
        for i in range(len(self.intervals)):
            if xval >= self.intervals[i][0] and xval < self.intervals[i][0] + self.intervals[i][1]:
                return  self.values[i]
        return self.values[len(self.values)-1]

    def lowervalue(self,  xval):
        return self.value(xval - 1E-8)

    def index(self,  xval):
        lastStart = 0.0
        for i in range(len(self.intervals)):
            newStart = self.intervals[i][0]
            if (xval >= lastStart) and (xval < newStart):
                return i-1
            lastStart = newStart
        return len(self.intervals)-1

    def interval(self, xval):
        "Return interval (begin, length) for this xval"
        return self.intervals[self.index(xval)]

    def begin(self, xval):
        return self.intervals[self.index(xval)][0]

    def length(self, xval):
        return self.intervals[self.index(xval)][1]

class StressFunction:
    "Specialization for segment-wise display of stresses"
    # The hairy thing about this is that the segments of the segfunc usually do not correspond with the intervals of the intfunc!

    def __init__(self,  f,  i):
        self.segfunc = f # The segment function for the force/moment
        self.intfunc = i # The divisors, an interval function giving a specific value for each interval
        name = "sigma"

    def isZero(self):
        return self.segfunc.isZero()

    def evaluate(self, maxX, pointsX):
        # Note: This usually creates a few more points than specified in pointsX
        offset = (maxX - self.segfunc.segments[0].start) / (pointsX - 1)
        xvals = set([self.segfunc.segments[0].start + s * offset for s in range(pointsX)])
        starts = set([self.segfunc.segments[i].start for i in range(len(self.segfunc.segments))])
        xvals = xvals.union(starts) # Make sure we have a point on each segment start
        divs = set([self.intfunc.intervals[i][0] for i in range(len(self.intfunc.intervals))])
        xvals = xvals.union(divs)

        xresult = []
        yresult = []
        for xval in sorted(xvals):
            if xval in starts:
                # create double point at segment border
                xresult.append(xval)
                yresult.append(self.segfunc.lowervalue(xval) / self.intfunc.value(xval))
            if (xval in divs):
                # create double point at divisor border
                xresult.append(xval)
                yresult.append(self.segfunc.value(xval) / self.intfunc.lowervalue(xval))
            xresult.append(xval)
            yresult.append(self.segfunc.value(xval) / self.intfunc.value(xval))
        return (xresult, yresult)

class TranslationFunction:
    "Specialization for segment-wise display of translations"

    def __init__(self,  f,  E,  d,  tangents,  translations):
        if f.isZero():
            self.transfunc = None
            return
        # Note: Integration has to be segment-wise because the area moment is not constant in different segments. But this only becomes relevant
        # when boundary conditions are being applied
        # E I_i w_i'(x) = tangfunc + C_i0
        self.tangfunc = f.integrated() # The segment function for the tangent to the bending line
        self.tangfunc.name = "w'"
        self.tangfunc.output()
        # E I_i w_i(x) = transfunc + C_i0 x + C_i1
        self.transfunc = self.tangfunc.integrated() # + C_i0 * x + C_i1 (integration constants for interval number i)
        self.transfunc.name = "w"
        self.transfunc.output()
        self.module = E
        self.intfunc = d
        self.name = "w"

        # Solve boundary conditions. There are two types:
        # External boundary conditions, e.g. a given tangent direction or translation value at a given x-value
        # Internal boundary conditions, i.e. at the segment borders the tangent direction and translation of the lines must be equal
        # Note that the relevant boundaries are those of the intfunc (where the area moment of the shaft cross-section changes)
        # Every interval of the transfunc has two integration constants C_i0 and C_i1 that need to be defined
        # Matrix of coefficients
        A = np.zeros(shape = (2 * len(self.intfunc.intervals),  2 * len(self.intfunc.intervals)))
        # Vector of RHS values
        b = np.zeros(shape = 2 * len(self.intfunc.intervals))
        # Current row where coefficients of next equation will be added
        row = 0

        # First look at external boundary conditions
        for bound in tangents:
            xval = bound[0]
            tang = bound[1]
            i = self.intfunc.index(xval) # index of this segment
            I_i = self.intfunc.value(xval) # Area moment of this segment
            # w_i'(xval) = tang    =>  (tangfunc(xval) + C_i0) / (E * I_i) = tang =>  C_i0  = tang * (E * I_i) - tangfunc(xval)
            A[row][2 * i] = 1.0
            b[row] = tang * E * I_i - self.tangfunc.value(xval)
            row += 1
        for bound in translations:
            xval = bound[0]
            trans = bound[1]
            i = self.intfunc.index(xval) # index of this segment
            I_i = self.intfunc.value(xval) # Area moment of this segment
            # w_i(xval) = trans    =>  (transfunc(xval) + C_i0 * xval + C_i1) / (E * I_i) = trans =>  xval / (E * I_i) * C_i0 + 1 / (E * I_i) * C_i1 = trans - transfunc(xval) / (E * I_i)
            A[row][2 * i] = xval / (E * I_i)
            A[row][2 * i + 1] = 1 / (E * I_i)
            b[row] = trans - self.transfunc.value(xval) / (E * I_i)
            row += 1

        # Now look at internal boundary conditions (n intervals have n-1 common segment boundaries)
        for i in range(len(self.intfunc.intervals) - 1):
            x_start = self.intfunc.intervals[i][0]
            x_end = x_start + self.intfunc.intervals[i][1]
            I_i = self.intfunc.value(x_start) # Area moment of this segment
            I_ip1 = self.intfunc.value(x_end)
            # w_i'(x_end) = w_i+1'(xend)    =>  (tangfunc(x_end) + C_i0) / (E * I_i) = (tangfunc(x_end) * C_i+1,0) / (E * I_i+1)
            #   => 1 / (E * I_i) C_i0 - 1 / (E * I_i+1) * C_i+1,0 = tangfunc(x_end) / (E * I_i+1) - tangfunc(x_end) / (E * I_i)
            A[row][2 * i] = 1 / (E * I_i)
            A[row][2 * (i+1)] = -1 / (E * I_ip1)
            b[row] = self.tangfunc.value(x_end) / (E * I_ip1) - self.tangfunc.value(x_end) / (E * I_i)
            row += 1
            # w_i(x_end) = w_i+1(xend)    =>  (transfunc(x_end) + C_i0 * x_end + C_i1) / (E * I_i) = (transfunc(x_end) * C_i+1,0) * x_end + C_i+1,1) / (E * I_i+1)
            #   => x_end / (E * I_i) C_i0 + 1 / (E * I_i) C_i1 - x_end / (E * I_i+1) * C_i+1,0 - 1 / (E * I_i+1) * C_i+1,1 = transfunc(x_end) / (E * I_i+1) - transfunc(x_end) / (E * I_i)
            A[row][2 * i] = x_end / (E * I_i)
            A[row][2 * i + 1] = 1 / (E * I_i)
            A[row][2 * (i+1)] = -x_end / (E * I_ip1)
            A[row][2 * (i+1) + 1] = -1 / (E * I_ip1)
            b[row] = self.transfunc.value(x_end) / (E * I_ip1) - self.transfunc.value(x_end) / (E * I_i)
            row += 1

        #FreeCAD.Console.PrintMessage(A)
        #FreeCAD.Console.PrintMessage(" * x = ")
        #FreeCAD.Console.PrintMessage(b)
        #FreeCAD.Console.PrintMessage("\n")

        try:
            self.boundaries = np.linalg.solve(A, b) # A * self.boundaries = b
        except np.linalg.linalg.LinAlgError as e:
            FreeCAD.Console.PrintMessage(e.message)
            FreeCAD.Console.PrintMessage(". No solution possible.\n")
            return

    def isZero(self):
        if self.transfunc is None:
            return True
        return self.transfunc.isZero()

    def evaluate(self, maxX, pointsX):
        # Note: This usually creates a few more points than specified in pointsX
        offset = (maxX - self.transfunc.segments[0].start) / (pointsX - 1)
        xvals = set([self.transfunc.segments[0].start + s * offset for s in range(pointsX)])
        starts = set([self.transfunc.segments[i].start for i in range(len(self.transfunc.segments))])
        xvals = xvals.union(starts) # Make sure we have a point on each segment start
        divs = set([self.intfunc.intervals[i][0] for i in range(len(self.intfunc.intervals))])
        xvals = xvals.union(divs)
        E = self.module

        xresult = []
        yresult = []
        # Coverity has reported a problem that I_i, C_i0 or C_i1:
        # Bad use of null-like value (CIDs are 192609, 192611, 192616)
        for xval in sorted(xvals):
            if xval in divs:
                i = self.intfunc.index(xval)
                (begin,  length) = self.intfunc.interval(xval)
                I_i = self.intfunc.value(xval)
                C_i0 = self.boundaries[2 * i]
                C_i1 = self.boundaries[2 * i + 1]
                FreeCAD.Console.PrintMessage("Interval %u: %f to %f, I_i: %f, C_i0: %f, C_i1: %f\n" % (i,  begin, length,  I_i,  C_i0,  C_i1))

            xresult.append(xval)
            # w(xval) = (transfunc(xval) + C_i0 * xval + C_i1) / (E * I_i)
            value = (self.transfunc.value(xval)  + C_i0 * xval + C_i1) / (E * I_i)
            yresult.append(value)

        return (xresult, yresult)

