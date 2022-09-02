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

""" Contains a class for adding a single content item, as well as auxilliary classes for
its dependent dialog boxes. """

import os
from typing import Optional, Tuple, List

import FreeCAD
import FreeCADGui

from Addon import INTERNAL_WORKBENCHES

from PySide2.QtWidgets import QDialog, QLayout, QFileDialog, QTableWidgetItem
from PySide2.QtGui import QIcon
from PySide2.QtCore import Qt

# pylint: disable=too-few-public-methods

translate = FreeCAD.Qt.translate


class AddContent:
    """A dialog for adding a single content item to the package metadata."""

    def __init__(self, path_to_addon: os.PathLike, toplevel_metadata: FreeCAD.Metadata):
        """path_to_addon is the full path to the toplevel directory of this Addon, and
        toplevel_metadata is to overall package.xml Metadata object for this Addon. This
        information is used to assist the use in filling out the dialog by providing
        sensible default values."""
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "developer_mode_add_content.ui")
        )
        # These are in alphabetical order in English, but their actual label may be translated in
        # the GUI. Store their underlying type as user data.
        self.dialog.addonKindComboBox.setItemData(0, "macro")
        self.dialog.addonKindComboBox.setItemData(1, "preferencepack")
        self.dialog.addonKindComboBox.setItemData(2, "workbench")

        self.toplevel_metadata = toplevel_metadata
        self.metadata = None
        self.path_to_addon = path_to_addon.replace("/", os.path.sep)
        if self.path_to_addon[-1] != os.path.sep:
            self.path_to_addon += (
                os.path.sep
            )  # Make sure the path ends with a separator

        self.dialog.iconLabel.hide()  # Until we have an icon to display

        self.dialog.iconBrowseButton.clicked.connect(self._browse_for_icon_clicked)
        self.dialog.subdirectoryBrowseButton.clicked.connect(
            self._browse_for_subdirectory_clicked
        )
        self.dialog.tagsButton.clicked.connect(self._tags_clicked)
        self.dialog.dependenciesButton.clicked.connect(self._dependencies_clicked)
        self.dialog.freecadVersionsButton.clicked.connect(
            self._freecad_versions_clicked
        )

    def exec(
        self,
        content_kind: str = "workbench",
        metadata: FreeCAD.Metadata = None,
        singleton: bool = True,
    ) -> Optional[Tuple[str, FreeCAD.Metadata]]:
        """Execute the dialog as a modal, returning a new Metadata object if the dialog
        is accepted, or None if it is rejected. This metadata object represents a single
        new content item. It's returned as a tuple with the object type as the first component,
        and the metadata object itself as the second."""
        self.metadata = metadata
        self.dialog.singletonCheckBox.setChecked(singleton)
        if singleton:
            # This doesn't happen automatically the first time
            self.dialog.otherMetadataGroupBox.hide()
        index = self.dialog.addonKindComboBox.findData(content_kind)
        if index == -1:
            index = 2  # Workbench
            FreeCAD.Console.PrintWarning(
                translate("AddonsInstaller", "Unrecognized content kind '{}'").format(
                    content_kind
                )
                + "\n"
            )
        self.dialog.addonKindComboBox.setCurrentIndex(index)
        if metadata:
            self._populate_dialog(metadata)

        self.dialog.layout().setSizeConstraint(QLayout.SetFixedSize)
        result = self.dialog.exec()
        if result == QDialog.Accepted:
            return self._generate_metadata()
        return None

    def _populate_dialog(self, metadata: FreeCAD.Metadata) -> None:
        """Fill in the dialog with the details from the passed metadata object"""
        addon_kind = self.dialog.addonKindComboBox.currentData()
        if addon_kind == "workbench":
            self.dialog.workbenchClassnameLineEdit.setText(metadata.Classname)
        elif addon_kind == "macro":
            pass
        elif addon_kind == "preferencepack":
            pass
        else:
            raise RuntimeError("Invalid data found for selection")

        # Now set the rest of it
        if metadata.Icon:
            self._set_icon(metadata.Icon)
        elif self.toplevel_metadata.Icon:
            if metadata.Subdirectory and metadata.Subdirectory != "./":
                self._set_icon("../" + self.toplevel_metadata.Icon)
            else:
                self._set_icon(self.toplevel_metadata.Icon)
        else:
            self.dialog.iconLabel.hide()
            self.dialog.iconLineEdit.setText("")

        if metadata.Subdirectory:
            self.dialog.subdirectoryLineEdit.setText(metadata.Subdirectory)
        else:
            self.dialog.subdirectoryLineEdit.setText("")

    def _set_icon(self, icon_relative_path):
        """Load the icon and display it, and its path, in the dialog."""
        icon_path = os.path.join(
            self.path_to_addon, icon_relative_path.replace("/", os.path.sep)
        )
        if os.path.isfile(icon_path):
            icon_data = QIcon(icon_path)
            if not icon_data.isNull():
                self.dialog.iconLabel.setPixmap(icon_data.pixmap(32, 32))
                self.dialog.iconLabel.show()
        else:
            FreeCAD.Console.PrintError(
                translate("AddonsInstaller", "Unable to locate icon at {}").format(
                    icon_path
                )
                + "\n"
            )
        self.dialog.iconLineEdit.setText(icon_relative_path)

    def _generate_metadata(self) -> Tuple[str, FreeCAD.Metadata]:
        """Create and return a new metadata object based on the contents of the dialog."""
        return ("workbench", FreeCAD.Metadata())

    ###############################################################################################
    #                                         DIALOG SLOTS
    ###############################################################################################

    def _browse_for_icon_clicked(self):
        """Callback: when the "Browse..." button for the icon field is clicked"""
        subdir = self.dialog.subdirectoryLineEdit.text()
        start_dir = os.path.join(self.path_to_addon, subdir)
        new_icon_path, _ = QFileDialog.getOpenFileName(
            parent=self.dialog,
            caption=translate(
                "AddonsInstaller",
                "Select an icon file for this content item",
            ),
            dir=start_dir,
        )

        base_path = self.path_to_addon.replace("/", os.path.sep)
        icon_path = new_icon_path.replace("/", os.path.sep)

        if not icon_path.startswith(base_path):
            FreeCAD.Console.PrintError(
                translate("AddonsInstaller", "{} is not a subdirectory of {}").format(
                    icon_path, base_path
                )
                + "\n"
            )
            return
        self._set_icon(new_icon_path[len(base_path) :])

    def _browse_for_subdirectory_clicked(self):
        """Callback: when the "Browse..." button for the subdirectory field is clicked"""
        subdir = self.dialog.subdirectoryLineEdit.text()
        start_dir = os.path.join(self.path_to_addon, subdir)
        new_subdir_path = QFileDialog.getExistingDirectory(
            parent=self.dialog,
            caption=translate(
                "AddonsInstaller",
                "Select the subdirectory for this content item",
            ),
            dir=start_dir,
        )
        if new_subdir_path[-1] != "/":
            new_subdir_path += "/"

        # Three legal possibilities:
        #   1) This might be the toplevel directory, in which case we want to set
        #      metadata.Subdirectory to "./"
        #   2) This might be a subdirectory with the same name as the content item, in which case
        #      we don't need to set metadata.Subdirectory at all
        #   3) This might be some other directory name, but still contained within the top-level
        #      directory, in which case we want to set metadata.Subdirectory to the relative path

        # First, reject anything that isn't within the appropriate directory structure:
        base_path = self.path_to_addon.replace("/", os.path.sep)
        subdir_path = new_subdir_path.replace("/", os.path.sep)
        if not subdir_path.startswith(base_path):
            FreeCAD.Console.PrintError(
                translate("AddonsInstaller", "{} is not a subdirectory of {}").format(
                    subdir_path, base_path
                )
                + "\n"
            )
            return

        relative_path = subdir_path[len(base_path) :]
        if not relative_path:
            relative_path = "./"
        elif relative_path[-1] == os.path.sep:
            relative_path = relative_path[:-1]
        self.dialog.subdirectoryLineEdit.setText(relative_path)

    def _tags_clicked(self):
        """Show the tag editor"""
        tags = []
        if self.metadata:
            tags = self.metadata.Tag
        dlg = EditTags(tags)
        new_tags = dlg.exec()

    def _freecad_versions_clicked(self):
        """Show the FreeCAD version editor"""
        dlg = EditFreeCADVersions()
        dlg.exec()

    def _dependencies_clicked(self):
        """Show the dependencies editor"""
        dlg = EditDependencies()
        dlg.exec()


