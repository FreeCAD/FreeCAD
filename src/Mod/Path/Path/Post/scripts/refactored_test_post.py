# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
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
TOOLTIP = """This is a postprocessor file for the Path workbench. It is used to
test the postprocessor code.  It probably isn't useful for "real" G-code.

import refactored_test_post
refactored_test_post.export(object,"/path/to/file.ncc","")
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
    # Turn off as much functionality as possible by default.
    # Then the tests can turn back on the appropriate options as needed.
    #
    # Used in the argparser code as the "name" of the postprocessor program.
    # This would normally show up in the usage message in the TOOLTIP_ARGS,
    # but we are suppressing the usage message, so it doesn't show up after all.
    #
    values["MACHINE_NAME"] = "test"
    #
    # Don't output comments by default
    #
    values["OUTPUT_COMMENTS"] = False
    #
    # Don't output the header by default
    #
    values["OUTPUT_HEADER"] = False
    #
    # Convert M56 tool change commands to comments,
    # which are then suppressed by default.
    #
    values["OUTPUT_TOOL_CHANGE"] = False
    values["POSTPROCESSOR_FILE_NAME"] = __name__
    #
    # Do not show the editor by default since we are testing.
    #
    values["SHOW_EDITOR"] = False
    #
    # Don't show the current machine units by default
    #
    values["SHOW_MACHINE_UNITS"] = False
    #
    # Don't show the current operation label by default.
    #
    values["SHOW_OPERATION_LABELS"] = False
    #
    # Don't output an M5 command to stop the spindle after an M6 tool change by default.
    #
    values["STOP_SPINDLE_FOR_TOOL_CHANGE"] = False
    #
    # Don't output a G43 tool length command following tool changes by default.
    #
    values["USE_TLO"] = False
    values["UNITS"] = UNITS


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


def init_arguments_visible(arguments_visible):
    """Initialize which argument pairs are visible in TOOLTIP_ARGS."""
    PostUtilsArguments.init_arguments_visible(arguments_visible)
    #
    # Modify the visibility of any arguments from the defaults here.
    #
    #
    # Make all arguments invisible by default.
    #
    for k in iter(arguments_visible):
        arguments_visible[k] = False


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
    global all_visible
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
