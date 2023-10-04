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
from typing import Any, Dict, List

import FreeCAD
import Path.Post.Utils as PostUtils
import Path.Post.UtilsParse as PostUtilsParse
import Path.Tool.Controller as PathToolController

# Define some types that are used throughout this file
Gcode = List[str]
Values = Dict[str, Any]


def check_canned_cycles(values: Values) -> None:
    """Check canned cycles for drilling."""
    if values["TRANSLATE_DRILL_CYCLES"]:
        if len(values["SUPPRESS_COMMANDS"]) == 0:
            values["SUPPRESS_COMMANDS"] = ["G99", "G98", "G80"]
        else:
            values["SUPPRESS_COMMANDS"] += ["G99", "G98", "G80"]


def determine_coolant_mode(obj) -> str:
    """Determine the coolant mode."""
    if (
        hasattr(obj, "CoolantMode")
        or hasattr(obj, "Base")
        and hasattr(obj.Base, "CoolantMode")
    ):
        if hasattr(obj, "CoolantMode"):
            return obj.CoolantMode
        return obj.Base.CoolantMode
    return "None"


def output_coolant_off(values: Values, gcode: Gcode, coolant_mode: str) -> None:
    """Output the commands to turn coolant off if necessary."""
    comment: str
    nl: str = "\n"

    if values["ENABLE_COOLANT"] and coolant_mode != "None":
        if values["OUTPUT_COMMENTS"]:
            comment = PostUtilsParse.create_comment(
                values, f"Coolant Off: {coolant_mode}"
            )
            gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
        gcode.append(f"{PostUtilsParse.linenumber(values)}M9{nl}")


def output_coolant_on(values: Values, gcode: Gcode, coolant_mode: str) -> None:
    """Output the commands to turn coolant on if necessary."""
    comment: str
    nl: str = "\n"

    if values["ENABLE_COOLANT"]:
        if values["OUTPUT_COMMENTS"] and coolant_mode != "None":
            comment = PostUtilsParse.create_comment(
                values, f"Coolant On: {coolant_mode}"
            )
            gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
        if coolant_mode == "Flood":
            gcode.append(f"{PostUtilsParse.linenumber(values)}M8{nl}")
        elif coolant_mode == "Mist":
            gcode.append(f"{PostUtilsParse.linenumber(values)}M7{nl}")


def output_end_bcnc(values: Values, gcode: Gcode) -> None:
    """Output the ending BCNC header."""
    comment: str
    nl: str = "\n"

    if values["OUTPUT_BCNC"]:
        comment = PostUtilsParse.create_comment(values, "Block-name: post_amble")
        gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
        comment = PostUtilsParse.create_comment(values, "Block-expand: 0")
        gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
        comment = PostUtilsParse.create_comment(values, "Block-enable: 1")
        gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")


def output_header(values: Values, gcode: Gcode) -> None:
    """Output the header."""
    cam_file: str
    comment: str
    nl: str = "\n"

    if not values["OUTPUT_HEADER"]:
        return
    comment = PostUtilsParse.create_comment(values, "Exported by FreeCAD")
    gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
    comment = PostUtilsParse.create_comment(
        values, f'Post Processor: {values["POSTPROCESSOR_FILE_NAME"]}'
    )
    gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
    if FreeCAD.ActiveDocument:
        cam_file = os.path.basename(FreeCAD.ActiveDocument.FileName)
    else:
        cam_file = "<None>"
    comment = PostUtilsParse.create_comment(values, f"Cam File: {cam_file}")
    gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
    comment = PostUtilsParse.create_comment(
        values, f"Output Time: {str(datetime.datetime.now())}"
    )
    gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")


def output_motion_mode(values: Values, gcode: Gcode) -> None:
    """Verify if PREAMBLE or SAFETYBLOCK have changed MOTION_MODE."""
    nl: str = "\n"

    if "G90" in values["PREAMBLE"] or "G90" in values["SAFETYBLOCK"]:
        values["MOTION_MODE"] = "G90"
    elif "G91" in values["PREAMBLE"] or "G91" in values["SAFETYBLOCK"]:
        values["MOTION_MODE"] = "G91"
    else:
        gcode.append(f'{PostUtilsParse.linenumber(values)}{values["MOTION_MODE"]}{nl}')