class EditTags:
    """A dialog to edit tags"""

    def __init__(self, tags: List[str] = None):
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "developer_mode_tags.ui")
        )
        self.original_tags = tags
        if tags:
            self.dialog.lineEdit.setText(", ".join(tags))

    def exec(self):
        """Execute the dialog, returning a list of tags (which may be empty, but still represents
        the expected list of tags to be set, e.g. the user may have removed them all)."""
        result = self.dialog.exec()
        if result == QDialog.Accepted:
            new_tags: List[str] = self.dialog.lineEdit.text().split(",")
            clean_tags: List[str] = []
            for tag in new_tags:
                clean_tags.append(tag.strip())
            return clean_tags
        return self.original_tags


class EditDependencies:
    """A dialog to edit dependency information"""

    def __init__(self):
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "developer_mode_dependencies.ui")
        )
        self.dialog.addDependencyToolButton.setIcon(
            QIcon.fromTheme("add", QIcon(":/icons/list-add.svg"))
        )
        self.dialog.removeDependencyToolButton.setIcon(
            QIcon.fromTheme("remove", QIcon(":/icons/list-remove.svg"))
        )
        self.dialog.addDependencyToolButton.clicked.connect(
            self._add_dependency_clicked
        )
        self.dialog.removeDependencyToolButton.clicked.connect(
            self._remove_dependency_clicked
        )
        self.dialog.tableWidget.itemDoubleClicked.connect(self._edit_dependency)
        self.dialog.tableWidget.itemSelectionChanged.connect(
            self._current_index_changed
        )

        self.dialog.removeDependencyToolButton.setDisabled(True)

    def exec(self):
        """Execute the dialog"""
        self.dialog.exec()

    def _add_dependency_clicked(self):
        """Callback: The add button was clicked"""
        dlg = EditDependency()
        dep_type, dep_name, dep_optional = dlg.exec()
        row = self.dialog.tableWidget.rowCount()
        self._add_row(row, dep_type, dep_name, dep_optional)

    def _add_row(self, row, dep_type, dep_name, dep_optional):
        """Utility function to add a row to the table."""
        translations = {
            "workbench": translate("AddonsInstaller", "Workbench"),
            "addon": translate("AddonsInstaller", "Addon"),
            "python": translate("AddonsInstaller", "Python"),
        }
        if dep_type and dep_name:
            self.dialog.tableWidget.insertRow(row)
            type_item = QTableWidgetItem(translations[dep_type])
            type_item.setData(Qt.UserRole, dep_type)
            self.dialog.tableWidget.setItem(row, 0, type_item)
            self.dialog.tableWidget.setItem(row, 1, QTableWidgetItem(dep_name))
            if dep_optional:
                self.dialog.tableWidget.setItem(
                    row, 2, QTableWidgetItem(translate("AddonsInstaller", "Yes"))
                )

    def _remove_dependency_clicked(self):
        """Callback: The remove button was clicked"""
        items = self.dialog.tableWidget.selectedItems()
        if items:
            row = items[0].row()
            self.dialog.tableWidget.removeRow(row)

    def _edit_dependency(self, item):
        """Callback: the dependency was double-clicked"""
        row = item.row()
        dlg = EditDependency()
        dep_type = self.dialog.tableWidget.item(row, 0).data(Qt.UserRole)
        dep_name = self.dialog.tableWidget.item(row, 1).text()
        dep_optional = bool(self.dialog.tableWidget.item(row, 2))
        dep_type, dep_name, dep_optional = dlg.exec(dep_type, dep_name, dep_optional)
        if dep_type and dep_name:
            self.dialog.tableWidget.removeRow(row)
            self._add_row(row, dep_type, dep_name, dep_optional)

    def _current_index_changed(self):
        if self.dialog.tableWidget.selectedItems():
            self.dialog.removeDependencyToolButton.setDisabled(False)
        else:
            self.dialog.removeDependencyToolButton.setDisabled(True)


