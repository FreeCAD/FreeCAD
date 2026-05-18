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

__title__ = "Code Aster add femelement materials"
__author__ = "Tim Swait"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{


from FreeCAD import Units


def define_femelement_material(commtxt, ca_writer):
    for mat_obj in ca_writer.mat_objs:
        commtxt += "# Defining materials\n"
        if "YoungsModulus" in mat_obj.Material.keys():
            if ca_writer.member.geos_shelllaminate:
                commtxt += "# Adding isotropic material using orthotropic definition to allow composite analysis {}\n".format(
                    mat_obj.Material["CardName"]
                )
                E = Units.Quantity(mat_obj.Material["YoungsModulus"])
                E = E.getValueAs("MPa").Value
                NU = float(mat_obj.Material["PoissonRatio"])
                G = E / (2 * (1 + NU))
                if "Density" in mat_obj.Material.keys():
                    RHO = Units.Quantity(mat_obj.Material["Density"])
                    RHO = RHO.getValueAs("t/mm^3").Value
                else:
                    commtxt += (
                        "# No value for Density given, gravitational effects cannot be calculated\n"
                    )
                    RHO = 0
                commtxt += "{} = DEFI_MATERIAU(ELAS_ORTH=_F(E_L={},\n".format(mat_obj.Name, E)
                commtxt += "                                E_T={},\n".format(E)
                commtxt += "                                E_N={},\n".format(E)
                commtxt += "                                G_LT={},\n".format(G)
                commtxt += "                                G_LN={},\n".format(G)
                commtxt += "                                G_TN={},\n".format(G)
                commtxt += "                                NU_LT={},\n".format(NU)
                commtxt += "                                NU_LN={},\n".format(NU)
                commtxt += "                                NU_TN={},\n".format(NU)
                commtxt += "                                RHO={}))\n\n".format(RHO)

            else:
                commtxt += "# Adding isotropic material {}\n".format(mat_obj.Material["CardName"])
                E = Units.Quantity(mat_obj.Material["YoungsModulus"])
                E = E.getValueAs("MPa").Value
                NU = float(mat_obj.Material["PoissonRatio"])
                if "Density" in mat_obj.Material.keys():
                    RHO = Units.Quantity(mat_obj.Material["Density"])
                    RHO = RHO.getValueAs("t/mm^3").Value
                else:
                    commtxt += (
                        "# No value for Density given, gravitational effects cannot be calculated\n"
                    )
                    RHO = 0
                commtxt += "{} = DEFI_MATERIAU(ELAS=_F(E={},\n".format(mat_obj.Name, E)
                commtxt += "                            NU={},\n".format(NU)
                commtxt += "                            RHO={}))\n\n".format(RHO)

        elif "YoungsModulusX" in mat_obj.Material.keys():
            commtxt += "# Adding orthotropic material {}\n".format(mat_obj.Material["CardName"])
            E_L = Units.Quantity(mat_obj.Material["YoungsModulusX"])
            E_L = E_L.getValueAs("MPa").Value
            E_T = Units.Quantity(mat_obj.Material["YoungsModulusY"])
            E_T = E_T.getValueAs("MPa").Value
            if "YoungsModulusZ" in mat_obj.Material.keys():
                E_N = Units.Quantity(mat_obj.Material["YoungsModulusZ"])
                E_N = E_N.getValueAs("MPa").Value
            else:
                commtxt += (
                    "# No value for through-thickness modulus given, using transverse value\n"
                )
                E_N = E_T
            G_LT = Units.Quantity(mat_obj.Material["ShearModulusXY"])
            G_LT = G_LT.getValueAs("MPa").Value
            if "ShearModulusXZ" in mat_obj.Material.keys():
                G_LN = Units.Quantity(mat_obj.Material["ShearModulusXZ"])
                G_LN = G_LN.getValueAs("MPa").Value
            else:
                commtxt += "# No value for longitudinal-normal modulus given, using longitudinal-transverse value\n"
                G_LN = G_LT
            if "ShearModulusYZ" in mat_obj.Material.keys():
                G_TN = Units.Quantity(mat_obj.Material["ShearModulusXZ"])
                G_TN = G_TN.getValueAs("MPa").Value
            else:
                commtxt += "# No value for transverse-normal modulus given, cannot calculate through thickness results (This may result in errors during calculation\n"
            NU_LT = float(mat_obj.Material["PoissonRatioXY"])
            if "Density" in mat_obj.Material.keys():
                RHO = Units.Quantity(mat_obj.Material["Density"])
                RHO = RHO.getValueAs("t/mm^3").Value
            else:
                commtxt += (
                    "# No value for Density given, gravitational effects cannot be calculated\n"
                )
                RHO = 0
            commtxt += "{} = DEFI_MATERIAU(ELAS_ORTH=_F(E_L={},\n".format(mat_obj.Name, E_L)
            commtxt += "                                E_T={},\n".format(E_T)
            commtxt += "                                E_N={},\n".format(E_N)
            commtxt += "                                G_LT={},\n".format(G_LT)
            commtxt += "                                G_LN={},\n".format(G_LN)
            commtxt += "                                G_TN={},\n".format(G_TN)
            commtxt += "                                NU_LT={},\n".format(NU_LT)
            commtxt += "                                RHO={}))\n\n".format(RHO)

    return commtxt


def assign_femelement_material(commtxt, lams, ca_writer):
    if len(lams) > 1:
        commtxt += "# Assigning multi lay ups to areas\n"
        commtxt += "fieldmat = AFFE_MATERIAU(AFFE=(\n"
        for l in lams:
            commtxt += "                               _F(MATER=({}, ),\n".format(l)
            commtxt += "                                  GROUP_MA=('{}', )),\n".format(l)
        commtxt += "                              ),\n"
        commtxt += "                              MODELE=model)\n\n"
    else:
        commtxt += "# Assigning single material/layup to all areas\n"
        commtxt += "fieldmat = AFFE_MATERIAU(AFFE=_F(MATER=({}, ),\n".format(lams[0])
        commtxt += "                                 TOUT='OUI'),\n"
        commtxt += "                         MAILLAGE=mesh)\n\n"

    return commtxt


##  @}
