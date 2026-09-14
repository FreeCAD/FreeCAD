# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Tilted work plane declarations for the post-processor.

An operation on a work plane stores its path in the plane's own frame and
carries the plane as its Placement. A control with tilted-work-plane support
takes that frame directly: the program declares the plane's origin and
orientation, the control positions the rotary axes and applies its pivot
offsets, and the path follows in plane coordinates unchanged.

What differs between controls is how the orientation is spelled. This module
turns a FreeCAD Rotation into each control's angle convention and holds the
line templates for declaring, aligning to and cancelling a plane. Templates
are format strings; the post fills them with numbers it has already formatted
in its own units and precision, so a post script can replace any of them
through its ``values`` without knowing the angle math.

Angle conventions, verified against composed rotations:

``G68.2`` / ``G268`` (Fanuc, Haas and the controls that copy them) take
I, J, K as intrinsic Euler angles: rotate about Z by I, then about the new X
by J, then about the newest Z by K. That is ``Rz(I) * Rx(J) * Rz(K)``, which
FreeCAD spells ``IZXZ``.

``PLANE SPATIAL`` (Heidenhain) takes SPA, SPB, SPC as spatial angles about
the fixed machine X, Y and Z, applied in that order: ``Rz(C) * Ry(B) * Rx(A)``,
which FreeCAD spells ``IZYX`` and returns as (C, B, A).

A plane parallel to the table is the singular case for both: only the sum of
the first and last angle is defined. FreeCAD puts it all in one of them, which
every control accepts.
"""

import FreeCAD

from Machine.models.machine import PlaneCommand


def euler_zxz(rotation):
    """(I, J, K) in degrees for G68.2 / G268: intrinsic Z, X', Z''."""
    return tuple(_clean(a) for a in rotation.toEulerAngles("IZXZ"))


def spatial_abc(rotation):
    """(SPA, SPB, SPC) in degrees for PLANE SPATIAL: about fixed X, then Y, then Z."""
    c, b, a = rotation.toEulerAngles("IZYX")
    return (_clean(a), _clean(b), _clean(c))


def _clean(angle):
    """Drop the -0.0 and the 1e-15 that a decomposition leaves behind."""
    angle = round(float(angle), 9)
    return 0.0 if angle == 0 else angle


# One entry per PlaneCommand. ``angles`` maps a Rotation to the three angles
# in the order the template names them; the templates take the plane origin
# as x, y, z and the angles as a1, a2, a3, all pre-formatted strings, and the
# retract height as z. ``align`` is what makes the tool axis follow the
# declared plane on a control that positions the rotaries itself.
DIALECTS = {
    PlaneCommand.G68_2: {
        "angles": euler_zxz,
        "declare": "G68.2 X{x} Y{y} Z{z} I{a1} J{a2} K{a3}",
        "align": "G53.1",
        "cancel": "G69",
        "retract": "G53 G0 Z{z}",
    },
    PlaneCommand.G268: {
        "angles": euler_zxz,
        "declare": "G268 X{x} Y{y} Z{z} I{a1} J{a2} K{a3}",
        "align": "G53.1",
        "cancel": "G269",
        "retract": "G53 G0 Z{z}",
    },
    PlaneCommand.PLANE_SPATIAL: {
        "angles": spatial_abc,
        "declare": "PLANE SPATIAL SPA{a1} SPB{a2} SPC{a3} TURN FMAX",
        "align": "",
        "cancel": "PLANE RESET STAY",
        "retract": "L Z{z} R0 FMAX M91",
    },
}


def plane_angles(plane_command, rotation):
    """The three angles the control wants for this rotation, in its order."""
    return DIALECTS[plane_command]["angles"](rotation)


def template(plane_command, key, override=None):
    """A line template for the dialect, unless the post supplied its own.

    ``override`` is the post's ``values`` entry for the key: None means use
    the dialect's line, a string replaces it (an empty string suppresses it).
    """
    if override is not None:
        return override
    return DIALECTS[plane_command][key]
