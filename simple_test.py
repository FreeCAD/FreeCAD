import FreeCAD as App
import Part

print('TEST 1: Backward Compatibility')
doc = App.newDocument('Test1')
box = doc.addObject('Part::Box', 'Box')
doc.recompute()

# Create extrusion using OLD syntax
extrude = doc.addObject('Part::Extrusion', 'Extrude')
extrude.Base = box  # Just the object, no sub-elements
extrude.Dir = App.Vector(0, 0, 10)
extrude.Solid = False
doc.recompute()

print('Base:', extrude.Base)
print('Shape null?', extrude.Shape.isNull())
if not extrude.Shape.isNull():
    print('PASS: Backward compatibility works')
else:
    print('FAIL: Shape is null')
App.closeDocument('Test1')

print()
print('TEST 2: New syntax with sub-elements')
doc = App.newDocument('Test2')
box = doc.addObject('Part::Box', 'Box')
doc.recompute()

extrude = doc.addObject('Part::Extrusion', 'Extrude')
# Set with sub-element (extrude just one face)
extrude.Base = (box, ['Face1'])
extrude.Dir = App.Vector(10, 0, 0)
extrude.Solid = False
doc.recompute()

print('Base:', extrude.Base)
print('Shape null?', extrude.Shape.isNull())
if not extrude.Shape.isNull():
    print('Shape type:', extrude.Shape.ShapeType)
    print('PASS: Sub-element selection works')
else:
    print('FAIL: Shape is null')
App.closeDocument('Test2')

print()
print('All tests completed')
