# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
""" Collection of functions for the Fem module.

This module contains function for managing a analysis and all the different
types of objects it contains, helper for executing a simulation.
"""


__title__ = "FEM analysis tools"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


from . import femutils


def get_member(analysis, t):
    """ Return list of all members of *analysis* of type *t*.

    Search *analysis* for members of type *t*. This method checks the custom
    python typesytem (BaseType class property) used by the Fem module if
    possible. If the object does not use the python typesystem the usual
    isDerivedFrom from the C++ dynamic type system is used.

    :param analysis: only objects part of this analysis are considered
    :param t:        only objects of this type are returned

    :note:
     Inheritance of Fem types is not checked. If *obj* uses Fems typesystem the
     type is just checked for equality. If the type doesn't match
     ``obj.isDerivedFrom`` is called as usual. See
     https://forum.freecadweb.org/viewtopic.php?f=10&t=32625
    """
    if analysis is None:
        raise ValueError("Analysis must not be None")
    matching = []
    for m in analysis.Group:
        # since is _derived_from is used the father could be used
        # to test too (ex. "Fem::FemMeshObject")
        if femutils.is_derived_from(m, t):
            matching.append(m)
    return matching


def get_single_member(analysis, t):
    """ Return one object of type *t* and part of *analysis*.

    Search *analysis* for members of type *t* and return the first one that's
    found. This method checks the custom python typesytem (BaseType class
    property) used by the Fem module if possible. If the object doesn't use the
    python typesystem the usual isDerivedFrom from the C++ dynamic type system
    is used.

    :param analysis: only objects part of this analysis are considered
    :param t:        only a object of this type is returned

    :note:
     Inheritance of Fem types is not checked. If *obj* uses Fems typesystem the
     type is just checked for equality. If the type doesn't match
     ``obj.isDerivedFrom`` is called as usual. See
     https://forum.freecadweb.org/viewtopic.php?f=10&t=32625
    """
    objs = get_member(analysis, t)
    return objs[0] if objs else None


def get_several_member(analysis, t):
    """ Get members and pack them for Calculix/Z88.

    Collect members by calling :py:func:`get_member` and pack them into a
    data structure that can be consumed by calculix and Z88 solver modules.

    :param analysis: see :py:func:`get_member`
    :param t: see :py:func:`get_member`

    :returns:
     A list containing one dict per member. Each dict has two entries:
     ``"Object"`` and ``"RefShapeType"``. ``dict["Object"]`` contains the
     member document object. ``dict["RefShapeType"]`` contains the shape type
     of the *References* property of the member (used by constraints) as a
     string ("Vertex", "Edge", "Face" or "Solid"). If the member doesn't have a
     *References* property ``dict["RefShapeType"]`` is the empty string ``""``.

    :note:
     Undefined behaviour if one of the members has a *References* property
     which is empty.

    :note:
     Undefined behaviour if the type of the references of one object are not
     all the same.

    :note:
     Inheritance of Fem types is not checked. If *obj* uses Fems typesystem the
     type is just checked for equality. If the type doesn't match
     ``obj.isDerivedFrom`` is called as usual. See
     https://forum.freecadweb.org/viewtopic.php?f=10&t=32625
    """
    # if no member is found, an empty list is returned
    objs = get_member(analysis, t)
    members = []
    for m in objs:
        obj_dict = {}
        obj_dict["Object"] = m
        obj_dict["RefShapeType"] = femutils.get_refshape_type(m)
        members.append(obj_dict)
    return members


def get_mesh_to_solve(analysis):
    """ Find one and only mesh object of *analysis*.

    :returns:
     A tuple ``(object, message)``. If and only if the analysis contains
     exactly one mesh object the first value of the tuple is the mesh document
     object. Otherwise the first value is ``None`` and the second value is a
     error message indicating what went wrong.
    """
    mesh_to_solve = None
    for m in analysis.Group:
        if m.isDerivedFrom("Fem::FemMeshObject") and not femutils.is_of_type(m, "Fem::FemMeshResult"):
            if not mesh_to_solve:
                mesh_to_solve = m
            else:
                return (None, "FEM: multiple mesh in analysis not yet supported!")
    if mesh_to_solve is not None:
        return (mesh_to_solve, "")
    else:
        return (None, "FEM: no mesh object found in analysis.")
