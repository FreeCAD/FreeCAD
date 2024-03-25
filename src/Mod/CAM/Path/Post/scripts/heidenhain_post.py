# -*- coding: UTF-8 -*-
# ***************************************************************************
# *   Copyright (C) 2020 Stefano Chiaro <stefano.chiaro@yahoo.com>          *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Lesser General Public            *
# *   License as published by the Free Software Foundation; either          *
# *   version 2.1 of the License, or (at your option) any later version.    *
# *                                                                         *
# *   This library is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with this library; if not, write to the Free Software   *
# *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
# *   02110-1301  USA                                                       *
# ***************************************************************************

# HEDENHAIN Post-Processor for FreeCAD

import argparse
import Path.Post.Utils as PostUtils
import PathScripts.PathUtils as PathUtils
import Path
import PathScripts
import shlex
import math

# **************************************************************************#
#                   USER  EDITABLE  STUFF  HERE                            #
#                                                                          #
#     THESE VALUES SHOULD BE CHANGED TO FIT MACHINE TYPE AND LANGUAGE      #

MACHINE_SKIP_PARAMS = False  # Print R F M values
# possible values:
#                    'True'    Skip to print a parameter if already active
#                    'False'   Print parameter on ALL line
# Old machines need these values on every lines

MACHINE_USE_FMAX = False  # Usage of FMAX
# possible values:
#                 'True'   Print FMAX
#                 'False'  Print the value set on FEED_MAX_SPEED
# Old machines don't accept FMAX and need a feed value

FEED_MAX_SPEED = 8000  # Max machine speed for FMAX
# possible values:
# integer >= 0

AXIS_DECIMALS = 3  # machine axis precision
# possible values:
# integer >= 0

FEED_DECIMALS = 0  # machine feed precision
# possible values:
# integer >= 0

SPINDLE_DECIMALS = 0  # machine spindle precision
# possible values:
# integer >= 0

FIRST_LBL = 1  # first LBL number for LBLIZE function
# possible values:
# integer >= 0

#                   TEMPLATES FOR CYCLE DEFINITION                         #

MACHINE_CYCLE_DEF = {
    1: "CYCL DEF 1.0 FORATURA PROF."
    + "\n"
    + "CYCL DEF 1.1 DIST"
    + "{DIST}\n"
    + "CYCL DEF 1.2 PROF"
    + "{DEPTH}\n"
    + "CYCL DEF 1.3 INCR"
    + "{INCR}\n"
    + "CYCL DEF 1.4 SOSTA"
    + "{DWELL}\n"
    + "CYCL DEF 1.5 F"
    + "{FEED}",
    2: "",
    7: "",
}

#                      OPTIONAL COMPUTING                                  #

SOLVE_COMPENSATION_ACTIVE = False  # Use the internal path compensation
# possible values:
#                          'True'   Try to solve compensation
#                          'False'  Use original FreeCAD path
# try to use RL and RR to get the real path where possible

SHOW_EDITOR = True  # Open the editor
# possible values:
#            'True'   before the file is written it is shown for inspection
#            'False'  the file is written directly

SKIP_WARNS = False  # Skip post-processor warnings
# possible values:
#            'True'   never prompt warning from post-processing problems
#            'False'  prompt active

#            STANDARD HEIDENHAIN VALUES FOR POSTPROCESSOR                  #

UNITS = "MM"  # post-processor units
# possible values:
#       'INCH'  for inches
#       'MM'    for metric units
# actually FreeCAD use only metric values

LINENUMBERS = True  # line numbers
# possible values:
#            'True'   Add line numbers (Standard)
#            'False'  No line numbers

STARTLINENR = 0  # first line number used
# possible values:
# any integer value >= 0 (Standard is 0)

LINENUMBER_INCREMENT = 1  # line number increment
# possible values:
# any integer value > 0 (Standard is 1)

#                POSTPROCESSOR VARIABLES AND CODE                          #

# **************************************************************************#
#  don't edit the stuff below this line unless you know what you're doing  #
# **************************************************************************#

#                   GCODE VARIABLES AND FUNCTIONS                          #

POSTGCODE = []  # Output string array
G_FUNCTION_STORE = {
    "G90": False,
    "G91": False,
    "G98": False,
    "G99": False,
    "G81": False,
    "G82": False,
    "G83": False,
}

#                     HEIDENHAIN MACHINE PARAMETERS                        #

MACHINE_WORK_AXIS = 2  # 0=X ; 1=Y ; 2=Z usually Z
MACHINE_SPINDLE_DIRECTION = 3  # CW = 3 ; CCW = 4
MACHINE_LAST_POSITION = {  # axis initial values to overwrite
    "X": 99999,
    "Y": 99999,
    "Z": 99999,
    "A": 99999,
    "B": 99999,
    "C": 99999,
}
MACHINE_LAST_CENTER = {  # CC initial values to overwrite
    "X": 99999,
    "Y": 99999,
    "Z": 99999,
}
MACHINE_TRASL_ROT = [0, 0, 0, 0]  # [X, Y, Z , Angle] !not implemented
MACHINE_STORED_PARAMS = ["", -1, ""]  # Store R F M parameter to skip

#                  POSTPROCESSOR VARIABLES STORAGE                         #

STORED_COMPENSATED_OBJ = ()  # Store a copy of compensated path
STORED_CANNED_PARAMS = {  # Store canned cycles for match
    "DIST": 99999,
    "DEPTH": 99999,
    "INCR": 99999,
    "DWELL": 99999,
    "FEED": 99999,
}
STORED_LBL = []  # Store array of LBL for match

#                  POSTPROCESSOR SPECIAL VARIABLES                         #

