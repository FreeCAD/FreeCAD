#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              * 
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

# add Import/Export types
App.addImportType("Autodesk DXF 2D (*.dxf)","importDXF") 
App.addImportType("SVG as geometry (*.svg)","importSVG")
App.addImportType("Open CAD Format (*.oca *.gcad)","importOCA")
App.addImportType("Common airfoil data (*.dat)","importAirfoilDAT")
App.addExportType("Autodesk DXF 2D (*.dxf)","importDXF")
App.addExportType("Flattened SVG (*.svg)","importSVG")
App.addExportType("Open CAD Format (*.oca)","importOCA")
App.addImportType("Autodesk DWG 2D (*.dwg)","importDWG") 
App.addExportType("Autodesk DWG 2D (*.dwg)","importDWG")
