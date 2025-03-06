# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022 - 2025 Larry Woestman <LarryWoestman2@gmail.com>   *
# *   Copyright (c) 2024 Ondsel <development@ondsel.com>                    *
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
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import argparse

from typing import Any, Dict, List, Optional, Tuple, Union

from Path.Post.Processor import PostProcessor
import Path.Post.UtilsArguments as PostUtilsArguments
import Path.Post.UtilsExport as PostUtilsExport

import Path
import FreeCAD

translate = FreeCAD.Qt.translate

DEBUG = False
if DEBUG:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

#
# Define some types that are used throughout this file.
#
Defaults = Dict[str, bool]
FormatHelp = str
GCodeOrNone = Optional[str]
GCodeSections = List[Tuple[str, GCodeOrNone]]
Parser = argparse.ArgumentParser
ParserArgs = Union[None, str, argparse.Namespace]
Postables = Union[List, List[Tuple[str, List]]]
Section = Tuple[str, List]
Sublist = List
Units = str
Values = Dict[str, Any]
Visible = Dict[str, bool]


class Refactored_Centroid(PostProcessor):
    """The Refactored Centroid post processor class."""

    def __init__(self, job) -> None:
        super().__init__(
            job=job,
            tooltip=translate("CAM", "Refactored Centroid post processor"),
            tooltipargs=[""],
            units="Metric",
        )
        self.reinitialize()
        Path.Log.debug("Refactored Centroid post processor initialized.")

    def reinitialize(self) -> None:
        """Initialize or reinitialize the 'core' data structures for the postprocessor."""
        #
        # This is also used to reinitialize the data structures between tests.
        #
        self.values: Values = {}
        self.init_values(self.values)
        self.argument_defaults: Defaults = {}
        self.init_argument_defaults(self.argument_defaults)
        self.arguments_visible: Visible = {}
        self.init_arguments_visible(self.arguments_visible)
        self.parser: Parser = self.init_arguments(
            self.values, self.argument_defaults, self.arguments_visible
        )
        #
        # Create another parser just to get a list of all possible arguments
        # that may be output using --output_all_arguments.
        #
        self.all_arguments_visible: Visible = {}
        for k in iter(self.arguments_visible):
            self.all_arguments_visible[k] = True
        self.all_visible: Parser = self.init_arguments(
            self.values, self.argument_defaults, self.all_arguments_visible
        )

    def init_values(self, values: Values) -> None:
        """Initialize values that are used throughout the postprocessor."""
        #
        PostUtilsArguments.init_shared_values(values)
        #
        # Set any values here that need to override the default values set
        # in the init_shared_values routine.
        #
        # Use 4 digits for axis precision by default.
        #
        values["AXIS_PRECISION"] = 4
        values["DEFAULT_AXIS_PRECISION"] = 4
        values["DEFAULT_INCH_AXIS_PRECISION"] = 4
        #
        # Use ";" as the comment symbol
        #
        values["COMMENT_SYMBOL"] = ";"
        #
        # Use 1 digit for feed precision by default.
        #
        values["FEED_PRECISION"] = 1
        values["DEFAULT_FEED_PRECISION"] = 1
        values["DEFAULT_INCH_FEED_PRECISION"] = 1
        #
        # This value usually shows up in the post_op comment as "Finish operation:".
        # Change it to "End" to produce "End operation:".
        #
        values["FINISH_LABEL"] = "End"
        #
        # If this value is True, then a list of tool numbers
        # with their labels are output just before the preamble.
        #
        values["LIST_TOOLS_IN_PREAMBLE"] = True
        #
        # Used in the argparser code as the "name" of the postprocessor program.
        # This would normally show up in the usage message in the TOOLTIP_ARGS.
        #
        values["MACHINE_NAME"] = "Centroid"
        #
        # This list controls the order of parameters in a line during output.
        # centroid doesn't want K properties on XY plane; Arcs need work.
        #
        values["PARAMETER_ORDER"] = [
            "X",
            "Y",
            "Z",
            "A",
            "B",
            "I",
            "J",
            "F",
            "S",
            "T",
            "Q",
            "R",
            "L",
            "H",
        ]
        #
        # Any commands in this value will be output as the last commands
        # in the G-code file.
        #
        values["POSTAMBLE"] = """M99"""
        values["POSTPROCESSOR_FILE_NAME"] = __name__
        #
        # Any commands in this value will be output after the header and
        # safety block at the beginning of the G-code file.
        #
        values["PREAMBLE"] = """G53 G00 G17"""
        #
        # Output any messages.
        #
        values["REMOVE_MESSAGES"] = False
        #
        # Any commands in this value are output after the header but before the preamble,
        # then again after the TOOLRETURN but before the POSTAMBLE.
        #
        values["SAFETYBLOCK"] = """G90 G80 G40 G49"""
        #
        # Do not show the current machine units just before the PRE_OPERATION.
        #
        values["SHOW_MACHINE_UNITS"] = False
        #
        # Do not show the current operation label just before the PRE_OPERATION.
        #
        values["SHOW_OPERATION_LABELS"] = False
        #
        # Do not output an M5 command to stop the spindle for tool changes.
        #
        values["STOP_SPINDLE_FOR_TOOL_CHANGE"] = False
        #
        # spindle off, height offset canceled, spindle retracted
        # (M25 is a centroid command to retract spindle)
        #
        values[
            "TOOLRETURN"
        ] = """M5
M25
G49 H0"""
        values["UNITS"] = self._units
        #
        # Default to not outputting a G43 following tool changes
        #
        values["USE_TLO"] = False
        #
        # This was in the original centroid postprocessor file
        # but does not appear to be used anywhere.
        #
        # ZAXISRETURN = """G91 G28 X0 Z0 G90"""
        #

    def init_argument_defaults(self, argument_defaults: Defaults) -> None:
        """Initialize which arguments (in a pair) are shown as the default argument."""
        PostUtilsArguments.init_argument_defaults(argument_defaults)
        #
        # Modify which argument to show as the default in flag-type arguments here.
        # If the value is True, the first argument will be shown as the default.
        # If the value is False, the second argument will be shown as the default.
        #
        # For example, if you want to show Metric mode as the default, use:
        #   argument_defaults["metric_inch"] = True
        #
        # If you want to show that "Don't pop up editor for writing output" is
        # the default, use:
        #   argument_defaults["show-editor"] = False.
        #
        # Note:  You also need to modify the corresponding entries in the "values" hash
        #        to actually make the default value(s) change to match.
        #

    def init_arguments_visible(self, arguments_visible: Visible) -> None:
        """Initialize which argument pairs are visible in TOOLTIP_ARGS."""
        PostUtilsArguments.init_arguments_visible(arguments_visible)
        #
        # Modify the visibility of any arguments from the defaults here.
        #
        arguments_visible["axis-modal"] = False
        arguments_visible["precision"] = False
        arguments_visible["tlo"] = False

    def init_arguments(
        self,
        values: Values,
        argument_defaults: Defaults,
        arguments_visible: Visible,
    ) -> Parser:
        """Initialize the shared argument definitions."""
        _parser: Parser = PostUtilsArguments.init_shared_arguments(
            values, argument_defaults, arguments_visible
        )
        #
        # Add any argument definitions that are not shared with other postprocessors here.
        #
        return _parser

    def process_arguments(self) -> Tuple[bool, ParserArgs]:
        """Process any arguments to the postprocessor."""
        #
        # This function is separated out to make it easier to inherit from this postprocessor.
        #
        args: ParserArgs
        flag: bool

        (flag, args) = PostUtilsArguments.process_shared_arguments(
            self.values, self.parser, self._job.PostProcessorArgs, self.all_visible, "-"
        )
        #
        # If the flag is True, then all of the arguments should be processed normally.
        #
        if flag:
            #
            # Process any additional arguments here.
            #
            #
            # Update any variables that might have been modified while processing the arguments.
            #
            self._units = self.values["UNITS"]
        #
        # If the flag is False, then args is either None (indicating an error while
        # processing the arguments) or a string containing the argument list formatted
        # for output.  Either way the calling routine will need to handle the args value.
        #
        return (flag, args)

    def process_postables(self) -> GCodeSections:
        """Postprocess the 'postables' in the job to g code sections."""
        #
        # This function is separated out to make it easier to inherit from this postprocessor.
        #
        gcode: GCodeOrNone
        g_code_sections: GCodeSections
        partname: str
        postables: Postables
        section: Section
        sublist: Sublist

        postables = self._buildPostList()

        Path.Log.debug(f"postables count: {len(postables)}")

        g_code_sections = []
        for _, section in enumerate(postables):
            partname, sublist = section
            gcode = PostUtilsExport.export_common(self.values, sublist, "-")
            g_code_sections.append((partname, gcode))

        return g_code_sections

    def export(self) -> GCodeSections:
        """Process the parser arguments, then postprocess the 'postables'."""
        args: ParserArgs
        flag: bool

        Path.Log.debug("Exporting the job")

        (flag, args) = self.process_arguments()
        #
        # If the flag is True, then continue postprocessing the 'postables'
        #
        if flag:
            return self.process_postables()
        #
        # The flag is False meaning something unusual happened.
        #
        # If args is None then there was an error during argument processing.
        #
        if args is None:
            return None
        #
        # Otherwise args will contain the argument list formatted for output
        # instead of the "usual" gcode.
        #
        return [("allitems", args)]  # type: ignore

    @property
    def tooltip(self):
        tooltip: str = """
        This is a postprocessor file for the CAM workbench.
        It is used to take a pseudo-gcode fragment from a CAM object
        and output 'real' GCode suitable for a centroid 3 axis mill.
        """
        return tooltip

    @property
    def tooltipArgs(self) -> FormatHelp:
        return self.parser.format_help()

    @property
    def units(self) -> Units:
        return self._units
