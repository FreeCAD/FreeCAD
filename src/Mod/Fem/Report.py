# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Markus Hovorka <m.hovorka@live.de>               *
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

__title__ = "Report"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


from PySide import QtCore
from PySide import QtGui

import FreeCAD as App
import FreeCADGui as Gui
from FreeCAD import Console


ERROR_COLOR = "red"
WARNING_COLOR = "yellow"
INFO_COLOR = "blue"


MSG_LOOKUP = {
    "wd_not_existent": "Working directory {} doesn't exist.",
    "wd_not_directory": "Working directory {} is not a directory.",
    "cd_not_directory": "Case directory {} is not a directory.",
    "must_save": "Please save the document before preceding.",
    "cd_not_created": "Failed to creade case directory {}: {}",
    "dir_not_created": "Failed to create directory {}: {}",
    "elmersolver_not_found": "ElmerSolver binary not found (required).",
    "elmergrid_not_found": "ElmerGrid binary not found (required).",
    "elmer_failed": "ElmerSolver failed with error code: ",
    "mesh_missing": "Mesh object missing.",
    "too_many_meshes": "Analysis contains more than one mesh (unsupported).",
    "material_missing": "Material object missing.",
    "too_many_materials": "Analysis contains more than one material (unsupported).",
    "unsupported_mesh": ("Unsupported type of mesh. Currently only Gmsh meshes"
        " are supported for ElmerSolver."),
    "unsupported_constraint": "Unsupported constraint {} of type {}.",
    "solve_first": ("Couldn't locate result file. "
        "Either the label of the analysis changed or it wasn't solved jet.")
}


def display(report, title=None, text=None):
    if App.GuiUp:
        displayGui(report, title, text)
    else:
        displayLog(report)


def displayGui(report, title=None, text=None):
    if not report.isEmpty():
        mw = Gui.getMainWindow()
        dialog = ReportDialog(mw, report, title, text)


def displayLog(report):
    for i in report.infos:
        Console.PrintLog("%s\n" % i)
    for w in report.warnings:
        Console.PrintWarning("%s\n" % w)
    for e in report.errors:
        Console.PrintError("%s\n" % e)


class Data(object):

    def __init__(self):
        self.infos = []
        self.warnings = []
        self.errors = []

    def extend(report):
        self.infos.extend(report.infos)
        self.warnings.extend(report.warnings)
        self.errors.extend(report.errors)

    def isValid(self):
        return len(self.errors) == 0

    def isEmpty(self):
        return not (self.infos or self.warnings or self.errors)

    def appendInfo(self, info, *args):
        self._append(self.infos, info, args)

    def appendWarning(self, warning, *args):
        self._append(self.warnings, warning, args)

    def appendError(self, error, *args):
        self._append(self.errors, error, args)

    def _append(self, msgList, msgKey, args):
        if msgKey not in MSG_LOOKUP:
            raise ValueError("Unknown error: %s" % msgKey)
        textFormatStr = MSG_LOOKUP[msgKey]
        msgList.append(textFormatStr.format(*args))


class ReportDialog(QtGui.QDialog):

    def __init__(self, parent, report, title="Report", text=None):
        super(ReportDialog, self).__init__(parent)
        msgDetails = QtGui.QTextEdit()
        msgDetails.setReadOnly(True)
        msgDetails.setHtml(self._getText(report))
        bttBox = QtGui.QDialogButtonBox(QtGui.QDialogButtonBox.Ok)
        bttBox.accepted.connect(self.close)
        layout = QtGui.QVBoxLayout()
        if text is not None:
            textLbl = QtGui.QLabel(text)
            textLbl.setWordWrap(True)
            layout.addWidget(textLbl)
        layout.addWidget(msgDetails)
        layout.addWidget(bttBox)
        self.setWindowTitle(title)
        self.setLayout(layout)
        self.resize(300, 200)
        self.exec_()

    def _getText(self, report):
        text = ""
        for i in report.infos:
            line = "<b>Info:</b> %s" % i
            text += "%s<br>" % self._getColoredLine(line, INFO_COLOR)
        for w in report.warnings:
            line = "<b>Warning:</b> %s" % w
            text += "%s<br>" % self._getColoredLine(line, WARNING_COLOR)
        for e in report.errors:
            line = "<b>Error:</b> %s" % e
            text += "%s<br>" % self._getColoredLine(line, ERROR_COLOR)
        return text

    def _getColoredLine(self, text, color):
        return '<font color="%s">%s</font>' % (color, text)
