import FreeCAD as App
import Part

doc = App.newDocument('Explore')

# Create a sketch with TWO separate closed rectangles
sketch = doc.addObject('Sketcher::SketchObject', 'Sketch')

# First rectangle: 10x10 at origin
sketch.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 0, 0)))
sketch.addGeometry(Part.LineSegment(App.Vector(10, 0, 0), App.Vector(10, 10, 0)))
sketch.addGeometry(Part.LineSegment(App.Vector(10, 10, 0), App.Vector(0, 10, 0)))
sketch.addGeometry(Part.LineSegment(App.Vector(0, 10, 0), App.Vector(0, 0, 0)))

# Second rectangle: 20x10 offset to the right
sketch.addGeometry(Part.LineSegment(App.Vector(30, 0, 0), App.Vector(50, 0, 0)))
sketch.addGeometry(Part.LineSegment(App.Vector(50, 0, 0), App.Vector(50, 10, 0)))
sketch.addGeometry(Part.LineSegment(App.Vector(50, 10, 0), App.Vector(30, 10, 0)))
sketch.addGeometry(Part.LineSegment(App.Vector(30, 10, 0), App.Vector(30, 0, 0)))

sketch.MakeInternals = True
doc.recompute()

print('Sketch properties:')
for prop in sketch.PropertiesList:
    if 'Shape' in prop or 'Internal' in prop:
        print(f'  {prop}')

print()
print('Shape:', sketch.Shape)
print('Shape.ShapeType:', sketch.Shape.ShapeType)
print('Shape.Faces:', len(sketch.Shape.Faces))
print('Shape.Wires:', len(sketch.Shape.Wires))

if hasattr(sketch, 'InternalShape'):
    print()
    print('InternalShape:', sketch.InternalShape)
    if not sketch.InternalShape.isNull():
        print('InternalShape.ShapeType:', sketch.InternalShape.ShapeType)
        print('InternalShape.Faces:', len(sketch.InternalShape.Faces))
        for i, f in enumerate(sketch.InternalShape.Faces):
            print(f'  Face{i+1}: Area = {f.Area:.2f}')

# Try to see what elements are available
print()
print('Getting internal element map...')
if hasattr(sketch, 'getInternalElementMap'):
    emap = sketch.getInternalElementMap()
    print('Internal element map:', emap)
else:
    print('No getInternalElementMap method')

# Check the full shape including internals
print()
print('Trying Part.Feature.getTopoShape...')
import Part
fullShape = Part.Feature.getTopoShape(sketch, Part.ShapeOption.ResolveLink | Part.ShapeOption.Transform)
print('Full shape type:', fullShape.ShapeType if fullShape else 'None')
print('Full shape faces:', len(fullShape.Faces) if fullShape else 0)

App.closeDocument('Explore')
