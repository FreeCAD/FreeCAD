# ***************************************************************************
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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
"""Provide the grid observer and background color change for the Draft Workbench.
"""
import lazy_loader.lazy_loader as lz

import FreeCAD
import FreeCADGui
from draftutils import gui_utils
from draftutils import utils


# View observer code to update the Draft Tray:
if FreeCAD.GuiUp:
    import PySide.QtCore as QtCore
    import PySide.QtGui as QtGui
    from PySide import QtWidgets
    from draftutils.todo import ToDo

    def _update_gui():
        try:
            view = gui_utils.get_3d_view()
            if view is None:
                mw = FreeCADGui.getMainWindow()
                if mw.findChild(QtWidgets.QToolBar, "Draft snap"):
                    tbSnap = mw.findChild(QtWidgets.QToolBar, "Draft snap")
                    btnGrid = _find_grid_toolbutton("Draft snap", "Toggle grid", "transparent")
                    if mw.findChild(QtWidgets.QToolBar, "draft_snap_widget"):
                        tbSnapWidget = mw.findChild(QtWidgets.QToolBar, "draft_snap_widget")
                        btnGridSW = _find_grid_toolbutton("draft_snap_widget", "Toggle grid", "transparent")
                return
            _update_gridgui()
        except Exception:
            pass

    def _view_observer_callback(sub_win):
        if not hasattr(FreeCADGui, "draftToolBar"):
            return
        tray = FreeCADGui.draftToolBar.tray
        if tray is None:
            return
        if FreeCADGui.draftToolBar.tray.isVisible() is False:
            return
        ToDo.delay(_update_gui, None)

    _view_observer_active = False

    def _view_observer_start():
        mw = FreeCADGui.getMainWindow()
        mdi = mw.findChild(QtWidgets.QMdiArea)
        global _view_observer_active
        if not _view_observer_active:
            mdi.subWindowActivated.connect(_view_observer_callback)
            _view_observer_active = True
            _view_observer_callback(mdi.activeSubWindow())  # Trigger initial update.

    def _view_observer_stop():
        mw = FreeCADGui.getMainWindow()
        mdi = mw.findChild(QtWidgets.QMdiArea)
        global _view_observer_active
        if _view_observer_active:
            mdi.subWindowActivated.disconnect(_view_observer_callback)
            _view_observer_active = False


    def _update_gridgui():
        if FreeCAD.GuiUp \
                and hasattr(FreeCADGui, "draftToolBar") \
                and gui_utils.get_3d_view() is not None:
            mw = FreeCADGui.getMainWindow()
            if mw.findChild(QtWidgets.QToolBar, "Draft snap"):
                if FreeCADGui.Snapper.grid.Visible:
                    tbSnap = mw.findChild(QtWidgets.QToolBar, "Draft snap")
                    btnGrid = _find_grid_toolbutton("Draft snap", "Toggle grid", "#469143")
                    if mw.findChild(QtWidgets.QToolBar, "draft_snap_widget"):
                        tbSnapWidget = mw.findChild(QtWidgets.QToolBar, "draft_snap_widget")
                        btnGridSW = _find_grid_toolbutton("draft_snap_widget", "Toggle grid", "#469143")
                else:
                    tbSnap = mw.findChild(QtWidgets.QToolBar, "Draft snap")
                    btnGrid = _find_grid_toolbutton("Draft snap", "Toggle grid", "#914343")
                    if mw.findChild(QtWidgets.QToolBar, "draft_snap_widget"):
                        tbSnapWidget = mw.findChild(QtWidgets.QToolBar, "draft_snap_widget")
                        btnGridSW = _find_grid_toolbutton("draft_snap_widget", "Toggle grid", "#914343")
 

    def _find_grid_toolbutton(tbObj, tbButton, tbButtonColor):
        mw = FreeCADGui.getMainWindow()
        for toolbar in mw.findChildren(QtWidgets.QToolBar):
            if toolbar.objectName() == tbObj:
                for toolbutton in toolbar.findChildren(QtWidgets.QToolButton):
                    if hasattr(toolbutton, "text"):
                        if toolbutton.text() == tbButton:
                            toolbutton.setStyleSheet('background-color: ' + tbButtonColor + ';')


