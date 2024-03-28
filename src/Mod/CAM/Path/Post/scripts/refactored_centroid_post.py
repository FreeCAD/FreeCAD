# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
# *   Copyright (c) 2020 Schildkroet                                        *
# *   Copyright (c) 2022 Larry Woestman <LarryWoestman2@gmail.com>          *
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

from typing import Any, Dict, Union

import Path.Post.UtilsArguments as PostUtilsArguments
import Path.Post.UtilsExport as PostUtilsExport

# Define some types that are used throughout this file
Parser = argparse.ArgumentParser
Values = Dict[str, Any]

#
# The following variables need to be global variables
# to keep the PathPostProcessor.load method happy:
#
#    TOOLTIP
#    TOOLTIP_ARGS
#    UNITS
#
#    The "argument_defaults", "arguments_visible", and the "values" hashes
#    need to be defined before the "init_shared_arguments" routine can be
#    called to create TOOLTIP_ARGS, so they also end up having to be globals.
#    TOOLTIP_ARGS can be defined, so they end up being global variables also.
#
TOOLTIP: str = """
This is a postprocessor file for the Path workbench. It is used to
take a pseudo-G-code fragment outputted by a Path object, and output
real G-code suitable for a centroid 3 axis mill. This postprocessor, once placed
in the appropriate PathScripts folder, can be used directly from inside
FreeCAD, via the GUI importer or via python scripts with:

import refactored_centroid_post
refactored_centroid_post.export(object,"/path/to/file.ncc","")
"""
#
# Default to metric mode
#
UNITS: str = "G21"


def init_values(values: Values) -> None:
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
    # This would normally show up in the usage message in the TOOLTIP_ARGS,
    # but we are suppressing the usage message, so it doesn't show up after all.
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
    values["UNITS"] = UNITS
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


def init_argument_defaults(argument_defaults: Dict[str, bool]) -> None:
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


def init_arguments_visible(arguments_visible: Dict[str, bool]) -> None:
    """Initialize which argument pairs are visible in TOOLTIP_ARGS."""
    PostUtilsArguments.init_arguments_visible(arguments_visible)
    #
    # Modify the visibility of any arguments from the defaults here.
    #
    arguments_visible["axis-modal"] = False
    arguments_visible["precision"] = False
    arguments_visible["tlo"] = False


def init_arguments(
    values: Values,
    argument_defaults: Dict[str, bool],
    arguments_visible: Dict[str, bool],
) -> Parser:
    """Initialize the shared argument definitions."""
    parser: Parser = PostUtilsArguments.init_shared_arguments(
        values, argument_defaults, arguments_visible
    )
    #
    # Add any argument definitions that are not shared with all other
    # postprocessors here.
    #
    return parser


#
# Creating global variables and using functions to modify them
# is useful for being able to test things later.
#
global_values: Values = {}
init_values(global_values)
global_argument_defaults: Dict[str, bool] = {}
init_argument_defaults(global_argument_defaults)
global_arguments_visible: Dict[str, bool] = {}
init_arguments_visible(global_arguments_visible)
global_parser: Parser = init_arguments(
    global_values, global_argument_defaults, global_arguments_visible
)
#
# The TOOLTIP_ARGS value is created from the help information about the arguments.
#
TOOLTIP_ARGS: str = global_parser.format_help()
#
# Create another parser just to get a list of all possible arguments
# that may be output using --output_all_arguments.
#
global_all_arguments_visible: Dict[str, bool] = {}
k: str
for k in iter(global_arguments_visible):
    global_all_arguments_visible[k] = True
global_all_visible: Parser = init_arguments(
    global_values, global_argument_defaults, global_all_arguments_visible
)


def export(objectslist, filename: str, argstring: str) -> str:
    """Postprocess the objects in objectslist to filename."""
    args: Union[str, argparse.Namespace]
    flag: bool

    global UNITS  # pylint: disable=global-statement

    # print(parser.format_help())

    (flag, args) = PostUtilsArguments.process_shared_arguments(
        global_values, global_parser, argstring, global_all_visible, filename
    )
    if not flag:
        return args  # type: ignore
    #
    # Process any additional arguments here
    #

    #
    # Update the global variables that might have been modified
    # while processing the arguments.
    #
    UNITS = global_values["UNITS"]

    return PostUtilsExport.export_common(global_values, objectslist, filename)
