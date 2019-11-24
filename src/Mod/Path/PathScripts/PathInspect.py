# ***************************************************************************
# *   (c) Yorik van Havre (yorik@uncreated.net) 2015                        *
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


from PySide import QtCore, QtGui
import FreeCAD
import FreeCADGui
import Path


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class GCodeHighlighter(QtGui.QSyntaxHighlighter):

    def __init__(self, parent=None):

        def convertcolor(c):
            return QtGui.QColor(int((c >> 24) & 0xFF), int((c >> 16) & 0xFF), int((c >> 8) & 0xFF))

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
            (QtCore.QRegExp("[\\-0-9\\.]"), numberFormat))
        keywordFormat = QtGui.QTextCharFormat()
        keywordFormat.setForeground(colors[1])
        keywordFormat.setFontWeight(QtGui.QFont.Bold)
        keywordPatterns = ["\\bG[0-9]+\\b", "\\bM[0-9]+\\b"]
        self.highlightingRules.extend(
            [(QtCore.QRegExp(pattern), keywordFormat) for pattern in keywordPatterns])
        speedFormat = QtGui.QTextCharFormat()
        speedFormat.setFontWeight(QtGui.QFont.Bold)
        speedFormat.setForeground(colors[2])
        self.highlightingRules.append(
            (QtCore.QRegExp("\\bF[0-9\\.]+\\b"), speedFormat))

    def highlightBlock(self, text):

        for pattern, fmt in self.highlightingRules:
            expression = QtCore.QRegExp(pattern)
            index = expression.indexIn(text)
            while index >= 0:
                length = expression.matchedLength()
                self.setFormat(index, length, fmt)
                index = expression.indexIn(text, index + length)


class GCodeEditorDialog(QtGui.QDialog):

    def __init__(self, PathObj, parent=FreeCADGui.getMainWindow()):
        self.PathObj = PathObj
        QtGui.QDialog.__init__(self, parent)
        layout = QtGui.QVBoxLayout(self)

        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        c = p.GetUnsigned("DefaultHighlightPathColor", 4286382335)
        Q = QtGui.QColor(int((c >> 24) & 0xFF), int((c >> 16) & 0xFF), int((c >> 8) & 0xFF))
        highlightcolor = (Q.red() / 255., Q.green() / 255., Q.blue() / 255., Q.alpha() / 255.)

        self.selectionobj = FreeCAD.ActiveDocument.addObject("Path::Feature", "selection")
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
        self.highlighter = GCodeHighlighter(self.editor.document())
        layout.addWidget(self.editor)

        # Note
        lab = QtGui.QLabel()
        lab.setText(translate("Path_Inspect", "<b>Note</b>: Pressing OK will commit any change you make above to the object, but if the object is parametric, these changes will be overridden on recompute."))
        lab.setWordWrap(True)
        layout.addWidget(lab)

        # OK and Cancel buttons
        self.buttons = QtGui.QDialogButtonBox(
            QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel,
            QtCore.Qt.Horizontal, self)
        layout.addWidget(self.buttons)
        self.buttons.accepted.connect(self.accept)
        self.buttons.rejected.connect(self.reject)
        self.editor.selectionChanged.connect(self.highlightpath)
        self.finished.connect(self.cleanup)

        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        Xpos = int(prefs.GetString('inspecteditorX', "0"))
        Ypos = int(prefs.GetString('inspecteditorY', "0"))
        height = int(prefs.GetString('inspecteditorH', "500"))
        width = int(prefs.GetString('inspecteditorW', "600"))
        self.move(Xpos, Ypos)
        self.resize(width, height)

    def cleanup(self):
        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
        prefs.SetString('inspecteditorX', str(self.x()))
        prefs.SetString('inspecteditorY', str(self.y()))
        prefs.SetString('inspecteditorW', str(self.width()))
        prefs.SetString('inspecteditorH', str(self.height()))
        FreeCAD.ActiveDocument.removeObject(self.selectionobj.Name)

    def highlightpath(self):
        cursor = self.editor.textCursor()
        sp = cursor.selectionStart()
        ep = cursor.selectionEnd()
        cursor.setPosition(sp)
        startrow = cursor.blockNumber()
        cursor.setPosition(ep)
        endrow = cursor.blockNumber()

        commands = self.PathObj.Commands

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

        selectionpath = [firstrapid] + commands[startrow:endrow + 1]
        p.Commands = selectionpath
        self.selectionobj.Path = p


def show(obj):
    "show(obj): shows the G-code data of the given Path object in a dialog"

    if hasattr(obj, "Path"):
        if obj.Path:
            dia = GCodeEditorDialog(obj.Path)
            dia.editor.setText(obj.Path.toGCode())
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
        return {'Pixmap': 'Path-Inspect',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Inspect", "Inspect G-code"),
                'Accel': "P, I",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Inspect", "Inspects the G-code contents of a path")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(
                translate("Path_Inspect", "Please select exactly one path object") + "\n")
            return
        if not(selection[0].isDerivedFrom("Path::Feature")):
            FreeCAD.Console.PrintError(
                translate("Path_Inspect", "Please select exactly one path object") + "\n")
            return

        # if everything is ok, execute
        FreeCADGui.addModule("PathScripts.PathInspect")
        FreeCADGui.doCommand(
            'PathScripts.PathInspect.show(FreeCAD.ActiveDocument.' + selection[0].Name + ')')


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Inspect', CommandPathInspect())