COMPENSATION_DIFF_STATUS = [False, True]  # Check compensation, Check Diff
LBLIZE_ACTIVE = False  # Check if search for LBL
LBLIZE_STAUS = False  # Activated by path type
LBLIZE_PATH_LEVEL = 99999  # Save milling level of actual LBL

#                   END OF POSTPROCESSOR VARIABLES                         #
# **************************************************************************#

TOOLTIP = """
This is a postprocessor file for the Path workbench. It is used to
take a pseudo-gcode fragment outputted by a Path object, and output
real GCode suitable for a heidenhain 3 axis mill. This postprocessor, once placed
in the appropriate Path/Tool folder, can be used directly from inside
FreeCAD, via the GUI importer or via python scripts with:

import heidenhain_post
heidenhain.export(object,"/path/to/file.ncc","")
"""

parser = argparse.ArgumentParser(prog="heidenhain", add_help=False)
parser.add_argument(
    "--skip-params",
    action="store_true",
    help="suppress R F M parameters where already stored",
)
parser.add_argument(
    "--use-fmax",
    action="store_true",
    help="suppress feedrate and use FMAX instead with rapid movements",
)
parser.add_argument(
    "--fmax-value", default="8000", help="feedrate to use instead of FMAX, default=8000"
)
parser.add_argument(
    "--axis-decimals", default="3", help="number of digits of axis precision, default=3"
)
parser.add_argument(
    "--feed-decimals",
    default="0",
    help="number of digits of feedrate precision, default=0",
)
parser.add_argument(
    "--spindle-decimals",
    default="0",
    help="number of digits of spindle precision, default=0",
)
parser.add_argument(
    "--solve-comp",
    action="store_true",
    help="try to get RL or RR real path where compensation is active",
)
parser.add_argument(
    "--solve-lbl",
    action="store_true",
    help="try to replace repetitive movements with LBL",
)
parser.add_argument(
    "--first-lbl", default="1", help="change the first LBL number, default=1"
)
parser.add_argument(
    "--no-show-editor",
    action="store_true",
    help="don't pop up editor before writing output",
)
parser.add_argument(
    "--no-warns", action="store_true", help="don't pop up post-processor warnings"
)

TOOLTIP_ARGS = parser.format_help()

if open.__module__ in ["__builtin__", "io"]:
    pythonopen = open


def processArguments(argstring):
    global MACHINE_SKIP_PARAMS
    global MACHINE_USE_FMAX
    global FEED_MAX_SPEED
    global AXIS_DECIMALS
    global FEED_DECIMALS
    global SPINDLE_DECIMALS
    global SOLVE_COMPENSATION_ACTIVE
    global LBLIZE_ACTIVE
    global FIRST_LBL
    global SHOW_EDITOR
    global SKIP_WARNS

    try:
        args = parser.parse_args(shlex.split(argstring))
        if args.skip_params:
            MACHINE_SKIP_PARAMS = True
        if args.use_fmax:
            MACHINE_USE_FMAX = True
        if args.fmax_value:
            FEED_MAX_SPEED = float(args.fmax_value)
        if args.axis_decimals:
            AXIS_DECIMALS = int(args.axis_decimals)
        if args.feed_decimals:
            FEED_DECIMALS = int(args.feed_decimals)
        if args.spindle_decimals:
            SPINDLE_DECIMALS = int(args.spindle_decimals)
        if args.solve_comp:
            SOLVE_COMPENSATION_ACTIVE = True
        if args.solve_lbl:
            LBLIZE_ACTIVE = True
        if args.first_lbl:
            FIRST_LBL = int(args.first_lbl)
        if args.no_show_editor:
            SHOW_EDITOR = False
        if args.no_warns:
            SKIP_WARNS = True
    except Exception:
        return False

    return True


