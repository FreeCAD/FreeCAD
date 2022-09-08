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

from PySide2.QtWidgets import QFileDialog, QTableWidgetItem, QListWidgetItem, QDialog
from PySide2.QtGui import (
    QIcon,
    QPixmap,
)
from PySide2.QtCore import Qt
from addonmanager_git import GitManager

from addonmanager_devmode_license_selector import LicenseSelector
from addonmanager_devmode_person_editor import PersonEditor
from addonmanager_devmode_add_content import AddContent
from addonmanager_devmode_validators import NameValidator, VersionValidator

translate = FreeCAD.Qt.translate

# pylint: disable=too-few-public-methods

ContentTypeRole = Qt.UserRole
ContentIndexRole = Qt.UserRole + 1

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

    @property
    def committers(self):
        """The commiters to this repo, in the last ten commits"""
        if self.git_exists:
            return AddonGitInterface.git_manager.get_last_committers(self.path, 10)
        return []

    @property
    def authors(self):
        """The commiters to this repo, in the last ten commits"""
        if self.git_exists:
            return AddonGitInterface.git_manager.get_last_authors(self.path, 10)
        return []



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
        self.pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        self.current_mod: str = ""
        self.git_interface = None
        self.has_toplevel_icon = False
        self.metadata = None

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
        result = self.dialog.exec()
        if result == QDialog.Accepted:
            self._sync_metadata_to_ui()

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

        self._populate_people_from_metadata(self.metadata)
        self._populate_licenses_from_metadata(self.metadata)
        self._populate_urls_from_metadata(self.metadata)
        self._populate_contents_from_metadata(self.metadata)

        self._populate_icon_from_metadata(self.metadata)

    def _populate_people_from_metadata(self, metadata):
        """Use the passed metadata object to populate the maintainers and authors"""
        self.dialog.peopleTableWidget.setRowCount(0)
        row = 0
        for maintainer in metadata.Maintainer:
            name = maintainer["name"]
            email = maintainer["email"]
            self._add_person_row(row, "maintainer", name, email)
            row += 1
        for author in metadata.Author:
            name = author["name"]
            email = author["email"]
            self._add_person_row(row, "author", name, email)
            row += 1

        if row == 0:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "WARNING: No maintainer data found in metadata file.",
                )
                + "\n"
            )

    def _add_person_row(self, row, person_type, name, email):
        """Add this person to the peopleTableWidget at row given"""
        self.dialog.peopleTableWidget.insertRow(row)
        item = QTableWidgetItem(self.person_type_translation[person_type])
        item.setData(Qt.UserRole, person_type)
        self.dialog.peopleTableWidget.setItem(row, 0, item)
        self.dialog.peopleTableWidget.setItem(row, 1, QTableWidgetItem(name))
        self.dialog.peopleTableWidget.setItem(row, 2, QTableWidgetItem(email))

    def _populate_licenses_from_metadata(self, metadata):
        """Use the passed metadata object to populate the licenses"""
        self.dialog.licensesTableWidget.setRowCount(0)
        row = 0
        for lic in metadata.License:
            name = lic["name"]
            path = lic["file"]
            self._add_license_row(row, name, path)
            row += 1
        if row == 0:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "WARNING: No license data found in metadata file",
                )
                + "\n"
            )

    def _add_license_row(self, row: int, name: str, path: str):
        """ Add a row to the table of licenses """
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
            counter = 0
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

                item = QListWidgetItem (contents_string)
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

    def _predict_metadata(self):
        """If there is no metadata, try to guess at values for it"""
        self.metadata = FreeCAD.Metadata()
        self._predict_author_info()
        self._predict_name()
        self._predict_description()
        self._predict_contents()
        self._predict_icon()
        self._predict_urls()
        self._predict_license()

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
        self.dialog.licensesTableWidget.setRowCount(0)
        self.dialog.peopleTableWidget.setRowCount(0)

    def _predict_author_info(self):
        """ Look at the git commit history and attempt to discern maintainer and author 
        information."""
        
        self.git_interface = AddonGitInterface(path)
        if self.git_interface.git_exists:
            committers = self.git_interface.get_last_committers()
        else:
            return

        # This is a dictionary keyed to the author's name (which can be many different 
        # things, depending on the author) containing two fields, "email" and "count". It
        # is common for there to be multiple entries representing the same human being,
        # so a passing attempt is made to reconcile:
        for key in committers:
            emails = committers[key]["email"]
            if "GitHub" in key:
                # Robotic merge commit (or other similar), ignore
                continue
            # Does any other committer share any of these emails?
            for other_key in committers:
                if other_key == key:
                    continue
                other_emails = committers[other_key]["email"]
                for other_email in other_emails:
                    if other_email in emails:
                        # There is overlap in the two email lists, so this is probably the
                        # same author, with a different name (username, pseudonym, etc.)
                        if not committers[key]["aka"]:
                            committers[key]["aka"] = set()
                        committers[key]["aka"].add(other_key)
                        committers[key]["count"] += committers[other_key]["count"]
                        committers[key]["email"].combine(committers[other_key]["email"])
                        committers.remove(other_key)
                        break
        maintainers = []
        for name,info in committers.items():
            if info["aka"]:
                for other_name in info["aka"]:
                    # Heuristic: the longer name is more likely to be the actual legal name
                    if len(other_name) > len(name):
                        name = other_name
            # There is no logical basis to choose one email address over another, so just
            # take the first one
            email = info["email"][0]
            commit_count = info["count"]
            maintainers.append( {"name":name,"email":email,"count":commit_count} )

        # Sort by count of commits
        maintainers.sort(lambda i:i["count"],reverse=True)

    def _setup_dialog_signals(self):
        """Set up the signal and slot connections for the main dialog."""

        self.dialog.addonPathBrowseButton.clicked.connect(
            self._addon_browse_button_clicked
        )
        self.dialog.pathToAddonComboBox.editTextChanged.connect(
            self._addon_combo_text_changed
        )

        self.dialog.addLicenseToolButton.clicked.connect(self._add_license_clicked)
        self.dialog.removeLicenseToolButton.clicked.connect(
            self._remove_license_clicked
        )
        self.dialog.licensesTableWidget.itemSelectionChanged.connect(
            self._license_selection_changed
        )
        self.dialog.licensesTableWidget.itemDoubleClicked.connect(self._edit_license)

        self.dialog.addPersonToolButton.clicked.connect(self._add_person_clicked)
        self.dialog.removePersonToolButton.clicked.connect(self._remove_person_clicked)
        self.dialog.peopleTableWidget.itemSelectionChanged.connect(
            self._person_selection_changed
        )
        self.dialog.peopleTableWidget.itemDoubleClicked.connect(self._edit_person)

        self.dialog.addContentItemToolButton.clicked.connect(self._add_content_clicked)
        self.dialog.removeContentItemToolButton.clicked.connect(self._remove_content_clicked)
        self.dialog.contentsListWidget.itemSelectionChanged.connect(self._content_selection_changed)
        self.dialog.contentsListWidget.itemDoubleClicked.connect(self._edit_content)

        # Finally, populate the combo boxes, etc.
        self._populate_combo()
        if self.dialog.pathToAddonComboBox.currentIndex() != -1:
            self._populate_dialog(self.dialog.pathToAddonComboBox.currentText())

        # Disable all of the "Remove" buttons until something is selected
        self.dialog.removeLicenseToolButton.setDisabled(True)
        self.dialog.removePersonToolButton.setDisabled(True)
        self.dialog.removeContentItemToolButton.setDisabled(True)

    def _sync_metadata_to_ui(self):
        """ Take the data from the UI fields and put it into the stored metadata
        object. Only overwrites known data fields: unknown metadata will be retained. """
        self.metadata.Name = self.dialog.displayNameLineEdit.text()
        self.metadata.Description = self.descriptionTextEdit.text()
        self.metadata.Version = self.dialog.versionLineEdit.text()
        self.metadata.Icon = self.dialog.iconPathLineEdit.text()
        
        url = {}
        url["website"] = self.dialog.websiteURLLineEdit.text()
        url["repository"] = self.dialog.repositoryURLLineEdit.text()
        url["bugtracker"] = self.dialog.bugtrackerURLLineEdit.text()
        url["readme"] = self.dialog.readmeURLLineEdit.text()
        url["documentation"] = self.dialog.documentationURLLineEdit.text()
        self.metadata.setUrl(url)

        # Licenses:
        licenses = []
        for row in range(self.dialog.licensesTableWidget.rowCount()):
            license = {}
            license["name"] = self.dialog.licensesTableWidget.item(row,0).text()
            license["file"] = self.dialog.licensesTableWidget.item(row,1).text()
            licenses.append(license)
        self.metadata.setLicense(licenses)

        # Maintainers:
        maintainers = []
        authors = []
        for row in range(self.dialog.peopleTableWidget.rowCount()):
            person = {}
            person["name"] = self.dialog.peopleTableWidget.item(row,1).text()
            person["email"] = self.dialog.peopleTableWidget.item(row,2).text()
            if self.dialog.peopleTableWidget.item(row,0).data(Qt.UserRole) == "maintainer":
                maintainers.append(person)
            elif self.dialog.peopleTableWidget.item(row,0).data(Qt.UserRole) == "author":
                authors.append(person)

        # Content:

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

    def _person_selection_changed(self):
        """ Callback: the current selection in the peopleTableWidget changed """
        items = self.dialog.peopleTableWidget.selectedItems()
        if items:
            self.dialog.removePersonToolButton.setDisabled(False)
        else:
            self.dialog.removePersonToolButton.setDisabled(True)

    def _license_selection_changed(self):
        """ Callback: the current selection in the licensesTableWidget changed """
        items = self.dialog.licensesTableWidget.selectedItems()
        if items:
            self.dialog.removeLicenseToolButton.setDisabled(False)
        else:
            self.dialog.removeLicenseToolButton.setDisabled(True)

    def _add_license_clicked(self):
        """ Callback: The Add License button was clicked """
        license_selector = LicenseSelector(self.current_mod)
        short_code, path = license_selector.exec()
        if short_code:
            self._add_license_row(
                self.dialog.licensesTableWidget.rowCount(), short_code, path
            )

    def _remove_license_clicked(self):
        """ Callback: the Remove License button was clicked """
        items = self.dialog.licensesTableWidget.selectedIndexes()
        if items:
            # We only support single-selection, so can just pull the row # from
            # the first entry
            self.dialog.licensesTableWidget.removeRow(items[0].row())

    def _edit_license(self, item):
        """ Callback: a license row was double-clicked """
        row = item.row()
        short_code = self.dialog.licensesTableWidget.item(row, 0).text()
        path = self.dialog.licensesTableWidget.item(row, 1).text()
        license_selector = LicenseSelector(self.current_mod)
        short_code, path = license_selector.exec(short_code, path)
        if short_code:
            self.dialog.licensesTableWidget.removeRow(row)
            self._add_license_row(row, short_code, path)

    def _add_person_clicked(self):
        """ Callback: the Add Person button was clicked """
        dlg = PersonEditor()
        person_type, name, email = dlg.exec()
        if person_type and name:
            self._add_person_row(row, person_type, name, email)

    def _remove_person_clicked(self):
        """ Callback: the Remove Person button was clicked """
        items = self.dialog.peopleTableWidget.selectedIndexes()
        if items:
            # We only support single-selection, so can just pull the row # from
            # the first entry
            self.dialog.peopleTableWidget.removeRow(items[0].row())

    def _edit_person(self, item):
        """ Callback: a row in the peopleTableWidget was double-clicked """
        row = item.row()
        person_type = self.dialog.peopleTableWidget.item(row, 0).data(Qt.UserRole)
        name = self.dialog.peopleTableWidget.item(row, 1).text()
        email = self.dialog.peopleTableWidget.item(row, 2).text()

        dlg = PersonEditor()
        dlg.setup(person_type, name, email)
        person_type, name, email = dlg.exec()

        if person_type and name:
            self.dialog.peopleTableWidget.removeRow(row)
            self._add_person_row(row, person_type, name, email)
            self.dialog.peopleTableWidget.selectRow(row)

    
    def _add_content_clicked(self):
        """ Callback: The Add Content button was clicked """
        dlg = AddContent(self.current_mod, self.metadata)
        singleton = False
        if self.dialog.contentsListWidget.count() == 0:
            singleton = True
        content_type,new_metadata = dlg.exec(singleton=singleton)
        if content_type and new_metadata:
            self.metadata.addContentItem(content_type, new_metadata)
            self._populate_contents_from_metadata(self.metadata)

    def _remove_content_clicked(self):
        """ Callback: the remove content button was clicked """
        
        item = self.dialog.contentsListWidget.currentItem()
        if not item:
            return
        content_type = item.data(ContentTypeRole)
        content_index = item.data(ContentIndexRole)
        if self.metadata.Content[content_type] and content_index < len(self.metadata.Content[content_type]):
            content_name = self.metadata.Content[content_type][content_index].Name
            self.metadata.removeContentItem(content_type,content_name)
            self._populate_contents_from_metadata(self.metadata)

    def _content_selection_changed(self):
        """ Callback: the selected content item changed """
        items = self.dialog.contentsListWidget.selectedItems()
        if items:
            self.dialog.removeContentItemToolButton.setDisabled(False)
        else:
            self.dialog.removeContentItemToolButton.setDisabled(True)

    def _edit_content(self, item):
        """ Callback: a content row was double-clicked """
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


