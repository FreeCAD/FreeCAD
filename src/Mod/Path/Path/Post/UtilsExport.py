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

import datetime
import os

import FreeCAD
import Path.Post.Utils as PostUtils
import Path.Post.UtilsParse as PostUtilsParse

from PathScripts import PathToolController


# to distinguish python built-in open function from the one declared below
if open.__module__ in ["__builtin__", "io"]:
    pythonopen = open

#
# This routine processes things in the following order:
#
#       OUTPUT_HEADER
#       SAFETYBLOCK
#       LIST_TOOLS_IN_PREAMBLE
#       PREAMBLE
#       OUTPUT_BCNC
#       SHOW_OPERATION_LABELS
#       SHOW_MACHINE_UNITS
#       PRE_OPERATION
#       ENABLE_COOLANT (coolant on)
#       operation(s)
#       POST_OPERATION
#       ENABLE_COOLANT (coolant off)
#       RETURN_TO
#       OUTPUT_BCNC
#       TOOLRETURN
#       SAFETYBLOCK
#       POSTAMBLE
#       SHOW_EDITOR
#
# The names in all caps may be enabled/disabled/modified by setting
# the corresponding value in the postprocessor.
#


def export_common(values, objectslist, filename):
    """Do the common parts of postprocessing the objects in objectslist to filename."""
    #
    for obj in objectslist:
        if not hasattr(obj, "Path"):
            print(
                "The object "
                + obj.Name
                + " is not a path. Please select only path and Compounds."
            )
            return None

    # for obj in objectslist:
    #    print(obj.Name)

    print("PostProcessor:  " + values["POSTPROCESSOR_FILE_NAME"] + " postprocessing...")
    gcode = ""

    # write header
    if values["OUTPUT_HEADER"]:
        comment = PostUtilsParse.create_comment(
            "Exported by FreeCAD", values["COMMENT_SYMBOL"]
        )
        gcode += PostUtilsParse.linenumber(values) + comment + "\n"
        comment = PostUtilsParse.create_comment(
            "Post Processor: " + values["POSTPROCESSOR_FILE_NAME"],
            values["COMMENT_SYMBOL"],
        )
        gcode += PostUtilsParse.linenumber(values) + comment + "\n"
        if FreeCAD.ActiveDocument:
            cam_file = os.path.basename(FreeCAD.ActiveDocument.FileName)
        else:
            cam_file = "<None>"
        comment = PostUtilsParse.create_comment(
            "Cam File: " + cam_file, values["COMMENT_SYMBOL"]
        )
        gcode += PostUtilsParse.linenumber(values) + comment + "\n"
        comment = PostUtilsParse.create_comment(
            "Output Time: " + str(datetime.datetime.now()), values["COMMENT_SYMBOL"]
        )
        gcode += PostUtilsParse.linenumber(values) + comment + "\n"

    # Check canned cycles for drilling
    if values["TRANSLATE_DRILL_CYCLES"]:
        if len(values["SUPPRESS_COMMANDS"]) == 0:
            values["SUPPRESS_COMMANDS"] = ["G99", "G98", "G80"]
        else:
            values["SUPPRESS_COMMANDS"] += ["G99", "G98", "G80"]

    for line in values["SAFETYBLOCK"].splitlines(False):
        gcode += PostUtilsParse.linenumber(values) + line + "\n"

    # Write the preamble
    if values["OUTPUT_COMMENTS"]:
        if values["LIST_TOOLS_IN_PREAMBLE"]:
            for item in objectslist:
                if hasattr(item, "Proxy") and isinstance(
                    item.Proxy, PathToolController.ToolController
                ):
                    comment = PostUtilsParse.create_comment(
                        "T{}={}".format(item.ToolNumber, item.Name),
                        values["COMMENT_SYMBOL"],
                    )
                    gcode += PostUtilsParse.linenumber(values) + comment + "\n"
        comment = PostUtilsParse.create_comment(
            "Begin preamble", values["COMMENT_SYMBOL"]
        )
        gcode += PostUtilsParse.linenumber(values) + comment + "\n"
    for line in values["PREAMBLE"].splitlines(False):
        gcode += PostUtilsParse.linenumber(values) + line + "\n"
    # verify if PREAMBLE or SAFETYBLOCK have changed MOTION_MODE or UNITS
    if "G90" in values["PREAMBLE"] or "G90" in values["SAFETYBLOCK"]:
        values["MOTION_MODE"] = "G90"
    elif "G91" in values["PREAMBLE"] or "G91" in values["SAFETYBLOCK"]:
        values["MOTION_MODE"] = "G91"
    else:
        gcode += PostUtilsParse.linenumber(values) + values["MOTION_MODE"] + "\n"
    if "G21" in values["PREAMBLE"] or "G21" in values["SAFETYBLOCK"]:
        values["UNITS"] = "G21"
        values["UNIT_FORMAT"] = "mm"
        values["UNIT_SPEED_FORMAT"] = "mm/min"
    elif "G20" in values["PREAMBLE"] or "G20" in values["SAFETYBLOCK"]:
        values["UNITS"] = "G20"
        values["UNIT_FORMAT"] = "in"
        values["UNIT_SPEED_FORMAT"] = "in/min"
    else:
        gcode += PostUtilsParse.linenumber(values) + values["UNITS"] + "\n"

    for obj in objectslist:

        # Debug...
        # print("\n" + "*"*70)
        # dump(obj)
        # print("*"*70 + "\n")

        # Skip inactive operations
        if hasattr(obj, "Active"):
            if not obj.Active:
                continue
        if hasattr(obj, "Base") and hasattr(obj.Base, "Active"):
            if not obj.Base.Active:
                continue

        # do the pre_op
        if values["OUTPUT_BCNC"]:
            comment = PostUtilsParse.create_comment(
                "Block-name: " + obj.Label, values["COMMENT_SYMBOL"]
            )
            gcode += PostUtilsParse.linenumber(values) + comment + "\n"
            comment = PostUtilsParse.create_comment(
                "Block-expand: 0", values["COMMENT_SYMBOL"]
            )
            gcode += PostUtilsParse.linenumber(values) + comment + "\n"
            comment = PostUtilsParse.create_comment(
                "Block-enable: 1", values["COMMENT_SYMBOL"]
            )
            gcode += PostUtilsParse.linenumber(values) + comment + "\n"
        if values["OUTPUT_COMMENTS"]:
            if values["SHOW_OPERATION_LABELS"]:
                comment = PostUtilsParse.create_comment(
                    "Begin operation: %s" % obj.Label, values["COMMENT_SYMBOL"]
                )
            else:
                comment = PostUtilsParse.create_comment(
                    "Begin operation", values["COMMENT_SYMBOL"]
                )
            gcode += PostUtilsParse.linenumber(values) + comment + "\n"
            if values["SHOW_MACHINE_UNITS"]:
                comment = PostUtilsParse.create_comment(
                    "Machine units: %s" % values["UNIT_SPEED_FORMAT"],
                    values["COMMENT_SYMBOL"],
                )
                gcode += PostUtilsParse.linenumber(values) + comment + "\n"
            if values["OUTPUT_MACHINE_NAME"]:
                comment = PostUtilsParse.create_comment(
                    "Machine: %s, %s"
                    % (values["MACHINE_NAME"], values["UNIT_SPEED_FORMAT"]),
                    values["COMMENT_SYMBOL"],
                )
                gcode += PostUtilsParse.linenumber(values) + comment + "\n"
        for line in values["PRE_OPERATION"].splitlines(False):
            gcode += PostUtilsParse.linenumber(values) + line + "\n"

        # get coolant mode
        coolantMode = "None"
        if (
            hasattr(obj, "CoolantMode")
            or hasattr(obj, "Base")
            and hasattr(obj.Base, "CoolantMode")
        ):
            if hasattr(obj, "CoolantMode"):
                coolantMode = obj.CoolantMode
            else:
                coolantMode = obj.Base.CoolantMode

        # turn coolant on if required
        if values["ENABLE_COOLANT"]:
            if values["OUTPUT_COMMENTS"]:
                if not coolantMode == "None":
                    comment = PostUtilsParse.create_comment(
                        "Coolant On:" + coolantMode, values["COMMENT_SYMBOL"]
                    )
                    gcode += PostUtilsParse.linenumber(values) + comment + "\n"
            if coolantMode == "Flood":
                gcode += PostUtilsParse.linenumber(values) + "M8" + "\n"
            if coolantMode == "Mist":
                gcode += PostUtilsParse.linenumber(values) + "M7" + "\n"

        # process the operation gcode
        gcode += PostUtilsParse.parse(values, obj)

        # do the post_op
        if values["OUTPUT_COMMENTS"]:
            comment = PostUtilsParse.create_comment(
                "%s operation: %s" % (values["FINISH_LABEL"], obj.Label),
                values["COMMENT_SYMBOL"],
            )
            gcode += PostUtilsParse.linenumber(values) + comment + "\n"
        for line in values["POST_OPERATION"].splitlines(False):
            gcode += PostUtilsParse.linenumber(values) + line + "\n"

        # turn coolant off if required
        if values["ENABLE_COOLANT"]:
            if not coolantMode == "None":
                if values["OUTPUT_COMMENTS"]:
                    comment = PostUtilsParse.create_comment(
                        "Coolant Off:" + coolantMode, values["COMMENT_SYMBOL"]
                    )
                    gcode += PostUtilsParse.linenumber(values) + comment + "\n"
                gcode += PostUtilsParse.linenumber(values) + "M9" + "\n"

    if values["RETURN_TO"]:
        gcode += PostUtilsParse.linenumber(values) + "G0 X%s Y%s Z%s\n" % tuple(
            values["RETURN_TO"]
        )

    # do the post_amble
    if values["OUTPUT_BCNC"]:
        comment = PostUtilsParse.create_comment(
            "Block-name: post_amble", values["COMMENT_SYMBOL"]
        )
        gcode += PostUtilsParse.linenumber(values) + comment + "\n"
        comment = PostUtilsParse.create_comment(
            "Block-expand: 0", values["COMMENT_SYMBOL"]
        )
        gcode += PostUtilsParse.linenumber(values) + comment + "\n"
        comment = PostUtilsParse.create_comment(
            "Block-enable: 1", values["COMMENT_SYMBOL"]
        )
        gcode += PostUtilsParse.linenumber(values) + comment + "\n"
    if values["OUTPUT_COMMENTS"]:
        comment = PostUtilsParse.create_comment(
            "Begin postamble", values["COMMENT_SYMBOL"]
        )
        gcode += PostUtilsParse.linenumber(values) + comment + "\n"
    for line in values["TOOLRETURN"].splitlines(False):
        gcode += PostUtilsParse.linenumber(values) + line + "\n"
    for line in values["SAFETYBLOCK"].splitlines(False):
        gcode += PostUtilsParse.linenumber(values) + line + "\n"
    for line in values["POSTAMBLE"].splitlines(False):
        gcode += PostUtilsParse.linenumber(values) + line + "\n"

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
        gfile = pythonopen(filename, "w", newline=values["END_OF_LINE_CHARACTERS"])
        gfile.write(final)
        gfile.close()

    return final
