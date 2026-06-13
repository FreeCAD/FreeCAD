# Example how to use the basic Text tools
from Text import *
from Part import *
from FreeCAD import *
import FreeCAD as App

# create a document and a Text object
if App.activeDocument() is None:
    App.newDocument()

f = App.activeDocument().addObject("Text::ShapeText", "Text")

# add geometry to the sketch
f.String = "This is a test"

# recompute (solving) the text
App.activeDocument().recompute()

f.Geometry
