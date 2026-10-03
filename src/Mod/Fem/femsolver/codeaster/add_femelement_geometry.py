# ***************************************************************************
# *   Copyright (c) 2024 Tim Swait <timswait@gmail.com>                     *
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

__title__ = "Code Aster add femelement geometry"
__author__ = "Tim Swait"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import FreeCAD
from .codeasterutils import Layup


def add_femelement_geometry(commtxt, ele_name, ca_writer):
    """Function to add elements to Code Aster input file, currently only supports shell elements"""
    mat_objs = ca_writer.mat_objs
    layups = []
    commtxt += "# Geometric properties of element\n"
    if ca_writer.member.geos_beamsection:
        FreeCAD.Console.PrintError("Beams not yet supported for Code Aster\n")

    elif ca_writer.member.geos_shelllaminate:
        commtxt, layups = add_shell_laminate(commtxt, mat_objs, ele_name, ca_writer)

    elif ca_writer.member.geos_shellthickness:
        commtxt, layups = add_shell2D(commtxt, mat_objs, ele_name, ca_writer)
    return commtxt, layups


def add_shell2D(commtxt, mat_objs, ele_name, ca_writer):
    """Adds a 2D element geometry type shell"""
    thicknesses = []
    geomses = []
    layups = []
    cardname = mat_objs[0].Material["CardName"]
    cardname = cardname.replace(" ", "")
    cardname = cardname.replace("-", "_")
    i = 0
    for shellth in ca_writer.member.geos_shellthickness:
        shellth_obj = shellth["Object"]
        thickness = shellth_obj.Thickness.getValueAs("mm").Value
        thicknesses.append(thickness)
        geoms = []
        if len(shellth_obj.References) == 0:
            femmesh = ca_writer.mesh_object.FemMesh
            for g in femmesh.Groups:
                if femmesh.getGroupElementType(g) == "Face":
                    geoms.append(femmesh.getGroupName(g))
        else:
            for ref in shellth_obj.References:
                for geom in ref[1]:
                    geoms.append(geom)
        geomses.append(geoms)

        if "YoungsModulusX" in mat_objs[0].Material.keys():
            LUname = cardname + "LAYUP" + str(i)
            i += 1
            # TODO: update these layups to use the new object
            layup = Layup(
                name=LUname,
                groups=[str(geoms)[1:-1]],
                matnames=[cardname],
                thicknesses=[thickness],
                orientations=[0],
            )
            layups.append(layup)
            commtxt += add_layup(layup)

        else:
            layups = [
                Layup(
                    name=cardname,
                    groups=[str(geoms)[1:-1]],
                    matnames=[cardname],
                    thicknesses=[thickness],
                    orientations=[0],
                )
            ]

        commtxt += f"# Shell elements detected, thickness {thickness}mm on item {geoms}\n"
    commtxt += f"{ele_name} = AFFE_CARA_ELEM(COQUE=(\n"
    for geoms, thickness in zip(geomses, thicknesses):
        commtxt += f"                                   _F(EPAIS={thickness},\n"
        commtxt += f"                                      GROUP_MA=({str(geoms)[1:-1]})),\n"
    commtxt += "                                  ),\n"
    commtxt += "                          MODELE=model)\n\n"

    FreeCAD.Console.PrintMessage(f"Shell of thickness {thickness}mm added.\n")
    return commtxt, layups


def add_shell_laminate(commtxt, mat_objs, ele_name, ca_writer):
    """Adds a shell laminate type object"""

    layups = []
    LU_id = 0
    for shelllam in ca_writer.member.geos_shelllaminate:
        shelllam_obj = shelllam["Object"]
        thicknesses = shelllam_obj.Thicknesses
        orientations = shelllam_obj.Orientations
        assert len(thicknesses) == len(
            orientations
        ), f"{len(thicknesses)} ply thicknesses given, {len(orientations)} orientation angles given, these should match (i.e provide one thickness and one angle for every ply"
        orientations = angle_correcter(orientations)

        if len(shelllam_obj.Windall["elements"]) == 0:
            commtxt, layup = apply_con_layup(
                commtxt, shelllam_obj, ele_name, mat_objs, LU_id, ca_writer
            )
            layups.append(layup)
        else:
            commtxt, layups = apply_vari_layup(commtxt, shelllam_obj, ele_name, mat_objs, ca_writer)
        LU_id += 1
    return commtxt, layups


