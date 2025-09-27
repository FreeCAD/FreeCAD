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


from Path.Base.MachineState import MachineState
from PySide import QtCore, QtGui

from PySide.QtWidgets import QWidget, QPlainTextEdit
from PySide.QtGui import QPainter
from PySide.QtCore import Qt, QRect, QSize

import FreeCAD
import Part
import Path
import os
import re

debug = False
if debug:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate

FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui


class FilenameGenerator:
    def __init__(self, job):
        self.job = job
        self.subpartname = ""
        self.sequencenumber = 0
        path, filename, ext = self.get_path_and_filename_default()

        self.qualified_path = self._apply_path_substitutions(path)
        self.qualified_filename = self._apply_filename_substitutions(filename)
        self.extension = ext

    def get_path_and_filename_default(self):
        outputpath = ""
        filename = ""
        ext = ".nc"

        validPathSubstitutions = ["D", "d", "M", "j"]
        validFilenameSubstitutions = ["j", "d", "T", "t", "W", "O", "S"]

        if self.job.PostProcessorOutputFile:
            candidateOutputPath, candidateFilename = os.path.split(self.job.PostProcessorOutputFile)

            if candidateOutputPath:
                outputpath = candidateOutputPath

            if candidateFilename:
                filename, ext = os.path.splitext(candidateFilename)
        else:
            outputpath, filename = os.path.split(Path.Preferences.defaultOutputFile())
            filename, ext = os.path.splitext(filename)

        # Make sure we have something to work with
        if not filename:
            filename = FreeCAD.ActiveDocument.Label

        if not outputpath:
            outputpath, _ = os.path.split(FreeCAD.ActiveDocument.getFileName())

        if not outputpath:
            outputpath = (
                os.getcwd()
            )  ## TODO: This should be avoided as it gives the Freecad executable's path in some systems (e.g. Windows)

        if not ext:
            ext = ".nc"

        # Check for invalid matches
        for match in re.findall(r"%(.)", outputpath):
            Path.Log.debug(f"match: {match}")
            if match not in validPathSubstitutions:
                outputpath = outputpath.replace(f"%{match}", "")
                FreeCAD.Console.PrintWarning(
                    "Invalid substitution strings will be ignored in output path: %s\n" % match
                )

        for match in re.findall(r"%(.)", filename):
            Path.Log.debug(f"match: {match}")
            if match not in validFilenameSubstitutions:
                filename = filename.replace(f"%{match}", "")
                FreeCAD.Console.PrintWarning(
                    "Invalid substitution strings will be ignored in file path: %s\n" % match
                )

        Path.Log.debug(f"outputpath: {outputpath} filename: {filename} ext: {ext}")
        return outputpath, filename, ext

    def set_subpartname(self, subpartname):
        self.subpartname = subpartname

    def _apply_path_substitutions(self, file_path):
        """Apply substitutions based on job settings and other parameters."""
        substitutions = {
            "%D": os.path.dirname(self.job.Document.FileName or "."),
            "%d": self.job.Document.Label,
            "%j": self.job.Label,
            "%M": os.path.dirname(FreeCAD.getUserMacroDir()),
        }
        for key, value in substitutions.items():
            file_path = file_path.replace(key, value)

        Path.Log.debug(f"file_path: {file_path}")
        return file_path

    def _apply_filename_substitutions(self, file_name):
        Path.Log.debug(f"file_name: {file_name}")
        """Apply substitutions based on job settings and other parameters."""
        substitutions = {
            "%d": self.job.Document.Label,
            "%j": self.job.Label,
            "%T": self.subpartname,  # Tool Number
            "%t": self.subpartname,  # Tool Controller Label
            "%W": self.subpartname,  # Fixture
            "%O": self.subpartname,  # Operation
        }
        for key, value in substitutions.items():
            file_name = file_name.replace(key, value)

        Path.Log.debug(f"file_name: {file_name}")
        return file_name

    def generate_filenames(self):
        """Yield filenames indefinitely with proper substitutions."""
        while True:
            temp_filename = self.qualified_filename
            Path.Log.debug(f"temp_filename: {temp_filename}")
            explicit_sequence = False
            matches = re.findall(r"%S", temp_filename)
            if matches:
                Path.Log.debug(f"matches: {matches}")
                temp_filename = re.sub(r"%S", str(self.sequencenumber), temp_filename)
                explicit_sequence = True

            subpart = f"-{self.subpartname}" if self.subpartname else ""
            sequence = (
                f"-{self.sequencenumber}" if not explicit_sequence and self.sequencenumber else ""
            )
            filename = f"{temp_filename}{subpart}{sequence}{self.extension}"

            # Trim leading dash if filename starts with one
            if filename.startswith("-"):
                filename = filename[1:]
            full_path = os.path.join(self.qualified_path, filename)

            self.sequencenumber += 1
            Path.Log.debug(f"yielding filename: {full_path}")
            yield os.path.normpath(full_path)


