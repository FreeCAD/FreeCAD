# ***************************************************************************
# *                                                                         *
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
"""Provide the grid observer for the Draft and BIM workbenches.
"""

import FreeCAD


# View observer code to update the Draft_ToggleGrid command button to reflect
# the grid's visibility status.
# Based on view observer code to update the Draft Tray.
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtWidgets
    from draftutils import gui_utils
    from draftutils.todo import ToDo

    def _update_grid_gui():
        """Update function executed by the callback to refresh the Toggle Grid
        button state on all toolbars and menus.
        """

        try:
            # Get the active view.
            view = gui_utils.get_3d_view()

            # If the active view is not a 3D view, uncheck the button.
            if view is None:
                _set_grid_button_state(False)
                return

            # Otherwise, if there is a view, update the button's checked state.

            # Retrieve the associated grid for each MDI document.
            # [1] is the index where the grid is stored in the
            # trackers list. See setTrackers().
            view_idx = FreeCADGui.Snapper.trackers[0].index(view)
            grid = FreeCADGui.Snapper.trackers[1][view_idx]
            if grid.Visible:
                _set_grid_button_state(True)
            else:
                _set_grid_button_state(False)

        except Exception:
            pass

    def _view_observer_callback():
        """Callback function to update the Toggle Grid button.

        The update will only happen if either the Draft or BIM
        workbenches are active.
        """

        ToDo.delay(_update_grid_gui, None)

    _view_observer_active = False

    def _view_observer_setup():
        """Start or stop the grid observer.

        Start: it is intended to happen when either the Draft or BIM workbench
        are activated. The _view_observer_callback is connected to Qt's
        subWindowActivated signal. The MDI area emits the subWindowActivated()
        signal when the active window changes.

        Stop: This happens when either the Draft or BIM workbenches are
        deactivated and is the reverse of the start operation.
        """

        mw = FreeCADGui.getMainWindow()
        mdi = mw.findChild(QtWidgets.QMdiArea)
        global _view_observer_active
        if not _view_observer_active:
            # Connect callback to subWindowActivated signal
            mdi.subWindowActivated.connect(_view_observer_callback)
            _view_observer_active = True
            # Trigger initial grid button update
            _view_observer_callback()
        else:
            mdi.subWindowActivated.disconnect(_view_observer_callback)
            _view_observer_active = False

    def _set_grid_button_state(button_check):
        """Sets the checked state of the Draft_ToggleGrid command.
        This is then reflected on the associated "Toggle Grid" buttons of every
        toolbar and menu.

        Args:
            button_check (bool): if True, check the grid button.

        Returns:
            None
        """

        action = FreeCADGui.Command.get("Draft_ToggleGrid").getAction()[0]
        action.setCheckable(True)
        action.setChecked(button_check)
