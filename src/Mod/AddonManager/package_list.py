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
import datetime
import threading

import FreeCAD

from PySide import QtCore, QtGui, QtWidgets

from Addon import Addon

from compact_view import Ui_CompactView
from expanded_view import Ui_ExpandedView

import addonmanager_utilities as utils
from addonmanager_metadata import get_first_supported_freecad_version, Version
from Widgets.addonmanager_widget_view_control_bar import WidgetViewControlBar, SortOptions
from Widgets.addonmanager_widget_view_selector import AddonManagerDisplayStyle
from Widgets.addonmanager_widget_filter_selector import StatusFilter, Filter
from Widgets.addonmanager_widget_progress_bar import WidgetProgressBar
from addonmanager_licenses import get_license_manager

translate = FreeCAD.Qt.translate


# pylint: disable=too-few-public-methods


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
        self.ui.view_bar.filter_changed.connect(self.update_status_filter)
        self.ui.view_bar.search_changed.connect(self.item_filter.setFilterRegularExpression)
        self.ui.view_bar.sort_changed.connect(self.item_filter.setSortRole)
        self.ui.view_bar.sort_changed.connect(self.item_delegate.set_sort)
        self.ui.view_bar.sort_order_changed.connect(lambda order: self.item_filter.sort(0, order))

        # Set up the view the same as the last time:
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        package_type = pref.GetInt("PackageTypeSelection", 0)
        status = pref.GetInt("StatusSelection", 0)
        search_string = pref.GetString("SearchString", "")
        self.ui.view_bar.filter_selector.set_contents_filter(package_type)
        self.ui.view_bar.filter_selector.set_status_filter(status)
        if search_string:
            self.ui.view_bar.search.filter_line_edit.setText(search_string)
        self.item_filter.setPackageFilter(package_type)
        self.item_filter.setStatusFilter(status)

        # Pre-init of other members:
        self.item_model = None

    def setModel(self, model):
        """This is a model-view-controller widget: set its model."""
        self.item_model = model
        self.item_filter.setSourceModel(self.item_model)
        self.item_filter.setSortRole(SortOptions.Alphabetical)
        self.item_filter.sort(0, QtCore.Qt.AscendingOrder)

        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        style = pref.GetInt("ViewStyle", AddonManagerDisplayStyle.EXPANDED)
        self.set_view_style(style)
        self.ui.view_bar.view_selector.set_current_view(style)

        self.item_filter.setHidePy2(pref.GetBool("HidePy2", False))
        self.item_filter.setHideObsolete(pref.GetBool("HideObsolete", False))
        self.item_filter.setHideNonOSIApproved(pref.GetBool("HideNonOSIApproved", False))
        self.item_filter.setHideNonFSFLibre(pref.GetBool("HideNonFSFFreeLibre", False))
        self.item_filter.setHideNewerFreeCADRequired(
            pref.GetBool("HideNewerFreeCADRequired", False)
        )
        self.item_filter.setHideUnlicensed(pref.GetBool("HideUnlicensed", False))

    def select_addon(self, addon_name: str):
        for index, addon in enumerate(self.item_model.repos):
            if addon.name == addon_name:
                row_index = self.item_model.createIndex(index, 0)
                if self.item_filter.filterAcceptsRow(index):
                    self.ui.listPackages.setCurrentIndex(row_index)
                else:
                    FreeCAD.Console.PrintLog(
                        f"Addon {addon_name} is not visible given current "
                        "filter: not selecting it."
                    )
                return
        FreeCAD.Console.PrintLog(f"Could not find addon '{addon_name}' to select it")

    def on_listPackages_clicked(self, index: QtCore.QModelIndex):
        """Determine what addon was selected and emit the itemSelected signal with it as
        an argument."""
        source_selection = self.item_filter.mapToSource(index)
        selected_repo = self.item_model.repos[source_selection.row()]
        self.itemSelected.emit(selected_repo)

    def update_status_filter(self, new_filter: Filter) -> None:
        """hide/show rows corresponding to the specified filter"""

        self.item_filter.setStatusFilter(new_filter.status_filter)
        self.item_filter.setPackageFilter(new_filter.content_filter)
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("StatusSelection", new_filter.status_filter)
        pref.SetInt("PackageTypeSelection", new_filter.content_filter)
        self.item_filter.invalidateFilter()

    def set_view_style(self, style: AddonManagerDisplayStyle) -> None:
        """Set the style (compact or expanded) of the list"""
        if self.item_model:
            self.item_model.layoutAboutToBeChanged.emit()
        self.item_delegate.set_view(style)
        if style == AddonManagerDisplayStyle.COMPACT or style == AddonManagerDisplayStyle.COMPOSITE:
            self.ui.listPackages.setSpacing(2)
            self.ui.listPackages.setVerticalScrollMode(QtWidgets.QAbstractItemView.ScrollPerItem)
            self.ui.listPackages.verticalScrollBar().setSingleStep(-1)
        else:
            self.ui.listPackages.setSpacing(5)
            self.ui.listPackages.setVerticalScrollMode(QtWidgets.QAbstractItemView.ScrollPerPixel)
            self.ui.listPackages.verticalScrollBar().setSingleStep(24)
        if self.item_model:
            self.item_model.layoutChanged.emit()

        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("ViewStyle", style)


