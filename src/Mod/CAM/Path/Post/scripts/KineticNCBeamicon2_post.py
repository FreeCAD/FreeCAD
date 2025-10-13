# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
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
# ***************************************************************************/

# ****************************************************************************
# *   Modifications by Samuel Mayer (samuel.mayer@posteo.de)                 *
# *   2021                                                                   *
# *                                                                          *
# *   This postprocessor is based on the linuxcnc_post coming with FreeCAD   *
# *   0.19 and modified to work with Kinetic-NC (cnc-step.com) and Beamicon2 *
# *   (benezan-electronics.de) (up to 4 Axis)                                *
# *                                                                          *
# ***************************************************************************/

# ****************************************************************************
# *   Modifications by neurallambda                                          *
# *   2025                                                                   *
# *                                                                          *
# *   Changes to KineticNCBeamicon2_post.py                                  *
# *     * fix tool change ordering (T2 *before* M6)                          *
# *     * remove globals mutation                                            *
# *     * handle newlines + line number more elegantly                       *
# *     * clean up `parse` and `export`                                      *
# *     * general refactor, clean up and comments                            *
# *                                                                          *
# ***************************************************************************/


import FreeCAD
from FreeCAD import Units
import Path
import Path.Post.Utils as PostUtils
import argparse
import datetime
import shlex
from PathScripts import PathUtils
from builtins import open as pyopen

now = datetime.datetime.now()

##########
# Globals
#
#   never mutated, could concievably be added to args parser if we want to edit
#   them.

# write comments at top: (Exported by FreeCAD) etc
OUTPUT_HEADER = True

# if false, duplicate axis values are suppressed if the same as previous line
#
# in the original Kinetic code, this is only False if `args.axis_modal` is True,
# but that arg doesn't exist, so we should probably remove `OUTPUT_DOUBLES`
# completely, and treat it as always true.
OUTPUT_DOUBLES = True

COMMAND_SPACE = " "

# Preamble text at top of g-code
#
#   G17: set XY plane for arcs
#   G21: set units to mm
#   G40: cancel cutter radius compensation
#   G49: cancel tool length offset
#   G80: cancel any active canned cycle
#   G90: absolute positions
#   M08: coolant on
PREAMBLE = """%
G17 G21 G40 G49 G80 G90
M08
"""

# Postamble follows last operation
#
#   M05: spindle stop
#   M09: coolant off
#   G17: XY plane selection
#   G90: absolute positioning
#   G80: cancel canned cycles
#   G40: cancel cutter radius compensation
#   M30: program end and rewind
POSTAMBLE = """M05 M09
G17 G90 G80 G40
M30
"""

# Pre operation text will be inserted before every operation
PRE_OPERATION = ""

# Post operation text will be inserted after every operation
POST_OPERATION = ""

# Tool Change commands will be inserted before a tool change
#
# M05: spindle stop
# M09: coolant off
TOOL_CHANGE = """M05
M09"""

TOOLTIP = """
This is a postprocessor file for the CAM workbench. It is used to
take a pseudo-G-code fragment outputted by a Path object, and output
real G-code suitable for the KineticNC/Beamicon2 Control Software for up to 4 Axis (3 plus rotary).

This postprocessor, once placed in the appropriate PathScripts folder, can be used directly from inside
FreeCAD, via the GUI importer or via python scripts with:

import KineticNCBeamicon2_post
KineticNCBeamicon2_post.export(object,"/path/to/file.ncc","")
"""


##########
# Arguments


def process_multiline_string(value):
    """Convert \\n to actual newlines in command-line strings"""
    return value.replace("\\n", "\n")


parser = argparse.ArgumentParser(
    prog="linuxcnc",
    add_help=False,
)
parser.add_argument("--no-header", action="store_true", help="suppress header output")
parser.add_argument(
    "--no-comments", action="store_true", help="suppress comment output", default=False
)
parser.add_argument(
    "--line-numbers", action="store_true", help="prefix with line numbers", default=False
)
parser.add_argument(
    "--no-show-editor",
    action="store_true",
    help="don't pop up editor before writing output",
)
parser.add_argument("--precision", default="3", help="number of digits of precision, default=3")

