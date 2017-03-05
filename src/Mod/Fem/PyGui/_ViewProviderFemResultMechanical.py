#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - Qingfeng Xia <qingfeng.xia()eng.ox.ac.uk> *
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

__title__ = "ViewProvider for FEM mechanical ResultObjectPython"
__author__ = "Qingfeng Xia, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package _ViewProviderFemResultMechanical
#  \ingroup FEM
#  \brief FreeCAD ViewProvider for mechanical ResultObjectPython in FEM workbench

import FreeCAD
import FreeCADGui
import FemGui
from PySide import QtGui


class _ViewProviderFemResultMechanical:
    """A View Provider for the FemResultObject Python dervied FemResult class
    """

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        """after load from FCStd file, self.icon does not exist, return constant path instead"""
        return ":/icons/fem-result.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def doubleClicked(self, vobj):
        if FreeCADGui.activeWorkbench().name() != 'FemWorkbench':
            FreeCADGui.activateWorkbench("FemWorkbench")
        doc = FreeCADGui.getDocument(vobj.Object.Document)
        if not doc.getInEdit():
            doc.setEdit(vobj.Object.Name)
        else:
            FreeCAD.Console.PrintError('Active Task Dialog found! Please close this one first!\n')
        return True

    def setEdit(self, vobj, mode=0):

        if FemGui.getActiveAnalysis() is not None:
            if hasattr(self.Object, "Mesh") and self.Object.Mesh:
                mem = FemGui.getActiveAnalysis().Member
                if self.Object in mem:
                    if self.Object.Mesh in mem:
                        import PyGui._TaskPanelFemResultShow
                        taskd = PyGui._TaskPanelFemResultShow._TaskPanelFemResultShow(self.Object)
                        taskd.obj = vobj.Object
                        FreeCADGui.Control.showDialog(taskd)
                        return True
                    else:
                        error_message = 'FEM: Result mesh object is not in active analysis.\n'
                        FreeCAD.Console.PrintError(error_message)
                        QtGui.QMessageBox.critical(None, 'Not in activate analysis', error_message)
                        return False
                else:
                    error_message = 'FEM: Result object is not in active analysis.\n'
                    FreeCAD.Console.PrintError(error_message)
                    QtGui.QMessageBox.critical(None, 'Not in activate analysis', error_message)
                    return False
            else:
                error_message = 'FEM: Result object has no appropriate FEM mesh.\n'
                FreeCAD.Console.PrintError(error_message)
                QtGui.QMessageBox.critical(None, 'No result object', error_message)
                return False
        else:
            error_message = 'FEM: No active analysis found! Please activate the analysis you would like to view results for.\n'
            FreeCAD.Console.PrintError(error_message)
            QtGui.QMessageBox.critical(None, 'No activate analysis', error_message)
            return False

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None