def output_postamble_header(values: Values, gcode: Gcode) -> None:
    """Output the postamble header."""
    comment: str = ""
    nl: str = "\n"

    if values["OUTPUT_COMMENTS"]:
        comment = PostUtilsParse.create_comment(values, "Begin postamble")
        gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")


def output_postamble(values: Values, gcode: Gcode) -> None:
    """Output the postamble."""
    line: str
    nl: str = "\n"

    for line in values["POSTAMBLE"].splitlines(False):
        gcode.append(f"{PostUtilsParse.linenumber(values)}{line}{nl}")


def output_postop(values: Values, gcode: Gcode, obj) -> None:
    """Output the post-operation information."""
    comment: str
    line: str
    nl: str = "\n"

    if values["OUTPUT_COMMENTS"]:
        comment = PostUtilsParse.create_comment(
            values, f'{values["FINISH_LABEL"]} operation: {obj.Label}'
        )
        gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
    for line in values["POST_OPERATION"].splitlines(False):
        gcode.append(f"{PostUtilsParse.linenumber(values)}{line}{nl}")


def output_preamble(values: Values, gcode: Gcode) -> None:
    """Output the preamble."""
    comment: str
    line: str
    nl: str = "\n"

    if values["OUTPUT_COMMENTS"]:
        comment = PostUtilsParse.create_comment(values, "Begin preamble")
        gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
    for line in values["PREAMBLE"].splitlines(False):
        gcode.append(f"{PostUtilsParse.linenumber(values)}{line}{nl}")


def output_preop(values: Values, gcode: Gcode, obj) -> None:
    """Output the pre-operation information."""
    comment: str
    line: str
    nl: str = "\n"

    if values["OUTPUT_COMMENTS"]:
        if values["SHOW_OPERATION_LABELS"]:
            comment = PostUtilsParse.create_comment(
                values, f"Begin operation: {obj.Label}"
            )
        else:
            comment = PostUtilsParse.create_comment(values, "Begin operation")
        gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
        if values["SHOW_MACHINE_UNITS"]:
            comment = PostUtilsParse.create_comment(
                values, f'Machine units: {values["UNIT_SPEED_FORMAT"]}'
            )
            gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
        if values["OUTPUT_MACHINE_NAME"]:
            comment = PostUtilsParse.create_comment(
                values,
                f'Machine: {values["MACHINE_NAME"]}, {values["UNIT_SPEED_FORMAT"]}',
            )
            gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
    for line in values["PRE_OPERATION"].splitlines(False):
        gcode.append(f"{PostUtilsParse.linenumber(values)}{line}{nl}")


def output_return_to(values: Values, gcode: Gcode) -> None:
    """Output the RETURN_TO command."""
    cmd: str
    nl: str = "\n"
    num_x: str
    num_y: str
    num_z: str

    if values["RETURN_TO"]:
        num_x = values["RETURN_TO"][0]
        num_y = values["RETURN_TO"][1]
        num_z = values["RETURN_TO"][2]
        cmd = PostUtilsParse.format_command_line(
            values, ["G0", f"X{num_x}", f"Y{num_y}", f"Z{num_z}"]
        )
        gcode.append(f"{PostUtilsParse.linenumber(values)}{cmd}{nl}")


def output_safetyblock(values: Values, gcode: Gcode) -> None:
    """Output the safety block."""
    line: str
    nl: str = "\n"

    for line in values["SAFETYBLOCK"].splitlines(False):
        gcode.append(f"{PostUtilsParse.linenumber(values)}{line}{nl}")


def output_start_bcnc(values: Values, gcode: Gcode, obj) -> None:
    """Output the starting BCNC header."""
    comment: str
    nl: str = "\n"

    if values["OUTPUT_BCNC"]:
        comment = PostUtilsParse.create_comment(values, f"Block-name: {obj.Label}")
        gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
        comment = PostUtilsParse.create_comment(values, "Block-expand: 0")
        gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")
        comment = PostUtilsParse.create_comment(values, "Block-enable: 1")
        gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")