display = lambda x: x.replace("%", "%%").replace("\n", "\\n")
parser.add_argument(
    "--preamble",
    help=rf'set commands to be issued before the first command, default="{display(PREAMBLE)}"',
    type=process_multiline_string,
    default=PREAMBLE,
)
parser.add_argument(
    "--postamble",
    help=rf'set commands to be issued after the last command, default="{display(POSTAMBLE)}"',
    type=process_multiline_string,
    default=POSTAMBLE,
)
parser.add_argument(
    "--inches", action="store_true", help="Convert output for US imperial mode (G20)", default=False
)
parser.add_argument(
    "--modal",
    action="store_true",
    help="If true, repeated commands are suppressed (eg G1 X1 <newline> Y2 <newline> Z3)",
    default=False,
)
parser.add_argument("--axis-modal", action="store_true", help="Output the Same Axis Value Mode")

TOOLTIP_ARGS = parser.format_help()


def get_units(use_inches):
    if use_inches:
        units = "G20"  # G20 for US standard
        unit_speed_format = "in/min"
        unit_format = "in"
        # precision = 4  # from original, but we shouldn't override the user's precision choice
    else:
        units = "G21"  # G21 for metric
        unit_speed_format = "mm/min"
        unit_format = "mm"
    return units, unit_speed_format, unit_format


def format_lines(lines, args):
    """
    Apply line numbers and join lines with newlines.

    This is a helper that takes a list of raw g-code lines and:
      - prepends line numbers if config.output_line_numbers is True
      - joins them with newline characters
    """
    out = []
    current_line = 100
    for line in lines:
        if args.line_numbers:
            out.append(f"N{current_line} {line}")
            current_line += 10
        else:
            out.append(line)
    return "\n".join(out) + "\n"


def export(objectslist, filename, argstring):
    """Build a g-code string from a list of objects within a single Job. This
    doesn't actually export anything, but WrapperPost expects the function to be
    called `export`.

    Args:
      objectslist: FreeCAD objects prepared by `WrapperPost`
      filename: always "-"
      argstring: arguments entered in the Job Task Panel

    Returns:
      str, gcode to be saved to disk downstream
    """
    job = PathUtils.findParentJob(objectslist[0])

    args = parser.parse_args(shlex.split(argstring))

    units, unit_speed_format, unit_format = get_units(args.inches)

    for obj in objectslist:
        if not hasattr(obj, "Path"):
            print(
                "the object " + obj.Name + " is not a path. Please select only path and Compounds."
            )
            return None

    print("postprocessing...")
    out_lines = []  # raw, unnumbered lines

    # write header
    if OUTPUT_HEADER:
        out_lines.append("(Exported by FreeCAD)")
        out_lines.append("(Post Processor: " + __name__ + ")")
        out_lines.append(
            "(Output Time:" + str(now) + ")"
        )  # remove while debugging to make deterministic

    # Write the preamble
    if not args.no_comments:
        out_lines.append("(begin preamble)")
    for line in args.preamble.splitlines():
        out_lines.append(line)

    # G20/G21
    out_lines.append(units)

    # Iterate across ops
    for obj in objectslist:
        # do the pre_op
        if not args.no_comments:
            out_lines.append(f"(begin operation: {obj.Label})")
            out_lines.append(f"(machine: not set, {unit_speed_format})")
        for line in PRE_OPERATION.splitlines(True):
            # splitlines(True) keeps the newline, so strip it
            out_lines.append(line.rstrip("\n\r"))

        # actual op
        lines = parse(obj, args)
        out_lines.extend(lines)

        # do the post_op
        if not args.no_comments:
            out_lines.append("(finish operation: %s)" % obj.Label)
        for line in POST_OPERATION.splitlines(True):
            # splitlines(True) keeps the newline, so strip it
            out_lines.append(line.rstrip("\n\r"))

    # do the post_amble
    if not args.no_comments:
        out_lines.append("(begin postamble)")
    for line in args.postamble.splitlines():
        out_lines.append(line)

    # Format all lines at once (add line numbers and join with newlines)
    gcode = format_lines(out_lines, args)

    if FreeCAD.GuiUp and not args.no_show_editor:
        dia = PostUtils.GCodeEditorDialog()
        dia.editor.setText(gcode)
        result = dia.exec_()
        if result:
            final = dia.editor.toPlainText()
        else:
            final = gcode
    else:
        final = gcode

    print("done postprocessing.")

    return final


