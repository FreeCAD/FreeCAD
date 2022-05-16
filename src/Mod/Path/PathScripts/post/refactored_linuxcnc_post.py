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

from __future__ import print_function

import argparse
import datetime
import shlex

import FreeCAD
from FreeCAD import Units
import Path
from PathScripts import PostUtils

# to distinguish python built-in open function from the one declared below
if open.__module__ in ["__builtin__", "io"]:
    pythonopen = open

#
# Holds various values that are used throughout the postprocessor code.
#
values = {}


def init_values():
    """Initialize many of the commonly used values."""
    values["now"] = datetime.datetime.now()
    values[
        "TOOLTIP"
    ] = """This is a postprocessor file for the Path workbench. It is used to
    take a pseudo-gcode fragment outputted by a Path object, and output
    real GCode suitable for a linuxcnc 3 axis mill. This postprocessor, once placed
    in the appropriate PathScripts folder, can be used directly from inside
    FreeCAD, via the GUI importer or via python scripts with:

    import linuxcnc_post
    linuxcnc_post.export(object,"/path/to/file.ncc","")
    """
    #
    # These values set common customization preferences
    #
    values["OUTPUT_COMMENTS"] = True
    values["OUTPUT_HEADER"] = True
    values["OUTPUT_LINE_NUMBERS"] = False
    values["SHOW_EDITOR"] = True
    # if true commands are suppressed if the same as previous line.
    values["MODAL"] = False
    # if true G43 will be output following tool changes
    values["USE_TLO"] = True
    # if false duplicate axis values are suppressed if the same as previous line.
    values["OUTPUT_DOUBLES"] = True
    values["COMMAND_SPACE"] = " "
    # line number starting value
    values["LINENR"] = 100
    #
    # These values will be reflected in the Machine configuration of the project
    #
    # G21 for metric, G20 for US standard
    values["UNITS"] = "G21"
    values["UNIT_SPEED_FORMAT"] = "mm/min"
    values["UNIT_FORMAT"] = "mm"
    values["MACHINE_NAME"] = "LinuxCNC"
    values["CORNER_MIN"] = {"x": 0, "y": 0, "z": 0}
    values["CORNER_MAX"] = {"x": 500, "y": 300, "z": 300}
    values["PRECISION"] = 3
    # Preamble text will appear at the beginning of the GCODE output file.
    values["PREAMBLE"] = """G17 G54 G40 G49 G80 G90"""
    # Postamble text will appear following the last operation.
    values[
        "POSTAMBLE"
    ] = """M05
G17 G54 G90 G80 G40
M2
"""
    # Pre operation text will be inserted before every operation
    values["PRE_OPERATION"] = """"""
    # Post operation text will be inserted after every operation
    values["POST_OPERATION"] = """"""
    # Tool Change commands will be inserted before a tool change
    values["TOOL_CHANGE"] = """"""


def processArguments(values, argstring):
    """Process the arguments to the postprocessor."""
    parser = argparse.ArgumentParser(prog="linuxcnc", add_help=False)
    parser.add_argument("--no-header", action="store_true", help="suppress header output")
    parser.add_argument("--no-comments", action="store_true", help="suppress comment output")
    parser.add_argument("--line-numbers", action="store_true", help="prefix with line numbers")
    parser.add_argument(
        "--no-show-editor",
        action="store_true",
        help="don't pop up editor before writing output",
    )
    parser.add_argument("--precision", default="3", help="number of digits of precision, default=3")
    parser.add_argument(
        "--preamble",
        help='set commands to be issued before the first command, default="G17\nG90"',
    )
    parser.add_argument(
        "--postamble",
        help='set commands to be issued after the last command, default="M05\nG17 G90\nM2"',
    )
    parser.add_argument(
        "--inches", action="store_true", help="Convert output for US imperial mode (G20)"
    )
    parser.add_argument(
        "--modal",
        action="store_true",
        help="Output the Same G-command Name USE NonModal Mode",
    )
    parser.add_argument("--axis-modal", action="store_true", help="Output the Same Axis Value Mode")
    parser.add_argument(
        "--no-tlo",
        action="store_true",
        help="suppress tool length offset (G43) following tool changes",
    )

    values["TOOLTIP_ARGS"] = parser.format_help()

    try:
        args = parser.parse_args(shlex.split(argstring))
        if args.no_header:
            values["OUTPUT_HEADER"] = False
        if args.no_comments:
            values["OUTPUT_COMMENTS"] = False
        if args.line_numbers:
            values["OUTPUT_LINE_NUMBERS"] = True
        if args.no_show_editor:
            values["SHOW_EDITOR"] = False
        values["PRECISION"] = args.precision
        if args.preamble is not None:
            values["PREAMBLE"] = args.preamble
        if args.postamble is not None:
            values["POSTAMBLE"] = args.postamble
        if args.inches:
            values["UNITS"] = "G20"
            values["UNIT_SPEED_FORMAT"] = "in/min"
            values["UNIT_FORMAT"] = "in"
            values["PRECISION"] = 4
        if args.modal:
            values["MODAL"] = True
        if args.no_tlo:
            values["USE_TLO"] = False
        if args.axis_modal:
            values["OUTPUT_DOUBLES"] = False

    except Exception:
        return False

    return True


