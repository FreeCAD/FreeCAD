# Raytracing gui init module
# (c) 2003 Juergen Riegel
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up

#***************************************************************************
#*   (c) Juergen Riegel (juergen.riegel@web.de) 2002                       *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#*   Juergen Riegel 2002                                                   *
#***************************************************************************/



class RaytracingWorkbench ( Workbench ):
	"Raytracing workbench object"
	Icon = """
                        /* XPM */
                        static char * raytracing_xpm[] = {
                        "16 16 9 1",
                        " 	c None",
                        ".	c #000100",
                        "+	c #0C1731",
                        "@	c #1D3154",
                        "#	c #29476F",
                        "$	c #3C6291",
                        "%	c #7394BB",
                        "&	c #B0BFD3",
                        "*	c #FBFDFB",
                        "                ",
                        "      $$        ",
                        "    %&%$$$#     ",
                        "  %&***$$###    ",
                        "  %****$###@@   ",
                        " $%***%###@@@   ",
                        " $$%&$###@@@+   ",
                        " $$$####@@@++   ",
                        " $$####@@@+++   ",
                        " $####@@@++++   ",
                        "  ###@@@+++++   ",
                        "  ##@@@+++++    ",
                        "   @@@+++++...  ",
                        "     ++++.....  ",
                        "                ",
                        "                "};
			"""
	MenuText = "Raytracing"
	ToolTip = "Raytracing workbench"

	def Initialize(self):
		# load the module
		import PartGui
		import RaytracingGui
	def GetClassName(self):
		return "RaytracingGui::Workbench"

Gui.addWorkbench(RaytracingWorkbench())

FreeCAD.addImportType("Povray format (*.pov *.inc)","RaytracingGui")
