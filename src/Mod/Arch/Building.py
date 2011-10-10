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

__title__="FreeCAD Building"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeBuilding(objectslist,join=False,name="Building"):
    '''makeBuilding(objectslist,[joinmode]): creates a building including the
    objects from the given list. If joinmode is True, components will be joined.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    Building(obj)
    ViewProviderBuilding(obj.ViewObject)
    obj.Components = objectslist
    for comp in obj.Components:
        comp.ViewObject.hide()
    obj.JoinMode = join
    return obj

class CommandBuilding:
    "the Arch Building command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Building',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Building","Building"),
                'Accel': "B, U",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Building","Creates a building object including selected objects.")}
        
    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Building")
        makeBuilding(FreeCADGui.Selection.getSelection())
        FreeCAD.ActiveDocument.commitTransaction()

class Building(Cell.Cell):
    "The Building object"
    def __init__(self,obj):
        Cell.Cell.__init__(self,obj)
        self.Type = "Building"
        
class ViewProviderBuilding(Cell.ViewProviderCell):
    "A View Provider for the Building object"
    def __init__(self,vobj):
        Cell.ViewProviderCell.__init__(self,vobj)

    def getIcon(self):
        return """
                /* XPM */
                static char * Arch_Building_xpm[] = {
                "16 16 9 1",
                " 	c None",
                ".	c #160E0A",
                "+	c #C10007",
                "@	c #FF0006",
                "#	c #8F3F00",
                "$	c #5E5F5D",
                "%	c #7F817E",
                "&	c #A0A29F",
                "*	c #F4F6F3",
                "                ",
                "     ........   ",
                "    ..#@@@@@.   ",
                "   .&&.+@@@@@.  ",
                "  .&**%.@@@@@+. ",
                " .&****..@@@+...",
                ".%******.##..$$.",
                ".&******&.$&**%.",
                ".%*...$**.****% ",
                ".%*..#.**.****% ",
                " %*..#.**.****$ ",
                " $*..#.**.***$. ",
                " $*..#$**.**..  ",
                " .$...$**.&.    ",
                "   .  .$%..     ",
                "        ..      "};
                """

FreeCADGui.addCommand('Arch_Building',CommandBuilding())
