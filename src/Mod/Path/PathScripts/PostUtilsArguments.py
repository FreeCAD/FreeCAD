# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
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

"""
These are functions related to arguments and values for creating custom post processors.
"""
import argparse
import os
import shlex


def add_flag_type_arguments(
    argument_group, default_flag, true_argument, false_argument, true_help, false_help, visible=True
):
    if visible:
        if default_flag:
            true_help += " (default)"
        else:
            false_help += " (default)"
    else:
        true_help = false_help = argparse.SUPPRESS
    argument_group.add_argument(true_argument, action="store_true", help=true_help)
    argument_group.add_argument(false_argument, action="store_true", help=false_help)


def init_argument_defaults(argument_defaults):
    """Initialize which argument to show as the default in flag-type arguments"""
    argument_defaults["axis-modal"] = False
    argument_defaults["bcnc"] = False
    argument_defaults["comments"] = True
    argument_defaults["header"] = True
    argument_defaults["line-numbers"] = False
    argument_defaults["metric_inches"] = True
    argument_defaults["modal"] = False
    argument_defaults["show-editor"] = True
    argument_defaults["tlo"] = True
    argument_defaults["tool_change"] = True
    argument_defaults["translate_drill"] = False


def init_arguments_visible(arguments_visible):
    """Initialize the flags for which arguments are visible in the arguments tooltip."""
    arguments_visible["bcnc"] = False
    arguments_visible["axis-modal"] = True
    arguments_visible["axis-precision"] = True
    arguments_visible["comments"] = True
    arguments_visible["feed-precision"] = True
    arguments_visible["header"] = True
    arguments_visible["line-numbers"] = True
    arguments_visible["metric_inches"] = True
    arguments_visible["modal"] = True
    arguments_visible["postamble"] = True
    arguments_visible["preamble"] = True
    arguments_visible["precision"] = True
    arguments_visible["return-to"] = False
    arguments_visible["show-editor"] = True
    arguments_visible["tlo"] = True
    arguments_visible["tool_change"] = False
    arguments_visible["translate_drill"] = False
    arguments_visible["wait-for-spindle"] = False


