import FreeCAD as App
import Part

print('='*60)
print('TEST 1: Backward Compatibility - Wire Extrusion')
print('='*60)
doc = App.newDocument('Test1')

# Create a closed wire (rectangle)
wire = Part.makePolygon([
    App.Vector(0,0,0),
    App.Vector(10,0,0),
    App.Vector(10,10,0),
    App.Vector(0,10,0),
    App.Vector(0,0,0)
])
wireObj = doc.addObject('Part::Feature', 'Wire')
wireObj.Shape = wire
doc.recompute()

# Create extrusion using OLD syntax - just assign the object
extrude = doc.addObject('Part::Extrusion', 'Extrude')
extrude.Base = wireObj  # No sub-elements
extrude.Dir = App.Vector(0, 0, 10)
extrude.Solid = True
doc.recompute()

print('Base:', extrude.Base)
print('Shape null?', extrude.Shape.isNull())
if not extrude.Shape.isNull():
    print('Shape type:', extrude.Shape.ShapeType)
    print('Volume:', extrude.Shape.Volume)
    if abs(extrude.Shape.Volume - 1000) < 1:
        print('PASS: Backward compatibility works for wire extrusion')
    else:
        print('FAIL: Unexpected volume')
else:
    print('FAIL: Shape is null')
App.closeDocument('Test1')

print()
print('='*60)
print('TEST 2: Sketch Extrusion (Typical Use Case)')
print('='*60)
doc = App.newDocument('Test2')

# Create a sketch with a rectangle
sketch = doc.addObject('Sketcher::SketchObject', 'Sketch')
sketch.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(20, 0, 0)))
sketch.addGeometry(Part.LineSegment(App.Vector(20, 0, 0), App.Vector(20, 15, 0)))
sketch.addGeometry(Part.LineSegment(App.Vector(20, 15, 0), App.Vector(0, 15, 0)))
sketch.addGeometry(Part.LineSegment(App.Vector(0, 15, 0), App.Vector(0, 0, 0)))
doc.recompute()

print('Sketch shape type:', sketch.Shape.ShapeType)

# Create extrusion - assign just the sketch
extrude = doc.addObject('Part::Extrusion', 'Extrude')
extrude.Base = sketch
extrude.Dir = App.Vector(0, 0, 5)
extrude.Solid = True
doc.recompute()

print('Base:', extrude.Base)
print('Shape null?', extrude.Shape.isNull())
if not extrude.Shape.isNull():
    print('Shape type:', extrude.Shape.ShapeType)
    print('Volume:', extrude.Shape.Volume)
    expected = 20 * 15 * 5  # 1500
    if abs(extrude.Shape.Volume - expected) < 1:
        print('PASS: Sketch extrusion works')
    else:
        print('FAIL: Unexpected volume (expected {})'.format(expected))
else:
    print('FAIL: Shape is null')
App.closeDocument('Test2')

print()
print('='*60)
print('TEST 3: Face Selection from Solid')
print('='*60)
doc = App.newDocument('Test3')

box = doc.addObject('Part::Box', 'Box')
box.Length = 10
box.Width = 10
box.Height = 10
doc.recompute()

# Extrude just one face of the box
extrude = doc.addObject('Part::Extrusion', 'Extrude')
extrude.Base = (box, ['Face1'])  # NEW syntax with sub-element
extrude.Dir = App.Vector(0, 0, 5)
extrude.Solid = False
doc.recompute()

print('Base:', extrude.Base)
print('Shape null?', extrude.Shape.isNull())
if not extrude.Shape.isNull():
    print('Shape type:', extrude.Shape.ShapeType)
    print('PASS: Face selection works')
else:
    print('FAIL: Shape is null')
App.closeDocument('Test3')

print()
print('='*60)
print('SUMMARY')
print('='*60)
