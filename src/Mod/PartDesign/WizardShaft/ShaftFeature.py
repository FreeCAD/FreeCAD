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
import Part, Sketcher

class ShaftFeature:
    "Creates and updates the feature of the shaft"
    App = FreeCAD
    Gui = FreeCADGui

    def __init__(self, doc):
        "Create new feature"
        self.Doc = doc

        # TODO: Discover existing sketch and get data from it
        self.sketch = self.Doc.addObject("Sketcher::SketchObject","SketchShaft")
        self.sketch.Placement = self.App.Placement(self.App.Vector(0,0,0),self.App.Rotation(0,0,0,1))

        self.feature = 0
        self.segments = 0 # number of segments
        self.totalLength = 0 # total length of all segments
        self.lastRadius = 0 # radius of last segment (required for adding segments)

    def addSegment(self, length, diameter, innerdiameter):
        "Add a segment at the end of the shaft"
        # Find constraint indices of vertical line constraint, horizontal line constraint
        # FIXME: Should have a unique id instead of indices that might change with user editing
        # 0-3 belong to the centerline
        # 4-6 to the first vertical section
        # 7-9 to the first horizontal section
        # 10-12 to the second vertical section
        # etc. etc.
        constrRadius = 4 + self.segments * 6
        constrLength = 7 + self.segments * 6
        constrInnerRadius = 1 + self.segments * 6
        # Find line index of vertical segment, horizontal segment, last shaft segment
        # FIXME: Should have a unique id instead of indices that might change with user editing
        segRadius = 1 + self.segments * 2
        segLength = 2 + self.segments * 2
        prevSegLength = 0 + self.segments * 2
        prevSegEnd = 3 + (self.segments - 1) * 2
        segEnd = prevSegEnd + 2

        radius = diameter / 2
        innerradius = innerdiameter / 2
        oldLength = self.totalLength
        self.totalLength += length
        self.segments += 1

        if oldLength == 0:
            # First segment of shaft
            # Create centerline
            self.sketch.addGeometry(Part.LineSegment(self.App.Vector(0,0,0), self.App.Vector(self.totalLength,0,0)))
            self.sketch.addConstraint(Sketcher.Constraint('DistanceX',0, self.totalLength)) # Constraint1
            self.sketch.addConstraint(Sketcher.Constraint('DistanceY', -1,1,0,1,innerradius)) # Constraint2
            self.sketch.addConstraint(Sketcher.Constraint('PointOnObject',0,1,-2)) # Constraint3
            self.sketch.addConstraint(Sketcher.Constraint('Horizontal', 0)) # Constraint4
            # Create first segment
            self.sketch.addGeometry(Part.LineSegment(self.App.Vector(0,innerradius,0), self.App.Vector(0,radius,0)))
            self.sketch.addConstraint(Sketcher.Constraint('DistanceY',-1,1,1,2,radius)) # Constraint5
            self.sketch.addConstraint(Sketcher.Constraint('Coincident',0,1,1,1)) # Constraint6
            self.sketch.addConstraint(Sketcher.Constraint('Vertical',1)) # Constraint7
            self.sketch.addGeometry(Part.LineSegment(self.App.Vector(0,radius,0), self.App.Vector(length,radius,0)))
            self.sketch.addConstraint(Sketcher.Constraint('DistanceX',2,length)) # Constraint8
            self.sketch.addConstraint(Sketcher.Constraint('Coincident',2,1,1,2)) # Constraint9
            self.sketch.addConstraint(Sketcher.Constraint('Horizontal',2)) # Constraint10
        else:
            # remove line that closes the shaft
            self.sketch.delGeometry(prevSegEnd)
            # TODO: Delete the two constraints? Or will they disappear automatically?
            # Adjust length of centerline
            self.sketch.setDatum(0,self.totalLength)
            # Add segment at the end
            self.sketch.addGeometry(Part.LineSegment(self.App.Vector(oldLength,self.lastRadius,0), self.App.Vector(oldLength,radius,0)))
            self.sketch.addConstraint(Sketcher.Constraint('DistanceY', -1,1, segRadius, 2, radius))
            self.sketch.addConstraint(Sketcher.Constraint('Coincident',segRadius,1,prevSegLength,2))
            self.sketch.addConstraint(Sketcher.Constraint('Vertical',segRadius))
            self.sketch.addGeometry(Part.LineSegment(self.App.Vector(oldLength,radius,0), self.App.Vector(oldLength+length,radius,0)))
            self.sketch.addConstraint(Sketcher.Constraint('DistanceX',segLength,length))
            self.sketch.addConstraint(Sketcher.Constraint('Coincident',segLength,1,segRadius,2))
            self.sketch.addConstraint(Sketcher.Constraint('Horizontal',segLength))

        # close the sketch
        self.sketch.addGeometry(Part.LineSegment(self.App.Vector(oldLength+length,radius,0), self.App.Vector(oldLength+length,innerradius,0)))
        self.sketch.addConstraint(Sketcher.Constraint('Coincident',0,2,segEnd,2))
        self.sketch.addConstraint(Sketcher.Constraint('Coincident',segEnd,1,segLength,2))
        self.lastRadius = radius

        if oldLength == 0:
            # create feature
            self.feature = self.Doc.addObject("PartDesign::Revolution","RevolutionShaft")
            self.feature.Profile = self.sketch
            self.feature.ReferenceAxis = (self.sketch,['H_Axis'])
            self.feature.Angle = 360.0
            self.Doc.recompute()
            self.Gui.hide("SketchShaft")
        else:
            self.Doc.recompute()
        # FIXME: Will give a warning in the console if the active window is not the feature
        self.Gui.SendMsgToActiveView("ViewFit")

    def updateSegment(self, segment, oldLength, length, diameter, innerdiameter):
        constrRadius = 4 + segment * 6
        constrLength = 7 + segment * 6
        constrInnerRadius = 1 # Currently we don't allow multiple different innner diameters
        # update total length
        self.totalLength = self.totalLength - oldLength + length
        # Adjust length of centerline
        self.sketch.setDatum(0,self.totalLength)
        # Adjust segment length
        self.sketch.setDatum(constrLength, length)
        # Adjust diameter
        self.sketch.setDatum(constrRadius, diameter/2)
        # Adjust inner diameter
        self.sketch.setDatum(constrInnerRadius, innerdiameter/2)
        # Update feature
        self.Doc.recompute()
        self.Gui.SendMsgToActiveView("ViewFit")
