# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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
__url__ = "https://www.freecad.org"


from . import femutils


def get_member(analysis, t):
    """ Return list of all members of *analysis* of type *t*.

    Search *analysis* for members of type *t*. This method checks the custom
    python typesystem (BaseType class property) used by the Fem module if
    possible. If the object does not use the python typesystem the usual
    isDerivedFrom from the C++ dynamic type system is used.

    :param analysis: only objects part of this analysis are considered
    :param t:        only objects of this type are returned

    :note:
     Inheritance of Fem types is not checked. If *obj* uses Fems typesystem the
     type is just checked for equality. If the type doesn't match
     ``obj.isDerivedFrom`` is called as usual. See
     https://forum.freecad.org/viewtopic.php?f=10&t=32625
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
    found. This method checks the custom python typesystem (BaseType class
    property) used by the Fem module if possible. If the object doesn't use the
    python typesystem the usual isDerivedFrom from the C++ dynamic type system
    is used.

    :param analysis: only objects part of this analysis are considered
    :param t:        only a object of this type is returned

    :note:
     Inheritance of Fem types is not checked. If *obj* uses Fems typesystem the
     type is just checked for equality. If the type doesn't match
     ``obj.isDerivedFrom`` is called as usual. See
     https://forum.freecad.org/viewtopic.php?f=10&t=32625
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
     https://forum.freecad.org/viewtopic.php?f=10&t=32625
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
        if (
            m.isDerivedFrom("Fem::FemMeshObject")
            # the next line should not be needed as the result mesh is not a analysis member
            and not femutils.is_of_type(m, "Fem::MeshResult")
        ):
            if not mesh_to_solve:
                mesh_to_solve = m
            else:
                return (None, "FEM: multiple mesh in analysis not yet supported!")
    if mesh_to_solve is not None:
        return (mesh_to_solve, "")
    else:
        return (None, "FEM: no mesh object found in analysis.")


class AnalysisMember():

    def __init__(self, analysis):
        self.analysis = analysis
        """
        # members of the analysis. All except solvers and the mesh

        materials:
        materials_linear : list of dictionaries
            list of nonlinear materials from the analysis.
            [{"Object":materials_linear}, {}, ...]

        materials_nonlinear : list of dictionaries
            list of nonlinear materials from the analysis.
            [{"Object":materials_nonlinear}, {}, ...]

        geometries:
        beam_sections : list of dictionaries
            list of beam sections from the analysis.
            [{"Object":beam_section_obj, "xxxxxxxx":value}, {}, ...]

        beam_rotations :  list of dictionaries
            list of beam rotations from the analysis.
            [{"Object":beam_rotation_obj, "xxxxxxxx":value}, {}, ...]

        fluid_sections : list of dictionaries
            list of fluid sections from the analysis.
            [{"Object":fluid_section_obj, "xxxxxxxx":value}, {}, ...]

        shell_thicknesses : list of dictionaries
            list of shell thicknesses from the analysis.
            [{"Object":shell_thickness_obj, "xxxxxxxx":value}, {}, ...]

        constraints:
        constraints_centrif : list of dictionaries
            list of centrifs for the analysis.
            [{"Object":centrif_obj, "xxxxxxxx":value}, {}, ...]

        constraints_contact : list of dictionaries
            list of contact constraints from the analysis.
            [{"Object":contact_obj, "xxxxxxxx":value}, {}, ...]

        constraints_displacement : list of dictionaries
            list of displacements for the analysis.
            [{"Object":displacement_obj, "xxxxxxxx":value}, {}, ...]

        constraints_fixed :  list of dictionaries
            list of fixed constraints from the analysis.
            [{"Object":fixed_obj, "NodeSupports":bool}, {}, ...]

        constraints_rigidbody : list of dictionaries
            list of displacements for the analysis.
            [{"Object":rigidbody_obj, "xxxxxxxx":value}, {}, ...]

        constraints_force : list of dictionaries
            list of force constraints from the analysis.
            [{"Object":force_obj, "NodeLoad":value}, {}, ...

        constraints_heatflux : list of dictionaries
            list of heatflux constraints for the analysis.
            [{"Object":heatflux_obj, "xxxxxxxx":value}, {}, ...]

        constraints_initialtemperature : list of dictionaries
            list of initial temperatures for the analysis.
            [{"Object":initialtemperature_obj, "xxxxxxxx":value}, {}, ...]

        constraints_planerotation : list of dictionaries
            list of plane rotation constraints from the analysis.
            [{"Object":planerotation_obj, "xxxxxxxx":value}, {}, ...]

        constraints_pressure : list of dictionaries
            list of pressure constraints from the analysis.
            [{"Object":pressure_obj, "xxxxxxxx":value}, {}, ...]

        constraints_sectionprint : list of dictionaries
            list of sectionprints for the analysis.
            [{"Object":sectionprint_obj, "xxxxxxxx":value}, {}, ...]

        constraints_selfweight : list of dictionaries
            list of selfweight constraints from the analysis.
            [{"Object":selfweight_obj, "xxxxxxxx":value}, {}, ...]

        constraints_temperature : list of dictionaries
            list of temperatures for the analysis.
            [{"Object":temperature_obj, "xxxxxxxx":value}, {}, ...]

        constraints_tie : list of dictionaries
            list of ties for the analysis.
            [{"Object":tie_obj, "xxxxxxxx":value}, {}, ...]

        constraints_transform : list of dictionaries
            list of transform constraints from the analysis.
            [{"Object":transform_obj, "xxxxxxxx":value}, {}, ...]
        """

        # get member
        # constants
        self.cota_vacuumpermittivity = self.get_several_member(
            "Fem::ConstantVacuumPermittivity"
        )

        # materials
        std_mats = self.get_several_member(
            "Fem::MaterialCommon"
        )
        rei_mats = self.get_several_member(
            "Fem::MaterialReinforced"
        )
        self.mats_linear = std_mats + rei_mats

        self.mats_nonlinear = self.get_several_member(
            "Fem::MaterialMechanicalNonlinear"
        )

        # geometries
        self.geos_beamsection = self.get_several_member(
            "Fem::ElementGeometry1D"
        )
        self.geos_beamrotation = self.get_several_member(
            "Fem::ElementRotation1D"
        )
        self.geos_fluidsection = self.get_several_member(
            "Fem::ElementFluid1D"
        )
        self.geos_shellthickness = self.get_several_member(
            "Fem::ElementGeometry2D"
        )

        # constraints
        self.cons_centrif = self.get_several_member(
            "Fem::ConstraintCentrif"
        )
        self.cons_contact = self.get_several_member(
            "Fem::ConstraintContact"
        )
        self.cons_displacement = self.get_several_member(
            "Fem::ConstraintDisplacement"
        )
        self.cons_fixed = self.get_several_member(
            "Fem::ConstraintFixed"
        )
        self.cons_rigidbody = self.get_several_member(
            "Fem::ConstraintRigidBody"
        )
        self.cons_force = self.get_several_member(
            "Fem::ConstraintForce"
        )
        self.cons_heatflux = self.get_several_member(
            "Fem::ConstraintHeatflux"
        )
        self.cons_initialtemperature = self.get_several_member(
            "Fem::ConstraintInitialTemperature"
        )
        self.cons_planerotation = self.get_several_member(
            "Fem::ConstraintPlaneRotation"
        )
        self.cons_pressure = self.get_several_member(
            "Fem::ConstraintPressure"
        )
        self.cons_sectionprint = self.get_several_member(
            "Fem::ConstraintSectionPrint"
        )
        self.cons_selfweight = self.get_several_member(
            "Fem::ConstraintSelfWeight"
        )
        self.cons_temperature = self.get_several_member(
            "Fem::ConstraintTemperature"
        )
        self.cons_tie = self.get_several_member(
            "Fem::ConstraintTie"
        )
        self.cons_transform = self.get_several_member(
            "Fem::ConstraintTransform"
        )

    def get_several_member(self, t):
        return get_several_member(self.analysis, t)