def export(objectslist, filename, argstring):
    """Postprocess the objects in objectslist to filename."""
    init_values()
    if not processArguments(values, argstring):
        return None

    for obj in objectslist:
        if not hasattr(obj, "Path"):
            print(
                "the object " + obj.Name + " is not a path. Please select only path and Compounds."
            )
            return None

    print("postprocessing...")
    gcode = ""

    # write header
    if values["OUTPUT_HEADER"]:
        gcode += linenumber(values) + "(Exported by FreeCAD)\n"
        gcode += linenumber(values) + "(Post Processor: " + __name__ + ")\n"
        gcode += linenumber(values) + "(Output Time:" + str(values["now"]) + ")\n"

    # Write the preamble
    if values["OUTPUT_COMMENTS"]:
        gcode += linenumber(values) + "(begin preamble)\n"
    for line in values["PREAMBLE"].splitlines(False):
        gcode += linenumber(values) + line + "\n"
    gcode += linenumber(values) + values["UNITS"] + "\n"

    for obj in objectslist:

        # Skip inactive operations
        if hasattr(obj, "Active"):
            if not obj.Active:
                continue
        if hasattr(obj, "Base") and hasattr(obj.Base, "Active"):
            if not obj.Base.Active:
                continue

        # do the pre_op
        if values["OUTPUT_COMMENTS"]:
            gcode += linenumber(values) + "(begin operation: %s)\n" % obj.Label
            gcode += linenumber(values) + "(machine units: %s)\n" % (values["UNIT_SPEED_FORMAT"])
        for line in values["PRE_OPERATION"].splitlines(True):
            gcode += linenumber(values) + line

        # get coolant mode
        coolantMode = "None"
        if hasattr(obj, "CoolantMode") or hasattr(obj, "Base") and hasattr(obj.Base, "CoolantMode"):
            if hasattr(obj, "CoolantMode"):
                coolantMode = obj.CoolantMode
            else:
                coolantMode = obj.Base.CoolantMode

        # turn coolant on if required
        if values["OUTPUT_COMMENTS"]:
            if not coolantMode == "None":
                gcode += linenumber(values) + "(Coolant On:" + coolantMode + ")\n"
        if coolantMode == "Flood":
            gcode += linenumber(values) + "M8" + "\n"
        if coolantMode == "Mist":
            gcode += linenumber(values) + "M7" + "\n"

        # process the operation gcode
        gcode += parse(values, obj)

        # do the post_op
        if values["OUTPUT_COMMENTS"]:
            gcode += linenumber(values) + "(finish operation: %s)\n" % obj.Label
        for line in values["POST_OPERATION"].splitlines(True):
            gcode += linenumber(values) + line

        # turn coolant off if required
        if not coolantMode == "None":
            if values["OUTPUT_COMMENTS"]:
                gcode += linenumber(values) + "(Coolant Off:" + coolantMode + ")\n"
            gcode += linenumber(values) + "M9" + "\n"

    # do the post_amble
    if values["OUTPUT_COMMENTS"]:
        gcode += "(begin postamble)\n"
    for line in values["POSTAMBLE"].splitlines(True):
        gcode += linenumber(values) + line

    if FreeCAD.GuiUp and values["SHOW_EDITOR"]:
        final = gcode
        if len(gcode) > 100000:
            print("Skipping editor since output is greater than 100kb")
        else:
            dia = PostUtils.GCodeEditorDialog()
            dia.editor.setText(gcode)
            result = dia.exec_()
            if result:
                final = dia.editor.toPlainText()
    else:
        final = gcode

    print("done postprocessing.")

    if not filename == "-":
        gfile = pythonopen(filename, "w")
        gfile.write(final)
        gfile.close()

    return final


