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

from dataclasses import dataclass
from enum import Enum, auto
import os
from typing import Optional

try:
    import FreeCAD

    translate = FreeCAD.Qt.translate
except ImportError:
    FreeCAD = None

    def translate(_: str, text: str):
        return text


# Get whatever version of PySide we can
try:
    import PySide  # Use the FreeCAD wrapper
except ImportError:
    try:
        import PySide6  # Outside FreeCAD, try Qt6 first

        PySide = PySide6
    except ImportError:
        import PySide2  # Fall back to Qt5 (if this fails, Python will kill this module's import)

        PySide = PySide2

from PySide import QtCore, QtWidgets

from .addonmanager_widget_addon_buttons import WidgetAddonButtons
from .addonmanager_widget_readme_browser import WidgetReadmeBrowser
from .addonmanager_colors import warning_color_string, attention_color_string, bright_color_string


class MessageType(Enum):
    Message = auto()
    Warning = auto()
    Error = auto()


@dataclass
class UpdateInformation:
    unchecked: bool = True
    check_in_progress: bool = False
    update_available: bool = False
    detached_head: bool = False
    version: str = ""
    tag: str = ""
    branch: Optional[str] = None


@dataclass
class WarningFlags:
    obsolete: bool = False
    python2: bool = False
    required_freecad_version: Optional[str] = None
    non_osi_approved = False
    non_fsf_libre = False


