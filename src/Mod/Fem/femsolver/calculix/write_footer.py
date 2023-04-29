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

__title__ = "FreeCAD FEM calculix write inpfile footer"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


import os
import time


def write_footer(f, ccxwriter):

    f.write("\n{}\n".format(59 * "*"))
    f.write("** CalculiX Input file\n")
    f.write("**   written by    --> FreeCAD {}.{}.{}\n".format(
        ccxwriter.fc_ver[0],
        ccxwriter.fc_ver[1],
        ccxwriter.fc_ver[2]
    ))
    f.write("**   written on    --> {}\n".format(
        time.ctime()
    ))
    f.write("**   file name     --> {}\n".format(
        os.path.basename(ccxwriter.document.FileName)
    ))
    f.write("**   analysis name --> {}\n".format(
        ccxwriter.analysis.Name
    ))
    f.write("**\n")
    f.write("**\n")
    f.write(ccxwriter.units_information)
    f.write("**\n")
