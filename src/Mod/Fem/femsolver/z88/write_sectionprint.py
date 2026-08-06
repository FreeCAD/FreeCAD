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

__title__ = "Write section print for Z88 solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

import numpy as np

from .writer_list import WriterList


class WriterSectionPrint(WriterList):
    def __init__(self, writer):
        super().__init__(writer, writer.member.cons_sectionprint)

    def write_item(self, item):
        obj = item["Object"]

        if obj.Variable != "Section Force":
            return

        idx_collection = set()
        for feat, surf, is_sub_el in item["SectionPrintFaces"]:
            if is_sub_el:
                for el, mask in surf:
                    el_index = self.writer.element_id_map[el]
                    size = self.writer.elements["size"][el_index]
                    nodes_array = self.writer.elements["nodes"][el_index][:size]
                    # Z88 nodes start from 1. Subtract 1 to get row in nodes array
                    idx_collection.update((nodes_array[mask] - 1).tolist())
            else:
                for el in surf:
                    el_index = self.writer.element_id_map[el]
                    size = self.writer.elements["size"][el_index]
                    nodes_array = self.writer.elements["nodes"][el_index][:size]
                    idx_collection.update((nodes_array - 1).tolist())

        self.writer.z88section_print_dict[obj.Name] = idx_collection