def export(objectslist, filename, argstring):
    if not processArguments(argstring):
        return None
    global UNITS
    global POSTGCODE
    global G_FUNCTION_STORE
    global MACHINE_WORK_AXIS
    global MACHINE_LAST_POSITION
    global MACHINE_TRASL_ROT
    global STORED_COMPENSATED_OBJ
    global COMPENSATION_DIFF_STATUS
    global LBLIZE_STAUS

    Object_Kind = None
    Feed = 0
    Spindle_Active = False
    Compensation = "0"
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
        "H",
        "S",
        "T",
        "Q",
        "R",
        "L",
    ]

    for obj in objectslist:
        if not hasattr(obj, "Path"):
            print(
                "the object "
                + obj.Name
                + " is not a path. Please select only path and Compounds."
            )
            return

    POSTGCODE.append(HEIDEN_Begin(objectslist))  # add header

    for obj in objectslist:
        Cmd_Count = 0  # command line number
        LBLIZE_STAUS = False

        if not hasattr(obj, "Proxy"):
            continue
        # useful to get idea of object kind
        if isinstance(obj.Proxy, Path.Tool.Controller.ToolController):
            Object_Kind = "TOOL"
            # like we go to change tool position
            MACHINE_LAST_POSITION["X"] = 99999
            MACHINE_LAST_POSITION["Y"] = 99999
            MACHINE_LAST_POSITION["Z"] = 99999
        elif isinstance(obj.Proxy, Path.Op.ProfileEdges.ObjectProfile):
            Object_Kind = "PROFILE"
            if LBLIZE_ACTIVE:
                LBLIZE_STAUS = True
        elif isinstance(obj.Proxy, Path.Op.MillFace.ObjectFace):
            Object_Kind = "FACE"
            if LBLIZE_ACTIVE:
                LBLIZE_STAUS = True
        elif isinstance(obj.Proxy, Path.Op.Helix.ObjectHelix):
            Object_Kind = "HELIX"

        commands = PathUtils.getPathWithPlacement(obj).Commands

        # If used compensated path, store, recompute and diff when asked
        if not hasattr(obj, "UseComp") or not SOLVE_COMPENSATION_ACTIVE:
            continue

        if not obj.UseComp:
            continue

        if hasattr(obj.Path, "Commands") and Object_Kind == "PROFILE":
            # Take a copy of compensated path
            STORED_COMPENSATED_OBJ = commands
        # Find mill compensation
        if hasattr(obj, "Side") and hasattr(obj, "Direction"):
            if obj.Side == "Outside" and obj.Direction == "CW":
                Compensation = "L"
            elif obj.Side == "Outside" and obj.Direction == "CCW":
                Compensation = "R"
            elif obj.Side != "Outside" and obj.Direction == "CW":
                Compensation = "R"
            else:
                Compensation = "L"
            # set obj.UseComp to false and recompute() to get uncompensated path
            obj.UseComp = False
            obj.recompute()
            commands = PathUtils.getPathWithPlacement(obj).Commands
            # small edges could be skipped and movements joints can add edges
            NameStr = ""
            if hasattr(obj, "Label"):
                NameStr = str(obj.Label)
            if len(commands) != len(STORED_COMPENSATED_OBJ):
                # not same number of edges
                obj.UseComp = True
                obj.recompute()
                commands = PathUtils.getPathWithPlacement(obj).Commands
                POSTGCODE.append("; MISSING EDGES UNABLE TO GET COMPENSATION")
                if not SKIP_WARNS:
                    (
                        PostUtils.editor(
                            "--solve-comp command ACTIVE\n\n"
                            + "UNABLE to solve "
                            + NameStr
                            + " compensation\n\n"
                            + "Some edges are missing\n"
                            + "try to change Join Type to Miter or Square\n"
                            + "try to use a smaller Tool Diameter\n"
                            + "Internal Path could have too small corners\n\n"
                            + "use --no-warns to not prompt this message"
                        )
                    )
            else:
                if not SKIP_WARNS:
                    (
                        PostUtils.editor(
                            "--solve-comp command ACTIVE\n\n"
                            + "BE CAREFUL with solved "
                            + NameStr
                            + " compensation\n\n"
                            + "USE AT YOUR OWN RISK\n"
                            + "Simulate it before use\n"
                            + "Offset Extra ignored use DR+ on TOOL CALL\n"
                            + "Path could be different and/or give tool radius errors\n\n"
                            + "use --no-warns to not prompt this message"
                        )
                    )
                # we can try to solve compensation
                POSTGCODE.append("; COMPENSATION ACTIVE")
                COMPENSATION_DIFF_STATUS[0] = True

        for c in commands:
            Cmd_Count += 1
            command = c.Name
            if command != "G0":
                command = command.replace("G0", "G")  # normalize: G01 -> G1

            for param in params:
                if param in c.Parameters:
                    if param == "F":
                        Feed = c.Parameters["F"]

            if command == "G90":
                G_FUNCTION_STORE["G90"] = True
                G_FUNCTION_STORE["G91"] = False

            if command == "G91":
                G_FUNCTION_STORE["G91"] = True
                G_FUNCTION_STORE["G90"] = False

            if command == "G98":
                G_FUNCTION_STORE["G98"] = True
                G_FUNCTION_STORE["G99"] = False

            if command == "G99":
                G_FUNCTION_STORE["G99"] = True
                G_FUNCTION_STORE["G98"] = False

            # Rapid movement
            if command == "G0":
                Spindle_Status = ""
                if (
                    Spindle_Active == False
                ):  # At first rapid movement we turn on spindle
                    Spindle_Status += str(MACHINE_SPINDLE_DIRECTION)  # Activate spindle
                    Spindle_Active = True
                else:  # At last rapid movement we turn off spindle
                    if Cmd_Count == len(commands):
                        Spindle_Status += "5"  # Deactivate spindle
                        Spindle_Active = False
                    else:
                        Spindle_Status += ""  # Spindle still active
                parsedElem = HEIDEN_Line(
                    c.Parameters, Compensation, Feed, True, Spindle_Status, Cmd_Count
                )
                if parsedElem is not None:
                    POSTGCODE.append(parsedElem)

            # Linear movement
            if command == "G1":
                parsedElem = HEIDEN_Line(
                    c.Parameters, Compensation, Feed, False, "", Cmd_Count
                )
                if parsedElem is not None:
                    POSTGCODE.append(parsedElem)

            # Arc movement
            if command == "G2" or command == "G3":
                parsedElem = HEIDEN_Arc(
                    c.Parameters, command, Compensation, Feed, False, "", Cmd_Count
                )
                if parsedElem is not None:
                    POSTGCODE.extend(parsedElem)

            if command == "G80":  # Reset Canned Cycles
                G_FUNCTION_STORE["G81"] = False
                G_FUNCTION_STORE["G82"] = False
                G_FUNCTION_STORE["G83"] = False

            # Drilling, Dwell Drilling, Peck Drilling
            if command == "G81" or command == "G82" or command == "G83":
                parsedElem = HEIDEN_Drill(obj, c.Parameters, command, Feed)
                if parsedElem is not None:
                    POSTGCODE.extend(parsedElem)

            # Tool change
            if command == "M6":
                parsedElem = HEIDEN_ToolCall(obj)
                if parsedElem is not None:
                    POSTGCODE.append(parsedElem)

        if COMPENSATION_DIFF_STATUS[0]:  # Restore the compensation if removed
            obj.UseComp = True
            obj.recompute()
            COMPENSATION_DIFF_STATUS[0] = False

        if LBLIZE_STAUS:
            HEIDEN_LBL_Replace()
            LBLIZE_STAUS = False

    if LBLIZE_ACTIVE:
        if not SKIP_WARNS:
            (
                PostUtils.editor(
                    "--solve-lbl command ACTIVE\n\n"
                    + "BE CAREFUL with LBL replacements\n\n"
                    + "USE AT YOUR OWN RISK\n"
                    + "Simulate it before use\n"
                    + "Path could be different and/or give errors\n\n"
                    + "use --no-warns to not prompt this message"
                )
            )
    POSTGCODE.append(HEIDEN_End(objectslist))  # add footer
    Program_Out = HEIDEN_Numberize(POSTGCODE)  # add line number

    if SHOW_EDITOR:
        PostUtils.editor(Program_Out)

    gfile = pythonopen(filename, "w")
    gfile.write(Program_Out)
    gfile.close()


