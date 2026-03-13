# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2014 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022 luvtofish <luvtofish@gmail.com>                    *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENSE text file.                                 *
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
# *   This file has been modified from Sliptonic original LinuxCNC post     *
# *   for use with Dynapath Delta 40M,50M,60M controllers. All changes      *
# *   and modifications (c) luvtofish (luvtofish@gmail.com) 2022            *
# ***************************************************************************

import FreeCAD
from FreeCAD import Units
import Path
import argparse
import datetime
import shlex
import Path.Base.Util as PathUtil
import Path.Post.Utils as PostUtils
import PathScripts.PathUtils as PathUtils
from builtins import open as pyopen

TOOLTIP = """
This is a post processor file for the FreeCAD Path workbench. It is used to
take a pseudo-G-code fragment outputted by a Path object, and output
real G-code suitable for Dynapath Delta 40,50, & 60 Controls. It has been written
and tested on FreeCAD Path workbench bundled with FreeCAD v21.
This post processor, once placed in the appropriate PathScripts folder, can be
used directly from inside FreeCAD, via the GUI importer or via python scripts with:

import delta_4060_post
delta_4060_post.export(object,"/path/to/file.ncc","")
"""

# Preamble text will appear at the beginning of the GCODE output file.
PREAMBLE = """G17
G90
G80
G40
"""

# Postamble text will appear following the last operation.
POSTAMBLE = """M05
G80
G40
G17
G90
M30
"""

parser = argparse.ArgumentParser(prog="delta_4060", add_help=False)
parser.add_argument("--no-header", action="store_true", help="suppress header output")
parser.add_argument(
    "--no-comments", action="store_true", help="suppress comment output"
)
parser.add_argument(
    "--line-numbers", action="store_true", help="prefix with line numbers"
)
parser.add_argument(
    "--no-show-editor",
    action="store_true",
    help="don't pop up editor before writing output",
)
parser.add_argument(
    "--precision", default="3", help="number of digits of precision, default=3"
)
parser.add_argument(
    "--preamble",
    help='set commands to be issued before the first command, default="'
    + PREAMBLE.replace("\n", "\\n")
    + '"',
    default=PREAMBLE,
)
parser.add_argument(
    "--postamble",
    help='set commands to be issued after the last command, default="'
    + POSTAMBLE.replace("\n", "\\n")
    + '"',
    default=POSTAMBLE,
)
parser.add_argument(
    "--inches", action="store_true", help="Convert output for US imperial mode (G70)"
)
parser.add_argument(
    "--modal",
    action="store_true",
    help="Suppress outputting the Same G-command",
)
parser.add_argument(
    "--axis-modal",
    action="store_true",
    help="Suppress outputting identical axis position",
)

TOOLTIP_ARGS = parser.format_help()

now = datetime.datetime.now()

# These globals set common customization preferences
OUTPUT_COMMENTS = True
OUTPUT_HEADER = True
OUTPUT_LINE_NUMBERS = False
SHOW_EDITOR = True
MODAL = False  # if true commands are suppressed if the same as previous line.
OUTPUT_DOUBLES = (
    True  # if false duplicate axis values are suppressed if the same as previous line.
)
COMMAND_SPACE = ""
LINENR = 0  # Line number starting value.
DWELL_TIME = 1  # Number of seconds to allow spindle to come up to speed.
RETRACT_MODE = False
QCYCLE_RANGE = (
    "G81",
    "G82",
    "G83",
    "G84",
    "G85",
)  # Quill Cycle range for conditional to enable 2nd reference plane.
SPINDLE_SPEED = 0
# These globals will be reflected in the Machine configuration of the project
UNITS = "G71"  # G71 for metric, G70 for US standard
UNIT_SPEED_FORMAT = "mm/min"
UNIT_FORMAT = "mm"
MACHINE_NAME = "Delta_4060"
CORNER_MIN = {"x": 0, "y": 0, "z": 0}
CORNER_MAX = {"x": 660, "y": 355, "z": 152}
PRECISION = 3
ABSOLUTE_CIRCLE_CENTER = True
# Dynapath requires absolute coordinates for arcs.
# possible values:
#                  True      use absolute values for the circle center in commands G2, G3
#                  False     values for I & J are incremental from last point

