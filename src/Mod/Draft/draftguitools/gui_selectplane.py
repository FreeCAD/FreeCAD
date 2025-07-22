# ***************************************************************************
# *   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2023 FreeCAD Project Association                        *
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
"""Provides GUI tools to set up the working plane and its grid."""
## @package gui_selectplane
# \ingroup draftguitools
# \brief Provides GUI tools to set up the working plane and its grid.

## \addtogroup draftguitools
# @{
from PySide import QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Part
import WorkingPlane

from FreeCAD import Units
from drafttaskpanels import task_selectplane
from draftutils import gui_utils
from draftutils import params
from draftutils import utils
from draftutils.messages import _msg
from draftutils.todo import todo
from draftutils.translate import translate

__title__ = "FreeCAD Draft Workbench GUI Tools - Working plane-related tools"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin")
__url__ = "https://www.freecad.org"


class Draft_SelectPlane:
    """The Draft_SelectPlane FreeCAD command definition."""

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {"Pixmap": "Draft_SelectPlane",
                "Accel": "W, P",
                "MenuText": QT_TRANSLATE_NOOP("Draft_SelectPlane", "Select working plane"),
                "ToolTip": QT_TRANSLATE_NOOP("Draft_SelectPlane", "Select 3 vertices, one or more shapes or an object to define a working plane.")}

    def IsActive(self):
        """Return True when this command should be available."""
        return bool(gui_utils.get_3d_view())

    def Activated(self):
        """Execute when the command is called."""
        # Finish active Draft command if any
        if App.activeDraftCommand is not None:
            App.activeDraftCommand.finish()

        App.activeDraftCommand = self
        self.call = None

        # Set variables
        self.wp = WorkingPlane.get_working_plane()
        self.view = self.wp._view
        self.grid = None
        if hasattr(Gui, "Snapper"):
            Gui.Snapper.setTrackers()
            self.grid = Gui.Snapper.grid
        self.offset = 0
        self.center = params.get_param("CenterPlaneOnView")

        # Create task panel
        self.taskd = task_selectplane.SelectPlaneTaskPanel()
        self.taskd.reject = self.reject
        form = self.taskd.form

        # Fill values
        form.fieldOffset.setText(Units.Quantity(self.offset, Units.Length).UserString)
        form.checkCenter.setChecked(self.center)
        try:
            q = Units.Quantity(params.get_param("gridSpacing"))
        except ValueError:
            q = Units.Quantity("1 mm")
        form.fieldGridSpacing.setText(q.UserString)
        form.fieldGridMainLine.setValue(params.get_param("gridEvery"))
        form.fieldGridExtension.setValue(params.get_param("gridSize"))
        form.fieldSnapRadius.setValue(params.get_param("snapRange"))

        # Set icons
        form.setWindowIcon(QtGui.QIcon(":/icons/Draft_SelectPlane.svg"))
        form.buttonTop.setIcon(QtGui.QIcon(":/icons/view-top.svg"))
        form.buttonFront.setIcon(QtGui.QIcon(":/icons/view-front.svg"))
        form.buttonSide.setIcon(QtGui.QIcon(":/icons/view-right.svg"))
        form.buttonAlign.setIcon(QtGui.QIcon(":/icons/view-isometric.svg"))
        form.buttonAuto.setIcon(QtGui.QIcon(":/icons/view-axonometric.svg"))
        form.buttonMove.setIcon(QtGui.QIcon(":/icons/Draft_Move.svg"))
        form.buttonCenter.setIcon(QtGui.QIcon(":/icons/view-fullscreen.svg"))
        form.buttonPrevious.setIcon(QtGui.QIcon(":/icons/sel-back.svg"))
        form.buttonNext.setIcon(QtGui.QIcon(":/icons/sel-forward.svg"))

        # Grid color
        color = params.get_param("gridColor")
        form.buttonColor.setProperty("color", QtGui.QColor(utils.rgba_to_argb(color)))

        # Connect slots
        form.buttonTop.clicked.connect(self.on_click_top)
        form.buttonFront.clicked.connect(self.on_click_front)
        form.buttonSide.clicked.connect(self.on_click_side)
        form.buttonAlign.clicked.connect(self.on_click_align)
        form.buttonAuto.clicked.connect(self.on_click_auto)
        form.buttonMove.clicked.connect(self.on_click_move)
        form.buttonCenter.clicked.connect(self.on_click_center)
        form.buttonPrevious.clicked.connect(self.on_click_previous)
        form.buttonNext.clicked.connect(self.on_click_next)
        form.fieldOffset.textEdited.connect(self.on_set_offset)
        if hasattr(form.checkCenter, "checkStateChanged"): # Qt version >= 6.7.0
            form.checkCenter.checkStateChanged.connect(self.on_set_center)
        else: # Qt version < 6.7.0
            form.checkCenter.stateChanged.connect(self.on_set_center)
        form.fieldGridSpacing.textEdited.connect(self.on_set_grid_size)
        form.fieldGridMainLine.valueChanged.connect(self.on_set_main_line)
        form.fieldGridExtension.valueChanged.connect(self.on_set_extension)
        form.fieldSnapRadius.valueChanged.connect(self.on_set_snap_radius)
        form.buttonColor.changed.connect(self.on_color_changed)

        # Enable/disable buttons.
        form.buttonPrevious.setEnabled(self.wp._has_previous())
        form.buttonNext.setEnabled(self.wp._has_next())

        # Try to find a WP from the current selection
        if Gui.Selection.hasSelection():
            if self.wp.align_to_selection(self.offset):
                Gui.Selection.clearSelection()
            self.finish()
            return

        # Execute the actual task panel delayed to catch possible active Draft command
        todo.delay(Gui.Control.showDialog, self.taskd)
        todo.delay(form.setFocus, None)
        _msg(translate(
                "draft",
                "Select 3 vertices, one or more shapes or an object to define a working plane"))
        self.call = self.view.addEventCallback("SoEvent", self.action)

    def finish(self):
        """Execute when the command is terminated."""
        App.activeDraftCommand = None
        Gui.Control.closeDialog()
        if hasattr(Gui, "Snapper"):
            Gui.Snapper.off()
        # Terminate coin callbacks
        if self.call:
            try:
                self.view.removeEventCallback("SoEvent", self.call)
            except RuntimeError:
                # The view has been deleted already
                pass
            self.call = None

    def reject(self):
        """Execute when clicking the Cancel button."""
        self.finish()

    def action(self, arg):
        """Set the callbacks for the view."""
        if arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
            self.reject()
        if arg["Type"] == "SoMouseButtonEvent" \
                and (arg["State"] == "DOWN") \
                and (arg["Button"] == "BUTTON1"):
            self.check_selection()

    def check_selection(self):
        """Check the selection, if it is usable, finish the command."""
        if self.wp.align_to_selection(self.offset):
            Gui.Selection.clearSelection()
            self.finish()

    def on_click_top(self):
        self.wp.set_to_top(self.offset, self.center)
        self.finish()

    def on_click_front(self):
        self.wp.set_to_front(self.offset, self.center)
        self.finish()

    def on_click_side(self):
        self.wp.set_to_side(self.offset, self.center)
        self.finish()

    def on_click_align(self):
        self.wp.set_to_view(self.offset, self.center)
        self.finish()

    def on_click_auto(self):
        self.wp.set_to_auto()
        self.finish()

    def on_click_move(self):
        sels = Gui.Selection.getSelectionEx("", 0)
        if len(sels) == 1 \
                and len(sels[0].SubObjects) == 1 \
                and sels[0].SubObjects[0].ShapeType == "Vertex":
            vert = Part.getShape(sels[0].Object,
                                 sels[0].SubElementNames[0],
                                 needSubElement=True,
                                 retType=0)
            self.wp.set_to_position(vert.Point)
            Gui.Selection.clearSelection()
            self.finish()
        else:
            # Move the WP to the center of the current view
            self.wp.center_on_view()
            self.finish()

    def on_click_center(self):
        self.wp.align_view()
        self.finish()

    def on_click_previous(self):
        self.wp._previous()
        self.finish()

    def on_click_next(self):
        self.wp._next()
        self.finish()

    def on_set_offset(self, text):
        try:
            q = Units.Quantity(text)
        except Exception:
            pass
        else:
            self.offset = q.Value

    def on_set_center(self, val):
        self.center = bool(getattr(val, "value", val))
        params.set_param("CenterPlaneOnView", self.center)

    def on_set_grid_size(self, text):
        try:
            q = Units.Quantity(text)
        except Exception:
            pass
        else:
            params.set_param("gridSpacing", q.UserString)
            # ParamObserver handles grid changes. See params.py.
            if self.grid is not None:
                self.grid.show_during_command = True
                self.grid.on()

    def on_set_main_line(self, i):
        if i > 1:
            params.set_param("gridEvery", i)
            # ParamObserver handles grid changes. See params.py.
            if self.grid is not None:
                self.grid.show_during_command = True
                self.grid.on()

    def on_set_extension(self, i):
        if i > 1:
            params.set_param("gridSize", i)
            # ParamObserver handles grid changes. See params.py.
            if self.grid is not None:
                self.grid.show_during_command = True
                self.grid.on()

    def on_set_snap_radius(self, i):
        params.set_param("snapRange", i)
        if hasattr(Gui, "Snapper"):
            Gui.Snapper.showradius()

    def on_color_changed(self):
        color = utils.argb_to_rgba(self.taskd.form.buttonColor.property("color").rgba())
        params.set_param("gridColor", color)

Gui.addCommand('Draft_SelectPlane', Draft_SelectPlane())

## @}
