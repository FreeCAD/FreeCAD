# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
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

""" Classes to manage "Developer Mode" """

import os
import datetime
import subprocess

import FreeCAD
import FreeCADGui

from PySide.QtWidgets import (
    QFileDialog,
    QListWidgetItem,
    QDialog,
    QSizePolicy,
    QMessageBox,
)
from PySide.QtGui import (
    QIcon,
    QPixmap,
)
from PySide.QtCore import Qt
from addonmanager_git import GitManager, NoGitFound

from addonmanager_devmode_add_content import AddContent
from addonmanager_devmode_validators import NameValidator, VersionValidator
from addonmanager_devmode_predictor import Predictor
from addonmanager_devmode_people_table import PeopleTable
from addonmanager_devmode_licenses_table import LicensesTable
import addonmanager_utilities as utils

translate = FreeCAD.Qt.translate

# pylint: disable=too-few-public-methods

ContentTypeRole = Qt.UserRole
ContentIndexRole = Qt.UserRole + 1


class AddonGitInterface:
    """Wrapper to handle the git calls needed by this class"""

    git_manager = None

    def __init__(self, path):
        self.git_exists = False
        if not AddonGitInterface.git_manager:
            try:
                AddonGitInterface.git_manager = GitManager()
            except NoGitFound:
                FreeCAD.Console.PrintLog("No git found, Addon Manager Developer Mode disabled.")
                return

        self.path = path
        if os.path.exists(os.path.join(path, ".git")):
            self.git_exists = True
            self.branch = AddonGitInterface.git_manager.current_branch(self.path)
            self.remote = AddonGitInterface.git_manager.get_remote(self.path)

    @property
    def branches(self):
        """The branches available for this repo."""
        if self.git_exists:
            return AddonGitInterface.git_manager.get_branches(self.path)
        return []

    @property
    def committers(self):
        """The committers to this repo, in the last ten commits"""
        if self.git_exists:
            return AddonGitInterface.git_manager.get_last_committers(self.path, 10)
        return []

    @property
    def authors(self):
        """The committers to this repo, in the last ten commits"""
        if self.git_exists:
            return AddonGitInterface.git_manager.get_last_authors(self.path, 10)
        return []


# pylint: disable=too-many-instance-attributes


