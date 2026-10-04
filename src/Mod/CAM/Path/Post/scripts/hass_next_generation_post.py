# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Fae Corrigan <propsmonsterproductions@gmail.com>
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

out_tooltip = """
Haas Next Generation post processor, "machine" based.

Has been tested on:


Targets but has not been tested on:
- CM1, DM1, DM2, DT-1, DT-2, EC-1600, EC-1600ZT, EC-series
- GM-2, GR-series, Mini Mill EDU, Mini Mill, TM-series
- UMC-series, VC-400, VF-series, VS-3

The base PostProcessor already writes the G-code these controls read;
this post adds what is Haas's own:

- the ``%`` tape marks and an ``O`` program number;
- rigid tapping: ``S<rpm>`` immediately before a ``G84``/``G74`` block,
  whose F is pitch times spindle speed;
- canned cycles kept native, every block carrying its full parameter set;
- dwell in milliseconds (``G4 P1500``, ``G82 P500``), the Haas Next Gen default;
- an optional parametric tool-length offset using the ``#4120`` system
  variable, for shops that program that way;
- an optional empty-spindle park (``M6 T0``) before the postamble;
- upper-case output for older controls.
- Chip Conveyor Control at start and end of file
- G187 Smoothing
- Optionally cycle or measure all tools at start of file
- Probe arm (M104/M105)
- Spindle Speed Variation (M138/M139)
- Safe Starts for all operations that rewrite the preamble,
    WCS and units to the top of each operation
    Note: this should probably be promoted to processor.py
- Can add Optional stops between operations or tool changes
    Note: this should probably be promoted to processor.py
- VFD coolant pressure

Rotation strategies:
- TWP: the G268 / G269 feature coordinate system. The plane is declared
  with ``G268 X Y Z I J K Q123`` (spatial angles about the fixed X, Y, Z axes)
  and the spindle is turned normal to it with ``G253``. G43 must already be
  active when G268 is called, which the tool change guarantees.
- DWO: the rotary move and the rotated path, with dynamic work offsets
  (G254) switched on before the rotary move and off (``G255``) before the
  next one and at the end of the section.
- Flat Before Tool Change sends the rotaries home before any tool change
- A program that ends with rotaries away from zero sends them home
    (``G0 G53 B0 C0``) after the plane is cancelled, behind the machine's
    Pre-Rotary Move block so the tool is clear first.
- Tool Center Point Control (``G234``) for simultaneous contouring
    cannot be active with DWO(``G254``) and this option cancels it
    ``G268`` is called before ``G234``
- Clamp codes for each axis. Clamps are disabled for simultaneous operations
- Option to set the rotary table flat before running a tool change
- Option to home rotary axes at the end of jobs

If your machine does not support spaces between commands and parameters, edit Command Space in the machine definition to remove the space character.


These Features are not yet supported:
- use Radius for arcs instead of IJK
- Non-peck tapping for  Software version < 100.23.000.1201
- Faster tool changes that skip spindle off, coolant off and z retract

These features are not supported, but can be accomplished through custom blocks for now
- Flood Coolant Through Tool M88 / M89
- Air Coolant M83/ M84
- Air Through Tool Coolant M73 / M74
- Flood Coolant and Coolant Through Tool M88/M89 and M8/M9
- home machine at center x when program complete
- Tool break checks

In Progress:
- G95 tapping cycles that use IPR/MPR instead of IPM/MPM for tapping
- Choose between degrees per min and inverse time for rotary moves


"""

import re
from typing import Any

import FreeCAD
import Path

import Constants
import Path.Base.Util as PathUtil
from Path.Post.Processor import (
    PostProcessor,
    SCOPE_MACHINE,
    SCOPE_JOB,
    _HeaderBuilder,
    _sanitize_comment,
)
from Path.Post.CAMErrors import CAMValueError
from Path.Post.UtilsParse import format_command_line
import Path.Dressup.Utils as PathDressup
import Path.Op.Custom
import Path.Op.Drilling

try:
    # Tilted work planes: the post names its plane command once the base
    # post-processor knows what one is.
    from Path.Post.TiltedWorkPlane import PlaneCommand
except ImportError:
    PlaneCommand = None


translate = FreeCAD.Qt.translate

DEBUG = False
if DEBUG:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

Values = dict[str, Any]

POST_TYPE = "machine"

TAPE_MARK = "%"
RIGID_TAP_MODE = ""  # Haas machines need a S parameter before rigid tap operations but no M29

# Beyond the base list: the plane select a preamble carries, the extended work
# offsets a work plane may name, and the program ends a Custom op may write.
EXTRA_SUPPORTED = (
    ["M0", "M1", "M31", "M33", "G65", "M104", "M105", "M138", "M139", "G17", "G54.1"]
    + Constants.MCODE_END
    + Constants.MCODE_END_RESET
)


HAAS_SMOOTHING_P_WORDS = {"Rough": 1, "Medium": 2, "Finish": 3}
# VFD coolant pressure P words, as the property's choices name them.
HAAS_COOLANT_PRESSURES = {"Low": 0, "Normal": 1, "High": 2}
DWO_ON = "G254"
DWO_OFF = "G255"
# Brake codes by rotary axis, in alphabetical axis order: A (or the first rotary) is the 4th axis, the next the 5th. (release, engage)
CLAMP_CODES = (("M11", "M10"), ("M13", "M12"))
TCP_ON = "G234"  # takes the tool's H word; G49 cancels it
TCP_OFF = "G49"
# setup for pre-job probing
HAAS_TOOL_TYPE = {  # TODO missing info for slitting saw, shell mill / boring bar (3)
    "drill": 1,
    "reamer": 1,
    "tap": 2,
    "endmill": 4,
    "bullnose": 4,
    "dovetail": 4,
    "radius": 4,
    "taperedballnose": 4,
    "chamfer": 5,
    "vbit": 5,
    "countersink": 5,
    "counterbore": 5,
    "threadmill": 5,
    "ballend": 6,
    "probe": 7,
}


