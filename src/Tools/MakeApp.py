# FreeCAD MakeNewBuildNbr script
# (c) 2003 Werner Mayer
#
# Create a new application

#***************************************************************************
#*   (c) Werner Mayer (werner.wm.mayer@gmx.de) 2003                        *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#*   Werner Mayer 2003                                                     *
#***************************************************************************

import os,sys,string
import FCFileTools
import MakeAppTools


if(len(sys.argv) != 2):
	sys.stdout.write("Please enter a name for your application.\n")
	sys.exit()

Application = sys.argv[1]

# create directory ../Mod/<Application>
if not os.path.isdir("../Mod/"+Application):
    os.mkdir("../Mod/"+Application)
else:
	sys.stdout.write(Application + " already exists. Please enter another name.\n")
	sys.exit()


# copying files from _TEMPLATE_ to ../Mod/<Application>
sys.stdout.write("Copying files...") 
MakeAppTools.copyTemplate("_TEMPLATE_","../Mod/"+Application,"_TEMPLATE_", Application)
sys.stdout.write("Ok\n") 

# replace the _TEMPLATE_ string by <Application>
sys.stdout.write("Modifying files...\n")
MakeAppTools.replaceTemplate("../Mod/" + Application,"_TEMPLATE_",Application)
# make the congigure script executable
#os.chmod("../Mod/" + Application + "/configure", 0777);
sys.stdout.write("Modifying files done.\n")

sys.stdout.write(Application + " module created successfully.\n") 