def init_shared_arguments(values, argument_defaults, arguments_visible):
    """Initialize the shared arguments for postprocessors."""
    parser = argparse.ArgumentParser(
        prog=values["MACHINE_NAME"], usage=argparse.SUPPRESS, add_help=False
    )
    shared = parser.add_argument_group("Arguments that are shared with all postprocessors")
    add_flag_type_arguments(
        shared,
        argument_defaults["metric_inches"],
        "--metric",
        "--inches",
        "Convert output for Metric mode (G21)",
        "Convert output for US imperial mode (G20)",
        arguments_visible["metric_inches"],
    )
    add_flag_type_arguments(
        shared,
        argument_defaults["axis-modal"],
        "--axis-modal",
        "--no-axis-modal",
        "Don't output axis values if they are the same as the previous line",
        "Output axis values even if they are the same as the previous line",
        arguments_visible["axis-modal"],
    )
    if arguments_visible["axis-precision"]:
        help_message = (
            "Number of digits of precision for axis moves, default is "
            + str(values["DEFAULT_AXIS_PRECISION"])
        )
    else:
        help_message = argparse.SUPPRESS
    shared.add_argument(
        "--axis-precision",
        default=-1,
        type=int,
        help=help_message,
    )
    add_flag_type_arguments(
        shared,
        argument_defaults["bcnc"],
        "--bcnc",
        "--no-bcnc",
        "Add Job operations as bCNC block headers. Consider suppressing comments by adding --no-comments",
        "Suppress bCNC block header output",
        arguments_visible["bcnc"],
    )
    add_flag_type_arguments(
        shared,
        argument_defaults["comments"],
        "--comments",
        "--no-comments",
        "Output comments",
        "Suppress comment output",
        arguments_visible["comments"],
    )
    if arguments_visible["feed-precision"]:
        help_message = (
            "Number of digits of precision for feed rate, default is "
            + str(values["DEFAULT_FEED_PRECISION"])
        )
    else:
        help_message = argparse.SUPPRESS
    shared.add_argument(
        "--feed-precision",
        default=-1,
        type=int,
        help=help_message,
    )
    add_flag_type_arguments(
        shared,
        argument_defaults["header"],
        "--header",
        "--no-header",
        "Output headers",
        "Suppress header output",
        arguments_visible["header"],
    )
    add_flag_type_arguments(
        shared,
        argument_defaults["line-numbers"],
        "--line-numbers",
        "--no-line-numbers",
        "Prefix with line numbers",
        "Don't prefix with line numbers",
        arguments_visible["line-numbers"],
    )
    add_flag_type_arguments(
        shared,
        argument_defaults["modal"],
        "--modal",
        "--no-modal",
        "Don't output the G-command name if it is the same as the previous line",
        "Output the G-command name even if it is the same as the previous line",
        arguments_visible["modal"],
    )
    if arguments_visible["postamble"]:
        help_message = (
            'Set commands to be issued after the last command, default is "'
            + values["POSTAMBLE"]
            + '"'
        )
    else:
        help_message = argparse.SUPPRESS
    shared.add_argument("--postamble", help=help_message)
    if arguments_visible["preamble"]:
        help_message = (
            'Set commands to be issued before the first command, default is "'
            + values["PREAMBLE"]
            + '"'
        )
    else:
        help_message = argparse.SUPPRESS
    shared.add_argument("--preamble", help=help_message)
    # The --precision argument is included for backwards compatibility with some postprocessors.
    # If both --axis-precision and --precision are provided, the --axis-precision value "wins".
    # If both --feed-precision and --precision are provided, the --feed-precision value "wins".
    if arguments_visible["precision"]:
        help_message = (
            "Number of digits of precision for both feed rate and axis moves, default is "
            + str(values["DEFAULT_AXIS_PRECISION"])
            + " for metric or "
            + str(values["DEFAULT_INCH_AXIS_PRECISION"])
            + " for inches"
        )
    else:
        help_message = argparse.SUPPRESS
    shared.add_argument(
        "--precision",
        default=-1,
        type=int,
        help=help_message,
    )
    if arguments_visible["return-to"]:
        help_message = "Move to the specified x,y,z coordinates at the end, e.g. --return-to=0,0,0 (default is do not move)"
    else:
        help_message = argparse.SUPPRESS
    shared.add_argument("--return-to", default="", help=help_message)
    add_flag_type_arguments(
        shared,
        argument_defaults["show-editor"],
        "--show-editor",
        "--no-show-editor",
        "Pop up editor before writing output",
        "Don't pop up editor before writing output",
        arguments_visible["show-editor"],
    )
    add_flag_type_arguments(
        shared,
        argument_defaults["tlo"],
        "--tlo",
        "--no-tlo",
        "Output tool length offset (G43) following tool changes",
        "Suppress tool length offset (G43) following tool changes",
        arguments_visible["tlo"],
    )
    add_flag_type_arguments(
        shared,
        argument_defaults["tool_change"],
        "--tool_change",
        "--no-tool_change",
        "Insert M6 and any other tool change G-code for all tool changes",
        "Convert M6 to a comment for all tool changes",
        arguments_visible["tool_change"],
    )
    add_flag_type_arguments(
        shared,
        argument_defaults["translate_drill"],
        "--translate_drill",
        "--no-translate_drill",
        "Translate drill cycles G81, G82 & G83 into G0/G1 movements",
        "Don't translate drill cycles G81, G82 & G83 into G0/G1 movements",
        arguments_visible["translate_drill"],
    )
    if arguments_visible["wait-for-spindle"]:
        help_message = "Time to wait (in seconds) after M3, M4 (default = 0.0)"
    else:
        help_message = argparse.SUPPRESS
    shared.add_argument("--wait-for-spindle", type=float, default=0.0, help=help_message)
    return parser


