# -*- coding: utf-8 -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
Generic Postprocessor for plasma, laser, and waterjet cutters that require a pierce delay
"""

from typing import Any, Dict
import copy

from Path.Post.Processor import PostProcessor

import Path
import FreeCAD

translate = FreeCAD.Qt.translate

DEBUG = False
if DEBUG:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

Path.Log.debug("generic_plasma_post.py module loaded")

# Define some types that are used throughout this file.
Values = Dict[str, Any]


class GenericPlasma(PostProcessor):
    """
    The GenericPlasma post processor class.
    """

    @classmethod
    def get_common_property_schema(cls):
        Path.Log.debug("GenericPlasma.get_common_property_schema() called")
        common_props = copy.deepcopy(super().get_common_property_schema())

        # Override defaults for GenericPlasma
        for prop in common_props:
            if prop["name"] == "file_extension":
                prop["default"] = "nc"
            elif prop["name"] == "supports_tool_radius_compensation":
                prop["default"] = True
            elif prop["name"] == "preamble":
                prop["default"] = "G17 G54 G40 G49 G80 G90"
            elif prop["name"] == "postamble":
                prop["default"] = "M05\nG17 G54 G90 G80 G40\nM2"
            elif prop["name"] == "safetyblock":
                prop["default"] = "G40 G49 G80"

        return common_props

    @classmethod
    def get_property_schema(cls):
        """Return schema for plasma-specific configurable properties."""
        return [
            {
                "name": "pierce_delay",
                "type": "integer",
                "label": translate("CAM", "Pierce Delay"),
                "default": 1000,
                "min": 0,
                "max": 10000,
                "help": translate(
                    "CAM",
                    "Pierce delay in milliseconds to wait after torch ignites (M3/M4) before starting movement",
                ),
            },
            {
                "name": "cooling_delay",
                "type": "integer",
                "label": translate("CAM", "Cooling Delay"),
                "default": 500,
                "min": 0,
                "max": 10000,
                "help": translate(
                    "CAM",
                    "Cooling delay in milliseconds to wait after torch extinguishes (M5) before movement",
                ),
            },
            {
                "name": "torch_zaxis_control",
                "type": "bool",
                "label": translate("CAM", "Torch Z-Axis Control"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Torch ignites (M3) on Z- movement and extinguishes (M5) on Z+ movement. "
                    "When disabled, M3/M5 commands are output as-is.",
                ),
            },
            {
                "name": "force_rapid_feeds",
                "type": "bool",
                "label": translate("CAM", "Force Rapid Feeds"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Force rapid-feed speeds for all feed specified commands. "
                    "Useful for dry runs to verify paths without cutting.",
                ),
            },
        ]

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "Generic Plasma post processor"),
        tooltipargs=[],
        units="Metric",
    ) -> None:
        super().__init__(
            job=job,
            tooltip=tooltip,
            tooltipargs=tooltipargs,
            units=units,
        )
        Path.Log.debug("Generic Plasma post processor initialized.")

        # State tracking for plasma-specific features
        self._first_entry_done = False
        self._torch_active = False
        self._last_z = None  # Track last Z position for direction detection
        self._last_z_direction = None  # Track Z movement direction

    def _reset_plasma_state(self):
        """Reset plasma-specific state tracking for each operation."""
        self._first_entry_done = False
        self._torch_active = False
        self._last_z = None
        self._last_z_direction = None

    def _get_property_value(self, name, default):
        """Get a property value from machine configuration with fallback to default."""
        if self._machine and hasattr(self._machine, "postprocessor_properties"):
            return self._machine.postprocessor_properties.get(name, default)
        return default

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor."""
        #
        super().init_values(values)
        #
        # Set any values here that need to override the default values set
        # in the parent routine.
        #
        values["ENABLE_COOLANT"] = True
        #
        # The order of parameters.
        #
        # linuxcnc doesn't want K properties on XY plane; Arcs need work.
        #
        values["PARAMETER_ORDER"] = [
            "X",
            "Y",
            "Z",
            "A",
            "B",
            "C",
            "I",
            "J",
            "F",
            "S",
            "T",
            "Q",
            "R",
            "L",
            "H",
            "D",
            "P",
        ]

        values["MACHINE_NAME"] = "GenericPlasma"
        values["POSTPROCESSOR_FILE_NAME"] = __name__
        #
        # Load preamble from machine configuration if available
        #
        if self._machine and hasattr(self._machine, "postprocessor_properties"):
            props = self._machine.postprocessor_properties
            values["PREAMBLE"] = props.get("preamble", "")
        else:
            values["PREAMBLE"] = ""

    def _inject_pierce_delay(self, postables):
        """Inject pierce delay after M3/M4 commands when torch is activated."""
        pierce_delay_ms = self._get_property_value("pierce_delay", 1000)
        if pierce_delay_ms <= 0:
            return

        # Convert milliseconds to seconds for G4 command
        pierce_delay_sec = pierce_delay_ms / 1000.0

        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    new_commands = []
                    for cmd in item.Path.Commands:
                        new_commands.append(cmd)
                        # After torch on commands, inject G4 pause
                        if cmd.Name in ["M3", "M4"]:
                            # Create G4 dwell command with P parameter (seconds)
                            pause_cmd = Path.Command("G4", {"P": pierce_delay_sec})
                            new_commands.append(pause_cmd)
                    # Replace Path with modified command list
                    item.Path = Path.Path(new_commands)

    def _inject_cooling_delay(self, postables):
        """Inject cooling delay after M5 commands when torch is extinguished."""
        cooling_delay_ms = self._get_property_value("cooling_delay", 500)
        if cooling_delay_ms <= 0:
            return

        # Convert milliseconds to seconds for G4 command
        cooling_delay_sec = cooling_delay_ms / 1000.0

        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    new_commands = []
                    for cmd in item.Path.Commands:
                        new_commands.append(cmd)
                        # After torch off command, inject G4 pause
                        if cmd.Name == "M5":
                            # Create G4 dwell command with P parameter (seconds)
                            pause_cmd = Path.Command("G4", {"P": cooling_delay_sec})
                            new_commands.append(pause_cmd)
                    # Replace Path with modified command list
                    item.Path = Path.Path(new_commands)

    def _inject_torch_control(self, postables):
        """Handle torch ignition/extinguishment based on Z-axis movement."""
        if not self._get_property_value("torch_zaxis_control", True):
            return

        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    # Reset state for each operation
                    self._reset_plasma_state()

                    # Get operation heights from the path object
                    pierce_height = self._get_operation_height(item, "StartDepth", 0)

                    new_commands = []
                    for cmd in item.Path.Commands:
                        # Track Z movement direction
                        z_direction = None
                        if "Z" in cmd.Parameters:
                            # This is a Z move - determine direction
                            if self._last_z is not None and "Z" in cmd.Parameters:
                                if cmd.Parameters["Z"] < self._last_z:
                                    z_direction = "down"
                                elif cmd.Parameters["Z"] > self._last_z:
                                    z_direction = "up"
                            self._last_z = cmd.Parameters["Z"]

                        # Handle torch control based on Z movement
                        if z_direction == "down" and not self._torch_active:
                            if (
                                not self._get_property_value("mark_entry_only", False)
                                or not self._first_entry_done
                            ):
                                # Move to pierce height first if not already there
                                if hasattr(self, "_last_z") and self._last_z > pierce_height:
                                    move_cmd = Path.Command("G0", {"Z": pierce_height})
                                    new_commands.append(move_cmd)

                                # Insert M3 before Z- move (torch ignition)
                                new_commands.append(Path.Command("M3"))
                                self._torch_active = True
                                self._first_entry_done = True
                        elif z_direction == "up" and self._torch_active:
                            # Insert M5 after Z+ move (torch extinguish)
                            new_commands.append(Path.Command("M5"))
                            self._torch_active = False

                        new_commands.append(cmd)
                    # Replace Path with modified command list
                    item.Path = Path.Path(new_commands)

    def _get_operation_height(self, item, height_type, default):
        """Get operation height (StartDepth/FinalDepth) from path object."""
        try:
            # Try to get the height from the path object's properties
            if hasattr(item, height_type):
                value = getattr(item, height_type)
                if value is not None:
                    return float(value)
            elif hasattr(item, "Base") and hasattr(item.Base, height_type):
                value = getattr(item.Base, height_type)
                if value is not None:
                    return float(value)
            else:
                # Try to get from proxy object
                proxy = getattr(item, "Proxy", None)
                if proxy and hasattr(proxy, height_type):
                    value = getattr(proxy, height_type)
                    if value is not None:
                        return float(value)
        except (AttributeError, TypeError, ValueError) as e:
            Path.Log.debug(f"GenericPlasma: Could not get {height_type}: {e}")
        return default

    def _inject_mark_entry_only(self, postables):
        """Mark first entry points only (Z- to cut height, torch on, short delay, torch off, Z+ to clearance)."""
        if not self._get_property_value("mark_entry_only", False):
            return

        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    # Reset state for each operation
                    self._reset_plasma_state()

                    # Get operation heights from the path object
                    pierce_height = self._get_operation_height(item, "StartDepth", 0)
                    cut_height = self._get_operation_height(item, "FinalDepth", 0)
                    clearance_height = self._get_clearance_height(item, pierce_height + 10)

                    new_commands = []
                    first_entry_done = False

                    for cmd in item.Path.Commands:
                        # Check if this is a Z move to cut height (first entry)
                        if (
                            not first_entry_done
                            and "Z" in cmd.Parameters
                            and self._last_z is not None
                            and cmd.Parameters["Z"] < self._last_z
                            and cmd.Parameters["Z"] <= cut_height
                        ):

                            # Mark the entry point
                            # 1. Move to pierce height if not already there
                            if self._last_z > pierce_height:
                                new_commands.append(Path.Command("G0", {"Z": pierce_height}))

                            # 2. Move to cut height (torch on)
                            new_commands.append(Path.Command("G1", {"Z": cut_height, "F": 200}))

                            # 3. Very short delay (torch mark)
                            new_commands.append(Path.Command("G4", {"P": 0.1}))

                            # 4. Torch off and retract to clearance
                            new_commands.append(Path.Command("M5"))
                            new_commands.append(Path.Command("G0", {"Z": clearance_height}))

                            first_entry_done = True
                            # Skip the original Z move since we handled it
                            continue

                        # Skip all movement commands for mark entry only mode
                        if cmd.Name in ["G0", "G1", "G2", "G3"] and "Z" in cmd.Parameters:
                            # Only allow Z+ movements (retractions)
                            if self._last_z is not None and cmd.Parameters["Z"] > self._last_z:
                                new_commands.append(cmd)
                                self._last_z = cmd.Parameters["Z"]
                        elif cmd.Name in ["M3", "M4", "M5"]:
                            # Skip torch commands in mark entry mode
                            continue
                        else:
                            # Keep non-movement commands
                            new_commands.append(cmd)

                        # Update last Z position
                        if "Z" in cmd.Parameters:
                            self._last_z = cmd.Parameters["Z"]

                    # Replace Path with modified command list
                    item.Path = Path.Path(new_commands)

    def _get_clearance_height(self, item, default):
        """Get clearance height from operation or use default."""
        try:
            # Try to get clearance height from operation
            if hasattr(item, "ClearanceHeight"):
                value = getattr(item, "ClearanceHeight")
                if value is not None:
                    return float(value)
            elif hasattr(item, "Base") and hasattr(item.Base, "ClearanceHeight"):
                value = getattr(item.Base, "ClearanceHeight")
                if value is not None:
                    return float(value)
        except (AttributeError, TypeError, ValueError) as e:
            Path.Log.debug(f"GenericPlasma: Could not get ClearanceHeight: {e}")
        return default

    def _force_rapid_feeds(self, postables):
        """Replace all feed rates with rapid speeds for dry runs."""
        if not self._get_property_value("force_rapid_feeds", False):
            return

        for section_name, sublist in postables:
            for item in sublist:
                if hasattr(item, "Path") and item.Path:
                    new_commands = []
                    for cmd in item.Path.Commands:
                        new_cmd = cmd
                        # Remove F parameter from all movement commands
                        if cmd.Name in ["G0", "G1", "G2", "G3"] and "F" in cmd.Parameters:
                            # Create new command without F parameter
                            new_params = dict(cmd.Parameters)
                            del new_params["F"]
                            new_cmd = Path.Command(cmd.Name, new_params)
                        new_commands.append(new_cmd)
                    # Replace Path with modified command list
                    item.Path = Path.Path(new_commands)

    def pre_processing_dialog(self):
        """
        Show plasma cutting mode dialog to ask user about mark-only operation.

        Returns:
            bool: True to continue with post-processing, False to cancel
        """
        try:
            from PySide import QtCore, QtGui, QtWidgets

            app = QtWidgets.QApplication.instance()
            if app is None:
                return True

            # Get current mark_entry_only setting from machine config
            mark_only = False
            if self._machine and hasattr(self._machine, "postprocessor_properties"):
                props = self._machine.postprocessor_properties
                mark_only = props.get("mark_entry_only", False)

            # Create dialog
            dialog = QtWidgets.QDialog()
            dialog.setWindowTitle("Plasma Cutting Mode")
            dialog.resize(400, 200)
            layout = QtWidgets.QVBoxLayout(dialog)

            # Add description label
            label = QtWidgets.QLabel(
                "Select plasma cutting mode:\n\n"
                "• Normal: Full cutting with torch control\n"
                "• Mark Only: Only mark first entry points (for drilling prep)"
            )
            label.setWordWrap(True)
            layout.addWidget(label)

            # Add radio buttons for mode selection
            button_group = QtWidgets.QButtonGroup(dialog)

            normal_radio = QtWidgets.QRadioButton("Normal Cutting")
            normal_radio.setChecked(not mark_only)
            button_group.addButton(normal_radio, 0)
            layout.addWidget(normal_radio)

            mark_radio = QtWidgets.QRadioButton("Mark Entry Points Only")
            mark_radio.setChecked(mark_only)
            button_group.addButton(mark_radio, 1)
            layout.addWidget(mark_radio)

            # Add info text about mark mode
            info_label = QtWidgets.QLabel(
                "Mark mode will:\n"
                "• Mark first entry point with torch\n"
                "• Skip all cutting movements\n"
                "• Useful for preparing holes for drilling"
            )
            info_label.setWordWrap(True)
            info_label.setStyleSheet("color: #666; font-size: 11px;")
            layout.addWidget(info_label)

            # Add OK/Cancel buttons
            button_box = QtWidgets.QDialogButtonBox(
                QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel
            )
            button_box.accepted.connect(dialog.accept)
            button_box.rejected.connect(dialog.reject)
            layout.addWidget(button_box)

            # Show dialog and get result
            result = dialog.exec_()

            if result == QtWidgets.QDialog.Accepted:
                # Update machine config with user's choice
                mark_only_selected = mark_radio.isChecked()
                if self._machine and hasattr(self._machine, "postprocessor_properties"):
                    props = self._machine.postprocessor_properties
                    props["mark_entry_only"] = mark_only_selected
                    mode_text = "Mark Only" if mark_only_selected else "Normal Cutting"
                    Path.Log.info(f"Plasma cutting mode set to: {mode_text}")
                return True
            else:
                Path.Log.info("Plasma cutting mode dialog cancelled")
                return False

        except ImportError:
            Path.Log.debug("GUI not available - using machine config for mark_entry_only")
            return True
        except Exception as e:
            Path.Log.error(f"Error showing plasma cutting dialog: {str(e)}")
            return True

    def export2(self):
        """Override export2 to inject plasma-specific commands before parent processing.

        This handles torch control, pierce delays, cooling delays, and rapid feeds
        before the parent's export2() processes the commands.
        """
        Path.Log.debug("GenericPlasma.export2() starting plasma post-processing")

        # Get the postables list from parent (before processing)
        postables = self._buildPostList()
        Path.Log.debug(f"GenericPlasma: Processing {len(postables)} sections")

        # Apply plasma-specific transformations
        self._inject_mark_entry_only(postables)
        self._inject_torch_control(postables)
        self._inject_pierce_delay(postables)
        self._inject_cooling_delay(postables)
        self._force_rapid_feeds(postables)

        Path.Log.debug("GenericPlasma: Plasma transformations applied, calling parent export2()")
        # Call parent export2 with modified postables
        return super().export2()

    @property
    def tooltip(self):
        tooltip: str = """
        This is a postprocessor file for the CAM workbench.
        It is used to take a pseudo-gcode fragment from a CAM object
        and output 'real' GCode suitable for a plasma cutter.

        Features:
        - Torch Z-axis control (M3/M5 based on Z movement)
        - Pierce delay after torch ignition
        - Cooling delay after torch extinguishment
        - Mark entry points only mode (via dialog)
        - Force rapid feeds for dry runs

        The postprocessor will show a dialog to select between
        normal cutting and mark-only modes.
        """
        return tooltip


# Class aliases for PostProcessorFactory
# The factory looks for a class with title-cased postname (e.g., "Generic_Plasma")
Generic_Plasma = GenericPlasma
Genericplasma = GenericPlasma  # Fallback for different title() behavior