class EditDependency:
    """A dialog to edit a single piece of dependency information"""

    def __init__(self):
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "developer_mode_edit_dependency.ui")
        )

        self.dialog.typeComboBox.addItem(
            translate("AddonsInstaller", "Internal Workbench"), "workbench"
        )
        self.dialog.typeComboBox.addItem(
            translate("AddonsInstaller", "External Addon"), "addon"
        )
        self.dialog.typeComboBox.addItem(
            translate("AddonsInstaller", "Python Package"), "python"
        )

        self.dialog.typeComboBox.currentIndexChanged.connect(
            self._type_selection_changed
        )
        self.dialog.dependencyComboBox.currentIndexChanged.connect(
            self._dependency_selection_changed
        )

        self.dialog.typeComboBox.setCurrentIndex(
            2
        )  # Expect mostly Python dependencies...

    def exec(
        self, dep_type="", dep_name="", dep_optional=False
    ) -> Tuple[str, str, bool]:
        """Execute the dialog, returning a tuple of the type of dependency (workbench, addon, or
        python), the name of the dependency, and a boolean indicating whether this is optional."""

        # If we are editing an existing row, set up the dialog:
        if dep_type and dep_name:
            index = self.dialog.typeComboBox.findData(dep_type)
            if index == -1:
                raise RuntimeError(f"Invaid dependency type {dep_type}")
            self.dialog.typeComboBox.setCurrentIndex(index)
            index = self.dialog.dependencyComboBox.findData(dep_name)
            if index == -1:
                index = self.dialog.dependencyComboBox.findData("other")
            self.dialog.dependencyComboBox.setCurrentIndex(index)
            self.dialog.lineEdit.setText(dep_name)
            self.dialog.optionalCheckBox.setChecked(dep_optional)

        # Run the dialog (modal)
        result = self.dialog.exec()
        if result == QDialog.Accepted:
            dep_type = self.dialog.typeComboBox.currentData()
            dep_optional = self.dialog.optionalCheckBox.isChecked()
            dep_name = self.dialog.dependencyComboBox.currentData()
            if dep_name == "other":
                dep_name = self.dialog.lineEdit.text()
            return (dep_type, dep_name, dep_optional)
        return ("", "", False)

    def _populate_internal_workbenches(self):
        """Add all known internal FreeCAD Workbenches to the list"""
        self.dialog.dependencyComboBox.clear()
        for display_name, name in INTERNAL_WORKBENCHES.items():
            self.dialog.dependencyComboBox.addItem(display_name, name)
        # No "other" option is supported for this type of dependency

    def _populate_external_addons(self):
        """Add all known addons to the list"""
        self.dialog.dependencyComboBox.clear()
        #pylint: disable=import-outside-toplevel
        from AddonManager import INSTANCE as AM_INSTANCE

        repo_dict = {}
        # We need a case-insensitive sorting of all repo types, displayed and sorted by their
        # display name, but keeping track of their official name as well (stored in the UserRole)
        for repo in AM_INSTANCE.item_model.repos:
            repo_dict[repo.display_name.lower()] = (repo.display_name, repo.name)
        sorted_keys = sorted(repo_dict.keys())
        for item in sorted_keys:
            self.dialog.dependencyComboBox.addItem(
                repo_dict[item][0], repo_dict[item][1]
            )
        self.dialog.dependencyComboBox.addItem(
            translate("AddonsInstaller", "Other..."), "other"
        )

    def _populate_allowed_python_packages(self):
        """Add all allowed python packages to the list"""
        self.dialog.dependencyComboBox.clear()
        #pylint: disable=import-outside-toplevel
        from AddonManager import INSTANCE as AM_INSTANCE

        packages = sorted(AM_INSTANCE.allowed_packages)
        for package in packages:
            self.dialog.dependencyComboBox.addItem(package, package)
        self.dialog.dependencyComboBox.addItem(
            translate("AddonsInstaller", "Other..."), "other"
        )

    def _type_selection_changed(self, _):
        """Callback: The type of dependency has been changed"""
        selection = self.dialog.typeComboBox.currentData()
        if selection == "workbench":
            self._populate_internal_workbenches()
        elif selection == "addon":
            self._populate_external_addons()
        elif selection == "python":
            self._populate_allowed_python_packages()
        else:
            raise RuntimeError("Invalid data found for selection")

    def _dependency_selection_changed(self, _):
        selection = self.dialog.dependencyComboBox.currentData()
        if selection == "other":
            self.dialog.lineEdit.show()
        else:
            self.dialog.lineEdit.hide()


class EditFreeCADVersions:
    """A dialog to edit minimum and maximum FreeCAD version support"""

    def __init__(self):
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(
                os.path.dirname(__file__), "developer_mode_freecad_versions.ui"
            )
        )

    def exec(self):
        """Execute the dialog"""
        self.dialog.exec()


class EditAdvancedVersions:
    """A dialog to support mapping specific git branches, tags, or commits to specific
    versions of FreeCAD."""

    def __init__(self):
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(
                os.path.dirname(__file__), "developer_mode_advanced_freecad_versions.ui"
            )
        )

    def exec(self):
        """Execute the dialog"""
        self.dialog.exec()
