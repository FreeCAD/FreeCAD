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

import FreeCAD
import FreeCADGui

from PySide2.QtWidgets import QFileDialog, QTableWidgetItem
from PySide2.QtGui import QIcon, QValidator, QRegularExpressionValidator, QPixmap
from PySide2.QtCore import QRegularExpression
from addonmanager_git import GitManager

translate = FreeCAD.Qt.translate

# pylint: disable=too-few-public-methods


class AddonGitInterface:
    """Wrapper to handle the git calls needed by this class"""

    git_manager = GitManager()

    def __init__(self, path):
        self.path = path
        self.git_exists = False
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


class NameValidator(QValidator):
    """Simple validator to exclude characters that are not valid in filenames."""

    invalid = '/\\?%*:|"<>'

    def validate(self, value: str, _: int):
        """Check the value against the validator"""
        for char in value:
            if char in NameValidator.invalid:
                return QValidator.Invalid
        return QValidator.Acceptable

    def fixup(self, value: str) -> str:
        """Remove invalid characters from value"""
        result = ""
        for char in value:
            if char not in NameValidator.invalid:
                result += char
        return result


class SemVerValidator(QRegularExpressionValidator):
    """Implements the officially-recommended regex validator for Semantic version numbers."""

    # https://semver.org/#is-there-a-suggested-regular-expression-regex-to-check-a-semver-string
    semver_re = QRegularExpression(
        r"^(?P<major>0|[1-9]\d*)\.(?P<minor>0|[1-9]\d*)\.(?P<patch>0|[1-9]\d*)"
        + r"(?:-(?P<prerelease>(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)"
        + r"(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?"
        + r"(?:\+(?P<buildmetadata>[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$"
    )

    def __init__(self):
        super().__init__()
        self.setRegularExpression(SemVerValidator.semver_re)

    @classmethod
    def check(cls, value: str) -> bool:
        """Returns true if value validates, and false if not"""
        return cls.semver_re.match(value).hasMatch()


class CalVerValidator(QRegularExpressionValidator):
    """Implements a basic regular expression validator that makes sure an entry corresponds
    to a CalVer version numbering standard."""

    calver_re = QRegularExpression(
        r"^(?P<major>[1-9]\d{3})\.(?P<minor>[0-9]{1,2})\.(?P<patch>0|[0-9]{0,2})"
        + r"(?:-(?P<prerelease>(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)"
        + r"(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?"
        + r"(?:\+(?P<buildmetadata>[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$"
    )

    def __init__(self):
        super().__init__()
        self.setRegularExpression(CalVerValidator.calver_re)

    @classmethod
    def check(cls, value: str) -> bool:
        """Returns true if value validates, and false if not"""
        return cls.calver_re.match(value).hasMatch()


class VersionValidator(QValidator):
    """Implements the officially-recommended regex validator for Semantic version numbers, and a
    decent approximation of the same thing for CalVer-style version numbers."""

    def __init__(self):
        super().__init__()
        self.semver = SemVerValidator()
        self.calver = CalVerValidator()

    def validate(self, value: str, position: int):
        """Called for validation, returns a tuple of the validation state, the value, and the
        position."""
        semver_result = self.semver.validate(value, position)
        calver_result = self.calver.validate(value, position)

        if semver_result[0] == QValidator.Acceptable:
            return semver_result
        if calver_result[0] == QValidator.Acceptable:
            return calver_result
        if semver_result[0] == QValidator.Intermediate:
            return semver_result
        if calver_result[0] == QValidator.Intermediate:
            return calver_result
        return (QValidator.Invalid, value, position)


