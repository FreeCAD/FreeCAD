# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2023 FreeCAD Project Association                   *
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

""" Defines the PackageList QWidget for displaying a list of Addons. """

from enum import IntEnum
import threading

import FreeCAD

from PySide import QtCore, QtGui, QtWidgets

from Addon import Addon

from compact_view import Ui_CompactView
from expanded_view import Ui_ExpandedView

import addonmanager_utilities as utils
from addonmanager_metadata import get_first_supported_freecad_version, Version

translate = FreeCAD.Qt.translate


# pylint: disable=too-few-public-methods


class ListDisplayStyle(IntEnum):
    """The display mode of the list"""

    COMPACT = 0
    EXPANDED = 1


class StatusFilter(IntEnum):
    """Predefined filers"""

    ANY = 0
    INSTALLED = 1
    NOT_INSTALLED = 2
    UPDATE_AVAILABLE = 3


class PackageList(QtWidgets.QWidget):
    """A widget that shows a list of packages and various widgets to control the
    display of the list"""

    itemSelected = QtCore.Signal(Addon)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.ui = Ui_PackageList()
        self.ui.setupUi(self)

        self.item_filter = PackageListFilter()
        self.ui.listPackages.setModel(self.item_filter)
        self.item_delegate = PackageListItemDelegate(self.ui.listPackages)
        self.ui.listPackages.setItemDelegate(self.item_delegate)

        self.ui.listPackages.clicked.connect(self.on_listPackages_clicked)
        self.ui.comboPackageType.currentIndexChanged.connect(self.update_type_filter)
        self.ui.comboStatus.currentIndexChanged.connect(self.update_status_filter)
        self.ui.lineEditFilter.textChanged.connect(self.update_text_filter)
        self.ui.buttonCompactLayout.clicked.connect(
            lambda: self.set_view_style(ListDisplayStyle.COMPACT)
        )
        self.ui.buttonExpandedLayout.clicked.connect(
            lambda: self.set_view_style(ListDisplayStyle.EXPANDED)
        )

        # Only shows when the user types in a filter
        self.ui.labelFilterValidity.hide()

        # Set up the view the same as the last time:
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        package_type = pref.GetInt("PackageTypeSelection", 1)
        self.ui.comboPackageType.setCurrentIndex(package_type)
        status = pref.GetInt("StatusSelection", 0)
        self.ui.comboStatus.setCurrentIndex(status)

        # Pre-init of other members:
        self.item_model = None

    def setModel(self, model):
        """This is a model-view-controller widget: set its model."""
        self.item_model = model
        self.item_filter.setSourceModel(self.item_model)
        self.item_filter.sort(0)

        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        style = pref.GetInt("ViewStyle", ListDisplayStyle.EXPANDED)
        self.set_view_style(style)
        if style == ListDisplayStyle.EXPANDED:
            self.ui.buttonExpandedLayout.setChecked(True)
        else:
            self.ui.buttonCompactLayout.setChecked(True)

        self.item_filter.setHidePy2(pref.GetBool("HidePy2", True))
        self.item_filter.setHideObsolete(pref.GetBool("HideObsolete", True))
        self.item_filter.setHideNewerFreeCADRequired(pref.GetBool("HideNewerFreeCADRequired", True))

    def on_listPackages_clicked(self, index: QtCore.QModelIndex):
        """Determine what addon was selected and emit the itemSelected signal with it as
        an argument."""
        source_selection = self.item_filter.mapToSource(index)
        selected_repo = self.item_model.repos[source_selection.row()]
        self.itemSelected.emit(selected_repo)

    def update_type_filter(self, type_filter: int) -> None:
        """hide/show rows corresponding to the type filter

        type_filter is an integer: 0 for all, 1 for workbenches, 2 for macros,
        and 3 for preference packs

        """

        self.item_filter.setPackageFilter(type_filter)
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("PackageTypeSelection", type_filter)

    def update_status_filter(self, status_filter: int) -> None:
        """hide/show rows corresponding to the status filter

        status_filter is an integer: 0 for any, 1 for installed, 2 for not installed,
        and 3 for update available

        """

        self.item_filter.setStatusFilter(status_filter)
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("StatusSelection", status_filter)

    def update_text_filter(self, text_filter: str) -> None:
        """filter name and description by the regex specified by text_filter"""

        if text_filter:
            if hasattr(self.item_filter, "setFilterRegularExpression"):  # Added in Qt 5.12
                test_regex = QtCore.QRegularExpression(text_filter)
            else:
                test_regex = QtCore.QRegExp(text_filter)
            if test_regex.isValid():
                self.ui.labelFilterValidity.setToolTip(
                    translate("AddonsInstaller", "Filter is valid")
                )
                icon = QtGui.QIcon.fromTheme("ok", QtGui.QIcon(":/icons/edit_OK.svg"))
                self.ui.labelFilterValidity.setPixmap(icon.pixmap(16, 16))
            else:
                self.ui.labelFilterValidity.setToolTip(
                    translate("AddonsInstaller", "Filter regular expression is invalid")
                )
                icon = QtGui.QIcon.fromTheme("cancel", QtGui.QIcon(":/icons/edit_Cancel.svg"))
                self.ui.labelFilterValidity.setPixmap(icon.pixmap(16, 16))
            self.ui.labelFilterValidity.show()
        else:
            self.ui.labelFilterValidity.hide()
        if hasattr(self.item_filter, "setFilterRegularExpression"):  # Added in Qt 5.12
            self.item_filter.setFilterRegularExpression(text_filter)
        else:
            self.item_filter.setFilterRegExp(text_filter)

    def set_view_style(self, style: ListDisplayStyle) -> None:
        """Set the style (compact or expanded) of the list"""
        self.item_model.layoutAboutToBeChanged.emit()
        self.item_delegate.set_view(style)
        if style == ListDisplayStyle.COMPACT:
            self.ui.listPackages.setSpacing(2)
        else:
            self.ui.listPackages.setSpacing(5)
        self.item_model.layoutChanged.emit()

        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("ViewStyle", style)


