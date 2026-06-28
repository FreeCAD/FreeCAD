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

__title__ = "Write displacement constraint for Z88 solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

from .writer_list import WriterList


class WriterDisplacement(WriterList):
    def __init__(self, writer):
        super().__init__(writer, writer.member.cons_displacement)

    def write_item(self, item):
        obj = item["Object"]

        x_free = obj.xFree
        x_disp = obj.xDisplacement.getValueAs("mm").Value
        y_free = obj.yFree
        y_disp = obj.yDisplacement.getValueAs("mm").Value
        z_free = obj.zFree
        z_disp = obj.zDisplacement.getValueAs("mm").Value
        rotx_free = obj.rotxFree
        x_rot = obj.xRotation.getValueAs("rad").Value
        roty_free = obj.rotyFree
        y_rot = obj.yRotation.getValueAs("rad").Value
        rotz_free = obj.rotzFree
        z_rot = obj.zRotation.getValueAs("rad").Value

        # for plate elements, dof-1 -> z, dof-2 -> rot_x, dof-3 -> rot_y
        index = self.writer.nodes["index"]
        if self.writer.solver_obj.ModelSpace == "plate":
            for idx in item["Nodes"]:
                n = index[self.writer.node_id_map(idx)]
                if not z_free:
                    self.writer.z88i2_rows.append(f"{n}  1  2  {z_disp}\n")
                if not rotx_free:
                    self.writer.z88i2_rows.append(f"{n}  2  2  {x_rot}\n")
                if not roty_free:
                    self.writer.z88i2_rows.append(f"{n}  3  2  {y_rot}\n")
        else:
            for idx in item["Nodes"]:
                n = index[self.writer.node_id_map(idx)]
                if not x_free:
                    self.writer.z88i2_rows.append(f"{n}  1  2  {x_disp}\n")
                if not y_free:
                    self.writer.z88i2_rows.append(f"{n}  2  2  {y_disp}\n")
                if not z_free:
                    self.writer.z88i2_rows.append(f"{n}  3  2  {z_disp}\n")
                if not rotx_free:
                    self.writer.z88i2_rows.append(f"{n}  4  2  {x_rot}\n")
                if not roty_free:
                    self.writer.z88i2_rows.append(f"{n}  5  2  {y_rot}\n")
                if not rotz_free:
                    self.writer.z88i2_rows.append(f"{n}  6  2  {z_rot}\n")
