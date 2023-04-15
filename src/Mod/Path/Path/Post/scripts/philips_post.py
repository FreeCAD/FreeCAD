# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 Christoph Blaue <blaue@fh-westkueste.de>           *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

# 03-24-2021  Sliptonic:  I've removed the PathUtils import and job lookup
# post processors shouldn't be reaching back to the job.  This can cause a
# proxy error.

import FreeCAD
import argparse
import time
import Path.Post.Utils as PostUtils
import PathScripts.PathUtils as PathUtils
import math

TOOLTIP = """Post processor for Maho M 600E mill

Machines with Philips or Heidenhain control should be very easy to adapt.

The post processor is configurable by changing the values of constants.
No programming experience required. This can make a generated g-code
program more readable and since older machines have very
limited memory it seems sensible to reduce the number of commands and
parameters, like e.g. suppress the units in the header and at every hop.
"""

# ***************************************************************************
# user editable stuff here

COMMAND_SPACE = " "

MACHINE_NAME = "Maho 600E"
CORNER_MIN = {"x": -51.877, "y": 0, "z": 0}  # use metric for internal units
CORNER_MAX = {"x": 591.5, "y": 391.498, "z": 391.5}  # use metric for internal units

UNITS = "G21"  # use metric units
# possible values:
#                  'G20'     for inches,
#                  'G21'     for metric units.
# a mapping to different GCodes is handled in the GCODE MAP below

UNITS_INCLUDED = False  # do not include the units in the GCode program
# possible values:
#                  True      if units should be included
#                  False     if units should not be included
# usually the units to be used are defined in the machine constants and almost never change,
# so this can be set to False.

COMMENT = ""
# possible values:
#                  ';'        centroid or sinumerik comment symbol,
#                  ''         leave blank for bracketed comments style "(comment comment comment)"
#                  '...'      any other symbol to start comments
# currently this can be only one symbol, if it should be a sequence of characters
# in PostUtils.py the line
#     if len(commentsym)==1:
# should be changed to
#     if len(commentsym)>1:

SHOW_EDITOR = True
# possible values:
#                  True      before the file is written it is shown to the user for inspection
#                  False     the file is written directly

LINENUMBERS = True
# possible values:
#                  True      if linenumbers of the form N1 N2 ... should be included
#                  False     linennumbers are suppressed
# if linenumbers are used, header and footer get numbered as well

STARTLINENR = 1  # first linenumber used
# possible values:
#                  any integer value >= 0
# to have the possibility to change some initial values directly at the CNC machine
# without renumbering the rest it is possible to start the numbering of
# the file with some value > 0

LINENUMBER_INCREMENT = 1
# possible values:
#                  any integer value > 0
# similar to STARTLINENR it is possible to leave gaps in the linenumbering
# of subsequent lines

MODAL = True
# possible values:
#                  True      repeated GCodes in subsequent lines are suppressed, like in the following snippet
#                            G1 X10 Y20
#                               X15 Y30
#                  False     repeated GCodes in subsequent lines are repeated in the GCode file
#                            G1 X10 Y20
#                            G1 X15 Y30

# suppress these parameters if they haven't changed
MODALPARAMS = ["X", "Y", "Z", "S", "F"]
# possible values:
#                  any list of GCode parameters
# if a parameter doesn't change from one line to the next ( or even further) it is suppressed.
# Example:
#                            G1 X10 Y20
#                            G1 Y30
# If in addition MODAL is set to True, the generated GCode changes to
#                            G1 X10 Y20
#                            Y30

SWAP_G2_G3 = True  # some machines have the sign of the X-axis swapped, so they behave like milling from the bottom
# possible values:
#                  True      if left and right turns are to be swapped
#                  False     don't swap
# this might be special with some maho machines or even with mine and
# might be changed in the machine constants as well

SWAP_Y_Z = (
    True  # machines with an angle milling head do not switch axes, so we do it here
)
# possible values:
#                  True      if Y and Z values have to be swapped
#                  False     do not swap
# For vertical milling machines the Z-axis is horizontal (of course).
# If they have an angle milling head, they mill vertical, alas the Z-axis stays horizontal.
# With this parameter we can swap the output values of Y and Z.
# For commands G2 and G3 this means that J and K are swapped as well