class PackageListItemModel(QtCore.QAbstractListModel):
    """The model for use with the PackageList class."""

    repos = []
    write_lock = threading.Lock()

    DataAccessRole = QtCore.Qt.UserRole
    StatusUpdateRole = QtCore.Qt.UserRole + 1
    IconUpdateRole = QtCore.Qt.UserRole + 2

    def rowCount(self, parent: QtCore.QModelIndex = QtCore.QModelIndex()) -> int:
        """The number of rows"""
        if parent.isValid():
            return 0
        return len(self.repos)

    def columnCount(self, parent: QtCore.QModelIndex = QtCore.QModelIndex()) -> int:
        """Only one column, always returns 1."""
        if parent.isValid():
            return 0
        return 1

    def data(self, index: QtCore.QModelIndex, role: int = QtCore.Qt.DisplayRole):
        """Get the data for a given index and role."""
        if not index.isValid():
            return None
        row = index.row()
        if role == QtCore.Qt.ToolTipRole:
            tooltip = ""
            if self.repos[row].repo_type == Addon.Kind.PACKAGE:
                tooltip = translate("AddonsInstaller", "Click for details about package {}").format(
                    self.repos[row].display_name
                )
            elif self.repos[row].repo_type == Addon.Kind.WORKBENCH:
                tooltip = translate(
                    "AddonsInstaller", "Click for details about workbench {}"
                ).format(self.repos[row].display_name)
            elif self.repos[row].repo_type == Addon.Kind.MACRO:
                tooltip = translate("AddonsInstaller", "Click for details about macro {}").format(
                    self.repos[row].display_name
                )
            return tooltip
        if role == PackageListItemModel.DataAccessRole:
            return self.repos[row]

    def headerData(self, _unused1, _unused2, _role=QtCore.Qt.DisplayRole):
        """No header in this implementation: always returns None."""
        return None

    def setData(self, index: QtCore.QModelIndex, value, role=QtCore.Qt.EditRole) -> None:
        """Set the data for this row. The column of the index is ignored."""

        row = index.row()
        with self.write_lock:
            if role == PackageListItemModel.StatusUpdateRole:
                self.repos[row].set_status(value)
                self.dataChanged.emit(
                    self.index(row, 2),
                    self.index(row, 2),
                    [PackageListItemModel.StatusUpdateRole],
                )
            elif role == PackageListItemModel.IconUpdateRole:
                self.repos[row].icon = value
                self.dataChanged.emit(
                    self.index(row, 0),
                    self.index(row, 0),
                    [PackageListItemModel.IconUpdateRole],
                )

    def append_item(self, repo: Addon) -> None:
        """Adds this addon to the end of the model. Thread safe."""
        if repo in self.repos:
            # Cowardly refuse to insert the same repo a second time
            return
        with self.write_lock:
            self.beginInsertRows(QtCore.QModelIndex(), self.rowCount(), self.rowCount())
            self.repos.append(repo)
            self.endInsertRows()

    def clear(self) -> None:
        """Clear the model, removing all rows. Thread safe."""
        if self.rowCount() > 0:
            with self.write_lock:
                self.beginRemoveRows(QtCore.QModelIndex(), 0, self.rowCount() - 1)
                self.repos = []
                self.endRemoveRows()

    def update_item_status(self, name: str, status: Addon.Status) -> None:
        """Set the status of addon with name to status."""
        for row, item in enumerate(self.repos):
            if item.name == name:
                self.setData(self.index(row, 0), status, PackageListItemModel.StatusUpdateRole)
                return

    def update_item_icon(self, name: str, icon: QtGui.QIcon) -> None:
        """Set the icon for Addon with name to icon"""
        for row, item in enumerate(self.repos):
            if item.name == name:
                self.setData(self.index(row, 0), icon, PackageListItemModel.IconUpdateRole)
                return

    def reload_item(self, repo: Addon) -> None:
        """Sets the addon data for the given addon (based on its name)"""
        for index, item in enumerate(self.repos):
            if item.name == repo.name:
                with self.write_lock:
                    self.repos[index] = repo
                return


