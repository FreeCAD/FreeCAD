# ***************************************************************************
# *   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2025 FreeCAD Project Association                        *
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
import WorkingPlane
from draftguitools import gui_tool_utils
from draftutils import params
from draftutils.translate import translate
from DraftVecUtils import toString

# So the resource file doesn't trigger errors from code checkers (flake8)
True if Draft_rc.__name__ else False


class ShapeStringTaskPanel:
    """Base class for Draft_ShapeString task panel."""

    def __init__(self, point=None, size=None, string="", font=""):

        self.form = Gui.PySideUic.loadUi(":/ui/TaskShapeString.ui")
        self.form.setWindowTitle(translate("draft", "ShapeString"))
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Draft_ShapeString.svg"))

        self.global_mode = params.get_param("GlobalMode")
        self.form.cbGlobalMode.setChecked(self.global_mode)
        self.change_coord_labels()

        self.wp = WorkingPlane.get_working_plane()
        if point is not None:
            self.point = point
        elif self.global_mode:
            self.point = App.Vector()
        else:
            self.point = self.wp.position
        self.display_point(self.point)
        self.pointPicked = False

        self.height = size if size is not None else params.get_param("ShapeStringHeight")
        self.form.sbHeight.setProperty("rawValue", self.height)
        self.text = string if string else params.get_param("ShapeStringText")
        self.form.leText.setText(self.text)
        self.platform_win_dialog("Overwrite")
        self.font_file = font if font else params.get_param("ShapeStringFontFile")
        self.form.fcFontFile.setFileName(self.font_file)

        # Prevent cyclic processing of point values:
        self.display_point_active = False
        # Default for the "DontUseNativeFontDialog" preference:
        self.font_dialog_pref = False
        # Dummy attribute used by gui_tool_utils.getPoint in action method
        self.node = None

        self.form.sbX.valueChanged.connect(self.set_point_x)
        self.form.sbY.valueChanged.connect(self.set_point_y)
        self.form.sbZ.valueChanged.connect(self.set_point_z)
        if hasattr(self.form.cbGlobalMode, "checkStateChanged"): # Qt version >= 6.7.0
            self.form.cbGlobalMode.checkStateChanged.connect(self.set_global_mode)
        else: # Qt version < 6.7.0
            self.form.cbGlobalMode.stateChanged.connect(self.set_global_mode)
        self.form.pbReset.clicked.connect(self.reset_point)
        self.form.sbHeight.valueChanged.connect(self.set_height)
        self.form.leText.textEdited.connect(self.set_text)
        self.form.fcFontFile.fileNameSelected.connect(self.set_file)

    def action(self, arg):
        """scene event handler"""
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.reject()
        elif arg["Type"] == "SoLocation2Event":  # mouse movement detection
            if not self.pointPicked:
                self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg, noTracker=True)
                self.display_point(self.point)
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                self.display_point(self.point)
                self.pointPicked = True

    def change_coord_labels(self):
        if self.global_mode:
            self.form.labelX.setText(translate("draft", "Global {}").format("X"))
            self.form.labelY.setText(translate("draft", "Global {}").format("Y"))
            self.form.labelZ.setText(translate("draft", "Global {}").format("Z"))
        else:
            self.form.labelX.setText(translate("draft", "Local {}").format("X"))
            self.form.labelY.setText(translate("draft", "Local {}").format("Y"))
            self.form.labelZ.setText(translate("draft", "Local {}").format("Z"))

    def display_point(self, point):
        """Display the selected point."""
        self.display_point_active = True
        if not self.global_mode:
            point = self.wp.get_local_coords(point)
        self.form.sbX.setProperty("rawValue", point.x)
        self.form.sbY.setProperty("rawValue", point.y)
        self.form.sbZ.setProperty("rawValue", point.z)
        self.display_point_active = False

    def escape_string(self, string):
        return string.replace("\\", "\\\\").replace("\"", "\\\"")

    def platform_win_dialog(self, flag):
        """Handle the type of dialog depending on the platform."""
        ParamGroup = App.ParamGet("User parameter:BaseApp/Preferences/Dialog")
        if flag == "Overwrite":
            if "DontUseNativeFontDialog" not in ParamGroup.GetBools():
                # initialize nonexisting one
                ParamGroup.SetBool("DontUseNativeFontDialog", True)
            param = ParamGroup.GetBool("DontUseNativeFontDialog")
            self.font_dialog_pref = ParamGroup.GetBool("DontUseNativeDialog")
            ParamGroup.SetBool("DontUseNativeDialog", param)
        elif flag == "Restore":
            ParamGroup.SetBool("DontUseNativeDialog", self.font_dialog_pref)

    def reset_point(self):
        """Reset the selected point and display new point in the task panel."""
        if self.global_mode:
            self.point = App.Vector()
        else:
            self.point = self.wp.position
        self.pointPicked = False
        self.display_point(self.point)

    def set_file(self, val):
        """Assign the selected font file."""
        self.font_file = val
        params.set_param("ShapeStringFontFile", self.font_file)

    def set_global_mode(self, val):
        self.global_mode = bool(getattr(val, "value", val))
        params.set_param("GlobalMode", self.global_mode)
        self.change_coord_labels()
        self.display_point(self.point)

    def set_height(self, val):
        self.height = App.Units.Quantity(val).Value
        params.set_param("ShapeStringHeight", self.height)

    def set_point_coord(self, coord, val):
        """Change self.point based on  X, Y or Z value entered in the task panel.

        coord should be "x", "y" or "z".
        """
        if self.display_point_active:
            return
        if self.global_mode:
            point = self.point
        else:
            point = self.wp.get_local_coords(self.point)
        setattr(point, coord, App.Units.Quantity(val).Value)
        if self.global_mode:
            self.point = point
        else:
            self.point = self.wp.get_global_coords(point)

    def set_point_x(self, val):
        self.set_point_coord("x", val)

    def set_point_y(self, val):
        self.set_point_coord("y", val)

    def set_point_z(self, val):
        self.set_point_coord("z", val)

    def set_text(self, val):
        self.text = val
        params.set_param("ShapeStringText", self.text)


