# SPDX-License-Identifier: LGPL-2.1-or-later
"""Representation constraints enforced at operation boundaries, including Python calls."""


def has_local_topology(obj):
    return bool(getattr(obj, "TMeshData", "") or getattr(obj, "LocalEdgeInserts", ()))


def require_base_topology(obj, operation):
    if has_local_topology(obj) or getattr(obj, "DissolvedEdges", ()):
        raise ValueError(
            f"{operation} requires a base control cage; existing local edits would be lost"
        )


def validate_local_creases(obj, edges, sharpness):
    # A smooth fitted root patch cannot represent a sharp internal trim seam.
    # Reject before storing a crease instead of reporting a successful no-op.
    if float(sharpness) <= 0 or not has_local_topology(obj):
        return
    from .topology import cage_edges
    faces = [tuple(map(int, str(face).split())) for face in obj.ControlFaces]
    base_edges = set(cage_edges(faces))
    if any(tuple(sorted(edge)) not in base_edges for edge in edges):
        raise ValueError("Creases on local subdivision edges are not supported by the surface fitter")
