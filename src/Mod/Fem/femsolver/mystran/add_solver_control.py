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

__title__ = "Mystran add solver control"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecad.org"

## \addtogroup FEM
#  @{


def add_solver_control(f, model, mystran_writer):

    # write the pyNastran code which will be executed into the file
    f.write(pynas_code)

    # print(model.get_bdf_stats())
    exec(pynas_code)
    # print(model.get_bdf_stats())

    return model


pynas_code = """
# executive control
model.sol = 101


# params cards
model.add_param(key="POST", values=-1)
# model.add_param(key="PRTMAXIM", values="YES")  # not recognized by Mystran


# case control
from pyNastran.bdf.bdf import CaseControlDeck
cc = CaseControlDeck([
    "ECHO = NONE",
    "TITLE = pyNastran for generating solverinput for for Mystran",
    "SUBCASE 1",
    "  SUBTITLE = Default",
    "  LOAD = 1",
    "  SPC = 1",
    "  SPCFORCES(SORT1,REAL) = ALL",
    "  STRESS(SORT1,REAL,VONMISES,BILIN) = ALL",
    "  DISPLACEMENT(SORT1,REAL) = ALL",
])
model.case_control_deck = cc
# model.validate()  # creates an error
"""


##  @}
