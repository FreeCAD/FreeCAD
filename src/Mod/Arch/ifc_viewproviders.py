#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2022 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
#*   as published by the Free Software Foundation; either version 3 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU General Public License for more details.                          *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import FreeCAD

class ifc_vp_object:
    """NativeIFC class placeholder"""
    def __init__(self):
        pass
    def attach(self, vobj):
        return
    def getDisplayModes(self, obj):
        return []
    def getDefaultDisplayMode(self):
        return "FlatLines"
    def setDisplayMode(self,mode):
        return mode
    def __getstate__(self):
        return None
    def __setstate__(self, state):
        return None

class ifc_vp_document(ifc_vp_object):
    """NativeIFC class placeholder"""
    def __init__(self):
        pass
    def attach(self, vobj):
        FreeCAD.Console.PrintWarning("Warning: Object "+vobj.Object.Label+" depends on the NativeIFC addon which is not installed, and will not display correctly in the 3D view\n")
        return
