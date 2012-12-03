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

import FreeCAD, FreeCADGui # FreeCAD just required for debug printing to the console...
from SegmentFunction import SegmentFunction
from ShaftFeature import ShaftFeature
from ShaftDiagram import Diagram

class ShaftSegment:
    length = 0.0
    diameter = 0.0
    innerdiameter = 0.0
    loadType = "None"
    loadSize = 0.0
    loadLocation = 0.0

    def __init__(self, l, d, di):
        self.length = l
        self.diameter = d
        self.innerdiameter = di

class Shaft:
    "The axis of the shaft is always assumed to correspond to the X-axis"
    # List of shaft segments (each segment has a different diameter)
    segments = []
    # The sketch
    sketch = 0
    #featureWindow = None
    # The diagrams
    diagrams = {} # map of function name against Diagram object
    # Calculation of shaft
    Qy = 0 # force in direction of y axis
    Qz = 0 # force in direction of z axis
    Mbz = 0 # bending moment around z axis
    Mby = 0 # bending moment around y axis
    Mtz = 0 # torsion moment around z axis

    def __init__(self, doc):
        self.sketch = ShaftFeature(doc)

    def getLengthTo(self, index):
        "Get the total length of all segments up to the given one"
        result = 0.0
        for i in range(index):
            result += self.segments[i].length
        return result

    def addSegment(self, l, d, di):
        #print "Adding segment: ", l, " : ", d
        self.segments.append(ShaftSegment(l,d,di))
        self.sketch.addSegment(l, d, di)
        # We don't call equilibrium() here because the new segment has no loads defined yet

    def updateSegment(self, index, length = None, diameter = None, innerdiameter = None):
        oldLength = self.segments[index].length
        #print "Old length of ", index, ": ", oldLength, ", new Length: ", length, " diameter: ", diameter
        if length is not None:
            self.segments[index].length = length
        if diameter is not None:
            self.segments[index].diameter = diameter
        if innerdiameter is not None:
            self.segments[index].innerdiameter = innerdiameter
        self.sketch.updateSegment(index, oldLength, self.segments[index].length,
                                  self.segments[index].diameter, self.segments[index].innerdiameter)
        self.equilibrium()
        self.updateDiagrams()

    def updateLoad(self, index, loadType = None, loadSize = None, loadLocation = None):
        if (loadType is not None):
            self.segments[index].loadType = loadType
        if (loadSize is not None):
            self.segments[index].loadSize = loadSize
        if (loadLocation is not None):
            if (loadLocation >= 0) and (loadLocation <= self.segments[index].length):
                self.segments[index].loadLocation = loadLocation
            else:
                # TODO: Show warning
                FreeCAD.Console.PrintMessage("Load location must be inside segment\n")

        #self.feature.updateForces() graphical representation of the forces
        self.equilibrium()
        self.updateDiagrams()

    def updateEdge(self, column, start):
        App.Console.PrintMessage("Not implemented yet - waiting for robust references...")
        return
        if self.sketchClosed is not True:
            return
        # Create a chamfer or fillet at the start or end edge of the segment
        if start is True:
            row = rowStartEdgeType
            idx = 0
        else:
            row = rowEndEdgeType
            idx = 1

        edgeType = self.tableWidget.item(row, column).text().toAscii()[0].upper()
        if not ((edgeType == "C") or (edgeType == "F")):
            return # neither chamfer nor fillet defined

        if edgeType == "C":
            objName = self.doc.addObject("PartDesign::Chamfer","ChamferShaft%u" % (column * 2 + idx))
        else:
            objName = self.doc.addObject("PartDesign::Fillet","FilletShaft%u" % (column * 2 + idx))
        if objName == "":
            return

        edgeName = "Edge%u" % self.getEdgeIndex(column, idx, edgeType)
        self.doc.getObject(objName).Base = (self.doc.getObject("RevolutionShaft"),"[%s]" % edgeName)
        # etc. etc.

    def getEdgeIndex(self, column, startIdx):
        # FIXME: This is impossible without robust references anchored in the sketch!!!
        return

    def updateDiagrams(self):
        if (self.Qy == 0) or (self.Mbz == 0):
            return
        if self.Qy.name in self.diagrams:
            # Update diagram
            self.diagrams[self.Qy.name].update(self.Qy, self.getLengthTo(len(self.segments)) / 1000.0)
        else:
            # Create diagram
            self.diagrams[self.Qy.name] = Diagram()
            self.diagrams[self.Qy.name].create("Shear force", self.Qy, self.getLengthTo(len(self.segments)) / 1000.0, "x", "mm", 1000.0, "Q_y", "N", 1.0, 10)
        if self.Mbz.name in self.diagrams:
            # Update diagram
            self.diagrams[self.Mbz.name].update(self.Mbz, self.getLengthTo(len(self.segments)) / 1000.0)
        else:
            # Create diagram
            self.diagrams[self.Mbz.name] = Diagram()
            self.diagrams[self.Mbz.name].create("Bending moment", self.Mbz, self.getLengthTo(len(self.segments)) / 1000.0, "x", "mm", 1000.0, "M_{b,z}", "Nm", 1.0, 10)

    def equilibrium(self):
        # Build equilibrium equations
        forces = {0.0:0.0} # dictionary of (location : outer force)
        moments = {0.0:0.0} # dictionary of (location : outer moment)
        variableNames = [""] # names of all variables
        locations = {} # dictionary of (variableName : location)
        coefficientsFy = [0] # force equilibrium equation
        coefficientsMbz = [0] # moment equilibrium equation

        for i in range(len(self.segments)):
            lType = self.segments[i].loadType
            load = -1 # -1 means unknown (just for debug printing)
            location = -1

            if lType == "Fixed":
                # Fixed segment
                if i == 0:
                    location = 0
                    variableNames.append("Fy%u" % i)
                    coefficientsFy.append(1)
                    coefficientsMbz.append(0)
                    variableNames.append("Mz%u" % i)
                    coefficientsFy.append(0)
                    coefficientsMbz.append(1) # Force does not contribute because location is zero
                elif i == len(self.segments) - 1:
                    location = self.getLengthTo(len(self.segments)) / 1000
                    variableNames.append("Fy%u" % i)
                    coefficientsFy.append(1)
                    coefficientsMbz.append(location)
                    variableNames.append("Mz%u" % i)
                    coefficientsFy.append(0)
                    coefficientsMbz.append(1)
                else:
                    # TODO: Better error message
                    FreeCAD.Console.PrintMessage("Fixed constraint must be at beginning or end of shaft\n")
                    return

                locations["Fy%u" % i] = location
                locations["Mz%u" % i] = location
            elif lType == "Static":
                # Static load (currently force only)
                load = self.segments[i].loadSize
                location = (self.getLengthTo(i) + self.segments[i].loadLocation)  / 1000 # convert to meters
                coefficientsFy[0] = coefficientsFy[0] - load
                forces[location] = load
                coefficientsMbz[0] = coefficientsMbz[0] - load * location
                moments[location] = 0
            #elif lType == "None":
            #    # No loads on segment

            FreeCAD.Console.PrintMessage("Segment: %u, type: %s, load: %f, location: %f\n" % (i, lType, load, location))

        self.printEquilibrium(variableNames, coefficientsFy)
        self.printEquilibrium(variableNames, coefficientsMbz)

        # Build matrix and vector for linear algebra solving algorithm
        try:
            import numpy as np
        except ImportError:
            FreeCAD.Console.PrintMessage("numpy is not installed on your system\n")
            raise ImportError("numpy not installed")
        if (len(coefficientsFy) < 3) or (len(coefficientsMbz) < 3):
            return
        A = np.array([coefficientsFy[1:], coefficientsMbz[1:]])
        b = np.array([coefficientsFy[0], coefficientsMbz[0]])
        solution = np.linalg.solve(A, b)

        # Complete dictionary of forces and moments
        if variableNames[1][0] == "F":
            forces[locations[variableNames[1]]] = solution[0]
        else:
            moments[locations[variableNames[1]]] = solution[0]

        if variableNames[2][0] == "F":
            forces[locations[variableNames[2]]] = solution[1]
        else:
            moments[locations[variableNames[2]]] = solution[1]

        FreeCAD.Console.PrintMessage(forces)
        FreeCAD.Console.PrintMessage(moments)
        self.Qy = SegmentFunction("Qy")
        self.Qy.buildFromDict("x", forces)
        self.Qy.output()
        self.Mbz = self.Qy.integrated().negate()
        self.Mbz.addSegments(moments) # takes care of boundary conditions
        self.Mbz.name = "Mbz"
        self.Mbz.output()

    def printEquilibrium(self, var, coeff):
        # Auxiliary method for debugging purposes
        for i in range(len(var)):
            if i == 0:
                FreeCAD.Console.PrintMessage("%f = " % coeff[i])
            else:
                FreeCAD.Console.PrintMessage("%f * %s" % (coeff[i], var[i]))
            if (i < len(var) - 1) and (i != 0):
                FreeCAD.Console.PrintMessage(" + ")
        FreeCAD.Console.PrintMessage("\n")
