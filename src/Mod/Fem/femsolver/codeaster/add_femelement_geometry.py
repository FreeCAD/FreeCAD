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


def add_femelement_geometry(commtxt, ele_name, ca_writer):
    """Function to add elements to Code Aster input file, currently only supports shell elements"""
    mat_objs = ca_writer.mat_objs
    matname = mat_objs[0].Name  # Set default material name for cases where no layup is specified
    lams = []
    commtxt += "# Geometric properties of element\n"
    if ca_writer.member.geos_beamsection:
        FreeCAD.Console.PrintError("Beams not yet supported for Code Aster\n")

    elif ca_writer.member.geos_shelllaminate:
        # only use the first shelllaminate object
        shelllam_obj = ca_writer.member.geos_shelllaminate[0]["Object"]
        thicknesses = shelllam_obj.Thicknesses
        orientations = shelllam_obj.Orientations
        assert len(thicknesses) == len(
            orientations
        ), f"{len(thicknesses)} ply thicknesses given, {len(orientations)} orientation angles given, these should match (i.e provide one thickness and one angle for every ply"
        if len(shelllam_obj.Windall["elements"]) == 0:
            FreeCAD.Console.PrintWarning("Overwriting materials list\n")
            materials = []
            matnames = (
                []
            )  # TODO work out better way of naming materials without needing to reference the object, i.e. use a version of the Card Name condensed to remove spaces
            if len(mat_objs) == 1:
                FreeCAD.Console.PrintMessage(
                    "Single material, {}, applied to all plies\n".format(
                        mat_objs[0].Material["CardName"]
                    )
                )
                for i in range(len(thicknesses)):
                    materials.append(mat_objs[0].Material)
                    matnames.append(mat_objs[0].Name)
            elif len(mat_objs) == len(shelllam_obj.Thicknesses):
                FreeCAD.Console.PrintMessage("Multiple materials applied to each ply\n")
                for mo in mat_objs:
                    materials.append(mo.Material)
                    matnames.append(mo.Name)
            else:
                FreeCAD.Console.PrintWarning(
                    "Number of materials in analysis more than 1 but not equal to number of plies\n"
                )
                for i in range(len(thicknesses)):
                    if i < len(mat_objs) - 1:
                        materials.append(mat_objs[i].Material)
                        matnames.append(mat_objs[i].Name)
                    else:
                        materials.append(mat_objs[-1].Material)
                        matnames.append(mat_objs[-1].Name)
            geoms = []
            i = 0
            for ref in shelllam_obj.References:
                # TODO: work out how to create group of all elements and apply to that in case where len(shelllam_obj.References) == 0.
                for geom in ref[1]:
                    geoms.append(geom)
                matname = "LAYUP" + str(i)
                layup = {
                    "group": ref[0].Name,
                    "matnames": matnames,
                    "thicknesses": thicknesses,
                    "orientations": orientations,
                }
                i += 1
                commtxt += add_layup(matname, layup)
                lams = [matname]
                ori_vec = shelllam_obj.Orientation
                commtxt += add_laminate([layup], ele_name, ori_vec)
                ca_writer.tools.group_elements[ref[0].Name] = geoms
        else:
            # TODO Need better way of managing material lists, at the moment the first one in the list is assumed as the default (so should be very thin, weak isotropic material) and 2nd entry in list is applied to all wound plies.
            FreeCAD.Console.PrintMessage("Applying filament winding layup\n")
            matnames = []
            for mo in mat_objs:
                matnames.append(mo.Name)

            commtxt += "# WindAll object detected\n"
            geoms = []
            i = 0
            for ref in shelllam_obj.References:
                # TODO: work out how to create group of all elements and apply to that in case where len(shelllam_obj.References) == 0.
                for geom in ref[1]:
                    geoms.append(geom)
                # Set default layup
                baselayup = {
                    "group": ref[0].Name,
                    "matnames": [matnames[0]],
                    "thicknesses": [thicknesses[0]],
                    "orientations": [orientations[0]],
                }
                layups = [baselayup]
                lams = [ref[0].Name]
                ori_vec = shelllam_obj.Orientation
                commtxt += add_layup(ref[0].Name, baselayup)
                for e, t, o in zip(
                    shelllam_obj.Windall["elements"],
                    shelllam_obj.Windall["thicknesslists"],
                    shelllam_obj.Windall["orientationlists"],
                ):
                    mn = [matnames[0]]
                    for i in range(1, len(t)):
                        mn.append(matnames[1])
                    gname = "E" + str(e)
                    lams.append(gname)
                    # TODO: WORK OUT WHY IT'S NOT PUTTING IN THE EXTRA PLIES!
                    layup = {"group": gname, "matnames": mn, "thicknesses": t, "orientations": o}
                    layups.append(layup)
                    commtxt += add_layup(layup["group"], layup)
                commtxt += add_grps(layups)
                commtxt += add_laminate(layups, ele_name, ori_vec)
                ca_writer.tools.group_elements[ref[0].Name] = geoms

    elif ca_writer.member.geos_shellthickness:
        # only use the first shellthickness object
        shellth_obj = ca_writer.member.geos_shellthickness[0]["Object"]
        thickness = shellth_obj.Thickness.getValueAs("mm").Value
        lams = [matname]
        geoms = []
        i = 0
        for (
            ref
        ) in (
            shellth_obj.References
        ):  # TODO: work out how to create group of all elements and apply to that in case where len(shellth_obj.References) == 0.
            for geom in ref[1]:
                geoms.append(geom)

            if "YoungsModulusX" in mat_objs[0].Material.keys():
                matname = mat_objs[0].Name + "LAYUP" + str(i)
                i += 1
                commtxt += "# Orthotropic material detected, added to shell at default angle\n"
                commtxt += f"{matname} = DEFI_COMPOSITE(COUCHE=(_F(EPAIS={thickness},\n"
                commtxt += f"                               MATER={mat_objs[0].Name},\n"
                commtxt += "                               ORIENTATION = 0)))\n\n"

            commtxt += (
                f"# Shell elements detected, thickness {thickness}mm on item {ref[0].Name,geom}\n"
            )
            commtxt += f"{ele_name} = AFFE_CARA_ELEM(COQUE=_F(EPAIS={thickness},\n"
            commtxt += f"                                   GROUP_MA=('{ref[0].Name}', )),\n"
            commtxt += "                          MODELE=model)\n\n"

            ca_writer.tools.group_elements[ref[0].Name] = geoms
        FreeCAD.Console.PrintMessage(f"Shell of thickness {thickness}mm added.\n")

    return commtxt, lams