def init_shared_values(values):
    """Initialize the default values in postprocessors."""
    #
    # The starting axis precision is 3 digits after the decimal point.
    #
    values["AXIS_PRECISION"] = 3
    #
    # If this is set to "", all spaces are removed from between commands and parameters.
    #
    values["COMMAND_SPACE"] = " "
    #
    # The character that indicates a comment.  While "(" is the most common,
    # ";" is also used.
    #
    values["COMMENT_SYMBOL"] = "("
    #
    # Variables storing the current position for the drill_translate routine.
    #
    values["CURRENT_X"] = 0
    values["CURRENT_Y"] = 0
    values["CURRENT_Z"] = 0
    #
    # Default axis precision for metric is 3 digits after the decimal point.
    # (see http://linuxcnc.org/docs/2.7/html/gcode/overview.html#_g_code_best_practices)
    #
    values["DEFAULT_AXIS_PRECISION"] = 3
    #
    # The default precision for feed is also set to 3 for metric.
    #
    values["DEFAULT_FEED_PRECISION"] = 3
    #
    # Default axis precision for inch/imperial is 4 digits after the decimal point.
    #
    values["DEFAULT_INCH_AXIS_PRECISION"] = 4
    #
    # The default precision for feed is also set to 4 for inch/imperial.
    #
    values["DEFAULT_INCH_FEED_PRECISION"] = 4
    #
    # If TRANSLATE_DRILL_CYCLES is True, these are the drill cycles
    # that get translated to G0 and G1 commands.
    #
    values["DRILL_CYCLES_TO_TRANSLATE"] = ("G81", "G82", "G83")
    #
    # The default value of drill retractations (CURRENT_Z).
    # The other possible value is G99.
    #
    values["DRILL_RETRACT_MODE"] = "G98"
    #
    # If this is set to True, then M7, M8, and M9 commands
    # to enable/disable coolant will be output.
    #
    values["ENABLE_COOLANT"] = False
    #
    # If this is set to True, then commands that are placed in
    # comments that look like (MC_RUN_COMMAND: blah) will be output.
    #
    values["ENABLE_MACHINE_SPECIFIC_COMMANDS"] = False
    #
    # By default the line ending characters of the output file(s)
    # are written to match the system that the postprocessor runs on.
    # If you need to force the line ending characters to a specific
    # value, set this variable to "\n" or "\r\n" instead.
    #
    values["END_OF_LINE_CHARACTERS"] = os.linesep
    #
    # The starting precision for feed is also set to 3 digits after the decimal point.
    #
    values["FEED_PRECISION"] = 3
    #
    # This value shows up in the post_op comment as "Finish operation:".
    # At least one postprocessor changes it to "End" to produce "End operation:".
    #
    values["FINISH_LABEL"] = "Finish"
    #
    # The name of the machine the postprocessor is for
    #
    values["MACHINE_NAME"] = "unknown machine"
    #
    # The line number increment value
    #
    values["LINE_INCREMENT"] = 10
    #
    # The line number starting value
    #
    values["line_number"] = 100
    #
    # If this value is True, then a list of tool numbers
    # with their labels are output just before the preamble.
    #
    values["LIST_TOOLS_IN_PREAMBLE"] = False
    #
    # If this value is true G-code commands are suppressed if they are
    # the same as the previous line.
    #
    values["MODAL"] = False
    #
    # This defines the motion commands that might change the X, Y, and Z position.
    #
    values["MOTION_COMMANDS"] = [
        "G0",
        "G00",
        "G1",
        "G01",
        "G2",
        "G02",
        "G3",
        "G03",
    ]
    #
    # Keeps track of the motion mode currently in use.
    # G90 for absolute moves, G91 for relative
    #
    values["MOTION_MODE"] = "G90"
    #
    # If True enables special processing for operations with "Adaptive" in the name
    #
    values["OUTPUT_ADAPTIVE"] = False
    #
    # If True adds bCNC operation block headers to the output G-code file.
    #
    values["OUTPUT_BCNC"] = False
    #
    # If True output comments.  If False comments are suppressed.
    #
    values["OUTPUT_COMMENTS"] = True
    #
    # if False duplicate axis values are suppressed if they are the same as the previous line.
    #
    values["OUTPUT_DOUBLES"] = True
    #
    # If True output the machine name in the pre_op
    #
    values["OUTPUT_MACHINE_NAME"] = False
    #
    # If True output a header at the front of the G-code file.
    # The header contains comments similar to:
    #   (Exported by FreeCAD)
    #   (Post Processor: centroid_post)
    #   (Cam File: box.fcstd)
    #   (Output Time:2020-01-01 01:02:03.123456)
    #
    values["OUTPUT_HEADER"] = True
    #
    # If True output line numbers at the front of each line.
    # If False do not output line numbers.
    #
    values["OUTPUT_LINE_NUMBERS"] = False
    #
    # If True output Path labels at the beginning of each Path.
    #
    values["OUTPUT_PATH_LABELS"] = False
    #
    # If True output tool change G-code for M6 commands followed
    # by any commands in the "TOOL_CHANGE" value.
    # If False output the M6 command as a comment and do not output
    # any commands in the "TOOL_CHANGE" value.
    #
    values["OUTPUT_TOOL_CHANGE"] = True
    #
    # This list controls the order of parameters in a line during output.
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
    values["POSTAMBLE"] = """"""
    #
    # Any commands in this value will be output after the operation(s).
    #
    values["POST_OPERATION"] = """"""
    #
    # Any commands in this value will be output after the header and
    # safety block at the beginning of the G-code file.
    #
    values["PREAMBLE"] = """"""
    #
    # Any commands in this value will be output before the operation(s).
    #
    values["PRE_OPERATION"] = """"""
    #
    # Defines which G-code commands are considered "rapid" moves.
    #
    values["RAPID_MOVES"] = ["G0", "G00"]
    #
    # If True suppress any messages.
    #
    values["REMOVE_MESSAGES"] = True
    #
    # Any commands in this value are output after the operation(s)
    # and post_operation commands are output but before the
    # TOOLRETURN, SAFETYBLOCK, and POSTAMBLE.
    #
    values["RETURN_TO"] = None
    #
    # Any commands in this value are output after the header but before the preamble,
    # then again after the TOOLRETURN but before the POSTAMBLE.
    #
    values["SAFETYBLOCK"] = """"""
    #
    # If True then the G-code editor widget is shown before writing
    # the G-code to the file.
    #
    values["SHOW_EDITOR"] = True
    #
    # If True then the current machine units are output just before the PRE_OPERATION.
    #
    values["SHOW_MACHINE_UNITS"] = True
    #
    # If True then the current operation label is output just before the PRE_OPERATION.
    #
    values["SHOW_OPERATION_LABELS"] = True
    #
    # The number of decimal places to use when outputting the speed (S) parameter.
    #
    values["SPINDLE_DECIMALS"] = 0
    #
    # The amount of time (in seconds) to wait after turning on the spindle
    # using an M3 or M4 command (a floating point number).
    #
    values["SPINDLE_WAIT"] = 0.0
    #
    # If true then then an M5 command to stop the spindle is output
    # after the M6 tool change command and before the TOOL_CHANGE commands.
    #
    values["STOP_SPINDLE_FOR_TOOL_CHANGE"] = True
    #
    # These commands are ignored by commenting them out.
    # Used when replacing the drill commands by G0 and G1 commands, for example.
    #
    values["SUPPRESS_COMMANDS"] = []
    #
    # Any commands in this value are output after the M6 command
    # when changing at tool (if OUTPUT_TOOL_CHANGE is True).
    #
    values["TOOL_CHANGE"] = """"""
    #
    # Any commands in this value are output after the POST_OPERATION,
    # RETURN_TO, and OUTPUT_BCNC and before the SAFETYBLOCK and POSTAMBLE.
    #
    values["TOOLRETURN"] = """"""
    #
    # If true, G81, G82 & G83 drill moves are translated into G0/G1 moves.
    #
    values["TRANSLATE_DRILL_CYCLES"] = False
    #
    # These values keep track of whether we are in Metric mode (G21)
    # or inches/imperial mode (G20).
    #
    values["UNITS"] = "G21"
    values["UNIT_FORMAT"] = "mm"
    values["UNIT_SPEED_FORMAT"] = "mm/min"
    #
    # If true a tool length command (G43) will be output following tool changes.
    #
    values["USE_TLO"] = True


