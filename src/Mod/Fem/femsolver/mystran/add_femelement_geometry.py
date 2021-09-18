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
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import math


def add_femelement_geometry(f, model, mystran_writer):

    # generate pyNastran code
    # HACK, the if statemant needs improvement, see calculix solver
    # print (model)
    # print (mystran_writer.member)
    if mystran_writer.member.geos_beamsection:

        beamsec_obj = mystran_writer.member.geos_beamsection[0]["Object"]

        # PBARL
        isRod = False
        if beamsec_obj.SectionType == "Rectangular_L":
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

        # PBAR
        # pyNastran def add_pbar(self, pid, mid, A=0., i1=0., i2=0., i12=0., j=0., nsm=0.,
        #             c1=0., c2=0., d1=0., d2=0., e1=0., e2=0.,
        #             f1=0., f2=0., k1=1.e8, k2=1.e8, comment="") -> PBAR:
        # PROD
        # pyNastran def add_prod(self, pid: int, mid: int, A: float,
        #             j: float=0., c: float=0., nsm: float=0., comment: str="") -> PROD:

        elif beamsec_obj.SectionType == "Rectangular":
            height = beamsec_obj.RectHeight.getValueAs("mm").Value
            width = beamsec_obj.RectWidth.getValueAs("mm").Value
            pynas_code = "# pbarl card, properties of a simple beam element (CBAR entry)\n"
            pynas_code += "# defined by cross-sectional dimensions\n"
            pynas_code += (
                "dim = [{}, {}]\n"
                .format(width, height)
            )
            pynas_code += (
                "A = {}\n"
                .format(width * height)
            )
            pynas_code += (
                "i1 = {}\n"
                .format(width * math.pow(height, 3) / 12.0)
            )
            pynas_code += (
                "i2 = {}\n"
                .format(math.pow(width, 3) * height / 12.0)
            )
            pynas_code += (
                "j = {}\n"
                .format(width * math.pow(height, 3) * (1.0 / 3.0 - 21.0 * height / width * (1.0 - math.pow(height / width, 4) / 12.0)))
            )
            if isRod == True:
                pynas_code += (
                    "model.add_prod(pid=1, mid=1, A=A)\n"
                )
            else:
                pynas_code += (
                    "model.add_pbar(pid=1, mid=1, A=A, i1=i1, i2=i2, j=j)\n"
                )

            pynas_code += "# pbar.validate()\n\n\n"

        elif beamsec_obj.SectionType == "Circular":
            diameter = beamsec_obj.CircDiameter.getValueAs("mm").Value
            area = math.pi / 4.0 * diameter * diameter
            i1 = math.pi / 4.0 * math.pow(diameter / 2.0, 4)
            i2 = i1
            j = math.pi / 2.0 * math.pow(diameter / 2.0, 4)
            pynas_code = "# pbar card, properties of a simple beam element (CBAR entry)\n"
            pynas_code += "# defined by cross-sectional dimensions\n"
            pynas_code += "A = {}\n".format(area)
            pynas_code += "i1 = {}\n".format(i1)
            pynas_code += "i2 = {}\n".format(i2)
            pynas_code += "j = {}\n".format(j)
            if isRod == True:
                pynas_code += (
                    "model.add_prod(pid=1, mid=1, A=A)\n"
                )
            else:
                pynas_code += (
                    "model.add_pbar(pid=1, mid=1, A=A, i1=i1, i2=i2, j=j)\n"
                )
            pynas_code += "# pbar.validate()\n\n\n"

        elif beamsec_obj.SectionType == "Pipe":
            diameter = beamsec_obj.PipeDiameter.getValueAs("mm").Value
            thickness = beamsec_obj.PipeThickness.getValueAs("mm").Value
            dim1 = diameter / 2.0
            dim2 = diameter / 2.0 - thickness
            pynas_code = "# pbar card, properties of a simple beam element (CBAR entry)\n"
            pynas_code += "# defined by cross-sectional dimensions\n"
            pynas_code += (
                "dim = [{}, {}]\n"
                .format(dim1, dim2)
            )
            pynas_code += (
                "A = {}\n"
                .format(math.pi / 4.0 * (dim1 * dim1 - dim2 * dim2))
            )
            pynas_code += (
                "i1 = {}\n"
                .format(math.pi / 4.0 * (math.pow(dim1, 4) - math.pow(dim2, 4)))
            )
            pynas_code += (
                "i2 = {}\n"
                .format(math.pi / 4.0 * (math.pow(dim1, 4) - math.pow(dim2, 4)))
            )
            pynas_code += (
                "j = {}\n"
                .format(math.pi / 2.0 * (math.pow(dim1, 4) - math.pow(dim2, 4)))
            )
            if isRod == True:
                pynas_code += (
                    "model.add_prod(pid=1, mid=1, A=A)\n"
                )
            else:
                pynas_code += (
                    "model.add_pbar(pid=1, mid=1, A=A, i1=i1, i2=i2, j=j)\n"
                )
            pynas_code += "# pbar.validate()\n\n\n"
        else:
            print("This type of 1D element section is not supported yet.")
            print(beamsec_obj.SectionType)
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
