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

__title__ = "Mystran add femelement materials"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecad.org"

## \addtogroup FEM
#  @{


from FreeCAD import Units


def add_femelement_material(f, model, mystran_writer):

    # generate pyNastran code
    # only use the first material object
    mat_obj = mystran_writer.member.mats_linear[0]["Object"]
    YM = Units.Quantity(mat_obj.Material["YoungsModulus"])
    YM_in_MPa = YM.getValueAs("MPa").Value
    PR = float(mat_obj.Material["PoissonRatio"])
    pynas_code = "# mat1 card, material properties for linear isotropic material\n"
    pynas_code += (
        "mat = model.add_mat1(mid=1, E={:.1f}, G=None, nu={})\n\n\n"
        .format(YM_in_MPa, PR)
    )

    # write the pyNastran code
    f.write(pynas_code)

    # execute pyNastran code to add data to the model
    # print(model.get_bdf_stats())
    exec(pynas_code)
    # print(model.get_bdf_stats())

    return model


##  @}