def process_shared_arguments(values, parser, argstring):
    """Process the arguments to the postprocessor."""
    try:
        args = parser.parse_args(shlex.split(argstring))
        if args.metric:
            values["UNITS"] = "G21"
        if args.inches:
            values["UNITS"] = "G20"
        if values["UNITS"] == "G21":
            values["UNIT_FORMAT"] = "mm"
            values["UNIT_SPEED_FORMAT"] = "mm/min"
        if values["UNITS"] == "G20":
            values["UNIT_FORMAT"] = "in"
            values["UNIT_SPEED_FORMAT"] = "in/min"
        # The precision-related arguments need to be processed
        # after the metric/inches arguments are processed.
        # If both --axis-precision and --precision are given,
        # the --axis-precision argument "wins".
        if args.axis_precision != -1:
            values["AXIS_PRECISION"] = args.axis_precision
        elif args.precision != -1:
            values["AXIS_PRECISION"] = args.precision
        else:
            if values["UNITS"] == "G21":
                values["AXIS_PRECISION"] = values["DEFAULT_AXIS_PRECISION"]
            if values["UNITS"] == "G20":
                values["AXIS_PRECISION"] = values["DEFAULT_INCH_AXIS_PRECISION"]
        # If both --feed-precision and --precision are given,
        # the --feed-precision argument "wins".
        if args.feed_precision != -1:
            values["FEED_PRECISION"] = args.feed_precision
        elif args.precision != -1:
            values["FEED_PRECISION"] = args.precision
        else:
            if values["UNITS"] == "G21":
                values["FEED_PRECISION"] = values["DEFAULT_FEED_PRECISION"]
            if values["UNITS"] == "G20":
                values["FEED_PRECISION"] = values["DEFAULT_INCH_FEED_PRECISION"]
        if args.axis_modal:
            values["OUTPUT_DOUBLES"] = False
        if args.no_axis_modal:
            values["OUTPUT_DOUBLES"] = True
        if args.bcnc:
            values["OUTPUT_BCNC"] = True
        if args.no_bcnc:
            values["OUTPUT_BCNC"] = False
        if args.comments:
            values["OUTPUT_COMMENTS"] = True
        if args.no_comments:
            values["OUTPUT_COMMENTS"] = False
        if args.header:
            values["OUTPUT_HEADER"] = True
        if args.no_header:
            values["OUTPUT_HEADER"] = False
        if args.line_numbers:
            values["OUTPUT_LINE_NUMBERS"] = True
        if args.no_line_numbers:
            values["OUTPUT_LINE_NUMBERS"] = False
        if args.modal:
            values["MODAL"] = True
        if args.no_modal:
            values["MODAL"] = False
        if args.postamble is not None:
            values["POSTAMBLE"] = args.postamble
        if args.preamble is not None:
            values["PREAMBLE"] = args.preamble
        if args.return_to != "":
            values["RETURN_TO"] = [int(v) for v in args.return_to.split(",")]
            if len(values["RETURN_TO"]) != 3:
                values["RETURN_TO"] = None
                print("--return-to coordinates must be specified as <x>,<y>,<z>, ignoring")
        if args.show_editor:
            values["SHOW_EDITOR"] = True
        if args.no_show_editor:
            values["SHOW_EDITOR"] = False
        if args.tlo:
            values["USE_TLO"] = True
        if args.no_tlo:
            values["USE_TLO"] = False
        if args.tool_change:
            values["OUTPUT_TOOL_CHANGE"] = True
        if args.no_tool_change:
            values["OUTPUT_TOOL_CHANGE"] = False
        if args.translate_drill:
            values["TRANSLATE_DRILL_CYCLES"] = True
        if args.no_translate_drill:
            values["TRANSLATE_DRILL_CYCLES"] = False
        if args.wait_for_spindle > 0.0:
            values["SPINDLE_WAIT"] = args.wait_for_spindle

    except Exception:
        return (False, None)

    return (True, args)