class DeveloperMode:
    """The main Developer Mode dialog, for editing package.xml metadata graphically."""

    def __init__(self):
        # In the UI we want to show a translated string for the person type, but the underlying
        # string must be the one expected by the metadata parser, in English
        self.person_type_translation = {
            "maintainer": translate("AddonsInstaller", "Maintainer"),
            "author": translate("AddonsInstaller", "Author"),
        }
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "developer_mode.ui")
        )
        self.people_table = PeopleTable()
        self.licenses_table = LicensesTable()
        large_size_policy = QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        large_size_policy.setHorizontalStretch(2)
        small_size_policy = QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        small_size_policy.setHorizontalStretch(1)
        self.people_table.widget.setSizePolicy(large_size_policy)
        self.licenses_table.widget.setSizePolicy(small_size_policy)
        self.dialog.peopleAndLicenseshorizontalLayout.addWidget(self.people_table.widget)
        self.dialog.peopleAndLicenseshorizontalLayout.addWidget(self.licenses_table.widget)
        self.pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        self.current_mod: str = ""
        self.git_interface = None
        self.has_toplevel_icon = False
        self.metadata = None

        self._setup_dialog_signals()

        self.dialog.displayNameLineEdit.setValidator(NameValidator())
        self.dialog.versionLineEdit.setValidator(VersionValidator())
        self.dialog.minPythonLineEdit.setValidator(VersionValidator())

        self.dialog.addContentItemToolButton.setIcon(
            QIcon.fromTheme("add", QIcon(":/icons/list-add.svg"))
        )
        self.dialog.removeContentItemToolButton.setIcon(
            QIcon.fromTheme("remove", QIcon(":/icons/list-remove.svg"))
        )

    def show(self, parent=None, path: str = None):
        """Show the main dev mode dialog"""
        if parent:
            self.dialog.setParent(parent)
        if path and os.path.exists(path):
            self.dialog.pathToAddonComboBox.setEditText(path)
        elif self.pref.HasGroup("recentModsList"):
            recent_mods_group = self.pref.GetGroup("recentModsList")
            entry = recent_mods_group.GetString("Mod0", "")
            if entry:
                self._populate_dialog(entry)
                self._update_recent_mods(entry)
                self._populate_combo()
            else:
                self._clear_all_fields()
        else:
            self._clear_all_fields()

        result = self.dialog.exec()
        if result == QDialog.Accepted:
            self._sync_metadata_to_ui()
            now = datetime.date.today()
            self.metadata.Date = str(now)
            self.metadata.write(os.path.join(self.current_mod, "package.xml"))

    def _populate_dialog(self, path_to_repo: str):
        """Populate this dialog using the best available parsing of the contents of the repo at
        path_to_repo. This is a multi-layered process that starts with any existing package.xml
        file or other known metadata files, and proceeds through examining the contents of the
        directory structure."""
        self.current_mod = path_to_repo
        self._scan_for_git_info(self.current_mod)

        metadata_path = os.path.join(path_to_repo, "package.xml")
        if os.path.exists(metadata_path):
            try:
                self.metadata = FreeCAD.Metadata(metadata_path)
            except FreeCAD.Base.XMLBaseException as e:
                FreeCAD.Console.PrintError(
                    translate(
                        "AddonsInstaller",
                        "XML failure while reading metadata from file {}",
                    ).format(metadata_path)
                    + "\n\n"
                    + str(e)
                    + "\n\n"
                )
            except FreeCAD.Base.RuntimeError as e:
                FreeCAD.Console.PrintError(
                    translate("AddonsInstaller", "Invalid metadata in file {}").format(
                        metadata_path
                    )
                    + "\n\n"
                    + str(e)
                    + "\n\n"
                )

        self._clear_all_fields()

        if not self.metadata:
            self._predict_metadata()

        self.dialog.displayNameLineEdit.setText(self.metadata.Name)
        self.dialog.descriptionTextEdit.setPlainText(self.metadata.Description)
        self.dialog.versionLineEdit.setText(self.metadata.Version)
        self.dialog.minPythonLineEdit.setText(self.metadata.PythonMin)

        self._populate_urls_from_metadata(self.metadata)
        self._populate_contents_from_metadata(self.metadata)

        self._populate_icon_from_metadata(self.metadata)

        self.people_table.show(self.metadata)
        self.licenses_table.show(self.metadata, self.current_mod)

    def _populate_urls_from_metadata(self, metadata):
        """Use the passed metadata object to populate the urls"""
        for url in metadata.Urls:
            if url["type"] == "website":
                self.dialog.websiteURLLineEdit.setText(url["location"])
            elif url["type"] == "repository":
                self.dialog.repositoryURLLineEdit.setText(url["location"])
                branch_from_metadata = url["branch"]
                branch_from_local_path = self.git_interface.branch
                if branch_from_metadata != branch_from_local_path:
                    # pylint: disable=line-too-long
                    FreeCAD.Console.PrintWarning(
                        translate(
                            "AddonsInstaller",
                            "WARNING: Path specified in package.xml metadata does not match currently checked-out branch.",
                        )
                        + "\n"
                    )
                self.dialog.branchComboBox.setCurrentText(branch_from_metadata)
            elif url["type"] == "bugtracker":
                self.dialog.bugtrackerURLLineEdit.setText(url["location"])
            elif url["type"] == "readme":
                self.dialog.readmeURLLineEdit.setText(url["location"])
            elif url["type"] == "documentation":
                self.dialog.documentationURLLineEdit.setText(url["location"])
            elif url["type"] == "discussion":
                self.dialog.discussionURLLineEdit.setText(url["location"])

    def _populate_contents_from_metadata(self, metadata):
        """Use the passed metadata object to populate the contents list"""
        contents = metadata.Content
        self.dialog.contentsListWidget.clear()
        for content_type in contents:
            counter = 0
            for item in contents[content_type]:
                contents_string = f"[{content_type}] "
                info = []
                if item.Name:
                    info.append(translate("AddonsInstaller", "Name") + ": " + item.Name)
                if item.Classname:
                    info.append(translate("AddonsInstaller", "Class") + ": " + item.Classname)
                if item.Description:
                    info.append(
                        translate("AddonsInstaller", "Description") + ": " + item.Description
                    )
                if item.Subdirectory:
                    info.append(
                        translate("AddonsInstaller", "Subdirectory") + ": " + item.Subdirectory
                    )
                if item.File:
                    info.append(translate("AddonsInstaller", "Files") + ": " + ", ".join(item.File))
                contents_string += ", ".join(info)

                item = QListWidgetItem(contents_string)
                item.setData(ContentTypeRole, content_type)
                item.setData(ContentIndexRole, counter)
                self.dialog.contentsListWidget.addItem(item)
                counter += 1

    def _populate_icon_from_metadata(self, metadata):
        """Use the passed metadata object to populate the icon fields"""
        self.dialog.iconDisplayLabel.setPixmap(QPixmap())
        icon = metadata.Icon
        icon_path = None
        if icon:
            icon_path = os.path.join(self.current_mod, icon.replace("/", os.path.sep))
            self.has_toplevel_icon = True
        else:
            self.has_toplevel_icon = False
            contents = metadata.Content
            if "workbench" in contents:
                for wb in contents["workbench"]:
                    icon = wb.Icon
                    path = wb.Subdirectory
                    if icon:
                        icon_path = os.path.join(
                            self.current_mod, path, icon.replace("/", os.path.sep)
                        )
                        break

        if icon_path and os.path.isfile(icon_path):
            icon_data = QIcon(icon_path)
            if not icon_data.isNull():
                self.dialog.iconDisplayLabel.setPixmap(icon_data.pixmap(32, 32))
        self.dialog.iconPathLineEdit.setText(icon)

    def _predict_metadata(self):
        """If there is no metadata, try to guess at values for it"""
        self.metadata = FreeCAD.Metadata()
        predictor = Predictor()
        self.metadata = predictor.predict_metadata(self.current_mod)

    def _scan_for_git_info(self, path: str):
        """Look for branch availability"""
        self.git_interface = AddonGitInterface(path)
        if self.git_interface.git_exists:
            self.dialog.branchComboBox.clear()
            for branch in self.git_interface.branches:
                if branch and branch.startswith("origin/") and branch != "origin/HEAD":
                    self.dialog.branchComboBox.addItem(branch[len("origin/") :])
            self.dialog.branchComboBox.setCurrentText(self.git_interface.branch)

    def _clear_all_fields(self):
        """Clear out all fields"""
        self.dialog.displayNameLineEdit.clear()
        self.dialog.descriptionTextEdit.clear()
        self.dialog.versionLineEdit.clear()
        self.dialog.websiteURLLineEdit.clear()
        self.dialog.repositoryURLLineEdit.clear()
        self.dialog.bugtrackerURLLineEdit.clear()
        self.dialog.readmeURLLineEdit.clear()
        self.dialog.documentationURLLineEdit.clear()
        self.dialog.discussionURLLineEdit.clear()
        self.dialog.minPythonLineEdit.clear()
        self.dialog.iconDisplayLabel.setPixmap(QPixmap())
        self.dialog.iconPathLineEdit.clear()

    def _setup_dialog_signals(self):
        """Set up the signal and slot connections for the main dialog."""

        self.dialog.addonPathBrowseButton.clicked.connect(self._addon_browse_button_clicked)
        self.dialog.pathToAddonComboBox.editTextChanged.connect(self._addon_combo_text_changed)
        self.dialog.detectMinPythonButton.clicked.connect(self._detect_min_python_clicked)
        self.dialog.iconBrowseButton.clicked.connect(self._browse_for_icon_clicked)

        self.dialog.addContentItemToolButton.clicked.connect(self._add_content_clicked)
        self.dialog.removeContentItemToolButton.clicked.connect(self._remove_content_clicked)
        self.dialog.contentsListWidget.itemSelectionChanged.connect(self._content_selection_changed)
        self.dialog.contentsListWidget.itemDoubleClicked.connect(self._edit_content)

        self.dialog.versionToTodayButton.clicked.connect(self._set_to_today_clicked)

        # Finally, populate the combo boxes, etc.
        self._populate_combo()

        # Disable all the "Remove" buttons until something is selected
        self.dialog.removeContentItemToolButton.setDisabled(True)

    def _sync_metadata_to_ui(self):
        """Take the data from the UI fields and put it into the stored metadata
        object. Only overwrites known data fields: unknown metadata will be retained."""

        if not self.metadata:
            self.metadata = FreeCAD.Metadata()

        self.metadata.Name = self.dialog.displayNameLineEdit.text()
        self.metadata.Description = self.dialog.descriptionTextEdit.document().toPlainText()
        self.metadata.Version = self.dialog.versionLineEdit.text()
        self.metadata.Icon = self.dialog.iconPathLineEdit.text()

        urls = []
        if self.dialog.websiteURLLineEdit.text():
            urls.append({"location": self.dialog.websiteURLLineEdit.text(), "type": "website"})
        if self.dialog.repositoryURLLineEdit.text():
            urls.append(
                {
                    "location": self.dialog.repositoryURLLineEdit.text(),
                    "type": "repository",
                    "branch": self.dialog.branchComboBox.currentText(),
                }
            )
        if self.dialog.bugtrackerURLLineEdit.text():
            urls.append(
                {
                    "location": self.dialog.bugtrackerURLLineEdit.text(),
                    "type": "bugtracker",
                }
            )
        if self.dialog.readmeURLLineEdit.text():
            urls.append({"location": self.dialog.readmeURLLineEdit.text(), "type": "readme"})
        if self.dialog.documentationURLLineEdit.text():
            urls.append(
                {
                    "location": self.dialog.documentationURLLineEdit.text(),
                    "type": "documentation",
                }
            )
        if self.dialog.discussionURLLineEdit.text():
            urls.append(
                {
                    "location": self.dialog.discussionURLLineEdit.text(),
                    "type": "discussion",
                }
            )
        self.metadata.Urls = urls

        if self.dialog.minPythonLineEdit.text():
            self.metadata.PythonMin = self.dialog.minPythonLineEdit.text()
        else:
            self.metadata.PythonMin = "0.0.0"  # Code for "unset"

        # Content, people, and licenses should already be synchronized

    ###############################################################################################
    #                                         DIALOG SLOTS
    ###############################################################################################

    def _addon_browse_button_clicked(self):
        """Launch a modal file/folder selection dialog -- if something is selected, it is
        processed by the parsing code and used to fill in the contents of the rest of the
        dialog."""

        start_dir = os.path.join(FreeCAD.getUserAppDataDir(), "Mod")
        mod_dir = QFileDialog.getExistingDirectory(
            parent=self.dialog,
            caption=translate(
                "AddonsInstaller",
                "Select the folder containing your Addon",
            ),
            dir=start_dir,
        )

        if mod_dir and os.path.exists(mod_dir):
            self.dialog.pathToAddonComboBox.setEditText(mod_dir)

    def _addon_combo_text_changed(self, new_text: str):
        """Called when the text is changed, either because it was directly edited, or because
        a new item was selected."""
        if new_text == self.current_mod:
            # It doesn't look like it actually changed, bail out
            return
        self.metadata = None
        self._clear_all_fields()
        if not os.path.exists(new_text):
            # This isn't a thing (Yet. Maybe the user is still typing?)
            return
        self._populate_dialog(new_text)
        self._update_recent_mods(new_text)
        self._populate_combo()

    def _populate_combo(self):
        """Fill in the combo box with the values from the stored recent mods list, selecting the
        top one. Does not trigger any signals."""
        combo = self.dialog.pathToAddonComboBox
        combo.blockSignals(True)
        recent_mods_group = self.pref.GetGroup("recentModsList")
        recent_mods = set()
        combo.clear()
        for i in range(10):
            entry_name = f"Mod{i}"
            mod = recent_mods_group.GetString(entry_name, "None")
            if mod != "None" and mod not in recent_mods and os.path.exists(mod):
                recent_mods.add(mod)
                combo.addItem(mod)
        if recent_mods:
            combo.setCurrentIndex(0)
        combo.blockSignals(False)

    def _update_recent_mods(self, path):
        """Update the list of recent mods, storing at most ten, with path at the top of the
        list."""
        recent_mod_paths = [path]
        if self.pref.HasGroup("recentModsList"):
            recent_mods_group = self.pref.GetGroup("recentModsList")

            # This group has a maximum of ten entries, sorted by last-accessed date
            for i in range(0, 10):
                entry_name = f"Mod{i}"
                entry = recent_mods_group.GetString(entry_name, "")
                if entry and entry not in recent_mod_paths and os.path.exists(entry):
                    recent_mod_paths.append(entry)

            # Remove the whole thing, so we can recreate it from scratch
            self.pref.RemGroup("recentModsList")

        if recent_mod_paths:
            recent_mods_group = self.pref.GetGroup("recentModsList")
            for i, mod in zip(range(10), recent_mod_paths):
                entry_name = f"Mod{i}"
                recent_mods_group.SetString(entry_name, mod)

    def _add_content_clicked(self):
        """Callback: The Add Content button was clicked"""
        dlg = AddContent(self.current_mod, self.metadata)
        singleton = False
        if self.dialog.contentsListWidget.count() == 0:
            singleton = True
        content_type, new_metadata = dlg.exec(singleton=singleton)
        if content_type and new_metadata:
            self.metadata.addContentItem(content_type, new_metadata)
            self._populate_contents_from_metadata(self.metadata)

    def _remove_content_clicked(self):
        """Callback: the remove content button was clicked"""

        item = self.dialog.contentsListWidget.currentItem()
        if not item:
            return
        content_type = item.data(ContentTypeRole)
        content_index = item.data(ContentIndexRole)
        if self.metadata.Content[content_type] and content_index < len(
            self.metadata.Content[content_type]
        ):
            content_name = self.metadata.Content[content_type][content_index].Name
            self.metadata.removeContentItem(content_type, content_name)
            self._populate_contents_from_metadata(self.metadata)

    def _content_selection_changed(self):
        """Callback: the selected content item changed"""
        items = self.dialog.contentsListWidget.selectedItems()
        if items:
            self.dialog.removeContentItemToolButton.setDisabled(False)
        else:
            self.dialog.removeContentItemToolButton.setDisabled(True)

    def _edit_content(self, item):
        """Callback: a content row was double-clicked"""
        dlg = AddContent(self.current_mod, self.metadata)

        content_type = item.data(ContentTypeRole)
        content_index = item.data(ContentIndexRole)

        content = self.metadata.Content
        metadata = content[content_type][content_index]
        old_name = metadata.Name
        new_type, new_metadata = dlg.exec(content_type, metadata, len(content) == 1)
        if new_type and new_metadata:
            self.metadata.removeContentItem(content_type, old_name)
            self.metadata.addContentItem(new_type, new_metadata)
            self._populate_contents_from_metadata(self.metadata)

    def _set_to_today_clicked(self):
        """Callback: the "set to today" button was clicked"""
        year = datetime.date.today().year
        month = datetime.date.today().month
        day = datetime.date.today().day
        version_string = f"{year}.{month:>02}.{day:>02}"
        self.dialog.versionLineEdit.setText(version_string)

    def _detect_min_python_clicked(self):
        if not self._ensure_vermin_loaded():
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "No Vermin, cancelling operation.",
                    "NOTE: Vermin is a Python package and proper noun - do not translate",
                )
                + "\n"
            )
            return
        FreeCAD.Console.PrintMessage(
            translate("AddonsInstaller", "Scanning Addon for Python version compatibility")
            + "...\n"
        )
        # pylint: disable=import-outside-toplevel
        import vermin

        required_minor_version = 0
        for dir_path, _, filenames in os.walk(self.current_mod):
            for filename in filenames:
                if filename.endswith(".py"):
                    with open(os.path.join(dir_path, filename), encoding="utf-8") as f:
                        contents = f.read()
                        version_strings = vermin.version_strings(vermin.detect(contents))
                        version = version_strings.split(",")
                        if len(version) >= 2:
                            # Only care about Py3, and only if there is a dot in the version:
                            if "." in version[1]:
                                py3 = version[1].split(".")
                                major = int(py3[0].strip())
                                minor = int(py3[1].strip())
                                if major == 3:
                                    FreeCAD.Console.PrintLog(
                                        f"Detected Python 3.{minor} required by {filename}\n"
                                    )
                                    required_minor_version = max(required_minor_version, minor)
        self.dialog.minPythonLineEdit.setText(f"3.{required_minor_version}")
        QMessageBox.information(
            self.dialog,
            translate("AddonsInstaller", "Minimum Python Version Detected"),
            translate(
                "AddonsInstaller",
                "Vermin auto-detected a required version of Python 3.{}",
            ).format(required_minor_version),
            QMessageBox.Ok,
        )

    def _ensure_vermin_loaded(self) -> bool:
        try:
            # pylint: disable=import-outside-toplevel,unused-import
            import vermin
        except ImportError:
            # pylint: disable=line-too-long
            response = QMessageBox.question(
                self.dialog,
                translate("AddonsInstaller", "Install Vermin?"),
                translate(
                    "AddonsInstaller",
                    "Auto-detecting the required version of Python for this Addon requires Vermin (https://pypi.org/project/vermin/). OK to install?",
                ),
                QMessageBox.Yes | QMessageBox.Cancel,
            )
            if response == QMessageBox.Cancel:
                return False
            FreeCAD.Console.PrintMessage(
                translate("AddonsInstaller", "Attempting to install Vermin from PyPi") + "...\n"
            )
            python_exe = utils.get_python_exe()
            vendor_path = os.path.join(FreeCAD.getUserAppDataDir(), "AdditionalPythonPackages")
            if not os.path.exists(vendor_path):
                os.makedirs(vendor_path)

            proc = subprocess.run(
                [
                    python_exe,
                    "-m",
                    "pip",
                    "install",
                    "--disable-pip-version-check",
                    "--target",
                    vendor_path,
                    "vermin",
                ],
                capture_output=True,
                check=True,
            )
            FreeCAD.Console.PrintMessage(proc.stdout.decode())
            if proc.returncode != 0:
                QMessageBox.critical(
                    self.dialog,
                    translate("AddonsInstaller", "Installation failed"),
                    translate(
                        "AddonsInstaller",
                        "Failed to install Vermin -- check Report View for details.",
                        "'Vermin' is the name of a Python package, do not translate",
                    ),
                    QMessageBox.Cancel,
                )
                return False
        try:
            # pylint: disable=import-outside-toplevel
            import vermin
        except ImportError:
            QMessageBox.critical(
                self.dialog,
                translate("AddonsInstaller", "Installation failed"),
                translate(
                    "AddonsInstaller",
                    "Failed to import vermin after installation -- cannot scan Addon.",
                    "'vermin' is the name of a Python package, do not translate",
                ),
                QMessageBox.Cancel,
            )
            return False
        return True

    def _browse_for_icon_clicked(self):
        """Callback: when the "Browse..." button for the icon field is clicked"""
        new_icon_path, _ = QFileDialog.getOpenFileName(
            parent=self.dialog,
            caption=translate(
                "AddonsInstaller",
                "Select an icon file for this package",
            ),
            dir=self.current_mod,
        )

        if not new_icon_path:
            return

        base_path = self.current_mod.replace("/", os.path.sep)
        icon_path = new_icon_path.replace("/", os.path.sep)
        if base_path[-1] != os.path.sep:
            base_path += os.path.sep

        if not icon_path.startswith(base_path):
            FreeCAD.Console.PrintError(
                translate("AddonsInstaller", "{} is not a subdirectory of {}").format(
                    new_icon_path, self.current_mod
                )
                + "\n"
            )
            return
        self.metadata.Icon = new_icon_path[len(base_path) :]
        self._populate_icon_from_metadata(self.metadata)