def haas_probing_type(
    haas_type, use9023=True
):  # TODO should this be a hard error or warning and fail or swap to unsupported tool without probe?
    if haas_type not in HAAS_TOOL_TYPE:
        raise ValueError(f"Invalid Haas tool type: {haas_type}")
    int_type = HAAS_TOOL_TYPE[haas_type]
    if int_type in (3, 4):
        return 23 if use9023 else 1  # rotate
    if int_type in (1, 2, 5, 6, 7):
        return 12 if use9023 else 2  # non-rotate
    if int_type == 0:
        return 13 if use9023 else 3  # rotate length and dia
    raise ValueError(f"Invalid Haas tool type: {haas_type}")


class HassHeaderBuilder(_HeaderBuilder):
    def __init__(
        self, measure_tools=False, cycle_tools=None, tool_arm_drive=None, use_chip_conveyor=False
    ):
        super().__init__()
        self._measure_tools = measure_tools
        self._cycle_tools = cycle_tools
        self._tool_arm_drive = tool_arm_drive
        self._use_chip_conveyor = use_chip_conveyor

    def add_tool(
        self, tool_number, tool_name, tool_diameter=None, tool_body_length=None, tool_type=None
    ):
        self._tools.append((tool_number, tool_name, tool_diameter, tool_body_length, tool_type))

    @property
    def Path(self) -> Path.Path:
        """Return a Path.Path containing Path.Commands as G-code comments for the header."""
        commands = []

        # Add exporter info
        if self._exporter:
            commands.append(Path.Command(f"(Exported by {self._exporter})"))

        # Add machine info
        if self._machine:
            commands.append(Path.Command(f"(Machine: {self._machine})"))

        # Add post processor info
        if self._post_processor:
            commands.append(Path.Command(f"(Post Processor: {self._post_processor})"))

        # Add CAM file info
        if self._cam_file:
            commands.append(Path.Command(f"(Cam File: {self._cam_file})"))

        # Add project file info
        if self._project_file:
            commands.append(Path.Command(f"(Project File: {self._project_file})"))

        # Add output units info
        if self._output_units:
            commands.append(Path.Command(f"(Output Units: {self._output_units})"))

        # Add document name
        if c := self._document_name:
            commands.append(Path.Command(f"(Document: {_sanitize_comment(c)})"))

        # Add description
        if c := self._description:
            commands.append(Path.Command(f"(Description: {_sanitize_comment(c)})"))

        # Add author info
        if c := self._author:
            commands.append(Path.Command(f"(Author: {_sanitize_comment(c)})"))

        # Add output time
        if self._output_time:
            commands.append(Path.Command(f"(Output Time: {self._output_time})"))

        has_tools = False
        tool_cycle_commands = []
        # Add tools
        if self._cycle_tools or self._measure_tools:
            tool_cycle_commands.append(Path.Command("M0", {}, {Constants.ANNOT_BLOCK_DELETE: True}))
            tool_cycle_commands.append(
                Path.Command("(With BLOCK DELETE turned off each tool will cycle through)")
            )
            tool_cycle_commands.append(
                Path.Command(
                    "(the spindle to verify that the correct tool is in the tool magazine)"
                )
            )
            if self._measure_tools:
                tool_cycle_commands.append(Path.Command("(and to automatically measure it)"))
            tool_cycle_commands.append(
                Path.Command(
                    "(Once the tools are verified turn BLOCK DELETE on to skip verification)"
                )
            )
            if self._measure_tools and self._tool_arm_drive:
                tool_cycle_commands.append(Path.Command("(Extend tool setting probe arm)"))
                tool_cycle_commands.append(
                    Path.Command("M104", {}, {Constants.ANNOT_BLOCK_DELETE: True})
                )

        for tool_number, tool_name, tool_diameter, tool_body_length, tool_type in self._tools:
            tool_cycle_commands.append(
                Path.Command(f"(T{tool_number}={_sanitize_comment(tool_name)})")
            )

            if tool_type == "probe":
                continue  # no measuring or cycling for probe tools
            has_tools = True
            if self._measure_tools:

                probing_type = haas_probing_type(tool_type, use9023=True)
                outstring = f"G65 P9023 A{probing_type} T{tool_number}"
                if tool_body_length is not None and tool_diameter is not None:
                    outstring += f" H{tool_body_length:g} D{tool_diameter:g}"
                tool_cycle_commands.append(
                    Path.Command(outstring, {}, {Constants.ANNOT_BLOCK_DELETE: True})
                )
            elif self._cycle_tools:
                tool_cycle_commands.append(
                    Path.Command(f"M6 T{tool_number}", {}, {Constants.ANNOT_BLOCK_DELETE: True})
                )
                tool_cycle_commands.append(
                    Path.Command("M0", {}, {Constants.ANNOT_BLOCK_DELETE: True})
                )
        if self._measure_tools and self._tool_arm_drive:
            tool_cycle_commands.append(Path.Command("(Retract tool setting probe arm)"))
            tool_cycle_commands.append(
                Path.Command("M105", {}, {Constants.ANNOT_BLOCK_DELETE: True})
            )
        if has_tools:
            commands += tool_cycle_commands

        # Add fixtures (if needed in header)
        for fixture in self._fixtures:
            commands.append(Path.Command(f"(Fixture: {_sanitize_comment(fixture)})"))

        # Add notes
        for note in self._notes:
            commands.append(Path.Command(f"(Note: {_sanitize_comment(note)})"))

        # Add Chip Conveyor
        if self._use_chip_conveyor:
            commands.append(Path.Command("(Chip Conveyor On)"))
            commands.append(Path.Command("M31"))

        return Path.Path(commands)