class DeveloperMode:
    """The main Developer Mode dialog, for editing package.xml metadata graphically."""

    def __init__(self):
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "developer_mode.ui")
        )
        self.pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        self.current_mod:str = ""
        self.git_interface = None
        self.has_toplevel_icon = False
        self._setup_dialog_signals()

        self.dialog.displayNameLineEdit.setValidator(NameValidator())
        self.dialog.versionLineEdit.setValidator(VersionValidator())

        self.dialog.addPersonToolButton.setIcon(
            QIcon.fromTheme("add", QIcon(":/icons/list-add.svg"))
        )
        self.dialog.removePersonToolButton.setIcon(
            QIcon.fromTheme("remove", QIcon(":/icons/list-remove.svg"))
        )
        self.dialog.addLicenseToolButton.setIcon(
            QIcon.fromTheme("add", QIcon(":/icons/list-add.svg"))
        )
        self.dialog.removeLicenseToolButton.setIcon(
            QIcon.fromTheme("remove", QIcon(":/icons/list-remove.svg"))
        )
        self.dialog.addContentItemToolButton.setIcon(
            QIcon.fromTheme("add", QIcon(":/icons/list-add.svg"))
        )
        self.dialog.removeContentItemToolButton.setIcon(
            QIcon.fromTheme("remove", QIcon(":/icons/list-remove.svg"))
        )

    def show(self, parent=None):
        """Show the main dev mode dialog"""
        if parent:
            self.dialog.setParent(parent)
        self.dialog.exec()

    def _populate_dialog(self, path_to_repo):
        """Populate this dialog using the best available parsing of the contents of the repo at
        path_to_repo. This is a multi-layered process that starts with any existing package.xml
        file or other known metadata files, and proceeds through examining the contents of the
        directory structure."""
        if self.current_mod == path_to_repo:
            return
        self.current_mod = path_to_repo
        self._scan_for_git_info(self.current_mod)

        metadata_path = os.path.join(path_to_repo, "package.xml")
        metadata = None
        if os.path.exists(metadata_path):
            try:
                metadata = FreeCAD.Metadata(metadata_path)
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

        if metadata:
            self.dialog.displayNameLineEdit.setText(metadata.Name)
            self.dialog.descriptionTextEdit.setPlainText(metadata.Description)
            self.dialog.versionLineEdit.setText(metadata.Version)

            self._populate_people_from_metadata(metadata)
            self._populate_licenses_from_metadata(metadata)
            self._populate_urls_from_metadata(metadata)
            self._populate_contents_from_metadata(metadata)

            self._populate_icon_from_metadata(metadata)
        else:
            self._populate_without_metadata()

    def _populate_people_from_metadata(self, metadata):
        """Use the passed metadata object to populate the maintainers and authors"""
        self.dialog.peopleTableWidget.setRowCount(0)
        row = 0
        for maintainer in metadata.Maintainer:
            name = maintainer["name"]
            email = maintainer["email"]
            self.dialog.peopleTableWidget.insertRow(row)
            self.dialog.peopleTableWidget.setItem(
                row, 0, QTableWidgetItem(translate("AddonsInstaller", "Maintainer"))
            )
            self.dialog.peopleTableWidget.setItem(row, 1, QTableWidgetItem(name))
            self.dialog.peopleTableWidget.setItem(row, 2, QTableWidgetItem(email))
            row += 1
        for author in metadata.Author:
            name = author["name"]
            email = author["email"]
            self.dialog.peopleTableWidget.insertRow(row)
            self.dialog.peopleTableWidget.setItem(
                row, 0, QTableWidgetItem(translate("AddonsInstaller", "Author"))
            )
            self.dialog.peopleTableWidget.setItem(row, 1, QTableWidgetItem(name))
            self.dialog.peopleTableWidget.setItem(row, 2, QTableWidgetItem(email))
            row += 1

        if row == 0:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "WARNING: No maintainer data found in metadata file.",
                )
                + "\n"
            )

    def _populate_licenses_from_metadata(self, metadata):
        """Use the passed metadata object to populate the licenses"""
        self.dialog.licensesTableWidget.setRowCount(0)
        row = 0
        for lic in metadata.License:
            name = lic["name"]
            path = lic["file"]
            self.dialog.licensesTableWidget.insertRow(row)
            self.dialog.licensesTableWidget.setItem(row, 0, QTableWidgetItem(name))
            self.dialog.licensesTableWidget.setItem(row, 1, QTableWidgetItem(path))
            full_path = os.path.join(self.current_mod, path)
            if not os.path.isfile(full_path):
                FreeCAD.Console.PrintError(
                    translate(
                        "AddonsInstaller",
                        "ERROR: Could not locate license file at {}",
                    ).format(full_path)
                    + "\n"
                )
            row += 1
        if row == 0:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "WARNING: No license data found in metadata file",
                )
                + "\n"
            )

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

    def _populate_contents_from_metadata(self, metadata):
        """Use the passed metadata object to populate the contents list"""
        contents = metadata.Content
        self.dialog.contentsListWidget.clear()
        for content_type in contents:
            for item in contents[content_type]:
                contents_string = f"[{content_type}] "
                info = []
                if item.Name:
                    info.append(translate("AddonsInstaller", "Name") + ": " + item.Name)
                if item.Classname:
                    info.append(
                        translate("AddonsInstaller", "Class") + ": " + item.Classname
                    )
                if item.Description:
                    info.append(
                        translate("AddonsInstaller", "Description")
                        + ": "
                        + item.Description
                    )
                if item.Subdirectory:
                    info.append(
                        translate("AddonsInstaller", "Subdirectory")
                        + ": "
                        + item.Subdirectory
                    )
                if item.File:
                    info.append(
                        translate("AddonsInstaller", "Files")
                        + ": "
                        + ", ".join(item.File)
                    )
                contents_string += ", ".join(info)

                self.dialog.contentsListWidget.addItem(contents_string)

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
            if contents["workbench"]:
                for wb in contents["workbench"]:
                    icon = wb.Icon
                    path = wb.Subdirectory
                    if icon:
                        icon_path = os.path.join(
                            self.current_mod, path, icon.replace("/", os.path.sep)
                        )
                        break

        if os.path.isfile(icon_path):
            icon_data = QIcon(icon_path)
            if not icon_data.isNull():
                self.dialog.iconDisplayLabel.setPixmap(icon_data.pixmap(32, 32))
        self.dialog.iconPathLineEdit.setText(icon)

    def _populate_without_metadata(self):
        """If there is no metadata, try to guess at values for it"""
        self._clear_all_fields()

    def _scan_for_git_info(self, path):
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
        self.dialog.iconDisplayLabel.setPixmap(QPixmap())
        self.dialog.iconPathLineEdit.clear()

    def _setup_dialog_signals(self):
        """Set up the signal and slot connections for the main dialog."""

        self.dialog.addonPathBrowseButton.clicked.connect(
            self._addon_browse_button_clicked
        )
        self.dialog.pathToAddonComboBox.editTextChanged.connect(
            self._addon_combo_text_changed
        )

        # Finally, populate the combo boxes, etc.
        self._populate_combo()
        if self.dialog.pathToAddonComboBox.currentIndex() != -1:
            self._populate_dialog(self.dialog.pathToAddonComboBox.currentText())

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

            # Remove the whole thing so we can recreate it from scratch
            self.pref.RemGroup("recentModsList")

        if recent_mod_paths:
            recent_mods_group = self.pref.GetGroup("recentModsList")
            for i, mod in zip(range(10), recent_mod_paths):
                entry_name = f"Mod{i}"
                recent_mods_group.SetString(entry_name, mod)
