# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2015 Yorik van Havre yorik@uncreated.net
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

from PySide import QtCore, QtGui
import FreeCAD
import FreeCADGui
import Path
from PySide.QtCore import QT_TRANSLATE_NOOP
from PathScripts import PathUtils
from Path.Base.Util import toolControllerForOp
from Path.Main.Gui.Editor import CodeEditor

translate = FreeCAD.Qt.translate


class GCodeEditorDialog(QtGui.QDialog):
    tool = None

    def __init__(self, PathObj, parent=None, readOnly=True, toolVisibility=None):
        self.commands = PathUtils.getPathWithPlacement(PathObj).Commands
        self.tool = getattr(toolControllerForOp(PathObj), "Tool", None)
        self.toolInitVisibility = getattr(self.tool, "Visibility", False)  # keep tool visibility

        QtGui.QDialog.__init__(self, parent or FreeCADGui.getMainWindow())
        self.setWindowTitle(translate("CAM", "CAM Inspect"))
        layout = QtGui.QVBoxLayout(self)

        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        c = p.GetUnsigned("DefaultHighlightPathColor", 4286382335)
        Q = QtGui.QColor(int((c >> 24) & 0xFF), int((c >> 16) & 0xFF), int((c >> 8) & 0xFF))
        highlightcolor = (
            Q.red() / 255.0,
            Q.green() / 255.0,
            Q.blue() / 255.0,
            Q.alpha() / 255.0,
        )

        self.selectionobj = FreeCAD.ActiveDocument.addObject("Path::Feature", "selection")
        self.selectionobj.ViewObject.LineWidth = 4
        self.selectionobj.ViewObject.NormalColor = highlightcolor

        self.editor = CodeEditor()
        if readOnly:
            self.editor.setTextInteractionFlags(
                QtGui.Qt.TextInteractionFlag.TextSelectableByMouse
                | QtGui.Qt.TextInteractionFlag.TextSelectableByKeyboard
            )  #  make a QPlainTextEdit read-only while keeping the text cursor visible
        layout.addWidget(self.editor)

        # Note
        lab = QtGui.QLabel()
        lab.setText(
            translate(
                "CAM_Inspect",
                "<b>Note</b>: This dialog shows path commands in FreeCAD base units (mm/s)."
                "<br>Values will be converted to the desired unit during post processing.",
            )
        )
        lab.setWordWrap(True)
        layout.addWidget(lab)

        bottomFrame = QtGui.QFrame()
        bottomFrame.setLayout(QtGui.QHBoxLayout())
        layout.addWidget(bottomFrame)

        self.chkTool = QtGui.QCheckBox("Show tool")
        if self.tool is not None:
            self.chkTool.setText(translate("CAM_Inspect", "Show tool: %s") % self.tool.Label)
            self.chkTool.setToolTip(
                translate(
                    "CAM_Inspect",
                    "Show tool shape\nG-code under the cursor defines tool shape placement",
                )
            )
            if toolVisibility is not None:
                self.chkTool.setChecked(toolVisibility)
            else:
                self.chkTool.setChecked(self.tool.Visibility)
            bottomFrame.layout().addWidget(self.chkTool)

        self.buttons = QtGui.QDialogButtonBox(
            QtGui.QDialogButtonBox.Close,
            QtCore.Qt.Horizontal,
            self,
        )  # add Close button
        self.buttons.rejected.connect(self.reject)
        bottomFrame.layout().addWidget(self.buttons)

        self.editor.cursorPositionChanged.connect(self.highlightpath)
        self.editor.cursorPositionChanged.connect(self.toolPlacement)
        self.finished.connect(self.cleanup)
        self.chkTool.checkStateChanged.connect(self.toolVisibility)

        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        Xpos = int(prefs.GetString("inspecteditorX", "0"))
        Ypos = int(prefs.GetString("inspecteditorY", "0"))
        height = int(prefs.GetString("inspecteditorH", "500"))
        width = int(prefs.GetString("inspecteditorW", "600"))
        self.move(Xpos, Ypos)
        self.resize(width, height)

    def cleanup(self):
        """Prepare for exit from Inspect"""
        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        prefs.SetString("inspecteditorX", str(self.x()))
        prefs.SetString("inspecteditorY", str(self.y()))
        prefs.SetString("inspecteditorW", str(self.width()))
        prefs.SetString("inspecteditorH", str(self.height()))
        FreeCAD.ActiveDocument.removeObject(self.selectionobj.Name)
        if self.tool:
            self.tool.Visibility = self.toolInitVisibility  # restore tool visibility

    def highlightpath(self):
        """Set highlighted path"""
        cursor = self.editor.textCursor()
        sp = cursor.selectionStart()
        ep = cursor.selectionEnd()
        cursor.setPosition(sp)
        startrow = cursor.blockNumber()
        cursor.setPosition(ep)
        endrow = cursor.blockNumber()

        # Derive the starting position for the first selected command
        x, y, z = self.getPosition(self.commands[max(0, startrow - 1) :: -1])
        firstrapid = Path.Command("G0", {"X": x, "Y": y, "Z": z})
        selectionCommands = [firstrapid] + self.commands[startrow : endrow + 1]
        self.selectionobj.Path = Path.Path()
        if len(selectionCommands) > 1:
            self.selectionobj.Path = Path.Path(selectionCommands)
        self.selectionobj.ViewObject.StartIndex = 1  # hide first rapid move

    def toolPlacement(self):
        """Set tool placement"""
        if self.tool is not None and self.tool.Visibility:
            line_number = self.editor.textCursor().blockNumber()
            self.tool.Placement.Base = self.getPosition(self.commands[line_number::-1])

    def toolVisibility(self):
        """Update tool visibility"""
        if self.tool is not None:
            self.tool.Visibility = self.chkTool.isChecked()

    def getPosition(self, commands):
        """Define position from commands list"""
        x = y = z = None
        for cmd in commands:
            x = cmd.x if x is None and cmd.x is not None else x
            y = cmd.y if y is None and cmd.y is not None else y
            z = cmd.z if z is None and cmd.z is not None else z
            if x is not None and y is not None and z is not None:
                break

        x = 0 if x is None else x
        y = 0 if y is None else y
        z = 0 if z is None else z

        return x, y, z


def show(obj):
    "show(obj): shows the G-code data of the given Path object in a dialog"

    if getattr(obj, "Path", None):
        dia = GCodeEditorDialog(obj)
        dia.editor.setPlainText(obj.Path.toGCode())
        dia.exec_()


class CommandPathInspect:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Inspect",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Inspect", "Inspect Toolpath"),
            "Accel": "P, I",
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_Inspect", "Inspects the contents of a toolpath object"
            ),
        }

    def IsActive(self):
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) == 0:
            return False
        obj = selection[0]
        return hasattr(obj, "Path") and len(obj.Path.Commands) > 0

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(
                translate("CAM_Inspect", "Select exactly one path object") + "\n"
            )
            return
        if not (selection[0].isDerivedFrom("Path::Feature")):
            FreeCAD.Console.PrintError(
                translate("CAM_Inspect", "Select exactly one path object") + "\n"
            )
            return

        # if everything is ok, execute
        FreeCADGui.addModule("Path.Main.Gui.Inspect")
        FreeCADGui.doCommand(f"obj = FreeCAD.ActiveDocument.getObject('{selection[0].Name}')")
        FreeCADGui.doCommand("Path.Main.Gui.Inspect.show(obj)")


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_Inspect", CommandPathInspect())
