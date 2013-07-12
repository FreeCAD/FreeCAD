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

__title__="Machine-Distortion Mesh managment"
__author__ = "Juergen Riegel"
__url__ = "http://free-cad.sourceforge.net"



class _CommandMesh:
    "the MachDist Mesh command definition"
    def GetResources(self):
        return {'Pixmap'  : 'MachDist_AddFemMesh',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("MachDist_Mesh","Add Part"),
                'Accel': "M",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("MachDist_Mesh","Add a Part to the Analysis.")}
        
    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Add part")
        FreeCADGui.addModule("FemGui")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Fem::FemMeshObject")):
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Member = App.activeDocument().ActiveObject.Member + [App.activeDocument()."+sel[0].Name+"]")
            if(sel[0].isDerivedFrom("Part::Feature")):
                FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject','"+sel[0].Name +"_Mesh')")
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Shape = App.activeDocument()."+sel[0].Name)                
                FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().ActiveObject]")
                FreeCADGui.doCommand("Gui.activeDocument().hide('"+sel[0].Name+"')")
                FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")
        else:
            import os
            filename = QtGui.QFileDialog.getOpenFileName(QtGui.qApp.activeWindow(),'Open part file..',os.getcwd(),'Mesh or Part files (*.bdf *.unv *.med *.dat *.stp *.igs);;All files(*)')
            if filename.right(3)  in (u'bdf',u'med',u'dat',u'unv'):
                FreeCADGui.addModule("Fem")
                FreeCADGui.doCommand("Fem.insert('" + str(filename) +"','"+FreeCAD.ActiveDocument.Name+ "')")
                FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().ActiveObject]")
                FreeCADGui.doCommand("Gui.SendMsgToActiveView('ViewFit')")
            if filename.right(3) in (u'stp',u'igs'):
                FreeCADGui.addModule("Part")
                FreeCADGui.doCommand("Part.insert('" + str(filename) +"','"+FreeCAD.ActiveDocument.Name+ "')")
                FreeCADGui.doCommand("Gui.SendMsgToActiveView('ViewFit')")
                name = FreeCAD.activeDocument().ActiveObject.Name
                FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject','"+name +"_Mesh')")
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Shape = App.activeDocument()."+name)                
                FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().ActiveObject]")
                FreeCADGui.doCommand("Gui.activeDocument().hide('"+name+"')")
                FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")
            
            
            

        FreeCAD.ActiveDocument.commitTransaction()
       
    def IsActive(self):
        import FemGui
        # check if a active analysis is present and no Mesh in it
        if FemGui.getActiveAnalysis() != None:
            for i in FemGui.getActiveAnalysis().Member:
                if i.isDerivedFrom("Fem::FemMeshObject"):
                    return False
            return True
        else:
            return False

FreeCADGui.addCommand('MachDist_Mesh',_CommandMesh())
