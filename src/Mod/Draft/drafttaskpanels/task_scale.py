# ***************************************************************************
# *   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2025 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides the task panel code for the Draft Scale tool."""
## @package task_scale
# \ingroup drafttaskpanels
# \brief Provides the task panel code for the Draft Scale tool.

## \addtogroup drafttaskpanels
# @{
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui
import PySide.QtWidgets as QtWidgets

import FreeCAD as App
import FreeCADGui as Gui
from draftutils import params
from draftutils import utils
from draftutils.translate import translate


class ScaleTaskPanel:
    """The task panel for the Draft Scale tool."""

    def __init__(self):
        decimals = max(6, params.get_param("Decimals", path="Units"))
        self.sourceCmd = None
        self.form = QtWidgets.QWidget()
        self.form.setWindowTitle(translate("Draft", "Scale"))
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Draft_Scale.svg"))
        layout = QtWidgets.QGridLayout(self.form)
        self.xLabel = QtWidgets.QLabel()
        self.xLabel.setText(translate("Draft", "X factor"))
        layout.addWidget(self.xLabel, 0, 0, 1, 1)
        self.xValue = QtWidgets.QDoubleSpinBox()
        self.xValue.setRange(-1000000.0, 1000000.0)
        self.xValue.setDecimals(decimals)
        self.xValue.setValue(1)
        layout.addWidget(self.xValue,0,1,1,1)
        self.yLabel = QtWidgets.QLabel()
        self.yLabel.setText(translate("Draft", "Y factor"))
        layout.addWidget(self.yLabel,1,0,1,1)
        self.yValue = QtWidgets.QDoubleSpinBox()
        self.yValue.setRange(-1000000.0, 1000000.0)
        self.yValue.setDecimals(decimals)
        self.yValue.setValue(1)
        layout.addWidget(self.yValue,1,1,1,1)
        self.zLabel = QtWidgets.QLabel()
        self.zLabel.setText(translate("Draft", "Z factor"))
        layout.addWidget(self.zLabel,2,0,1,1)
        self.zValue = QtWidgets.QDoubleSpinBox()
        self.zValue.setRange(-1000000.0, 1000000.0)
        self.zValue.setDecimals(decimals)
        self.zValue.setValue(1)
        layout.addWidget(self.zValue,2,1,1,1)
        self.lock = QtWidgets.QCheckBox()
        self.lock.setText(translate("Draft", "Uniform scaling"))
        self.lock.setChecked(params.get_param("ScaleUniform"))
        layout.addWidget(self.lock,3,0,1,2)

        QtCore.QObject.connect(self.xValue,QtCore.SIGNAL("valueChanged(double)"),self.setValue)
        QtCore.QObject.connect(self.yValue,QtCore.SIGNAL("valueChanged(double)"),self.setValue)
        QtCore.QObject.connect(self.zValue,QtCore.SIGNAL("valueChanged(double)"),self.setValue)
        QtCore.QObject.connect(self.lock,QtCore.SIGNAL("toggled(bool)"),self.setLock)

        if self.__class__.__name__ != "ScaleTaskPanelEdit":
            self.relative = QtWidgets.QCheckBox()
            self.relative.setText(translate("Draft", "Working plane orientation"))
            self.relative.setChecked(params.get_param("ScaleRelative"))
            layout.addWidget(self.relative,4,0,1,2)
            self.isCopy = QtWidgets.QCheckBox()
            self.isCopy.setText(translate("Draft", "Copy"))
            self.isCopy.setChecked(params.get_param("ScaleCopy"))
            layout.addWidget(self.isCopy,5,0,1,2)
            self.isSubelementMode = QtWidgets.QCheckBox()
            self.isSubelementMode.setText(translate("Draft", "Modify subelements"))
            self.isSubelementMode.setChecked(params.get_param("SubelementMode"))
            layout.addWidget(self.isSubelementMode,6,0,1,2)
            self.isClone = QtWidgets.QCheckBox()
            self.isClone.setText(translate("Draft", "Create a clone"))
            self.isClone.setChecked(params.get_param("ScaleClone"))
            layout.addWidget(self.isClone,7,0,1,2)
            self.pickrefButton = QtWidgets.QPushButton()
            self.pickrefButton.setText(translate("Draft", "Pick from/to points"))
            layout.addWidget(self.pickrefButton,8,0,1,2)

            QtCore.QObject.connect(self.relative,QtCore.SIGNAL("toggled(bool)"),self.setRelative)
            QtCore.QObject.connect(self.isCopy,QtCore.SIGNAL("toggled(bool)"),self.setCopy)
            QtCore.QObject.connect(self.isSubelementMode,QtCore.SIGNAL("toggled(bool)"),self.setSubelementMode)
            QtCore.QObject.connect(self.isClone,QtCore.SIGNAL("toggled(bool)"),self.setClone)
            QtCore.QObject.connect(self.pickrefButton,QtCore.SIGNAL("clicked()"),self.pickRef)

    def setValue(self, val=None):
        """Set the value of the scale factors."""
        if self.lock.isChecked():
            if not self.xValue.hasFocus():
                self.xValue.setValue(val)
            if not self.yValue.hasFocus():
                self.yValue.setValue(val)
            if not self.zValue.hasFocus():
                self.zValue.setValue(val)
        if self.sourceCmd:
            # self.sourceCmd is always None for ScaleTaskPanelEdit
            self.sourceCmd.scale_ghosts(self.xValue.value(),self.yValue.value(),self.zValue.value(),self.relative.isChecked())

    def setLock(self, state):
        """Set the uniform scaling."""
        if self.sourceCmd:
            # self.sourceCmd is always None for ScaleTaskPanelEdit
            params.set_param("ScaleUniform", state)
        if state:
            val = self.xValue.value()
            self.yValue.setValue(val)
            self.zValue.setValue(val)

    def setRelative(self, state):
        """Set the relative scaling."""
        params.set_param("ScaleRelative", state)
        if self.sourceCmd:
            self.sourceCmd.scale_ghosts(self.xValue.value(),self.yValue.value(),self.zValue.value(),self.relative.isChecked())

    def setCopy(self, state):
        """Set the copy option."""
        params.set_param("ScaleCopy", state)
        if state and self.isClone.isChecked():
            self.isClone.setChecked(False)

    def setSubelementMode(self, state):
        """Set the subelement option."""
        params.set_param("SubelementMode", state)
        if state and self.isClone.isChecked():
            self.isClone.setChecked(False)
        if self.sourceCmd:
            self.sourceCmd.set_ghosts()
            self.sourceCmd.scale_ghosts(self.xValue.value(),self.yValue.value(),self.zValue.value(),self.relative.isChecked())

    def setClone(self, state):
        """Set the clone option."""
        params.set_param("ScaleClone", state)
        if state and self.isCopy.isChecked():
            self.isCopy.setChecked(False)
        if state and self.isSubelementMode.isChecked():
            self.isSubelementMode.setChecked(False)

    def pickRef(self):
        """Pick a reference point from the calling class."""
        if self.sourceCmd:
            self.sourceCmd.pick_ref()

    def accept(self):
        """Execute when clicking the OK button."""
        if self.sourceCmd:
            self.sourceCmd.scale()
        Gui.ActiveDocument.resetEdit()
        return True

    def reject(self):
        """Execute when clicking the Cancel button."""
        if self.sourceCmd:
            self.sourceCmd.finish()
        Gui.ActiveDocument.resetEdit()
        return True


