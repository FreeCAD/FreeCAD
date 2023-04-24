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

__title__ = "Mystran add fixed constraint"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecad.org"

## \addtogroup FEM
#  @{


def add_con_fixed(f, model, mystran_writer):

    # generate pyNastran code
    # spc1 card
    spc_ids = []
    fixed_code = "# spc1 card, Defines a set of single-point constraints\n"
    for i, femobj in enumerate(mystran_writer.member.cons_fixed):

        conid = i + 2  # 1 will be the conid of the spcadd card
        spc_ids.append(conid)
        fixed_obj = femobj["Object"]
        # print(fixed_obj.Name)
        fixed_code += "# {}\n".format(fixed_obj.Name)
        # node set
        fixed_code += "nodes_{} = {}\n".format(fixed_obj.Name, femobj["Nodes"])
        # all nodes in one line may be to long ... FIXME
        fixed_code += (
            "model.add_spc1(conid={}, components={}, nodes=nodes_{})\n\n"
            .format(conid, "123456", fixed_obj.Name)
        )

    # spcadd card
    spcadd_code = "# spcadd card, Single-Point Constraint Set Combination from SPC or SPC1 cards\n"
    spcadd_code += (
        "model.add_spcadd(conid=1, sets={})\n\n".format(spc_ids)
    )

    pynas_code = "{}\n{}".format(fixed_code, spcadd_code)
    # print(pynas_code)

    # write the pyNastran code
    f.write(pynas_code)

    # execute pyNastran code to add data to the model
    # print(model.get_bdf_stats())
    exec(pynas_code)
    # print(model.get_bdf_stats())
    return model


##  @}
