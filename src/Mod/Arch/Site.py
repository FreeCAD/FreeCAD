#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
#*   Yorik van Havre <yorik@uncreated.net>                                 *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
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

import Cell,FreeCAD,FreeCADGui
from PyQt4 import QtCore

__title__="FreeCAD Site"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeSite(objectslist,name="Site"):
    '''makeBuilding(objectslist): creates a site including the
    objects from the given list.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    Site(obj)
    ViewProviderSite(obj.ViewObject)
    obj.Components = objectslist
    obj.JoinMode = False
    return obj

class CommandSite:
    "the Arch Site command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Site',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Site","Site"),
                'Accel': "S, I",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Site","Creates a site object including selected objects.")}
        
    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Site")
        makeSite(FreeCADGui.Selection.getSelection())
        FreeCAD.ActiveDocument.commitTransaction()

class Site(Cell.Cell):
    "The Site object"
    def __init__(self,obj):
        Cell.Cell.__init__(self,obj)
        self.Type = "Site"
        
    def execute(self,obj):
        pass
        
    def onChanged(self,obj,prop):
        pass
    
class ViewProviderSite(Cell.ViewProviderCell):
    "A View Provider for the Site object"
    def __init__(self,vobj):
        Cell.ViewProviderCell.__init__(self,vobj)

    def getIcon(self):
        return """
                /* XPM */
                static char * Arch_Site_xpm[] = {
                "16 16 9 1",
                " 	c None",
                ".	c #49370C",
                "+	c #204F0E",
                "@	c #535219",
                "#	c #6F551D",
                "$	c #127210",
                "%	c #049512",
                "&	c #08BD16",
                "*	c #00EB1B",
                "        +$++    ",
                "       $%%%$++++",
                "      $&&%%$$$+@",
                "     $&&&%%$$+@.",
                "     &&&&%%$$+#.",
                "    $*&&&%%$+##.",
                "    &*&&&%%$@##.",
                "   %**&&&%%+###.",
                "  %***&&&%$@###.",
                " %****&&&%+##.  ",
                "+&****&&&$@#.   ",
                ".#+%&*&&$@#.    ",
                " ..#@$%$@#.     ",
                "   ..#@@.       ",
                "      ..        ",
                "                "};
                """

FreeCADGui.addCommand('Arch_Site',CommandSite())
