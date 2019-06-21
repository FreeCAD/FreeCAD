# ***************************************************************************
# *   (c) sliptonic (shopinthewoods<at>gmail.com) 2014                      *
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

'''
This is a preprocessor file for the Path workbench. Its aim is to
parse the contents of a given OpenSBP file, and transform it to make it
suitable for use in a Path object. This preprocessor, once placed in the
appropriate PathScripts folder, can be used directly from inside FreeCAD,
via the GUI importer or via python scripts with:

import opensbp_pre
opensbp_pre.insert("/path/to/myfile.ngc","DocumentName")


DONE
Correctly imports single axis and multi axis moves.
Stores Jog and Feed speeds
Appends Multiaxis Feed speed to G1 moves
Jog rates don't append to G0 moves
Make single axis feed rates work
Imports CG (non-diameter) arcs.
Handles CW and CCW spindle speeds
if operations are preceded by a comment ('New Path ...)  They are split into multiple paths

TODO
Many other OpenSBP commands not handled

'''
from __future__ import print_function
import FreeCAD
import PathScripts.PathUtil as PathUtil
import os
import Path

AXIS = 'X', 'Y', 'Z', 'A', 'B'  # OpenSBP always puts multiaxis move parameters in this order
SPEEDS = 'XY', 'Z', 'A', 'B'

# to distinguish python built-in open function from the one declared below
if open.__module__ in ['__builtin__', 'io']:
    pythonopen = open


def open(filename):
    "called when freecad opens a file."
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    insert(filename, doc.Name)


def insert(filename, docname):
    "called when freecad imports a file"
    "This insert expects parse to return a list of strings"
    "each string will become a separate path"
    gfile = pythonopen(filename)
    gcode = gfile.read()
    gfile.close()
    gcode = parse(gcode)
    doc = FreeCAD.getDocument(docname)
    for subpath in gcode:
        obj = doc.addObject("Path::Feature", "Path")
        path = Path.Path(subpath)
        obj.Path = path


def parse(inputstring):
    "parse(inputstring): returns a list of parsed output string"
    print("preprocessing...")
    # split the input by line
    lines = inputstring.split("\n")
    return_output = []
    output = ""
    last = {'X': None, 'Y': None, 'Z': None, 'A': None, 'B': None}
    lastrapidspeed = {'XY': "50", 'Z': "50", 'A': "50", 'B': "50"}  # set default rapid speeds
    lastfeedspeed = {'XY': "50", 'Z': "50", 'A': "50", 'B': "50"}  # set default feed speed
    movecommand = ['G1', 'G0', 'G02', 'G03']

    for line in lines:
        # remove any leftover trailing and preceding spaces
        line = line.strip()
        if not line:
            # discard empty lines
            continue
        if line[0] in ["'", "&"]:
            # discard comment and other non strictly gcode lines
            if line[0:9] == "'New Path":
                # starting new path
                if any(x in output for x in movecommand):  # make sure the path has at least one move command.
                    return_output.append(output)
                    output = ""
            continue

        words = [a.strip() for a in line.split(",")]
        words[0] = words[0].upper()
        if words[0] in ["J2", "J3", "J4", "J5", "M2", "M3", "M4", "M5"]:  # multi-axis jogs and moves
            if words[0][0] == 'J':  # jog move
                s = "G0 "
            else:   # feed move
                s = "G1 "
            speed = lastfeedspeed["XY"]

            for i in range(1, len(words)):
                if words[i] == '':
                    if last[AXIS[i - 1]] is None:
                        continue
                    else:
                        s += AXIS[i - 1] + last[AXIS[i - 1]]
                else:
                    s += AXIS[i - 1] + words[i]
                    last[AXIS[i - 1]] = words[i]
            output += s + " F" + speed + '\n'

        if words[0] in ["JA", "JB", "JX", "JY", "JZ", "MA", "MB", "MX", "MY", "MZ"]:  # single axis jogs and moves
            if words[0][0] == 'J':  # jog move
                s = "G0 "
                if words[0][1] in ['X', 'Y']:
                    speed = lastrapidspeed["XY"]
                else:
                    speed = lastrapidspeed[words[0][1]]

            else:    # feed move
                s = "G1 "
                if words[0][1] in ['X', 'Y']:
                    speed = lastfeedspeed["XY"]
                else:
                    speed = lastfeedspeed[words[0][1]]

            last[words[0][1]] = words[1]
            output += s
            for key, val in PathUtil.keyValueIter(last):
                if val is not None:
                    output += key + str(val) + " F" + speed + "\n"

        if words[0] in ["JS"]:  # set jog speed
            for i in range(1, len(words)):
                if words[i] == '':
                    continue
                else:
                    lastrapidspeed[SPEEDS[i - 1]] = words[i]

        if words[0] in ["MD"]:  # move distance with distance and angle.
            # unsupported at this time
            continue
        if words[0] in ["MH"]:  # move home
            # unsupported at this time
            continue
        if words[0] in ["MS"]:  # set move speed
            for i in range(1, len(words)):
                if words[i] == '':
                    continue
                else:
                    lastfeedspeed[SPEEDS[i - 1]] = words[i]
        if words[0] in ["MO"]:  # motors off
            # unsupported at this time
            continue

        if words[0] in ["TR"]:  # Setting spindle speed
            if float(words[1]) < 0:
                s = "M4 S"
            else:
                s = "M3 S"
            s += str(abs(float(words[1])))
            output += s + '\n'

        if words[0] in ["CG"]:  # Gcode circle/arc
            if words[1] != "":  # diameter mode
                print("diameter mode not supported")
                continue

            else:
                if words[7] == "1":  # CW
                    s = "G2"
                else:  # CCW
                    s = "G3"

                s += " X" + words[2] + " Y" + words[3] + " I" + words[4] + " J" + words[5] + " F" + str(lastfeedspeed["XY"])
                output += s + '\n'

                last["X"] = words[2]
                last["Y"] = words[3]

    # Make sure all appended paths have at least one move command.
    if any(x in output for x in movecommand):
        return_output.append(output)
        print("done preprocessing.")

    return return_output


print(__name__ + " gcode preprocessor loaded.")