def apply_con_layup(commtxt, shelllam_obj, ele_name, mat_objs, LU_id, ca_writer):
    """Applies a constant layup over the whole part"""
    FreeCAD.Console.PrintMessage("Overwriting materials list\n")
    thicknesses = shelllam_obj.Thicknesses
    orientations = shelllam_obj.Orientations
    matnames = shelllam_obj.Materials
    if len(matnames) == 0:
        assert (
            len(mat_objs) == 1
        ), "If Materials are not specified in Shell Laminate Geometry object then there must be only one Material specified in analysis"
        cardname = mat_objs[0].Material["CardName"]
        # Remove any spaces and replace hyphens as these will cause CA to fail
        cardname = cardname.replace(" ", "")
        cardname = cardname.replace("-", "_")
        FreeCAD.Console.PrintMessage(f"Single material, {cardname}, applied to all plies\n")
        for i in range(len(thicknesses)):
            matnames.append(cardname)
    elif len(matnames) == len(thicknesses):
        FreeCAD.Console.PrintMessage("Multiple materials applied to each ply\n")
        matnames = make_mat_list(mat_objs, matnames, True)
    else:
        raise Exception(
            "Number of plies in materials list not equal to number of plies in thickness list"
        )

    shelllam_obj.Materials = matnames
    geoms = make_geom_list(shelllam_obj, ca_writer)
    matname = "LAYUP" + str(LU_id)
    layup = Layup(
        name=matname,
        groups=geoms,
        matnames=matnames,
        thicknesses=thicknesses,
        orientations=orientations,
    )
    commtxt += add_layup(layup)
    ori_vec = shelllam_obj.Orientation
    commtxt += add_laminate([layup], ele_name, ori_vec)
    return commtxt, layup


def apply_vari_layup(commtxt, shelllam_obj, ele_name, mat_objs, ca_writer):
    """Apply a layup which varies across elements"""
    FreeCAD.Console.PrintMessage("Applying variable layup\n")
    thicknesses = shelllam_obj.Thicknesses
    orientations = shelllam_obj.Orientations
    matnames = make_mat_list(mat_objs)
    commtxt += "# WindAll object detected\n"
    geoms, faces = make_geom_list(shelllam_obj, ca_writer, True)
    baselayup = Layup(
        name="base",
        groups=geoms,
        matnames=[matnames[0]],
        thicknesses=[thicknesses[0]],
        orientations=[orientations[0]],
    )

    layups = [baselayup]
    ori_vec = shelllam_obj.Orientation
    commtxt += add_layup(baselayup)
    for e, t, o, mn in zip(
        shelllam_obj.Windall["elements"],
        shelllam_obj.Windall["thicknesslists"],
        shelllam_obj.Windall["orientationlists"],
        shelllam_obj.Windall["materialslists"],
    ):
        o = angle_correcter(o)
        fi = None
        for i in range(len(faces)):
            if e in faces[i]:
                fi = i
        if fi is None:
            FreeCAD.Console.PrintWarning(f"Element number {e} not in any group\n")
        # Corrects eid to Code Aster numbering
        eid_CA = e - faces[fi][0] + 1
        luname = f"L{fi}U{eid_CA}"
        if len(str(fi)) > 1:
            FreeCAD.Console.PrintWarning(
                "Number of groups in double digits, group assignment maybe unreliable\n"
            )
        layup = Layup(
            name=luname,
            groups=[luname],
            ninit=eid_CA,
            nfin=eid_CA,
            parentgroup=geoms[fi],
            matnames=mn,
            thicknesses=t,
            orientations=o,
        )
        layups.append(layup)
        commtxt += add_layup(layup)
    commtxt += add_grps(layups)
    commtxt += add_laminate(layups, ele_name, ori_vec)
    return commtxt, layups


