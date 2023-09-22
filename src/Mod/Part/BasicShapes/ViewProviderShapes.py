# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

__title__ = "BasicShapes.ViewProviderShapes"
__author__ = "Werner Mayer"
__url__ = "http://www.freecad.org"
__doc__ = "Basic shapes"


import os
import FreeCAD
import FreeCADGui

from PySide import QtGui

class ViewProviderTube:
    def __init__(self, obj):
        ''' Set this object to the proxy object of the actual view provider '''
        obj.Proxy = self
        obj.addExtension("PartGui::ViewProviderAttachExtensionPython")
        obj.setIgnoreOverlayIcon(True, "PartGui::ViewProviderAttachExtensionPython")

    def attach(self, obj):
        ''' Setup the scene sub-graph of the view provider, this method is mandatory '''
        return

    def setupContextMenu(self, viewObject, menu):
        action = menu.addAction(FreeCAD.Qt.translate("QObject", "Edit %1").replace("%1", viewObject.Object.Label))
        action.triggered.connect(lambda: self.startDefaultEditMode(viewObject))
        return False

    def startDefaultEditMode(self, viewObject):
        document = viewObject.Document.Document
        if not document.HasPendingTransaction:
            text = FreeCAD.Qt.translate("QObject", "Edit %1").replace("%1", viewObject.Object.Label)
            document.openTransaction(text)
        viewObject.Document.setEdit(viewObject.Object, 0)

    def setEdit(self, viewObject, mode):
        if mode == 0:
            FreeCADGui.Control.showDialog(TaskTubeUI(viewObject))
            return True

    def unsetEdit(self, viewObject, mode):
        if mode == 0:
            FreeCADGui.Control.closeDialog()
            return True

    def getIcon(self):
        return ":/icons/parametric/Part_Tube_Parametric.svg"

    def dumps(self):
        return None

    def loads(self,state):
        return None


class TaskTubeUI:
    """A default task panel for editing tube objects."""

    def __init__(self, viewObject):
        self.viewObject = viewObject
        ui_file = os.path.join(os.path.dirname(__file__), "TaskTube.ui")
        ui = FreeCADGui.UiLoader()
        self.form = ui.load(ui_file)

        object = self.viewObject.Object
        self.form.tubeOuterRadius.setProperty("rawValue", object.OuterRadius.Value)
        self.form.tubeInnerRadius.setProperty("rawValue", object.InnerRadius.Value)
        self.form.tubeHeight.setProperty("rawValue", object.Height.Value)

        self.form.tubeOuterRadius.valueChanged.connect(lambda x: self.onChangeOuterRadius(x))
        self.form.tubeInnerRadius.valueChanged.connect(lambda x: self.onChangeInnerRadius(x))
        self.form.tubeHeight.valueChanged.connect(lambda x: self.onChangeHeight(x))

        FreeCADGui.ExpressionBinding(self.form.tubeOuterRadius).bind(object,"OuterRadius")
        FreeCADGui.ExpressionBinding(self.form.tubeInnerRadius).bind(object,"InnerRadius")
        FreeCADGui.ExpressionBinding(self.form.tubeHeight).bind(object,"Height")

    def onChangeOuterRadius(self, radius):
        object = self.viewObject.Object
        object.OuterRadius = radius
        object.recompute()

    def onChangeInnerRadius(self, radius):
        object = self.viewObject.Object
        object.InnerRadius = radius
        object.recompute()

    def onChangeHeight(self, height):
        object = self.viewObject.Object
        object.Height = height
        object.recompute()

    def accept(self):
        object = self.viewObject.Object
        if not object.isValid():
            QtGui.QMessageBox.warning(None, "Error", object.getStatusString())
            return False
        document = self.viewObject.Document.Document
        document.commitTransaction()
        document.recompute()
        self.viewObject.Document.resetEdit()
        return True

    def reject(self):
        guidocument = self.viewObject.Document
        document = guidocument.Document
        document.abortTransaction()
        document.recompute()
        guidocument.resetEdit()
        return True
