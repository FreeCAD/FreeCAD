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
    import FreeCADGui, FemGui
    from FreeCAD import Vector
    from PyQt4 import QtCore, QtGui
    from pivy import coin
    import PyQt4.uic as uic

__title__="Machine-Distortion Alignment managment"
__author__ = "Juergen Riegel"
__url__ = "http://free-cad.sourceforge.net"



class _CommandAlignment:
    "the MachDist Alignment command definition"
    def GetResources(self):
        return {'Pixmap'  : 'MachDist_Align',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("MachDist_Alignment","Part Alignment"),
                'Accel': "A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("MachDist_Alignment","Part Alignment")}
        
    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Alignment")
        taskd = _AlignTaskPanel()
        FreeCADGui.Control.showDialog(taskd)
        FreeCAD.ActiveDocument.commitTransaction()
       
    def IsActive(self):
        if FemGui.getActiveAnalysis():
            return True
        else:
            return False

            
class _AlignTaskPanel:
    '''The editmode TaskPanel for Material objects'''
    def __init__(self):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.
        form_class, base_class = uic.loadUiType(FreeCAD.getHomePath() + "Mod/Machining_Distortion/Aligment.ui")

        self.obj = None
        self.formUi = form_class()
        self.form = QtGui.QWidget()
        self.formUi.setupUi(self.form)

        #Connect Signals and Slots
        #QtCore.QObject.connect(form.button_select_files, QtCore.SIGNAL("clicked()"), self.select_files)

        self.update()


    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok) | int(QtGui.QDialogButtonBox.Cancel)
    
    def update(self):
        'fills the widgets'
        return 
                
    def accept(self):
        FreeCADGui.Control.closeDialog()
                    
    def reject(self):
        FreeCADGui.Control.closeDialog()
                    

FreeCADGui.addCommand('MachDist_Alignment',_CommandAlignment())
