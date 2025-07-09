# -*- coding: utf8 -*-
# FreeCAD InitGui script of the Export3DPDF module
# (c) 2024 FreeCAD Developers

# ***************************************************************************
# *   Copyright (c) 2024 FreeCAD Developers                                 *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

# Import the GUI module to register commands
try:
    import Export3DPDFGui
    FreeCAD.Console.PrintLog("Export3DPDF GUI module loaded successfully\n")
except ImportError as e:
    FreeCAD.Console.PrintError("Failed to load Export3DPDF GUI module: {}\n".format(str(e)))

# Register export type for 3D PDF in GUI layer
FreeCAD.changeExportModule("3D PDF (*.pdf)", "Export3DPDF", "Export3DPDFGui") 