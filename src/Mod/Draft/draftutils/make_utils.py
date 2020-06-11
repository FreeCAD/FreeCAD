# ***************************************************************************
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides utility functions for make_polarray and make_circulararray."""
## @package make_utils
# \ingroup DRAFT
# \brief This module provides functions for make_polarray & make_circulararray.

import FreeCAD as App
import Part

import draftutils.utils as utils

from draftutils.messages import _msg, _err
from draftutils.translate import _tr


def make_polcirc_shared(_name, center, axis_object, axis_edge):
    """Typecheck center, axis_object and axis_edge. Return axis_reference if
    axis_object and axis_edge (optional) are set and correct. Otherwise return
    None.

    This is the shared functionality of make_polar_array and make_circulararray.
    """
    CORRECT = True
    axis_reference = None

    _msg("center: {}".format(center))
    try:
        utils.type_check([(center, App.Vector)], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a vector."))
        return not CORRECT, axis_reference

    _msg("axis_object: {}".format(axis_object))
    _msg("axis_edge: {}".format(axis_edge))
    axis = axis_object
    if axis_object and isinstance(axis_object, str):
        found, axis = utils.find_object(axis_object,
                                        doc=App.activeDocument())
        if not found:
            _err(_tr(
                "Wrong input. Given axis_name does not refer to an "
                "existing DocumentObject."))
            return not CORRECT, axis_reference
    if axis:
        axis_edge_name = axis_edge
        if axis_edge:
            if isinstance(axis_edge, str):
                print(axis)
                print(axis_edge)
                edge_object = axis.getSubObject(axis_edge)
            elif isinstance(axis_edge, int):
                axis_edge_name = "Edge" + str(axis_edge)
                edge_object = axis.getSubObject(axis_edge_name)
            else:
                _err(_tr(
                    "Wrong input. Given axis_edge has to be of type int or str."
                ))
                return not CORRECT, axis_reference
        else:
            if not (hasattr(axis, "Shape") and hasattr(axis.Shape, "Edges")):
                _err(_tr("Wrong input: axis_object cannot be used for Axis"
                         " Reference, it lacks a Shape with Edges."))
                return not CORRECT, axis_reference
            axis_edge_name = "Edge1"  # default if axis_edge is missing
            edge_object = axis.getSubObject(axis_edge_name)
        if not edge_object:
            _err(_tr(
                "Wrong input. Given axis_edge does not refer to a "
                "SubObject of axis_object or given axis_object lacks"
                "having edges."))
            return not CORRECT, axis_reference
        if not isinstance(edge_object, Part.Edge):
            _err(_tr(
                "Wrong input. Given axis_edge does not refer to a "
                "SubObject of type Part.Edge"))
            return not CORRECT, axis_reference
        if not isinstance(edge_object.Curve, Part.Line):
            _err(_tr(
                "Wrong input. Given axis_edge does not refer to a "
                "SubObject with Curve of type Part.Line"))
            return not CORRECT, axis_reference

    if axis_object:
        axis_reference = [axis, axis_edge_name]
        _msg("axis_reference: {}".format(axis_reference))
        return CORRECT, axis_reference
    return CORRECT, axis_reference