def HEIDEN_Begin(ActualJob):  # use Label for program name
    global UNITS
    # JobParent = PathUtils.findParentJob(ActualJob[0])
    # if hasattr(JobParent, "Label"):
    #     program_id = JobParent.Label
    # else:
    #     program_id = "NEW"
    return "BEGIN PGM {}".format(UNITS)


def HEIDEN_End(ActualJob):  # use Label for program name
    global UNITS
    # JobParent = PathUtils.findParentJob(ActualJob[0])
    # if hasattr(JobParent, "Label"):
    #     program_id = JobParent.Label
    # else:
    #     program_id = "NEW"
    return "END PGM {}".format(UNITS)


# def HEIDEN_ToolDef(tool_id, tool_length, tool_radius): # old machines don't have tool table, need tooldef list
#    return "TOOL DEF  " + tool_id + " R" + "{:.3f}".format(tool_length) + " L" + "{:.3f}".format(tool_radius)


def HEIDEN_ToolCall(tool_Params):
    global MACHINE_SPINDLE_DIRECTION
    global MACHINE_WORK_AXIS
    H_Tool_Axis = ["X", "Y", "Z"]
    H_Tool_ID = "0"
    H_Tool_Speed = 0
    H_Tool_Comment = ""

    if hasattr(tool_Params, "Label"):  # use Label as tool comment
        H_Tool_Comment = tool_Params.Label
    if hasattr(tool_Params, "SpindleDir"):  # get spindle direction for this tool
        if tool_Params.SpindleDir == "Forward":
            MACHINE_SPINDLE_DIRECTION = 3
        else:
            MACHINE_SPINDLE_DIRECTION = 4
    if hasattr(tool_Params, "SpindleSpeed"):  # get tool speed for spindle
        H_Tool_Speed = tool_Params.SpindleSpeed
    if hasattr(tool_Params, "ToolNumber"):  # use ToolNumber for tool id
        H_Tool_ID = tool_Params.ToolNumber

    if H_Tool_ID == "0" and H_Tool_Speed == 0 and H_Tool_Comment == "":
        return None
    else:
        return (
            "TOOL CALL "
            + str(H_Tool_ID)
            + " "
            + H_Tool_Axis[MACHINE_WORK_AXIS]
            + HEIDEN_Format(" S", H_Tool_Speed)
            + " ;"
            + str(H_Tool_Comment)
        )


# create a linear movement
def HEIDEN_Line(
    line_Params, line_comp, line_feed, line_rapid, line_M_funct, Cmd_Number
):
    global FEED_MAX_SPEED
    global COMPENSATION_DIFF_STATUS
    global G_FUNCTION_STORE
    global MACHINE_WORK_AXIS
    global MACHINE_LAST_POSITION
    global MACHINE_STORED_PARAMS
    global MACHINE_SKIP_PARAMS
    global MACHINE_USE_FMAX
    H_Line_New = {  # axis initial values to overwrite
        "X": 99999,
        "Y": 99999,
        "Z": 99999,
        "A": 99999,
        "B": 99999,
        "C": 99999,
    }
    H_Line = "L"
    H_Line_Params = [["X", "Y", "Z"], ["R0", line_feed, line_M_funct]]

    # check and hide duplicated axis movements, not last, update with new ones
    for i in H_Line_New:
        if G_FUNCTION_STORE["G91"]:  # incremental
            H_Line_New[i] = 0
            if i in line_Params:
                if line_Params[i] != 0 or line_M_funct != "":
                    H_Line += " I" + HEIDEN_Format(
                        i, line_Params[i]
                    )  # print incremental
            # update to absolute position
            H_Line_New[i] = MACHINE_LAST_POSITION[i] + H_Line_New[i]
        else:  # absolute
            H_Line_New[i] = MACHINE_LAST_POSITION[i]
            if i in line_Params:
                if line_Params[i] != H_Line_New[i] or line_M_funct != "":
                    H_Line += " " + HEIDEN_Format(i, line_Params[i])
                    H_Line_New[i] = line_Params[i]

    if H_Line == "L":  # No movements no line
        return None

    if COMPENSATION_DIFF_STATUS[0]:  # Diff from compensated ad not compensated path
        if COMPENSATION_DIFF_STATUS[
            1
        ]:  # skip if already compensated, not active by now
            Cmd_Number -= 1  # align
            # initialize like true, set false if not same point compensated and not compensated
            i = True
            for j in H_Line_Params[0]:
                if (
                    j in STORED_COMPENSATED_OBJ[Cmd_Number].Parameters
                    and j in line_Params
                ):
                    if (
                        STORED_COMPENSATED_OBJ[Cmd_Number].Parameters[j]
                        != line_Params[j]
                    ):
                        i = False
            if i == False:
                H_Line_Params[1][0] = "R" + line_comp
        #                   we can skip this control if already in compensation
        #                   COMPENSATION_DIFF_STATUS[1] = False
        else:
            H_Line_Params[1][0] = "R" + line_comp  # not used by now

    # check if we need to skip already active parameters
    # R parameter
    if MACHINE_SKIP_PARAMS == False or H_Line_Params[1][0] != MACHINE_STORED_PARAMS[0]:
        MACHINE_STORED_PARAMS[0] = H_Line_Params[1][0]
        H_Line += " " + H_Line_Params[1][0]

    # F parameter (check rapid o feed)
    if line_rapid:
        H_Line_Params[1][1] = FEED_MAX_SPEED
    if MACHINE_USE_FMAX and line_rapid:
        H_Line += " FMAX"
    else:
        if (
            MACHINE_SKIP_PARAMS == False
            or H_Line_Params[1][1] != MACHINE_STORED_PARAMS[1]
        ):
            MACHINE_STORED_PARAMS[1] = H_Line_Params[1][1]
            H_Line += HEIDEN_Format(" F", H_Line_Params[1][1])

    # M parameter
    if MACHINE_SKIP_PARAMS == False or H_Line_Params[1][2] != MACHINE_STORED_PARAMS[2]:
        MACHINE_STORED_PARAMS[2] = H_Line_Params[1][2]
        H_Line += " M" + H_Line_Params[1][2]

    # LBLIZE check and array creation
    if LBLIZE_STAUS:
        i = H_Line_Params[0][MACHINE_WORK_AXIS]
        # to skip reposition movements rapid or not
        if MACHINE_LAST_POSITION[i] == H_Line_New[i] and line_rapid == False:
            HEIDEN_LBL_Get(MACHINE_LAST_POSITION, H_Line_New[i])
        else:
            HEIDEN_LBL_Get()

    # update machine position with new values
    for i in H_Line_New:
        MACHINE_LAST_POSITION[i] = H_Line_New[i]

    return H_Line


