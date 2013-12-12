# Mesh gui init module
# (c) 2004 Werner Mayer
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up

#***************************************************************************
#*   (c) Werner Mayer <werner.wm.mayer@gmx.de> 2004                        *
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
#*   Werner Mayer 2004                                                     *
#***************************************************************************/



class MeshWorkbench ( Workbench ):
	"Mesh workbench object"
	Icon = """
                /* XPM */
                static char * mesh_xpm[] = {
                "16 16 9 1",
                " 	c None",
                ".	c #000200",
                "+	c #002001",
                "@	c #003A00",
                "#	c #006500",
                "$	c #018400",
                "%	c #00B000",
                "&	c #00CD00",
                "*	c #00F600",
                "                ",
                "          ..    ",
                "    ....   ..   ",
                "   ..&$@.  ..   ",
                "  .&+&**.  .$.  ",
                " .&*+&**.. .*+  ",
                " .&*+&**.. .&$. ",
                " .**@&**.+..%%. ",
                " .**@%**.@$@$&. ",
                " .**@#%&.@#$#*. ",
                " .*%.....@#&.&. ",
                " .$.    .+@*#.. ",
                " ..      .+**+. ",
                " .       .+*&@. ",
                "         .+@..  ",
                "          ..    "};
		"""
	MenuText = "Mesh design"
	ToolTip = "Mesh design workbench"

	def Initialize(self):
		# load the module
		import MeshGui
	def GetClassName(self):
		return "MeshGui::Workbench"

Gui.addWorkbench(MeshWorkbench)