class CompactView(QtWidgets.QWidget):
    """A single-line view of the package information"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.ui = Ui_CompactView()
        self.ui.setupUi(self)


class ExpandedView(QtWidgets.QWidget):
    """A multi-line view of the package information"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.ui = Ui_ExpandedView()
        self.ui.setupUi(self)


class PackageListItemDelegate(QtWidgets.QStyledItemDelegate):
    """Render the repo data as a formatted region"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.displayStyle = ListDisplayStyle.EXPANDED
        self.expanded = ExpandedView()
        self.compact = CompactView()
        self.widget = self.expanded

    def set_view(self, style: ListDisplayStyle) -> None:
        """Set the view of to style"""
        if not self.displayStyle == style:
            self.displayStyle = style

    def sizeHint(self, _option, index):
        """Attempt to figure out the correct height for the widget based on its
        current contents."""
        self.update_content(index)
        return self.widget.sizeHint()

    def update_content(self, index):
        """Creates the display of the content for a given index."""
        repo = index.data(PackageListItemModel.DataAccessRole)
        if self.displayStyle == ListDisplayStyle.EXPANDED:
            self.widget = self.expanded
            self.widget.ui.labelPackageName.setText(f"<h1>{repo.display_name}</h1>")
            self.widget.ui.labelIcon.setPixmap(repo.icon.pixmap(QtCore.QSize(48, 48)))
        else:
            self.widget = self.compact
            self.widget.ui.labelPackageName.setText(f"<b>{repo.display_name}</b>")
            self.widget.ui.labelIcon.setPixmap(repo.icon.pixmap(QtCore.QSize(16, 16)))

        self.widget.ui.labelIcon.setText("")
        if self.displayStyle == ListDisplayStyle.EXPANDED:
            self.widget.ui.labelTags.setText("")
        if repo.metadata:
            self.widget.ui.labelDescription.setText(repo.metadata.description)
            self.widget.ui.labelVersion.setText(f"<i>v{repo.metadata.version}</i>")
            if self.displayStyle == ListDisplayStyle.EXPANDED:
                self._setup_expanded_package(repo)
        elif repo.macro and repo.macro.parsed:
            self._setup_macro(repo)
        else:
            self.widget.ui.labelDescription.setText("")
            self.widget.ui.labelVersion.setText("")
            if self.displayStyle == ListDisplayStyle.EXPANDED:
                self.widget.ui.labelMaintainer.setText("")

        # Update status
        if self.displayStyle == ListDisplayStyle.EXPANDED:
            self.widget.ui.labelStatus.setText(self.get_expanded_update_string(repo))
        else:
            self.widget.ui.labelStatus.setText(self.get_compact_update_string(repo))

        self.widget.adjustSize()

    def _setup_expanded_package(self, repo: Addon):
        """Set up the display for a package in expanded view"""
        maintainers = repo.metadata.maintainer
        maintainers_string = ""
        if len(maintainers) == 1:
            maintainers_string = (
                translate("AddonsInstaller", "Maintainer")
                + f": {maintainers[0].name} <{maintainers[0].email}>"
            )
        elif len(maintainers) > 1:
            n = len(maintainers)
            maintainers_string = translate("AddonsInstaller", "Maintainers:", "", n)
            for maintainer in maintainers:
                maintainers_string += f"\n{maintainer.name} <{maintainer.email}>"
        self.widget.ui.labelMaintainer.setText(maintainers_string)
        if repo.tags:
            self.widget.ui.labelTags.setText(
                translate("AddonsInstaller", "Tags") + ": " + ", ".join(repo.tags)
            )

    def _setup_macro(self, repo: Addon):
        """Set up the display for a macro"""
        self.widget.ui.labelDescription.setText(repo.macro.comment)
        version_string = ""
        if repo.macro.version:
            version_string = repo.macro.version + " "
        if repo.macro.on_wiki:
            version_string += "(wiki)"
        elif repo.macro.on_git:
            version_string += "(git)"
        else:
            version_string += "(unknown source)"
        if repo.macro.date:
            version_string = (
                version_string
                + ", "
                + translate("AddonsInstaller", "updated")
                + " "
                + repo.macro.date
            )
        self.widget.ui.labelVersion.setText("<i>" + version_string + "</i>")
        if self.displayStyle == ListDisplayStyle.EXPANDED:
            if repo.macro.author:
                caption = translate("AddonsInstaller", "Author")
                self.widget.ui.labelMaintainer.setText(caption + ": " + repo.macro.author)
            else:
                self.widget.ui.labelMaintainer.setText("")

    @staticmethod
    def get_compact_update_string(repo: Addon) -> str:
        """Get a single-line string listing details about the installed version and
        date"""

        result = ""
        if repo.status() == Addon.Status.UNCHECKED:
            result = translate("AddonsInstaller", "Installed")
        elif repo.status() == Addon.Status.NO_UPDATE_AVAILABLE:
            result = translate("AddonsInstaller", "Up-to-date")
        elif repo.status() == Addon.Status.UPDATE_AVAILABLE:
            result = translate("AddonsInstaller", "Update available")
        elif repo.status() == Addon.Status.PENDING_RESTART:
            result = translate("AddonsInstaller", "Pending restart")

        if repo.is_disabled():
            style = "style='color:" + utils.warning_color_string() + "; font-weight:bold;'"
            result += f"<span {style}> [" + translate("AddonsInstaller", "DISABLED") + "]</span>"

        return result

    @staticmethod
    def get_expanded_update_string(repo: Addon) -> str:
        """Get a multi-line string listing details about the installed version and
        date"""

        result = ""

        installed_version_string = ""
        if repo.status() != Addon.Status.NOT_INSTALLED:
            if repo.installed_version:
                installed_version_string = (
                    "<br/>" + translate("AddonsInstaller", "Installed version") + ": "
                )
                installed_version_string += str(repo.installed_version)
            else:
                installed_version_string = "<br/>" + translate("AddonsInstaller", "Unknown version")

        installed_date_string = ""
        if repo.updated_timestamp:
            installed_date_string = "<br/>" + translate("AddonsInstaller", "Installed on") + ": "
            installed_date_string += (
                QtCore.QDateTime.fromTime_t(repo.updated_timestamp)
                .date()
                .toString(QtCore.Qt.SystemLocaleShortDate)
            )

        available_version_string = ""
        if repo.metadata:
            available_version_string = (
                "<br/>" + translate("AddonsInstaller", "Available version") + ": "
            )
            available_version_string += str(repo.metadata.version)

        if repo.status() == Addon.Status.UNCHECKED:
            result = translate("AddonsInstaller", "Installed")
            result += installed_version_string
            result += installed_date_string
        elif repo.status() == Addon.Status.NO_UPDATE_AVAILABLE:
            result = translate("AddonsInstaller", "Up-to-date")
            result += installed_version_string
            result += installed_date_string
        elif repo.status() == Addon.Status.UPDATE_AVAILABLE:
            result = translate("AddonsInstaller", "Update available")
            result += installed_version_string
            result += installed_date_string
            result += available_version_string
        elif repo.status() == Addon.Status.PENDING_RESTART:
            result = translate("AddonsInstaller", "Pending restart")

        if repo.is_disabled():
            style = "style='color:" + utils.warning_color_string() + "; font-weight:bold;'"
            result += (
                f"<br/><span {style}>[" + translate("AddonsInstaller", "DISABLED") + "]</span>"
            )

        return result

    def paint(
        self,
        painter: QtGui.QPainter,
        option: QtWidgets.QStyleOptionViewItem,
        _: QtCore.QModelIndex,
    ):
        """Main paint function: renders this widget into a given rectangle,
        successively drawing all of its children."""
        painter.save()
        self.widget.resize(option.rect.size())
        painter.translate(option.rect.topLeft())
        self.widget.render(
            painter, QtCore.QPoint(), QtGui.QRegion(), QtWidgets.QWidget.DrawChildren
        )
        painter.restore()


class PackageListFilter(QtCore.QSortFilterProxyModel):
    """Handle filtering the item list on various criteria"""

    def __init__(self):
        super().__init__()
        self.package_type = 0  # Default to showing everything
        self.status = 0  # Default to showing any
        self.setSortCaseSensitivity(QtCore.Qt.CaseInsensitive)
        self.hide_obsolete = False
        self.hide_py2 = False
        self.hide_newer_freecad_required = False

    def setPackageFilter(
        self, package_type: int
    ) -> None:  # 0=All, 1=Workbenches, 2=Macros, 3=Preference Packs
        """Set the package filter to package_type and refreshes."""
        self.package_type = package_type
        self.invalidateFilter()

    def setStatusFilter(
        self, status: int
    ) -> None:  # 0=Any, 1=Installed, 2=Not installed, 3=Update available
        """Sets the status filter to status and refreshes."""
        self.status = status
        self.invalidateFilter()

    def setHidePy2(self, hide_py2: bool) -> None:
        """Sets whether to hide Python 2-only Addons"""
        self.hide_py2 = hide_py2
        self.invalidateFilter()

    def setHideObsolete(self, hide_obsolete: bool) -> None:
        """Sets whether to hide Addons marked obsolete"""
        self.hide_obsolete = hide_obsolete
        self.invalidateFilter()

    def setHideNewerFreeCADRequired(self, hide_nfr: bool) -> None:
        """Sets whether to hide packages that have indicated they need a newer version
        of FreeCAD than the one currently running."""
        self.hide_newer_freecad_required = hide_nfr
        self.invalidateFilter()

    def lessThan(self, left_in, right_in) -> bool:
        """Enable sorting of display name (not case-sensitive)."""

        left = self.sourceModel().data(left_in, PackageListItemModel.DataAccessRole)
        right = self.sourceModel().data(right_in, PackageListItemModel.DataAccessRole)

        return left.display_name.lower() < right.display_name.lower()

    def filterAcceptsRow(self, row, _parent=QtCore.QModelIndex()):
        """Do the actual filtering (called automatically by Qt when drawing the list)"""

        index = self.sourceModel().createIndex(row, 0)
        data = self.sourceModel().data(index, PackageListItemModel.DataAccessRole)
        if self.package_type == 1:
            if not data.contains_workbench():
                return False
        elif self.package_type == 2:
            if not data.contains_macro():
                return False
        elif self.package_type == 3:
            if not data.contains_preference_pack():
                return False

        if self.status == StatusFilter.INSTALLED:
            if data.status() == Addon.Status.NOT_INSTALLED:
                return False
        elif self.status == StatusFilter.NOT_INSTALLED:
            if data.status() != Addon.Status.NOT_INSTALLED:
                return False
        elif self.status == StatusFilter.UPDATE_AVAILABLE:
            if data.status() != Addon.Status.UPDATE_AVAILABLE:
                return False

        # If it's not installed, check to see if it's Py2 only
        if data.status() == Addon.Status.NOT_INSTALLED and self.hide_py2 and data.python2:
            return False

        # If it's not installed, check to see if it's marked obsolete
        if data.status() == Addon.Status.NOT_INSTALLED and self.hide_obsolete and data.obsolete:
            return False

        # If it's not installed, check to see if it's for a newer version of FreeCAD
        if (
            data.status() == Addon.Status.NOT_INSTALLED
            and self.hide_newer_freecad_required
            and data.metadata
        ):
            # Only hide if ALL content items require a newer version, otherwise
            # it's possible that this package actually provides versions of itself
            # for newer and older versions

            first_supported_version = get_first_supported_freecad_version(data.metadata)
            if first_supported_version is not None:
                current_fc_version = Version(from_list=FreeCAD.Version())
                if first_supported_version > current_fc_version:
                    return False

        name = data.display_name
        desc = data.description
        if hasattr(self, "filterRegularExpression"):  # Added in Qt 5.12
            re = self.filterRegularExpression()
            if re.isValid():
                re.setPatternOptions(QtCore.QRegularExpression.CaseInsensitiveOption)
                if re.match(name).hasMatch():
                    return True
                if re.match(desc).hasMatch():
                    return True
                if data.macro and data.macro.comment and re.match(data.macro.comment).hasMatch():
                    return True
                for tag in data.tags:
                    if re.match(tag).hasMatch():
                        return True
            return False
        # Only get here for Qt < 5.12
        re = self.filterRegExp()
        if re.isValid():
            re.setCaseSensitivity(QtCore.Qt.CaseInsensitive)
            if re.indexIn(name) != -1:
                return True
            if re.indexIn(desc) != -1:
                return True
            if data.macro and data.macro.comment and re.indexIn(data.macro.comment) != -1:
                return True
            for tag in data.tags:
                if re.indexIn(tag) != -1:
                    return True
        return False


# pylint: disable=attribute-defined-outside-init, missing-function-docstring


class Ui_PackageList:
    """The contents of the PackageList widget"""

    def setupUi(self, form):
        if not form.objectName():
            form.setObjectName("PackageList")
        self.verticalLayout = QtWidgets.QVBoxLayout(form)
        self.verticalLayout.setObjectName("verticalLayout")
        self.horizontalLayout_6 = QtWidgets.QHBoxLayout()
        self.horizontalLayout_6.setObjectName("horizontalLayout_6")
        self.buttonCompactLayout = QtWidgets.QToolButton(form)
        self.buttonCompactLayout.setObjectName("buttonCompactLayout")
        self.buttonCompactLayout.setCheckable(True)
        self.buttonCompactLayout.setAutoExclusive(True)
        self.buttonCompactLayout.setIcon(
            QtGui.QIcon.fromTheme("expanded_view", QtGui.QIcon(":/icons/compact_view.svg"))
        )

        self.horizontalLayout_6.addWidget(self.buttonCompactLayout)

        self.buttonExpandedLayout = QtWidgets.QToolButton(form)
        self.buttonExpandedLayout.setObjectName("buttonExpandedLayout")
        self.buttonExpandedLayout.setCheckable(True)
        self.buttonExpandedLayout.setChecked(True)
        self.buttonExpandedLayout.setAutoExclusive(True)
        self.buttonExpandedLayout.setIcon(
            QtGui.QIcon.fromTheme("expanded_view", QtGui.QIcon(":/icons/expanded_view.svg"))
        )

        self.horizontalLayout_6.addWidget(self.buttonExpandedLayout)

        self.labelPackagesContaining = QtWidgets.QLabel(form)
        self.labelPackagesContaining.setObjectName("labelPackagesContaining")

        self.horizontalLayout_6.addWidget(self.labelPackagesContaining)

        self.comboPackageType = QtWidgets.QComboBox(form)
        self.comboPackageType.addItem("")
        self.comboPackageType.addItem("")
        self.comboPackageType.addItem("")
        self.comboPackageType.addItem("")
        self.comboPackageType.setObjectName("comboPackageType")

        self.horizontalLayout_6.addWidget(self.comboPackageType)

        self.labelStatus = QtWidgets.QLabel(form)
        self.labelStatus.setObjectName("labelStatus")

        self.horizontalLayout_6.addWidget(self.labelStatus)

        self.comboStatus = QtWidgets.QComboBox(form)
        self.comboStatus.addItem("")
        self.comboStatus.addItem("")
        self.comboStatus.addItem("")
        self.comboStatus.addItem("")
        self.comboStatus.setObjectName("comboStatus")

        self.horizontalLayout_6.addWidget(self.comboStatus)

        self.lineEditFilter = QtWidgets.QLineEdit(form)
        self.lineEditFilter.setObjectName("lineEditFilter")
        self.lineEditFilter.setClearButtonEnabled(True)

        self.horizontalLayout_6.addWidget(self.lineEditFilter)

        self.labelFilterValidity = QtWidgets.QLabel(form)
        self.labelFilterValidity.setObjectName("labelFilterValidity")

        self.horizontalLayout_6.addWidget(self.labelFilterValidity)

        self.verticalLayout.addLayout(self.horizontalLayout_6)

        self.listPackages = QtWidgets.QListView(form)
        self.listPackages.setObjectName("listPackages")
        self.listPackages.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        self.listPackages.setProperty("showDropIndicator", False)
        self.listPackages.setSelectionMode(QtWidgets.QAbstractItemView.NoSelection)
        self.listPackages.setResizeMode(QtWidgets.QListView.Adjust)
        self.listPackages.setUniformItemSizes(False)
        self.listPackages.setAlternatingRowColors(True)
        self.listPackages.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)

        self.verticalLayout.addWidget(self.listPackages)

        self.retranslateUi(form)

        QtCore.QMetaObject.connectSlotsByName(form)

    def retranslateUi(self, _):
        self.labelPackagesContaining.setText(
            QtCore.QCoreApplication.translate("AddonsInstaller", "Show Addons containing:", None)
        )
        self.comboPackageType.setItemText(
            0, QtCore.QCoreApplication.translate("AddonsInstaller", "All", None)
        )
        self.comboPackageType.setItemText(
            1, QtCore.QCoreApplication.translate("AddonsInstaller", "Workbenches", None)
        )
        self.comboPackageType.setItemText(
            2, QtCore.QCoreApplication.translate("AddonsInstaller", "Macros", None)
        )
        self.comboPackageType.setItemText(
            3,
            QtCore.QCoreApplication.translate("AddonsInstaller", "Preference Packs", None),
        )
        self.labelStatus.setText(
            QtCore.QCoreApplication.translate("AddonsInstaller", "Status:", None)
        )
        self.comboStatus.setItemText(
            StatusFilter.ANY,
            QtCore.QCoreApplication.translate("AddonsInstaller", "Any", None),
        )
        self.comboStatus.setItemText(
            StatusFilter.INSTALLED,
            QtCore.QCoreApplication.translate("AddonsInstaller", "Installed", None),
        )
        self.comboStatus.setItemText(
            StatusFilter.NOT_INSTALLED,
            QtCore.QCoreApplication.translate("AddonsInstaller", "Not installed", None),
        )
        self.comboStatus.setItemText(
            StatusFilter.UPDATE_AVAILABLE,
            QtCore.QCoreApplication.translate("AddonsInstaller", "Update available", None),
        )
        self.lineEditFilter.setPlaceholderText(
            QtCore.QCoreApplication.translate("AddonsInstaller", "Filter", None)
        )
        self.labelFilterValidity.setText(
            QtCore.QCoreApplication.translate("AddonsInstaller", "OK", None)
        )