# create a arc movement
def HEIDEN_Arc(
    arc_Params, arc_direction, arc_comp, arc_feed, arc_rapid, arc_M_funct, Cmd_Number
):
    global FEED_MAX_SPEED
    global COMPENSATION_DIFF_STATUS
    global G_FUNCTION_STORE
    global MACHINE_WORK_AXIS
    global MACHINE_LAST_POSITION
    global MACHINE_LAST_CENTER
    global MACHINE_STORED_PARAMS
    global MACHINE_SKIP_PARAMS
    global MACHINE_USE_FMAX
    Cmd_Number -= 1
    H_ArcSameCenter = False
    H_ArcIncr = ""
    H_ArcCenter = "CC "
    H_ArcPoint = "C"
    H_Arc_Params = [["X", "Y", "Z"], ["R0", arc_feed, arc_M_funct], ["I", "J", "K"]]
    H_Arc_CC = {"X": 99999, "Y": 99999, "Z": 99999}  # CC initial values to overwrite
    H_Arc_P_NEW = {  # end point initial values to overwrite
        "X": 99999,
        "Y": 99999,
        "Z": 99999,
    }

    # get command values
    if G_FUNCTION_STORE["G91"]:  # incremental
        H_ArcIncr = "I"
        for i in range(0, 3):
            a = H_Arc_Params[0][i]
            b = H_Arc_Params[2][i]
            # X Y Z
            if a in arc_Params:
                H_Arc_P_NEW[a] = arc_Params[a]
            else:
                H_Arc_P_NEW[a] = 0
            # I J K skip update for machine work axis
            if i != MACHINE_WORK_AXIS:
                if b in arc_Params:
                    H_Arc_CC[a] = arc_Params[b]
                else:
                    H_Arc_CC[a] = 0
    else:  # absolute
        for i in range(0, 3):
            a = H_Arc_Params[0][i]
            b = H_Arc_Params[2][i]
            # X Y Z
            H_Arc_P_NEW[a] = MACHINE_LAST_POSITION[a]
            if a in arc_Params:
                H_Arc_P_NEW[a] = arc_Params[a]
            # I J K skip update for machine work axis
            if i != MACHINE_WORK_AXIS:
                H_Arc_CC[a] = MACHINE_LAST_POSITION[a]
                if b in arc_Params:
                    # to change if I J K are not always incremental
                    H_Arc_CC[a] = H_Arc_CC[a] + arc_Params[b]

    def Axis_Select(a, b, c, incr):
        if a in arc_Params and b in arc_Params:
            _H_ArcCenter = (
                incr
                + HEIDEN_Format(a, H_Arc_CC[a])
                + " "
                + incr
                + HEIDEN_Format(b, H_Arc_CC[b])
            )
            if c in arc_Params and arc_Params[c] != MACHINE_LAST_POSITION[c]:
                # if there are 3 axis movements it need to be polar arc
                _H_ArcPoint = HEIDEN_PolarArc(
                    H_Arc_CC[a],
                    H_Arc_CC[b],
                    H_Arc_P_NEW[a],
                    H_Arc_P_NEW[b],
                    arc_Params[c],
                    c,
                    incr,
                )
            else:
                _H_ArcPoint = (
                    " "
                    + incr
                    + HEIDEN_Format(a, H_Arc_P_NEW[a])
                    + " "
                    + incr
                    + HEIDEN_Format(b, H_Arc_P_NEW[b])
                )
            return [_H_ArcCenter, _H_ArcPoint]
        else:
            return ["", ""]

    # set the right work plane based on tool direction
    if MACHINE_WORK_AXIS == 0:  # tool on X axis
        Axis_Result = Axis_Select("Y", "Z", "X", H_ArcIncr)
    elif MACHINE_WORK_AXIS == 1:  # tool on Y axis
        Axis_Result = Axis_Select("X", "Z", "Y", H_ArcIncr)
    elif MACHINE_WORK_AXIS == 2:  # tool on Z axis
        Axis_Result = Axis_Select("X", "Y", "Z", H_ArcIncr)
    # and fill with values
    H_ArcCenter += Axis_Result[0]
    H_ArcPoint += Axis_Result[1]

    if H_ArcCenter == "CC ":  # No movements no circle
        return None

    if arc_direction == "G2":  # set the right arc direction
        H_ArcPoint += " DR-"
    else:
        H_ArcPoint += " DR+"

    if COMPENSATION_DIFF_STATUS[0]:  # Diff from compensated ad not compensated path
        if COMPENSATION_DIFF_STATUS[1]:  # skip if already compensated
            Cmd_Number -= 1  # align
            i = True
            for j in H_Arc_Params[0]:
                if (
                    j in STORED_COMPENSATED_OBJ[Cmd_Number].Parameters
                    and j in arc_Params
                ):
                    if (
                        STORED_COMPENSATED_OBJ[Cmd_Number].Parameters[j]
                        != arc_Params[j]
                    ):
                        i = False
            if i == False:
                H_Arc_Params[1][0] = "R" + arc_comp
        # COMPENSATION_DIFF_STATUS[1] = False # we can skip this control if already in compensation
        else:
            H_Arc_Params[1][0] = "R" + arc_comp  # not used by now

    # check if we need to skip already active parameters

    # R parameter
    if MACHINE_SKIP_PARAMS == False or H_Arc_Params[1][0] != MACHINE_STORED_PARAMS[0]:
        MACHINE_STORED_PARAMS[0] = H_Arc_Params[1][0]
        H_ArcPoint += " " + H_Arc_Params[1][0]

    # F parameter
    if arc_rapid:
        H_Arc_Params[1][1] = FEED_MAX_SPEED
    if MACHINE_USE_FMAX and arc_rapid:
        H_ArcPoint += " FMAX"
    else:
        if (
            MACHINE_SKIP_PARAMS == False
            or H_Arc_Params[1][1] != MACHINE_STORED_PARAMS[1]
        ):
            MACHINE_STORED_PARAMS[1] = H_Arc_Params[1][1]
            H_ArcPoint += HEIDEN_Format(" F", H_Arc_Params[1][1])

    # M parameter
    if MACHINE_SKIP_PARAMS == False or H_Arc_Params[1][2] != MACHINE_STORED_PARAMS[2]:
        MACHINE_STORED_PARAMS[2] = H_Arc_Params[1][2]
        H_ArcPoint += " M" + H_Arc_Params[1][2]

    # update values to absolute if are incremental before store
    if G_FUNCTION_STORE["G91"]:  # incremental
        for i in H_Arc_Params[0]:
            H_Arc_P_NEW[i] = MACHINE_LAST_POSITION[i] + H_Arc_P_NEW[i]
            H_Arc_CC[i] = MACHINE_LAST_POSITION[i] + H_Arc_CC[i]

    # check if we can skip CC print
    if (
        MACHINE_LAST_CENTER["X"] == H_Arc_CC["X"]
        and MACHINE_LAST_CENTER["Y"] == H_Arc_CC["Y"]
        and MACHINE_LAST_CENTER["Z"] == H_Arc_CC["Z"]
    ):
        H_ArcSameCenter = True

    # LBLIZE check and array creation
    if LBLIZE_STAUS:
        i = H_Arc_Params[0][MACHINE_WORK_AXIS]
        # to skip reposition movements
        if MACHINE_LAST_POSITION[i] == H_Arc_P_NEW[i]:
            if H_ArcSameCenter:
                HEIDEN_LBL_Get(MACHINE_LAST_POSITION, H_Arc_P_NEW[i])
            else:
                HEIDEN_LBL_Get(MACHINE_LAST_POSITION, H_Arc_P_NEW[i], 1)
        else:
            HEIDEN_LBL_Get()

    # update machine position with new values
    for i in H_Arc_Params[0]:
        MACHINE_LAST_CENTER[i] = H_Arc_CC[i]
        MACHINE_LAST_POSITION[i] = H_Arc_P_NEW[i]

    # if the circle center is already the same we don't need to print it
    if H_ArcSameCenter:
        return [H_ArcPoint]
    else:
        return [H_ArcCenter, H_ArcPoint]


