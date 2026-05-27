# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "Write force constraint for Z88 solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

import math

from .writer_list import WriterList


class WriterForce(WriterList):
    def __init__(self, writer):
        super().__init__(writer, writer.member.cons_force)
        self._abs_tol = 1e-10

    def write_item(self, item):
        obj = item["Object"]

        has_x, has_y, has_z = [
            not math.isclose(i, 0, abs_tol=self._abs_tol) for i in obj.DirectionVector
        ]
        index = self.writer.nodes["index"]
        for ref_shape in item["NodeLoadTable"]:
            for idx, node_load in sorted(ref_shape[1].items()):
                # the loads in ref_shape[1][n] are without unit
                node_force = obj.DirectionVector * node_load
                n = index[self.writer.node_id_map(idx)]
                if has_x:
                    self.writer.z88i2_rows.append(f"{n}  1  1  {node_force.x}\n")
                if has_y:
                    self.writer.z88i2_rows.append(f"{n}  2  1  {node_force.y}\n")
                if has_z:
                    self.writer.z88i2_rows.append(f"{n}  3  1  {node_force.z}\n")