class PackageListItemModel(QtCore.QAbstractListModel):
    """The model for use with the PackageList class."""

    repos = []
    write_lock = threading.Lock()

    DataAccessRole = QtCore.Qt.UserRole

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

        # Sorting
        if role == SortOptions.Alphabetical:
            return self.repos[row].display_name
        if role == SortOptions.LastUpdated:
            update_date = self.repos[row].update_date
            if update_date and hasattr(update_date, "timestamp"):
                return update_date.timestamp()
            return 0
        if role == SortOptions.DateAdded:
            if self.repos[row].stats and self.repos[row].stats.date_created:
                return self.repos[row].stats.date_created.timestamp()
            return 0
        if role == SortOptions.Stars:
            if self.repos[row].stats and self.repos[row].stats.stars:
                return self.repos[row].stats.stars
            return 0
        if role == SortOptions.Score:
            return self.repos[row].score

    def headerData(self, _unused1, _unused2, _role=QtCore.Qt.DisplayRole):
        """No header in this implementation: always returns None."""
        return None

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
        self.displayStyle = AddonManagerDisplayStyle.EXPANDED
        self.sort_order = SortOptions.Alphabetical
        self.expanded = ExpandedView()
        self.compact = CompactView()
        self.widget = self.expanded

    def set_view(self, style: AddonManagerDisplayStyle) -> None:
        """Set the view of to style"""
        if not self.displayStyle == style:
            self.displayStyle = style

    def set_sort(self, sort: SortOptions) -> None:
        """When sorting by various things, we display the thing that's being sorted on."""
        if not self.sort_order == sort:
            self.sort_order = sort

    def sizeHint(self, _option, index):
        """Attempt to figure out the correct height for the widget based on its
        current contents."""
        self.update_content(index)
        return self.widget.sizeHint()

    def update_content(self, index):
        """Creates the display of the content for a given index."""
        repo = index.data(PackageListItemModel.DataAccessRole)
        if self.displayStyle == AddonManagerDisplayStyle.EXPANDED:
            self.widget = self.expanded
            self._setup_expanded_view(repo)
        elif self.displayStyle == AddonManagerDisplayStyle.COMPACT:
            self.widget = self.compact
            self._setup_compact_view(repo)
        elif self.displayStyle == AddonManagerDisplayStyle.COMPOSITE:
            self.widget = self.compact  # For now reuse the compact list
            self._setup_composite_view(repo)
        self.widget.adjustSize()

    def _setup_expanded_view(self, addon: Addon) -> None:
        self.widget.ui.labelPackageName.setText(f"<h1>{addon.display_name}</h1>")
        self.widget.ui.labelIcon.setPixmap(addon.icon.pixmap(QtCore.QSize(48, 48)))
        self.widget.ui.labelStatus.setText(self.get_expanded_update_string(addon))
        self.widget.ui.labelIcon.setText("")
        self.widget.ui.labelTags.setText("")
        if addon.metadata:
            self.widget.ui.labelDescription.setText(addon.metadata.description)
            self.widget.ui.labelVersion.setText(f"<i>v{addon.metadata.version}</i>")
            self._set_package_maintainer_label(addon)
        elif addon.macro:
            self.widget.ui.labelDescription.setText(addon.macro.comment)
            self._set_macro_version_label(addon)
            self._set_macro_maintainer_label(addon)
        else:
            self.widget.ui.labelDescription.setText("")
            self.widget.ui.labelMaintainer.setText("")
            self.widget.ui.labelVersion.setText("")
        if addon.tags:
            self.widget.ui.labelTags.setText(
                translate("AddonsInstaller", "Tags") + ": " + ", ".join(addon.tags)
            )
        if self.sort_order == SortOptions.Alphabetical:
            self.widget.ui.labelSort.setText("")
        else:
            self.widget.ui.labelSort.setText(self._get_sort_label_text(addon))

    def _setup_compact_view(self, addon: Addon) -> None:
        self.widget.ui.labelPackageName.setText(f"<b>{addon.display_name}</b>")
        self.widget.ui.labelIcon.setPixmap(addon.icon.pixmap(QtCore.QSize(16, 16)))
        self.widget.ui.labelStatus.setText(self.get_compact_update_string(addon))
        self.widget.ui.labelIcon.setText("")
        if addon.metadata:
            self.widget.ui.labelVersion.setText(f"<i>v{addon.metadata.version}</i>")
        elif addon.macro:
            self._set_macro_version_label(addon)
        else:
            self.widget.ui.labelVersion.setText("")
        if self.sort_order == SortOptions.Alphabetical:
            description = self._get_compact_description(addon)
            self.widget.ui.labelDescription.setText(description)
        else:
            self.widget.ui.labelDescription.setText(self._get_sort_label_text(addon))

    def _setup_composite_view(self, addon: Addon) -> None:
        self.widget.ui.labelPackageName.setText(f"<b>{addon.display_name}</b>")
        self.widget.ui.labelIcon.setPixmap(addon.icon.pixmap(QtCore.QSize(16, 16)))
        self.widget.ui.labelStatus.setText(self.get_compact_update_string(addon))
        self.widget.ui.labelIcon.setText("")
        if addon.metadata:
            self.widget.ui.labelVersion.setText(f"<i>v{addon.metadata.version}</i>")
        elif addon.macro:
            self._set_macro_version_label(addon)
        else:
            self.widget.ui.labelVersion.setText("")
        if self.sort_order != SortOptions.Alphabetical:
            self.widget.ui.labelDescription.setText(self._get_sort_label_text(addon))
        else:
            self.widget.ui.labelDescription.setText("")

    def _set_package_maintainer_label(self, addon: Addon):
        maintainers = addon.metadata.maintainer
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

    def _set_macro_maintainer_label(self, repo: Addon):
        if repo.macro.author:
            caption = translate("AddonsInstaller", "Author")
            self.widget.ui.labelMaintainer.setText(caption + ": " + repo.macro.author)
        else:
            self.widget.ui.labelMaintainer.setText("")

    def _set_macro_version_label(self, addon: Addon):
        version_string = ""
        if addon.macro.version:
            version_string = addon.macro.version + " "
        if addon.macro.on_wiki:
            version_string += "(wiki)"
        elif addon.macro.on_git:
            version_string += "(git)"
        else:
            version_string += "(unknown source)"
        self.widget.ui.labelVersion.setText("<i>" + version_string + "</i>")

    def _get_sort_label_text(self, addon: Addon) -> str:
        if self.sort_order == SortOptions.Alphabetical:
            return ""
        elif self.sort_order == SortOptions.Stars:
            if addon.stats and addon.stats.stars and addon.stats.stars > 0:
                return translate("AddonsInstaller", "{} ★ on GitHub").format(addon.stats.stars)
            return translate("AddonsInstaller", "No ★, or not on GitHub")
        elif self.sort_order == SortOptions.DateAdded:
            if addon.stats and addon.stats.date_created:
                epoch_seconds = addon.stats.date_created.timestamp()
                qdt = QtCore.QDateTime.fromSecsSinceEpoch(int(epoch_seconds)).date()
                time_string = QtCore.QLocale().toString(qdt, QtCore.QLocale.ShortFormat)
                return translate("AddonsInstaller", "Created ") + time_string
            return ""
        elif self.sort_order == SortOptions.LastUpdated:
            update_date = addon.update_date
            if update_date:
                epoch_seconds = update_date.timestamp()
                qdt = QtCore.QDateTime.fromSecsSinceEpoch(int(epoch_seconds)).date()
                time_string = QtCore.QLocale().toString(qdt, QtCore.QLocale.ShortFormat)
                return translate("AddonsInstaller", "Updated ") + time_string
            return ""
        elif self.sort_order == SortOptions.Score:
            return translate("AddonsInstaller", "Score: ") + str(addon.score)
        return ""

    def _get_compact_description(self, addon: Addon) -> str:
        description = ""
        if addon.metadata:
            description = addon.metadata.description
        elif addon.macro and addon.macro.comment:
            description = addon.macro.comment
        trimmed_text, _, _ = description.partition(".")
        return trimmed_text.replace("\n", " ")

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
            if repo.installed_version or repo.installed_metadata:
                installed_version_string = (
                    "<br/>" + translate("AddonsInstaller", "Installed version") + ": "
                )
                if repo.installed_metadata:
                    installed_version_string += str(repo.installed_metadata.version)
                elif repo.installed_version:
                    installed_version_string += str(repo.installed_version)
            else:
                installed_version_string = "<br/>" + translate("AddonsInstaller", "Unknown version")

        installed_date_string = ""
        if repo.updated_timestamp:
            installed_date_string = "<br/>" + translate("AddonsInstaller", "Installed on") + ": "
            installed_date_string += QtCore.QLocale().toString(
                QtCore.QDateTime.fromSecsSinceEpoch(int(round(repo.updated_timestamp, 0))),
                QtCore.QLocale.ShortFormat,
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
        self.hide_non_OSI_approved = False
        self.hide_non_FSF_libre = False
        self.hide_unlicensed = False
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

    def setHideNonOSIApproved(self, hide: bool) -> None:
        """Sets whether to hide Addons with non-OSI-approved licenses"""
        self.hide_non_OSI_approved = hide
        self.invalidateFilter()

    def setHideNonFSFLibre(self, hide: bool) -> None:
        """Sets whether to hide Addons with non-FSF-Libre licenses"""
        self.hide_non_FSF_libre = hide
        self.invalidateFilter()

    def setHideUnlicensed(self, hide: bool) -> None:
        """Sets whether to hide addons without a specified license"""
        self.hide_unlicensed = hide
        self.invalidateFilter()

    def setHideNewerFreeCADRequired(self, hide_nfr: bool) -> None:
        """Sets whether to hide packages that have indicated they need a newer version
        of FreeCAD than the one currently running."""
        self.hide_newer_freecad_required = hide_nfr
        self.invalidateFilter()

    # def lessThan(self, left_in, right_in) -> bool:
    #    """Enable sorting of display name (not case-sensitive)."""
    #
    #    left = self.sourceModel().data(left_in, self.sortRole)
    #    right = self.sourceModel().data(right_in, self.sortRole)
    #
    #    return left.display_name.lower() < right.display_name.lower()

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

        license_manager = get_license_manager()
        if data.status() == Addon.Status.NOT_INSTALLED:

            # If it's not installed, check to see if it's Py2 only
            if self.hide_py2 and data.python2:
                return False

            # If it's not installed, check to see if it's marked obsolete
            if self.hide_obsolete and data.obsolete:
                return False

            if self.hide_unlicensed:
                if not data.license or data.license in ["UNLICENSED", "UNLICENCED"]:
                    FreeCAD.Console.PrintLog(f"Hiding {data.name} because it has no license set\n")
                    return False

            # If it is not an OSI-approved license, check to see if we are hiding those
            if self.hide_non_OSI_approved or self.hide_non_FSF_libre:
                if not data.license:
                    return False
                licenses_to_check = []
                if type(data.license) is str:
                    licenses_to_check.append(data.license)
                elif type(data.license) is list:
                    for license_id in data.license:
                        if type(license_id) is str:
                            licenses_to_check.append(license_id)
                        else:
                            licenses_to_check.append(license_id.name)
                else:
                    licenses_to_check.append(data.license.name)

                fsf_libre = False
                osi_approved = False
                for license_id in licenses_to_check:
                    if not osi_approved and license_manager.is_osi_approved(license_id):
                        osi_approved = True
                    if not fsf_libre and license_manager.is_fsf_libre(license_id):
                        fsf_libre = True
                if self.hide_non_OSI_approved and not osi_approved:
                    # FreeCAD.Console.PrintLog(
                    #    f"Hiding addon {data.name} because its license, {licenses_to_check}, "
                    #    f"is "
                    #    f"not OSI approved\n"
                    # )
                    return False
                if self.hide_non_FSF_libre and not fsf_libre:
                    # FreeCAD.Console.PrintLog(
                    #    f"Hiding addon {data.name} because its license, {licenses_to_check},  is "
                    #    f"not FSF Libre\n"
                    # )
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

        self.view_bar = WidgetViewControlBar(form)
        self.view_bar.setObjectName("ViewControlBar")
        self.horizontalLayout_6.addWidget(self.view_bar)

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
        self.listPackages.setVerticalScrollMode(QtWidgets.QAbstractItemView.ScrollPerPixel)
        self.listPackages.verticalScrollBar().setSingleStep(24)

        self.verticalLayout.addWidget(self.listPackages)

        self.progressBar = WidgetProgressBar()
        self.verticalLayout.addWidget(self.progressBar)

        QtCore.QMetaObject.connectSlotsByName(form)