def HEIDEN_PolarArc(pol_cc_X, pol_cc_Y, pol_X, pol_Y, pol_Z, pol_Axis, pol_Incr):
    pol_Angle = 0
    pol_Result = ""

    # get delta distance form point to circle center
    delta_X = pol_X - pol_cc_X
    delta_Y = pol_Y - pol_cc_Y
    # prevent undefined result of atan
    if delta_X != 0 or delta_Y != 0:
        pol_Angle = math.degrees(math.atan2(delta_X, delta_Y))

    # set the appropriate zero and direction of angle
    # with X axis zero have the Y+ direction
    if pol_Axis == "X":
        pol_Angle = 90 - pol_Angle
    # with Y axis zero have the Z+ direction
    elif pol_Axis == "Y":
        pass
    # with Z axis zero have the X+ direction
    elif pol_Axis == "Z":
        pol_Angle = 90 - pol_Angle

    # set inside +0° +360° range
    if pol_Angle > 0:
        if pol_Angle >= 360:
            pol_Angle = pol_Angle - 360
    elif pol_Angle < 0:
        pol_Angle = pol_Angle + 360
        if pol_Angle < 0:
            pol_Angle = pol_Angle + 360

    pol_Result = (
        "P"
        + HEIDEN_Format(" PA+", pol_Angle)
        + " "
        + pol_Incr
        + HEIDEN_Format(pol_Axis, pol_Z)
    )

    return pol_Result


