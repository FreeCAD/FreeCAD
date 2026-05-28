# SPDX-License-Identifier: LGPL-2.1-or-later
"""Shared constants for the Copilot module."""

# Maximum image size (bytes) for reference images
MAX_IMAGE_SIZE_BYTES = 8 * 1024 * 1024

# Color palette (RGB tuples, 0.0-1.0)
COLORS = {
    "red": (1.0, 0.0, 0.0),
    "green": (0.0, 0.8, 0.0),
    "blue": (0.0, 0.2, 1.0),
    "yellow": (1.0, 0.9, 0.0),
    "orange": (1.0, 0.45, 0.0),
    "white": (1.0, 1.0, 1.0),
    "black": (0.0, 0.0, 0.0),
    "gray": (0.45, 0.45, 0.45),
    "grey": (0.45, 0.45, 0.45),
}

# Default primitive dimensions
DEFAULTS = {
    "box_size": 10.0,
    "cylinder_radius": 5.0,
    "cylinder_height": 10.0,
    "sphere_radius": 5.0,
    "cone_radius1": 5.0,
    "cone_radius2": 0.0,
    "cone_height": 10.0,
    "torus_radius1": 10.0,
    "torus_radius2": 2.0,
    "rotation_angle": 90.0,
    "scale_factor": 1.0,
}

# Allowed actions for plan validation
ALLOWED_ACTIONS = {
    "create_box",
    "create_cylinder",
    "create_sphere",
    "create_cone",
    "create_torus",
    "move_selected",
    "rotate_selected",
    "scale_selected",
    "select_by_name",
    "delete_selected",
    "set_color",
    "boolean_fuse",
    "boolean_cut",
    "fit_view",
    "save",
    "open",
}