class ShapeStringTaskPanelCmd(ShapeStringTaskPanel):
    """Task panel for Draft_ShapeString."""

    def __init__(self, sourceCmd):
        super().__init__()
        self.sourceCmd = sourceCmd

    def accept(self):
        """Execute when clicking the OK button."""
        self.create_object()
        self.reject()
        return True

    def create_object(self):
        """Create object in the current document."""
        Gui.addModule("Draft")
        Gui.addModule("WorkingPlane")
        cmd = "Draft.make_shapestring("
        cmd += "String=\"" + self.escape_string(self.text) + "\", "
        cmd += "FontFile=\"" + self.escape_string(self.font_file) + "\", "
        cmd += "Size=" + str(self.height) + ", "
        cmd += "Tracking=0.0"
        cmd += ")"
        self.sourceCmd.commit(
            translate("draft", "Create ShapeString"),
            ["ss = " + cmd,
             "pl = FreeCAD.Placement()",
             "pl.Base = " + toString(self.point),
             "pl.Rotation = WorkingPlane.get_working_plane().get_placement().Rotation",
             "ss.Placement = pl",
             "Draft.autogroup(ss)",
             "FreeCAD.ActiveDocument.recompute()"]
        )

    def reject(self):
        """Run when clicking the Cancel button."""
        self.sourceCmd.finish()
        self.platform_win_dialog("Restore")
        return True


class ShapeStringTaskPanelEdit(ShapeStringTaskPanel):
    """Task panel for Draft ShapeString object in edit mode."""

    def __init__(self, vobj):

        self.obj = vobj.Object
        super().__init__(
            self.obj.Placement.Base, self.obj.Size.Value, self.obj.String, self.obj.FontFile
        )
        self.pointPicked = True
        self.call = Gui.activeView().addEventCallback("SoEvent", self.action)

    def accept(self):

        Gui.doCommand("ss = FreeCAD.ActiveDocument.getObject(\"" + self.obj.Name + "\")")
        Gui.doCommand("ss.String=\"" + self.escape_string(self.text) + "\"")
        Gui.doCommand("ss.FontFile=\"" + self.escape_string(self.font_file) + "\"")
        Gui.doCommand("ss.Size=" + str(self.height))
        Gui.doCommand("ss.Placement.Base=" + toString(self.point))
        Gui.doCommand("FreeCAD.ActiveDocument.recompute()")

        self.reject()
        return True

    def finish(self):

        Gui.activeView().removeEventCallback("SoEvent", self.call)
        Gui.Snapper.off()
        Gui.Control.closeDialog()
        return None

    def reject(self):

        self.obj.ViewObject.Document.resetEdit()
        self.platform_win_dialog("Restore")
        return True


## @}