def add_grps(layups):
    commtxt = "# Adding WindAll groups\n"
    commtxt += "grps = DEFI_GROUP(MAILLAGE=mesh, CREA_GROUP_MA = (\n"
    for layup in layups[1:]:
        commtxt += f"                                                  _F(GROUP_MA = ('{layups[0]['group']}',),\n"
        commtxt += f"                                                     NUME_INIT = {layup['group'][1:]},\n"
        commtxt += f"                                                     NUME_FIN = {layup['group'][1:]},\n"
        commtxt += (
            f"                                                     NOM = '{layup['group']}'),\n"
        )
    commtxt += "                                                     ))\n\n"
    return commtxt


def add_layup(LUname, layup):
    thicknesses, orientations, matnames = (
        layup["thicknesses"],
        layup["orientations"],
        layup["matnames"],
    )
    commtxt = "# Composite layup detected, added to shell\n"
    commtxt += f"{LUname} = DEFI_COMPOSITE(COUCHE=(\n"
    for j in range(len(thicknesses)):
        commtxt += f"                               _F(EPAIS={thicknesses[j]},\n"
        commtxt += f"                                MATER={matnames[j]},\n"
        commtxt += f"                                ORIENTATION = {orientations[j]}),\n"
    commtxt += "                                ))\n\n"
    return commtxt


def add_laminate(layups, ele_name, ori_vec):
    ori_vec_str = f"({ori_vec.x}, {ori_vec.y}, {ori_vec.z})"
    commtxt = "# Shell elements detected, applying composite laminate definition\n"
    commtxt += f"{ele_name} = AFFE_CARA_ELEM(COQUE=(\n"
    for layup in layups:
        thicknesses, group = layup["thicknesses"], layup["group"]
        thicktot = sum(thicknesses)
        commtxt += f"                                _F(COQUE_NCOU = {len(thicknesses)},\n"
        commtxt += f"                                   EPAIS={thicktot},\n"
        commtxt += f"                                   GROUP_MA=('{group}', ),\n"
        commtxt += f"                                   VECTEUR={ori_vec_str}),\n"
    commtxt += "                          ),\n"
    commtxt += "                          MODELE=model)\n\n"
    return commtxt


##  @}
