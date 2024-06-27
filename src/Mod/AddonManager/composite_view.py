# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2024 The FreeCAD Project Association AISBL         *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

""" Provides a class for showing the list view and detail view at the same time. """

import base64

from addonmanager_freecad_interface import Preferences

from Addon import Addon
from Widgets.addonmanager_widget_package_details_view import PackageDetailsView
from Widgets.addonmanager_widget_view_selector import AddonManagerDisplayStyle
from addonmanager_package_details_controller import PackageDetailsController
from package_list import PackageList

# Get whatever version of PySide we can
try:
    import PySide  # Use the FreeCAD wrapper
except ImportError:
    try:
        import PySide6 as PySide  # Outside FreeCAD, try Qt6 first
    except ImportError:
        # Fall back to Qt5 (if this fails, Python will kill this module's import)
        import PySide2 as PySide

from PySide import QtCore, QtWidgets


class CompositeView(QtWidgets.QWidget):
    """A widget that displays the Addon Manager's top bar, the list of Addons, and the detail
    view. Depending on the view mode selected, these may all be displayed at once, or selecting
    an addon in the list may case the list to hide and the detail view to show."""

    install = QtCore.Signal(Addon)
    uninstall = QtCore.Signal(Addon)
    update = QtCore.Signal(Addon)
    execute = QtCore.Signal(Addon)
    update_status = QtCore.Signal(Addon)
    check_for_update = QtCore.Signal(Addon)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.package_details = PackageDetailsView(self)
        self.package_details_controller = PackageDetailsController(self.package_details)
        self.package_list = PackageList(self)
        prefs = Preferences()
        self.display_style = prefs.get("ViewStyle")
        self.main_layout = QtWidgets.QHBoxLayout(self)
        self.splitter = QtWidgets.QSplitter(self)
        self.splitter.addWidget(self.package_list)
        self.splitter.addWidget(self.package_details)
        self.splitter.setOrientation(QtCore.Qt.Horizontal)
        self.splitter.setContentsMargins(0, 0, 0, 0)
        self.splitter.setSizePolicy(
            QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding
        )
        self.main_layout.addWidget(self.splitter)
        self.layout().setContentsMargins(0, 0, 0, 0)
        self._setup_ui()
        self._setup_connections()
        self._restore_splitter_state()
        self.scroll_position = 0

    def _save_splitter_state(self):
        """Write the splitter state into an Addon manager preference, CompositeSplitterState"""
        prefs = Preferences()
        state = self.splitter.saveState()
        encoded = base64.b64encode(state).decode("ASCII")
        prefs.set("CompositeSplitterState", encoded)

    def _restore_splitter_state(self):
        """Restore the splitter state from CompositeSplitterState"""
        prefs = Preferences()
        encoded = prefs.get("CompositeSplitterState")
        if encoded:
            state = base64.b64decode(encoded)
            self.splitter.restoreState(state)

    def setModel(self, model):
        self.package_list.setModel(model)

    def set_display_style(self, style: AddonManagerDisplayStyle):
        self.display_style = style
        self._setup_ui()

    def _setup_ui(self):
        if self.display_style == AddonManagerDisplayStyle.EXPANDED:
            self._setup_expanded_ui()
        elif self.display_style == AddonManagerDisplayStyle.COMPACT:
            self._setup_compact_ui()
        elif self.display_style == AddonManagerDisplayStyle.COMPOSITE:
            self._setup_composite_ui()
        else:
            raise RuntimeError("Invalid display style")
        self.package_list.set_view_style(self.display_style)

    def _setup_expanded_ui(self):
        self.package_list.show()
        self.package_details.hide()
        self.package_details.button_bar.set_show_back_button(True)

    def _setup_compact_ui(self):
        self.package_list.show()
        self.package_details.hide()
        self.package_details.button_bar.set_show_back_button(True)

    def _setup_composite_ui(self):
        self.package_list.show()
        self.package_details.show()
        self.package_details.button_bar.set_show_back_button(False)

    def _setup_connections(self):
        self.package_list.itemSelected.connect(self.addon_selected)
        self.package_details_controller.back.connect(self._back_button_clicked)
        self.package_details_controller.install.connect(self.install)
        self.package_details_controller.uninstall.connect(self.uninstall)
        self.package_details_controller.update.connect(self.update)
        self.package_details_controller.execute.connect(self.execute)
        self.package_details_controller.update_status.connect(self.update_status)
        self.package_list.ui.view_bar.view_changed.connect(self.set_display_style)
        self.splitter.splitterMoved.connect(self._splitter_moved)

    def addon_selected(self, addon):
        """Depending on the display_style, show addon details (possibly hiding the package_list
        widget in the process."""
        self.package_details_controller.show_repo(addon)
        if self.display_style != AddonManagerDisplayStyle.COMPOSITE:
            self.scroll_position = (
                self.package_list.ui.listPackages.verticalScrollBar().sliderPosition()
            )
            print(f"Saved slider position at {self.scroll_position}")
            self.package_list.hide()
            self.package_details.show()
            self.package_details.button_bar.set_show_back_button(True)

    def _back_button_clicked(self):
        if self.display_style != AddonManagerDisplayStyle.COMPOSITE:
            print(f"Set slider position to {self.scroll_position}")
            self.package_list.show()
            self.package_details.hide()
            # The following must be done *after* a cycle through the event loop
            QtCore.QTimer.singleShot(
                0,
                lambda: self.package_list.ui.listPackages.verticalScrollBar().setSliderPosition(
                    self.scroll_position
                ),
            )

    def _splitter_moved(self, _1: int, _2: int) -> None:
        self._save_splitter_state()
