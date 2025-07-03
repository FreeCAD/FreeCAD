# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
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
from PySide import QtGui, QtCore
from PySide.QtGui import QComboBox, QPushButton, QHBoxLayout
import FreeCAD
import Path
from ..models.axis import AngularAxis
from ..models.machine import Machine, MachineFeature
from ..models.component import MachineComponent
from ..models.spindle import Spindle
from .post import PostProcessorSettingsDialog
from .component import MachineComponentWidget
from .angular_axis import AngularAxisWidget
from .spindle_prop import SpindleWidget
from .tree import MachineComponentTreeView


translate = FreeCAD.Qt.translate


class MachinePropertiesDialog(QtGui.QDialog):
    """Base dialog for adding or editing a machine with common properties."""

    def __init__(self, machine: Machine, parent=None):
        super().__init__(parent)
        self.machine = machine
        self.setWindowTitle(translate("CAM", "Edit Machine"))
        self.layout = QtGui.QVBoxLayout(self)

        # Rigidity visibility based on MILLING_3D feature flag
        self.rigidity_visible = MachineFeature.MILLING_3D in self.machine.feature_flags

        # General group
        general_group = QtGui.QGroupBox(translate("CAM", "General"))
        general_layout = QtGui.QFormLayout()

        self.machine_id_label = QtGui.QLabel(machine.get_id())
        self.machine_id_label.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)
        general_layout.addRow(translate("CAM", "ID"), self.machine_id_label)

        self.label_edit = QtGui.QLineEdit(machine.label)
        self.label_edit.textChanged.connect(self._update_window_title)
        general_layout.addRow(translate("CAM", "Label"), self.label_edit)
        self._update_window_title()

        self.post_processor_combo = QComboBox()
        available_post_processors = Path.Preferences.allAvailablePostProcessors()
        self.post_processor_combo.addItems(available_post_processors)
        if machine.post_processor in available_post_processors:
            self.post_processor_combo.setCurrentIndex(
                available_post_processors.index(machine.post_processor)
            )
        self.post_processor_combo.currentIndexChanged.connect(self._update_machine_post_processor)

        post_processor_layout = QHBoxLayout()
        post_processor_layout.addWidget(self.post_processor_combo)

        self.configure_post_processor_btn = QPushButton(translate("CAM", "Configure…"))
        self.configure_post_processor_btn.clicked.connect(self.launch_post_processor_settings)
        post_processor_layout.addWidget(self.configure_post_processor_btn)
        general_layout.addRow(translate("CAM", "Post processor"), post_processor_layout)

        general_group.setLayout(general_layout)
        self.layout.addWidget(general_group)

        # Kinematic Structure
        kinematic_structure_label = QtGui.QLabel(translate("CAM", "Kinematic Structure"))
        self.layout.addWidget(kinematic_structure_label)

        # Component Tree View
        self.component_tree_view = MachineComponentTreeView(self.machine)
        self.component_tree_view.componentSelected.connect(self._on_component_selected)
        self.layout.addWidget(self.component_tree_view)

        # Properties label and scroll area
        self.properties_label = QtGui.QLabel(translate("CAM", "Properties"))
        self.layout.addWidget(self.properties_label)

        scroll_area = QtGui.QScrollArea()
        scroll_area.setWidgetResizable(True)
        properties_container = QtGui.QWidget()
        self.component_widget_area = QtGui.QVBoxLayout(properties_container)
        scroll_area.setWidget(properties_container)
        self.layout.addWidget(scroll_area)

        self.component_widgets = []  # List to hold currently displayed component widgets

        # Dialog buttons
        buttons = QtGui.QDialogButtonBox(
            QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel, QtCore.Qt.Horizontal
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        self.layout.addWidget(buttons)

        self.resize(750, self.sizeHint().height() + 200)

    def launch_post_processor_settings(self):
        dialog = PostProcessorSettingsDialog(self.machine, self)
        if dialog.exec_():
            # Update the post processor combo box
            current_post_processor = self.machine.post_processor
            index = self.post_processor_combo.findText(current_post_processor)
            if index != -1:
                self.post_processor_combo.setCurrentIndex(index)

    def _update_machine_post_processor(self, index):
        self.machine.post_processor = self.post_processor_combo.itemText(index)

    def _on_component_selected(self, component: MachineComponent):
        # Clear existing widgets
        for widget in self.component_widgets:
            widget.setParent(None)
            widget.deleteLater()
        self.component_widgets.clear()

        # Create and add the new widget based on component type
        if isinstance(component, AngularAxis):
            widget = AngularAxisWidget(component)
        elif isinstance(component, Spindle):
            widget = SpindleWidget(component)
        else:
            widget = MachineComponentWidget(component, component.get_attribute_configs())
        widget.labelChanged.connect(self.component_tree_view.update_selected_component_label)
        # Set rigidity visibility for the component widget
        widget.set_visibility(
            ["App::PropertyAngularRigidity", "App::PropertyRigidity"], self.rigidity_visible
        )
        self.component_widget_area.addWidget(widget)
        self.component_widgets.append(widget)
        self._update_properties_label(component)

    def _update_properties_label(self, component: MachineComponent):
        self.properties_label.setText(translate("CAM", f"{component.type} Properties"))

    def accept(self):
        # Check the label
        label = self.label_edit.text()
        if not label:
            QtGui.QMessageBox.warning(
                self,
                translate("CAM", "Warning"),
                translate("CAM", "Machine label cannot be empty."),
            )
            return
        self.machine.label = label

        self.machine.post_processor = self.post_processor_combo.currentText()

        # Update component properties from dynamic widgets
        for widget in self.component_widgets:
            if not widget.validate_inputs():
                return False  # Stop if any validation fails

        # Validate the updates.
        try:
            self.machine.validate()
        except AttributeError as e:
            QtGui.QMessageBox.warning(self, translate("CAM", "Warning"), str(e))
            return

        super().accept()

    def _update_window_title(self):
        title = translate("CAM", "Edit Machine")
        if self.label_edit.text():
            title = f"{self.label_edit.text()} - {title}"
        self.setWindowTitle(title)