def HEIDEN_Drill(
    drill_Obj, drill_Params, drill_Type, drill_feed
):  # create a drill cycle and movement
    global FEED_MAX_SPEED
    global MACHINE_WORK_AXIS
    global MACHINE_LAST_POSITION
    global MACHINE_USE_FMAX
    global G_FUNCTION_STORE
    global STORED_CANNED_PARAMS
    drill_Defs = {"DIST": 0, "DEPTH": 0, "INCR": 0, "DWELL": 0, "FEED": drill_feed}
    drill_Output = []
    drill_Surface = None
    drill_SafePoint = 0
    drill_StartPoint = 0

    # try to get the distance from Clearance Height and Start Depth
    if hasattr(drill_Obj, "StartDepth") and hasattr(drill_Obj.StartDepth, "Value"):
        drill_Surface = drill_Obj.StartDepth.Value

    # initialize
    if "R" in drill_Params:
        # SafePoint equals to R position
        if G_FUNCTION_STORE["G91"]:  # incremental
            drill_SafePoint = drill_Params["R"] + MACHINE_LAST_POSITION["Z"]
        else:
            drill_SafePoint = drill_Params["R"]
        # Surface equals to theoric start point of drilling
        if drill_Surface is not None and drill_SafePoint > drill_Surface:
            drill_Defs["DIST"] = drill_Surface - drill_SafePoint
            drill_StartPoint = drill_Surface
        else:
            drill_Defs["DIST"] = 0
            drill_StartPoint = drill_SafePoint

    if "Z" in drill_Params:
        if G_FUNCTION_STORE["G91"]:  # incremental
            drill_Defs["DEPTH"] = drill_SafePoint + drill_Params["Z"]
        else:
            drill_Defs["DEPTH"] = drill_Params["Z"] - drill_StartPoint

    if drill_Defs["DEPTH"] > 0:
        drill_Output.append("; WARNING START DEPTH LOWER THAN FINAL DEPTH")

    drill_Defs["INCR"] = drill_Defs["DEPTH"]
    # overwrite
    if "P" in drill_Params:
        drill_Defs["DWELL"] = drill_Params["P"]
    if "Q" in drill_Params:
        drill_Defs["INCR"] = drill_Params["Q"]

    # set the parameters for rapid movements
    if MACHINE_USE_FMAX:
        if MACHINE_SKIP_PARAMS:
            drill_Rapid = " FMAX"
        else:
            drill_Rapid = " R0 FMAX M"
    else:
        if MACHINE_SKIP_PARAMS:
            drill_Rapid = HEIDEN_Format(" F", FEED_MAX_SPEED)
        else:
            drill_Rapid = " R0" + HEIDEN_Format(" F", FEED_MAX_SPEED) + " M"

    # move to drill location
    drill_Movement = "L"
    if G_FUNCTION_STORE["G91"]:  # incremental
        # update Z value to R + actual_Z if first call
        if G_FUNCTION_STORE[drill_Type] == False:
            MACHINE_LAST_POSITION["Z"] = drill_Defs["DIST"] + MACHINE_LAST_POSITION["Z"]
            drill_Movement = (
                "L" + HEIDEN_Format(" Z", MACHINE_LAST_POSITION["Z"]) + drill_Rapid
            )
            drill_Output.append(drill_Movement)

        # update X and Y position
        if "X" in drill_Params and drill_Params["X"] != 0:
            MACHINE_LAST_POSITION["X"] = drill_Params["X"] + MACHINE_LAST_POSITION["X"]
            drill_Movement += HEIDEN_Format(" X", MACHINE_LAST_POSITION["X"])
        if "Y" in drill_Params and drill_Params["Y"] != 0:
            MACHINE_LAST_POSITION["Y"] = drill_Params["Y"] + MACHINE_LAST_POSITION["Y"]
            drill_Movement += HEIDEN_Format(" Y", MACHINE_LAST_POSITION["Y"])
        if drill_Movement != "L":  # same location
            drill_Output.append(drill_Movement + drill_Rapid)
    else:  # not incremental
        # check if R is higher than actual Z and move if needed
        if drill_SafePoint > MACHINE_LAST_POSITION["Z"]:
            drill_Movement = "L" + HEIDEN_Format(" Z", drill_SafePoint) + drill_Rapid
            drill_Output.append(drill_Movement)
            MACHINE_LAST_POSITION["Z"] = drill_SafePoint

        # update X and Y position
        if "X" in drill_Params and drill_Params["X"] != MACHINE_LAST_POSITION["X"]:
            MACHINE_LAST_POSITION["X"] = drill_Params["X"]
            drill_Movement += HEIDEN_Format(" X", MACHINE_LAST_POSITION["X"])
        if "Y" in drill_Params and drill_Params["Y"] != MACHINE_LAST_POSITION["Y"]:
            MACHINE_LAST_POSITION["Y"] = drill_Params["Y"]
            drill_Movement += HEIDEN_Format(" Y", MACHINE_LAST_POSITION["Y"])
        if drill_Movement != "L":  # same location
            drill_Output.append(drill_Movement + drill_Rapid)

        # check if R is not than actual Z and move if needed
        if drill_SafePoint != MACHINE_LAST_POSITION["Z"]:
            drill_Movement = "L" + HEIDEN_Format(" Z", drill_SafePoint) + drill_Rapid
            drill_Output.append(drill_Movement)
            MACHINE_LAST_POSITION["Z"] = drill_SafePoint

    # check if cycle is already stored
    i = True
    for j in drill_Defs:
        if drill_Defs[j] != STORED_CANNED_PARAMS[j]:
            i = False

    if i == False:  # not same cycle, update and print
        for j in drill_Defs:
            STORED_CANNED_PARAMS[j] = drill_Defs[j]

        # get the DEF template and replace the strings
        drill_CycleDef = MACHINE_CYCLE_DEF[1].format(
            DIST=str("{:.3f}".format(drill_Defs["DIST"])),
            DEPTH=str("{:.3f}".format(drill_Defs["DEPTH"])),
            INCR=str("{:.3f}".format(drill_Defs["INCR"])),
            DWELL=str("{:.0f}".format(drill_Defs["DWELL"])),
            FEED=str("{:.0f}".format(drill_Defs["FEED"])),
        )
        drill_Output.extend(drill_CycleDef.split("\n"))

    # add the cycle call to do the drill
    if MACHINE_SKIP_PARAMS:
        drill_Output.append("CYCL CALL")
    else:
        drill_Output.append("CYCL CALL M")

    # set already active cycle,    to do: check changes and movements until G80
    G_FUNCTION_STORE[drill_Type] = True

    return drill_Output


