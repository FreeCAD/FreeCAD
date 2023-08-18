# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
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


from PySide import QtCore, QtGui
import FreeCAD
import FreeCADGui
import Path
from PySide.QtCore import QT_TRANSLATE_NOOP
import PathScripts.PathUtils as PathUtils

translate = FreeCAD.Qt.translate


class GCodeHighlighter(QtGui.QSyntaxHighlighter):
    def __init__(self, parent=None):
        def convertcolor(c):
            return QtGui.QColor(
                int((c >> 24) & 0xFF), int((c >> 16) & 0xFF), int((c >> 8) & 0xFF)
            )

        super(GCodeHighlighter, self).__init__(parent)
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Editor")
        colors = []
        c = p.GetUnsigned("Number")
        if c:
            colors.append(convertcolor(c))
        else:
            colors.append(QtCore.Qt.red)
        c = p.GetUnsigned("Keyword")
        if c:
            colors.append(convertcolor(c))
        else:
            colors.append(QtGui.QColor(0, 170, 0))
        c = p.GetUnsigned("Define name")
        if c:
            colors.append(convertcolor(c))
        else:
            colors.append(QtGui.QColor(160, 160, 164))

        self.highlightingRules = []
        numberFormat = QtGui.QTextCharFormat()
        numberFormat.setForeground(colors[0])
        self.highlightingRules.append(
            (QtCore.QRegularExpression("[\\-0-9\\.]"), numberFormat)
        )
        keywordFormat = QtGui.QTextCharFormat()
        keywordFormat.setForeground(colors[1])
        keywordFormat.setFontWeight(QtGui.QFont.Bold)
        keywordPatterns = ["\\bG[0-9]+\\b", "\\bM[0-9]+\\b"]
        self.highlightingRules.extend(
            [
                (QtCore.QRegularExpression(pattern), keywordFormat)
                for pattern in keywordPatterns
            ]
        )
        speedFormat = QtGui.QTextCharFormat()
        speedFormat.setFontWeight(QtGui.QFont.Bold)
        speedFormat.setForeground(colors[2])
        self.highlightingRules.append(
            (QtCore.QRegularExpression("\\bF[0-9\\.]+\\b"), speedFormat)
        )

    def highlightBlock(self, text):

        for pattern, fmt in self.highlightingRules:
            expression = QtCore.QRegularExpression(pattern)
            index = expression.match(text)
            while index.hasMatch():
                length = index.capturedLength()
                self.setFormat(index.capturedStart(), length, fmt)
                index = expression.match(text, index.capturedStart() + length)


