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

__title__ = "Constraint writer base class for Z88 solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"


class WriterList:
    def __init__(self, writer, member_list):
        self.writer = writer
        self.member_list = member_list

    def write_items(self):
        for item in self.member_list:
            self.write_item(item)

    def get_elements_ranges(self, elements):
        """Create Z88 ranges to reduce file size"""
        rg = []
        first = self.writer.element_id_map[elements[0]]
        start_end = [first, first]
        for el in elements[1:]:
            el_index = self.writer.element_id_map[el]
            if el_index - start_end[1] == 1:
                start_end[1] = el_index
            else:
                rg.append(start_end)
                start_end = [el_index, el_index]
        # add last computed range
        rg.append(start_end)

        return rg

    def add_file_rows(self, item_elements, rows, param):
        """Add range + parameter to list of lines"""
        for feat, elements, is_sub_el in item_elements:
            if not is_sub_el and elements:
                rg = self.get_elements_ranges(elements)
                for start, end in rg:
                    rows.append((start + 1, end + 1, param))

    def get_start_end_id(self):
        el_range = tuple(self.writer.element_id_map.values())
        return (el_range[0] + 1, el_range[-1] + 1)

    def fill_sorted_ranges(self, a, b, sort, value):
        """Continuosly complete a list of sorted Z88 ranges from a bigger range (a, b)"""
        res = []
        if not sort:
            res.append((a, b, value))
            return res

        first = sort[0]
        if first[0] == a:
            res.append(first)
        else:
            res.append((a, first[0] - 1, value))
            res.append(first)
        for i in sort[1:]:
            if i[0] - 1 == res[-1][1]:
                # contiguous
                res.append(i)
            else:
                res.append((res[-1][1] + 1, i[0] - 1, value))
                res.append(i)
        last = sort[-1]
        if last[1] < b:
            res.append((last[1] + 1, b, value))
        return res

    def fill_default(self, rows, func):
        """If there is any object without references, use it as default for not assigned  elements"""
        rows = sorted(rows)
        for item in self.member_list:
            obj = item["Object"]
            if not obj.References:
                param = func(obj)
                start, end = self.get_start_end_id()
                return self.fill_sorted_ranges(start, end, rows, param)
        return rows