class GCodeHighlighter(QtGui.QSyntaxHighlighter):
    def __init__(self, parent=None):
        super(GCodeHighlighter, self).__init__(parent)

        keywordFormat = QtGui.QTextCharFormat()
        keywordFormat.setForeground(QtCore.Qt.cyan)
        keywordFormat.setFontWeight(QtGui.QFont.Bold)
        keywordPatterns = ["\\bG[0-9]+\\b", "\\bM[0-9]+\\b"]

        self.highlightingRules = [
            (QtCore.QRegularExpression(pattern), keywordFormat) for pattern in keywordPatterns
        ]

        speedFormat = QtGui.QTextCharFormat()
        speedFormat.setFontWeight(QtGui.QFont.Bold)
        speedFormat.setForeground(QtCore.Qt.green)
        self.highlightingRules.append((QtCore.QRegularExpression("\\bF[0-9\\.]+\\b"), speedFormat))

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
        # self.editor = QtGui.QTextEdit()
        self.editor = CodeEditor()
        font = QtGui.QFont()
        font.setFamily("Courier")
        font.setFixedPitch(True)
        font.setPointSize(10)
        self.editor.setFont(font)
        # self.editor.setText("G01 X55 Y4.5 F300.0")
        self.editor.setPlainText("G01 X55 Y4.5 F300.0")
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


class LineNumberArea(QWidget):
    def __init__(self, editor):
        super().__init__(editor)
        self.editor = editor

    def sizeHint(self):
        return QSize(self.editor.fontMetrics().horizontalAdvance("999") + 4, 0)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.fillRect(event.rect(), Qt.black)

        block = self.editor.firstVisibleBlock()
        blockNumber = block.blockNumber()
        top = self.editor.blockBoundingGeometry(block).translated(self.editor.contentOffset()).top()
        bottom = top + self.editor.blockBoundingRect(block).height()

        while block.isValid() and top <= event.rect().bottom():
            if block.isVisible() and bottom >= event.rect().top():
                number = str(blockNumber + 1)
                painter.setPen(Qt.gray)
                painter.drawText(
                    -6,
                    int(top),
                    self.width(),
                    self.editor.fontMetrics().height(),
                    Qt.AlignRight,
                    number,
                )

            block = block.next()
            top = bottom
            bottom = top + self.editor.blockBoundingRect(block).height()
            blockNumber += 1


class CodeEditor(QPlainTextEdit):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.lineNumberArea = LineNumberArea(self)
        self.blockCountChanged.connect(self.updateLineNumberAreaWidth)
        self.updateRequest.connect(self.updateLineNumberArea)
        self.verticalScrollBar().valueChanged.connect(self.lineNumberArea.update)  # Simple update

        self.updateLineNumberAreaWidth(0)  # Initial call

    def lineNumberAreaWidth(self):
        digits = 2
        max_value = max(1, self.blockCount())
        while max_value >= 10:
            max_value /= 10
            digits += 1
        space = 3 + self.fontMetrics().horizontalAdvance("9") * digits
        return space

    def updateLineNumberAreaWidth(self, newBlockCount):
        self.setViewportMargins(self.lineNumberAreaWidth(), 0, 0, 0)

    def updateLineNumberArea(self, rect, dy):
        if dy:
            self.lineNumberArea.scroll(0, dy)
        else:
            self.lineNumberArea.update(0, rect.y(), self.lineNumberArea.width(), rect.height())

        if rect.contains(self.viewport().rect()):
            self.updateLineNumberAreaWidth(0)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        cr = self.contentsRect()
        self.lineNumberArea.setGeometry(
            QRect(cr.left(), cr.top(), self.lineNumberAreaWidth(), cr.height())
        )


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
                "GCode size too big ({} o), disabling syntax highlighter.".format(gcodeSize),
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
