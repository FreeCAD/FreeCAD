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

__title__ = "Code Aster add fixed constraint"
__author__ = "Tim Swait"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{


def add_con_fixed(commtxt, ca_writer):

    commtxt += "# Adding fixed constraints\n"
    geoms = []
    for i, femobj in enumerate(ca_writer.member.cons_fixed):
        fixed_obj = femobj["Object"]
        for ref in fixed_obj.References:
            for geom in ref[1]:
                geoms.append(geom)
        ca_writer.fixes.append(f"fix{len(ca_writer.fixes)}")
        commtxt += f"{ca_writer.fixes[-1]} = AFFE_CHAR_MECA(DDL_IMPO=_F(DRX=0.0,\n"
        commtxt += "                                 DRY=0.0,\n"
        commtxt += "                                 DRZ=0.0,\n"
        commtxt += "                                 DX=0.0,\n"
        commtxt += "                                 DY=0.0,\n"
        commtxt += "                                 DZ=0.0,\n"
        commtxt += f"                                 GROUP_MA=({str(geoms)[1:-1]} )),\n"
        commtxt += "                     MODELE=model)\n\n"

    return commtxt


##  @}
