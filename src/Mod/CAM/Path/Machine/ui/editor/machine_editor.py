# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Billy Huddleston <billy@ivdc.com>                  *
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
import Path
import json
from typing import Optional, Dict, Any, get_origin, get_args
from dataclasses import fields
from enum import Enum
from ...models import Machine, MachineFactory, LinearAxis, RotaryAxis, Spindle
from ....Main.Gui.Editor import CodeEditor
from Path.Post.Processor import PostProcessorFactory
import re

translate = FreeCAD.Qt.translate


class DataclassGUIGenerator:
    """Generates Qt widgets dynamically from dataclass definitions.

    This class introspects dataclass fields and creates appropriate GUI widgets
    based on field types. It supports nested dataclasses, creating grouped layouts
    for better organization.
    """

    # Fields that should use multi-line text editors instead of single-line
    MULTILINE_FIELDS = {
        "preamble",
        "postamble",
        "pre_job",
        "post_job",
        "pre_operation",
        "post_operation",
        "pre_tool_change",
        "post_tool_change",
        "tool_return",
        "pre_fixture_change",
        "post_fixture_change",
        "pre_rotary_move",
        "post_rotary_move",
        "pre_spindle_change",
        "post_spindle_change",
        "safetyblock",
        "drill_cycles_to_translate",
        "suppress_commands",
    }

    # Field display name overrides
    FIELD_LABELS = {
        "comments": translate("CAM_MachineEditor", "Include Comments"),
        "blank_lines": translate("CAM_MachineEditor", "Include Blank Lines"),
        "header": translate("CAM_MachineEditor", "Include Header"),
        "line_numbers": translate("CAM_MachineEditor", "Line Numbers"),
        "path_labels": translate("CAM_MachineEditor", "Path Labels"),
        "machine_name": translate("CAM_MachineEditor", "Include Machine Name"),
        "output_double_parameters": translate("CAM_MachineEditor", "Output Duplicate Axis Values"),
        "output_bcnc_comments": translate("CAM_MachineEditor", "Output bCNC Comments"),
        "doubles": translate("CAM_MachineEditor", "Output Duplicate Axis Values"),
        "adaptive": translate("CAM_MachineEditor", "Adaptive Output"),
        "axis_precision": translate("CAM_MachineEditor", "Axis Precision"),
        "feed_precision": translate("CAM_MachineEditor", "Feed Precision"),
        "spindle_decimals": translate("CAM_MachineEditor", "Spindle Decimals"),
        "command_space": translate("CAM_MachineEditor", "Command Spacing"),
        "comment_symbol": translate("CAM_MachineEditor", "Comment Symbol"),
        "line_increment": translate("CAM_MachineEditor", "Line Number Increment"),
        "line_number_start": translate("CAM_MachineEditor", "Line Number Start"),
        "end_of_line_chars": translate("CAM_MachineEditor", "End of Line Characters"),
        "modal": translate("CAM_MachineEditor", "Modal Output (Suppress Repeats)"),
        "translate_drill_cycles": translate("CAM_MachineEditor", "Translate Drill Cycles"),
        "split_arcs": translate("CAM_MachineEditor", "Split Arcs"),
        "show_editor": translate("CAM_MachineEditor", "Show Editor After Generation"),
        "list_tools_in_preamble": translate("CAM_MachineEditor", "List Tools in Preamble"),
        "show_machine_units": translate("CAM_MachineEditor", "Show Machine Units"),
        "show_operation_labels": translate("CAM_MachineEditor", "Show Operation Labels"),
        "tool_before_change": translate("CAM_MachineEditor", "Output T Before M6"),
        "chipbreaking_amount": translate("CAM_MachineEditor", "Chipbreaking Amount (mm)"),
        "spindle_wait": translate("CAM_MachineEditor", "Spindle Wait Time (seconds)"),
        "postprocessor_file_name": translate("CAM_MachineEditor", "Post Processor"),
        "postprocessor_args": translate("CAM_MachineEditor", "Post Processor Arguments"),
        "use_tlo": translate("CAM_MachineEditor", "Use Tool Length Offset"),
        "stop_spindle_for_tool_change": translate(
            "CAM_MachineEditor", "Stop Spindle for Tool Change"
        ),
        "enable_coolant": translate("CAM_MachineEditor", "Enable Coolant"),
        "enable_machine_specific_commands": translate(
            "CAM_MachineEditor", "Enable Machine-Specific Commands"
        ),
    }

    @staticmethod
    def get_field_label(field_name: str) -> str:
        """Get a human-readable label for a field name."""
        if field_name in DataclassGUIGenerator.FIELD_LABELS:
            return DataclassGUIGenerator.FIELD_LABELS[field_name]
        # Convert snake_case to Title Case
        return " ".join(word.capitalize() for word in field_name.split("_"))

    @staticmethod
    def create_widget_for_field(
        field_name: str, field_type: type, current_value: Any
    ) -> QtGui.QWidget:
        """Create an appropriate widget for a dataclass field.

        Args:
            field_name: Name of the field
            field_type: Type annotation of the field
            current_value: Current value of the field

        Returns:
            Tuple of (widget, getter_function) where getter returns the current value
        """
        origin = get_origin(field_type)

        # Handle Optional types
        if origin is type(None) or (origin and str(origin).startswith("typing.Union")):
            args = get_args(field_type)
            if args:
                # Get the non-None type
                field_type = next((arg for arg in args if arg is not type(None)), args[0])
                origin = get_origin(field_type)

        # Boolean -> Checkbox
        if field_type is bool:
            widget = QtGui.QCheckBox()
            widget.setChecked(current_value if current_value is not None else False)
            widget.value_getter = lambda: widget.isChecked()
            return widget

        # Enum -> ComboBox
        if isinstance(field_type, type) and issubclass(field_type, Enum):
            widget = QtGui.QComboBox()
            for member in field_type:
                widget.addItem(member.value if hasattr(member, "value") else str(member), member)
            if current_value:
                index = widget.findData(current_value)
                if index >= 0:
                    widget.setCurrentIndex(index)
            widget.value_getter = lambda: widget.itemData(widget.currentIndex())
            return widget

        # List[str] -> Multi-line text area
        if origin is list:
            args = get_args(field_type)
            if args and args[0] is str:
                widget = QtGui.QPlainTextEdit()
                widget.setMaximumHeight(100)
                if current_value:
                    widget.setPlainText("\n".join(current_value))
                else:
                    widget.setPlainText("")
                widget.value_getter = lambda: [
                    line.strip() for line in widget.toPlainText().split("\n") if line.strip()
                ]
                return widget

        # Int -> SpinBox
        if field_type is int:
            widget = QtGui.QSpinBox()
            widget.setRange(-999999, 999999)
            widget.setValue(current_value if current_value is not None else 0)
            widget.value_getter = lambda: widget.value()
            return widget

        # Float -> DoubleSpinBox
        if field_type is float:
            widget = QtGui.QDoubleSpinBox()
            widget.setRange(-999999.0, 999999.0)
            widget.setDecimals(4)
            widget.setValue(current_value if current_value is not None else 0.0)
            widget.value_getter = lambda: widget.value()
            return widget

        # String -> LineEdit or PlainTextEdit
        if field_type is str:
            if field_name in DataclassGUIGenerator.MULTILINE_FIELDS:
                widget = QtGui.QPlainTextEdit()
                widget.setMaximumHeight(100)
                widget.setPlainText(current_value if current_value else "")
                widget.value_getter = lambda: widget.toPlainText()
            else:
                widget = QtGui.QLineEdit()
                widget.setText(current_value if current_value else "")
                widget.value_getter = lambda: widget.text()
            return widget

        # Fallback to string representation
        widget = QtGui.QLineEdit()
        widget.setText(str(current_value) if current_value is not None else "")
        widget.value_getter = lambda: widget.text()
        return widget

    @staticmethod
    def create_group_for_dataclass(
        dataclass_instance: Any, group_title: str
    ) -> tuple[QtGui.QGroupBox, Dict[str, QtGui.QWidget]]:
        """Create a QGroupBox with widgets for all fields in a dataclass.

        Args:
            dataclass_instance: Instance of a dataclass
            group_title: Title for the group box

        Returns:
            Tuple of (QGroupBox, dict mapping field_name to widget)
        """
        group = QtGui.QGroupBox(group_title)
        layout = QtGui.QFormLayout(group)
        widgets = {}

        for field in fields(dataclass_instance):
            # Skip private fields and complex types we don't handle
            if field.name.startswith("_"):
                continue

            current_value = getattr(dataclass_instance, field.name)
            field_type = field.type

            # Skip parameter_functions and other callable/complex types
            if field.name in ["parameter_functions", "parameter_order"]:
                continue

            widget = DataclassGUIGenerator.create_widget_for_field(
                field.name, field_type, current_value
            )

            label = DataclassGUIGenerator.get_field_label(field.name)
            layout.addRow(label, widget)
            widgets[field.name] = widget

        return group, widgets


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

    ROTATIONAL_AXIS_OPTIONS = [
        ("+X", [1, 0, 0]),
        ("-X", [-1, 0, 0]),
        ("+Y", [0, 1, 0]),
        ("-Y", [0, -1, 0]),
        ("+Z", [0, 0, 1]),
        ("-Z", [0, 0, -1]),
    ]

    def __init__(self, machine_filename: Optional[str] = None, parent=None):
        super().__init__(parent)
        self.setMinimumSize(700, 900)
        self.resize(700, 900)

        self.current_units = "metric"

        # Initialize machine object first (needed by setup methods)
        self.filename = machine_filename
        self.machine = None  # Store the Machine object

        if machine_filename:
            self.machine = MachineFactory.load_configuration(machine_filename)
        else:
            self.machine = Machine(name="New Machine")

        # Set window title with machine name
        title = translate("CAM_MachineEditor", "Machine Editor")
        if self.machine and self.machine.name:
            title += f" - {self.machine.name}"
        self.setWindowTitle(title)

        # Initialize widget and processor caches
        self.post_widgets = {}
        self.processor = {}

        self.layout = QtGui.QVBoxLayout(self)

        # Tab widget for sections
        self.tabs = QtGui.QTabWidget()
        self.layout.addWidget(self.tabs)

        # Machine tab
        self.machine_tab = QtGui.QWidget()
        self.tabs.addTab(self.machine_tab, translate("CAM_MachineEditor", "Machine"))
        self.setup_machine_tab()

        # Output Options tab
        self.output_tab = QtGui.QWidget()
        self.tabs.addTab(self.output_tab, translate("CAM_MachineEditor", "Output Options"))
        self.setup_output_tab()

        # G-Code Blocks tab
        self.blocks_tab = QtGui.QWidget()
        self.tabs.addTab(self.blocks_tab, translate("CAM_MachineEditor", "G-Code Blocks"))
        self.setup_blocks_tab()

        # Processing Options tab
        self.processing_tab = QtGui.QWidget()
        self.tabs.addTab(self.processing_tab, translate("CAM_MachineEditor", "Processing Options"))
        self.setup_processing_tab()

        # Check experimental flag for machine post processor
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        self.enable_machine_postprocessor = param.GetBool("EnableMachinePostprocessor", True)
        self.tabs.setTabVisible(
            self.tabs.indexOf(self.output_tab), self.enable_machine_postprocessor
        )
        self.tabs.setTabVisible(
            self.tabs.indexOf(self.blocks_tab), self.enable_machine_postprocessor
        )
        self.tabs.setTabVisible(
            self.tabs.indexOf(self.processing_tab), self.enable_machine_postprocessor
        )
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

        # Populate GUI from machine object
        self.populate_from_machine(self.machine)

        # Update spindle button state
        self._update_spindle_button_state()

        # Set focus and select the name field for new machines
        if not machine_filename:
            self.name_edit.setFocus()
            self.name_edit.selectAll()

    def normalize_text(self, edit, suffix, axis=None, field=None):
        """Normalize and validate text input for numeric fields with units.

        Parses user input, converts between metric/imperial units, and ensures
        proper formatting. Updates stored data for axes configuration.

        Args:
            edit: QLineEdit widget to update
            suffix: Unit suffix (e.g., "mm", "in", "deg")
            axis: Axis identifier for data storage
            field: Field name for data storage
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

        # Determine if this is a rotary axis by checking the suffix
        is_angular = suffix in ["deg", "deg/min"]

        # Convert to internal mm (or degrees for angles)
        if is_angular:
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

        # Update machine's axis directly (store in metric)
        if axis and field and self.machine:
            # Find the axis in machine.linear_axes or machine.rotary_axes (both are dicts)
            if axis in self.machine.linear_axes:
                ax = self.machine.linear_axes[axis]
                if field == "min":
                    ax.min_limit = internal_value
                elif field == "max":
                    ax.max_limit = internal_value
                elif field == "max_velocity":
                    ax.max_velocity = internal_value
            elif axis in self.machine.rotary_axes:
                ax = self.machine.rotary_axes[axis]
                if field == "min":
                    ax.min_limit = internal_value
                elif field == "max":
                    ax.max_limit = internal_value
                elif field == "max_velocity":
                    ax.max_velocity = internal_value

    # Signal handlers that update self.machine directly
    def _on_name_changed(self, text):
        """Update machine name when text changes."""
        if self.machine:
            self.machine.name = text
            # Update window title with new name
            title = translate("CAM_MachineEditor", "Machine Editor")
            if self.machine.name:
                title += f" - {self.machine.name}"
            self.setWindowTitle(title)

    def _on_rotary_sequence_changed(self, axis_name, value):
        """Update rotary axis sequence."""
        if self.machine and axis_name in self.machine.rotary_axes:
            self.machine.rotary_axes[axis_name].sequence = value

    def _on_rotary_joint_changed(self, axis_name, combo):
        """Update rotary axis joint/rotation vector."""
        if self.machine and axis_name in self.machine.rotary_axes:
            vector = combo.itemData(combo.currentIndex())
            self.machine.rotary_axes[axis_name].rotation_vector = FreeCAD.Vector(*vector)

    def _on_rotary_prefer_positive_changed(self, axis_name, checked):
        """Update rotary axis prefer_positive."""
        if self.machine and axis_name in self.machine.rotary_axes:
            self.machine.rotary_axes[axis_name].prefer_positive = checked

    def _on_spindle_field_changed(self, spindle_index, field_name, value):
        """Update spindle field in Machine object when UI field changes.

        Args:
            spindle_index: Index of the spindle in machine.spindles
            field_name: Name of the field being updated
            value: New value for the field
        """
        if self.machine and spindle_index < len(self.machine.spindles):
            spindle = self.machine.spindles[spindle_index]
            setattr(spindle, field_name, value)

    def _add_spindle(self):
        """Add a new spindle to the machine."""
        if self.machine and len(self.machine.spindles) < 9:
            new_index = len(self.machine.spindles) + 1
            new_spindle = Spindle(
                name=f"Spindle {new_index}",
                id=f"spindle{new_index}",
                max_power_kw=3.0,
                max_rpm=24000,
                min_rpm=6000,
                tool_change="manual",
            )
            self.machine.spindles.append(new_spindle)
            self.update_spindles()
            self._update_spindle_button_state()
            # Set focus to the new tab
            self.spindles_tabs.setCurrentIndex(len(self.machine.spindles) - 1)

    def _remove_spindle(self, index):
        """Remove a spindle from the machine with confirmation.

        Args:
            index: Index of the tab/spindle to remove
        """
        if not self.machine or len(self.machine.spindles) <= 1:
            return  # Don't allow removing the last spindle

        spindle = self.machine.spindles[index]
        reply = QtGui.QMessageBox.question(
            self,
            translate("CAM_MachineEditor", "Remove Spindle"),
            translate("CAM_MachineEditor", f"Remove '{spindle.name}'? This cannot be undone."),
            QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
            QtGui.QMessageBox.No,
        )

        if reply == QtGui.QMessageBox.Yes:
            self.machine.spindles.pop(index)
            self.update_spindles()
            self._update_spindle_button_state()

    def _update_spindle_button_state(self):
        """Enable/disable the add spindle button based on count."""
        if self.machine:
            self.add_spindle_button.setEnabled(len(self.machine.spindles) < 9)

    def _on_manufacturer_changed(self, text):
        """Update manufacturer when text changes."""
        if self.machine:
            self.machine.manufacturer = text

    def _on_description_changed(self, text):
        """Update description when text changes."""
        if self.machine:
            self.machine.description = text

    def _on_units_changed(self, index):
        """Update units and refresh axes display."""
        if self.machine:
            units = self.units_combo.itemData(index)
            self.machine.configuration_units = units
            self.current_units = units
            self.update_axes()

    def _on_type_changed(self, index):
        """Update machine type and refresh axes."""
        if self.machine:
            machine_type = self.type_combo.itemData(index)
            # Note: machine_type is a read-only property determined by axes configuration
            # Don't try to set it directly - instead modify the axes below

            # Rebuild axes in machine based on new type
            config = self.MACHINE_TYPES.get(machine_type, {})

            # Store existing axes for preservation
            old_linear_axes = self.machine.linear_axes.copy()
            old_rotary_axes = self.machine.rotary_axes.copy()

            # Clear and rebuild linear axes
            self.machine.linear_axes = {}
            for axis_name in config.get("linear", []):
                # Preserve existing axis if available
                if axis_name in old_linear_axes:
                    self.machine.linear_axes[axis_name] = old_linear_axes[axis_name]
                else:
                    # Create with defaults
                    self.machine.linear_axes[axis_name] = LinearAxis(
                        name=axis_name,
                        direction_vector=(
                            FreeCAD.Vector(1, 0, 0)
                            if axis_name == "X"
                            else (
                                FreeCAD.Vector(0, 1, 0)
                                if axis_name == "Y"
                                else FreeCAD.Vector(0, 0, 1)
                            )
                        ),
                        min_limit=0,
                        max_limit=1000,
                        max_velocity=10000,
                    )

            # Clear and rebuild rotary axes
            self.machine.rotary_axes = {}
            for axis_name in config.get("rotary", []):
                # Preserve existing axis if available
                if axis_name in old_rotary_axes:
                    self.machine.rotary_axes[axis_name] = old_rotary_axes[axis_name]
                else:
                    # Create with defaults
                    default_vector = (
                        [1, 0, 0]
                        if axis_name == "A"
                        else [0, 1, 0] if axis_name == "B" else [0, 0, 1]
                    )
                    self.machine.rotary_axes[axis_name] = RotaryAxis(
                        name=axis_name,
                        rotation_vector=FreeCAD.Vector(*default_vector),
                        min_limit=-180,
                        max_limit=180,
                        max_velocity=36000,
                        sequence=0,
                        prefer_positive=True,
                    )

            self.update_axes()

    def setup_machine_tab(self):
        """Set up the machine configuration tab with form fields.

        Creates input fields for machine name, manufacturer, description,
        units, type, spindle count, axes configuration, and spindles.
        Connects change handlers for dynamic updates.
        """
        layout = QtGui.QFormLayout(self.machine_tab)

        self.name_edit = QtGui.QLineEdit()
        self.name_edit.textChanged.connect(self._on_name_changed)
        layout.addRow(translate("CAM_MachineEditor", "Name"), self.name_edit)

        self.manufacturer_edit = QtGui.QLineEdit()
        self.manufacturer_edit.textChanged.connect(self._on_manufacturer_changed)
        layout.addRow(translate("CAM_MachineEditor", "Manufacturer"), self.manufacturer_edit)

        self.description_edit = QtGui.QLineEdit()
        self.description_edit.textChanged.connect(self._on_description_changed)
        layout.addRow(translate("CAM_MachineEditor", "Description"), self.description_edit)

        self.units_combo = QtGui.QComboBox()
        self.units_combo.addItem(translate("CAM_MachineEditor", "Metric"), "metric")
        self.units_combo.addItem(translate("CAM_MachineEditor", "Imperial"), "imperial")
        self.units_combo.currentIndexChanged.connect(self._on_units_changed)
        layout.addRow(translate("CAM_MachineEditor", "Units"), self.units_combo)

        self.type_combo = QtGui.QComboBox()
        for key, value in self.MACHINE_TYPES.items():
            self.type_combo.addItem(value["name"], key)
        self.type_combo.currentIndexChanged.connect(self._on_type_changed)
        layout.addRow(translate("CAM_MachineEditor", "Type"), self.type_combo)

        # Post Processor Selection
        self.post_processor_combo = QtGui.QComboBox()
        postProcessors = Path.Preferences.allEnabledPostProcessors([""])
        for post in postProcessors:
            self.post_processor_combo.addItem(post)
        self.post_processor_combo.currentIndexChanged.connect(self.updatePostProcessorTooltip)
        self.post_processor_combo.currentIndexChanged.connect(
            lambda: self._update_machine_field(
                "postprocessor_file_name", self.post_processor_combo.currentText()
            )
        )
        self.postProcessorDefaultTooltip = translate("CAM_MachineEditor", "Select a post processor")
        self.post_processor_combo.setToolTip(self.postProcessorDefaultTooltip)
        layout.addRow(translate("CAM_MachineEditor", "Post Processor"), self.post_processor_combo)

        # self.post_processor_args_edit = QtGui.QLineEdit()
        # self.post_processor_args_edit.textChanged.connect(
        #     lambda text: self._update_machine_field("postprocessor_args", text)
        # )
        # self.postProcessorArgsDefaultTooltip = translate(
        #     "CAM_MachineEditor", "Additional arguments"
        # )
        # self.post_processor_args_edit.setToolTip(self.postProcessorArgsDefaultTooltip)
        # layout.addRow(translate("CAM_MachineEditor", "Arguments"), self.post_processor_args_edit)

        # Axes group
        self.axes_group = QtGui.QGroupBox(translate("CAM_MachineEditor", "Axes"))
        self.axes_layout = QtGui.QVBoxLayout(self.axes_group)
        self.axes_group.setVisible(False)  # Initially hidden, shown when axes are configured
        layout.addRow(self.axes_group)

        # Spindles group
        self.spindles_group = QtGui.QGroupBox(translate("CAM_MachineEditor", "Spindles"))
        spindles_layout = QtGui.QVBoxLayout(self.spindles_group)

        self.spindles_tabs = QtGui.QTabWidget()
        self.spindles_tabs.setTabsClosable(True)
        self.spindles_tabs.tabCloseRequested.connect(self._remove_spindle)

        # Add + button to the tab bar corner, vertically centered
        corner_container = QtGui.QWidget()
        corner_container_layout = QtGui.QVBoxLayout(corner_container)
        corner_container_layout.setContentsMargins(0, 0, 0, 0)
        corner_container_layout.setSpacing(0)
        corner_container_layout.addStretch()
        self.add_spindle_button = QtGui.QPushButton("+")
        self.add_spindle_button.setToolTip(translate("CAM_MachineEditor", "Add Spindle"))
        self.add_spindle_button.clicked.connect(self._add_spindle)
        self.add_spindle_button.setSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        corner_container_layout.addWidget(self.add_spindle_button, 0, QtCore.Qt.AlignCenter)
        corner_container_layout.addStretch()
        self.spindles_tabs.setCornerWidget(corner_container, QtCore.Qt.TopRightCorner)

        spindles_layout.addWidget(self.spindles_tabs)
        layout.addRow(self.spindles_group)

    def update_axes(self):
        """Update the axes configuration UI based on machine type and units.

        Dynamically creates input fields for all axes based on the selected
        machine type. Uses flattened structure with type property.
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
        self.axis_edits = {}

        # Clear existing axes widgets and disconnect signals
        for i in reversed(range(self.axes_layout.count())):
            widget = self.axes_layout.itemAt(i).widget()
            if widget:
                # Disconnect all signals to prevent callbacks to deleted widgets
                widget.blockSignals(True)
                widget.deleteLater()
                self.axes_layout.removeWidget(widget)

        # Get current type
        type_key = self.type_combo.itemData(self.type_combo.currentIndex())
        if not type_key:
            return
        config = self.MACHINE_TYPES[type_key]

        # Create axes group
        all_axes = config.get("linear", []) + config.get("rotary", [])
        if not all_axes:
            self.axes_group.setVisible(False)
            return

        # Get axes directly from machine object
        linear_axes = list(self.machine.linear_axes.keys()) if self.machine else []
        rotary_axes = list(self.machine.rotary_axes.keys()) if self.machine else []

        # Linear axes
        if linear_axes:
            linear_group = QtGui.QGroupBox("Linear Axes")
            linear_layout = QtGui.QFormLayout(linear_group)

            for axis in linear_axes:
                axis_obj = self.machine.linear_axes[axis]
                converted_min = (
                    axis_obj.min_limit if units == "metric" else axis_obj.min_limit / 25.4
                )
                min_edit = QtGui.QLineEdit()
                min_edit.setText(f"{converted_min:.2f}{length_suffix}")
                min_edit.editingFinished.connect(
                    lambda edit=min_edit, suffix=length_suffix.strip(), ax=axis, fld="min": self.normalize_text(
                        edit, suffix, ax, fld
                    )
                )

                converted_max = (
                    axis_obj.max_limit if units == "metric" else axis_obj.max_limit / 25.4
                )
                max_edit = QtGui.QLineEdit()
                max_edit.setText(f"{converted_max:.2f}{length_suffix}")
                max_edit.editingFinished.connect(
                    lambda edit=max_edit, suffix=length_suffix.strip(), ax=axis, fld="max": self.normalize_text(
                        edit, suffix, ax, fld
                    )
                )

                converted_vel = (
                    axis_obj.max_velocity if units == "metric" else axis_obj.max_velocity / 25.4
                )
                vel_edit = QtGui.QLineEdit()
                vel_edit.setText(f"{converted_vel:.2f}{vel_suffix}")
                vel_edit.editingFinished.connect(
                    lambda edit=vel_edit, suffix=vel_suffix.strip(), ax=axis, fld="max_velocity": self.normalize_text(
                        edit, suffix, ax, fld
                    )
                )

                axis_layout = QtGui.QHBoxLayout()
                axis_layout.addWidget(QtGui.QLabel("Min"))
                axis_layout.addWidget(min_edit)
                axis_layout.addWidget(QtGui.QLabel("Max"))
                axis_layout.addWidget(max_edit)
                axis_layout.addWidget(QtGui.QLabel("Max Vel"))
                axis_layout.addWidget(vel_edit)

                linear_layout.addRow(f"{axis}", axis_layout)
                self.axis_edits[axis] = {
                    "min": min_edit,
                    "max": max_edit,
                    "max_velocity": vel_edit,
                    "type": "linear",
                }
            self.axes_layout.addWidget(linear_group)

        # Rotary axes
        if rotary_axes:
            rotary_group = QtGui.QGroupBox("Rotary Axes")
            rotary_layout = QtGui.QFormLayout(rotary_group)

            for axis in rotary_axes:
                axis_obj = self.machine.rotary_axes[axis]

                min_edit = QtGui.QLineEdit()
                min_edit.setText(f"{axis_obj.min_limit:.2f}{angle_suffix}")
                min_edit.editingFinished.connect(
                    lambda edit=min_edit, suffix=angle_suffix.strip(), ax=axis, fld="min": self.normalize_text(
                        edit, suffix, ax, fld
                    )
                )

                max_edit = QtGui.QLineEdit()
                max_edit.setText(f"{axis_obj.max_limit:.2f}{angle_suffix}")
                max_edit.editingFinished.connect(
                    lambda edit=max_edit, suffix=angle_suffix.strip(), ax=axis, fld="max": self.normalize_text(
                        edit, suffix, ax, fld
                    )
                )

                vel_edit = QtGui.QLineEdit()
                vel_edit.setText(f"{axis_obj.max_velocity:.2f}{angle_vel_suffix}")
                vel_edit.editingFinished.connect(
                    lambda edit=vel_edit, suffix=angle_vel_suffix.strip(), ax=axis, fld="max_velocity": self.normalize_text(
                        edit, suffix, ax, fld
                    )
                )

                # Sequence number for rotary axes
                sequence_spin = QtGui.QSpinBox()
                sequence_spin.setRange(0, 10)
                sequence_spin.setValue(axis_obj.sequence)
                sequence_spin.valueChanged.connect(
                    lambda value, ax=axis: self._on_rotary_sequence_changed(ax, value)
                )

                # Joint (rotation axis) combo
                joint_combo = QtGui.QComboBox()
                for label, vector in self.ROTATIONAL_AXIS_OPTIONS:
                    joint_combo.addItem(label, vector)
                # Get rotation vector from axis object
                rotation_vec = [
                    axis_obj.rotation_vector.x,
                    axis_obj.rotation_vector.y,
                    axis_obj.rotation_vector.z,
                ]
                # Find matching option and set it
                for i, (label, vector) in enumerate(self.ROTATIONAL_AXIS_OPTIONS):
                    if vector == rotation_vec:
                        joint_combo.setCurrentIndex(i)
                        break
                joint_combo.currentIndexChanged.connect(
                    lambda index, ax=axis, combo=joint_combo: self._on_rotary_joint_changed(
                        ax, combo
                    )
                )

                prefer_positive = QtGui.QCheckBox()
                prefer_positive.setChecked(axis_obj.prefer_positive)
                prefer_positive.stateChanged.connect(
                    lambda state, ax=axis: self._on_rotary_prefer_positive_changed(
                        ax, state == QtCore.Qt.Checked
                    )
                )

                # Grid layout
                axis_grid = QtGui.QGridLayout()

                # Row 0: Min, Max, Vel
                axis_grid.addWidget(QtGui.QLabel("Min"), 0, 0, QtCore.Qt.AlignRight)
                axis_grid.addWidget(min_edit, 0, 1)
                axis_grid.addWidget(QtGui.QLabel("Max"), 0, 2, QtCore.Qt.AlignRight)
                axis_grid.addWidget(max_edit, 0, 3)
                axis_grid.addWidget(QtGui.QLabel("Max Vel"), 0, 4, QtCore.Qt.AlignRight)
                axis_grid.addWidget(vel_edit, 0, 5)

                # Row 1: Sequence, Joint, Prefer+
                axis_grid.addWidget(QtGui.QLabel("Sequence"), 1, 0, QtCore.Qt.AlignRight)
                axis_grid.addWidget(sequence_spin, 1, 1)
                axis_grid.addWidget(QtGui.QLabel("Joint"), 1, 2, QtCore.Qt.AlignRight)
                axis_grid.addWidget(joint_combo, 1, 3)
                axis_grid.addWidget(QtGui.QLabel("Prefer+"), 1, 4, QtCore.Qt.AlignRight)
                axis_grid.addWidget(prefer_positive, 1, 5)

                axis_label = QtGui.QLabel(f"{axis}")
                axis_label.setMinimumWidth(30)  # Prevent layout shift when axis names change
                rotary_layout.addRow(axis_label, axis_grid)

                self.axis_edits[axis] = {
                    "min": min_edit,
                    "max": max_edit,
                    "max_velocity": vel_edit,
                    "sequence": sequence_spin,
                    "joint": joint_combo,
                    "prefer_positive": prefer_positive,
                    "type": "angular",
                }
            self.axes_layout.addWidget(rotary_group)

        # Show axes group if any axes configured
        self.axes_group.setVisible(bool(linear_axes or rotary_axes))

    def update_spindles(self):
        """Update the spindle configuration UI based on spindle count.

        Dynamically creates tabbed interface for multiple spindles with
        input fields for name, ID, power, speed, and tool holder.
        Updates Machine.spindles directly.
        """
        # Clear existing spindle tabs - this properly disconnects signals
        while self.spindles_tabs.count() > 0:
            tab = self.spindles_tabs.widget(0)
            if tab:
                tab.blockSignals(True)
                tab.deleteLater()
            self.spindles_tabs.removeTab(0)

        self.spindle_edits = []
        count = len(self.machine.spindles) if self.machine else 1

        # Ensure machine has at least 1 spindle
        if self.machine:
            while len(self.machine.spindles) < count:
                self.machine.spindles.append(
                    Spindle(
                        name=f"Spindle {len(self.machine.spindles) + 1}",
                        id=f"spindle{len(self.machine.spindles) + 1}",
                        max_power_kw=3.0,
                        max_rpm=24000,
                        min_rpm=6000,
                        tool_change="manual",
                    )
                )
            while len(self.machine.spindles) > count:
                self.machine.spindles.pop()

        for i in range(count):
            tab = QtGui.QWidget()
            layout = QtGui.QFormLayout(tab)

            # Get spindle object or use defaults
            spindle = (
                self.machine.spindles[i]
                if self.machine and i < len(self.machine.spindles)
                else None
            )

            name_edit = QtGui.QLineEdit()
            name_edit.setText(spindle.name if spindle else f"Spindle {i+1}")
            name_edit.textChanged.connect(
                lambda text, idx=i: self._on_spindle_field_changed(idx, "name", text)
            )
            layout.addRow("Name", name_edit)

            id_edit = QtGui.QLineEdit()
            id_edit.setText(spindle.id if spindle and spindle.id else f"spindle{i+1}")
            id_edit.textChanged.connect(
                lambda text, idx=i: self._on_spindle_field_changed(idx, "id", text)
            )
            layout.addRow("ID", id_edit)

            max_power_edit = QtGui.QDoubleSpinBox()
            max_power_edit.setRange(0, 100)
            max_power_edit.setValue(spindle.max_power_kw if spindle else 3.0)
            max_power_edit.valueChanged.connect(
                lambda value, idx=i: self._on_spindle_field_changed(idx, "max_power_kw", value)
            )
            layout.addRow("Max Power (kW)", max_power_edit)

            max_rpm_edit = QtGui.QSpinBox()
            max_rpm_edit.setRange(0, 100000)
            max_rpm_edit.setValue(int(spindle.max_rpm) if spindle else 24000)
            max_rpm_edit.valueChanged.connect(
                lambda value, idx=i: self._on_spindle_field_changed(idx, "max_rpm", value)
            )
            layout.addRow("Max RPM", max_rpm_edit)

            min_rpm_edit = QtGui.QSpinBox()
            min_rpm_edit.setRange(0, 100000)
            min_rpm_edit.setValue(int(spindle.min_rpm) if spindle else 6000)
            min_rpm_edit.valueChanged.connect(
                lambda value, idx=i: self._on_spindle_field_changed(idx, "min_rpm", value)
            )
            layout.addRow("Min RPM", min_rpm_edit)

            tool_change_combo = QtGui.QComboBox()
            tool_change_combo.addItem("Manual", "manual")
            tool_change_combo.addItem("ATC", "atc")
            if spindle:
                index = tool_change_combo.findData(spindle.tool_change)
                if index >= 0:
                    tool_change_combo.setCurrentIndex(index)
            tool_change_combo.currentIndexChanged.connect(
                lambda idx, spindle_idx=i, combo=tool_change_combo: self._on_spindle_field_changed(
                    spindle_idx, "tool_change", combo.itemData(combo.currentIndex())
                )
            )
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

    def setup_output_tab(self):
        """Set up the output options configuration tab."""
        # Use scroll area for all the options
        scroll = QtGui.QScrollArea()
        scroll.setWidgetResizable(True)
        scroll_widget = QtGui.QWidget()
        layout = QtGui.QVBoxLayout(scroll_widget)
        scroll.setWidget(scroll_widget)

        main_layout = QtGui.QVBoxLayout(self.output_tab)
        main_layout.addWidget(scroll)

        # === Output Options ===
        if self.machine:
            output_group, output_widgets = DataclassGUIGenerator.create_group_for_dataclass(
                self.machine.output, "Output Options"
            )
            layout.addWidget(output_group)
            self._connect_widgets_to_machine(output_widgets, "output")

        layout.addStretch()

    def setup_blocks_tab(self):
        """Set up the G-Code blocks configuration tab."""
        # Use scroll area for all the options
        scroll = QtGui.QScrollArea()
        scroll.setWidgetResizable(True)
        scroll_widget = QtGui.QWidget()
        layout = QtGui.QVBoxLayout(scroll_widget)
        scroll.setWidget(scroll_widget)

        main_layout = QtGui.QVBoxLayout(self.blocks_tab)
        main_layout.addWidget(scroll)

        # === G-Code Blocks ===
        if self.machine:
            blocks_group, blocks_widgets = DataclassGUIGenerator.create_group_for_dataclass(
                self.machine.blocks, "G-Code Blocks"
            )
            layout.addWidget(blocks_group)
            self._connect_widgets_to_machine(blocks_widgets, "blocks")

        layout.addStretch()

    def setup_processing_tab(self):
        """Set up the processing options configuration tab."""
        # Use scroll area for all the options
        scroll = QtGui.QScrollArea()
        scroll.setWidgetResizable(True)
        scroll_widget = QtGui.QWidget()
        layout = QtGui.QVBoxLayout(scroll_widget)
        scroll.setWidget(scroll_widget)

        main_layout = QtGui.QVBoxLayout(self.processing_tab)
        main_layout.addWidget(scroll)

        # === Processing Options ===
        if self.machine:
            processing_group, processing_widgets = DataclassGUIGenerator.create_group_for_dataclass(
                self.machine.processing, "Processing Options"
            )
            layout.addWidget(processing_group)
            self._connect_widgets_to_machine(processing_widgets, "processing")

        layout.addStretch()

    def _connect_widgets_to_machine(self, widgets: Dict[str, QtGui.QWidget], parent_path: str):
        """Connect widgets to update Machine object fields.

        Args:
            widgets: Dictionary of field_name -> widget
            parent_path: Path to parent object (e.g., 'output', 'precision')
        """
        for field_name, widget in widgets.items():
            field_path = f"{parent_path}.{field_name}"

            # Store widget for later population
            self.post_widgets[field_path] = widget

            # Connect based on widget type
            if isinstance(widget, QtGui.QCheckBox):
                # Use a wrapper to avoid lambda capture issues
                def make_checkbox_handler(path):
                    def handler(state):
                        # Qt.Checked = 2, Qt.Unchecked = 0
                        value = state == 2
                        self._update_machine_field(path, value)

                    return handler

                handler_func = make_checkbox_handler(field_path)
                widget.stateChanged.connect(handler_func)
            elif isinstance(widget, QtGui.QLineEdit):

                def make_lineedit_handler(path):
                    def handler(text):
                        self._update_machine_field(path, text)

                    return handler

                widget.textChanged.connect(make_lineedit_handler(field_path))
            elif isinstance(widget, QtGui.QPlainTextEdit):

                def make_plaintext_handler(path, w):
                    def handler():
                        self._update_machine_field(path, w.value_getter())

                    return handler

                widget.textChanged.connect(make_plaintext_handler(field_path, widget))
            elif isinstance(widget, (QtGui.QSpinBox, QtGui.QDoubleSpinBox)):

                def make_spinbox_handler(path):
                    def handler(value):
                        self._update_machine_field(path, value)

                    return handler

                widget.valueChanged.connect(make_spinbox_handler(field_path))
            elif isinstance(widget, QtGui.QComboBox):

                def make_combo_handler(path, w):
                    def handler(index):
                        self._update_machine_field(path, w.value_getter())

                    return handler

                widget.currentIndexChanged.connect(make_combo_handler(field_path, widget))

    def _update_machine_field(self, field_path, value):
        """Update a nested field in the machine object using dot notation."""
        if not self.machine:
            return

        try:
            parts = field_path.split(".")
            obj = self.machine

            # Navigate to the parent object
            for part in parts[:-1]:
                obj = getattr(obj, part)

            # Set the final field
            final_field = parts[-1]
            current_value = getattr(obj, final_field, None)

            # Only update if value actually changed
            if current_value != value:
                setattr(obj, final_field, value)
        except Exception as e:
            Path.Log.error(f"Error updating {field_path}: {e}")

    def _populate_post_widgets_from_machine(self, machine: Machine):
        """Populate dynamically generated post-processor widgets from machine object.

        This updates all the nested dataclass widgets (output, precision, formatting,
        blocks, processing) and top-level machine post-processing options.

        Args:
            machine: Machine object to read values from
        """

        # Helper to set widget value without triggering signals
        def set_widget_value_silent(widget, value):
            if isinstance(widget, QtGui.QCheckBox):
                widget.blockSignals(True)
                widget.setChecked(value)
                widget.blockSignals(False)
            elif isinstance(widget, QtGui.QLineEdit):
                widget.blockSignals(True)
                widget.setText(str(value) if value is not None else "")
                widget.blockSignals(False)
            elif isinstance(widget, QtGui.QPlainTextEdit):
                widget.blockSignals(True)
                if isinstance(value, list):
                    widget.setPlainText("\n".join(value))
                else:
                    widget.setPlainText(str(value) if value is not None else "")
                widget.blockSignals(False)
            elif isinstance(widget, QtGui.QSpinBox):
                widget.blockSignals(True)
                widget.setValue(int(value) if value is not None else 0)
                widget.blockSignals(False)
            elif isinstance(widget, QtGui.QDoubleSpinBox):
                widget.blockSignals(True)
                widget.setValue(float(value) if value is not None else 0.0)
                widget.blockSignals(False)
            elif isinstance(widget, QtGui.QComboBox):
                widget.blockSignals(True)
                if hasattr(value, "value"):  # Enum
                    value = value.value
                # Find the item with this value
                for i in range(widget.count()):
                    item_data = widget.itemData(i)
                    if item_data == value or widget.itemText(i) == str(value):
                        widget.setCurrentIndex(i)
                        break
                widget.blockSignals(False)

        # Update nested dataclass fields
        dataclass_groups = [
            ("output", machine.output),
            ("blocks", machine.blocks),
            ("processing", machine.processing),
        ]

        for group_name, dataclass_obj in dataclass_groups:
            if dataclass_obj is None:
                continue

            # Get all fields from the dataclass
            from dataclasses import fields as dataclass_fields

            for field in dataclass_fields(dataclass_obj):
                widget_key = f"{group_name}.{field.name}"
                # Find the widget in post_widgets (it might be stored with or without the parent path)
                widget = None
                if widget_key in self.post_widgets:
                    widget = self.post_widgets[widget_key]
                elif field.name in self.post_widgets:
                    widget = self.post_widgets[field.name]

                if widget:
                    value = getattr(dataclass_obj, field.name)
                    set_widget_value_silent(widget, value)

        # Update top-level machine post-processing fields
        top_level_fields = [
            "use_tlo",
            "stop_spindle_for_tool_change",
            "enable_coolant",
            "enable_machine_specific_commands",
        ]

        for field_name in top_level_fields:
            if field_name in self.post_widgets:
                widget = self.post_widgets[field_name]
                value = getattr(machine, field_name, None)
                if value is not None:
                    set_widget_value_silent(widget, value)

    def getPostProcessor(self, name):
        if name not in self.processor:
            processor = PostProcessorFactory.get_post_processor(None, name)
            self.processor[name] = processor
            return processor
        return self.processor[name]

    def setPostProcessorTooltip(self, widget, name, default):
        processor = self.getPostProcessor(name)
        if processor.tooltip:
            widget.setToolTip(processor.tooltip)
        else:
            widget.setToolTip(default)

    def updatePostProcessorTooltip(self):
        name = str(self.post_processor_combo.currentText())
        if name:
            self.setPostProcessorTooltip(
                self.post_processor_combo, name, self.postProcessorDefaultTooltip
            )
            processor = self.getPostProcessor(name)
            # if processor.tooltipArgs:
            #     self.post_processor_args_edit.setToolTip(processor.tooltipArgs)
            # else:
            #     self.post_processor_args_edit.setToolTip(self.postProcessorArgsDefaultTooltip)
        else:
            self.post_processor_combo.setToolTip(self.postProcessorDefaultTooltip)
            # self.post_processor_args_edit.setToolTip(self.postProcessorArgsDefaultTooltip)

    def populate_from_machine(self, machine: Machine):
        """Populate UI fields from Machine object.

        Args:
            machine: Machine object containing configuration
        """
        self.name_edit.setText(machine.name)
        self.manufacturer_edit.setText(machine.manufacturer)
        self.description_edit.setText(machine.description)
        units = machine.configuration_units
        self.units_combo.blockSignals(True)
        index = self.units_combo.findData(units)
        if index >= 0:
            self.units_combo.setCurrentIndex(index)
        self.units_combo.blockSignals(False)
        self.current_units = units
        machine_type = machine.machine_type
        self.type_combo.blockSignals(True)
        index = self.type_combo.findData(machine_type)
        if index >= 0:
            self.type_combo.setCurrentIndex(index)
        self.type_combo.blockSignals(False)

        # Post processor selection
        if self.enable_machine_postprocessor:
            post_processor = machine.postprocessor_file_name
            index = self.post_processor_combo.findText(post_processor, QtCore.Qt.MatchFixedString)
            if index >= 0:
                self.post_processor_combo.setCurrentIndex(index)
            else:
                self.post_processor_combo.setCurrentIndex(0)
            self.updatePostProcessorTooltip()

            # Post processor arguments
            # self.post_processor_args_edit.setText(machine.postprocessor_args)

        # Get units for suffixes in populate
        units = self.units_combo.itemData(self.units_combo.currentIndex())

        # Update axes UI after loading machine data
        self.update_axes()

        # Ensure at least 1 spindle
        if len(machine.spindles) == 0:
            machine.spindles.append(
                Spindle(
                    name="Spindle 1",
                    id="spindle1",
                    max_power_kw=3.0,
                    max_rpm=24000,
                    min_rpm=6000,
                    tool_change="manual",
                )
            )
        self.update_spindles()  # Update spindles UI
        self._update_spindle_button_state()

        # Post processor configuration - populate dynamically generated widgets
        if self.enable_machine_postprocessor and hasattr(self, "post_widgets"):
            # Update all post-processor widgets from machine object
            self._populate_post_widgets_from_machine(machine)

    def to_machine(self) -> Machine:
        """Convert UI state to Machine object.

        Returns:
            Machine object with configuration from UI
        """
        # The machine object is already up-to-date from signal handlers
        # (both axes and spindles update in real-time)
        return self.machine

    def to_data(self) -> Dict[str, Any]:
        """Convert UI state to machine data dictionary.

        Returns:
            Dict containing complete machine configuration in JSON format
        """
        # Update machine from UI first (spindles need to be synchronized)
        machine = self.to_machine()

        # Use Machine's to_dict method for serialization
        return machine.to_dict()

    def toggle_editor_mode(self):
        """Toggle between form view and text editor view."""
        if self.text_mode:
            # Switching from text to form mode
            try:
                # Parse JSON from text editor
                json_text = self.text_editor.toPlainText()
                data = json.loads(json_text)
                # Create Machine object from JSON and populate form
                self.machine = Machine.from_dict(data)
                self.populate_from_machine(self.machine)
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
                # self.machine is already up-to-date from signal handlers
                # Just serialize it to JSON
                data = self.machine.to_dict()
                json_text = json.dumps(data, indent=4, sort_keys=True)
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
        # Check for duplicate machine names when creating new machines
        if self.text_mode:
            try:
                json_text = self.text_editor.toPlainText()
                data = json.loads(json_text)
                machine_name = data.get("machine", {}).get("name", "")
            except json.JSONDecodeError:
                machine_name = ""
        else:
            machine_name = self.name_edit.text().strip()

        # Check for duplicate machine names
        if machine_name:
            existing_machines = MachineFactory.list_configurations()
            # Case-insensitive check to match get_machine behavior
            machine_name_lower = machine_name.lower()
            existing_names_lower = [name.lower() for name in existing_machines]

            # For existing machines, allow keeping the same name (case-insensitive)
            current_name_allowed = False
            if self.filename:
                try:
                    current_machine = MachineFactory.load_configuration(self.filename)
                    current_name = current_machine.name.lower()
                    if machine_name_lower == current_name:
                        current_name_allowed = True
                except Exception:
                    # Failed to load current machine configuration, assume name is not allowed
                    pass

            if machine_name_lower in existing_names_lower and not current_name_allowed:
                QtGui.QMessageBox.warning(
                    self,
                    translate("CAM_MachineEditor", "Duplicate Machine Name"),
                    translate(
                        "CAM_MachineEditor",
                        "A machine with the name '{}' already exists. Please choose a different name.",
                    ).format(machine_name),
                )
                return

        try:
            if self.text_mode:
                # If in text mode, parse JSON and update Machine
                json_text = self.text_editor.toPlainText()
                data = json.loads(json_text)
                self.machine = Machine.from_dict(data)

            # self.machine is already up-to-date from signal handlers, just save it
            if self.filename:
                saved_path = MachineFactory.save_configuration(self.machine, self.filename)
            else:
                saved_path = MachineFactory.save_configuration(self.machine)
                self.filename = saved_path.name
            self.path = str(saved_path)  # Keep for compatibility

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
        super().accept()
