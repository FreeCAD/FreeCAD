# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 - Bernd Hahnebach <bernd@bimstatik.org>            *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "_FemViewProviderSolverCalculix"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


import FreeCAD
import FreeCADGui
import FemGui


class _ViewProviderFemSolverCalculix:
    "A View Provider for the FemSolverCalculix object"
    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-solver.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode=0):
        import _TaskPanelFemSolverCalculix
        taskd = _TaskPanelFemSolverCalculix._TaskPanelFemSolverCalculix(self.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return

    def doubleClicked(self, vobj):
        doc = FreeCADGui.getDocument(vobj.Object.Document)
        if not doc.getInEdit():
            # may be go the other way around and just activate the analysis the user has doubleClicked on ?!
            if FemGui.getActiveAnalysis() is not None:
                if FemGui.getActiveAnalysis().Document is FreeCAD.ActiveDocument:
                    if self.Object in FemGui.getActiveAnalysis().Member:
                        doc.setEdit(vobj.Object.Name)
                    else:
                        FreeCAD.Console.PrintError('Activate the analysis this solver belongs to!\n')
                else:
                    FreeCAD.Console.PrintError('Active Analysis is not in active Document!\n')
            else:
                FreeCAD.Console.PrintError('No active Analysis found!\n')
        else:
            FreeCAD.Console.PrintError('Active Task Dialog found! Please close this one first!\n')
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None