class GCodeEditorDialog(QtGui.QDialog):
    tool = None

    def __init__(self, PathObj, parent=FreeCADGui.getMainWindow()):
        self.PathObj = PathObj
        if hasattr(PathObj, "ToolController"):
            self.tool = PathObj.ToolController.Tool
        else:
            self.tool = None

        QtGui.QDialog.__init__(self, parent)
        layout = QtGui.QVBoxLayout(self)

        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        c = p.GetUnsigned("DefaultHighlightPathColor", 4286382335)
        Q = QtGui.QColor(
            int((c >> 24) & 0xFF), int((c >> 16) & 0xFF), int((c >> 8) & 0xFF)
        )
        highlightcolor = (
            Q.red() / 255.0,
            Q.green() / 255.0,
            Q.blue() / 255.0,
            Q.alpha() / 255.0,
        )

        self.selectionobj = FreeCAD.ActiveDocument.addObject(
            "Path::Feature", "selection"
        )
        self.selectionobj.ViewObject.LineWidth = 4
        self.selectionobj.ViewObject.NormalColor = highlightcolor

        # nice text editor widget for editing the gcode
        self.editor = QtGui.QTextEdit()
        font = QtGui.QFont()
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Editor")
        font.setFamily(p.GetString("Font", "Courier"))
        font.setFixedPitch(True)
        font.setPointSize(p.GetInt("FontSize", 10))
        self.editor.setFont(font)
        self.editor.setText("G01 X55 Y4.5 F300.0")
        layout.addWidget(self.editor)

        # Note
        lab = QtGui.QLabel()
        lab.setText(
            translate(
                "Path_Inspect",
                "<b>Note</b>: This dialog shows Path Commands in FreeCAD base units (mm/s). \n Values will be converted to the desired unit during post processing.",
            )
        )
        lab.setWordWrap(True)
        layout.addWidget(lab)

        # OK and Cancel buttons
        self.buttons = QtGui.QDialogButtonBox(
            QtGui.QDialogButtonBox.Close,
            QtCore.Qt.Horizontal,
            self,
        )

        layout.addWidget(self.buttons)
        self.buttons.rejected.connect(self.reject)
        self.editor.selectionChanged.connect(self.highlightpath)
        self.finished.connect(self.cleanup)

        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        Xpos = int(prefs.GetString("inspecteditorX", "0"))
        Ypos = int(prefs.GetString("inspecteditorY", "0"))
        height = int(prefs.GetString("inspecteditorH", "500"))
        width = int(prefs.GetString("inspecteditorW", "600"))
        self.move(Xpos, Ypos)
        self.resize(width, height)

    def cleanup(self):
        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        prefs.SetString("inspecteditorX", str(self.x()))
        prefs.SetString("inspecteditorY", str(self.y()))
        prefs.SetString("inspecteditorW", str(self.width()))
        prefs.SetString("inspecteditorH", str(self.height()))
        FreeCAD.ActiveDocument.removeObject(self.selectionobj.Name)

    def highlightpath(self):
        cursor = self.editor.textCursor()
        sp = cursor.selectionStart()
        ep = cursor.selectionEnd()
        cursor.setPosition(sp)
        startrow = cursor.blockNumber()
        cursor.setPosition(ep)
        endrow = cursor.blockNumber()

        commands = PathUtils.getPathWithPlacement(self.PathObj).Commands

        # Derive the starting position for the first selected command
        prevX = prevY = prevZ = None
        prevcommands = commands[:startrow]
        prevcommands.reverse()
        for c in prevcommands:
            if prevX is None:
                if c.Parameters.get("X") is not None:
                    prevX = c.Parameters.get("X")
            if prevY is None:
                if c.Parameters.get("Y") is not None:
                    prevY = c.Parameters.get("Y")
            if prevZ is None:
                if c.Parameters.get("Z") is not None:
                    prevZ = c.Parameters.get("Z")
            if prevX is not None and prevY is not None and prevZ is not None:
                break
        if prevX is None:
            prevX = 0.0
        if prevY is None:
            prevY = 0.0
        if prevZ is None:
            prevZ = 0.0

        # Build a new path with selection
        p = Path.Path()
        firstrapid = Path.Command("G0", {"X": prevX, "Y": prevY, "Z": prevZ})

        selectionpath = [firstrapid] + commands[startrow : endrow + 1]
        p.Commands = selectionpath
        self.selectionobj.Path = p

        if self.tool is not None:
            self.tool.Placement.Base.x = prevX
            self.tool.Placement.Base.y = prevY
            self.tool.Placement.Base.z = prevZ


def show(obj):
    "show(obj): shows the G-code data of the given Path object in a dialog"

    prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
    # default Max Highlighter Size = 512 Ko
    defaultMHS = 512 * 1024
    mhs = prefs.GetUnsigned("inspecteditorMaxHighlighterSize", defaultMHS)

    if hasattr(obj, "Path"):
        if obj.Path:
            dia = GCodeEditorDialog(obj)
            dia.editor.setText(obj.Path.toGCode())
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
            # exec_() returns 0 or 1 depending on the button pressed (Ok or
            # Cancel)
            if result:
                p = Path.Path(dia.editor.toPlainText())
                FreeCAD.ActiveDocument.openTransaction("Edit Path")
                obj.Path = p
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()


class CommandPathInspect:
    def GetResources(self):
        return {
            "Pixmap": "Path_Inspect",
            "MenuText": QT_TRANSLATE_NOOP("Path_Inspect", "Inspect Path Commands"),
            "Accel": "P, I",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Path_Inspect", "Inspects the contents of a Path object"
            ),
        }

    def IsActive(self):
        obj = FreeCADGui.Selection.getSelection()[0]
        return hasattr(obj, "Path") and len(obj.Path.Commands) > 0

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(
                translate("Path_Inspect", "Please select exactly one path object")
                + "\n"
            )
            return
        if not (selection[0].isDerivedFrom("Path::Feature")):
            FreeCAD.Console.PrintError(
                translate("Path_Inspect", "Please select exactly one path object")
                + "\n"
            )
            return

        # if everything is ok, execute
        FreeCADGui.addModule("Path.Main.Gui.Inspect")
        FreeCADGui.doCommand(
            "Path.Main.Gui.Inspect.show(FreeCAD.ActiveDocument."
            + selection[0].Name
            + ")"
        )


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("Path_Inspect", CommandPathInspect())
