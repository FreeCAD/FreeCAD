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

__title__="Machine-Distortion Analysis managment"
__author__ = "Juergen Riegel"
__url__ = "http://free-cad.sourceforge.net"



class _CommandAnalysis:
    "the MachDist Analysis command definition"
    def GetResources(self):
        return {'Pixmap'  : 'MachDist_NewAnalysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("MachDist_Analysis","Machine-Distortion Analysis"),
                'Accel': "A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("MachDist_Analysis","Add or edit a Machine-Distortion Analysis")}
        
    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Analysis")
        FreeCADGui.addModule("FemGui")
        FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemAnalysis','MachineDistortion')")
        FreeCADGui.doCommand("FemGui.setActiveAnalysis(App.activeDocument().ActiveObject)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.Selection.clearSelection()
       
    def IsActive(self):
        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False


FreeCADGui.addCommand('MachDist_Analysis',_CommandAnalysis())
