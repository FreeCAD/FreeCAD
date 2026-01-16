import FreeCAD as App
import Part

print('DEBUG: Testing extrusion')
doc = App.newDocument('Debug')
box = doc.addObject('Part::Box', 'Box')
doc.recompute()

print('Box shape type:', box.Shape.ShapeType)
print('Box faces:', len(box.Shape.Faces))

extrude = doc.addObject('Part::Extrusion', 'Extrude')
extrude.Base = box
extrude.Dir = App.Vector(0, 0, 10)
extrude.Solid = False

print('Before recompute:')
print('  Base:', extrude.Base)
print('  Base[0]:', extrude.Base[0] if extrude.Base else None)
print('  Base[1]:', extrude.Base[1] if extrude.Base else None)

doc.recompute()

print('After recompute:')
print('  Shape null?', extrude.Shape.isNull())
if extrude.Shape.isNull():
    print('  State:', extrude.State)

# Try with a wire instead
print()
print('Testing with a wire:')
wire = Part.makePolygon([App.Vector(0,0,0), App.Vector(10,0,0), App.Vector(10,10,0), App.Vector(0,10,0), App.Vector(0,0,0)])
wireObj = doc.addObject('Part::Feature', 'Wire')
wireObj.Shape = wire
doc.recompute()

extrude2 = doc.addObject('Part::Extrusion', 'Extrude2')
extrude2.Base = wireObj
extrude2.Dir = App.Vector(0, 0, 10)
extrude2.Solid = True
doc.recompute()

print('Wire extrusion shape null?', extrude2.Shape.isNull())
if not extrude2.Shape.isNull():
    print('Shape type:', extrude2.Shape.ShapeType)

App.closeDocument('Debug')