def output_tool_list(values: Values, gcode: Gcode, objectslist) -> None:
    """Output a list of the tools used in the objects."""
    comment: str
    nl: str = "\n"

    if values["OUTPUT_COMMENTS"] and values["LIST_TOOLS_IN_PREAMBLE"]:
        for item in objectslist:
            if hasattr(item, "Proxy") and isinstance(
                item.Proxy, PathToolController.ToolController
            ):
                comment = PostUtilsParse.create_comment(
                    values, f"T{item.ToolNumber}={item.Name}"
                )
                gcode.append(f"{PostUtilsParse.linenumber(values)}{comment}{nl}")


def output_tool_return(values: Values, gcode: Gcode) -> None:
    """Output the tool return block."""
    line: str
    nl: str = "\n"

    for line in values["TOOLRETURN"].splitlines(False):
        gcode.append(f"{PostUtilsParse.linenumber(values)}{line}{nl}")


def output_units(values: Values, gcode: Gcode) -> None:
    """Verify if PREAMBLE or SAFETYBLOCK have changed UNITS."""
    nl: str = "\n"

    if "G21" in values["PREAMBLE"] or "G21" in values["SAFETYBLOCK"]:
        values["UNITS"] = "G21"
        values["UNIT_FORMAT"] = "mm"
        values["UNIT_SPEED_FORMAT"] = "mm/min"
    elif "G20" in values["PREAMBLE"] or "G20" in values["SAFETYBLOCK"]:
        values["UNITS"] = "G20"
        values["UNIT_FORMAT"] = "in"
        values["UNIT_SPEED_FORMAT"] = "in/min"
    else:
        gcode.append(f'{PostUtilsParse.linenumber(values)}{values["UNITS"]}{nl}')


def export_common(values: Values, objectslist, filename: str) -> str:
    """Do the common parts of postprocessing the objects in objectslist to filename."""
    coolant_mode: str
    dia: PostUtils.GCodeEditorDialog
    final: str
    gcode: Gcode = []
    result: bool

    for obj in objectslist:
        if not hasattr(obj, "Path"):
            print(f"The object {obj.Name} is not a path.")
            print("Please select only path and Compounds.")
            return ""

    print(f'PostProcessor:  {values["POSTPROCESSOR_FILE_NAME"]} postprocessing...')

    check_canned_cycles(values)
    output_header(values, gcode)
    output_safetyblock(values, gcode)
    output_tool_list(values, gcode, objectslist)
    output_preamble(values, gcode)
    output_motion_mode(values, gcode)
    output_units(values, gcode)

    for obj in objectslist:
        # Skip inactive operations
        if hasattr(obj, "Active") and not obj.Active:
            continue
        if hasattr(obj, "Base") and hasattr(obj.Base, "Active") and not obj.Base.Active:
            continue
        coolant_mode = determine_coolant_mode(obj)
        output_start_bcnc(values, gcode, obj)
        output_preop(values, gcode, obj)
        output_coolant_on(values, gcode, coolant_mode)
        # output the G-code for the group (compound) or simple path
        PostUtilsParse.parse_a_group(values, gcode, obj)
        output_postop(values, gcode, obj)
        output_coolant_off(values, gcode, coolant_mode)

    output_return_to(values, gcode)
    #
    # This doesn't make sense to me.  It seems that both output_start_bcnc and
    # output_end_bcnc should be in the for loop or both should be out of the
    # for loop.  However, that is the way that grbl post code was written, so
    # for now I will leave it that way until someone has time to figure it out.
    #
    output_end_bcnc(values, gcode)
    output_postamble_header(values, gcode)
    output_tool_return(values, gcode)
    output_safetyblock(values, gcode)
    output_postamble(values, gcode)

    final = "".join(gcode)

    if FreeCAD.GuiUp and values["SHOW_EDITOR"]:
        if len(final) > 100000:
            print("Skipping editor since output is greater than 100kb")
        else:
            dia = PostUtils.GCodeEditorDialog()
            dia.editor.setText(final)
            result = dia.exec_()
            if result:
                final = dia.editor.toPlainText()

    print("done postprocessing.")

    if not filename == "-":
        with open(
            filename, "w", encoding="utf-8", newline=values["END_OF_LINE_CHARACTERS"]
        ) as gfile:
            gfile.write(final)

    return final
