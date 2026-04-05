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

__title__ = "Write pressure constraint for Z88 solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

import numpy as np

from .writer_list import WriterList


class WriterPressure(WriterList):
    def __init__(self, writer):
        super().__init__(writer, writer.member.cons_pressure)

    def write_item(self, item):
        obj = item["Object"]
        pressure = obj.Pressure.getValueAs("MPa").Value
        pressure *= -1 if obj.Reversed else 1

        for feat, surf, is_sub_el in item["PressureFaces"]:
            if is_sub_el:
                for el, mask in surf:
                    el_index = self.writer.element_id_map[el]
                    size = self.writer.elements["size"][el_index]
                    nodes_array = self.writer.elements["nodes"][el_index][:size]
                    el_type = self.writer.elements["type"][el_index]
                    match el_type:
                        case 1 | 10:
                            # hexa8, hexa20
                            load = f"{pressure} 0. 0."
                        case 16 | 17:
                            # tetra10, tetra4
                            load = f"{pressure}"
                        case 7 | 14:
                            # quad8, tria6 plane stress
                            load = f"{pressure} 0."
                        case 8 | 15:
                            # quad8, tria6 axisymmetric, multiply pressure by 2*pi*R
                            n_index = self.writer.node_id_map[nodes_array[mask][2]]
                            y_coord = self.writer.nodes["coords"][n_index][0]
                            load = f"{pressure*2*np.pi*y_coord} 0."
                        case _:
                            raise RuntimeError(
                                f"{obj.Name} load not supported on {feat[0].Name}.{feat[1][0]}"
                            )

                    nodes = " ".join(map(str, nodes_array[mask]))
                    self.writer.z88i5_rows.append(f"{el_index + 1} {load} {nodes}\n")
            else:
                for el in surf:
                    el_index = self.writer.element_id_map[el]
                    size = self.writer.elements["size"][el_index]
                    nodes_array = self.writer.elements["nodes"][el_index][:size]
                    el_type = self.writer.elements["type"][el_index]
                    match el_type:
                        case 18 | 20 | 23 | 24:
                            # tria6, quad8 shell, plate
                            # pressure must be reversed for shell elements
                            load = f"{-1*pressure}"
                        case _:
                            raise RuntimeError(
                                f"{obj.Name} load not supported on {feat[0].Name}.{feat[1][0]}"
                            )

                    nodes = " ".join(map(str, nodes_array))
                    self.writer.z88i5_rows.append(f"{el_index + 1} {load} {nodes}\n")
