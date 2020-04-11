"""Provide the task panel for the Draft ShapeString tool."""
## @package task_shapestring
# \ingroup DRAFT
# \brief Provide the task panel for the Draft ShapeString tool.

# ***************************************************************************
# *   (c) 2009 Yorik van Havre <yorik@uncreated.net>                        *
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
import sys
import FreeCAD as App
import FreeCADGui as Gui
import Draft
import Draft_rc
import DraftVecUtils
import DraftTools
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui
from draftutils.translate import translate
from draftutils.messages import _msg, _err

_Quantity = App.Units.Quantity


# So the resource file doesn't trigger errors from code checkers (flake8)
True if Draft_rc.__name__ else False


class ShapeStringTaskPanel:
    """TaskPanel for Draft_ShapeString."""

    oldValueBuffer = False

    def __init__(self):
        self.form = QtGui.QWidget()
        self.form.setObjectName("ShapeStringTaskPanel")
        self.form.setWindowTitle(translate("draft", "ShapeString"))
        layout = QtGui.QVBoxLayout(self.form)
        uiFile = QtCore.QFile(":/ui/TaskShapeString.ui")
        loader = Gui.UiLoader()
        self.task = loader.load(uiFile)
        layout.addWidget(self.task)

        qStart = _Quantity(0.0, App.Units.Length)
        self.task.sbX.setProperty('rawValue', qStart.Value)
        self.task.sbX.setProperty('unit', qStart.getUserPreferred()[2])
        self.task.sbY.setProperty('rawValue', qStart.Value)
        self.task.sbY.setProperty('unit', qStart.getUserPreferred()[2])
        self.task.sbZ.setProperty('rawValue', qStart.Value)
        self.task.sbZ.setProperty('unit', qStart.getUserPreferred()[2])
        self.task.sbHeight.setProperty('rawValue', 10.0)
        self.task.sbHeight.setProperty('unit', qStart.getUserPreferred()[2])

        self.stringText = translate("draft", "Default")
        self.task.leString.setText(self.stringText)
        self.platWinDialog("Overwrite")
        self.task.fcFontFile.setFileName(Draft.getParam("FontFile", ""))
        self.fileSpec = Draft.getParam("FontFile", "")
        self.point = App.Vector(0.0, 0.0, 0.0)
        self.pointPicked = False

        QtCore.QObject.connect(self.task.fcFontFile, QtCore.SIGNAL("fileNameSelected(const QString&)"), self.fileSelect)
        QtCore.QObject.connect(self.task.pbReset, QtCore.SIGNAL("clicked()"), self.resetPoint)
        self.point = None
        self.view = Draft.get3DView()
        self.call = self.view.addEventCallback("SoEvent", self.action)
        _msg(translate("draft", "Pick ShapeString location point:"))

    def fileSelect(self, fn):
        """Assign the selected file."""
        self.fileSpec = fn

    def resetPoint(self):
        """Reset the selected point."""
        self.pointPicked = False
        origin = App.Vector(0.0, 0.0, 0.0)
        self.setPoint(origin)

    def action(self, arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.reject()
        elif arg["Type"] == "SoLocation2Event":  # mouse movement detection
            self.point,ctrlPoint,info = DraftTools.getPoint(self.sourceCmd, arg, noTracker=True)
            if not self.pointPicked:
                self.setPoint(self.point)
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                self.setPoint(self.point)
                self.pointPicked = True

    def setPoint(self, point):
        """Assign the selected point."""
        self.task.sbX.setProperty('rawValue', point.x)
        self.task.sbY.setProperty('rawValue', point.y)
        self.task.sbZ.setProperty('rawValue', point.z)

    def createObject(self):
        """Create object in the current document."""
        dquote = '"'
        if sys.version_info.major < 3:  # Python3: no more unicode
            String = 'u' + dquote + str(self.task.leString.text().encode('unicode_escape')) + dquote
        else:
            String = dquote + self.task.leString.text() + dquote
        FFile = dquote + str(self.fileSpec) + dquote

        Size = str(_Quantity(self.task.sbHeight.text()).Value)
        Tracking = str(0.0)
        x = _Quantity(self.task.sbX.text()).Value
        y = _Quantity(self.task.sbY.text()).Value
        z = _Quantity(self.task.sbZ.text()).Value
        ssBase = App.Vector(x, y, z)
        # this try block is almost identical to the one in DraftTools
        try:
            qr, sup, points, fil = self.sourceCmd.getStrings()
            Gui.addModule("Draft")
            self.sourceCmd.commit(translate("draft", "Create ShapeString"),
                                  ['ss=Draft.makeShapeString(String='+String+',FontFile='+FFile+',Size='+Size+',Tracking='+Tracking+')',
                                   'plm=FreeCAD.Placement()',
                                   'plm.Base='+DraftVecUtils.toString(ssBase),
                                   'plm.Rotation.Q='+qr,
                                   'ss.Placement=plm',
                                   'ss.Support='+sup,
                                   'Draft.autogroup(ss)'])
        except Exception:
            _err("Draft_ShapeString: error delaying commit\n")

    def platWinDialog(self, flag):
        """Handle the type of dialog depending on the platform."""
        ParamGroup = App.ParamGet("User parameter:BaseApp/Preferences/Dialog")
        if flag == "Overwrite":
            GroupContent = ParamGroup.GetContents()
            found = False
            if GroupContent:
                for ParamSet in GroupContent:
                    if ParamSet[1] == "DontUseNativeFontDialog":
                        found = True
                        break

            if found is False:
                # initialize nonexisting one
                ParamGroup.SetBool("DontUseNativeFontDialog", True)

            param = ParamGroup.GetBool("DontUseNativeFontDialog")
            self.oldValueBuffer = ParamGroup.GetBool("DontUseNativeDialog")
            ParamGroup.SetBool("DontUseNativeDialog", param)

        elif flag == "Restore":
            ParamGroup.SetBool("DontUseNativeDialog", self.oldValueBuffer)

    def accept(self):
        """Execute when clicking the OK button."""
        self.createObject()
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        Gui.ActiveDocument.resetEdit()
        Gui.Snapper.off()
        self.sourceCmd.creator.finish(self.sourceCmd)
        self.platWinDialog("Restore")
        return True

    def reject(self):
        """Run when clicking the Cancel button."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)
        Gui.ActiveDocument.resetEdit()
        Gui.Snapper.off()
        self.sourceCmd.creator.finish(self.sourceCmd)
        self.platWinDialog("Restore")
        return True
