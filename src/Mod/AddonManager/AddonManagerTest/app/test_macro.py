# -*- coding: utf-8 -*-

# ***************************************************************************
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Lesser General Public            *
# *   License as published by the Free Software Foundation; either          *
# *   version 2.1 of the License, or (at your option) any later version.    *
# *                                                                         *
# *   This library is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with this library; if not, write to the Free Software   *
# *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
# *   02110-1301  USA                                                       *
# *                                                                         *
# ***************************************************************************

import unittest
import os
import tempfile
import FreeCAD

from typing import Dict

from addonmanager_macro import Macro

class TestMacro(unittest.TestCase):

    MODULE = "test_macro"  # file name without extension

    def setUp(self):
        self.test_dir = os.path.join(FreeCAD.getHomePath(), "Mod", "AddonManager", "AddonManagerTest", "data")

    def test_basic_metadata(self):
        replacements = {
            "COMMENT": "test comment",
            "WEB": "https://test.url",
            "VERSION": "1.2.3",
            "AUTHOR": "Test Author",
            "DATE": "2022-03-09",
            "ICON": "testicon.svg",
        }
        m = self.generate_macro(replacements)
        self.assertEqual(m.comment, replacements["COMMENT"])
        self.assertEqual(m.url, replacements["WEB"])
        self.assertEqual(m.version, replacements["VERSION"])
        self.assertEqual(m.author, replacements["AUTHOR"])
        self.assertEqual(m.date, replacements["DATE"])
        self.assertEqual(m.icon, replacements["ICON"])
    
    def test_other_files(self):
        replacements = {
            "FILES": "file_a,file_b,file_c",
        }
        m = self.generate_macro(replacements)
        self.assertEqual(len(m.other_files), 3)
        self.assertEqual(m.other_files[0], "file_a")
        self.assertEqual(m.other_files[1], "file_b")
        self.assertEqual(m.other_files[2], "file_c")

        replacements = {
            "FILES": "file_a, file_b, file_c",
        }
        m = self.generate_macro(replacements)
        self.assertEqual(len(m.other_files), 3)
        self.assertEqual(m.other_files[0], "file_a")
        self.assertEqual(m.other_files[1], "file_b")
        self.assertEqual(m.other_files[2], "file_c")

        replacements = {
            "FILES": "file_a file_b file_c",
        }
        m = self.generate_macro(replacements)
        self.assertEqual(len(m.other_files), 1)
        self.assertEqual(m.other_files[0], "file_a file_b file_c")

    def test_version_from_string(self):
        replacements = {
            "VERSION": "1.2.3",
        }
        m = self.generate_macro(replacements)
        self.assertEqual(m.version, "1.2.3")

    def test_version_from_date(self):
        replacements = {
            "DATE": "2022-03-09",
        }
        outfile = self.generate_macro_file(replacements)
        with open(outfile) as f:
            lines = f.readlines()
            output_lines = []
            for line in lines:
                if "VERSION" in line:
                    line = "__Version__ = __Date__"
                output_lines.append(line)
        with open(outfile,"w") as f:
            f.write("\n".join(output_lines))
        m = Macro("Unit Test Macro")
        m.fill_details_from_file(outfile)
        self.assertEqual(m.version, "2022-03-09")

    def test_version_from_float(self):
        outfile = self.generate_macro_file()
        with open(outfile) as f:
            lines = f.readlines()
            output_lines = []
            for line in lines:
                if "VERSION" in line:
                    line = "__Version__ = 1.23"
                output_lines.append(line)
        with open(outfile,"w") as f:
            f.write("\n".join(output_lines))
        m = Macro("Unit Test Macro")
        m.fill_details_from_file(outfile)
        self.assertEqual(m.version, "1.23")

    def test_version_from_int(self):
        outfile = self.generate_macro_file()
        with open(outfile) as f:
            lines = f.readlines()
            output_lines = []
            for line in lines:
                if "VERSION" in line:
                    line = "__Version__ = 1"
                output_lines.append(line)
        with open(outfile,"w") as f:
            f.write("\n".join(output_lines))
        m = Macro("Unit Test Macro")
        m.fill_details_from_file(outfile)
        self.assertEqual(m.version, "1")

    def test_xpm(self):
        outfile = self.generate_macro_file()
        xpm_data = """/* XPM */
static char * blarg_xpm[] = {
"16 7 2 1",
"* c #000000",
". c #ffffff",
"**..*...........",
"*.*.*...........",
"**..*..**.**..**",
"*.*.*.*.*.*..*.*",
"**..*..**.*...**",
"...............*",
".............**."
};"""
        with open(outfile) as f:
            contents = f.read()
            contents += f"\n__xpm__ = \"\"\"{xpm_data}\"\"\"\n"

        with open(outfile,"w") as f:
            f.write(contents)
        m = Macro("Unit Test Macro")
        m.fill_details_from_file(outfile)
        self.assertEqual(m.xpm, xpm_data)


    def generate_macro_file(self, replacements:Dict[str,str] = {}) -> os.PathLike:
        with open(os.path.join(self.test_dir,"macro_template.FCStd")) as f:
            lines = f.readlines()
            outfile = tempfile.NamedTemporaryFile(mode="wt",delete=False)
            for line in lines:
                for key,value in replacements.items():
                    line = line.replace(key,value)

                outfile.write(line)
            outfile.close()
            return outfile.name

    def generate_macro(self, replacements:Dict[str,str] = {}) -> Macro:
        outfile = self.generate_macro_file(replacements)
        m = Macro("Unit Test Macro")
        m.fill_details_from_file(outfile)
        os.unlink(outfile)
        return m