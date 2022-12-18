# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
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

"""Mock objects for use when testing the addon manager non-GUI code."""

import os
import shutil


class MockAddon:
    """Minimal Addon class"""

    def __init__(self):
        self.name = "TestAddon"
        self.url = "https://github.com/FreeCAD/FreeCAD-addons"
        self.branch = "master"
        self.macro = None


class MockMacro:
    """Minimal Macro class"""

    def __init__(self):
        self.name = "MockMacro"
        self.filename = self.name + ".FCMacro"
        self.icon = ""  # If set, should just be fake filename, doesn't have to exist
        self.xpm = ""
        self.other_files = []  # If set, should be fake names, don't have to exist

    def install(self, location: os.PathLike):
        """Installer function for the mock macro object: creates a file with the src_filename
        attribute, and optionally an icon, xpm, and other_files. The data contained in these files
        is not usable and serves only as a placeholder for the existence of the files."""

        with open(
            os.path.join(location, self.filename),
            "w",
            encoding="utf-8",
        ) as f:
            f.write("Test file for macro installation unit tests")
        if self.icon:
            with open(os.path.join(location, self.icon), "wb") as f:
                f.write(b"Fake icon data - nothing to see here\n")
        if self.xpm:
            with open(
                os.path.join(location, "MockMacro_icon.xpm"), "w", encoding="utf-8"
            ) as f:
                f.write(self.xpm)
        for name in self.other_files:
            if "/" in name:
                new_location = os.path.dirname(os.path.join(location, name))
                os.makedirs(new_location, exist_ok=True)
            with open(os.path.join(location, name), "w", encoding="utf-8") as f:
                f.write("# Fake macro data for unit testing\n")
        return True, []
