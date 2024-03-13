# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

"""
These are common functions and classes for creating custom post processors.
"""

from PySide import QtCore, QtGui

import FreeCAD

import Path
import Part

from Path.Base.MachineState import MachineState

translate = FreeCAD.Qt.translate
FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui


class GCodeHighlighter(QtGui.QSyntaxHighlighter):
    def __init__(self, parent=None):
        super(GCodeHighlighter, self).__init__(parent)

        keywordFormat = QtGui.QTextCharFormat()
        keywordFormat.setForeground(QtCore.Qt.cyan)
        keywordFormat.setFontWeight(QtGui.QFont.Bold)
        keywordPatterns = ["\\bG[0-9]+\\b", "\\bM[0-9]+\\b"]

        self.highlightingRules = [
            (QtCore.QRegularExpression(pattern), keywordFormat)
            for pattern in keywordPatterns
        ]

        speedFormat = QtGui.QTextCharFormat()
        speedFormat.setFontWeight(QtGui.QFont.Bold)
        speedFormat.setForeground(QtCore.Qt.green)
        self.highlightingRules.append(
            (QtCore.QRegularExpression("\\bF[0-9\\.]+\\b"), speedFormat)
        )

    def highlightBlock(self, text):
        for pattern, hlFormat in self.highlightingRules:
            expression = QtCore.QRegularExpression(pattern)
            index = expression.match(text)
            while index.hasMatch():
                length = index.capturedLength()
                self.setFormat(index.capturedStart(), length, hlFormat)
                index = expression.match(text, index.capturedStart() + length)


class GCodeEditorDialog(QtGui.QDialog):
    def __init__(self, parent=None):
        if parent is None:
            parent = FreeCADGui.getMainWindow()
        QtGui.QDialog.__init__(self, parent)

        layout = QtGui.QVBoxLayout(self)

        # nice text editor widget for editing the gcode
        self.editor = QtGui.QTextEdit()
        font = QtGui.QFont()
        font.setFamily("Courier")
        font.setFixedPitch(True)
        font.setPointSize(10)
        self.editor.setFont(font)
        self.editor.setText("G01 X55 Y4.5 F300.0")
        layout.addWidget(self.editor)

        # OK and Cancel buttons
        self.buttons = QtGui.QDialogButtonBox(
            QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel,
            QtCore.Qt.Horizontal,
            self,
        )
        layout.addWidget(self.buttons)

        # restore placement and size
        self.paramKey = "User parameter:BaseApp/Values/Mod/CAM/GCodeEditor/"
        params = FreeCAD.ParamGet(self.paramKey)
        posX = params.GetInt("posX")
        posY = params.GetInt("posY")
        if posX > 0 and posY > 0:
            self.move(posX, posY)
        width = params.GetInt("width")
        height = params.GetInt("height")
        if width > 0 and height > 0:
            self.resize(width, height)

        self.buttons.accepted.connect(self.accept)
        self.buttons.rejected.connect(self.reject)

    def done(self, *args, **kwargs):
        params = FreeCAD.ParamGet(self.paramKey)
        params.SetInt("posX", self.x())
        params.SetInt("posY", self.y())
        params.SetInt("width", self.size().width())
        params.SetInt("height", self.size().height())
        return QtGui.QDialog.done(self, *args, **kwargs)


def stringsplit(commandline):
    returndict = {
        "command": None,
        "X": None,
        "Y": None,
        "Z": None,
        "A": None,
        "B": None,
        "F": None,
        "T": None,
        "S": None,
        "I": None,
        "J": None,
        "K": None,
        "txt": None,
    }
    wordlist = [a.strip() for a in commandline.split(" ")]
    if wordlist[0][0] == "(":
        returndict["command"] = "message"
        returndict["txt"] = wordlist[0]
    else:
        returndict["command"] = wordlist[0]
    for word in wordlist[1:]:
        returndict[word[0]] = word[1:]

    return returndict


def fmt(num, dec, units):
    """Use to format axis moves, feedrate, etc for decimal places and units."""
    if units == "G21":  # metric
        fnum = "%.*f" % (dec, num)
    else:  # inch
        fnum = "%.*f" % (dec, num / 25.4)  # since FreeCAD uses metric units internally
    return fnum


def editor(gcode):
    """Pops up a handy little editor to look at the code output."""
    prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
    # default Max Highlighter Size = 512 Ko
    defaultMHS = 512 * 1024
    mhs = prefs.GetUnsigned("inspecteditorMaxHighlighterSize", defaultMHS)

    dia = GCodeEditorDialog()
    dia.editor.setText(gcode)
    gcodeSize = len(dia.editor.toPlainText())
    if gcodeSize <= mhs:
        # because of poor performance, syntax highlighting is
        # limited to mhs octets (default 512 KB).
        # It seems than the response time curve has an inflexion near 500 KB
        # beyond 500 KB, the response time increases exponentially.
        dia.highlighter = GCodeHighlighter(dia.editor.document())
    else:
        FreeCAD.Console.PrintMessage(
            translate(
                "Path",
                "GCode size too big ({} o), disabling syntax highlighter.".format(
                    gcodeSize
                ),
            )
        )
    result = dia.exec_()
    if result:  # If user selected 'OK' get modified G Code
        final = dia.editor.toPlainText()
    else:
        final = gcode
    return final


def fcoms(string, commentsym):
    """Filter and rebuild comments with user preferred comment symbol."""
    if len(commentsym) == 1:
        s1 = string.replace("(", commentsym)
        comment = s1.replace(")", "")
    else:
        return string
    return comment


def splitArcs(path):
    """Filter a path object and replace all G2/G3 moves with discrete G1 moves.

    Returns a Path object.
    """
    prefGrp = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
    deflection = prefGrp.GetFloat("LibAreaCurveAccuarcy", 0.01)

    results = []
    if not isinstance(path, Path.Path):
        raise TypeError("path must be a Path object")

    machine = MachineState()
    for command in path.Commands:

        if command.Name not in Path.Geom.CmdMoveArc:
            machine.addCommand(command)
            results.append(command)
            continue

        edge = Path.Geom.edgeForCmd(command, machine.getPosition())
        pts = edge.discretize(Deflection=deflection)
        edges = [Part.makeLine(v1, v2) for v1, v2 in zip(pts, pts[1:])]
        for edge in edges:
            results.extend(Path.Geom.cmdsForEdge(edge))

        machine.addCommand(command)

    return Path.Path(results)
