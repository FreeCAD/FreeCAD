# ***************************************************************************
# *   Copyright (c) 2026 Tim Swait <t.swait@sheffield.ac.uk>                *
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

__title__ = "Code Aster add displacement constraint"
__author__ = "Tim Swait"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{


def add_con_displacement(commtxt, ca_writer):

    commtxt += "# Adding displacement constraints\n"
    for i, femobj in enumerate(ca_writer.member.cons_displacement):
        geoms = []
        obj = femobj["Object"]
        x_free = obj.xFree
        x_disp = obj.xDisplacement.getValueAs("mm").Value
        y_free = obj.yFree
        y_disp = obj.yDisplacement.getValueAs("mm").Value
        z_free = obj.xFree
        z_disp = obj.xDisplacement.getValueAs("mm").Value
        rotx_free = obj.rotxFree
        x_rot = obj.xRotation.getValueAs("rad").Value
        roty_free = obj.rotyFree
        y_rot = obj.yRotation.getValueAs("rad").Value
        rotz_free = obj.rotzFree
        z_rot = obj.zRotation.getValueAs("rad").Value
        for ref in obj.References:
            for geom in ref[1]:
                geoms.append(geom)
        ca_writer.disps.append(f"dis{len(ca_writer.disps)}")
        commtxt += f"{ca_writer.disps[-1]} = AFFE_CHAR_MECA(DDL_IMPO=_F(\n"
        if not x_free:
            commtxt += f"                                 DX={x_disp},\n"
        if not y_free:
            commtxt += f"                                 DY={y_disp},\n"
        if not z_free:
            commtxt += f"                                 DZ={z_disp},\n"
        if not rotx_free:
            commtxt += f"                                 DRX={x_rot},\n"
        if not roty_free:
            commtxt += f"                                 DRY={y_rot},\n"
        if not rotz_free:
            commtxt += f"                                 DRZ={z_rot},\n"
        commtxt += f"                                 GROUP_MA=({str(geoms)[1:-1]} )),\n"
        commtxt += "                     MODELE=model)\n\n"

    return commtxt


##  @}
