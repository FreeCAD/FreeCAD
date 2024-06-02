# ***************************************************************************
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2023 FreeCAD Project Association                        *
# *   Copyright (c) 2024 Syres                                              *
# *   Copyright (c) 2024 Furgo                                              *
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

import FreeCAD
from draftutils import gui_utils
from draftutils import utils


# View observer code to update the Draft_ToggleGrid command button to reflect
# the grid's visibility status
# Based on view observer code to update the Draft Tray
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtWidgets
    from draftutils.todo import ToDo

    def _update_grid_gui():
        """Callback function to update the Toggle Grid button on all
        toolbars and menus
        """
        try:
            # Get the active view
            view = gui_utils.get_3d_view()

            # If there isn't a view (no document loaded?), disable the button
            if view is None:
                _set_grid_button_state(False, False)
                return
            else:
                # Otherwise, if there is a view, update the button's status
                # Update only if FreeCAD has started with GUI, the Draft
                # workbench has loaded, and there is a view
                if (
                    FreeCAD.GuiUp
                    and hasattr(FreeCADGui, "draftToolBar")
                    and gui_utils.get_3d_view() is not None
                ):
                    if FreeCADGui.Snapper.grid.Visible:
                        _set_grid_button_state(True, True)
                    else:
                        _set_grid_button_state(True, False)
        except Exception:
            pass

    def _view_observer_callback(sub_win):
        # FIXME: the original Draft Tray observer had this commented out code
        # Check if it's necessary
        #if sub_win is None:
        #    return
        #view = gui_utils.get_3d_view()
        #if view is None:
        #    return
        if not hasattr(FreeCADGui, "draftToolBar"):
            return

        tray = FreeCADGui.draftToolBar.tray
        if tray is None:
            return

        if not tray.isVisible():
            return

        ToDo.delay(_update_grid_gui, None)

    _view_observer_active = False

    def _view_observer_start():
        """Start the grid observer. This is intended to happen when the draft
        workbench is activated. This function connects the
        _view_observer_callback to Qt's subWindowActivated signal. The MDI
        area emits the subWindowActivated() signal when the active window
        changes.
        """
        mw = FreeCADGui.getMainWindow()
        mdi = mw.findChild(QtWidgets.QMdiArea)
        global _view_observer_active
        if not _view_observer_active:
            # Connect callback to subWindowActivated signal
            mdi.subWindowActivated.connect(_view_observer_callback)
            _view_observer_active = True
            # Trigger initial grid button update
            _view_observer_callback(mdi.activeSubWindow())

    def _view_observer_stop():
        mw = FreeCADGui.getMainWindow()
        mdi = mw.findChild(QtWidgets.QMdiArea)
        global _view_observer_active
        if _view_observer_active:
            mdi.subWindowActivated.disconnect(_view_observer_callback)
            _view_observer_active = False

    def _set_grid_button_state(button_enable, button_check):
        """Sets the enabled and check states of the Draft_ToggleGrid command.
        This is then reflected on the associated "Toggle Grid" buttons of every
        toolbar and menu.

        Args:
            button_enable (bool): if True, enable the grid button
            button_check (bool): if True, check the grid button

        Returns:
            None
        """

        action = FreeCADGui.Command.get("Draft_ToggleGrid").getAction()[0]
        action.setCheckable(button_enable)
        action.setChecked(button_check)