class HaasNextGeneration(PostProcessor):
    """Post processor for Haas Next Gen mill controls and Haas Next Gen-compatible G-code."""

    ROTATION_STRATEGIES = ("dwo", "twp")

    if PlaneCommand is not None:
        PLANE_COMMAND = PlaneCommand.G268

    # HaasNextGeneration canned cycles whose P word is a dwell.
    DwellCycles = ("G82", "G88", "G89", "G84", "G74", "G76")

    # ------------------------------------------------------------------
    # Property schema
    # ------------------------------------------------------------------

    @classmethod
    def get_common_property_schema(cls):
        """HaasNextGeneration defaults for the common properties."""
        common_props = super().get_common_property_schema()

        for prop in common_props:
            name = prop["name"]
            if name == "file_extension":
                prop["default"] = "nc"
            elif name == "preamble":
                prop["default"] = "G90 G94 G17"
            elif name == "postamble":
                prop["default"] = "M5\nM9\nM30"
            elif name == "safetyblock":  # TODO this is being used as safe retracts mode
                # The preamble establishes the safe state; a separate block would repeat it.
                prop["default"] = "G53 G0 Z0\n"
            elif name == "pre_tool_change":
                prop["default"] = "M05\nG28 G91 Z0\nG90"
            elif name == "drill_cycles_to_translate":
                # Every cycle CAM emits is native on a HaasNextGeneration.
                prop["default"] = ""
            elif name == "supports_tool_radius_compensation":
                prop["default"] = True
            elif name == "supported_commands":
                prop["default"] = "\n".join(prop["default"].split("\n") + EXTRA_SUPPORTED)
            elif name == "spindle_decimals":
                # S takes an integer.
                prop["default"] = 0
            elif name == "pre_rotary_move":  # TODO check through this description
                prop["help"] = translate(
                    "CAM",
                    "G-code inserted before the rotary axes move, and before a tilted work "
                    "plane is aligned. Put the moves that bring the tool clear "
                    "of the part here, in machine coordinates: G53 G0 Z0 lifts the tool to "
                    "machine Z zero; G91 G28 Z0 followed by G90 references it."
                    "M05 Turns off the spindle and is usually desired. Left empty, "
                    "nothing clears the part before the table turns, and the sanity check "
                    "says so.",
                )
            elif name == "post_rotary_move":  # TODO check through this description
                prop["help"] = translate(
                    "CAM",
                    "G-code inserted after the rotary axes have moved. Under the DWO rotation "
                    "strategy the post writes G254 (dynamic work offset) before the rotary move "
                    "and G255 to cancel it, so this block is usually empty. Under TWP the G268 "
                    "plane and G253 handle the rotation and this block is usually empty too.",
                )

        return common_props

    @classmethod
    def get_property_schema(cls):
        """HaasNextGeneration-specific properties."""
        return [
            {
                "name": "program_number",
                "scope": SCOPE_JOB,
                "type": "int",
                "label": translate("CAM", "Program Number"),
                "default": 0,
                "min": 0,
                "max": 99999999,
                "help": translate(
                    "CAM",
                    "The O number written after the leading % mark, zero-padded to four "
                    "digits. 0 writes no O line. The control files the program under this "
                    "number when it is loaded.",
                ),
            },
            {
                "name": "wrap_in_percent",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Tape Marks (%)"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write % as the first and last line, as tape-format DNC expects.",
                ),
            },
            {
                "name": "uppercase_output",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Upper-Case Output"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write the whole program in upper case, comments included. Older controls "
                    "read nothing else.",
                ),
            },
            {
                "name": "dwell_in_milliseconds",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Dwell in Milliseconds"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write the P word of G4 and of the dwelling canned cycles as an integer "
                    "number of milliseconds, \n Haas parameter DWL = 0.\n"
                    "Off: P is written in seconds with decimals, for a control set to read it that way",
                ),
            },
            {
                "name": "useTCP",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Tool Center Point Control"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Enable if the machine has the Tool Center Point Control option (G234).\n "
                    "An operation that moves the rotary axes along the path gets G234 H<tool> "
                    "before it and G49, then the tool length offset again, after it./n "
                    "G234 cancels G43, cannot be active with G254, and must come after G268",
                ),
            },
            {
                "name": "rigid_tapping",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Rigid Tapping"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write S<rpm> immediately before every G84 or G74 block, so the "
                    "control synchronises the spindle with the feed.\n"
                    "Off, the cycle runs with a floating tap holder.\n"
                    "The feed is pitch times spindle speed in either case",
                ),
            },
            {
                "name": "tool_length_offset_macro",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Parametric Tool Length Offset"),
                "default": False,
                "help": translate(
                    "CAM",
                    "After a tool change write G91 G0 G43 G54 Z-[#[2000+#4120]] H#4120 and "
                    "G90 instead of G43 H<n>. \nFor shops whose offset table keeps the negated "
                    "length of tool n in #[2000+n]; #4120 is the active T code",
                ),
            },
            {
                "name": "end_spindle_empty",
                "scope": SCOPE_JOB,
                "type": "bool",
                "label": translate("CAM", "Park Empty Spindle at End"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Before the postamble stop the spindle, retract, and write M6 T0 so the "
                    "last tool goes back to the magazine\n"
                    "Not every control accepts T0; leave this off unless yours does",
                ),
            },
            {
                "name": "end_spindle_empty_retract",
                "scope": SCOPE_MACHINE,
                "type": "text",
                "label": translate("CAM", "Empty-Spindle Retract"),
                "default": "(Retract Empty Spindle)\nG28 G91 Z0\nG90",
                "help": translate(
                    "CAM",
                    "Retract the spindle before emptying the tool at the end of the file.\n"
                    "The retract written between M05 and M6 T0 when the empty-spindle park "
                    "is on",
                ),
            },
            {
                "name": "useDPMFeeds",  # TODO
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Rotary Moves Use DPM"),
                "default": False,
                "help": translate(
                    "CAM",
                    "True: Rotary  moves use degrees per min\n"
                    "False: Rotary moves use inverse time mode",
                ),
            },
            {
                "name": "gotChipConveyor",
                "scope": SCOPE_JOB,
                "type": "bool",
                "label": translate("CAM", "Chip Transport"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Enable to turn on chip transport at the start of all programs",
                ),
            },
            {
                "name": "optionalStop",  # TODO Promote to parent
                "scope": SCOPE_JOB,
                "type": "choice",
                "label": translate("CAM", "Optional Stops"),
                "choices": ["Off", "Operation", "Tool Change"],
                "default": "Off",
                "help": translate(
                    "CAM",
                    "Enable to add M1 optional stops between operations or tool changes",
                ),
            },
            {
                "name": "useSmoothing",
                "type": "choice",
                "scope": SCOPE_JOB,
                "label": translate("CAM", "Use G187 Smoothing"),
                "choices": ["Off", "Automatic", "Rough", "Medium", "Finish"],
                "default": "Off",
                "help": translate(
                    "CAM",
                    "G187 can improve cycle times at the cost of maximum accuracy\n"
                    "for machines that support it",
                ),
            },
            {
                "name": "SmoothingRoughingTolerance",
                "scope": SCOPE_MACHINE,
                "type": "float",
                "label": translate("CAM", "Smoothing Roughing Stock To Leave"),
                "default": 0.5,
                "help": translate(
                    "CAM",
                    "When automatic G187 is used, operations with stock/tolerance above that threshold will use roughing level",
                ),
            },
            {
                "name": "SmoothingSemiFinishingTolerance",
                "scope": SCOPE_MACHINE,
                "type": "float",
                "label": translate("CAM", "Smoothing Semi-Finish Stock To Leave"),
                "default": 0.1,
                "help": translate(
                    "CAM",
                    "When automatic G187 is used, operations with stock/tolerance above that threshold will use roughing level",
                ),
            },
            {
                "name": "SmoothingFinishingTolerance",
                "scope": SCOPE_MACHINE,
                "type": "float",
                "label": translate("CAM", "Smoothing Finishing Stock To Leave"),
                "default": 0.05,
                "help": translate(
                    "CAM",
                    "When automatic G187 is used, operations with stock/tolerance above that threshold will use roughing level",
                ),
            },
            {
                "name": "optionallyCycleToolsAtStart",
                "scope": SCOPE_JOB,
                "type": "bool",
                "label": translate("CAM", "Cycle Tools At Start"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Cycle through all tools at the beginning of the program when block delete is off",
                ),
            },
            {
                "name": "measureTools",
                "scope": SCOPE_JOB,
                "type": "bool",
                "label": translate("CAM", "Measure Tools At Start"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Measure TLO on each tool at the beginning of the program when block delete is off",
                ),
            },
            {
                "name": "toolArmDrive",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Tool Setting Probe Arm"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Outputs M104/M105 to extend the tool setting probe arm",
                ),
            },
            {
                "name": "useSSV",
                "scope": SCOPE_JOB,
                "type": "bool",
                "label": translate("CAM", "Spindle Speed Variation"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Enable to Output M138/139 for spindle speed variation",
                ),
            },
            {
                "name": "safeStartAllOperations",
                "scope": SCOPE_JOB,
                "type": "bool",
                "label": translate("CAM", "Safe Start All Operations"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Write all commands to start the program at the top of each operation",
                ),
            },
            {
                "name": "useG95forTapping",  # TODO
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Use G95 for tapping"),
                "default": False,
                "help": translate(
                    "CAM",
                    "use IPR/MPR instead of IPM/MPM for tapping",
                ),
            },
            {
                "name": "coolantPressure",
                "type": "choice",
                "scope": SCOPE_JOB,
                "label": translate("CAM", "VFD Coolant Pressure"),
                "choices": ["None", "Low", "Normal", "High"],  # values of "", P0, P1, P2
                "default": "None",
                "help": translate(
                    "CAM",
                    "Select coolant pressure for machines with a VFD. Leave default for those without",
                ),
            },
            {
                "name": "useClampCodes",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Use Clamp Codes"),
                "default": True,
                "help": translate(
                    "CAM",
                    "Write the rotary axis brake codes.\n"
                    "M11/M13 release the 4th/5th axis before a rotary move and M10/M12 engage them after it.\n"
                    "This ensures the axes are clamped while cutting.\n"
                    "Axes are numbered in alphabetical order: A, then B, then C.\n"
                    "A simultaneous operation is always unclamped, and clamped again after it",
                ),
            },
            {
                "name": "flat_before_tool_change",
                "scope": SCOPE_JOB,
                "type": "bool",
                "label": translate("CAM", "Lay Table Flat Before Tool Change"),
                "default": False,
                "help": translate(
                    "CAM",
                    "Before each tool change, if the rotary axes are away from zero, write "
                    "G0 G53 with each rotary axis at 0,\n"
                    "for machines whose tool changer or door needs the table level.\n"
                    "The work plane is cancelled first, and the "
                    "Pre-Rotary Move block is written before the move so the tool is clear.\n"
                    "The next operation positions the rotaries again",
                ),
            },
            {
                "name": "return_rotaries_home",
                "scope": SCOPE_MACHINE,
                "type": "bool",
                "label": translate("CAM", "Return Rotaries Home"),
                "default": True,
                "help": translate(
                    "CAM",
                    "When the program ends with the rotary axes away from zero, write "
                    "G0 G53 with each rotary axis at 0 after the work plane is cancelled.\n"
                    "The Pre-Rotary Move block is written first, so put the move that "
                    "lifts the tool clear of the part there",
                ),
            },
        ]

    # ------------------------------------------------------------------
    # Setup
    # ------------------------------------------------------------------

    def __init__(
        self,
        job,
        tooltip=translate("CAM", "Haas Next Gen Post Processor"),
        tooltipargs=[],
        units="Metric",
    ) -> None:
        super().__init__(job=job, tooltip=tooltip, tooltipargs=tooltipargs, units=units)
        Path.Log.debug("HaasNextGeneration post processor initialized.")
        self._ssv_active = False

    def init_values(self, values: Values) -> None:
        super().init_values(values)
        values["MACHINE_NAME"] = "Haas Next Generation"
        values["POSTPROCESSOR_FILE_NAME"] = __name__

    def _merge_machine_config(self):
        super()._merge_machine_config()
        # HaasNextGeneration comments are parenthesised, whatever the machine file says.
        self.values["COMMENT_SYMBOL"] = "("

    # ------------------------------------------------------------------
    # Postable expansion
    # ------------------------------------------------------------------

    @staticmethod
    def _as_is(text: str) -> Path.Command:
        return Path.Command("", {}, {Constants.ANNOT_AS_IS: text})

    def _tool_dims(self, tool_number):  # TODO make sure this works with imperial
        """(diameter, body_length, tool_type) for a tool number."""
        for tc in getattr(getattr(self._job, "Tools", None), "Group", []):
            if tc.ToolNumber == tool_number:
                bit = tc.Tool
                shape = str(getattr(bit, "ShapeType", "") or getattr(bit, "ShapeName", "")).lower()
                return float(bit.Diameter.Value), float(bit.Length.Value), shape
        return None, None, False

    def _build_header(self, postables):
        base = super()._build_header(postables)

        header = HassHeaderBuilder(
            measure_tools=self.values.get("MEASURETOOLS", False),
            cycle_tools=self.values.get("OPTIONALLYCYCLETOOLSATSTART", False),
            tool_arm_drive=self.values.get("TOOLARMDRIVE", False),
            use_chip_conveyor=self.values.get("GOTCHIPCONVEYOR", False),
        )
        header.__dict__.update(base.__dict__)
        header._tools = []  # drop the base's 2-tuples; rebuild with extra info

        if (
            (self.values["OUTPUT_HEADER"] and self.values["LIST_TOOLS_IN_HEADER"])
            or self.values.get("OPTIONALLYCYCLETOOLSATSTART", False)
            or self.values.get("MEASURETOOLS", False)
        ):
            seen = set()
            for _, sublist in postables:
                for item in sublist:
                    if item.item_type == "tool_controller":
                        number = item.data["tool_number"]
                        if number in seen:
                            continue
                        seen.add(number)
                        diameter, body_length, tool_type = self._tool_dims(number)
                        header.add_tool(number, item.label, diameter, body_length, tool_type)

        return header

    def _plane_postables(self, key, placement=None):
        """Mark the G268 declaration, so the expansion below knows the table is tilted."""
        items = super()._plane_postables(key, placement)
        if key == "TWP_DECLARE":
            for item in items:
                item.data["twp_declare"] = True
        return items

    def _rotary_home(self, axes):  # this is likely redundant and needs thoroghly tested
        """Clear the part, then move the rotaries to zero in machine coordinates.
        The clearance is the machine's own Pre-Rotary Move block, the same one
        that protects every other rotary move; the Post-Rotary block follows.
        """
        space = self.values.get("COMMAND_SPACE", " ")
        words = space.join(f"{axis.upper()}0" for axis in axes)
        return (
            self._rotary_block_postables("PRE_ROTARY_MOVE")
            + self._clamp_rotaries(engage=False)
            + [self._make_postable("Post: rotaries home", f"G0{space}G53{space}{words}")]
            + self._clamp_rotaries(engage=True)
            + self._rotary_block_postables("POST_ROTARY_MOVE")
        )

    def _clamp_rotaries(self, engage):
        """The brake codes for the rotary axes: M10/M12 engage, M11/M13 release.

        One M code to a block on a Haas, so one line each. Empty when Use Clamp
        Codes is off or the machine has no rotary axes.
        """
        if not self.values.get("USECLAMPCODES", True):
            return []
        import Path.Base.Generator.rotation as rotation

        axes = sorted(axis.name.upper() for axis in rotation.build_kinematic_chain(self._machine))
        lines = [pair[1 if engage else 0] for pair in CLAMP_CODES[: len(axes)]]
        if not lines:
            return []
        return [self._make_postable("Post: clamp" if engage else "Post: unclamp", lines)]

    def _pose_change_postables(
        self, strategy, placement, positions, declared, rotaries_move, fixture=None
    ):
        """Adds rotary brakes to the base post
        Brakse released straight after the Pre-Rotary Move block, which has lifted the
        tool; engaged again before the Post-Rotary block, so the axes are
        clamped for the cut. Nothing is written when the rotaries do not move.
        """
        items = super()._pose_change_postables(
            strategy, placement, positions, declared, rotaries_move, fixture
        )
        if not rotaries_move or not self.values.get("USECLAMPCODES", True):
            return items
        start = 1 if items and items[0].label == "Post: pre-rotary" else 0
        end = len(items) - 1 if items and items[-1].label == "Post: post-rotary" else len(items)
        return (
            items[:start]
            + self._clamp_rotaries(engage=False)
            + items[start:end]
            + self._clamp_rotaries(engage=True)
            + items[end:]
        )

    @staticmethod
    def _moves_rotaries(command):
        return any(word in command.Parameters for word in ("A", "B", "C"))

    def _tcp_postables(self, item, tool_number):
        """Add G234 H<n> before a simultaneous operation; includes clamp (on, off) control."""
        if tool_number is None:
            raise CAMValueError(
                translate(
                    "CAM",
                    "{op} moves the rotary axes along its path, which needs Tool Center Point "
                    "Control, but no tool controller precedes it to name the H word",
                ).format(op=item.label),
                job=self._job,
                operation=item.source,
                pp=self.values["MACHINE_NAME"],
            )
        space = self.values.get("COMMAND_SPACE", " ")
        on = self._clamp_rotaries(engage=False)  # a simultaneous path always runs unclamped
        on.append(self._make_postable("Post: TCP on", f"{TCP_ON}{space}H{int(tool_number)}"))
        off = [self._make_postable("Post: TCP off", TCP_OFF)]
        # G49 dropped the length offset; G234 had replaced it.
        if self.values.get("OUTPUT_TOOL_LENGTH_OFFSET", True):
            tlo = self._expand_tool_length_offset_post_command(
                item, Path.Command("M6", {"T": tool_number})
            )
            off.append(self._make_postable("Post: tool length offset", tlo))
        off.extend(self._clamp_rotaries(engage=True))
        return on, off

    def _expand_workplane_frames(self, postables):
        """Haas additions to the base work-plane expansion.

        TCP: an operation whose path moves the rotary axes is bracketed with
        G234 H<tool> and G49 and the tool length offset.
        DWO, which G234 cannot share, is cancelled first.
        Any G268 plane is already declared, which is the order Haas requires.
        The base does not know a simultaneous operation moved the rotaries,
        so keep one flat between 3+2 operations that share a pose.
        DWO: G254 is written before each rotary move, and G255 before the next
        one, at a tool or fixture change (the base forgets the pose there, so
        the next operation switches it on again) and at the end of the section.

        Clamp codes: the rotary brakes are released before each rotary move
        for the pose change, the home move, and any simultaneous operations and
        engaged after the axis is stationary.

        Flat Before Tool Change:  when the rotaries are not zero, this parameter
        moves them back to zero before each tool change

        Return Rotaries Home Parameter:
        When a section ends with the rotaries away from zero
        (a G268 plane was declared, or the last rotary move was not to zero),
        the rotaries are sent home after the plane is cancelled
        """

        from Machine.models.machine import RotationStrategy
        import Path.Base.Generator.rotation as rotation

        result = super()._expand_workplane_frames(postables)
        strategy = self._rotation_strategy()
        if strategy is None:
            return result

        dwo = strategy == RotationStrategy.DWO
        go_home = self.values.get("RETURN_ROTARIES_HOME", True)
        flat_for_tool = self.values.get("FLAT_BEFORE_TOOL_CHANGE", False)
        use_tcp = self.values.get("USETCP", False)
        axes = [axis.name for axis in rotation.build_kinematic_chain(self._machine)]

        out = []
        for section_name, items in result:
            rebuilt = []
            dwo_on = False
            away = False  # the rotaries are not at zero
            tool_number = None
            for item in items:
                if item.item_type == "tool_controller":
                    tool_number = item.data.get("tool_number")
                if flat_for_tool and away and axes and item.item_type == "tool_controller":
                    # The plane is already cancelled; home with DWO still on.
                    rebuilt.extend(self._rotary_home(axes))
                    away = False
                if dwo and dwo_on and item.item_type in ("tool_controller", "fixture"):
                    rebuilt.append(self._make_postable("Post: DWO off", DWO_OFF))
                    dwo_on = False
                if item.data.get("twp_declare"):
                    away = True
                if item.item_type == "rotation" and item.data.get("pose_change"):
                    params = item.path.Commands[0].Parameters
                    away = any(abs(float(v)) > 1e-6 for v in params.values())
                    if dwo:
                        if dwo_on:
                            rebuilt.append(self._make_postable("Post: DWO off", DWO_OFF))
                        rebuilt.append(self._make_postable("Post: DWO on", DWO_ON))
                        dwo_on = True
                if (
                    use_tcp
                    and item.item_type == "operation"
                    and item.path
                    and any(self._moves_rotaries(c) for c in item.path.Commands)
                ):
                    if dwo_on:
                        rebuilt.append(self._make_postable("Post: DWO off", DWO_OFF))
                        dwo_on = False
                    on, off = self._tcp_postables(item, tool_number)
                    last = [c for c in item.path.Commands if self._moves_rotaries(c)][-1]
                    away = any(
                        abs(float(last.Parameters.get(w, 0))) > 1e-6 for w in ("A", "B", "C")
                    )
                    rebuilt.extend(on)
                    rebuilt.append(item)
                    rebuilt.extend(off)
                    continue
                rebuilt.append(item)
            # Home while DWO is still on, so the offset follows the table down.
            if go_home and away and axes:
                rebuilt.extend(self._rotary_home(axes))
            if dwo_on:
                rebuilt.append(self._make_postable("Post: DWO off", DWO_OFF))
            out.append((section_name, rebuilt))
        return out

    def _expand_postprocessor_commands(self, postables):
        """Mark up the tapping and drilling cycles before anything else sees them.

        A rigid tap gets a S<rpm> block at the top, in its own command so it is
        line-numbered like any other. The speed is the cycle's own S or the
        last spindle speed commanded before it. A run of taps ends in G80,
        which the base terminator only writes for the drilling cycles; on a
        Haas the tap is modal like any cycle and G80 is what drops rigid
        mode. Every canned cycle is made a modal barrier, so a run of holes
        keeps the cycle word and its full parameter set on every block: a
        Haas treats a coordinate-only block inside a cycle as another hole,
        but operators expect to read the cycle on each line, and a control set
        to non-modal cycles needs it.
        """
        rigid = self.values.get("RIGID_TAPPING", True)
        cycles = set(Constants.GCODE_MOVE_DRILL + Constants.GCODE_DRILL_EXTENDED)
        cancel = Constants.GCODE_CYCLE_CANCEL[0]

        for section_name, sublist in postables:
            spindle_speed = 0.0
            for item in sublist:
                if not item.path:
                    continue
                commands = []
                changed = False
                tapping = False
                for cmd in item.path.Commands:
                    if tapping and cmd.Name not in Constants.GCODE_MOVE_TAP:
                        if cmd.Name not in Constants.GCODE_CYCLE_CANCEL:
                            commands.append(Path.Command(cancel))
                            changed = True
                        tapping = False
                    if cmd.Name in Constants.MCODE_SPINDLE_ON and "S" in cmd.Parameters:
                        spindle_speed = float(cmd.Parameters["S"])
                    if cmd.Name in cycles:
                        cmd = Path.Command(
                            cmd.Name,
                            cmd.Parameters,
                            {**cmd.Annotations, Constants.ANNOT_MODAL_BARRIER: True},
                        )
                        changed = True
                    if rigid and cmd.Name in Constants.GCODE_MOVE_TAP:
                        speed = float(cmd.Parameters.get("S", 0) or 0) or spindle_speed
                        if speed <= 0:
                            raise CAMValueError(
                                translate(
                                    "CAM",
                                    "Rigid tapping needs a spindle speed: {op} taps with {cmd} "
                                    "and no S has been commanded",
                                ).format(op=item.label, cmd=cmd.Name),
                                job=self._job,
                                operation=item.source,
                                command=cmd,
                                pp=self.values["MACHINE_NAME"],
                            )
                        commands.append(
                            Path.Command(
                                RIGID_TAP_MODE,
                                {"S": speed},
                                {Constants.ANNOT_ALLOW_UNSUPPORTED: True},
                            )
                        )
                        changed = True
                        tapping = True
                    commands.append(cmd)
                if tapping:
                    commands.append(Path.Command(cancel))
                    changed = True
                if changed:
                    item.path = Path.Path(commands)

    def _expand_prefix(self, postables) -> None:
        """The tape mark and the O number come before everything, header included."""
        super()._expand_prefix(postables)

        leading = []
        if self.values.get("WRAP_IN_PERCENT", True):
            leading.append(self._as_is(TAPE_MARK))
        number = int(self.values.get("PROGRAM_NUMBER", 0) or 0)
        if number > 0:
            line = f"O{number:04d}"
            if self.values["OUTPUT_COMMENTS"] and self._job is not None:
                label = re.sub(r"[()]", "", getattr(self._job, "Label", "") or "")
                if label:
                    line += f" ({label})"
            leading.append(self._as_is(line))

        if leading:
            for _, section in postables:
                section.insert(0, self._make_postable("Post: tape start", leading))

        for _, section in postables:  # per-operation additions
            rebuilt = []
            seen_operation = False
            for item in section:
                if item.item_type == "operation":
                    if self.values.get("USESSV", False):
                        if str(
                            getattr(item.data.get("tool_controller").Tool, "ShapeType", "")
                        ).lower() not in ["probe", "drill"] and not isinstance(
                            getattr(item, "Proxy", None), Path.Op.Custom.ObjectCustom
                        ):
                            if not self._ssv_active:
                                rebuilt.append(
                                    self._make_postable("Post: SSV on", ["M138 (SSV On)"])
                                )
                                self._ssv_active = True
                        else:
                            if self._ssv_active:
                                rebuilt.append(
                                    self._make_postable("Post: SSV Off", ["M139 (SSV Off)"])
                                )
                                self._ssv_active = False

                    smoothing = self.values.get(
                        "USESMOOTHING", "Off"
                    )  # TODO this needs a fine toothed comb. It probably also needs to use the E parameter
                    if smoothing != "Off":
                        level = 3
                        if smoothing == "Rough":
                            level = 1
                        elif smoothing == "Medium":
                            level = 2
                        elif smoothing == "Finish":
                            level = 3
                        elif smoothing == "Automatic":
                            op = (
                                PathDressup.baseOp(item.source) if item.source is not None else None
                            )
                            level = 3
                            stock_to_leave = getattr(op, "StockToLeave", 0)
                            z_stock_to_leave = getattr(op, "ZStockToLeave", 0)
                            if max(stock_to_leave, z_stock_to_leave) >= self.values.get(
                                "SMOOTHINGROUGHINGTOLERANCE", 0.5
                            ):
                                level = 1
                            elif max(stock_to_leave, z_stock_to_leave) >= self.values.get(
                                "SMOOTHINGSEMIFINISHINGTOLERANCE", 0.1
                            ):
                                level = 2
                            elif max(stock_to_leave, z_stock_to_leave) > self.values.get(
                                "SMOOTHINGFINISHINGTOLERANCE", 0.05
                            ):
                                level = 3

                        if not isinstance(
                            getattr(item, "Proxy", None), Path.Op.Drilling.ObjectDrilling
                        ) and not isinstance(
                            getattr(item, "Proxy", None), Path.Op.Custom.ObjectCustom
                        ):  # TODO this needs to be false on multi-axis ops as well
                            rebuilt.append(
                                self._make_postable("Post: smoothing", [f"G187 P{level}"])
                            )
                        else:
                            rebuilt.append(self._make_postable("Post: smoothing", [f"G187"]))

                    if seen_operation:  # addions only after the first operation has completed
                        if self.values.get("OPTIONALSTOP", "Off") == "Operation":
                            rebuilt.append(
                                self._make_postable("Post: op stop", ["M1 (Optional Stop)"])
                            )
                        if self.values.get(
                            "SAFESTARTALLOPERATIONS", False
                        ):  # TODO this also WCS system
                            rebuilt.append(
                                self._make_postable("Post: safe start", "(Safe Start Operation)")
                            )
                            if (lines := self.values["PREAMBLE"]) is not None and lines != "":
                                rebuilt.append(self._make_postable("Post: preamble", lines))
                            # OUTPUT_UNITS
                            if unit_command := self._collect_unit_command():
                                rebuilt.append(self._make_postable("Post: units", unit_command))
                    seen_operation = True
                rebuilt.append(item)
            section[:] = rebuilt

    def _expand_trailing_lines(self, postables) -> None:
        """Empty-spindle park before the postamble; turn off chip conveyor; the closing tape mark after it."""

        if self.values.get("GOTCHIPCONVEYOR", False):
            block = ["(Stop Chip Conveyor)", "M33"]
            for _, section in postables:
                section.append(self._make_postable("Post: conveyor off", block))

        if self.values.get(
            "USESSV", False
        ):  # TODO this is only getting appended at the end of the file
            block = ["M139 (SSV Off)"]
            for _, section in postables:
                section.append(self._make_postable("Post: ssv off", block))

        if self.values.get("END_SPINDLE_EMPTY", False):
            retract = [
                line
                for line in (self.values.get("END_SPINDLE_EMPTY_RETRACT") or "").split("\n")
                if line.strip()
            ]
            block = ["M05"] + retract + ["(End spindle empty)"] + ["M6 T0"]
            for _, section in postables:
                section.append(self._make_postable("Post: empty spindle", block))

        super()._expand_trailing_lines(postables)

        if self.values.get("WRAP_IN_PERCENT", True):
            for _, section in postables:
                section.append(self._make_postable("Post: tape end", [self._as_is(TAPE_MARK)]))

    def _convert_coolant_command(self, command):
        # handle VFD pressure additions
        line = self._convert_move(command)
        if not line:
            return line

        pressure = self.values.get("COOLANTPRESSURE")
        if (
            command.Name in Constants.MCODE_COOLANT_ON
            and pressure
            and "P" not in command.Parameters
            and pressure != "None"
        ):

            if pressure not in HAAS_COOLANT_PRESSURES:
                raise ValueError(f"Invalid Haas Coolant Pressure: {pressure}")
            pressure_val = HAAS_COOLANT_PRESSURES[pressure]

            if float(pressure_val) == int(float(pressure_val)):
                text = str(int(float(pressure_val)))
            else:
                text = f"{float(pressure):g}"
            line += f"{self.values.get('COMMAND_SPACE', ' ')}P{text}"

        return line

    def _expand_tool_length_offset_post_command(self, item, command):
        """G43 H<n>, or the macro form that reads the offset table itself."""
        if not self.values.get("TOOL_LENGTH_OFFSET_MACRO", False):
            return super()._expand_tool_length_offset_post_command(item, command)
        return [
            self._as_is("G91 G0 G43 G54 Z-[#[2000+#4120]] H#4120"),
            self._as_is("G90"),
        ]

    # ------------------------------------------------------------------
    # Command conversion
    # ------------------------------------------------------------------

    def format_parameter(self, param_name, value, command_name=None):  # TODO
        """P is a dwell in whole milliseconds on a HaasNextGeneration, unless the control is
        set otherwise; on G54.1 it is the extended WCS offset number."""
        if param_name == "P":
            if command_name == "G54.1":
                return str(int(value))
            if command_name in Constants.GCODE_P_IS_DWELL and self.values.get(
                "DWELL_IN_MILLISECONDS", True
            ):
                return str(round(float(value) * 1000))
        return super().format_parameter(param_name, value, command_name)

    def _convert_tool_change(self, command: Path.Command) -> str:
        outstring = ""
        if self.values.get("OPTIONALSTOP", "Off") == "Tool Change":
            outstring = "M1 (Optional Stop)\n"

        outstring += super()._convert_tool_change(command)

        return outstring

    def _convert_drill_cycle(self, command: Path.Command) -> str:
        """Every cycle block carries its full parameter set, whatever the
        duplicate setting: a hole is a hole, and a control set to non-modal
        cycles drills nothing from a block missing its Z or R.

        A rigid tap's speed is on a standalone S parameter; the cycle block does not repeat it.
        """
        command = self._tapping_to_speed(command)
        if (
            command.Name in Constants.GCODE_MOVE_TAP
            and self.values.get("RIGID_TAPPING", True)
            and "S" in command.Parameters
        ):
            params = {k: v for k, v in command.Parameters.items() if k != "S"}
            command = Path.Command(command.Name, params, command.Annotations)

        doubles = self.values["OUTPUT_DOUBLES"]
        self.values["OUTPUT_DOUBLES"] = True
        try:
            return self._convert_move(command)
        finally:
            self.values["OUTPUT_DOUBLES"] = doubles

    def _convert_generic_command(self, command: Path.Command) -> str:  # TODO
        """Rigid Tapping S<rpm>: the S must be written even when it repeats the spindle's."""
        if command.Name == RIGID_TAP_MODE:
            words = []
            if (n := command.Parameters.get("N")) is not None:
                words.append(f"{self.values['LINE_NUMBER_PREFIX']}{int(n):d}")
            words.append(RIGID_TAP_MODE)
            words.append(f"S{self.format_parameter('S', command.Parameters['S'])}")
            return format_command_line(self.values, words)
        return super()._convert_generic_command(command)

    def _convert_job_sections(self, postables):
        sections = super()._convert_job_sections(postables)
        if not self.values.get("UPPERCASE_OUTPUT", True):
            return sections
        return [(name, gcode.upper()) for name, gcode in sections]

    # ------------------------------------------------------------------
    # Sanity checks
    # ------------------------------------------------------------------

    def sanity_check_methods(self):  # TODO
        return super().sanity_check_methods() + [self._sanity_rigid_tapping]

    def _sanity_rigid_tapping(self, job):
        """A tap without a spindle speed cannot run rigid; the export refuses it,
        so say so where the operator reads the report."""
        if not self.values.get("RIGID_TAPPING", True):
            return []
        squawks = []
        for op in self._operations:
            if not PathUtil.activeForOp(op):
                continue
            path = getattr(op, "Path", None)
            if not path:
                continue
            spindle = 0.0
            for command in path.Commands:
                if command.Name in Constants.MCODE_SPINDLE_ON:
                    spindle = float(command.Parameters.get("S", 0) or 0)
                if command.Name in Constants.GCODE_MOVE_TAP:
                    speed = float(command.Parameters.get("S", 0) or 0) or spindle
                    tc = getattr(op, "ToolController", None)
                    if speed <= 0 and tc is not None:
                        speed = float(getattr(tc, "SpindleSpeed", 0) or 0)
                    if speed <= 0:
                        squawks.append(
                            self._create_squawk(
                                "WARNING",
                                translate(
                                    "CAM",
                                    "'{op}' taps with {cmd} but commands no spindle speed; "
                                    "rigid tapping (M29) needs one",
                                ).format(op=op.Label, cmd=command.Name),
                            )
                        )
                        break
        return squawks

    @property
    def tooltip(self):
        return out_tooltip


# Class alias for PostProcessorFactory: it looks for the title-cased post name.
Haasnextgeneration = HaasNextGeneration
hass_next_generation = HaasNextGeneration
Hass_Next_Generation = HaasNextGeneration
