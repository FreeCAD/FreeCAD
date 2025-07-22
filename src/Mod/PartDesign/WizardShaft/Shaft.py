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

import FreeCAD, FreeCADGui
from .SegmentFunction import SegmentFunction,  IntervalFunction,  StressFunction,  TranslationFunction
from .ShaftFeature import ShaftFeature
from .ShaftDiagram import Diagram
import math

class ShaftSegment:
    def __init__(self, l, d, di):
        self.length = l
        self.diameter = d
        self.innerdiameter = di
        self.constraintType = "None"
        self.constraint = None

class Shaft:
    "The axis of the shaft is always assumed to correspond to the X-axis"
    # Names (note Qy corresponds with Mz, and Qz with My)
    Fstr = ["Nx","Qy","Qz"] # Forces
    Mstr = ["Mx","Mz","My"] # Moments
    wstr = ["",  "wy",  "wz"] # Translations
    sigmaNstr = ["sigmax","sigmay","sigmaz"] # Normal/shear stresses
    sigmaBstr = ["taut","sigmabz", "sigmaby"] # Torsion/bending stresses
    # For diagram labeling
    Qstrings = (("Normal force [x]",  "x", "mm", "N_x",  "N"),
                        ("Shear force [y]",  "x",  "mm",  "Q_y",  "N"),
                        ("Shear force [z]",  "x",  "mm",  "Q_z",  "N"))
    Mstrings = (("Torque [x]",  "x", "mm", "M_t",  "Nm"),
                        ("Bending moment [z]",  "x",  "mm",  "M_{b,z}",  "Nm"),
                        ("Bending moment [y]",  "x",  "mm",  "M_{b,y}",  "Nm"))
    wstrings = (("",  "",  "",  "",  ""),
                        ("Translation [y]",  "x",  "mm",  "w_y",  "mm"),
                        ("Translation [z]",  "x",  "mm",  "w_z",  "mm"))
    sigmaNstrings =  (("Normal stress [x]",  "x", "mm", "\\sigma_x",  u"N/mm²"),
                        ("Shear stress [y]",  "x",  "mm",  "\\sigma_y",  u"N/mm²"),
                        ("Shear stress [z]",  "x",  "mm",  "\\sigma_z",  u"N/mm²"))
    sigmaBstrings = (("Torque stress [x]",  "x", "mm", "\\tau_t",  u"N/mm²"),
                        ("Bending stress [z]",  "x",  "mm",  "\\sigma_{b,z}",  u"N/mm²"),
                        ("Bending stress [y]",  "x",  "mm",  "\\sigma_{b,y}",  u"N/mm²"))

    def __init__(self, parent):
        self.parent = parent
        self.doc = parent.doc
        self.feature = ShaftFeature(self.doc)
        # List of shaft segments (each segment has a different diameter)
        self.segments = []
        # The diagrams
        self.diagrams = {} # map of function name against Diagram object
        # Calculation of shaft
        self.F = [None,  None,  None] # force in direction of [x,y,z]-axis
        self.M = [None,  None,  None] # bending moment around [x,z,y]-axis
        self.w = [None,  None,  None] # Shaft translation due to bending
        self.sigmaN = [None,  None,  None]  # normal stress in direction of x-axis, shear stress in direction of [y,z]-axis
        self.sigmaB = [None,  None,  None]  # # torque stress around x-axis, maximum bending stress in direction of [y,z]-axis

    def getLengthTo(self, index):
        "Get the total length of all segments up to the given one"
        result = 0.0
        for i in range(index):
            result += self.segments[i].length
        return result

    def addSegment(self, l, d, di):
        self.segments.append(ShaftSegment(l,d,di))
        self.feature.addSegment(l, d, di)
        # We don't call equilibrium() here because the new segment has no constraints defined yet
        # Fix face reference of fixed segment if it is the last one
        for i in range(1,  len(self.segments)):
            if self.segments[i].constraintType != "Fixed":
                continue
            if i == len(self.segments) - 1:
                self.segments[index].constraint.References = [( self.feature.feature,  "Face%u" % (2 * (index+1) + 1) )]
            else:
                # Remove reference since it is now in the middle of the shaft (which is not allowed)
                self.segments[index].constraint.References = [(None,  "")]

    def updateSegment(self, index, length = None, diameter = None, innerdiameter = None):
        oldLength = self.segments[index].length

        if length is not None:
            self.segments[index].length = length
        if diameter is not None:
            self.segments[index].diameter = diameter
        if innerdiameter is not None:
            self.segments[index].innerdiameter = innerdiameter

        self.feature.updateSegment(index, oldLength, self.segments[index].length, self.segments[index].diameter, self.segments[index].innerdiameter)
        self.equilibrium()
        self.updateDiagrams()

    def updateConstraint(self, index, constraintType):
        if (constraintType is not None):
            # Did the constraint type change?
            if (self.segments[index].constraintType != "None") and (self.segments[index].constraintType != constraintType):
                self.doc.removeObject(self.segments[index].constraint.Name)
                self.segments[index].constraint = None

            self.segments[index].constraintType = constraintType

            # Create constraint if it does not exist yet or has changed
            if self.segments[index].constraint is None:
                if (constraintType == "Force"):
                    # TODO: Create a reference point and put the force onto it
                    constraint = self.doc.addObject("Fem::ConstraintForce","ShaftConstraintForce")
                    constraint.Force = 1000.0
                    self.segments[index].constraint = constraint
                elif (constraintType == "Fixed"):
                    # TODO: Use robust reference as soon as it is available for the face
                    constraint = self.doc.addObject("Fem::ConstraintFixed","ShaftConstraintFixed")
                    if index == 0:
                        constraint.References = [( self.feature.feature,  "Face1")]
                    elif index == len(self.segments) - 1:
                        constraint.References = [( self.feature.feature,  "Face%u" % (2 * (index+1) + 1) )]
                    self.segments[index].constraint = constraint
                elif (constraintType == "Bearing"):
                    # TODO: Use robust reference as soon as it is available for the cylindrical face reference
                    constraint = self.doc.addObject("Fem::ConstraintBearing","ShaftConstraintBearing")
                    constraint.References = [( self.feature.feature,  "Face%u" % (2 * (index+1)) )]
                    constraint.AxialFree = True
                    self.segments[index].constraint = constraint
                elif (constraintType == "Pulley"):
                    constraint= self.doc.addObject("Fem::ConstraintPulley","ShaftConstraintPulley")
                    constraint.References = [( self.feature.feature,  "Face%u" % (2 * (index+1)) )]
                    self.segments[index].constraint = constraint
                elif (constraintType == "Gear"):
                    constraint = self.doc.addObject("Fem::ConstraintGear","ShaftConstraintGear")
                    constraint.References = [( self.feature.feature,  "Face%u" % (2 * (index+1)) )]
                    self.segments[index].constraint = constraint

        self.equilibrium()
        self.updateDiagrams()

    def editConstraint(self,  index):
        if (self.segments[index].constraint is not None):
            FreeCADGui.activeDocument().setEdit(self.segments[index].constraint.Name)

    def getConstraint(self,  index):
        return self.segments[index].constraint

    def updateEdge(self, column, start):
        App.Console.PrintMessage("Not implemented yet - waiting for robust references...")
        return
        """
        if self.sketchClosed is not True:
            return
        # Create a chamfer or fillet at the start or end edge of the segment
        if start is True:
            row = rowStartEdgeType
            idx = 0
        else:
            row = rowEndEdgeType
            idx = 1

        edgeType = self.tableWidget.item(row, column).text()[0].upper()
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
        """

    def getEdgeIndex(self, column, startIdx):
        # FIXME: This is impossible without robust references anchored in the sketch!!!
        return

    def updateDiagrams(self):
        for ax in range(3):
            if self.F[ax] is not None:
                if self.F[ax].name in self.diagrams:
                    self.diagrams[self.F[ax].name].update(self.F[ax], self.getLengthTo(len(self.segments)) / 1000.0)
            if self.M[ax] is not None:
                if self.M[ax].name in self.diagrams:
                    self.diagrams[self.M[ax].name].update(self.M[ax], self.getLengthTo(len(self.segments)) / 1000.0)
            if self.w[ax] is not None:
                if self.w[ax].name in self.diagrams:
                    self.diagrams[self.w[ax].name].update(self.w[ax], self.getLengthTo(len(self.segments)) / 1000.0)
            if self.sigmaN[ax] is not None:
                if self.sigmaN[ax].name in self.diagrams:
                    self.diagrams[self.sigmaN[ax].name].update(self.sigmaN[ax], self.getLengthTo(len(self.segments)) / 1000.0)
            if self.sigmaB[ax] is not None:
                if self.sigmaB[ax].name in self.diagrams:
                    self.diagrams[self.sigmaB[ax].name].update(self.sigmaB[ax], self.getLengthTo(len(self.segments)) / 1000.0)

    def showDiagram(self,  which):
        if which in self.Fstr:
            ax = self.Fstr.index(which)
            text = self.Qstrings[ax]
            if self.F[ax] is None:
                # No data
                return
            if self.F[ax].name in self.diagrams:
                # Diagram is already open, close it again
                self.diagrams[self.F[ax].name].close()
                del (self.diagrams[self.F[ax].name])
                return
            self.diagrams[self.F[ax].name] = Diagram()
            self.diagrams[self.F[ax].name].create(text[0], self.F[ax], self.getLengthTo(len(self.segments)) / 1000.0, text[1], text[2], 1000.0, text[3], text[4], 1.0, 10)
        elif which in self.Mstr:
            ax = self.Mstr.index(which)
            text = self.Mstrings[ax]
            if self.M[ax] is None:
                # No data
                return
            if self.M[ax].name in self.diagrams:
                # Diagram is already open, close it again
                self.diagrams[self.M[ax].name].close()
                del (self.diagrams[self.M[ax].name])
                return
            self.diagrams[self.M[ax].name] = Diagram()
            self.diagrams[self.M[ax].name].create(text[0], self.M[ax], self.getLengthTo(len(self.segments)) / 1000.0, text[1], text[2], 1000.0, text[3], text[4], 1.0, 20)
        elif which in self.wstr:
            ax = self.wstr.index(which)
            text = self.wstrings[ax]
            if self.w[ax] is None:
                # No data
                return
            if self.w[ax].name in self.diagrams:
                # Diagram is already open, close it again
                self.diagrams[self.w[ax].name].close()
                del (self.diagrams[self.w[ax].name])
                return
            self.diagrams[self.w[ax].name] = Diagram()
            self.diagrams[self.w[ax].name].create(text[0], self.w[ax], self.getLengthTo(len(self.segments)) / 1000.0, text[1], text[2], 1000.0, text[3], text[4], 1000.0, 30)
        elif which in self.sigmaNstr:
            ax = self.sigmaNstr.index(which)
            text = self.sigmaNstrings[ax]
            if self.sigmaN[ax] is None:
                # No data
                return
            if self.sigmaN[ax].name in self.diagrams:
                # Diagram is already open, close it again
                self.diagrams[self.sigmaN[ax].name].close()
                del (self.diagrams[self.sigmaN[ax].name])
                return
            self.diagrams[self.sigmaN[ax].name] = Diagram()
            self.diagrams[self.sigmaN[ax].name].create(text[0], self.sigmaN[ax], self.getLengthTo(len(self.segments)) / 1000.0, text[1], text[2], 1000.0, text[3], text[4], 1.0E-6, 10)
        elif which in self.sigmaBstr:
            ax = self.sigmaBstr.index(which)
            text = self.sigmaBstrings[ax]
            if self.sigmaB[ax] is None:
                # No data
                return
            if self.sigmaB[ax].name in self.diagrams:
                # Diagram is already open, close it again
                self.diagrams[self.sigmaB[ax].name].close()
                del (self.diagrams[self.sigmaB[ax].name])
                return
            self.diagrams[self.sigmaB[ax].name] = Diagram()
            self.diagrams[self.sigmaB[ax].name].create(text[0], self.sigmaB[ax], self.getLengthTo(len(self.segments)) / 1000.0, text[1], text[2], 1000.0, text[3], text[4], 1.0E-6, 20)

    def addTo(self,  dict,  location,  value):
        if location not in dict:
            dict[location] = value
        else:
            dict[location] += value

    def equilibrium(self):
        # Build equilibrium equations
        try:
            import numpy as np
        except ImportError:
            FreeCAD.Console.PrintMessage("numpy is not installed on your system\n")
            raise ImportError("numpy not installed")

        # Initialization of structures. All three axes are handled separately so everything is 3-fold
        # dictionaries of (location : outer force/moment) with reverse sign, which means that the segment functions for the section force and section moment
        # created from them will have signs as by the convention in
        # http://www.umwelt-campus.de/ucb/fileadmin/users/90_t.preussler/dokumente/Skripte/TEMECH/TMI/Ebene_Balkenstatik.pdf (page 10)
        # (see also example on page 19)
        forces = [{0.0:0.0},  {0.0:0.0},  {0.0:0.0}]
        moments = [{0.0:0.0},  {0.0:0.0},  {0.0:0.0}]
        # Boundary conditions for shaft bending line
        tangents = [[],  [],  []] # Tangents to shaft bending line
        translations = [[],  [],  []] # Shaft displacement
        # Variable names, e.g. Fx, Mz. Because the system must be exactly determined, not more than two independent variables for each
        # force/moment per axis are possible (if there are more no solution is calculated)
        variableNames = [[""],  [""], [""]]
        # # dictionary of (variableName : location) giving the x-coordinate at which the force/moment represented by the variable acts on the shaft
        locations = {}
        # Coefficients of the equilibrium equations in the form a = b * F1 + c * F2 and d = e * M1 + f * M2
        # LHS (variables a1, a2, a3, d3) initialized to zero
        coefficientsF = [[0], [0], [0]]
        coefficientsM = [[0], [0], [0]]

        for i in range(len(self.segments)):
            cType = self.segments[i].constraintType
            constraint = self.segments[i].constraint

            if cType == "Fixed":
                # Fixed segment
                if i == 0:
                    # At beginning of shaft
                    location = 0
                elif i == len(self.segments) - 1:
                    # At end of shaft
                    location = self.getLengthTo(len(self.segments)) / 1000.0 # convert to meters
                else:
                    # TODO: Better error message
                    FreeCAD.Console.PrintMessage("Fixed constraint must be at beginning or end of shaft\n")
                    return

                for ax in range(3):
                    # Create a new reaction force
                    variableNames[ax].append("%s%u" % (self.Fstr[ax], i))
                    coefficientsF[ax].append(1)
                    # Register location of reaction force
                    locations["%s%u" % (self.Fstr[ax], i)] = location
                    # Boundary conditions for the translations
                    tangents[ax].append((location,  0.0))
                    translations[ax].append((location,  0.0))
                coefficientsM[0].append(0) # Reaction force contributes no moment around x axis
                coefficientsM[1].append(location) # Reaction force contributes a positive moment around z axis
                coefficientsM[2].append(-location) # Reaction force contributes a negative moment around y axis

                for ax in range(3):
                    # Create a new reaction moment
                    variableNames[ax].append("%s%u" % (self.Mstr[ax], i))
                    coefficientsF[ax].append(0)
                    coefficientsM[ax].append(1)
                    locations["%s%u" % (self.Mstr[ax], i)] = location

            elif cType == "Force":
                # Static force (currently force on midpoint of segment only)
                force = constraint.DirectionVector.multiply(constraint.Force)
                # TODO: Extract value of the location from geometry
                location = (self.getLengthTo(i) + self.segments[i].length/2.0)  / 1000.0
                # The force itself
                for ax in range(3):
                    if abs(force[ax]) > 0.0:
                        coefficientsF[ax][0] = coefficientsF[ax][0] - force[ax] # neg. because this coefficient is on the LHS of the equilibrium equation
                        self.addTo(forces[ax],  location,  -force[ax]) # neg. to fulfill the convention mentioned above
                # Moments created by the force (by definition no moment is created by the force in x-direction)
                if abs(force[1]) > 0.0:
                    coefficientsM[1][0] = coefficientsM[1][0] - force[1] * location # moment around z-axis
                    self.addTo(moments[1],  location,  0)
                if abs(force[2]) > 0.0:
                    coefficientsM[2][0] = coefficientsM[2][0] + force[2] * location # moment around y-axis
                    self.addTo(moments[2],  location,  0) # No outer moment acts here!

            elif cType == "Bearing":
                 location = constraint.BasePoint.x / 1000.0 # TODO: This assumes that the shaft feature starts with the first segment at (0,0,0)  and its axis corresponds to the x-axis
                 # Bearing reaction forces. TODO: the bearing is assumed to not induce any reaction moments
                 start = (0 if constraint.AxialFree == False else 1)
                 for ax in range(start, 3):
                    variableNames[ax].append("%s%u" % (self.Fstr[ax], i))
                    coefficientsF[ax].append(1)
                    locations["%s%u" % (self.Fstr[ax], i)] = location
                    # Boundary condition
                    translations[ax].append((location,  0.0))
                 if constraint.AxialFree == False:
                    coefficientsM[0].append(0)             # Reaction force contributes no moment around x axis
                 coefficientsM[1].append(location) # Reaction force contributes a positive moment around z axis
                 coefficientsM[2].append(-location) # Reaction force contributes a negative moment around y axis

            elif cType == "Gear":
                force = constraint.DirectionVector.multiply(constraint.Force)
                location = constraint.BasePoint.x / 1000.0
                lever = [0,  constraint.Diameter/2.0/1000.0 * math.sin(constraint.ForceAngle / 180.0 * math.pi),
                                    constraint.Diameter/2.0 /1000.0* math.cos(constraint.ForceAngle / 180.0 * math.pi)]

                # Effect of the gear force
                for ax in range(3):
                    if abs(force[ax]) > 0.0:
                        # Effect of the force
                        coefficientsF[ax][0] = coefficientsF[ax][0] - force[ax]
                        self.addTo(forces[ax],  location,  -force[ax])
                # Moments created by the force (by definition no moment is created by the force in x-direction)
                if abs(force[1]) > 0.0:
                    coefficientsM[1][0] = coefficientsM[1][0] - force[1] * location # moment around z-axis
                    self.addTo(moments[1],  location,  0)
                if abs(force[2]) > 0.0:
                    coefficientsM[2][0] = coefficientsM[2][0] + force[2] * location # moment around y-axis
                    self.addTo(moments[2],  location,  0) # No outer moment acts here!

                # Moments created by the force and lever
                if abs(force[0]) > 0.0:
                    momenty = force[0] * lever[2]
                    momentz = force[0] * lever[1]
                    coefficientsM[1][0] = coefficientsM[1][0] + momentz # moment around z-axis
                    self.addTo(moments[1],  location,  momentz)
                    coefficientsM[2][0] = coefficientsM[2][0] - momenty # moment around y-axis
                    self.addTo(moments[2],  location,  -momenty)
                if abs(force[1]) > 0.0:
                    moment = force[1] * lever[2]
                    coefficientsM[0][0] = coefficientsM[0][0] + moment
                    self.addTo(moments[0],  location,  moment)
                if abs(force[2]) > 0.0:
                    moment = force[2] * lever[1]
                    coefficientsM[0][0] = coefficientsM[0][0] - moment
                    self.addTo(moments[0],  location,  -moment)
            elif cType == "Pulley":
                forceAngle1 = (constraint.ForceAngle + constraint.BeltAngle + 90.0) / 180.0 * math.pi
                forceAngle2 = (constraint.ForceAngle - constraint.BeltAngle + 90.0) / 180.0 * math.pi
                #FreeCAD.Console.PrintMessage("BeltForce1: %f, BeltForce2: %f\n" % (constraint.BeltForce1,  constraint.BeltForce2))
                #FreeCAD.Console.PrintMessage("Angle1: %f, Angle2: %f\n" % (forceAngle1,  forceAngle2))
                force = [0,  -constraint.BeltForce1 * math.sin(forceAngle1) - constraint.BeltForce2 * math.sin(forceAngle2),
                                   constraint.BeltForce1 * math.cos(forceAngle1) + constraint.BeltForce2 * math.cos(forceAngle2)]
                location = constraint.BasePoint.x / 1000.0

                # Effect of the pulley forces
                for ax in range(3):
                    if abs(force[ax]) > 0.0:
                        # Effect of the force
                        coefficientsF[ax][0] = coefficientsF[ax][0] - force[ax]
                        self.addTo(forces[ax],  location,  -force[ax])
                # Moments created by the force (by definition no moment is created by the force in x-direction)
                if abs(force[1] ) > 0.0:
                    coefficientsM[1][0] = coefficientsM[1][0] - force[1] * location # moment around z-axis
                    self.addTo(moments[1],  location,  0)
                if abs(force[2]) > 0.0:
                    coefficientsM[2][0] = coefficientsM[2][0] + force[2] * location # moment around y-axis
                    self.addTo(moments[2],  location,  0) # No outer moment acts here!

                # Torque
                moment = constraint.Force * (1 if constraint.IsDriven is True else -1)
                coefficientsM[0][0] = coefficientsM[0][0] + moment
                self.addTo(moments[0],  location,  moment)

        areas = [None,  None,  None]
        areamoments = [None,  None,  None]
        bendingmoments = [None,  None,  None]
        torquemoments = [None,  None,  None]

        for ax in range(3):
            FreeCAD.Console.PrintMessage("Axis: %u\n" %  ax)
            self.printEquilibrium(variableNames[ax], coefficientsF[ax])
            self.printEquilibrium(variableNames[ax], coefficientsM[ax])

            if  len(coefficientsF[ax]) <= 1:
                # Note: coefficientsF and coefficientsM always have the same length
                FreeCAD.Console.PrintMessage("Matrix is singular, no solution possible\n")
                self.parent.updateButtons(ax,  False)
                continue

            # Handle special cases. Note that the code above should ensure that coefficientsF and coefficientsM always have same length
            solution = [None,  None]
            if len(coefficientsF[ax]) == 2:
                if coefficientsF[ax][1] != 0.0 and coefficientsF[ax][0] != 0.0:
                    solution[0] = coefficientsF[ax][0] / coefficientsF[ax][1]
                if coefficientsM[ax][1] != 0.0 and coefficientsM[ax][0] != 0.0:
                    solution[1] = coefficientsM[ax][0] / coefficientsM[ax][1]
                    if abs(solution[0] - solution[1]) < 1E9:
                        FreeCAD.Console.PrintMessage("System is statically undetermined. No solution possible.\n")
                        self.parent.updateButtons(ax,  False)
                        continue
            else:
                # Build matrix and vector for linear algebra solving algorithm
               # TODO: This could easily be done manually... there are only 2 variables and 6 coefficients
                A = np.array([coefficientsF[ax][1:], coefficientsM[ax][1:]])
                b = np.array([coefficientsF[ax][0], coefficientsM[ax][0]])
                try:
                    solution = np.linalg.solve(A, b) # A * solution = b
                except np.linalg.linalg.LinAlgError as e:
                    FreeCAD.Console.PrintMessage(str(e))
                    FreeCAD.Console.PrintMessage(". No solution possible.\n")
                    self.parent.updateButtons(ax,  False)
                    continue

            # Complete dictionary of forces and moments with the two reaction forces that were calculated
            for i in range(2):
                if solution[i] is None:
                    continue
                FreeCAD.Console.PrintMessage("Reaction force/moment: %s = %f\n" % (variableNames[ax][i+1],  solution[i]))
                if variableNames[ax][i+1][0] == "M":
                    moments[ax][locations[variableNames[ax][i+1]]] = -solution[i]
                else:
                    forces[ax][locations[variableNames[ax][i+1]]] = -solution[i]

            FreeCAD.Console.PrintMessage(forces[ax])
            FreeCAD.Console.PrintMessage("\n")
            FreeCAD.Console.PrintMessage(moments[ax])
            FreeCAD.Console.PrintMessage("\n")

            # Forces
            self.F[ax] = SegmentFunction(self.Fstr[ax])
            self.F[ax].buildFromDict("x", forces[ax])
            self.parent.updateButton(1,  ax,  not self.F[ax].isZero())
            self.F[ax].output()
            # Moments
            if ax == 0:
                self.M[0] = SegmentFunction(self.Mstr[0])
                self.M[0].buildFromDict("x",  moments[0])
            elif ax == 1:
                self.M[1] = self.F[1].integrated().negate()
                self.M[1].name = self.Mstr[1]
                self.M[1].addSegments(moments[1]) # takes care of boundary conditions
            elif ax == 2:
                self.M[2] = self.F[2].integrated()
                self.M[2].name = self.Mstr[2]
                self.M[2].addSegments(moments[2]) # takes care of boundary conditions
            self.parent.updateButton(2,  ax,  not self.M[ax].isZero())
            self.M[ax].output()

            # Areas and area moments
            location = 0.0
            areas[ax] = IntervalFunction() # A [m²]
            areamoments[ax] = IntervalFunction() # I [m⁴]
            bendingmoments[ax] = IntervalFunction() # W_b [m³]
            torquemoments[ax] = IntervalFunction() # W_t [m³]

            for i in range(len(self.segments)):
                od = self.segments[i].diameter/1000.0
                id = self.segments[i].innerdiameter/1000.0
                length = self.segments[i].length/1000.0
                areas[ax].addInterval(location, length,  math.pi/4.0 * (math.pow(od,  2.0) - math.pow(id,  2.0)))
                areamoment = math.pi/64.0 * (math.pow(od,  4.0) - math.pow(id,  4.0))
                areamoments[ax].addInterval(location,  length,  areamoment)
                bendingmoments[ax].addInterval(location,  length,  areamoment / (od / 2.0))
                torquemoments[ax].addInterval(location,  length,  2 * (areamoment / (od / 2.0)))
                location += length

            # Bending line
            if ax > 0:
                if len(tangents[ax])+ len(translations[ax]) == 2:
                    # TODO: Get Young's module from material type instead of using 210000 N/mm² = 2.1E12 N/m²
                    self.w[ax] = TranslationFunction(self.M[ax].negated(),  2.1E12,  areamoments[ax], tangents[ax], translations[ax])
                    self.w[ax].name= self.wstr[ax]
                    self.parent.updateButton(3,  ax,  not self.w[ax].isZero())
                else:
                    self.parent.updateButton(3,  ax,  False)

            # Normal/shear stresses and torque/bending stresses
            self.sigmaN[ax] = StressFunction(self.F[ax],  areas[ax])
            self.sigmaN[ax].name = self.sigmaNstr[ax]
            self.parent.updateButton(4,  ax,  not self.sigmaN[ax].isZero())
            if ax == 0:
                self.sigmaB[ax] = StressFunction(self.M[ax] ,  torquemoments[ax])
            else:
                self.sigmaB[ax] = StressFunction(self.M[ax],  bendingmoments[ax])
            self.sigmaB[ax].name = self.sigmaBstr[ax]
            self.parent.updateButton(5,  ax,  not self.sigmaB[ax].isZero())

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
