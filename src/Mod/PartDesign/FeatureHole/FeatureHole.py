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
import Part, Sketcher,  PartDesignGui
import math

def makeVector(point):
    if point.__class__ == FreeCAD.Vector:
        return point
    return FreeCAD.Vector(point.X,  point.Y,  point.Z)

class Hole():
    "Hole feature"
    App = FreeCAD
    Gui = FreeCADGui

    def __init__(self, feature):
        self.feature = feature
        self.feature.addProperty("App::PropertyString","HoleType","Hole","Type of hole").HoleType="Depth"
        self.feature.addProperty("App::PropertyBool","Threaded","Hole","Threaded hole").Threaded=False
        self.feature.addProperty("App::PropertyBool","Counterbore","Hole","Counterbore hole").Counterbore=False
        self.feature.addProperty("App::PropertyBool","Countersink","Hole","Countersink hole").Countersink=False
        self.feature.addProperty("App::PropertyString","Norm","Hole","Name of norm").Norm="Custom"
        self.feature.addProperty("App::PropertyString","NormTolerance","Hole","Tolerance field of norm").NormTolerance="medium"
        self.feature.addProperty("App::PropertyLength","NormDiameter","Hole","Nominal diameter of hole").NormDiameter=4.0
        self.feature.addProperty("App::PropertyString", "ExtraNorm", "Hole", "Norm of bolt or washer used in hole").ExtraNorm="ISO 4762"
        self.feature.addProperty("App::PropertyString", "NormThread", "Hole", "Norm of thread").NormThread="DIN 13-1"
        self.feature.addProperty("App::PropertyString", "NormThreadFinish", "Hole", "Norm defining thread finish length").NormThreadFinish="DIN 76-2"
        self.feature.addProperty("App::PropertyLength","Diameter","Hole","Diameter of hole").Diameter=5.0
        self.feature.addProperty("App::PropertyLength","Depth","Hole","Depth of hole").Depth=8.0
        self.feature.addProperty("App::PropertyLength","CounterboreDiameter","Hole","Diameter of counterbore").CounterboreDiameter=10.0
        self.feature.addProperty("App::PropertyLength","CounterboreDepth","Hole","Depth of counterbore").CounterboreDepth=4.0
        self.feature.addProperty("App::PropertyLength","CountersinkAngle","Hole","Angle of countersink").CountersinkAngle=45.0
        self.feature.addProperty("App::PropertyLength","ThreadLength","Hole","Length of thread").ThreadLength=5.0
        self.feature.addProperty("App::PropertyString","PositionType","Hole","Type of position references").PositionType="Linear"
        self.feature.addProperty("App::PropertyLinkSub","Support","Hole","Support of hole feature").Support=None
        self.feature.addProperty("App::PropertyLink","HoleGroove","Hole","Revolution feature creating the hole").HoleGroove=None
        # Create new HoleGroove feature
        body = FreeCADGui.activeView().getActiveObject("pdbody")
        self.sketchaxis = self.feature.Document.addObject("PartDesign::Line", "HoleSketchAxis")
        body.addFeature(self.sketchaxis)
        self.Gui.ActiveDocument.hide(self.sketchaxis.Name)
        self.sketchplane = self.feature.Document.addObject("PartDesign::Plane", "HoleSketchPlane")
        self.sketchplane.References = (self.sketchaxis,  "")
        body.addFeature(self.sketchplane)
        self.Gui.ActiveDocument.hide(self.sketchplane.Name)
        self.sketch = self.feature.Document.addObject("Sketcher::SketchObject","HoleSketch")
        self.sketch.Support = (self.sketchplane,  ["front"])
        body.addFeature(self.sketch)
        self.Gui.ActiveDocument.hide(self.sketch.Name)
        feature.HoleGroove = feature.Document.addObject("PartDesign::Groove","HoleGroove")
        feature.HoleGroove.Angle = 360.0
        feature.HoleGroove.Sketch = self.sketch
        body.addFeature(feature.HoleGroove)
        self.Gui.ActiveDocument.hide(feature.HoleGroove.Name)
        self.feature.Proxy = self
        self.oldCounterbore = False
        self.oldCountersink = False

    def execute(self, feature):
        if feature.Support is not None:
            (support, element) = feature.Support
            feature.Placement = feature.HoleGroove.Placement
            shape = feature.HoleGroove.Shape.copy()
            shape.Placement = FreeCAD.Placement()
            feature.Shape = shape

            self.Gui.ActiveDocument.hide(support.Name)
            # Copy display properties from support
            featview = feature.ViewObject
            suppview = support.ViewObject
            for p in suppview.PropertiesList:
                if not p in ["DisplayMode","BoundingBox","Proxy","RootNode","Visibility"]:
                    if p in featview.PropertiesList:
                        val = getattr(suppview,p)
                        setattr(featview,p,val)
            if suppview.DisplayMode in featview.listDisplayModes():
                featview.DisplayMode = suppview.DisplayMode
            if hasattr(suppview,"DiffuseColor") and hasattr(featview,"DiffuseColor"):
                featview.DiffuseColor = suppview.DiffuseColor

    def onChanged(self, fp, prop):
        #self.App.Console.PrintMessage("Change property: " + str(prop) + "\n")
        if fp is None or fp.Support is None:
            return

        if (prop == "HoleType" or prop == "Threaded" or prop == "Counterbore" or prop == "Countersink"
                or prop == "Diameter" or prop == "Depth"
                or prop == "CounterboreDiameter" or prop == "CounterboreDepth"
                or prop == "CountersinkAngle"):
            self.executeSketchChanged(fp)
            fp.Document.recompute()
        elif prop == "Support":
            self.executePositionChanged(fp)
            fp.Document.recompute()

    def executePositionChanged(self, fp):
        "Change the position of the hole"
        if fp.Support is None:
            return
        plane = self.feature.HoleGroove.Sketch.Support[0]
        # Get support (face)
        (support, elementList) = fp.Support
        face = eval("support.Shape." + elementList[0])
        refs = plane.References
        if len(refs) == 0:
            return

        axis = plane.References[0][0]
        firstTime = (len(axis.References) == 0)
        if firstTime:
            # Try to guess some references (using arcs or lines of the outer wire of the support face)
            wire = face.OuterWire
            firstLine = None
            for e in wire.Edges:
                if type(e.Curve) == Part.LineSegment:
                    if firstLine is None:
                        firstLine = e
                        firstDirection = e.Curve.EndPoint - e.Curve.StartPoint
                    else:
                        if firstDirection == e.Curve.EndPoint - e.Curve.StartPoint or firstDirection == e.Curve.StartPoint - e.Curve.EndPoint:
                            continue # Parallel edges
                        allEdges = support.Shape.Edges
                        firstLineIndex = -1
                        secondLineIndex = -1
                        for i in range(len(allEdges)):
                            try:
                                if type(allEdges[i].Curve) != Part.LineSegment:
                                    continue
                                if (allEdges[i].Curve.StartPoint == firstLine.Curve.StartPoint and allEdges[i].Curve.EndPoint == firstLine.Curve.EndPoint) or (allEdges[i].Curve.EndPoint == firstLine.Curve.StartPoint and allEdges[i].Curve.StartPoint == firstLine.Curve.EndPoint):
                                    firstLineIndex = i
                                elif (allEdges[i].Curve.StartPoint == e.Curve.StartPoint and allEdges[i].Curve.EndPoint == e.Curve.EndPoint) or (allEdges[i].Curve.EndPoint == e.Curve.StartPoint and allEdges[i].Curve.StartPoint == e.Curve.EndPoint):
                                    secondLineIndex = i
                                if (firstLineIndex > -1) and (secondLineIndex > -1):
                                    break
                            except Exception:
                                # Unknown curvetype GeomAbs_OtherCurve
                                continue
                        axis.References = [(support,  elementList[0]),  (support,  "Edge" + str(firstLineIndex+1)),  (support,  "Edge" + str(secondLineIndex+1))]
                        axis.Offset = 1.0
                        axis.Offset2 = 1.0
                        self.feature.PositionType = "Linear"
                        # Place the axis approximately in the center of the face
                        #p = face.CenterOfMass
                        #l1 = Part.LineSegment(firstLine.Curve)
                        #l2 = Part.LineSegment(e.Curve)
                        #axis.Offset = p.distanceToLine(l1.StartPoint,  l1.EndPoint - l1.StartPoint)
                        #axis.Offset2 = p.distanceToLine(l1.StartPoint,  l2.EndPoint - l2.StartPoint)
                        # TODO: Ensure that the hole is inside the face!
                        break
                elif type(e.Curve) == Part.Circle:
                    allEdges = support.Shape.Edges
                    for i in range(len(allEdges)):
                        try:
                            if type(allEdges[i].Curve) != Part.Circle:
                                continue
                            c = allEdges[i].Curve
                            if c.Center == e.Curve.Center and c.Axis == e.Curve.Axis and c.Radius == e.Curve.Radius:
                                axis.References = [(support,  "Edge" + str(i+1))]
                                self.feature.PositionType = "Coaxial"
                                break
                        except:
                            # Unknown curvetype
                            continue
                elif type(e.Curve) == Part.ArcOfCircle:
                    allEdges = support.Shape.Edges
                    for i in range(len(allEdges)):
                        try:
                            if type(allEdges[i].Curve) != Part.ArcOfCircle:
                                continue
                            a = allEdges[i].Curve
                            if a.Center == e.Curve.Center and a.Axis == e.Curve.Axis and a.Radius == e.Curve.Radius and a.FirstParameter == e.Curve.FirstParameter and a.LastParameter == e.Curve.LastParameter:
                                axis.References = [(support,  "Edge" + str(i+1))]
                                self.feature.PositionType = "Coaxial"
                                break
                        except:
                            continue
                    break

        # Grab a point from the wire of the support face
        axisbase = axis.Shape.Curve.StartPoint
        axisdir = axis.Shape.Curve.EndPoint - axisbase
        found = False
        if not firstTime and len(refs) > 1:
            # Try to keep the old point, to avoid the sketch plane jumping around
            (obj, sub) = refs[1]
            point = eval("support.Shape." + sub)
            if point.Point.distanceToLine(axisbase,  axisdir) > 1E-10: # TODO: Precision::Confusion()
                found = True
        if not found:
            for p in face.OuterWire.Vertexes:
                if p.Point.distanceToLine(axisbase,  axisdir) > 1E-10: # TODO: Precision::Confusion()
                    point = p
                    found = True
                    break
        if not found:
            point = face.OuterWire.Vertexes[0] # Better this than nothing... and it can't actually happen, can it?

        # Find the index of the point in the support shape
        allVertexes = support.Shape.Vertexes
        for v in range(len(allVertexes)):
            if allVertexes[v].Point == point.Point:
                # Use this point and the axis to define the sketch plane
                if len(refs) < 2:
                    refs.append((support,  "Vertex" + str(v+1)))
                else:
                    refs[1] = (support,  "Vertex" + str(v+1))
                break
        plane.References = refs
        if firstTime:
            fp.Document.recompute() # Update the Sketch Placement property
            self.executeSketchChanged(fp) # Build the sketch of the hole
            fp.Document.recompute()
        else:
            self.executeSketchChanged(fp) # Update the sketch of the hole
        self.setHoleDirection(fp)

    def setHoleDirection(self,  feature):
        # Make sure the hole goes into the material, not out of it
        sketch = feature.HoleGroove.Sketch
        axis = sketch.Support[0].References[0][0]
        axisbase = axis.Shape.Curve.StartPoint
        axisdir = axis.Shape.Curve.EndPoint - axisbase
        p1 = None
        p2 = None
        for v in sketch.Shape.Vertexes:
            # Find the two sketch vertices that are on the sketch axis
            if v.Point.distanceToLine(axisbase, axisdir) < 1E-10: # TODO: use Precision::Confusion()
                if p1 is None:
                    p1 = v.Point
                else:
                    p2 = v.Point
                    break
        if p1 is not None and p2 is not None:
            (support, elementList) = feature.Support
            face = eval("support.Shape." + elementList[0])
            plane = face.Surface
            if type(plane) != Part.Plane:
                return
            # Find the vertex that is on the top of the hole
            if p1.distanceToPlane(plane.Position,  plane.Axis) < 1E-10:
                top = p1
                dir = p2 - p1
            else:
                top = p2
                dir = p1 - p2
            if not support.Shape.isInside(top + dir.multiply(1E-8),  1E-10,  False):
                # Toggle the angle
                angle = sketch.Constraints[12].Value
                if angle == math.pi:
                    sketch.setDatum(12,  0.0)
                else:
                    sketch.setDatum(12,  math.pi)

    def executeSketchChanged(self, fp):
        "Change the sketch shape of the hole"
        if self.feature.HoleGroove is None:
            return
        if fp.HoleType == "Thru":
            # TODO: Make this more stable
            length = 1E+4
        else:
            length = fp.Depth
        radius = fp.Diameter / 2.0

        if fp.Counterbore:
            self.createOrUpdateCounterboreSketch(fp, length, radius)
        elif fp.Countersink:
            self.createOrUpdateCountersinkSketch(fp, length, radius)
        else:
            self.createOrUpdateStandardSketch(fp, length, radius)

    def createOrUpdateStandardSketch(self, fp, depth, radius):
        (support,  elements) = fp.Support
        if fp.HoleGroove.Sketch.GeometryCount == 0:
            #FreeCAD.Console.PrintMessage("Standard sketch\n")
            # New sketch
            sketch = fp.HoleGroove.Sketch
            axis = sketch.Support[0].References[0][0]
            # Geo  -1,1 is the origin (Point)
            # Geo -1 is the X-axis
            # Geo -2 is the Y-axis
            # First external geometry is -3
            sketch.addExternal(axis.Name,"LineSegment") # Geo -3: Datum axis
            sketch.addExternal(support.Name,  elements[0]) # Geo -4: Support face
            # Note: Creating the sketch first with depth = 100.0 and then changing the constraint later seems to be more stable
            tempDepth = 100.0
            # Build the sketch
            sketch.addGeometry(Part.LineSegment(self.App.Vector(10.0,50.0,0),self.App.Vector(10.0,-50.0,0))) # Geo0: Rotation axis
            sketch.toggleConstruction(0)
            sketch.addGeometry(Part.LineSegment(self.App.Vector(10.0,-10.0,0),self.App.Vector(10.0,-30.0,0))) # Geo1: Vertical axis of hole
            sketch.addConstraint(Sketcher.Constraint('PointOnObject',1,1,0))# Datum0
            sketch.addConstraint(Sketcher.Constraint('PointOnObject',1,2,0))# Datum1
            sketch.addGeometry(Part.LineSegment(self.App.Vector(10.0,-10.0,0),self.App.Vector(20.0,-10.0,0))) # Geo2: Top of hole
            sketch.addConstraint(Sketcher.Constraint('Coincident',1,1,2,1)) # Datum2
            sketch.addConstraint(Sketcher.Constraint('Perpendicular',2, 1))  # Datum3
            sketch.addGeometry(Part.LineSegment(self.App.Vector(20.0,-10.0,0),self.App.Vector(20.0,-25.0,0))) # Geo3: Vertical mantle of hole
            sketch.addConstraint(Sketcher.Constraint('Coincident',2,2,3,1)) # temporary
            sketch.addConstraint(Sketcher.Constraint('Parallel',3, 1))         # Datum4
            sketch.addConstraint(Sketcher.Constraint('Distance',3,2,1, 10.0))  # Datum5: Radius
            sketch.addConstraint(Sketcher.Constraint('Distance',3,2,2, 15.0)) # Datum6: Depth
            sketch.addGeometry(Part.LineSegment(self.App.Vector(10.0,-30.0,0),self.App.Vector(20.0,-25.0,0))) # Geo4: 118 degree tip angle
            sketch.addConstraint(Sketcher.Constraint('Coincident',4,1,1,2)) # Datum7
            sketch.addConstraint(Sketcher.Constraint('Coincident',4,2,3,2)) # Datum8
            # TODO: The tip angle of 118 degrees is for steel only. It should be taken from Part material data
            # (as soon as that is implemented)
            sketch.addConstraint(Sketcher.Constraint('Angle',4,1,1,2, 118.0/2.0 * math.pi / 180.0)) # Datum9
            # Locate at the intersection of the two external geometries
            sketch.addConstraint(Sketcher.Constraint('PointOnObject',1,1,-3))# Datum10
            sketch.addConstraint(Sketcher.Constraint('PointOnObject',1,1,-4))# Datum11
            sketch.addConstraint(Sketcher.Constraint('Angle',0,1,-3, 1,  0.0))# Datum12
            # This datum is specific for this holetype, so move it to the last position
            sketch.delConstraint(4)
            sketch.addConstraint(Sketcher.Constraint('Coincident',2,2,3,1)) # Datum13
            fp.HoleGroove.ReferenceAxis = (sketch,['Axis0'])
        if self.oldCounterbore == True:
            # Remove counterbore from existing sketch
            #FreeCAD.Console.PrintMessage("Counter to Standard sketch\n")
            sketch = fp.HoleGroove.Sketch
            sketch.delConstraint(19)
            sketch.delConstraint(18)
            sketch.delConstraint(17)
            sketch.delConstraint(16)
            sketch.delConstraint(15)
            sketch.delConstraint(14)
            sketch.delConstraint(13)
            sketch.delGeometry(6)
            sketch.delGeometry(5)
            sketch.addConstraint(Sketcher.Constraint('Coincident',2,2,3,1)) # Datum13
        elif self.oldCountersink == True:
            # Remove countersink from existing sketch
            #FreeCAD.Console.PrintMessage("Sink to Standard sketch\n")
            sketch = fp.HoleGroove.Sketch
            sketch.delConstraint(16)
            sketch.delConstraint(15)
            sketch.delConstraint(14)
            sketch.delConstraint(13)
            sketch.delGeometry(5)
            sketch.addConstraint(Sketcher.Constraint('Coincident',2,2,3,1)) # Datum13
        else:
            # Update existing standard sketch
            #FreeCAD.Console.PrintMessage("Update Standard sketch\n")
            sketch = fp.HoleGroove.Sketch
            sketch.setDatum(5, radius)
            sketch.setDatum(6, depth)
            if sketch.ExternalGeometry[1] != (support,  elements[0]):
                # Update the external geometry references
                angle = sketch.Constraints[12].Value
                sketch.delConstraint(13)
                sketch.delConstraint(12)
                sketch.delConstraint(11)
                sketch.delExternal(1)
                sketch.addExternal(support.Name,  elements[0]) # Geo -4: Support face
                sketch.addConstraint(Sketcher.Constraint('PointOnObject',1,1,-4))# Datum11
                sketch.addConstraint(Sketcher.Constraint('Angle',0,1,-3, 1,  angle))# Datum12
                sketch.addConstraint(Sketcher.Constraint('Coincident',2,2,3,1)) # Datum13

        self.setHoleDirection(fp)
        self.oldCounterbore = False
        self.oldCountersink = False

    def createOrUpdateCounterboreSketch(self, fp, depth, radius):
        cradius = fp.CounterboreDiameter / 2.0
        cdepth = fp.CounterboreDepth
        (support,  elements) = fp.Support

        if self.oldCounterbore == True:
            # Update properties of existing counterbore sketch
            #FreeCAD.Console.PrintMessage("Update to Counterbore sketch\n")
            sketch = fp.HoleGroove.Sketch
            sketch.setDatum(5, radius)
            sketch.setDatum(6, depth)
            sketch.setDatum(13, cradius)
            sketch.setDatum(15, cdepth)
            if sketch.ExternalGeometry[1] != (support,  elements[0]):
                # Update the external geometry references
                angle = sketch.Constraints[12].Value
                sketch.delConstraint(19)
                sketch.delConstraint(18)
                sketch.delConstraint(17)
                sketch.delConstraint(16)
                sketch.delConstraint(15)
                sketch.delConstraint(14)
                sketch.delConstraint(13)
                sketch.delConstraint(12)
                sketch.delConstraint(11)
                sketch.delExternal(1)
                sketch.addExternal(support.Name,  elements[0]) # Geo -4: Support face
                sketch.addConstraint(Sketcher.Constraint('PointOnObject',1,1,-4))# Datum11
                sketch.addConstraint(Sketcher.Constraint('Angle',0,1,-3, 1,  angle))# Datum12
                sketch.addConstraint(Sketcher.Constraint('Distance',2, cradius))  # Datum13
                sketch.addConstraint(Sketcher.Constraint('Coincident',2,2,5,1)) # Datum14
                sketch.addConstraint(Sketcher.Constraint('Distance',3, 1, 2, cdepth))  # Datum15
                sketch.addConstraint(Sketcher.Constraint('Parallel',5, 1))         # Datum16
                sketch.addConstraint(Sketcher.Constraint('Coincident',5,2,6,1)) # Datum17
                sketch.addConstraint(Sketcher.Constraint('Perpendicular',6, -3))  # Datum18
                sketch.addConstraint(Sketcher.Constraint('Coincident',6,2,3,1)) # Datum19
        else:
            # Change standard to counterbore in existing sketch
            #FreeCAD.Console.PrintMessage("Standard to Counterbore sketch\n")
            sketch = fp.HoleGroove.Sketch
            sketch.delConstraint(13)
            sketch.addConstraint(Sketcher.Constraint('Distance',2, cradius))  # Datum13
            p2 = sketch.Geometry[2].EndPoint
            sketch.addGeometry(Part.LineSegment(p2,self.App.Vector(p2.x,p2.y-20.0,0))) # Geo5: Vertical mantle of counterbore
            sketch.addConstraint(Sketcher.Constraint('Coincident',2,2,5,1)) # Datum14
            sketch.addConstraint(Sketcher.Constraint('Distance',3, 1, 2, cdepth))  # Datum15
            sketch.addConstraint(Sketcher.Constraint('Parallel',5, 1))         # Datum16
            p3 = sketch.Geometry[3].StartPoint
            sketch.addGeometry(Part.LineSegment(self.App.Vector(p2.x,p2.y-20.0, 0),p3)) # Geo6: bottom of counterbore
            sketch.addConstraint(Sketcher.Constraint('Coincident',5,2,6,1)) # Datum17
            sketch.addConstraint(Sketcher.Constraint('Perpendicular',6, -3))  # Datum18
            sketch.addConstraint(Sketcher.Constraint('Coincident',6,2,3,1)) # Datum19

        self.setHoleDirection(fp)
        self.oldCounterbore = True
        self.oldCountersink = False

    def createOrUpdateCountersinkSketch(self, fp, depth, radius):
        sradius = fp.CounterboreDiameter / 2.0
        sangle = fp.CountersinkAngle * math.pi / 180.0
        (support,  elements) = fp.Support

        if self.oldCountersink == True:
            # Update properties of existing countersink sketch
            #FreeCAD.Console.PrintMessage("Update to Countersink sketch\n")
            sketch = fp.HoleGroove.Sketch
            sketch.setDatum(5, radius)
            sketch.setDatum(6, depth)
            sketch.setDatum(13, sradius)
            sketch.setDatum(15, sangle)
            if sketch.ExternalGeometry[1] != (support,  elements[0]):
                # Update the external geometry references
                angle = sketch.Constraints[12].Value
                sketch.delConstraint(16)
                sketch.delConstraint(15)
                sketch.delConstraint(14)
                sketch.delConstraint(13)
                sketch.delConstraint(12)
                sketch.delConstraint(11)
                sketch.delExternal(1)
                sketch.addExternal(support.Name,  elements[0]) # Geo -4: Support face
                sketch.addConstraint(Sketcher.Constraint('PointOnObject',1,1,-4))# Datum11
                sketch.addConstraint(Sketcher.Constraint('Angle',0,1,-3, 1,  angle))# Datum12
                sketch.addConstraint(Sketcher.Constraint('Distance',2, sradius))  # Datum13
                sketch.addConstraint(Sketcher.Constraint('Coincident',2,2,5,1)) # Datum14
                sketch.addConstraint(Sketcher.Constraint('Angle',5,2, 1,2,  sangle))  # Datum15
                sketch.addConstraint(Sketcher.Constraint('Coincident',3,1,5,2)) # Datum16
        else:
            # Change standard to countersink in existing sketch
            #FreeCAD.Console.PrintMessage("Standard to Countersink sketch\n")
            sketch = fp.HoleGroove.Sketch
            sketch.delConstraint(13)
            sketch.addConstraint(Sketcher.Constraint('Distance',2, sradius))  # Datum13
            p2 = sketch.Geometry[2].EndPoint
            sketch.addGeometry(Part.LineSegment(p2,self.App.Vector(p2.x,p2.y-20.0,0))) # Geo5: Chamfer of countersink
            sketch.addConstraint(Sketcher.Constraint('Coincident',2,2,5,1)) # Datum14
            sketch.addConstraint(Sketcher.Constraint('Angle',5,2, 1,2,  sangle))  # Datum15
            sketch.addConstraint(Sketcher.Constraint('Coincident',3,1,5,2)) # Datum16

        self.setHoleDirection(fp)
        self.oldCounterbore = False
        self.oldCountersink = True
