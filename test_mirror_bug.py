"""Test script to reproduce issue #20834 - mirror() regression with Placement.

Run this in FreeCAD Python console to verify the bug.
"""
import FreeCAD as App
import Part

V = App.Vector
Vx = V(1, 0, 0)
Vy = V(0, 1, 0)
Vz = V(0, 0, 1)
VO = V(0, 0, 0)

# Create two identical boxes using different methods:
# 1. Direct coordinates (no explicit Placement)
tiltbox = Part.makeBox(10, 20, 30, V(0, 30, 0), V(0, 1, 1))

# 2. Using explicit Placement
tiltbox_placed = Part.makeBox(10, 20, 30)
tiltbox_placed.Placement = App.Placement(V(0, 30, 0), App.Rotation(-VO, -Vx, V(0, 1, 1), 'YZX'))

# Both boxes should be at the same location
print("Original boxes:")
print(f"  tiltbox BoundBox: {tiltbox.BoundBox}")
print(f"  tiltbox_placed BoundBox: {tiltbox_placed.BoundBox}")

# Mirror both in the XZ plane (normal = Y axis)
tiltbox_mirror = tiltbox.mirror(VO, Vy)
tiltbox_placed_mirror = tiltbox_placed.mirror(VO, Vy)

print("\nMirrored boxes:")
print(f"  tiltbox_mirror BoundBox: {tiltbox_mirror.BoundBox}")
print(f"  tiltbox_placed_mirror BoundBox: {tiltbox_placed_mirror.BoundBox}")

# Check if they match (they should!)
if tiltbox_mirror.BoundBox == tiltbox_placed_mirror.BoundBox:
    print("\n✓ SUCCESS: Both mirrored shapes have the same BoundBox")
else:
    print("\n✗ BUG CONFIRMED: Mirrored shapes differ!")
    print("  The mirror() function incorrectly handles shapes with non-identity Placement.")

# Show them for visual inspection
Part.show(tiltbox, 'Tiltbox')
Part.show(tiltbox_mirror, 'Tiltbox_mirror')
Part.show(tiltbox_placed, 'Tiltbox_placed')
Part.show(tiltbox_placed_mirror, 'Tiltbox_placed_mirror_BAD')

print("\nCheck the 3D view - 'Tiltbox_placed_mirror_BAD' should overlap 'Tiltbox_mirror' but doesn't.")
