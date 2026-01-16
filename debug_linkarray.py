"""Debug script to understand Link Array subname resolution."""
import FreeCAD as App
import Part

# This script should be run after creating:
# 1. A sketch with geometry
# 2. An extrusion of that sketch
# 3. A Draft polar link array of the extrusion
#
# Rename the array to "Array" before running

doc = App.ActiveDocument
if not doc:
    print("No active document!")
else:
    # Find the array object
    array = doc.getObject("Array")
    if not array:
        print("No object named 'Array' found")
        print("Available objects:", [o.Name for o in doc.Objects])
    else:
        print("Array object:", array.Name)
        print("Array type:", array.TypeId)
        print()

        # Check if it has Link extension
        if array.hasExtension("App::LinkBaseExtension"):
            print("Has LinkBaseExtension: YES")
            print("  ElementCount:", array.ElementCount if hasattr(array, 'ElementCount') else "N/A")
            print("  PlacementList size:", len(array.PlacementList) if hasattr(array, 'PlacementList') else "N/A")
            if hasattr(array, 'PlacementList') and len(array.PlacementList) > 0:
                print("  PlacementList[0]:", array.PlacementList[0])
                if len(array.PlacementList) > 1:
                    print("  PlacementList[1]:", array.PlacementList[1])
            # Check LinkedObject / Base
            if hasattr(array, 'LinkedObject'):
                print("  LinkedObject:", array.LinkedObject)
            if hasattr(array, 'Base'):
                print("  Base:", array.Base)
        else:
            print("Has LinkBaseExtension: NO")
        print()

        # Check what subobjects are available
        print("SubObjects:", array.getSubObjects())
        print()

        # First check the original/base object
        if hasattr(array, 'Base') and array.Base:
            base = array.Base
            print("Base object:", base.Name)
            if hasattr(base, 'Shape') and base.Shape:
                bb = base.Shape.BoundBox
                print(f"  Base Shape BoundBox center: ({bb.Center.x:.2f}, {bb.Center.y:.2f}, {bb.Center.z:.2f})")
                # Get Edge1 from base
                try:
                    edge = base.Shape.getElement("Edge1")
                    param = (edge.FirstParameter + edge.LastParameter) / 2
                    pt = edge.valueAt(param)
                    print(f"  Base Edge1 midpoint: ({pt.x:.2f}, {pt.y:.2f}, {pt.z:.2f})")
                except:
                    print("  Could not get Edge1 from base")
            print()

        # Try to get shape with different subnames
        test_subnames = ["", "0.", "1.", "0.Edge1", "1.Edge1"]
        for subname in test_subnames:
            print(f"Testing subname: '{subname}'")
            try:
                mat = App.Matrix()
                subobj = array.getSubObject(subname, retType=1, matrix=mat)
                print(f"  getSubObject result: {subobj}")
                print(f"  Matrix translation: ({mat.A14:.2f}, {mat.A24:.2f}, {mat.A34:.2f})")

                # Also try Part.getShape
                shape = Part.getShape(array, subname, needSubElement=True, transform=True)
                if not shape.isNull():
                    if shape.ShapeType == "Edge":
                        # Get edge midpoint
                        edge = shape
                        param = (edge.FirstParameter + edge.LastParameter) / 2
                        pt = edge.valueAt(param)
                        print(f"  Shape edge midpoint: ({pt.x:.2f}, {pt.y:.2f}, {pt.z:.2f})")
                    elif shape.ShapeType == "Compound":
                        bb = shape.BoundBox
                        print(f"  Shape BoundBox center: ({bb.Center.x:.2f}, {bb.Center.y:.2f}, {bb.Center.z:.2f})")
                    else:
                        bb = shape.BoundBox
                        print(f"  Shape type: {shape.ShapeType}, BoundBox center: ({bb.Center.x:.2f}, {bb.Center.y:.2f}, {bb.Center.z:.2f})")
                else:
                    print(f"  Shape is null")
            except Exception as e:
                print(f"  Error: {e}")
            print()

        # Now test Part.getShape with subPath only (simulating what Sketcher does)
        print("=" * 50)
        print("Testing Part.getShape with subPath only (Sketcher simulation):")
        print("=" * 50)
        for subPath in ["0.", "1."]:
            print(f"\nTesting with subPath='{subPath}', then getSubShape('Edge1'):")
            try:
                # This simulates what Sketcher does:
                # 1. Get shape with just the path (e.g., "0.")
                # 2. Then extract the element with getSubShape
                refShape = Part.getShape(array, subPath, needSubElement=False, transform=True)
                if not refShape.isNull():
                    bb = refShape.BoundBox
                    print(f"  refShape BoundBox center: ({bb.Center.x:.2f}, {bb.Center.y:.2f}, {bb.Center.z:.2f})")
                    # Now get Edge1 from this shape
                    try:
                        edge = refShape.getElement("Edge1")
                        param = (edge.FirstParameter + edge.LastParameter) / 2
                        pt = edge.valueAt(param)
                        print(f"  Edge1 midpoint: ({pt.x:.2f}, {pt.y:.2f}, {pt.z:.2f})")
                    except Exception as e:
                        print(f"  Could not get Edge1: {e}")
                else:
                    print("  refShape is null")
            except Exception as e:
                print(f"  Error: {e}")

        # Compare with full subname
        print("\n" + "=" * 50)
        print("Compare: Part.getShape with FULL subname (correct way):")
        print("=" * 50)
        for subname in ["0.Edge1", "1.Edge1"]:
            print(f"\nTesting with full subname='{subname}':")
            try:
                shape = Part.getShape(array, subname, needSubElement=True, transform=True)
                if not shape.isNull():
                    if shape.ShapeType == "Edge":
                        param = (shape.FirstParameter + shape.LastParameter) / 2
                        pt = shape.valueAt(param)
                        print(f"  Edge midpoint: ({pt.x:.2f}, {pt.y:.2f}, {pt.z:.2f})")
                else:
                    print("  Shape is null")
            except Exception as e:
                print(f"  Error: {e}")

        # Check what's stored in any sketch's ExternalGeometry that references the array
        print("\n" + "=" * 50)
        print("Checking Sketch ExternalGeometry references:")
        print("=" * 50)
        for obj in doc.Objects:
            if obj.TypeId == "Sketcher::SketchObject":
                if hasattr(obj, 'ExternalGeometry'):
                    extGeo = obj.ExternalGeometry
                    if extGeo:
                        print(f"\nSketch '{obj.Name}' ExternalGeometry:")
                        for i, (refObj, subs) in enumerate(extGeo):
                            objName = refObj.Name if refObj else "None"
                            print(f"  [{i}] Object: {objName}, SubElements: {subs}")