def linenumber(values):
    """Output the next line number if appropriate."""
    if values["OUTPUT_LINE_NUMBERS"]:
        values["LINENR"] += 10
        return "N" + str(values["LINENR"]) + " "
    return ""


def parse(values, pathobj):
    """Parse a Path."""
    out = ""
    lastcommand = None
    precision_string = "." + str(values["PRECISION"]) + "f"
    currLocation = {}  # keep track for no doubles

    # the order of parameters
    # linuxcnc doesn't want K properties on XY plane  Arcs need work.
    params = [
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
    firstmove = Path.Command("G0", {"X": -1, "Y": -1, "Z": -1, "F": 0.0})
    currLocation.update(firstmove.Parameters)  # set First location Parameters

    if hasattr(pathobj, "Group"):  # We have a compound or project.
        # if values["OUTPUT_COMMENTS"]:
        #     out += linenumber(values) + "(compound: " + pathobj.Label + ")\n"
        for p in pathobj.Group:
            out += parse(values, p)
        return out
    else:  # parsing simple path

        # groups might contain non-path things like stock.
        if not hasattr(pathobj, "Path"):
            return out

        # if values["OUTPUT_COMMENTS"]:
        #     out += linenumber(values) + "(" + pathobj.Label + ")\n"

        for c in pathobj.Path.Commands:

            outstring = []
            command = c.Name
            outstring.append(command)

            # if modal: suppress the command if it is the same as the last one
            if values["MODAL"]:
                if command == lastcommand:
                    outstring.pop(0)

            if c.Name[0] == "(" and not values["OUTPUT_COMMENTS"]:  # command is a comment
                continue

            # Now add the remaining parameters in order
            for param in params:
                if param in c.Parameters:
                    if param == "F" and (
                        currLocation[param] != c.Parameters[param] or values["OUTPUT_DOUBLES"]
                    ):
                        if c.Name not in [
                            "G0",
                            "G00",
                        ]:  # linuxcnc doesn't use rapid speeds
                            speed = Units.Quantity(c.Parameters["F"], FreeCAD.Units.Velocity)
                            if speed.getValueAs(values["UNIT_SPEED_FORMAT"]) > 0.0:
                                outstring.append(
                                    param
                                    + format(
                                        float(speed.getValueAs(values["UNIT_SPEED_FORMAT"])),
                                        precision_string,
                                    )
                                )
                        else:
                            continue
                    elif param == "T":
                        outstring.append(param + str(int(c.Parameters["T"])))
                    elif param == "H":
                        outstring.append(param + str(int(c.Parameters["H"])))
                    elif param == "D":
                        outstring.append(param + str(int(c.Parameters["D"])))
                    elif param == "S":
                        outstring.append(param + str(int(c.Parameters["S"])))
                    else:
                        if (
                            (not values["OUTPUT_DOUBLES"])
                            and (param in currLocation)
                            and (currLocation[param] == c.Parameters[param])
                        ):
                            continue
                        else:
                            pos = Units.Quantity(c.Parameters[param], FreeCAD.Units.Length)
                            outstring.append(
                                param
                                + format(
                                    float(pos.getValueAs(values["UNIT_FORMAT"])), precision_string
                                )
                            )

            # store the latest command
            lastcommand = command
            currLocation.update(c.Parameters)

            # Check for Tool Change:
            if command == "M6":
                # stop the spindle
                out += linenumber(values) + "M5\n"
                for line in values["TOOL_CHANGE"].splitlines(True):
                    out += linenumber(values) + line

                # add height offset
                if values["USE_TLO"]:
                    tool_height = "\nG43 H" + str(int(c.Parameters["T"]))
                    outstring.append(tool_height)

            if command == "message":
                if values["OUTPUT_COMMENTS"] is False:
                    out = []
                else:
                    outstring.pop(0)  # remove the command

            # prepend a line number and append a newline
            if len(outstring) >= 1:
                if values["OUTPUT_LINE_NUMBERS"]:
                    outstring.insert(0, (linenumber(values)))

                # append the line to the final output
                for w in outstring:
                    out += w + values["COMMAND_SPACE"]
                # Note: Do *not* strip `out`, since that forces the allocation
                # of a contiguous string & thus quadratic complexity.
                out += "\n"

        return out


# print(__name__ + " gcode postprocessor loaded.")
