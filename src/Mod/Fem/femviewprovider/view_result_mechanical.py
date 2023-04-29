# ***************************************************************************
# *   Copyright (c) 2015 Qingfeng Xia <qingfeng.xia()eng.ox.ac.uk>          *
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD result mechanical ViewProvider for the document object"
__author__ = "Qingfeng Xia, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package view_result_mechanical
#  \ingroup FEM
#  \brief view provider for mechanical ResultObjectPython

import FreeCADGui

from PySide import QtGui
from femtaskpanels import task_result_mechanical
from . import view_base_femconstraint


class VPResultMechanical(view_base_femconstraint.VPBaseFemConstraint):
    """
    A View Provider for the ResultObject Python derived FemResult class
    """

    def setEdit(self, vobj, mode=0):
        view_base_femconstraint.VPBaseFemConstraint.setEdit(
            self,
            vobj,
            mode,
            task_result_mechanical._TaskPanel,
        )

    # overwrite unsetEdit, hide result mesh object on task panel exit
    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        # hide the mesh after result viewing is finished, but do not reset the coloring
        self.Object.Mesh.ViewObject.hide()
        return True

    def claimChildren(self):
        return [self.Object.Mesh]  # claimChildren needs to return a list !

    def onDelete(self, feature, subelements):
        children = self.claimChildren()
        filtered = filter(lambda obj: obj is not None, children)
        children = list(filtered)
        if len(children) > 0:
            # issue a warning
            bodyMessage = (
                "The results object is not empty, therefore the\n"
                "following referencing objects might be lost:\n"
            )
            for obj in children:
                bodyMessage += "\n" + obj.Label
            bodyMessage += "\n\nAre you sure you want to continue?"
            reply = QtGui.QMessageBox.warning(
                None,
                "Object dependencies",
                bodyMessage,
                QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                QtGui.QMessageBox.No
            )
            if reply == QtGui.QMessageBox.Yes:
                return True
            else:
                return False
        return True