# Create Map / for renaming Fixture Commands from Gxx to Exx as needed by Dynapath.
GCODE_MAP = {
    "G54": "E01",
    "G55": "E02",
    "G56": "E03",
    "G57": "E04",
    "G58": "E05",
    "G59": "E06",
}

# Create following variable for use with the 2nd reference plane.
clearanceHeight = None


def processArguments(argstring):
    global OUTPUT_HEADER
    global OUTPUT_COMMENTS
    global OUTPUT_LINE_NUMBERS
    global SHOW_EDITOR
    global PRECISION
    global PREAMBLE
    global POSTAMBLE
    global UNITS
    global UNIT_SPEED_FORMAT
    global UNIT_FORMAT
    global MODAL
    global OUTPUT_DOUBLES

    try:
        args = parser.parse_args(shlex.split(argstring))
        if args.no_header:
            OUTPUT_HEADER = False
        if args.no_comments:
            OUTPUT_COMMENTS = False
        if args.line_numbers:
            OUTPUT_LINE_NUMBERS = True
        if args.no_show_editor:
            SHOW_EDITOR = False
            print("Show editor = %r" % (SHOW_EDITOR))
        if args.preamble is not None:
            PREAMBLE = args.preamble.replace("\\n", "\n")
        if args.postamble is not None:
            POSTAMBLE = args.postamble.replace("\\n", "\n")
        if args.inches:
            UNITS = "G70"
            UNIT_SPEED_FORMAT = "in/min"
            UNIT_FORMAT = "in"
            PRECISION = 3
        if args.precision is not None:
            PRECISION = int(args.precision)
        if args.modal:
            MODAL = True
            print("Command duplicates suppressed")
        if args.axis_modal:
            OUTPUT_DOUBLES = False
            print("Doubles suppressed")

    except Exception:
        return False

    return True


def export(objectslist, filename, argstring):
    if not processArguments(argstring):
        return None
    global UNITS
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT
    global clearanceHeight

    for obj in objectslist:
        if not hasattr(obj, "Path"):
            print(
                "the object "
                + obj.Name
                + " is not a path. Please select only path and Compounds."
            )
            return None

    print("postprocessing...")
    gcode = ""

    # write header
    if OUTPUT_HEADER:
        gcode += "(%s)\n" % str.upper(obj.Document.Label[0:8])
        gcode += linenumber() + "(T)EXPORTED BY FREECAD$\n"
        gcode += linenumber() + "(T)POST PROCESSOR: " + str.upper(__name__) + "$\n"
        gcode += linenumber() + "(T)OUTPUT TIME: " + str(now) + "$\n"

        # --- THE UNIVERSAL MASTER NOTES ---
        gcode += linenumber() + "(T)DYNAPATH 4060 UNIVERSAL MASTER V1.0$\n"
        gcode += linenumber() + "(T)UNITS: G70 INCH / G71 MM BRIDGE ACTIVE$\n"
        gcode += linenumber() + "(T)G82/G84 DWELL (L) IS SECONDS$\n"

    # Write the preamble
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "(T)" + "BEGIN PREAMBLE$\n"
    for line in PREAMBLE.splitlines():
        gcode += linenumber() + line + "\n"
    gcode += linenumber() + UNITS + "\n"

    for obj in objectslist:

        # Skip inactive operations
        if not PathUtil.activeForOp(obj):
            continue

        if hasattr(obj, "ClearanceHeight"):
            clearanceHeight = obj.ClearanceHeight.Value

        # Fix fixture Offset label. Needed by Dynapath.
        if obj.Label in GCODE_MAP:
            obj.Label = GCODE_MAP[obj.Label]

        # do the pre_op. Inserts "(T)" in comment to signify textfield.
        if OUTPUT_COMMENTS:
            gcode += (
                linenumber()
                + "(T)"
                + str.upper("begin operation: " + obj.Label)
                + "$\n"
            )
            gcode += (
                linenumber()
                + "(T)"
                + str.upper("machine units: ")
                + "%s/%s$\n"
                % (str.upper(UNIT_SPEED_FORMAT[0:2]), str.upper(UNIT_SPEED_FORMAT[3:6]))
            )

        # get coolant mode
        coolantMode = PathUtil.coolantModeForOp(obj)

        # turn coolant on if required
        if OUTPUT_COMMENTS:
            if not coolantMode == "None":
                gcode += (
                    linenumber()
                    + "(T)"
                    + str.upper("Coolant On:" + coolantMode)
                    + "$\n"
                )
        if coolantMode == "Flood":
            gcode += linenumber() + "M8" + "\n"
        if coolantMode == "Mist":
            gcode += linenumber() + "M7" + "\n"

        # process the operation gcode
        gcode += parse(obj)

        # do the post_op
        if OUTPUT_COMMENTS:
            gcode += (
                linenumber()
                + "(T)"
                + str.upper("finish operation: " + obj.Label)
                + "$\n"
            )

        # turn coolant off if required
        if not coolantMode == "None":
            if OUTPUT_COMMENTS:
                gcode += (
                    linenumber()
                    + "(T)"
                    + str.upper("Coolant Off:" + coolantMode)
                    + "$\n"
                )
            gcode += linenumber() + "M9" + "\n"

    # do the post_amble
    if OUTPUT_COMMENTS:
        gcode += linenumber() + "(T)" + "BEGIN POSTAMBLE$\n"
    for line in POSTAMBLE.splitlines():
        gcode += linenumber() + line + "\n"

    # Following is required by Dynapath Controls to signify "EOF" when loading in to control
    # from external media. The control strips the "E" off as part of the load process.
    gcode += "E\n"

    if FreeCAD.GuiUp and SHOW_EDITOR:
        dia = PostUtils.GCodeEditorDialog()
        dia.editor.setText(gcode)
        if dia.exec_():
            final = dia.editor.toPlainText()
        else:
            final = gcode
    else:
        final = gcode

    # Handshake for FreeCAD 1.2dev internal preview
    if filename == "-":
        return final

    # Standard file writing
    try:
        with pyopen(filename, "w") as gfile:
            gfile.write(final)
        print("Done postprocessing.")
        # CRITICAL: Return True to clear the 'must re-save' status
        return True
    except Exception as e:
        print("Error saving: " + str(e))
        return False


