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

__title__ = "Mystran add femelement geometry"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecad.org"

## \addtogroup FEM
#  @{


def add_femelement_geometry(f, model, mystran_writer):

    # generate pyNastran code
    # HACK, the if statemant needs improvement, see calculix solver
    if mystran_writer.member.geos_beamsection:
        beamsec_obj = mystran_writer.member.geos_beamsection[0]["Object"]
        if beamsec_obj.SectionType == "Rectangular":
            height = beamsec_obj.RectHeight.getValueAs("mm").Value
            width = beamsec_obj.RectWidth.getValueAs("mm").Value
            pynas_code = "# pbarl card, properties of a simple beam element (CBAR entry)\n"
            pynas_code += "# defined by cross-sectional dimensions\n"
            pynas_code += (
                "dim = [{}, {}]\n"
                .format(width, height)
            )
            pynas_code += (
                "model.add_pbarl(pid=1, mid=1, Type={}, dim=dim, nsm=0.0)\n"
                .format('"BAR"')
            )
            pynas_code += "# pbarl.validate()\n\n\n"
        else:
            return
    elif mystran_writer.member.geos_shellthickness:
        # only use the first shellthickness object
        shellth_obj = mystran_writer.member.geos_shellthickness[0]["Object"]
        thickness = shellth_obj.Thickness.getValueAs("mm").Value
        pynas_code = "# pshell card, thin shell element properties\n"
        pynas_code += (
            "model.add_pshell(pid=1, mid1=1, t={}, mid2=1, mid3=1)\n\n\n"
            .format(thickness)
        )
    else:
        pynas_code = "# psolid card, defines solid element\n"
        pynas_code += "model.add_psolid(pid=1, mid=1)\n\n\n"

    # write the pyNastran code
    f.write(pynas_code)

    # execute pyNastran code to add data to the model
    # print(model.get_bdf_stats())
    exec(pynas_code)
    # print(model.get_bdf_stats())

    return model


pynas_code = """
# pshell card, thin shell element properties
model.add_pshell(1, mid1=1, t=0.3, mid2=1, mid3=1)


"""


##  @}
