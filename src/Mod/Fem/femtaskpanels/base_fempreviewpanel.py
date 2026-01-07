# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD FEM mesh gmsh refinement preview task panel"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package task_mesh_preview
#  \ingroup FEM
#  \brief task panel for mesh gmsh refinement mesh size previews

import FreeCAD, FreeCADGui

from PySide import QtCore, QtGui

from femmesh import gmshtools
from femtools import femutils

from . import base_femlogtaskpanel


class _LogTask(base_femlogtaskpanel._BaseLogTaskPanel):

    def __init__(self, obj, tool):
        super().__init__(obj, tool)

    def get_object_params(self):
        pass

    def set_object_params(self):
        pass


class SettingsDialog(QtGui.QDialog):

    def __init__(self):
        super().__init__()

        self.setWindowTitle("Preview preferences")

        QBtn = QtGui.QDialogButtonBox.Ok

        self.buttonBox = QtGui.QDialogButtonBox(QBtn)
        self.buttonBox.accepted.connect(self.accept)

        self.widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshPreviewSettings.ui"
        )

        auto = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Gmsh").GetBool(
            "previewAutoEnable", False
        )
        self.widget.AutoOpen.setChecked(auto)

        factor = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Gmsh").GetInt(
            "previewMeshFactor", 5
        )
        self.widget.PreviewFactor.setValue(factor)

        layout = QtGui.QVBoxLayout()
        layout.addWidget(self.widget)
        layout.addWidget(self.buttonBox)
        self.setLayout(layout)

    def accept(self):

        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Gmsh").SetBool(
            "previewAutoEnable", self.widget.AutoOpen.isChecked()
        )

        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Gmsh").SetInt(
            "previewMeshFactor", self.widget.PreviewFactor.value()
        )

        self.close()


def _get_parent_gmsh_obj(obj):

    for parent in obj.InList:
        if femutils.is_derived_from(parent, "Fem::FemMeshGmsh"):
            return parent
            break
        else:
            gmsh = _get_parent_gmsh_obj(parent)
            if gmsh:
                return gmsh
    return None


class _TaskPanel:
    """
    The TaskPanel allowing to preview gmsh refinement mesh sizes
    Does not provide a UI on its own, but has functions to easily create
    and show the preview
    """

    def __init__(self, obj):

        # find the parent gmsh object
        self.gmsh_obj = _get_parent_gmsh_obj(obj)

        # we do not derive from this task panel to enable deriving from
        # annother base task panel for functionality, and adding this only
        # for visualization button purposes
        self.logtask = _LogTask(obj, gmshtools.GmshPreviewTools(self.gmsh_obj, obj))
        self.logtask.setup_connections()
        self.logtask.tool.preview_finished.connect(self._calculation_finished)

        self._preview_running = False

        # setup ui
        self._preview_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshPreview.ui"
        )
        self._preview_widget.Preferences.setIcon(
            FreeCADGui.getIcon(":icons/preferences-general.svg")
        )
        self._preview_widget.Process.hide()

        auto = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Gmsh").GetBool(
            "previewAutoEnable", False
        )
        if auto:
            self.start_preview()
            self._preview_widget.Visualize.setChecked(True)

        self._preview_widget.Visualize.toggled.connect(self._visualize)
        self._preview_widget.Preferences.clicked.connect(self._settings)

    def preview_is_calculating(self):
        return self.logtask._thread.isRunning() or (
            self.logtask.tool.process.state() != QtCore.QProcess.ProcessState.NotRunning
        )

    def preview_is_running(self):
        return self._preview_running

    def preview_widget(self):
        # returns the preview widget
        return self._preview_widget

    def start_preview(self):

        if self._preview_running:
            return

        # store the old mesh, to reinstate when the prview is finished
        # Note: ideally we would use the existing mesh and project the
        #       preview onto it, however, it seems not to work with gmsh
        #       scripting, only via GUI. no workaround is known at this moment.
        self.default_mesh = self.gmsh_obj.FemMesh.copy()

        # create the initial view
        self._preview_widget.Process.show()
        self._preview_widget.Process.setCurrentIndex(0)
        self.logtask.run_process()
        self._mesh_display_mode = self.gmsh_obj.ViewObject.DisplayMode
        self._preview_running = True

    def update_preview(self):

        if not self._preview_running:
            return

        if self.preview_is_calculating():
            self._abort_process()

        self._preview_widget.Process.setCurrentIndex(0)
        self.logtask.run_process()

    def _abort_process(self):
        if self.logtask._thread.isRunning():
            self.logtask._thread.quit()

        self.logtask.timer.stop()
        QtGui.QApplication.restoreOverrideCursor()
        if self.logtask.tool.process.state() != QtCore.QProcess.ProcessState.NotRunning:
            self.logtask.tool.process.kill()

    def stop_preview(self):

        if not self._preview_running:
            return

        if self.preview_is_calculating():
            self._abort_process()

        self.gmsh_obj.ViewObject.resetNodeColor()  # does not really work if color mode is not Node
        self.gmsh_obj.ViewObject.ColorMode = self.gmsh_obj.ViewObject.ColorMode  # workaround
        self.gmsh_obj.FemMesh = self.default_mesh

        self.gmsh_obj.ViewObject.Visibility = False
        self.gmsh_obj.ViewObject.DisplayMode = self._mesh_display_mode
        if self.gmsh_obj.Shape:
            self.gmsh_obj.Shape.ViewObject.Visibility = True

        self._preview_widget.Process.hide()

        self._preview_running = False

    @QtCore.Slot()
    def _calculation_finished(self):

        self._preview_widget.Process.setCurrentIndex(1)

        if not self.gmsh_obj.ViewObject.Visibility:
            self.gmsh_obj.ViewObject.Visibility = True
            self.gmsh_obj.ViewObject.DisplayMode = "Faces"
            if self.gmsh_obj.Shape:
                self.gmsh_obj.Shape.ViewObject.Visibility = False

        limits = self.logtask.tool.size_limits
        self._preview_widget.Min.setText("%.2e" % limits[0])
        self._preview_widget.Max.setText("%.2e" % limits[1])

    @QtCore.Slot(bool)
    def _visualize(self, enabled):
        if enabled:
            self.start_preview()
        else:
            self.stop_preview()

    @QtCore.Slot()
    def _settings(self):
        dlg = SettingsDialog()
        dlg.exec()
