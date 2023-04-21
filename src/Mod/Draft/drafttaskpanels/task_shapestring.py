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
"""Provides the task panel code for the Draft ShapeString tool."""
## @package task_shapestring
# \ingroup drafttaskpanels
# \brief Provides the task panel code for the Draft ShapeString tool.

## \addtogroup drafttaskpanels
# @{
import PySide.QtCore as QtCore
import PySide.QtGui as QtGui

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc

import draftguitools.gui_tool_utils as gui_tool_utils

from DraftVecUtils import toString
from draftutils.utils import get_param
from draftutils.translate import translate
from draftutils.messages import _msg, _err

# So the resource file doesn't trigger errors from code checkers (flake8)
True if Draft_rc.__name__ else False


class ShapeStringTaskPanel:
    """Base class for Draft_ShapeString task panel."""

    def __init__(self, point=App.Vector(0,0,0), size=10, string="", font=""):

        self.form = Gui.PySideUic.loadUi(":/ui/TaskShapeString.ui")
        self.form.setObjectName("ShapeStringTaskPanel")
        self.form.setWindowTitle(translate("draft", "ShapeString"))
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Draft_ShapeString.svg"))

        unit_length = App.Units.Quantity(0.0, App.Units.Length).getUserPreferred()[2]
        self.form.sbX.setProperty('rawValue', point.x)
        self.form.sbX.setProperty('unit', unit_length)
        self.form.sbY.setProperty('rawValue', point.y)
        self.form.sbY.setProperty('unit', unit_length)
        self.form.sbZ.setProperty('rawValue', point.z)
        self.form.sbZ.setProperty('unit', unit_length)
        self.form.sbHeight.setProperty('rawValue', size)
        self.form.sbHeight.setProperty('unit', unit_length)

        self.stringText = string if string else translate("draft", "Default")
        self.form.leString.setText(self.stringText)
        self.platWinDialog("Overwrite")
        self.fileSpec = font if font else get_param("FontFile", "")
        self.form.fcFontFile.setFileName(self.fileSpec)
        self.point = point
        self.pointPicked = False
        # Default for the "DontUseNativeFontDialog" preference:
        self.font_dialog_pref = False
        # Dummy attribute used by gui_tools_utils.getPoint in action method
        self.node = None

        QtCore.QObject.connect(self.form.fcFontFile, QtCore.SIGNAL("fileNameSelected(const QString&)"), self.fileSelect)
        QtCore.QObject.connect(self.form.pbReset, QtCore.SIGNAL("clicked()"), self.resetPoint)

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
            self.point,ctrlPoint,info = gui_tool_utils.getPoint(self, arg, noTracker=True)
            if not self.pointPicked:
                self.setPoint(self.point)
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                self.setPoint(self.point)
                self.pointPicked = True

    def setPoint(self, point):
        """Assign the selected point."""
        self.form.sbX.setProperty('rawValue', point.x)
        self.form.sbY.setProperty('rawValue', point.y)
        self.form.sbZ.setProperty('rawValue', point.z)


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
            self.font_dialog_pref = ParamGroup.GetBool("DontUseNativeDialog")
            ParamGroup.SetBool("DontUseNativeDialog", param)

        elif flag == "Restore":
            ParamGroup.SetBool("DontUseNativeDialog", self.font_dialog_pref)


class ShapeStringTaskPanelCmd(ShapeStringTaskPanel):
    """Task panel for Draft_ShapeString."""

    def __init__(self, sourceCmd):
        super().__init__()
        self.sourceCmd = sourceCmd

    def accept(self):
        """Execute when clicking the OK button."""
        self.createObject()
        self.reject()

        return True

    def reject(self):
        """Run when clicking the Cancel button."""
        Gui.ActiveDocument.resetEdit()
        self.sourceCmd.finish()
        self.platWinDialog("Restore")
        return True

    def createObject(self):
        """Create object in the current document."""
        dquote = '"'
        String = self.form.leString.text()
        String = dquote + String.replace(dquote, '\\"') + dquote
        FFile = dquote + str(self.fileSpec) + dquote

        Size = str(App.Units.Quantity(self.form.sbHeight.text()).Value)
        Tracking = str(0.0)
        x = App.Units.Quantity(self.form.sbX.text()).Value
        y = App.Units.Quantity(self.form.sbY.text()).Value
        z = App.Units.Quantity(self.form.sbZ.text()).Value
        ssBase = App.Vector(x, y, z)
        try:
            qr, sup, points, fil = self.sourceCmd.getStrings()
            Gui.addModule("Draft")
            self.sourceCmd.commit(translate("draft", "Create ShapeString"),
                                  ['ss=Draft.make_shapestring(String=' + String + ', FontFile=' + FFile + ', Size=' + Size + ', Tracking=' + Tracking + ')',
                                   'plm=FreeCAD.Placement()',
                                   'plm.Base=' + toString(ssBase),
                                   'plm.Rotation.Q=' + qr,
                                   'ss.Placement=plm',
                                   'ss.Support=' + sup,
                                   'Draft.autogroup(ss)',
                                   'FreeCAD.ActiveDocument.recompute()'])
        except Exception:
            _err("Draft_ShapeString: error delaying commit\n")


class ShapeStringTaskPanelEdit(ShapeStringTaskPanel):
    """Task panel for Draft ShapeString object in edit mode."""
    def __init__(self, vobj):

        base = vobj.Object.Placement.Base
        size = vobj.Object.Size.Value
        string = vobj.Object.String
        font = vobj.Object.FontFile

        super().__init__(base, size, string, font)
        self.pointPicked = True
        self.vobj = vobj
        self.call = Gui.activeView().addEventCallback("SoEvent", self.action)

    def accept(self):

        x = App.Units.Quantity(self.form.sbX.text()).Value
        y = App.Units.Quantity(self.form.sbY.text()).Value
        z = App.Units.Quantity(self.form.sbZ.text()).Value

        base = App.Vector(x, y, z)
        size = App.Units.Quantity(self.form.sbHeight.text()).Value
        string = self.form.leString.text()
        font_file = self.fileSpec

        o = "FreeCAD.ActiveDocument.getObject(\"" + self.vobj.Object.Name + "\")"
        Gui.doCommand(o+".Placement.Base=" + toString(base))
        Gui.doCommand(o+".Size=" + str(size))
        Gui.doCommand(o+".String=\"" + string + "\"")
        Gui.doCommand(o+".FontFile=\"" + font_file + "\"")
        Gui.doCommand("FreeCAD.ActiveDocument.recompute()")

        self.reject()

        return True

    def reject(self):

        self.vobj.Document.resetEdit()
        self.platWinDialog("Restore")

        return True

    def finish(self):

        Gui.activeView().removeEventCallback("SoEvent", self.call)
        Gui.Snapper.off()
        Gui.Control.closeDialog()

        return None

## @}
