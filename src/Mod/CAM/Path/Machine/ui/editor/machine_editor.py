# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Billy Huddleston <billy@ivdc.com>                  *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************
from PySide import QtGui, QtCore
import FreeCAD
import json
import Path
from typing import Optional, Dict, Any
from ...models.machine import *
from ....Main.Gui.Editor import CodeEditor
import re

translate = FreeCAD.Qt.translate


class MachineEditorDialog(QtGui.QDialog):
    """A dialog to edit machine JSON assets with proper form fields."""

    # Todo - Make this a json schema driven form in the future
    MACHINE_TYPES = {
        "custom": {
            "name": translate("CAM_MachineEditor", "Custom Machine"),
            "linear": [],
            "rotary": [],
        },
        "xz": {
            "name": translate("CAM_MachineEditor", "2-Axis Lathe (X, Z)"),
            "linear": ["X", "Z"],
            "rotary": [],
        },
        "xyz": {
            "name": translate("CAM_MachineEditor", "3-Axis Mill (XYZ)"),
            "linear": ["X", "Y", "Z"],
            "rotary": [],
        },
        "xyza": {
            "name": translate("CAM_MachineEditor", "4-Axis Mill (XYZ + A)"),
            "linear": ["X", "Y", "Z"],
            "rotary": ["A"],
        },
        "xyzb": {
            "name": translate("CAM_MachineEditor", "4-Axis Mill (XYZ + B)"),
            "linear": ["X", "Y", "Z"],
            "rotary": ["B"],
        },
        "xyzac": {
            "name": translate("CAM_MachineEditor", "5-Axis Mill (XYZ + A, C)"),
            "linear": ["X", "Y", "Z"],
            "rotary": ["A", "C"],
        },
        "xyzbc": {
            "name": translate("CAM_MachineEditor", "5-Axis Mill (XYZ + B, C)"),
            "linear": ["X", "Y", "Z"],
            "rotary": ["B", "C"],
        },
    }

    def __init__(self, machine_path: Optional[str] = None, parent=None):
        super().__init__(parent)
        self.setWindowTitle(translate("CAM_MachineEditor", "Machine Editor"))
        self.setMinimumSize(700, 800)
        self.resize(700, 800)

        self.current_units = "metric"

        self.layout = QtGui.QVBoxLayout(self)

        # Tab widget for sections
        self.tabs = QtGui.QTabWidget()
        self.layout.addWidget(self.tabs)

        # Machine tab
        self.machine_tab = QtGui.QWidget()
        self.tabs.addTab(self.machine_tab, translate("CAM_MachineEditor", "Machine"))
        self.setup_machine_tab()

        # Post tab
        self.post_tab = QtGui.QWidget()
        self.tabs.addTab(self.post_tab, translate("CAM_MachineEditor", "Post Processor"))
        self.setup_post_tab()
        # Text editor (initially hidden)
        self.text_editor = CodeEditor()

        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Editor")
        font = QtGui.QFont()
        font.setFamily(p.GetString("Font", "Courier"))
        font.setFixedPitch(True)
        font.setPointSize(p.GetInt("FontSize", 10))

        self.text_editor.setFont(font)
        self.layout.addWidget(self.text_editor)
        self.text_editor.hide()

        button_layout = QtGui.QHBoxLayout()

        self.toggle_button = QtGui.QPushButton(translate("CAM_MachineEditor", "Edit as Text"))
        self.toggle_button.clicked.connect(self.toggle_editor_mode)
        button_layout.addWidget(self.toggle_button)

        button_layout.addStretch()

        buttons = QtGui.QDialogButtonBox(
            QtGui.QDialogButtonBox.Save | QtGui.QDialogButtonBox.Close,
            QtCore.Qt.Horizontal,
        )
        buttons.button(QtGui.QDialogButtonBox.Save).setText(translate("CAM_MachineEditor", "Save"))
        buttons.button(QtGui.QDialogButtonBox.Close).setText(
            translate("CAM_MachineEditor", "Close")
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        button_layout.addWidget(buttons)

        self.layout.addLayout(button_layout)
        self.text_mode = False
        self.path = machine_path

        if machine_path:
            import os

            if os.path.exists(machine_path):
                try:
                    data = load_machine_file(machine_path)
                    self.populate_from_data(data)
                except Exception as e:
                    Path.Log.error(f"Failed to load machine file {machine_path}: {e}")
                    self.set_defaults()
            else:
                self.set_defaults()
        else:
            self.set_defaults()

    def normalize_text(self, edit, suffix, axis=None, field=None, section=None):
        """Normalize and validate text input for numeric fields with units.

        Parses user input, converts between metric/imperial units, and ensures
        proper formatting. Updates stored data for axes configuration.

        Args:
            edit: QLineEdit widget to update
            suffix: Unit suffix (e.g., "mm", "in", "deg")
            axis: Axis identifier for data storage
            field: Field name for data storage
            section: Section name ("linear" or "rotary") for data storage
        """
        text = edit.text().strip()
        if not text:
            edit.setText("0 " + suffix)
            return

        # Parse input manually based on machine units
        machine_units = self.units_combo.itemData(self.units_combo.currentIndex())

        # Split number and unit
        match = re.match(r"^([+-]?\d*\.?\d+)\s*(.*)$", text)
        if not match:
            edit.setText("0 " + suffix)
            return

        num_str, unit_str = match.groups()
        try:
            value = float(num_str)
        except ValueError:
            edit.setText("0 " + suffix)
            return

        # Convert to internal mm (or degrees for angles)
        if section == "rotary":
            # For rotary axes, always use degrees, no unit conversion
            if unit_str.strip():
                if unit_str.strip() in ["deg", "degree", "degrees", "°"]:
                    internal_value = value
                else:
                    # Unknown unit for angles, assume degrees
                    internal_value = value
            else:
                # No unit, assume degrees
                internal_value = value
        else:
            # Linear axes
            if unit_str.strip():
                if unit_str.strip() in ["mm", "millimeter", "millimeters"]:
                    internal_value = value
                elif unit_str.strip() in ["in", "inch", "inches", '"']:
                    internal_value = value * 25.4
                elif unit_str.strip() in ["cm", "centimeter", "centimeters"]:
                    internal_value = value * 10
                elif unit_str.strip() in ["m", "meter", "meters"]:
                    internal_value = value * 1000
                else:
                    # Unknown unit, assume machine units
                    if machine_units == "metric":
                        internal_value = value  # assume mm
                    else:
                        internal_value = value * 25.4  # assume in
            else:
                # No unit, assume machine units
                if machine_units == "metric":
                    internal_value = value  # assume mm
                else:
                    internal_value = value * 25.4  # assume in

        # Convert to display units
        if suffix == "mm":
            display_value = round(internal_value, 2)
        elif suffix == "in":
            display_value = round(internal_value / 25.4, 2)
        elif suffix == "mm/min":
            display_value = round(internal_value, 2)
        elif suffix == "in/min":
            display_value = round(internal_value / 25.4, 2)
        elif suffix == "deg":
            display_value = round(internal_value, 2)
        elif suffix == "deg/min":
            display_value = round(internal_value, 2)
        else:
            display_value = round(internal_value, 2)

        edit.setText(f"{display_value:.2f} {suffix}")

        # Update saved data (store in metric)
        if axis and field and section:
            if section not in self.saved_axes_data:
                self.saved_axes_data[section] = {}
            if axis not in self.saved_axes_data[section]:
                self.saved_axes_data[section][axis] = {}
            if section == "linear":
                if field in ["min", "max"]:
                    self.saved_axes_data[section][axis][field] = internal_value
                else:  # max_velocity
                    self.saved_axes_data[section][axis][field] = internal_value
            elif section == "rotary":
                self.saved_axes_data[section][axis][field] = internal_value

    def setup_machine_tab(self):
        """Set up the machine configuration tab with form fields.

        Creates input fields for machine name, manufacturer, description,
        units, type, spindle count, axes configuration, and spindles.
        Connects change handlers for dynamic updates.
        """
        layout = QtGui.QFormLayout(self.machine_tab)

        self.name_edit = QtGui.QLineEdit()
        layout.addRow(translate("CAM_MachineEditor", "Name:"), self.name_edit)

        self.manufacturer_edit = QtGui.QLineEdit()
        layout.addRow(translate("CAM_MachineEditor", "Manufacturer:"), self.manufacturer_edit)

        self.description_edit = QtGui.QLineEdit()
        layout.addRow(translate("CAM_MachineEditor", "Description:"), self.description_edit)

        self.units_combo = QtGui.QComboBox()
        self.units_combo.addItem(translate("CAM_MachineEditor", "Metric"), "metric")
        self.units_combo.addItem(translate("CAM_MachineEditor", "Imperial"), "imperial")
        self.units_combo.currentIndexChanged.connect(self.update_axes)
        layout.addRow(translate("CAM_MachineEditor", "Units:"), self.units_combo)

        self.type_combo = QtGui.QComboBox()
        for key, value in self.MACHINE_TYPES.items():
            self.type_combo.addItem(value["name"], key)
        self.type_combo.currentIndexChanged.connect(self.update_axes)
        layout.addRow(translate("CAM_MachineEditor", "Type:"), self.type_combo)

        self.spindle_count_combo = QtGui.QComboBox()
        for i in range(1, 10):  # 1 to 9 spindles
            self.spindle_count_combo.addItem(str(i), i)
        self.spindle_count_combo.currentIndexChanged.connect(self.update_spindles)
        layout.addRow(
            translate("CAM_MachineEditor", "Number of Spindles:"), self.spindle_count_combo
        )

        # Axes group
        self.axes_group = QtGui.QGroupBox(translate("CAM_MachineEditor", "Axes"))
        self.axes_layout = QtGui.QVBoxLayout(self.axes_group)
        self.axes_group.setVisible(False)  # Initially hidden, shown when axes are configured
        self.saved_axes_data = {"linear": {}, "rotary": {}}
        layout.addRow(self.axes_group)

        # Spindles group
        self.spindles_group = QtGui.QGroupBox(translate("CAM_MachineEditor", "Spindles"))
        spindles_layout = QtGui.QVBoxLayout(self.spindles_group)
        self.spindles_tabs = QtGui.QTabWidget()
        spindles_layout.addWidget(self.spindles_tabs)
        self.saved_spindle_data = []
        layout.addRow(self.spindles_group)

    def update_axes(self):
        """Update the axes configuration UI based on machine type and units.

        Dynamically creates input fields for linear and rotary axes based on
        the selected machine type. Handles unit conversion and maintains
        stored axis data across UI updates.
        """
        # Get current units for suffixes and conversion
        units = self.units_combo.itemData(self.units_combo.currentIndex())
        length_suffix = " mm" if units == "metric" else " in"
        vel_suffix = " mm/min" if units == "metric" else " in/min"
        angle_suffix = " deg"
        angle_vel_suffix = " deg/min"

        # Convert saved data if units changed
        if hasattr(self, "current_units") and self.current_units != units:
            self.current_units = units

        # Clear references before deleting widgets
        self.linear_edits = {}
        self.rotary_edits = {}

        # Clear existing axes widgets
        for i in reversed(range(self.axes_layout.count())):
            widget = self.axes_layout.itemAt(i).widget()
            if widget:
                widget.setParent(None)

        # Get current type
        type_key = self.type_combo.itemData(self.type_combo.currentIndex())
        if not type_key:
            return
        config = self.MACHINE_TYPES[type_key]

        # Linear axes
        if config["linear"]:
            linear_group = QtGui.QGroupBox("Linear Axes")
            linear_layout = QtGui.QFormLayout(linear_group)
            self.linear_edits = {}
            for axis in config["linear"]:
                saved_min = self.saved_axes_data["linear"].get(axis, {}).get("min", 0)
                converted_min = saved_min if units == "metric" else saved_min / 25.4
                min_edit = QtGui.QLineEdit()
                min_edit.setText(f"{converted_min:.2f} {length_suffix}")
                min_edit.editingFinished.connect(
                    lambda edit=min_edit, suffix=length_suffix.strip(), ax=axis, fld="min", sec="linear": self.normalize_text(
                        edit, suffix, ax, fld, sec
                    )
                )
                saved_max = self.saved_axes_data["linear"].get(axis, {}).get("max", 1000)
                converted_max = saved_max if units == "metric" else saved_max / 25.4
                max_edit = QtGui.QLineEdit()
                max_edit.setText(f"{converted_max:.2f} {length_suffix}")
                max_edit.editingFinished.connect(
                    lambda edit=max_edit, suffix=length_suffix.strip(), ax=axis, fld="max", sec="linear": self.normalize_text(
                        edit, suffix, ax, fld, sec
                    )
                )
                saved_vel = self.saved_axes_data["linear"].get(axis, {}).get("max_velocity", 10000)
                converted_vel = saved_vel if units == "metric" else saved_vel / 25.4
                vel_edit = QtGui.QLineEdit()
                vel_edit.setText(f"{converted_vel:.2f} {vel_suffix}")
                vel_edit.editingFinished.connect(
                    lambda edit=vel_edit, suffix=vel_suffix.strip(), ax=axis, fld="max_velocity", sec="linear": self.normalize_text(
                        edit, suffix, ax, fld, sec
                    )
                )
                axis_layout = QtGui.QHBoxLayout()
                axis_layout.addWidget(QtGui.QLabel("Min:"))
                axis_layout.addWidget(min_edit)
                axis_layout.addWidget(QtGui.QLabel("Max:"))
                axis_layout.addWidget(max_edit)
                axis_layout.addWidget(QtGui.QLabel("Max Vel:"))
                axis_layout.addWidget(vel_edit)
                linear_layout.addRow(f"{axis}:", axis_layout)
                self.linear_edits[axis] = {
                    "min": min_edit,
                    "max": max_edit,
                    "max_velocity": vel_edit,
                }
            self.axes_layout.addWidget(linear_group)

        # Rotary axes
        if config["rotary"]:
            rotary_group = QtGui.QGroupBox("Rotary Axes")
            rotary_layout = QtGui.QFormLayout(rotary_group)
            self.rotary_edits = {}
            for axis in config["rotary"]:
                min_edit = QtGui.QLineEdit()
                min_edit.setText(
                    f"{self.saved_axes_data['rotary'].get(axis, {}).get('min_angle', -180):.2f} {angle_suffix}"
                )
                min_edit.editingFinished.connect(
                    lambda edit=min_edit, suffix=angle_suffix.strip(), ax=axis, fld="min_angle", sec="rotary": self.normalize_text(
                        edit, suffix, ax, fld, sec
                    )
                )
                max_edit = QtGui.QLineEdit()
                max_edit.setText(
                    f"{self.saved_axes_data['rotary'].get(axis, {}).get('max_angle', 180):.2f} {angle_suffix}"
                )
                max_edit.editingFinished.connect(
                    lambda edit=max_edit, suffix=angle_suffix.strip(), ax=axis, fld="max_angle", sec="rotary": self.normalize_text(
                        edit, suffix, ax, fld, sec
                    )
                )
                vel_edit = QtGui.QLineEdit()
                vel_edit.setText(
                    f"{self.saved_axes_data['rotary'].get(axis, {}).get('max_velocity', 36000):.2f} {angle_vel_suffix}"
                )
                vel_edit.editingFinished.connect(
                    lambda edit=vel_edit, suffix=angle_vel_suffix.strip(), ax=axis, fld="max_velocity", sec="rotary": self.normalize_text(
                        edit, suffix, ax, fld, sec
                    )
                )
                axis_layout = QtGui.QHBoxLayout()
                axis_layout.addWidget(QtGui.QLabel("Min Angle:"))
                axis_layout.addWidget(min_edit)
                axis_layout.addWidget(QtGui.QLabel("Max Angle:"))
                axis_layout.addWidget(max_edit)
                axis_layout.addWidget(QtGui.QLabel("Max Vel:"))
                axis_layout.addWidget(vel_edit)
                rotary_layout.addRow(f"{axis}:", axis_layout)
                self.rotary_edits[axis] = {
                    "min_angle": min_edit,
                    "max_angle": max_edit,
                    "max_velocity": vel_edit,
                }
            self.axes_layout.addWidget(rotary_group)

        # Hide axes group if no axes configured
        self.axes_group.setVisible(bool(config["linear"] or config["rotary"]))

    def update_spindles(self):
        """Update the spindle configuration UI based on spindle count.

        Dynamically creates tabbed interface for multiple spindles with
        input fields for name, ID, power, speed, and tool holder.
        Maintains spindle data across UI updates.
        """
        # Update saved data with current edits
        if hasattr(self, "spindle_edits"):
            for i, edits in enumerate(self.spindle_edits):
                if i >= len(self.saved_spindle_data):
                    self.saved_spindle_data.append({})
                self.saved_spindle_data[i] = {
                    "name": edits["name"].text(),
                    "id": edits["id"].text(),
                    "max_power_kw": edits["max_power_kw"].value(),
                    "max_rpm": edits["max_rpm"].value(),
                    "min_rpm": edits["min_rpm"].value(),
                    "tool_change": edits["tool_change"].itemData(
                        edits["tool_change"].currentIndex()
                    ),
                }

        # Clear existing spindle tabs
        self.spindles_tabs.clear()
        self.spindle_edits = []
        count = self.spindle_count_combo.itemData(self.spindle_count_combo.currentIndex())
        for i in range(count):
            tab = QtGui.QWidget()
            layout = QtGui.QFormLayout(tab)

            name_edit = QtGui.QLineEdit()
            name_edit.setText(
                self.saved_spindle_data[i]["name"]
                if i < len(self.saved_spindle_data) and "name" in self.saved_spindle_data[i]
                else f"Spindle {i+1}"
            )
            layout.addRow("Name:", name_edit)

            id_edit = QtGui.QLineEdit()
            id_edit.setText(
                self.saved_spindle_data[i]["id"]
                if i < len(self.saved_spindle_data) and "id" in self.saved_spindle_data[i]
                else f"spindle{i+1}"
            )
            layout.addRow("ID:", id_edit)

            max_power_edit = QtGui.QDoubleSpinBox()
            max_power_edit.setRange(0, 100)
            max_power_edit.setValue(
                self.saved_spindle_data[i]["max_power_kw"]
                if i < len(self.saved_spindle_data) and "max_power_kw" in self.saved_spindle_data[i]
                else 3.0
            )
            layout.addRow("Max Power (kW):", max_power_edit)

            max_rpm_edit = QtGui.QSpinBox()
            max_rpm_edit.setRange(0, 100000)
            max_rpm_edit.setValue(
                self.saved_spindle_data[i]["max_rpm"]
                if i < len(self.saved_spindle_data) and "max_rpm" in self.saved_spindle_data[i]
                else 24000
            )
            layout.addRow("Max RPM:", max_rpm_edit)

            min_rpm_edit = QtGui.QSpinBox()
            min_rpm_edit.setRange(0, 100000)
            min_rpm_edit.setValue(
                self.saved_spindle_data[i]["min_rpm"]
                if i < len(self.saved_spindle_data) and "min_rpm" in self.saved_spindle_data[i]
                else 6000
            )
            layout.addRow("Min RPM:", min_rpm_edit)

            tool_change_combo = QtGui.QComboBox()
            tool_change_combo.addItem("Manual", "manual")
            tool_change_combo.addItem("ATC", "atc")
            if i < len(self.saved_spindle_data) and "tool_change" in self.saved_spindle_data[i]:
                index = tool_change_combo.findData(self.saved_spindle_data[i]["tool_change"])
                if index >= 0:
                    tool_change_combo.setCurrentIndex(index)
            layout.addRow("Tool Change:", tool_change_combo)

            self.spindles_tabs.addTab(tab, f"Spindle {i+1}")
            self.spindle_edits.append(
                {
                    "name": name_edit,
                    "id": id_edit,
                    "max_power_kw": max_power_edit,
                    "max_rpm": max_rpm_edit,
                    "min_rpm": min_rpm_edit,
                    "tool_change": tool_change_combo,
                }
            )

    def setup_post_tab(self):
        """Set up the post processor configuration tab.

        Creates input fields for output units, comments, line numbers,
        tool length offsets, and placeholder for additional post settings.
        """
        layout = QtGui.QFormLayout(self.post_tab)

        self.output_unit_combo = QtGui.QComboBox()
        self.output_unit_combo.addItem("Metric", "metric")
        self.output_unit_combo.addItem("Imperial", "imperial")
        layout.addRow("Output Unit:", self.output_unit_combo)

        self.comments_combo = QtGui.QComboBox()
        self.comments_combo.addItem("Yes", True)
        self.comments_combo.addItem("No", False)
        layout.addRow("Comments:", self.comments_combo)

        self.line_numbers_combo = QtGui.QComboBox()
        self.line_numbers_combo.addItem("Yes", True)
        self.line_numbers_combo.addItem("No", False)
        layout.addRow("Line Numbers:", self.line_numbers_combo)

        self.tool_length_offset_combo = QtGui.QComboBox()
        self.tool_length_offset_combo.addItem("Yes", True)
        self.tool_length_offset_combo.addItem("No", False)
        layout.addRow("Tool Length Offset:", self.tool_length_offset_combo)

        # Placeholder for other post settings
        self.other_post_group = QtGui.QGroupBox("Other Post Settings")
        other_layout = QtGui.QVBoxLayout(self.other_post_group)
        other_layout.addWidget(
            QtGui.QLabel("Additional post processor settings (to be implemented)")
        )
        layout.addRow(self.other_post_group)

    def set_defaults(self):
        """Set default values for all form fields.

        Initializes the dialog with sensible defaults for creating a new machine.
        """
        data = create_default_machine_data()
        self.populate_from_data(data)

    def load_file(self, path: str):
        """Load machine configuration from a JSON file.

        Args:
            path: Path to the .fcm machine file to load
        """
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
            self.populate_from_data(data)
            Path.Log.info(f"Loaded machine file: {path}")
        except Exception as e:
            Path.Log.error(f"Failed to load machine file {path}: {e}")
            self.set_defaults()

    def populate_from_data(self, data: Dict[str, Any]):
        """Populate UI fields from machine data dictionary.

        Args:
            data: Dictionary containing machine configuration data
        """
        machine = data.get("machine", {})
        self.name_edit.setText(machine.get("name", ""))
        self.manufacturer_edit.setText(machine.get("manufacturer", ""))
        self.description_edit.setText(machine.get("description", ""))
        units = machine.get("units", "metric")
        index = self.units_combo.findData(units)
        if index >= 0:
            self.units_combo.setCurrentIndex(index)
        self.current_units = units
        machine_type = machine.get("type", "custom")
        index = self.type_combo.findData(machine_type)
        if index >= 0:
            self.type_combo.setCurrentIndex(index)
        self.update_axes()  # Update axes after setting type

        # Get units for suffixes in populate
        units = self.units_combo.itemData(self.units_combo.currentIndex())
        length_suffix = " mm" if units == "metric" else " in"
        vel_suffix = " mm/min" if units == "metric" else " in/min"
        angle_suffix = " deg"
        angle_vel_suffix = " deg/min"

        # Populate axes values
        axes = machine.get("axes", {})
        linear_axes = axes.get("linear", {})
        for axis, values in linear_axes.items():
            if axis in getattr(self, "linear_edits", {}):
                converted_min = (
                    values.get("min", 0) if units == "metric" else values.get("min", 0) / 25.4
                )
                self.linear_edits[axis]["min"].setText(f"{converted_min:.2f} {length_suffix}")
                converted_max = (
                    values.get("max", 1000) if units == "metric" else values.get("max", 1000) / 25.4
                )
                self.linear_edits[axis]["max"].setText(f"{converted_max:.2f} {length_suffix}")
                converted_vel = (
                    values.get("max_velocity", 10000)
                    if units == "metric"
                    else values.get("max_velocity", 10000) / 25.4
                )
                self.linear_edits[axis]["max_velocity"].setText(f"{converted_vel:.2f} {vel_suffix}")

        rotary_axes = axes.get("rotary", {})
        for axis, values in rotary_axes.items():
            if axis in getattr(self, "rotary_edits", {}):
                self.rotary_edits[axis]["min_angle"].setText(
                    f"{values.get('min_angle', -180):.2f} {angle_suffix}"
                )
                self.rotary_edits[axis]["max_angle"].setText(
                    f"{values.get('max_angle', 180):.2f} {angle_suffix}"
                )
                self.rotary_edits[axis]["max_velocity"].setText(
                    f"{values.get('max_velocity', 36000):.2f} {angle_vel_suffix}"
                )

        # Save loaded axes data to persistent storage (in metric)
        self.saved_axes_data = {"linear": {}, "rotary": {}}
        for axis, values in linear_axes.items():
            self.saved_axes_data["linear"][axis] = {
                "min": values.get("min", 0),
                "max": values.get("max", 1000),
                "max_velocity": values.get("max_velocity", 10000),
            }
        for axis, values in rotary_axes.items():
            self.saved_axes_data["rotary"][axis] = {
                "min_angle": values.get("min_angle", -180),
                "max_angle": values.get("max_angle", 180),
                "max_velocity": values.get("max_velocity", 36000),
            }

        spindles = machine.get("spindles", [])
        spindle_count = len(spindles)
        if spindle_count == 0:
            spindle_count = 1  # Default to 1 if none
        spindle_count = min(spindle_count, 9)  # Cap at 9
        self.spindle_count_combo.setCurrentText(str(spindle_count))
        self.update_spindles()  # Update spindles after setting count

        # Populate spindle values
        for i, spindle in enumerate(spindles):
            if i < len(self.spindle_edits):
                edits = self.spindle_edits[i]
                edits["name"].setText(spindle.get("name", ""))
                edits["id"].setText(spindle.get("id", ""))
                edits["max_power_kw"].setValue(spindle.get("max_power_kw", 3.0))
                edits["max_rpm"].setValue(spindle.get("max_rpm", 24000))
                edits["min_rpm"].setValue(spindle.get("min_rpm", 6000))
                tool_change = spindle.get("tool_change", {}).get("type", "manual")
                index = edits["tool_change"].findData(tool_change)
                if index >= 0:
                    edits["tool_change"].setCurrentIndex(index)

        # Save loaded spindle data to persistent storage
        self.saved_spindle_data = []
        for edits in self.spindle_edits:
            self.saved_spindle_data.append(
                {
                    "name": edits["name"].text(),
                    "id": edits["id"].text(),
                    "max_power_kw": edits["max_power_kw"].value(),
                    "max_rpm": edits["max_rpm"].value(),
                    "min_rpm": edits["min_rpm"].value(),
                    "tool_change": edits["tool_change"].itemData(
                        edits["tool_change"].currentIndex()
                    ),
                }
            )

        post = data.get("post", {})
        output_unit = post.get("output_unit", "metric")
        index = self.output_unit_combo.findData(output_unit)
        if index >= 0:
            self.output_unit_combo.setCurrentIndex(index)

        comments = post.get("comments", True)
        index = self.comments_combo.findData(comments)
        if index >= 0:
            self.comments_combo.setCurrentIndex(index)

        line_numbers = post.get("line_numbers", {}).get("enabled", True)
        index = self.line_numbers_combo.findData(line_numbers)
        if index >= 0:
            self.line_numbers_combo.setCurrentIndex(index)

        tool_length_offset = post.get("tool_length_offset", True)
        index = self.tool_length_offset_combo.findData(tool_length_offset)
        if index >= 0:
            self.tool_length_offset_combo.setCurrentIndex(index)

    def to_data(self) -> Dict[str, Any]:
        """Convert UI state to machine data dictionary.

        Returns:
            Dict containing complete machine configuration in JSON format
        """
        machine_type = self.type_combo.itemData(self.type_combo.currentIndex())
        # Always save in metric units
        axes = {"linear": {}, "rotary": {}}
        for axis, values in self.saved_axes_data["linear"].items():
            axes["linear"][axis] = {
                "min": values.get("min", 0),
                "max": values.get("max", 1000),
                "max_velocity": values.get("max_velocity", 10000),
            }
        for axis, values in self.saved_axes_data["rotary"].items():
            axes["rotary"][axis] = {
                "min_angle": values.get("min_angle", -180),
                "max_angle": values.get("max_angle", 180),
                "max_velocity": values.get("max_velocity", 36000),
            }
        spindles = []
        if hasattr(self, "spindle_edits"):
            for edits in self.spindle_edits:
                spindles.append(
                    {
                        "name": edits["name"].text(),
                        "id": edits["id"].text(),
                        "max_power_kw": edits["max_power_kw"].value(),
                        "max_rpm": edits["max_rpm"].value(),
                        "min_rpm": edits["min_rpm"].value(),
                        "tool_change": {
                            "type": edits["tool_change"].itemData(
                                edits["tool_change"].currentIndex()
                            )
                        },
                    }
                )
        data = {
            "machine": {
                "name": self.name_edit.text(),
                "manufacturer": self.manufacturer_edit.text(),
                "description": self.description_edit.text(),
                "units": self.units_combo.itemData(self.units_combo.currentIndex()),
                "type": machine_type,
                "axes": axes,
                "spindles": spindles,
            },
            "post": {
                "output_unit": self.output_unit_combo.itemData(
                    self.output_unit_combo.currentIndex()
                ),
                "comments": self.comments_combo.itemData(self.comments_combo.currentIndex()),
                "line_numbers": {
                    "enabled": self.line_numbers_combo.itemData(
                        self.line_numbers_combo.currentIndex()
                    )
                },
                "tool_length_offset": self.tool_length_offset_combo.itemData(
                    self.tool_length_offset_combo.currentIndex()
                ),
                # Add other post settings later
            },
            "version": 1,
        }
        return data

    def save_file(self, path: Optional[str] = None):
        """Save machine configuration to a JSON file.

        Args:
            path: Optional path to save to. Uses self.path if None.
        """
        if path is None:
            path = self.path
        if path is None:
            Path.Log.error("No path specified for save_file")
            return
        try:
            data = self.to_data()
            save_machine_file(data, path)
        except Exception as e:
            Path.Log.error(f"Failed to save machine file {path}: {e}")

    def toggle_editor_mode(self):
        """Toggle between form view and text editor view."""
        if self.text_mode:
            # Switching from text to form mode
            try:
                # Parse JSON from text editor
                json_text = self.text_editor.toPlainText()
                data = json.loads(json_text)
                # Validate and populate form
                self.populate_from_data(data)
                # Show form, hide editor
                self.tabs.show()
                self.text_editor.hide()
                self.toggle_button.setText(translate("CAM_MachineEditor", "Edit as Text"))
                self.text_mode = False
            except json.JSONDecodeError as e:
                QtGui.QMessageBox.critical(
                    self,
                    translate("CAM_MachineEditor", "JSON Error"),
                    translate("CAM_MachineEditor", "Invalid JSON: {}").format(str(e)),
                )
            except Exception as e:
                QtGui.QMessageBox.critical(
                    self,
                    translate("CAM_MachineEditor", "Error"),
                    translate("CAM_MachineEditor", "Failed to parse data: {}").format(str(e)),
                )
        else:
            # Switching from form to text mode
            try:
                # Get current data from form
                data = self.to_data()
                # Convert to JSON with nice formatting
                json_text = json.dumps(data, indent=2, ensure_ascii=False)
                self.text_editor.setPlainText(json_text)
                # Hide form, show editor
                self.tabs.hide()
                self.text_editor.show()
                self.toggle_button.setText(translate("CAM_MachineEditor", "Edit as Form"))
                self.text_mode = True
            except Exception as e:
                QtGui.QMessageBox.critical(
                    self,
                    translate("CAM_MachineEditor", "Error"),
                    translate("CAM_MachineEditor", "Failed to generate JSON: {}").format(str(e)),
                )

    def accept(self):
        """Handle save and close action."""
        if self.text_mode:
            # If in text mode, parse JSON before saving
            try:
                json_text = self.text_editor.toPlainText()
                data = json.loads(json_text)
                if self.path:
                    save_machine_file(data, self.path)
            except json.JSONDecodeError as e:
                QtGui.QMessageBox.critical(
                    self,
                    translate("CAM_MachineEditor", "JSON Error"),
                    translate("CAM_MachineEditor", "Invalid JSON: {}").format(str(e)),
                )
                return
            except Exception as e:
                QtGui.QMessageBox.critical(
                    self,
                    translate("CAM_MachineEditor", "Error"),
                    translate("CAM_MachineEditor", "Failed to save: {}").format(str(e)),
                )
                return
        else:
            # Form mode - use existing save logic
            if self.path:
                self.save_file()
        super().accept()