# Modified for Dynapath to Include a 4 digit field that begins with an "N".
def linenumber():
    global LINENR
    if OUTPUT_LINE_NUMBERS is True:
        LINENR += 1
        return "N" + "%04d" % (LINENR)  # Added formatting for 4 digit line number.
    return ""


def parse(pathobj):
    global PRECISION
    global MODAL
    global OUTPUT_DOUBLES
    global UNIT_FORMAT
    global UNIT_SPEED_FORMAT
    global RETRACT_MODE
    global DWELL_TIME
    global clearanceHeight
    global NEEDS_XOYO_FIX
    global Z_HANDLED

    Z_HANDLED = False
    lastX = 0
    lastY = 0
    lastZ = 0
    out = ""
    lastcommand = None
    precision_string = "." + str(PRECISION) + "f"
    currLocation = {}  # keep track for no doubles

    # 1. DEFINE op_clearance AT THE VERY START
    try:
        import FreeCAD as App

        # Pull from the operation directly to avoid the "re-save" requirement
        op_clearance = float(pathobj.ClearanceHeight.Value)
    except:
        # Fallback to the global or a hardcoded default
        op_clearance = clearanceHeight if "clearanceHeight" in locals() else 0.1968

    # the order of parameters
    # Added O and L since these are needed by Dynapath.
    # "O" is used as a 2nd reference plane in canned Cycles.
    # "L" is equivalent to "P" for dwell entries.
    params = [
        "X",
        "Y",
        "Z",
        "A",
        "B",
        "C",
        "I",
        "J",
        "K",
        "F",
        "S",
        "T",
        "Q",
        "O",
        "R",
        "L",
        "P",
    ]

    firstmove = Path.Command("G0", {"X": -1, "Y": -1, "Z": -1, "F": 0.0})
    currLocation.update(firstmove.Parameters)  # set first location parameters

    if hasattr(pathobj, "Group"):  # We have a compound or project.
        if OUTPUT_COMMENTS:
            out += (
                linenumber() + "(T)" + str.upper("compound: " + pathobj.Label) + "$\n"
            )
        for p in pathobj.Group:
            out += parse(p)
        return out
    else:  # parsing simple path

        # groups might contain non-path things like stock.
        if not hasattr(pathobj, "Path"):
            return out

        for c in PathUtils.getPathWithPlacement(pathobj).Commands:
            outstring = []
            command = c.Name

            # --- THE TAPPING CLEANUP ---
            if command in ("G84", "G74"):
                # Delete S and F so the standard loop skips them
                if "S" in c.Parameters:
                    c.Parameters.pop("S")
                if "F" in c.Parameters:
                    c.Parameters.pop("F")

            # Convert G54-G59 Fixture offsets to E01-E06 for Dynapath Delta Control
            if command in GCODE_MAP:
                command = GCODE_MAP[command]

            # Inserts "(T)" in front of comments to signify textfield (Dynapath)
            if command.startswith("("):
                if OUTPUT_COMMENTS:
                    command = "(T)" + str.upper(command) + "$"

            outstring.append(command)

            # if modal: suppress the command if it is the same as the last one
            if MODAL is True:
                if command == lastcommand:
                    outstring.pop(0)

            if c.Name.startswith("(") and not OUTPUT_COMMENTS:  # command is a comment
                continue

            # Needed for Z param logic below.
            g80_buffer = ""
            outstring = [command]

            # Now add the remaining parameters in order
            for param in params:
                if param in c.Parameters:
                    if param == "F":
                        if command in ("G84", "G74"):
                            tc = pathobj.ToolController
                            # RPM * Pitch math
                            s_attr = getattr(tc, "SpindleSpeed", 100)
                            rpm = float(getattr(s_attr, "Value", s_attr))
                            p_attr = getattr(
                                tc,
                                "Pitch",
                                getattr(getattr(tc, "Tool", None), "Pitch", 0.03125),
                            )
                            pitch = float(getattr(p_attr, "Value", p_attr))
                            if UNITS == "G70" and pitch > 0.5:
                                pitch /= 25.4
                            f_val = rpm * pitch
                        else:
                            # Your original Golden Logic
                            f_raw = float(c.Parameters.get("F", 20.0))
                            f_val = float(
                                Units.Quantity(
                                    f_raw, FreeCAD.Units.Velocity
                                ).getValueAs(UNIT_SPEED_FORMAT)
                            )

                        f_out = format(f_val, ".2f")

                        # FORCE the output for Tapping
                        if (
                            command in ("G84", "G74")
                            or currLocation.get(param) != f_out
                        ):
                            outstring.append(param + f_out)
                            currLocation[param] = f_out

                    elif param == "Z":
                        z_val = float(c.Parameters["Z"])
                        z_fmt = PostUtils.fmt(z_val, PRECISION, UNITS)

                        # --- 1. THE CATCH-ALL FEEDRATE GRAB ---
                        f_raw = 20.0  # Safe fallback

                        if (
                            hasattr(pathobj, "ToolController")
                            and pathobj.ToolController is not None
                        ):
                            tc = pathobj.ToolController
                            # Search all properties for the Vertical Rapid value
                            for p in tc.PropertiesList:
                                if "Rapid" in p and ("Vert" in p or "Z" in p):
                                    val = getattr(tc, p)
                                    # Get the numerical value (Inches or MM)
                                    temp_f = (
                                        val.Value
                                        if hasattr(val, "Value")
                                        else float(val)
                                    )
                                    if temp_f > 0:
                                        f_raw = temp_f
                                        break

                        # If the search failed, use the command's F-parameter
                        if f_raw == 20.0:
                            f_raw = float(c.Parameters.get("F", 20.0))

                        # Convert and Ghost-Bust (1200mm -> 20.00)
                        speed_qty = Units.Quantity(f_raw, FreeCAD.Units.Velocity)
                        f_val = float(speed_qty.getValueAs(UNIT_SPEED_FORMAT))

                        # Catch the 1200mm default (47.24) OR any value that feels like a metric rapid
                        if abs(f_val - 47.24) < 0.1 or f_val > 100.0:
                            f_val = 25.0  # Force to your preferred safe vertical speed

                        f_out = format(f_val, ".2f")

                        # --- 2. THE SURGICAL RESET (First Move Only) ---
                        if NEEDS_XOYO_FIX:
                            NEEDS_XOYO_FIX = False
                            outstring[:] = [
                                command,
                                "X0.0000",
                                "Y0.0000",
                                "Z" + z_fmt,
                                "F" + f_out,
                            ]
                            currLocation.update({"X": 0.0, "Y": 0.0, "Z": z_val})
                            c.Parameters.clear()
                            break

                        # --- 3. THE G80 INJECTOR (Post-Drill Retract) ---
                        if lastcommand in QCYCLE_RANGE and z_val > 0:
                            g80_buffer = (linenumber()) + "G80\n"

                        # --- 4. NORMAL Z HANDLING ---
                        if abs(z_val - currLocation.get("Z", -999.0)) > 0.0001:
                            outstring.append(param + z_fmt)
                            currLocation.update({"Z": z_val})

                    elif param == "X" and (command in QCYCLE_RANGE):
                        pos = Units.Quantity(c.Parameters["X"], FreeCAD.Units.Length)
                        outstring.append(
                            param
                            + format(
                                float(pos.getValueAs(UNIT_FORMAT)), precision_string
                            )
                        )
                    elif param == "Y" and (command in QCYCLE_RANGE):
                        pos = Units.Quantity(c.Parameters["Y"], FreeCAD.Units.Length)
                        outstring.append(
                            param
                            + format(
                                float(pos.getValueAs(UNIT_FORMAT)), precision_string
                            )
                        )

                    elif param == "S":
                        SPINDLE_SPEED = c.Parameters["S"]
                        outstring.append(
                            param + "{:.0f}".format(c.Parameters["S"])
                        )  # Added formatting to strip trailing .000 from RPM (needed by dynapath)
                    elif param == "T":
                        outstring.append(
                            param + "{:.0f}".format(c.Parameters["T"])
                        )  # Added formatting to strip trailing .000 from Tool number (needed by dynapath)

                    elif param in ["I", "J", "K"] and command in [
                        "G2",
                        "G3",
                        "G02",
                        "G03",
                    ]:
                        val = float(c.Parameters[param])

                        # Add the last position to get the Absolute Center
                        if ABSOLUTE_CIRCLE_CENTER:
                            if param == "I":
                                val += lastX
                            elif param == "J":
                                val += lastY
                            elif param == "K":
                                val += lastZ

                        # I and J always post for standard G17 (XY) arcs
                        if param in ["I", "J"]:
                            outstring.append(
                                param + PostUtils.fmt(val, PRECISION, UNITS)
                            )

                        # K ONLY posts if we are in G18 or G19 (Vertical Arcs)
                        elif param == "K" and command in ["G18", "G19"]:
                            outstring.append(
                                param + PostUtils.fmt(val, PRECISION, UNITS)
                            )

                    # Converts "Q" to "K" as needed by Dynapath.
                    elif param == "Q":
                        pos = Units.Quantity(c.Parameters["Q"], FreeCAD.Units.Length)
                        outstring.append(
                            "K"
                            + format(
                                float(pos.getValueAs(UNIT_FORMAT)), precision_string
                            )
                        )
                    # Following inserts a 2nd reference plane in all canned cycles (dynapath).
                    # This provides the ability to manually go in and bump up the "O" offset in
                    # order to avoid obstacles. The "O" overrides "R", so set them both equal if you
                    # don't need the 2nd reference plane.

                    elif (param == "R") and (command in QCYCLE_RANGE):
                        # --- 1. CLEARANCE HEIGHT (O) ---
                        o_val = float(pathobj.ClearanceHeight.Value)
                        if UNITS == "G70" and o_val > 0.5:  # Lower threshold for 1.2dev
                            o_val /= 25.4
                        outstring.insert(6, "O" + format(o_val, precision_string))

                        # --- 2. THE R-VALUE TRAP (1.0mm = 0.0394") ---
                        r_raw = float(c.Parameters.get("R", 0.0))

                        # If R is 1.0 (exactly 1mm) or 0.0 (missing), hunt for the truth
                        if r_raw == 0.0 or abs(r_raw - 1.0) < 0.001:
                            tc = pathobj.ToolController
                            r_attr = getattr(
                                tc, "SafeHeight", getattr(tc, "RetractHeight", 1.0)
                            )
                            r_raw = float(getattr(r_attr, "Value", r_attr))

                        # THE UNIT SCALER: If we are in G70 and value is 0.5 or higher,
                        # it's likely a metric value (1mm, 3.81mm, etc.)
                        if UNITS == "G70" and r_raw > 0.5:
                            r_raw /= 25.4

                        outstring.append("R" + format(r_raw, precision_string))

                    elif param == "P":
                        outstring.append(
                            "L" + format(c.Parameters[param], ".1f")
                        )  # Converts "P" to "L" for dynapath.
                    else:
                        if (
                            (not OUTPUT_DOUBLES)
                            and (param in currLocation)
                            and (currLocation[param] == c.Parameters[param])
                        ):
                            continue
                        else:
                            pos = Units.Quantity(
                                c.Parameters[param], FreeCAD.Units.Length
                            )
                            outstring.append(
                                param
                                + format(
                                    float(pos.getValueAs(UNIT_FORMAT)), precision_string
                                )
                            )
            # save the last X, Y values
            if "X" in c.Parameters:
                lastX = c.Parameters["X"]
            if "Y" in c.Parameters:
                lastY = c.Parameters["Y"]
            if "Z" in c.Parameters:
                lastZ = c.Parameters["Z"]

            # store the latest command
            lastcommand = command
            currLocation.update(c.Parameters)

            # Check for Tool Change:
            if command == "M6":
                # Inject a M5 line before the tool change starts
                out += (linenumber()) + "M05\n"

                NEEDS_XOYO_FIX = True
                if OUTPUT_COMMENTS:
                    out += linenumber() + "(T)" + "BEGIN TOOLCHANGE$\n"

            if command == "message":
                if OUTPUT_COMMENTS is False:
                    out = []
                else:
                    outstring.pop(0)  # remove the command
            # G98 is not used by dynapath and G99 is not used in Drilling/Boring/Tapping.
            if c.Name == "G98" or (c.Name == "G99" and pathobj.Label == "Drilling"):
                outstring = []

            # If we have a G80 in the buffer, write it as a standalone line now
            if g80_buffer:
                out += g80_buffer
                # Reset the buffer so it doesn't repeat
                g80_buffer = ""

            # --- THE GOLDEN TAPPING FEED INJECTOR ---
            if command in ("G84", "G74"):
                tc = pathobj.ToolController
                # 1. Get RPM and Pitch (Bridge 1.1rc3 and 1.2dev)
                s_attr = getattr(tc, "SpindleSpeed", 100)
                rpm = float(getattr(s_attr, "Value", s_attr))

                # Hunt for Pitch or Stepover inside the Tool object
                p_attr = getattr(
                    tc, "Pitch", getattr(getattr(tc, "Tool", None), "Pitch", 0.03125)
                )
                pitch = float(getattr(p_attr, "Value", p_attr))

                # 2. Metric-to-Inch Safety
                if UNITS == "G70" and pitch > 0.5:
                    pitch /= 25.4

                # 3. Calculate and Append Feedrate to the END of the line
                f_val = rpm * pitch
                f_out = format(f_val, ".2f")
                outstring.append("F" + f_out)

                # Update brain so the retract move knows the previous speed
                currLocation["F"] = f_out

            # If it's just a G-code and an F-code with no X, Y, or Z, skip it.
            has_move = any(
                item.startswith(("X", "Y", "Z", "O", "R", "L", "K"))
                for item in outstring
            )
            if (
                not has_move
                and any(item.startswith("G") for item in outstring)
                and any(item.startswith("F") for item in outstring)
            ):
                outstring = []

            # prepend a line number and append a newline
            if len(outstring) > 1 or any(
                item.startswith(("E", "G80")) for item in outstring
            ):
                if OUTPUT_LINE_NUMBERS:
                    outstring.insert(0, (linenumber()))

                # append the line to the final output
                for w in outstring:
                    out += w.strip() + COMMAND_SPACE
                out += "\n"

            # Add a DWELL after spindle start based on RPM.
            # Used by Dynapath for implementing a scalable dwell time.
            if DWELL_TIME > 0.0:
                if command in ["M3", "M03", "M4", "M04"]:
                    DWELL_TIME = (SPINDLE_SPEED / 100) / 10
                    out += linenumber() + "L%.1f\n" % DWELL_TIME

        return out
