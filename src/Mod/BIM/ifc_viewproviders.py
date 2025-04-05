# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 Yorik van Havre <yorik@uncreated.net>              *
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

"""This module contains placeholders for viewproviders provided by the NativeIFC addon"""

import FreeCAD

class ifc_vp_object:
    """NativeIFC class placeholder"""
    def __init__(self):
        pass

class ifc_vp_document:
    """NativeIFC class placeholder"""
    def __init__(self):
        pass
    def attach(self, vobj):
        FreeCAD.Console.PrintWarning("Warning: Object "+vobj.Object.Label+" depends on the NativeIFC addon which is not installed, and might not display correctly in the 3D view\n")
        return

class ifc_vp_group:
    """NativeIFC class placeholder"""
    def __init__(self):
        pass

class ifc_vp_material:
    """NativeIFC class placeholder"""
    def __init__(self):
        pass
