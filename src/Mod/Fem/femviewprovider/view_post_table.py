# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
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

__title__ = "FreeCAD FEM postprocessing table ViewProvider for the document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package view_post_table
#  \ingroup FEM
#  \brief view provider for post table object

import FreeCAD
import FreeCADGui

from PySide import QtGui, QtCore
from PySide.QtCore import QT_TRANSLATE_NOOP

from . import view_base_fempostextractors
from . import view_base_fempostvisualization
from femtaskpanels import task_post_table
from femguiutils import vtk_table_view as vtv

from . import view_base_femobject

_GuiPropHelper = view_base_femobject._GuiPropHelper


class EditViewWidget(QtGui.QWidget):

    def __init__(self, obj, post_dialog):
        super().__init__()

        self._object = obj
        self._post_dialog = post_dialog

        # load the ui and set it up
        self.widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/PostTableFieldViewEdit.ui"
        )
        layout = QtGui.QVBoxLayout()
        layout.addWidget(self.widget)
        self.setLayout(layout)

        self.__init_widget()

    def __init_widget(self):
        # set the other properties
        self.widget.Name.setText(self._object.ViewObject.Name)
        self.widget.Name.editingFinished.connect(self.legendChanged)

    @QtCore.Slot()
    def legendChanged(self):
        self._object.ViewObject.Name = self.widget.Name.text()


class EditFieldAppWidget(QtGui.QWidget):

    def __init__(self, obj, post_dialog):
        super().__init__()

        self._object = obj
        self._post_dialog = post_dialog

        # load the ui and set it up (we reuse histogram, as we need the exact same)
        self.widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/PostHistogramFieldAppEdit.ui"
        )
        layout = QtGui.QVBoxLayout()
        layout.addWidget(self.widget)
        self.setLayout(layout)

        self.__init_widget()

    def __init_widget(self):
        # set the other properties

        self._post_dialog._enumPropertyToCombobox(self._object, "XField", self.widget.Field)
        self._post_dialog._enumPropertyToCombobox(self._object, "XComponent", self.widget.Component)
        self.widget.Extract.setChecked(self._object.ExtractFrames)

        self.widget.Field.activated.connect(self.fieldChanged)
        self.widget.Component.activated.connect(self.componentChanged)
        self.widget.Extract.toggled.connect(self.extractionChanged)

    @QtCore.Slot(int)
    def fieldChanged(self, index):
        self._object.XField = index
        self._post_dialog._enumPropertyToCombobox(self._object, "XComponent", self.widget.Component)
        self._post_dialog._recompute()

    @QtCore.Slot(int)
    def componentChanged(self, index):
        self._object.XComponent = index
        self._post_dialog._recompute()

    @QtCore.Slot(bool)
    def extractionChanged(self, extract):
        self._object.ExtractFrames = extract
        self._post_dialog._recompute()


class EditIndexAppWidget(QtGui.QWidget):

    def __init__(self, obj, post_dialog):
        super().__init__()

        self._object = obj
        self._post_dialog = post_dialog

        # load the ui and set it up (we reuse histogram, as we need the exact same)
        self.widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/PostHistogramIndexAppEdit.ui"
        )
        layout = QtGui.QVBoxLayout()
        layout.addWidget(self.widget)
        self.setLayout(layout)

        self.__init_widget()

    def __init_widget(self):
        # set the other properties

        self.widget.Index.setValue(self._object.Index)
        self._post_dialog._enumPropertyToCombobox(self._object, "XField", self.widget.Field)
        self._post_dialog._enumPropertyToCombobox(self._object, "XComponent", self.widget.Component)

        self.widget.Index.valueChanged.connect(self.indexChanged)
        self.widget.Field.activated.connect(self.fieldChanged)
        self.widget.Component.activated.connect(self.componentChanged)

        # sometimes weird sizes occur with spinboxes
        self.widget.Index.setMaximumHeight(self.widget.Field.sizeHint().height())

    @QtCore.Slot(int)
    def fieldChanged(self, index):
        self._object.XField = index
        self._post_dialog._enumPropertyToCombobox(self._object, "XComponent", self.widget.Component)
        self._post_dialog._recompute()

    @QtCore.Slot(int)
    def componentChanged(self, index):
        self._object.XComponent = index
        self._post_dialog._recompute()

    @QtCore.Slot(int)
    def indexChanged(self, value):
        self._object.Index = value
        self._post_dialog._recompute()


class VPPostTableFieldData(view_base_fempostextractors.VPPostExtractor):
    """
    A View Provider for extraction of 1D field data specially for tables
    """

    def __init__(self, vobj):
        super().__init__(vobj)

    def _get_properties(self):

        prop = [
            _GuiPropHelper(
                type="App::PropertyString",
                name="Name",
                group="Table",
                doc=QT_TRANSLATE_NOOP(
                    "FEM", "The name used in the table header. Default name is used if empty"
                ),
                value="",
            ),
        ]
        return super()._get_properties() + prop

    def attach(self, vobj):
        self.Object = vobj.Object
        self.ViewObject = vobj

    def getIcon(self):
        return ":/icons/FEM_PostField.svg"

    def get_app_edit_widget(self, post_dialog):
        return EditFieldAppWidget(self.Object, post_dialog)

    def get_view_edit_widget(self, post_dialog):
        return EditViewWidget(self.Object, post_dialog)

    def get_preview(self):
        name = QT_TRANSLATE_NOOP("FEM", "default")
        if self.ViewObject.Name:
            name = self.ViewObject.Name
        return (QtGui.QPixmap(), name)

    def get_default_color_property(self):
        return None


class VPPostTableIndexOverFrames(VPPostTableFieldData):
    """
    A View Provider for extraction of 1D index over frames data
    """

    def __init__(self, vobj):
        super().__init__(vobj)

    def getIcon(self):
        return ":/icons/FEM_PostIndex.svg"

    def get_app_edit_widget(self, post_dialog):
        return EditIndexAppWidget(self.Object, post_dialog)


class VPPostTable(view_base_fempostvisualization.VPPostVisualization):
    """
    A View Provider for Table plots
    """

    def __init__(self, vobj):
        super().__init__(vobj)

    def getIcon(self):
        return ":/icons/FEM_PostSpreadsheet.svg"

    def setEdit(self, vobj, mode):

        # build up the task panel
        taskd = task_post_table._TaskPanel(vobj)

        # show it
        FreeCADGui.Control.showDialog(taskd)

        return True

    def show_visualization(self):

        if not hasattr(self, "_tableview") or not self._tableview:
            main = FreeCADGui.getMainWindow()
            self._tableModel = vtv.VtkTableModel()
            self._tableview = vtv.VtkTableView(self._tableModel)
            self._tableview.setWindowTitle(self.Object.Label)
            self._tableview.setParent(main)
            self._tableview.setWindowFlags(QtGui.Qt.Dialog)
            self._tableview.resize(
                main.size().height() / 2, main.size().height() / 3
            )  # keep the aspect ratio

            self.update_visualization()

        self._tableview.show()

    def update_visualization(self):

        if not hasattr(self, "_tableModel") or not self._tableModel:
            return

        # we collect the header names from the viewproviders
        table = self.Object.Table
        header = {}
        for child in self.Object.Group:

            if not child.Source:
                continue

            new_name = child.ViewObject.Name
            if new_name:
                # this child uses a custom name. We try to find all
                # columns that are from this child and use custom header for it
                for i in range(table.GetNumberOfColumns()):
                    if child.Source.Name in table.GetColumnName(i):
                        header[table.GetColumnName(i)] = new_name

        self._tableModel.setTable(self.Object.Table, header)
