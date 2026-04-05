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

__title__ = "Write element1D for Z88 solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

from .writer_list import WriterList


class WriterElement1D(WriterList):
    def __init__(self, writer):
        super().__init__(writer, writer.member.geos_beamsection)

    def write_item(self, item):
        obj = item["Object"]

        param = "0"
        if self.writer.solver_obj.ExcludeBendingStiffness:
            param = obj.TrussArea.getValueAs("mm^2").Value

        self.add_file_rows(item["BeamElements"], self.writer.z88elp_rows, param)

        # if there is only one beam section and without references, assign it to all elements.
        if len(self.member_list) == 1 and not self.writer.z88elp_rows:
            start, end = self.get_start_end_id()
            self.writer.z88elp_rows.append(f"{start + 1} {end + 1} {param}\n")
