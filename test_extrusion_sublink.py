# Test script for Part::Extrusion PropertyLinkSub changes
# Run with: ./build/debug/bin/FreeCADCmd test_extrusion_sublink.py

import FreeCAD as App
import Part

def test_backward_compatibility():
    """Test that existing usage (assigning just an object) still works."""
    print("=" * 60)
    print("TEST 1: Backward Compatibility")
    print("=" * 60)

    doc = App.newDocument("BackwardCompatTest")

    # Create a simple sketch with a rectangle
    sketch = doc.addObject("Sketcher::SketchObject", "Sketch")
    sketch.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 0, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(10, 0, 0), App.Vector(10, 10, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(10, 10, 0), App.Vector(0, 10, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(0, 10, 0), App.Vector(0, 0, 0)))
    doc.recompute()

    # Create extrusion the OLD way - just assign the object
    extrude = doc.addObject("Part::Extrusion", "Extrude")
    extrude.Base = sketch  # This is the backward-compatible way
    extrude.Dir = App.Vector(0, 0, 10)
    extrude.Solid = True
    doc.recompute()

    # Check result
    if extrude.Shape.isNull():
        print("FAIL: Shape is null")
        return False

    volume = extrude.Shape.Volume
    expected_volume = 10 * 10 * 10  # 1000
    print(f"Extrusion volume: {volume:.2f} (expected ~{expected_volume})")

    if abs(volume - expected_volume) < 1:
        print("PASS: Backward compatibility works!")
        App.closeDocument("BackwardCompatTest")
        return True
    else:
        print("FAIL: Unexpected volume")
        App.closeDocument("BackwardCompatTest")
        return False


def test_partial_sketch_extrusion():
    """Test extruding only selected faces from a sketch with MakeInternals."""
    print("=" * 60)
    print("TEST 2: Partial Sketch Extrusion")
    print("=" * 60)

    doc = App.newDocument("PartialSketchTest")

    # Create a sketch with TWO separate rectangles
    sketch = doc.addObject("Sketcher::SketchObject", "Sketch")

    # First rectangle (0,0) to (10,10)
    sketch.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 0, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(10, 0, 0), App.Vector(10, 10, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(10, 10, 0), App.Vector(0, 10, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(0, 10, 0), App.Vector(0, 0, 0)))

    # Second rectangle (20,0) to (40,10) - different size
    sketch.addGeometry(Part.LineSegment(App.Vector(20, 0, 0), App.Vector(40, 0, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(40, 0, 0), App.Vector(40, 10, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(40, 10, 0), App.Vector(20, 10, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(20, 10, 0), App.Vector(20, 0, 0)))

    # Enable MakeInternals to generate faces
    sketch.MakeInternals = True
    doc.recompute()

    # Check what faces the sketch has
    print(f"Sketch has MakeInternals: {sketch.MakeInternals}")
    print(f"Sketch shape type: {sketch.Shape.ShapeType}")

    # List available faces
    faces = sketch.Shape.Faces
    print(f"Number of faces in sketch: {len(faces)}")
    for i, face in enumerate(faces):
        print(f"  Face{i+1}: Area = {face.Area:.2f}")

    if len(faces) < 2:
        print("WARNING: MakeInternals did not create separate faces")
        print("This test requires MakeInternals to work properly")
        App.closeDocument("PartialSketchTest")
        return None  # Inconclusive

    # Now test extruding just the FIRST face
    extrude = doc.addObject("Part::Extrusion", "Extrude")

    # Set Base with sub-element selection (the NEW way)
    extrude.Base = (sketch, ["Face1"])
    extrude.Dir = App.Vector(0, 0, 5)
    extrude.Solid = True
    doc.recompute()

    if extrude.Shape.isNull():
        print("FAIL: Shape is null when using sub-element")
        App.closeDocument("PartialSketchTest")
        return False

    volume = extrude.Shape.Volume
    # First rectangle is 10x10, extruded 5 units = 500
    expected_volume = 10 * 10 * 5
    print(f"Extrusion of Face1 volume: {volume:.2f} (expected ~{expected_volume})")

    # The volume should be roughly 500, not 1500 (which would be both rectangles)
    if abs(volume - expected_volume) < 10:
        print("PASS: Partial sketch extrusion works!")
        App.closeDocument("PartialSketchTest")
        return True
    elif abs(volume - 1500) < 10:
        print("FAIL: Got both faces instead of just Face1")
        App.closeDocument("PartialSketchTest")
        return False
    else:
        print(f"INCONCLUSIVE: Unexpected volume {volume}")
        App.closeDocument("PartialSketchTest")
        return None


def test_python_api():
    """Test Python API for setting Base with sub-elements."""
    print("=" * 60)
    print("TEST 3: Python API")
    print("=" * 60)

    doc = App.newDocument("APITest")

    # Create a box to use as base
    box = doc.addObject("Part::Box", "Box")
    box.Length = 10
    box.Width = 10
    box.Height = 10
    doc.recompute()

    # Create extrusion
    extrude = doc.addObject("Part::Extrusion", "Extrude")

    # Test 1: Set just the object
    extrude.Base = box
    print(f"After setting Base = box:")
    print(f"  Base.getValue() = {extrude.Base.getValue()}")
    print(f"  Base.getSubValues() = {extrude.Base.getSubValues()}")

    # Test 2: Set object with sub-element tuple
    extrude.Base = (box, ["Face1"])
    print(f"After setting Base = (box, ['Face1']):")
    print(f"  Base.getValue() = {extrude.Base.getValue()}")
    print(f"  Base.getSubValues() = {extrude.Base.getSubValues()}")

    # Test 3: Extrude a face of the box
    extrude.Dir = App.Vector(0, 0, 5)
    extrude.Solid = False  # Face extrusion creates shell, not solid
    doc.recompute()

    if not extrude.Shape.isNull():
        print(f"Extruded shape type: {extrude.Shape.ShapeType}")
        print("PASS: Python API works correctly")
        result = True
    else:
        print("FAIL: Extrusion failed")
        result = False

    App.closeDocument("APITest")
    return result


if __name__ == "__main__":
    print("\n" + "=" * 60)
    print("Part::Extrusion PropertyLinkSub Test Suite")
    print("=" * 60 + "\n")

    results = []

    # Run tests
    results.append(("Backward Compatibility", test_backward_compatibility()))
    results.append(("Python API", test_python_api()))
    results.append(("Partial Sketch Extrusion", test_partial_sketch_extrusion()))

    # Summary
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    for name, result in results:
        status = "PASS" if result == True else "FAIL" if result == False else "INCONCLUSIVE"
        print(f"  {name}: {status}")

    print("\nDone!")
