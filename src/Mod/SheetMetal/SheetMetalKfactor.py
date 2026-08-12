########################################################################
#
#  SheetMetalKfactor.py
#
#  Copyright 2014, 2018 Ulrich Brammer <ulrich@Pauline>
#  Copyright 2023 Ondsel Inc.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
########################################################################

import re

import FreeCAD


def findObjectsByTypeRecursive(doc, tp):
    return _find_objects(doc.Objects, lambda obj: obj and obj.isDerivedFrom(tp))


def _find_objects(objs, _filter):
    res = []
    queue = list(objs)
    visited = set(objs)
    while queue:
        obj = queue.pop(0)
        r = _filter(obj)
        if r:
            res.append(obj)
            if r > 1:
                break
        elif r < 0:
            break
        else:
            linked = obj.getLinkedObject()
            if linked not in visited:
                visited.add(linked)
                queue.append(linked)
        try:
            names = obj.getSubObjects()
        except Exception:
            names = []
        for name in names:
            sobj = obj.getSubObject(name, retType=1)
            if sobj not in visited:
                visited.add(sobj)
                queue.append(sobj)
    return res


def getSpreadSheetNames():
    material_sheet_regex_str = r"material_([a-zA-Z0-9_\-\[\]\.]+)"
    material_sheet_regex = re.compile(material_sheet_regex_str)

    spreadsheets = findObjectsByTypeRecursive(FreeCAD.ActiveDocument, "Spreadsheet::Sheet")
    candidateSpreadSheets = [o for o in spreadsheets if material_sheet_regex.match(o.Label)]

    availableMdsObjects = []
    for candidate in candidateSpreadSheets:
        try:
            KFactorLookupTable(candidate.Label)
            availableMdsObjects.append(candidate)
        except ValueError as e:
            FreeCAD.Console.PrintWarning(
                f"Spreadsheet with name {candidate.Label} is not a valid material definition "
                "table.\n")
            FreeCAD.Console.PrintWarning(f"Error: {e}\n")

    return availableMdsObjects


class KFactorLookupTable:
    cell_regex = re.compile(r"^([A-Z]+)([0-9]+)$")

    def __init__(self, material_sheet):
        lookup_sheet = FreeCAD.ActiveDocument.getObjectsByLabel(material_sheet)
        if len(lookup_sheet) >= 1:
            lookup_sheet = lookup_sheet[0]
        else:
            raise ValueError(
                "No spreadsheet found containing material definition: %s" % material_sheet)

        key_cell = self.find_cell_by_label(lookup_sheet, "Radius / Thickness")
        value_cell, k_factor_standard = self.find_k_factor_cell(lookup_sheet)
        if key_cell is None:
            raise ValueError("No cell found with label: 'Radius / Thickness'")
        if value_cell is None:
            raise ValueError("No cell found with label: 'K-factor (ANSI/DIN)'")
        if k_factor_standard is None:
            raise ValueError("No 'Options' column or 'K-factor (????)' cell found.")

        key_column_name, key_column_row = self.get_cell_tuple(key_cell)
        value_column_name = self.get_cell_tuple(value_cell)[0]
        k_factor_lookup = self.build_k_factor_lookup(lookup_sheet, key_column_name, key_column_row,
                                                     value_column_name)
        options_cell = self.find_cell_by_label(lookup_sheet, "Options")
        k_factor_standard = self.get_k_factor_standard(lookup_sheet, options_cell,
                                                       k_factor_standard)

        if k_factor_standard not in ["ansi", "din"]:
            raise ValueError("Invalid K-factor standard: %s" % k_factor_standard)

        self.k_factor_lookup = k_factor_lookup
        self.k_factor_standard = k_factor_standard

    def get_cells(self, sheet):
        return sorted(filter(self.cell_regex.search, sheet.PropertiesList))

    def get_cell_tuple(self, cell_name):
        m = self.cell_regex.match(cell_name)
        col_name = m.group(1)
        row_num = int(m.group(2))
        return (col_name, row_num)

    def find_cell_by_label(self, sheet, label):
        for cell in self.get_cells(sheet):
            content = sheet.get(cell)
            if str(content).strip() == label:
                return cell
        return None

    def find_k_factor_cell(self, sheet):
        k_factor_standard = None
        value_cell = None
        for cell in self.get_cells(sheet):
            content = sheet.get(cell)
            if content == "Radius / Thickness":
                key_cell = cell
            try:
                m = re.search(r"(K-[fF]actor)\s?\(?([a-zA-Z]*)\)?", content)
                if m:
                    value_cell = cell
                    k_factor_standard = m.group(2).lower() or None
            except:
                pass
        return value_cell, k_factor_standard

    def build_k_factor_lookup(self, sheet, key_column_name, key_column_row, value_column_name):
        k_factor_lookup = {}
        for i in range(key_column_row + 1, 1000):
            try:
                key = float(sheet.get(key_column_name + str(i)))
                value = float(sheet.get(value_column_name + str(i)))
                k_factor_lookup[key] = value
            except (ValueError, TypeError):
                break
        return k_factor_lookup

    def get_k_factor_standard(self, sheet, options_cell, k_factor_standard):
        if options_cell is not None:
            opt_col, opt_row = self.get_cell_tuple(options_cell)
            i = 1
            while True:
                opt_key_cell = "%s%i" % (opt_col, opt_row + i)
                next_col = chr(ord(opt_col) + 1)
                opt_value_cell = "%s%i" % (next_col, opt_row + i)
                i += 1
                try:
                    option = sheet.get(opt_key_cell)
                    value = sheet.get(opt_value_cell)
                    if option == "K-factor standard":
                        if k_factor_standard is not None:
                            raise ValueError("Multiple K-factor definitions found")
                        k_factor_standard = value.lower()
                except:
                    break
        if k_factor_standard is None:
            raise ValueError("'K-factor standard' option is required (ANSI or DIN)")
        return k_factor_standard
