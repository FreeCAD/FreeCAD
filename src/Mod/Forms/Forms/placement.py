# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

"""Placement helpers for Forms objects and linked reference geometry."""


def global_placement(obj):
    """Return *obj*'s placement in its unique document-tree context.

    ``GeoFeature.getGlobalPlacement`` does not account for Links and is being
    removed from FreeCAD. ``getGlobalPlacementOf`` needs the selection root
    and path explicitly, which are available through ``Parents`` for the
    normal single-parent Forms workflows.
    """
    parents = getattr(obj, "Parents", ())
    if len(parents) == 1 and len(parents[0]) == 2:
        root, subname = parents[0]
    else:
        root, subname = obj, ""
    try:
        return obj.getGlobalPlacementOf(obj, root, str(subname))
    except (AttributeError, RuntimeError, TypeError):
        # Keep documents usable on FreeCAD versions predating the replacement
        # API. This fallback can be removed with FreeCAD's deprecated method.
        try:
            return obj.getGlobalPlacement()
        except (AttributeError, RuntimeError):
            return obj.Placement


__all__ = ["global_placement"]