def add_grps(layups, group=0):
    """
    Add groups for the different layups, assume using first groupname
    """

    commtxt = "# Adding WindAll groups\n"
    commtxt += "grps = DEFI_GROUP(MAILLAGE=mesh, CREA_GROUP_MA = (\n"
    for layup in layups[1:]:
        commtxt += f"                                                  _F(GROUP_MA = ('{layup.parentgroup}',),\n"
        commtxt += (
            f"                                                     NUME_INIT = {layup.ninit},\n"
        )
        commtxt += (
            f"                                                     NUME_FIN = {layup.nfin},\n"
        )
        commtxt += f"                                                     NOM = '{layup.name}'),\n"
    commtxt += "                                                     ))\n\n"
    return commtxt


def add_layup(layup):
    commtxt = "# Composite layup detected, added to shell\n"
    name = layup.name
    commtxt += f"{name} = DEFI_COMPOSITE(COUCHE=(\n"
    for th, o, m in zip(layup.thicknesses, layup.orientations, layup.matnames):
        commtxt += f"                               _F(EPAIS={th},\n"
        commtxt += f"                                MATER={m},\n"
        commtxt += f"                                ORIENTATION = {o}),\n"
    commtxt += "                                ))\n\n"
    return commtxt


def add_laminate(layups, ele_name, ori_vec):
    ori_vec_str = f"({ori_vec.x}, {ori_vec.y}, {ori_vec.z})"
    commtxt = "# Shell elements detected, applying composite laminate definition\n"
    commtxt += f"{ele_name} = AFFE_CARA_ELEM(COQUE=(\n"
    for layup in layups:
        thicknesses, groups = layup.thicknesses, layup.groups
        if len(groups) == 1:
            group = groups[0]
        else:
            group = str(groups)[2:-2]
        thicktot = sum(thicknesses)
        commtxt += f"                                _F(COQUE_NCOU = {len(thicknesses)},\n"
        commtxt += f"                                   EPAIS={thicktot},\n"
        commtxt += f"                                   GROUP_MA=('{group}'),\n"
        commtxt += f"                                   VECTEUR={ori_vec_str}),\n"
    commtxt += "                          ),\n"
    commtxt += "                          MODELE=model)\n\n"
    return commtxt


def make_geom_list(shelllam_obj, ca_writer, getFaces=False):
    geoms = []
    faces = []
    femmesh = ca_writer.mesh_object.FemMesh
    if len(shelllam_obj.References) == 0:
        for g in femmesh.Groups:
            if femmesh.getGroupElementType(g) == "Face":
                geoms.append(femmesh.getGroupName(g))
                if getFaces:
                    faces.append(femmesh.getGroupElements(g))
    else:
        for ref in shelllam_obj.References:
            for geom in ref[1]:
                geoms.append(geom)
        if getFaces:
            for g in femmesh.Groups:
                if femmesh.getGroupElementType(g) == "Face":
                    faces.append(femmesh.getGroupElements(g))
    if getFaces:
        return geoms, faces
    else:
        return geoms


def make_mat_list(mat_objs, matnames=[], check=False):
    mat_obj_names = []
    for mo in mat_objs:
        cardname = mo.Material["CardName"]
        cardname = cardname.replace(" ", "")
        cardname = cardname.replace("-", "_")
        mat_obj_names.append(cardname)
    mns = []
    if check:
        for mn in matnames:
            mn = mn.replace(" ", "")
            mn = mn.replace("-", "_")
            if mn not in mat_obj_names:
                raise IndexError(
                    f"Material named {mn} is in ply materials list but is not present in analysis"
                )
            mns.append(mn)
        matnames = mns
    else:
        matnames = mat_obj_names

    return matnames


def angle_correcter(anglistIP):
    """
    Function to take a list of angles outside of Code Aster's allowables and
    convert to equivalent in range angle
    """
    anglistOP = []
    for ang in anglistIP:
        while not (-90 <= ang <= 90):
            if ang < -90:
                ang += 180
            if ang > 90:
                ang -= 180
        anglistOP.append(ang)
    return anglistOP


##  @}