ABSOLUTE_CIRCLE_CENTER = True
# possible values:
#                  True      use absolute values for the circle center in commands G2, G3
#                  False     values for I, J, K are given relative to the last point

USE_RADIUS_IF_POSSIBLE = True
# possible values:
#                  True      if in commands G2 and G3 the usage of radius R is preferred
#                  False     if in commands G2 and G3 we use always I and J
# When milling arcs there are two reasons to use the radius instead of the center:
# 1. the GCode program might be easier to understand
# 2. Some machines seem to have a different scheme for calculating / rounding the values of the center
#    Thus it is possible that the machine complains, that the endpoint of the arc does not lie on the arc.
#    Using the radius instead avoids this problem.
# The post processor takes care of the fact, that only angles <= 180 degrees can be used with R
# for larger angles the center is used independent of the setting of this
# constant

RADIUS_COMMENT = True
# possible values:
#                  True      for better understanding the radius of an arc is included as a comment
#                  False     no additional comment is included
# In case the comments are included they are always included with the bracketing syntax like  '(R20.456)'
# and never with the comment symbol, because the radius might appear in
# the middle of a line.

GCODE_MAP = {
    "M1": "M0",
    "M6": "M66",
    "G20": "G70",
    "G21": "G71",
}  # cb: this could be used to swap G2/G3
# possible values:
# Comma separated list of values of the form 'sourceGCode':'targetGCode'
#
# Although the basic movement commands G0, G1, G2 seem to be used uniformly in different GCode dialects,
# this is not the case for all commands.
# E.g the Maho dialect uses G70 and G71 for the units inches vs. metric.
# The map {'M1':'M0', 'G20':'G70', 'G21':'G71'} maps the optional stop command M1 to M0,
# because some Maho machines do not have the optional button on its panel
# in addition it maps inches G20 to G70 and metric G21 to G71

AXIS_DECIMALS = 3
# possible values:
# integer >= 0

FEED_DECIMALS = 2
# possible values:
# integer >= 0

SPINDLE_DECIMALS = 0
# possible values:
# integer >= 0

SUPPRESS_ZERO_FEED = True
# possible values: True    if feed is zero the F command is suppressed
#                  False   F commands are written even if they are zero
# This is useful for machines without special speeds for the G0 command. They could be
# left zero and are suppressed in the output

# The header is divided into two parts, one is dynamic, the other is a static GCode header.
# If the current selection and the current time should be included in the header,
# it has to be generated at execution time, and thus it cannot be held in constant values.
# The last linefeed should be omitted, it is inserted automatically
# linenumbers are inserted automatically if LINENUMBERS is True
# if you don't want to use this header you have to provide a minimal function
# def mkHeader(selection):
#   return ''

parser = argparse.ArgumentParser(prog="philips", add_help=False)
parser.add_argument("--header", action="store_true", help="create header output")
parser.add_argument("--no-header", action="store_true", help="suppress header output")

parser.add_argument("--comments", action="store_true", help="create comment output")
parser.add_argument(
    "--no-comments", action="store_true", help="suppress comment output"
)

parser.add_argument(
    "--line-numbers", action="store_true", help="prefix with line numbers"
)
parser.add_argument(
    "--no-line-numbers", action="store_true", help="omit line number prefixes"
)

parser.add_argument(
    "--show-editor", action="store_true", help="pop up editor before writing output"
)
parser.add_argument(
    "--no-show-editor",
    action="store_true",
    help="don't pop up editor before writing output",
)

TOOLTIP_ARGS = parser.format_help()


def processArguments(argstring):
    global LINENUMBERS
    global SHOW_EDITOR

    for arg in argstring.split():
        if arg == "--line-numbers":
            LINENUMBERS = True
        elif arg == "--no-line-numbers":
            LINENUMBERS = False
        elif arg == "--show-editor":
            SHOW_EDITOR = True
        elif arg == "--no-show-editor":
            SHOW_EDITOR = False


