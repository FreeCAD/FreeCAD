# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   Copyright (c) 2022 FreeCAD Project Association                             #
#   Copyright (c) 2024 Werner Mayer                                            #
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
Helper module to convert zip files
"""


import zipfile

def rewrite(source: str, target: str):
    """Copy a zip file source to target."""

    source_zip = zipfile.ZipFile(source, "r")
    target_zip = zipfile.ZipFile(target, "w")

    for name in source_zip.namelist():
        target_zip.writestr(name, source_zip.open(name).read())

    source_zip.close()
    target_zip.close()
