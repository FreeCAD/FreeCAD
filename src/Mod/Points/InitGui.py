# Points gui init module
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



class PointsWorkbench ( Workbench ):
	"Points workbench object"
	Icon = """
			/* XPM */
			static const char *test_icon[]={
			"16 16 2 1",
			"a c #000000",
			". c None",
			"......##......##",
			"............##..",
			"..##....##......",
			"......##.....##.",
			"....##....##....",
			"##..............",
			"....##....##....",
			".......##.......",
			"...##......##...",
			".....##.........",
			".........##.....",
			"...##........##.",
			".....##.........",
			".........##.....",
			"...##......##...",
			"................"};
			"""
	MenuText = "Points"
	ToolTip = "Points workbench"

	def Initialize(self):
		# load the module
		import PointsGui
	def GetClassName(self):
		return "PointsGui::Workbench"

Gui.addWorkbench(PointsWorkbench())