def mkHeader(selection):
    # job = PathUtils.findParentJob(selection[0])
    # this is within a function, because otherwise filename and time don't change when changing the FreeCAD project
    #  now = datetime.datetime.now()
    now = time.strftime("%Y-%m-%d %H:%M")
    originfile = FreeCAD.ActiveDocument.FileName
    headerNoNumber = "%PM\n"  # this line gets no linenumber
    # if hasattr(job, "Description"):
    #     description = job.Description
    # else:
    #     description = ""
    description = ""
    # this line gets no linenumber, it is already a specially numbered
    headerNoNumber += "N9XXX (" + description + ", " + now + ")\n"
    header = ""
    #  header += "(Output Time:" + str(now) + ")\n"
    header += "(" + originfile + ")\n"
    #    header += "(Exported by FreeCAD)\n"
    header += "(Post Processor: " + __name__ + ")\n"
    #    header += "(Target machine: " + MACHINE_NAME + ")\n"
    header += "G18\n"  # Select XY plane
    header += "G90\n"  # Absolute coordinates
    header += "G51\n"  # Reset Zero
    header += "G52 (ersetze G55-G59)"  # set zero
    return headerNoNumber + linenumberify(header)


GCODE_HEADER = ""  # do not terminate with a newline, it is inserted by linenumberify
# GCODE_HEADER = "G40 G90"   # do not terminate with a newline, it is inserted by linenumberify
# possible values:
#                 any sequence of GCode, multiple lines are welcome
# this constant header follows the text generated by the function mkheader
# linenumbers are inserted automatically if LINENUMBERS is True

# do not terminate with a newline, it is inserted by linenumberify
GCODE_FOOTER = "M30"
# possible values:
#                 any sequence of GCode, multiple lines are welcome
# the footer is used to clean things up, reset modal commands and stop the machine
# linenumbers are inserted automatically if LINENUMBERS is True

# don't edit with the stuff below the next line unless you know what you're doing :)
# ***************************************************************************

linenr = 0  # variable has to be global because it is used by linenumberify and export

if open.__module__ in ["__builtin__", "io"]:
    pythonopen = open


def angleUnder180(command, lastX, lastY, x, y, i, j):
    # radius R can be used iff angle is < 180.
    # This is the case
    #   if the previous point is left of the current and the center is below (or on) the connection line
    # or if the previous point is right of the current and the center is above
    # (or on) the connection line
    middleOfLineY = (lastY + y) / 2
    centerY = lastY + j
    if (
        command == "G2"
        and (
            (lastX == x and ((lastY < y and i >= 0) or (lastY > y and i <= 0)))
            or (lastX < x and centerY <= middleOfLineY)
            or (lastX > x and centerY >= middleOfLineY)
        )
    ) or (
        command == "G3"
        and (
            (lastX == x and ((lastY < y and i <= 0) or (lastY > y and i >= 0)))
            or (lastX < x and centerY >= middleOfLineY)
            or (lastX > x and centerY <= middleOfLineY)
        )
    ):
        return True
    else:
        return False


def mapGCode(command):
    if command in GCODE_MAP:
        mappedCommand = GCODE_MAP[command]
    else:
        mappedCommand = command
    if SWAP_G2_G3:
        if command == "G2":
            mappedCommand = "G3"
        elif command == "G3":
            mappedCommand = "G2"
    return mappedCommand


def linenumberify(GCodeString):
    # add a linenumber at every beginning of line
    global linenr
    if not LINENUMBERS:
        result = GCodeString + "\n"
    else:
        result = ""
        strList = GCodeString.split("\n")
        for s in strList:
            if s:
                # only non empty lines get numbered. the special lines "%PM"
                # and prognumber "N9XXX" are skipped

                result += "N" + str(linenr) + " " + s + "\n"
                linenr += LINENUMBER_INCREMENT
            else:
                result += s + "\n"
    return result