def HEIDEN_Format(formatType, formatValue):
    global SPINDLE_DECIMALS
    global FEED_DECIMALS
    global AXIS_DECIMALS
    returnString = ""

    formatType = str(formatType)
    if formatType == "S" or formatType == " S":
        returnString = "%.*f" % (SPINDLE_DECIMALS, formatValue)
    elif formatType == "F" or formatType == " F":
        returnString = "%.*f" % (FEED_DECIMALS, formatValue)
    else:
        returnString = "%.*f" % (AXIS_DECIMALS, formatValue)
    return formatType + str(returnString)


def HEIDEN_Numberize(GCodeStrings):  # add line numbers and concatenation
    global LINENUMBERS
    global STARTLINENR
    global LINENUMBER_INCREMENT

    if LINENUMBERS:
        linenr = STARTLINENR
        result = ""
        for s in GCodeStrings:
            result += str(linenr) + " " + s + "\n"
            linenr += LINENUMBER_INCREMENT
        return result
    else:
        return "\n".join(GCodeStrings)


def HEIDEN_LBL_Get(GetPoint=None, GetLevel=None, GetAddit=0):
    global POSTGCODE
    global STORED_LBL
    global LBLIZE_PATH_LEVEL

    if GetPoint is not None:
        if LBLIZE_PATH_LEVEL != GetLevel:
            LBLIZE_PATH_LEVEL = GetLevel
            if len(STORED_LBL) != 0 and len(STORED_LBL[-1][-1]) == 0:
                STORED_LBL[-1].pop()
            STORED_LBL.append([[len(POSTGCODE) + GetAddit, len(POSTGCODE) + GetAddit]])
        else:
            if len(STORED_LBL[-1][-1]) == 0:
                STORED_LBL[-1][-1][0] = len(POSTGCODE) + GetAddit
            STORED_LBL[-1][-1][1] = len(POSTGCODE) + GetAddit
    else:
        if len(STORED_LBL) != 0 and len(STORED_LBL[-1][-1]) != 0:
            STORED_LBL[-1].append([])
    return


def HEIDEN_LBL_Replace():
    global POSTGCODE
    global STORED_LBL
    global FIRST_LBL

    Gcode_Lenght = len(POSTGCODE)

    # this function look inside strings to find and replace it with LBL
    # the array is bi-dimensional every "Z" step down create a column
    # until the "Z" axis is moved or rapid movements are performed
    # these "planar with feed movements" create a new row with
    # the index of start and end POSTGCODE line
    # to LBLize we need to diff with the first column backward
    # from the last row in the last column of indexes
    Cols = len(STORED_LBL) - 1
    if Cols > 0:
        FIRST_LBL += len(STORED_LBL[0])  # last LBL number to print
        for Col in range(Cols, 0, -1):
            LBL_Shift = 0  # LBL number iterator
            Rows = len(STORED_LBL[0]) - 1
            for Row in range(Rows, -1, -1):
                LBL_Shift += 1
                # get the indexes from actual column
                CellStart = STORED_LBL[Col][Row][1]
                CellStop = STORED_LBL[Col][Row][0] - 1
                # get the indexes from first column
                RefStart = STORED_LBL[0][Row][1]
                i = 0
                Remove = True  # initial value True, be False on error
                for j in range(CellStart, CellStop, -1):
                    # diff from actual to first index column/row
                    if POSTGCODE[j] != POSTGCODE[RefStart - i]:
                        Remove = False
                        break
                    i += 1
                if Remove:
                    # we can remove duplicate movements
                    for j in range(CellStart, CellStop, -1):
                        POSTGCODE.pop(j)
                    # add the LBL calls
                    POSTGCODE.insert(
                        CellStop + 1, "CALL LBL " + str(FIRST_LBL - LBL_Shift)
                    )
        if Gcode_Lenght != len(POSTGCODE):
            LBL_Shift = 0
            Rows = len(STORED_LBL[0]) - 1
            for Row in range(Rows, -1, -1):
                LBL_Shift += 1
                RefEnd = STORED_LBL[0][Row][1] + 1
                RefStart = STORED_LBL[0][Row][0]
                POSTGCODE.insert(RefEnd, "LBL 0")
                POSTGCODE.insert(RefStart, "LBL " + str(FIRST_LBL - LBL_Shift))
    STORED_LBL.clear()
    return
