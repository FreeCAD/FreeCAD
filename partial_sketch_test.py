import FreeCAD as App
import Part

print('='*60)
print('TEST: Partial Sketch Extrusion with MakeInternals')
print('='*60)

doc = App.newDocument('PartialSketch')

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

# Enable MakeInternals to generate selectable faces
print('Setting MakeInternals = True')
sketch.MakeInternals = True
doc.recompute()

print('Sketch MakeInternals:', sketch.MakeInternals)
print('Sketch shape type:', sketch.Shape.ShapeType)

# Check what faces are available
faces = sketch.Shape.Faces
print('Number of faces:', len(faces))
for i, f in enumerate(faces):
    print(f'  Face{i+1}: Area = {f.Area:.2f}')

if len(faces) < 2:
    print('WARNING: MakeInternals did not create multiple faces')
    print('This feature requires MakeInternals to generate internal faces')
    print('Test cannot proceed.')
else:
    print()
    print('--- Test A: Extrude ALL faces (no sub-selection) ---')
    extrudeAll = doc.addObject('Part::Extrusion', 'ExtrudeAll')
    extrudeAll.Base = sketch  # No sub-elements = all faces
    extrudeAll.Dir = App.Vector(0, 0, 5)
    extrudeAll.Solid = True
    doc.recompute()

    if not extrudeAll.Shape.isNull():
        volumeAll = extrudeAll.Shape.Volume
        # 10*10*5 + 20*10*5 = 500 + 1000 = 1500
        print(f'Volume of all faces extruded: {volumeAll:.2f}')
        print(f'Expected: ~1500 (both rectangles)')
    else:
        print('FAIL: ExtrudeAll shape is null')

    print()
    print('--- Test B: Extrude ONLY Face1 (first rectangle) ---')
    extrudeFace1 = doc.addObject('Part::Extrusion', 'ExtrudeFace1')
    extrudeFace1.Base = (sketch, ['Face1'])  # Only first face
    extrudeFace1.Dir = App.Vector(0, 0, 5)
    extrudeFace1.Solid = True
    doc.recompute()

    if not extrudeFace1.Shape.isNull():
        volumeFace1 = extrudeFace1.Shape.Volume
        # 10*10*5 = 500
        print(f'Volume of Face1 extruded: {volumeFace1:.2f}')
        print(f'Expected: ~500 (first rectangle only)')

        if abs(volumeFace1 - 500) < 10:
            print('PASS: Partial sketch extrusion works!')
        elif abs(volumeFace1 - 1500) < 10:
            print('FAIL: Got all faces instead of just Face1')
        else:
            print(f'INCONCLUSIVE: Got unexpected volume')
    else:
        print('FAIL: ExtrudeFace1 shape is null')

App.closeDocument('PartialSketch')
print()
print('Test completed')