class ScaleTaskPanelEdit(ScaleTaskPanel):
    """The task panel to edit the scale of Draft Clones."""

    def __init__(self, obj):
        super().__init__()
        self.ghost = None
        self.selection = Gui.Selection.getSelectionEx("", 0)
        self.obj = obj
        self.obj_x, self.obj_y, self.obj_z = self.obj.Scale
        self.form.setWindowTitle(translate("Draft", "Edit scale"))
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Draft_Clone.svg"))
        self.xValue.setValue(self.obj_x)
        self.yValue.setValue(self.obj_y)
        self.zValue.setValue(self.obj_z)
        self.lock.setChecked(self.obj_x == self.obj_y == self.obj_z)

    def setValue(self, val=None):
        """Set the value of the scale factors."""
        super().setValue(val)
        self.scale_ghost(self.xValue.value(), self.yValue.value(), self.zValue.value())

    def scale_ghost(self, x, y, z):
        """Scale the preview of the object."""
        x = x / (self.obj_x if abs(self.obj_x) > 1e-7 else 1e-7)
        y = y / (self.obj_y if abs(self.obj_y) > 1e-7 else 1e-7)
        z = z / (self.obj_z if abs(self.obj_z) > 1e-7 else 1e-7)

        if self.ghost is None:
            self.set_ghost()

        mtx_scale = App.Matrix()
        mtx_scale.scale(x, y, z)
        mtx = self.global_place.Matrix * mtx_scale
        mtx = mtx * self.global_place.Matrix.inverse()

        delta = self.global_place.inverse().Rotation.multVec(self.global_place.Base)
        delta = -App.Vector(delta.x*x, delta.y*y, delta.z*z)
        delta = self.global_place.multVec(delta)

        self.ghost.setMatrix(mtx)
        self.ghost.move(delta)
        # self.ghost.flip_normals(x * y * z < 0)  # Does not work properly for Draft_Circles for example.
        self.ghost.on()

    def set_ghost(self):
        """Set the ghost to display."""
        # Import has to happen here to avoid circular imports.
        from draftguitools import gui_trackers as trackers

        if self.ghost is not None:
            self.ghost.remove()
        objs, places, _ = utils._modifiers_process_selection(self.selection, copy=False, scale=True)
        self.ghost = trackers.ghostTracker(objs, parent_places=places)
        self.global_place = places[0] * self.obj.Placement

    def accept(self):
        """Execute when clicking the OK button."""
        self.obj.Scale = (self.xValue.value(), self.yValue.value(), self.zValue.value())
        App.ActiveDocument.recompute()
        if self.ghost is not None:
            self.ghost.finalize()
        Gui.ActiveDocument.resetEdit()
        return True

    def reject(self):
        """Execute when clicking the Cancel button."""
        if self.ghost is not None:
            self.ghost.finalize()
        Gui.ActiveDocument.resetEdit()
        return True

    def finish(self):
        """Called by unsetEdit in view_clone.py."""
        Gui.Control.closeDialog()
        return None

## @}