class PackageDetailsView(QtWidgets.QWidget):
    """The view class for the package details"""

    def __init__(self, parent: QtWidgets.QWidget = None):
        super().__init__(parent)
        self.button_bar = None
        self.readme_browser = None
        self.message_label = None
        self.location_label = None
        self.url_label = None
        self.installed = False
        self.disabled = False
        self.update_info = UpdateInformation()
        self.warning_flags = WarningFlags()
        self.installed_version = None
        self.installed_branch = None
        self.installed_timestamp = None
        self.can_disable = True
        self._setup_ui()

    def _setup_ui(self):
        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.button_bar = WidgetAddonButtons(self)
        self.readme_browser = WidgetReadmeBrowser(self)
        self.message_label = QtWidgets.QLabel(self)
        self.location_label = QtWidgets.QLabel(self)
        self.url_label = QtWidgets.QLabel(self)
        self.url_label.setOpenExternalLinks(True)
        self.location_label.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)
        self.vertical_layout.addWidget(self.button_bar)
        self.vertical_layout.addWidget(self.message_label)
        self.vertical_layout.addWidget(self.location_label)
        self.vertical_layout.addWidget(self.url_label)
        self.vertical_layout.addWidget(self.readme_browser)
        self.button_bar.hide()  # Start with no bar

    def set_location(self, location: Optional[str]):
        if location is not None:
            text = (
                translate("AddonsInstaller", "Installation location")
                + ": "
                + os.path.normpath(location)
            )
            self.location_label.setText(text)
            self.location_label.show()
        else:
            self.location_label.hide()

    def set_url(self, url: Optional[str]):
        if url is not None:
            text = (
                translate("AddonsInstaller", "Repository URL")
                + ': <a href="'
                + url
                + '">'
                + url
                + "</a>"
            )
            self.url_label.setText(text)
            self.url_label.show()
        else:
            self.url_label.hide()

    def set_installed(
        self,
        installed: bool,
        on_date: Optional[str] = None,
        version: Optional[str] = None,
        branch: Optional[str] = None,
    ):
        self.installed = installed
        self.installed_timestamp = on_date
        self.installed_version = version
        self.installed_branch = branch
        if not self.installed:
            self.set_location(None)
        self._sync_ui_state()

    def set_update_available(self, info: UpdateInformation):
        self.update_info = info
        self._sync_ui_state()

    def set_disabled(self, disabled: bool):
        self.disabled = disabled
        self._sync_ui_state()

    def allow_disabling(self, allow: bool):
        self.can_disable = allow
        self._sync_ui_state()

    def allow_running(self, show: bool):
        self.button_bar.run_macro.setVisible(show)

    def set_warning_flags(self, flags: WarningFlags):
        self.warning_flags = flags
        self._sync_ui_state()

    def set_new_disabled_status(self, disabled: bool):
        """If the user just changed the enabled/disabled state of the addon, display a message
        indicating that will not take place until restart. Do not call except in a case of a
        state change during this run."""

        if disabled:
            message = translate(
                "AddonsInstaller", "This Addon will be disabled next time you restart FreeCAD."
            )
        else:
            message = translate(
                "AddonsInstaller", "This Addon will be enabled next time you restart FreeCAD."
            )
        self.message_label.setText(f"<h3>{message}</h3>")
        self.message_label.setStyleSheet("color:" + attention_color_string())

    def set_new_branch(self, branch: str):
        """If the user just changed branches, update the message to show that a restart is
        needed."""
        message_string = "<h3>"
        message_string += translate(
            "AddonsInstaller", "Changed to branch '{}' -- please restart to use Addon."
        ).format(branch)
        message_string += "</h3>"
        self.message_label.setText(message_string)
        self.message_label.setStyleSheet("color:" + attention_color_string())

    def set_updated(self):
        """If the user has just updated the addon but not yet restarted, show an indication that
        we are awaiting a restart."""
        message = translate(
            "AddonsInstaller", "This Addon has been updated. Restart FreeCAD to see changes."
        )
        self.message_label.setText(f"<h3>{message}</h3>")
        self.message_label.setStyleSheet("color:" + attention_color_string())

    def _sync_ui_state(self):
        self._sync_button_state()
        self._create_status_label_text()

    def _sync_button_state(self):
        self.button_bar.install.setVisible(not self.installed)
        self.button_bar.uninstall.setVisible(self.installed)
        if not self.installed:
            self.button_bar.disable.hide()
            self.button_bar.enable.hide()
            self.button_bar.update.hide()
            self.button_bar.check_for_update.hide()
        else:
            self.button_bar.update.setVisible(self.update_info.update_available)
            if self.update_info.detached_head:
                self.button_bar.check_for_update.hide()
            else:
                self.button_bar.check_for_update.setVisible(not self.update_info.update_available)
            if self.can_disable:
                self.button_bar.enable.setVisible(self.disabled)
                self.button_bar.disable.setVisible(not self.disabled)
            else:
                self.button_bar.enable.hide()
                self.button_bar.disable.hide()

    def _create_status_label_text(self):
        if self.installed:
            installation_details = self._get_installation_details_string()
            update_details = self._get_update_status_string()
            message_text = f"{installation_details} {update_details}"
            if self.disabled:
                message_text += " [" + translate("AddonsInstaller", "Disabled") + "]"
            self.message_label.setText(f"<h3>{message_text}</h3>")
            if self.disabled:
                self.message_label.setStyleSheet("color:" + warning_color_string())
            elif self.update_info.update_available:
                self.message_label.setStyleSheet("color:" + attention_color_string())
            else:
                self.message_label.setStyleSheet("color:" + bright_color_string())
            self.message_label.show()
        elif self._there_are_warnings_to_show():
            warnings = self._get_warning_string()
            self.message_label.setText(f"<h3>{warnings}</h3>")
            self.message_label.setStyleSheet("color:" + warning_color_string())
            self.message_label.show()
        else:
            self.message_label.hide()

    def _get_installation_details_string(self) -> str:
        version = self.installed_version
        date = ""
        installed_version_string = ""
        if self.installed_timestamp:
            date = QtCore.QLocale().toString(
                QtCore.QDateTime.fromSecsSinceEpoch(int(round(self.installed_timestamp, 0))),
                QtCore.QLocale.ShortFormat,
            )
        if version and date:
            installed_version_string += (
                translate("AddonsInstaller", "Version {version} installed on {date}").format(
                    version=version, date=date
                )
                + ". "
            )
        elif version:
            installed_version_string += (
                translate("AddonsInstaller", "Version {version} installed") + "."
            ).format(version=version)
        elif date:
            installed_version_string += (
                translate("AddonsInstaller", "Installed on {date}") + "."
            ).format(date=date)
        else:
            installed_version_string += translate("AddonsInstaller", "Installed") + "."
        return installed_version_string

    def _get_update_status_string(self) -> str:
        if self.update_info.check_in_progress:
            return translate("AddonsInstaller", "Update check in progress") + "."
        elif self.update_info.unchecked:
            return ""
        if self.update_info.detached_head:
            return (
                translate(
                    "AddonsInstaller", "Git tag '{}' checked out, no updates possible"
                ).format(self.update_info.tag)
                + "."
            )
        if self.update_info.update_available:
            if self.installed_branch and self.update_info.branch:
                if self.installed_branch != self.update_info.branch:
                    return (
                        translate(
                            "AddonsInstaller", "Currently on branch {}, name changed to {}"
                        ).format(self.installed_branch, self.update_info.branch)
                        + "."
                    )
                if self.update_info.version:
                    return (
                        translate(
                            "AddonsInstaller",
                            "Currently on branch {}, update available to version {}",
                        ).format(self.installed_branch, str(self.update_info.version).strip())
                        + "."
                    )
                return translate("AddonsInstaller", "Update available") + "."
            if self.update_info.version:
                return (
                    translate("AddonsInstaller", "Update available to version {}").format(
                        str(self.update_info.version).strip()
                    )
                    + "."
                )
            return translate("AddonsInstaller", "Update available") + "."
        return translate("AddonsInstaller", "This is the latest version available") + "."

    def _there_are_warnings_to_show(self) -> bool:
        if self.disabled:
            return True
        if (
            self.warning_flags.obsolete
            or self.warning_flags.python2
            or self.warning_flags.required_freecad_version
        ):
            return True
        return False  # TODO: Someday support optional warnings on license types

    def _get_warning_string(self) -> str:
        if self.installed and self.disabled:
            return translate(
                "AddonsInstaller",
                "WARNING: This addon is currently installed, but disabled. Use the 'enable' "
                "button to re-enable.",
            )
        if self.warning_flags.obsolete:
            return translate("AddonsInstaller", "WARNING: This addon is obsolete")
        if self.warning_flags.python2:
            return translate("AddonsInstaller", "WARNING: This addon is Python 2 only")
        if self.warning_flags.required_freecad_version:
            return translate("AddonsInstaller", "WARNING: This addon requires FreeCAD {}").format(
                self.warning_flags.required_freecad_version
            )
        return ""
