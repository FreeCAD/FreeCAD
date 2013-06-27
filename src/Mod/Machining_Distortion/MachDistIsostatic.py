#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013 - Juergen Riegel <FreeCAD@juergen-riegel.net>      *  
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

import FreeCAD, Fem

if FreeCAD.GuiUp:
    import FreeCADGui
    from FreeCAD import Vector
    from PyQt4 import QtCore, QtGui
    from pivy import coin

__title__="Machine-Distortion Isostatic managment"
__author__ = "Juergen Riegel"
__url__ = "http://free-cad.sourceforge.net"



class _CommandIsostatic:
    "the MachDist Isostatic command definition"
    def GetResources(self):
        return {'Pixmap'  : 'MachDist_Isostatic',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("MachDist_Isostatic","Machine-Distortion Isostatic"),
                'Accel': "A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("MachDist_Isostatic","Add or edit a Machine-Distortion Isostatic")}
        
    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Isostatic")
        FreeCADGui.doCommand("import MachDist")
        FreeCADGui.doCommand("axe = MachDist.makeIsostatic()")
        FreeCADGui.doCommand("MachDist.makeStructuralSystem(" + MachDistCommands.getStringList(st) + ",[axe])")
        FreeCADGui.doCommand("MachDist.makeIsostatic()")
        FreeCAD.ActiveDocument.commitTransaction()

    def IsActive(self):
        if FemGui.getActiveAnalysis():
            return True
        else:
            return False
       

FreeCADGui.addCommand('MachDist_Isostatic',_CommandIsostatic())
