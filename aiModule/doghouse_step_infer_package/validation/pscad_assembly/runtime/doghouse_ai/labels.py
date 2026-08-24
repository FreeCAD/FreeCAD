"""Shared label maps for doghouse point/face datasets."""

from __future__ import annotations

ROLE_TO_ID = {
    "background": 0,
    "doghouse": 1,
    "mount": 2,
    "hole_wall": 3,
    "hole_bottom": 4,
    "transition": 5,
    "root_boundary": 6,
    "non_hole_cylinder": 7,
    "non_hole_fillet": 8,
    "negative_rib": 9,
    "negative_boundary": 10,
    "negative_protrusion": 11,
    "negative_fragment": 12,
}

ID_TO_ROLE = {v: k for k, v in ROLE_TO_ID.items()}

# These roles belong to a doghouse instance, but are hard negatives for more
# specific semantics such as hole_wall or transition.
INTERNAL_HARD_NEGATIVE_ROLES = {
    "non_hole_cylinder",
    "non_hole_fillet",
}

# These roles are outside doghouse instances and are negatives for the doghouse
# binary mask.
NEGATIVE_ROLES = {
    "negative_rib",
    "negative_boundary",
    "negative_protrusion",
    "negative_fragment",
}

FACE_TYPE_TO_ID = {
    "other": 0,
    "plane": 1,
    "cylinder": 2,
    "cone": 3,
    "sphere": 4,
    "torus": 5,
    "bspline": 6,
    "bezier": 7,
}

ID_TO_FACE_TYPE = {v: k for k, v in FACE_TYPE_TO_ID.items()}

