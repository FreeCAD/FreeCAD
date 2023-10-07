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

__title__ = "Mystran add force constraint"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecad.org"

## \addtogroup FEM
#  @{


def add_con_force(f, model, mystran_writer):

    # generate pyNastran code
    # force card
    scale_factors = []
    load_ids = []
    force_code = "# force cards, mesh node loads\n"
    for i, femobj in enumerate(mystran_writer.member.cons_force):

        sid = i + 2  # 1 will be the id of the load card
        scale_factors.append(1.0)
        load_ids.append(sid)
        force_obj = femobj["Object"]
        # print(force_obj.Name)

        force_code += "# {}\n".format(force_obj.Name)
        dirvec = femobj["Object"].DirectionVector
        print(femobj["NodeLoadTable"])
        for ref_shape in femobj["NodeLoadTable"]:
            force_code += "# {}\n".format(ref_shape[0])
            for n in sorted(ref_shape[1]):
                # the loads in ref_shape[1][n] are without unit
                node_load = ref_shape[1][n]
                force_code += (
                    "model.add_force(sid={}, node={}, mag={}, xyz=({}, {}, {}))\n"
                    .format(sid, n, node_load, dirvec.x, dirvec.y, dirvec.z)
                )
        force_code += "\n"

    # generate calce factors lists
    # load card, static load combinations
    load_code = (
        "model.add_load(sid=1, scale=1.0, scale_factors={}, load_ids={})\n\n\n"
        .format(scale_factors, load_ids)
    )

    pynas_code = "{}\n{}".format(force_code, load_code)
    # print(pynas_code)

    # write the pyNastran code
    f.write(pynas_code)

    # execute pyNastran code to add data to the model
    # print(model.get_bdf_stats())
    exec(pynas_code)
    # print(model.get_bdf_stats())

    return model


##  @}
