#!/usr/bin/env python3
"""Small repeatable benchmark for the lazy Part.ShapeList accessors.

Run with the FreeCAD command-line executable, for example:

    FreeCADCmd -c "exec(open('tools/profile/shape_list_benchmark.py').read())"

The setup is intentionally outside the timed sections. The shape is a
compound of 1000 boxes, giving 6000 faces, matching the scale used by the
original ShapeList port benchmark.
"""

import statistics
import time

import FreeCAD as App
import Part


BOX_COUNT = 1000
REPEATS = 5


def measure(label, operation, iterations, repeats=REPEATS):
    samples = []
    for _ in range(repeats):
        start = time.perf_counter_ns()
        for _ in range(iterations):
            operation()
        elapsed = time.perf_counter_ns() - start
        samples.append(elapsed / iterations / 1000.0)
    print(f"{label:18} median {statistics.median(samples):10.3f} us", flush=True)


def main():
    boxes = [
        Part.makeBox(1, 1, 1, App.Vector(index % 50 * 2, index // 50 * 2, 0))
        for index in range(BOX_COUNT)
    ]
    shape = Part.makeCompound(boxes)
    expected_faces = BOX_COUNT * 6
    if len(shape.Faces) != expected_faces:
        raise RuntimeError(f"expected {expected_faces} faces, got {len(shape.Faces)}")

    faces = shape.Faces
    faces[0]

    def first_mutation():
        shape.Faces.append(boxes[0].Faces[0])

    def full_iteration():
        return sum(1 for _ in shape.Faces)

    def list_retained_view():
        retained = shape.Faces
        retained[0]
        return list(retained)

    print(f"compound: {BOX_COUNT} boxes, {expected_faces} faces", flush=True)
    measure("len(s.Faces)", lambda: len(shape.Faces), 100)
    measure("s.Faces[0]", lambda: shape.Faces[0], 100)
    measure("len(faces)", lambda: len(faces), 1000)
    measure("faces[0]", lambda: faces[0], 1000)
    measure("list(faces)", list_retained_view, 1, repeats=3)
    measure("full iteration", full_iteration, 1, repeats=3)
    # Appending to a fresh view is the expensive transition from the lazy
    # cache-backed view to the authoritative Python list.
    measure("first mutation", first_mutation, 1, repeats=3)


if __name__ == "__main__":
    main()