def parse(pathobj, args):
    """
    Convert a pathobj to a list of lines of g-code
    """
    out_lines = []  # raw, unnumbered lines
    lastcommand = None
    precision_string = "." + str(args.precision) + "f"
    currLocation = {}  # keep track for no doubles

    units, unit_speed_format, unit_format = get_units(args.inches)

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
        "Q",
        "R",
        "L",
        "H",
        "D",
        "P",
        # "T"  # tool changes handled directly now
    ]
    firstmove = Path.Command("G0", {"X": -1, "Y": -1, "Z": -1, "F": 0.0})
    currLocation.update(firstmove.Parameters)  # set First location Parameters

    if hasattr(pathobj, "Group"):  # We have a compound or project.
        if not args.no_comments:
            out_lines.append("(compound: " + pathobj.Label + ")")

        for p in pathobj.Group:
            child_lines = parse(p, args)
            if child_lines:
                out_lines.extend(child_lines)
        return out_lines

    else:  # parsing simple path

        # groups might contain non-path things like stock.
        if not hasattr(pathobj, "Path"):
            return []

        ##########
        # Loop over all commands
        for c in PathUtils.getPathWithPlacement(pathobj).Commands:
            # getPathWithPlacement: apply Placement attribute to paths

            outstring = []  # a line of gcode (eg: G0 X1 Y2 Z3)
            command = c.Name

            # In modal mode, suppress repeated commands
            # Ex:
            #   G1 X1
            #   Y1  ( doesn't mention G1 again )
            if (
                not args.modal  # if not modal, always output
                or command != lastcommand  # if modal and its a new command, output
            ):
                outstring.append(command)

            # Comments
            if c.Name.startswith("("):
                if not args.no_comments:
                    out_lines.append(c.Name)
                continue

            # Messages (convert to comments)
            if command == "message":
                if not args.no_comments:
                    # Get all parameters and join them into a comment
                    message = " ".join([str(v) for v in c.Parameters.values()])
                    if message:
                        out_lines.append("(" + message + ")")
                continue

            # Tool changes (Standard format: T2 M6)
            #
            # Controllers typically expect "T2 M6", but the original
            # postprocessor emits "M6 T2". The reasoning is big machines can
            # prepare for the tool change while still cutting with the old tool,
            # and issuing T2 first prepares for a faster M6 tool change.
            if command == "M6":
                tool_num = c.Parameters.get("T", None)

                # Tool change preamble + correct order, eg T2 *before* M6
                if tool_num is not None:
                    out_lines.append(f"{TOOL_CHANGE}T{int(tool_num)} M6")

                else:
                    raise Exception("M6 received, but tool_num was None")
                continue

            ##########
            # Add remaining parameters in order (G0/G1 happen here)
            for param in params:
                if param in c.Parameters:
                    if param == "F" and (  # Feed rate
                        currLocation[param] != c.Parameters[param] or OUTPUT_DOUBLES
                    ):
                        if c.Name not in [
                            "G0",
                            "G00",
                        ]:  # linuxcnc doesn't use rapid speeds
                            speed = Units.Quantity(c.Parameters["F"], FreeCAD.Units.Velocity)
                            if speed.getValueAs(unit_speed_format) > 0.0:
                                outstring.append(
                                    param
                                    + format(
                                        float(speed.getValueAs(unit_speed_format)),
                                        precision_string,
                                    )
                                )
                        else:
                            continue

                    elif param in [
                        # "T",  # Tool selection (should now be handled before param loop)
                        "H",  # Tool length offset (eg G43 H01 Z100)
                        "D",  # Cutter radius compensation (eg G41 D02)
                        "S",  # Spindle speed
                    ]:
                        outstring.append(param + str(int(c.Parameters[param])))

                    else:
                        if (
                            (not OUTPUT_DOUBLES)
                            and (param in currLocation)
                            and (currLocation[param] == c.Parameters[param])
                        ):
                            continue
                        else:
                            pos = Units.Quantity(c.Parameters[param], FreeCAD.Units.Length)
                            outstring.append(
                                param + format(float(pos.getValueAs(unit_format)), precision_string)
                            )

            # store the latest command
            lastcommand = command
            currLocation.update(c.Parameters)

            # Stitch outstring together
            if len(outstring) >= 1:
                line = (COMMAND_SPACE.join(outstring)).strip()
                out_lines.append(line)

        return out_lines
