# Example how to use the basic sketcher tools
from Sketcher import *
from Part import *
from FreeCAD import *
import FreeCAD as App

# set some constances for the constraints
StartPoint = 1
EndPoint = 2
MiddlePoint = 3

# create a document and a Sketch object
if App.activeDocument() is None:
    App.newDocument()

f = App.activeDocument().addObject("Sketcher::SketchObject", "Sketch")

# add geometry to the sketch
f.Geometry = [
    LineSegment(Vector(0, 0, 0), Vector(2, 20, 0)),
    LineSegment(Vector(0, 0, 0), Vector(20, 2, 0)),
]

# add constraints to the sketch
f.Constraints = [Constraint("Vertical", 0), Constraint("Horizontal", 1)]

# recompute (solving) the sketch
App.activeDocument().recompute()

# add another constraint to tie the start points together
l = f.Constraints
l.append(Constraint("Coincident", 0, StartPoint, 1, StartPoint))
f.Constraints = l

# again recompute
App.activeDocument().recompute()

f.Geometry
