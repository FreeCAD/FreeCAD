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

__title__ = "Utils for CalculiX solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"


def define_masks(solver):
    # edge masks to search edges from face elements (in smesh order)
    shell_mode = solver.ModelSpace == "3D"
    offset = 2 if shell_mode else 0
    mask_tria3 = {0b011: 1 + offset, 0b110: 2 + offset, 0b101: 3 + offset}
    mask_tria6 = {0b001011: 1 + offset, 0b010110: 2 + offset, 0b100101: 3 + offset}
    mask_quad4 = {
        0b0011: 1 + offset,
        0b0110: 2 + offset,
        0b1100: 3 + offset,
        0b1001: 4 + offset,
    }
    mask_quad8 = {
        0b00010011: 1 + offset,
        0b00100110: 2 + offset,
        0b01001100: 3 + offset,
        0b10001001: 4 + offset,
    }

    # face masks to search faces from volume elements (in smesh order)
    mask_tetra4 = {0b0111: 1, 0b1011: 2, 0b1101: 3, 0b1110: 4}
    mask_tetra10 = {0b0001110111: 1, 0b0110011011: 2, 0b1011001101: 3, 0b1100101110: 4}
    mask_hexa8 = {
        0b11110000: 1,
        0b00001111: 2,
        0b01100110: 3,
        0b11001100: 4,
        0b10011001: 5,
        0b00110011: 6,
    }
    mask_hexa20 = {
        0b00001111000011110000: 1,
        0b00000000111100001111: 2,
        0b01100010001001100110: 3,
        0b11000100010011001100: 4,
        0b10011000100010011001: 5,
        0b00110001000100110011: 6,
    }
    mask_penta6 = {0b111000: 1, 0b000111: 2, 0b110110: 3, 0b101101: 4, 0b011011: 5}
    mask_penta15 = {
        0b000111000111000: 1,
        0b000000111000111: 2,
        0b110010010110110: 3,
        0b101100100101101: 4,
        0b011001001011011: 5,
    }

    return {
        "tria3": mask_tria3,
        "tria6": mask_tria6,
        "quad4": mask_quad4,
        "quad8": mask_quad8,
        "tetra4": mask_tetra4,
        "tetra10": mask_tetra10,
        "hexa8": mask_hexa8,
        "hexa20": mask_hexa20,
        "penta6": mask_penta6,
        "penta15": mask_penta15,
    }
