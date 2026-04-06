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

__title__ = "Write material for Z88 solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

import os
import FreeCAD

from .writer_list import WriterList


class WriterMaterial(WriterList):
    def __init__(self, writer):
        super().__init__(writer, writer.member.mats_linear)

    def write_item(self, item):
        obj = item["Object"]

        file_name = f"{obj.Name}.txt"
        mat_path = os.path.join(self.writer.solver_obj.WorkingDirectory, file_name)
        mat_file = open(mat_path, "w")

        mat = obj.Material
        ym = FreeCAD.Units.Quantity(mat["YoungsModulus"]).getValueAs("MPa")
        pr = float(mat["PoissonRatio"])
        mat_file.write(f"{ym} {pr:.3f}\n")
        mat_file.close()

        self.add_file_rows(item["MaterialElements"], self.writer.z88mat_rows, file_name)

        # if there is only one material and without references, assign it to all elements.
        if len(self.member_list) == 1 and not self.writer.z88mat_rows:
            start, end = self.get_start_end_id()
            self.writer.z88mat_rows.append(f"{start + 1} {end + 1} {file_name}\n")
