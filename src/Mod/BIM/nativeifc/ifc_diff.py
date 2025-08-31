# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""Diffing tool for NativeIFC project objects"""

import difflib

import ifcopenshell

import FreeCAD
import FreeCADGui
import Arch_rc

from . import ifc_tools

translate = FreeCAD.Qt.translate


def get_diff(proj):
    """Obtains a diff between the current version and the saved version of a project"""

    if not getattr(proj, "IfcFilePath", None):
        old = []
        return 1
    else:
        # cannot use open() here as it gives different encoding
        # than ifcopenshell and diff does not work
        f = ifcopenshell.open(proj.IfcFilePath)
        old = f.wrapped_data.to_string().split("\n")
    if not old:
        return ""
    ifcfile = ifc_tools.get_ifcfile(proj)
    if not ifcfile:
        return ""
    new = ifcfile.wrapped_data.to_string().split("\n")
    # diff = difflib.HtmlDiff().make_file(old,new) # UGLY
    res = [l for l in difflib.unified_diff(old, new, lineterm="")]
    res = [l for l in res if l.startswith("+") or l.startswith("-")]
    res = [l for l in res if not l.startswith("+++") and not l.startswith("---")]
    return "\n".join(res)


def htmlize(diff):
    """Returns an HTML version of a diff list"""

    html = "<html><body>\n"
    if diff == 1:
        html += translate("BIM", "The IFC file is not saved. Save once"
        " to have an existing IFC file to compare with."
        " Then, run this command again.") + "<br/>\n"
    elif diff:
        diff = diff.split("\n")
        for l in diff:
            if l.startswith("+"):
                html += "<span style='color:green;'>" + l[:100] + "</span><br/>\n"
            elif l.startswith("-"):
                html += "<span style='color:red;'>" + l[:100] + "</span><br/>\n"
            else:
                html += l + "<br/>\n"
    else:
        html += translate("BIM", "No changes to display.") + "<br/>\n"
    html += "</body></html>"
    return html


def show_diff(diff):
    """Shows a dialog showing the diff contents"""

    dlg = FreeCADGui.PySideUic.loadUi(":/ui/dialogDiff.ui")
    dlg.textEdit.setStyleSheet("background-color: white; color: black;")
    dlg.textEdit.setHtml(htmlize(diff))
    result = dlg.exec_()
