# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM calculix write inpfile material and geometry sets"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecadweb.org"


import six


def write_femelement_matgeosets(f, ccxwriter):

    # write ccx_elsets to file
    f.write("\n{}\n".format(59 * "*"))
    f.write("** Element sets for materials and FEM element type (solid, shell, beam, fluid)\n")

    for ccx_elset in ccxwriter.ccx_elsets:

        f.write("*ELSET,ELSET={}\n".format(ccx_elset["ccx_elset_name"]))

        # use six to be sure to be Python 2.7 and 3.x compatible
        if isinstance(ccx_elset["ccx_elset"], six.string_types):
            f.write("{}\n".format(ccx_elset["ccx_elset"]))
        else:
            for elid in ccx_elset["ccx_elset"]:
                f.write(str(elid) + ",\n")