def export(objectslist, filename, argstring):
    global UNITS
    global linenr

    linenr = STARTLINENR
    lastX = 0
    lastY = 0
    lastZ = 0
    params = [
        "X",
        "Y",
        "Z",
        "A",
        "B",
        "I",
        "J",
        "F",
        "H",
        "S",
        "T",
        "Q",
        "R",
        "L",
    ]  # Using XY plane most of the time so skipping K
    modalParamsDict = dict()
    for mp in MODALPARAMS:
        modalParamsDict[mp] = None
    for obj in objectslist:
        if not hasattr(obj, "Path"):
            print(
                "the object "
                + obj.Name
                + " is not a path. Please select only path and Compounds."
            )
            return
    myMachine = None
    for pathobj in objectslist:
        if hasattr(pathobj, "MachineName"):
            myMachine = pathobj.MachineName
        if hasattr(pathobj, "MachineUnits"):
            if pathobj.MachineUnits == "Metric":
                UNITS = "G21"
            else:
                UNITS = "G20"
    if myMachine is None:
        print("philips_post: No machine found in this selection")

    gcode = ""
    gcode += mkHeader(objectslist)
    gcode += linenumberify(GCODE_HEADER)
    if UNITS_INCLUDED:
        gcode += linenumberify(mapGCode(UNITS))
    lastcommand = None
    for obj in objectslist:
        if hasattr(obj, "Comment"):
            gcode += linenumberify("(" + obj.Comment + ")")
        for c in PathUtils.getPathWithPlacement(obj).Commands:
            outstring = []
            command = c.Name
            if command != "G0":
                command = command.replace("G0", "G")  # normalize: G01 -> G1

            if command != UNITS or UNITS_INCLUDED:
                if command[0] == "(":
                    command = PostUtils.fcoms(command, COMMENT)
                # the mapping is done for output only! For internal things we
                # still use the old value.
                mappedCommand = mapGCode(command)

                if not MODAL or command != lastcommand:
                    outstring.append(mappedCommand)
                #               if MODAL:
                # #\better:   append iff MODAL == False
                #                   if command == lastcommand:
                #                       outstring.pop(0)
                if len(c.Parameters) >= 1:
                    for param in params:
                        # test   print("param: " + param + ",  command: " + command)
                        if param in c.Parameters:
                            if (param in MODALPARAMS) and (
                                modalParamsDict[str(param)] == c.Parameters[str(param)]
                            ):
                                # do nothing or append white space
                                outstring.append("  ")
                            elif param == "F":
                                feed = c.Parameters["F"]
                                if SUPPRESS_ZERO_FEED and feed == 0:
                                    pass
                                else:
                                    outstring.append(
                                        param
                                        + PostUtils.fmt(feed, FEED_DECIMALS, UNITS)
                                    )
                            elif param == "H":
                                outstring.append(param + str(int(c.Parameters["H"])))
                            elif param == "S":
                                # rpm is unitless-therefore I had to 'fake it
                                # out' by using metric units which don't get
                                # converted from entered value
                                outstring.append(
                                    param
                                    + PostUtils.fmt(
                                        c.Parameters["S"], SPINDLE_DECIMALS, "G21"
                                    )
                                )
                            elif param == "T":
                                outstring.append(param + str(int(c.Parameters["T"])))
                            elif param == "I" and (command == "G2" or command == "G3"):
                                # test print("param = 'I'")
                                # this is the special case for circular paths,
                                # where relative coordinates have to be changed
                                # to absolute
                                i = c.Parameters["I"]
                                # calculate the radius r
                                j = c.Parameters["J"]
                                r = math.sqrt(i**2 + j**2)
                                if USE_RADIUS_IF_POSSIBLE and angleUnder180(
                                    command,
                                    lastX,
                                    lastY,
                                    c.Parameters["X"],
                                    c.Parameters["Y"],
                                    i,
                                    j,
                                ):
                                    outstring.append(
                                        "R" + PostUtils.fmt(r, AXIS_DECIMALS, UNITS)
                                    )
                                else:
                                    if RADIUS_COMMENT:
                                        outstring.append(
                                            "(R"
                                            + PostUtils.fmt(r, AXIS_DECIMALS, UNITS)
                                            + ")"
                                        )
                                    if ABSOLUTE_CIRCLE_CENTER:
                                        i += lastX
                                    outstring.append(
                                        param + PostUtils.fmt(i, AXIS_DECIMALS, UNITS)
                                    )
                            elif param == "J" and (command == "G2" or command == "G3"):
                                # this is the special case for circular paths,
                                # where incremental center has to be changed to
                                # absolute center
                                i = c.Parameters["I"]
                                j = c.Parameters["J"]
                                if USE_RADIUS_IF_POSSIBLE and angleUnder180(
                                    command,
                                    lastX,
                                    lastY,
                                    c.Parameters["X"],
                                    c.Parameters["Y"],
                                    i,
                                    j,
                                ):
                                    # R is handled with the I parameter, here:
                                    # do nothing at all, keep the structure as
                                    # with I command
                                    pass
                                else:
                                    if ABSOLUTE_CIRCLE_CENTER:
                                        j += lastY
                                    if SWAP_Y_Z:
                                        # we have to swap j and k as well
                                        outstring.append(
                                            "K" + PostUtils.fmt(j, AXIS_DECIMALS, UNITS)
                                        )
                                    else:
                                        outstring.append(
                                            param
                                            + PostUtils.fmt(j, AXIS_DECIMALS, UNITS)
                                        )
                            elif param == "K" and (command == "G2" or command == "G3"):
                                # this is the special case for circular paths,
                                # where incremental center has to be changed to
                                # absolute center
                                outstring.append(
                                    "("
                                    + param
                                    + PostUtils.fmt(
                                        c.Parameters[param], AXIS_DECIMALS, UNITS
                                    )
                                    + ")"
                                )
                                z = c.Parameters["Z"]
                                k = c.Parameters["K"]
                                if USE_RADIUS_IF_POSSIBLE and angleUnder180(
                                    command,
                                    lastX,
                                    lastY,
                                    c.Parameters["X"],
                                    c.Parameters["Y"],
                                    i,
                                    j,
                                ):
                                    # R is handled with the I parameter, here:
                                    # do nothing at all, keep the structure as
                                    # with I command
                                    pass
                                else:
                                    if ABSOLUTE_CIRCLE_CENTER:
                                        k += lastZ
                                if SWAP_Y_Z:
                                    # we have to swap j and k as well
                                    outstring.append(
                                        "J" + PostUtils.fmt(j, AXIS_DECIMALS, UNITS)
                                    )
                                else:
                                    outstring.append(
                                        param + PostUtils.fmt(j, AXIS_DECIMALS, UNITS)
                                    )
                            elif param == "Y" and SWAP_Y_Z:
                                outstring.append(
                                    "Z"
                                    + PostUtils.fmt(
                                        c.Parameters[param], AXIS_DECIMALS, UNITS
                                    )
                                )
                            elif param == "Z" and SWAP_Y_Z:
                                outstring.append(
                                    "Y"
                                    + PostUtils.fmt(
                                        c.Parameters[param], AXIS_DECIMALS, UNITS
                                    )
                                )
                            else:
                                # To Do: suppress unknown commands, if this is done here, all X parameters are suppressed
                                # this is an unknown command, don't create GCode for it
                                #                                print("parameter " + param + " for command " + command + " ignored")
                                outstring.append(
                                    param
                                    + PostUtils.fmt(
                                        c.Parameters[param], AXIS_DECIMALS, UNITS
                                    )
                                )

                            if param in MODALPARAMS:
                                modalParamsDict[str(param)] = c.Parameters[param]
                    # save the last X, Y, Z values
                    if "X" in c.Parameters:
                        lastX = c.Parameters["X"]
                    if "Y" in c.Parameters:
                        lastY = c.Parameters["Y"]
                    if "Z" in c.Parameters:
                        lastZ = c.Parameters["Z"]

                outstr = ""
                for w in outstring:
                    outstr += w + COMMAND_SPACE
                outstr = outstr.replace("]", "")
                outstr = outstr.replace("[", "")
                outstr = outstr.replace("'", "")
                outstr = outstr.replace(",", ".")
                if LINENUMBERS:
                    gcode += "N" + str(linenr) + " "
                    linenr += LINENUMBER_INCREMENT
                gcode += outstr + "\n"
                lastcommand = c.Name
    gcode = gcode.replace("_", "-")
    gcode += linenumberify(GCODE_FOOTER)
    if SHOW_EDITOR:
        PostUtils.editor(gcode)
    gfile = pythonopen(filename, "w")
    gfile.write(gcode)
    gfile.close()
