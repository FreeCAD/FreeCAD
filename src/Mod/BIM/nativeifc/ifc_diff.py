# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU General Public License (GPL)            *
# *   as published by the Free Software Foundation; either version 3 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU General Public License for more details.                          *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Diffing tool for NativeIFC project objects"""

import difflib
import FreeCAD
import FreeCADGui
import ifcopenshell
from nativeifc import ifc_tools
import Arch_rc

translate = FreeCAD.Qt.translate


def get_diff(proj):
    """Obtains a diff between the current version and the saved version of a project"""

    if not getattr(proj, "IfcFilePath", None):
        old = []
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
    if diff:
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
