# Image gui init module
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

import FreeCAD


class ImageWorkbench ( Workbench ):
    "Image workbench object"
    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/Image/Resources/icons/ImageWorkbench.svg"
        self.__class__.MenuText = "Image"
        self.__class__.ToolTip = "Image workbench"

    def Initialize(self):
        # load the module
        import ImageGui
        
        try:
            import ImageTools._CommandImageScaling
        except ImportError as err:
            FreeCAD.Console.PrintError("Features from ImageTools package cannot be loaded. {err}\n".format(err= str(err)))        
        
    def GetClassName(self):
        return "ImageGui::Workbench"

Gui.addWorkbench(ImageWorkbench())

# Append the open handler
FreeCAD.EndingAdd("Image formats (*.bmp *.jpg *.png *.xpm)","ImageGui")
