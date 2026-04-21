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

__title__ = "Write fixed constraint for Z88 solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

from .writer_list import WriterList


class WriterFixed(WriterList):
    def __init__(self, writer):
        super().__init__(writer, writer.member.cons_fixed)

    def write_item(self, item):
        dofs = self.writer.nodes["dof"]
        for n in item["Nodes"]:
            dof = dofs[self.writer.node_id_map[n]]
            self.writer.z88i2_rows.append(f"{n}  1  2  0\n")
            self.writer.z88i2_rows.append(f"{n}  2  2  0\n")
            if dof >= 3:
                self.writer.z88i2_rows.append(f"{n}  3  2  0\n")
            if dof == 6:
                self.writer.z88i2_rows.append(f"{n}  4  2  0\n")
                self.writer.z88i2_rows.append(f"{n}  5  2  0\n")
                self.writer.z88i2_rows.append(f"{n}  6  2  0\n")
