# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2018, 2019 Gauthier Briere                              *
# *   Copyright (c) 2019, 2020 Schildkroet                                  *
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


import Path.Post.UtilsArguments as PostUtilsArguments
import Path.Post.UtilsExport as PostUtilsExport

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
#
TOOLTIP = """
Generate g-code from a Path that is compatible with the grbl controller:

import refactored_grbl_post
refactored_grbl_post.export(object, "/path/to/file.ncc")
"""
#
# Default to metric mode
#
UNITS = "G21"


def init_values(values):
    """Initialize values that are used throughout the postprocessor."""
    #
    global UNITS

    PostUtilsArguments.init_shared_values(values)
    #
    # Set any values here that need to override the default values set
    # in the init_shared_values routine.
    #
    #
    # If this is set to True, then commands that are placed in
    # comments that look like (MC_RUN_COMMAND: blah) will be output.
    #
    values["ENABLE_MACHINE_SPECIFIC_COMMANDS"] = True
    #
    # Used in the argparser code as the "name" of the postprocessor program.
    # This would normally show up in the usage message in the TOOLTIP_ARGS,
    # but we are suppressing the usage message, so it doesn't show up after all.
    #
    values["MACHINE_NAME"] = "Grbl"
    #
    # Default to outputting Path labels at the beginning of each Path.
    #
    values["OUTPUT_PATH_LABELS"] = True
    #
    # Default to not outputting M6 tool changes (comment it) as grbl
    # currently does not handle it
    #
    values["OUTPUT_TOOL_CHANGE"] = False
    #
    # The order of the parameters.
    # Arcs may only work on the XY plane (this needs to be verified).
    #
    values["PARAMETER_ORDER"] = [
        "X",
        "Y",
        "Z",
        "A",
        "B",
        "C",
        "U",
        "V",
        "W",
        "I",
        "J",
        "K",
        "F",
        "S",
        "T",
        "Q",
        "R",
        "L",
        "P",
    ]
    #
    # Any commands in this value will be output as the last commands
    # in the G-code file.
    #
    values[
        "POSTAMBLE"
    ] = """M5
G17 G90
M2"""
    values["POSTPROCESSOR_FILE_NAME"] = __name__
    #
    # Any commands in this value will be output after the header and
    # safety block at the beginning of the G-code file.
    #
    values["PREAMBLE"] = """G17 G90"""
    #
    # Do not show the current machine units just before the PRE_OPERATION.
    #
    values["SHOW_MACHINE_UNITS"] = False
    values["UNITS"] = UNITS
    #
    # Default to not outputting a G43 following tool changes
    #
    values["USE_TLO"] = False


def init_argument_defaults(argument_defaults):
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
    argument_defaults["tlo"] = False
    argument_defaults["tool_change"] = False


def init_arguments_visible(arguments_visible):
    """Initialize which argument pairs are visible in TOOLTIP_ARGS."""
    PostUtilsArguments.init_arguments_visible(arguments_visible)
    #
    # Modify the visibility of any arguments from the defaults here.
    #
    arguments_visible["bcnc"] = True
    arguments_visible["axis-modal"] = False
    arguments_visible["return-to"] = True
    arguments_visible["tlo"] = False
    arguments_visible["tool_change"] = True
    arguments_visible["translate_drill"] = True
    arguments_visible["wait-for-spindle"] = True


def init_arguments(values, argument_defaults, arguments_visible):
    """Initialize the shared argument definitions."""
    parser = PostUtilsArguments.init_shared_arguments(
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
values = {}
init_values(values)
argument_defaults = {}
init_argument_defaults(argument_defaults)
arguments_visible = {}
init_arguments_visible(arguments_visible)
parser = init_arguments(values, argument_defaults, arguments_visible)
#
# The TOOLTIP_ARGS value is created from the help information about the arguments.
#
TOOLTIP_ARGS = parser.format_help()
#
# Create another parser just to get a list of all possible arguments
# that may be output using --output_all_arguments.
#
all_arguments_visible = {}
for k in iter(arguments_visible):
    all_arguments_visible[k] = True
all_visible = init_arguments(values, argument_defaults, all_arguments_visible)


def export(objectslist, filename, argstring):
    """Postprocess the objects in objectslist to filename."""
    #
    global parser
    global UNITS
    global values

    # print(parser.format_help())

    (flag, args) = PostUtilsArguments.process_shared_arguments(
        values, parser, argstring, all_visible, filename
    )
    if not flag:
        return args
    #
    # Process any additional arguments here
    #

    #
    # Update the global variables that might have been modified
    # while processing the arguments.
    #
    UNITS = values["UNITS"]

    return PostUtilsExport.export_common(values, objectslist, filename)
