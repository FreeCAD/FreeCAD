#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013-2015 - Juergen Riegel <FreeCAD@juergen-riegel.net> *
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

__title__ = "Command Mechanical Show Result"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

import FreeCAD

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui


class _CommandMechanicalShowResult:
    "the Fem JobControl command definition"
    def GetResources(self):
        return {'Pixmap': 'fem-result',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Result", "Show result"),
                'Accel': "S, R",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Result", "Show result information of an analysis")}

    def Activated(self):
        self.result_object = get_results_object(FreeCADGui.Selection.getSelection())

        if not self.result_object:
            QtGui.QMessageBox.critical(None, "Missing prerequisite", "No result found in active Analysis")
            return

        import _ResultControlTaskPanel
        taskd = _ResultControlTaskPanel._ResultControlTaskPanel()
        FreeCADGui.Control.showDialog(taskd)

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and results_present()


#Code duplidation - to be removed after migration to FemTools
def results_present():
    import FemGui
    results = False
    analysis_members = FemGui.getActiveAnalysis().Member
    for o in analysis_members:
        if o.isDerivedFrom('Fem::FemResultObject'):
            results = True
    return results


#Code duplidation - to be removed after migration to FemTools
def get_results_object(sel):
    import FemGui
    if (len(sel) == 1):
        if sel[0].isDerivedFrom("Fem::FemResultObject"):
            return sel[0]

    for i in FemGui.getActiveAnalysis().Member:
        if(i.isDerivedFrom("Fem::FemResultObject")):
            return i
    return None

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_ShowResult', _CommandMechanicalShowResult())
