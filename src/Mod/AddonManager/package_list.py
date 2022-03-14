# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
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

import FreeCAD

from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *

from enum import IntEnum
import threading

from Addon import Addon

from compact_view import Ui_CompactView
from expanded_view import Ui_ExpandedView

import addonmanager_utilities as utils

translate = FreeCAD.Qt.translate


class ListDisplayStyle(IntEnum):
    COMPACT = 0
    EXPANDED = 1


class StatusFilter(IntEnum):
    ANY = 0
    INSTALLED = 1
    NOT_INSTALLED = 2
    UPDATE_AVAILABLE = 3


class PackageList(QWidget):
    """A widget that shows a list of packages and various widgets to control the display of the list"""

    itemSelected = Signal(Addon)

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

    def setModel(self, model):
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
        self.item_filter.setHideNewerFreeCADRequired(
            pref.GetBool("HideNewerFreeCADRequired", True)
        )

    def on_listPackages_clicked(self, index: QModelIndex):
        source_selection = self.item_filter.mapToSource(index)
        selected_repo = self.item_model.repos[source_selection.row()]
        self.itemSelected.emit(selected_repo)

    def update_type_filter(self, type_filter: int) -> None:
        """hide/show rows corresponding to the type filter

        type_filter is an integer: 0 for all, 1 for workbenches, 2 for macros, and 3 for preference packs

        """

        self.item_filter.setPackageFilter(type_filter)
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("PackageTypeSelection", type_filter)

    def update_status_filter(self, status_filter: int) -> None:
        """hide/show rows corresponding to the status filter

        status_filter is an integer: 0 for any, 1 for installed, 2 for not installed, and 3 for update available

        """

        self.item_filter.setStatusFilter(status_filter)
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("StatusSelection", status_filter)

    def update_text_filter(self, text_filter: str) -> None:
        """filter name and description by the regex specified by text_filter"""

        if text_filter:
            if hasattr(
                self.item_filter, "setFilterRegularExpression"
            ):  # Added in Qt 5.12
                test_regex = QRegularExpression(text_filter)
            else:
                test_regex = QRegExp(text_filter)
            if test_regex.isValid():
                self.ui.labelFilterValidity.setToolTip(
                    translate("AddonsInstaller", "Filter is valid")
                )
                icon = QIcon.fromTheme("ok", QIcon(":/icons/edit_OK.svg"))
                self.ui.labelFilterValidity.setPixmap(icon.pixmap(16, 16))
            else:
                self.ui.labelFilterValidity.setToolTip(
                    translate("AddonsInstaller", "Filter regular expression is invalid")
                )
                icon = QIcon.fromTheme("cancel", QIcon(":/icons/edit_Cancel.svg"))
                self.ui.labelFilterValidity.setPixmap(icon.pixmap(16, 16))
            self.ui.labelFilterValidity.show()
        else:
            self.ui.labelFilterValidity.hide()
        if hasattr(self.item_filter, "setFilterRegularExpression"):  # Added in Qt 5.12
            self.item_filter.setFilterRegularExpression(text_filter)
        else:
            self.item_filter.setFilterRegExp(text_filter)

    def set_view_style(self, style: ListDisplayStyle) -> None:
        self.item_model.layoutAboutToBeChanged.emit()
        self.item_delegate.set_view(style)
        if style == ListDisplayStyle.COMPACT:
            self.ui.listPackages.setSpacing(2)
        else:
            self.ui.listPackages.setSpacing(5)
        self.item_model.layoutChanged.emit()

        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("ViewStyle", style)


class PackageListItemModel(QAbstractListModel):

    repos = []
    write_lock = threading.Lock()

    DataAccessRole = Qt.UserRole
    StatusUpdateRole = Qt.UserRole + 1
    IconUpdateRole = Qt.UserRole + 2

    def __init__(self, parent=None) -> None:
        super().__init__(parent)

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:
        if parent.isValid():
            return 0
        return len(self.repos)

    def columnCount(self, parent: QModelIndex = QModelIndex()) -> int:
        if parent.isValid():
            return 0
        return 1

    def data(self, index: QModelIndex, role: int = Qt.DisplayRole):
        if not index.isValid():
            return None
        row = index.row()
        if role == Qt.ToolTipRole:
            tooltip = ""
            if self.repos[row].repo_type == Addon.Kind.PACKAGE:
                tooltip = translate(
                    "AddonsInstaller", "Click for details about package {}"
                ).format(self.repos[row].display_name)
            elif self.repos[row].repo_type == Addon.Kind.WORKBENCH:
                tooltip = translate(
                    "AddonsInstaller", "Click for details about workbench {}"
                ).format(self.repos[row].display_name)
            elif self.repos[row].repo_type == Addon.Kind.MACRO:
                tooltip = translate(
                    "AddonsInstaller", "Click for details about macro {}"
                ).format(self.repos[row].display_name)
            return tooltip
        elif role == PackageListItemModel.DataAccessRole:
            return self.repos[row]

    def headerData(self, section, orientation, role=Qt.DisplayRole):
        return None

    def setData(self, index: QModelIndex, value, role=Qt.EditRole) -> None:
        """Set the data for this row. The column of the index is ignored."""

        row = index.row()
        self.write_lock.acquire()
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
        self.write_lock.release()

    def append_item(self, repo: Addon) -> None:
        if repo in self.repos:
            # Cowardly refuse to insert the same repo a second time
            return
        self.write_lock.acquire()
        self.beginInsertRows(QModelIndex(), self.rowCount(), self.rowCount())
        self.repos.append(repo)
        self.endInsertRows()
        self.write_lock.release()

    def clear(self) -> None:
        if self.rowCount() > 0:
            self.write_lock.acquire()
            self.beginRemoveRows(QModelIndex(), 0, self.rowCount() - 1)
            self.repos = []
            self.endRemoveRows()
            self.write_lock.release()

    def update_item_status(self, name: str, status: Addon.Status) -> None:
        for row, item in enumerate(self.repos):
            if item.name == name:
                self.setData(
                    self.index(row, 0), status, PackageListItemModel.StatusUpdateRole
                )
                return

    def update_item_icon(self, name: str, icon: QIcon) -> None:
        for row, item in enumerate(self.repos):
            if item.name == name:
                self.setData(
                    self.index(row, 0), icon, PackageListItemModel.IconUpdateRole
                )
                return

    def reload_item(self, repo: Addon) -> None:
        for index, item in enumerate(self.repos):
            if item.name == repo.name:
                self.write_lock.acquire()
                self.repos[index] = repo
                self.write_lock.release()
                return


class CompactView(QWidget):
    """A single-line view of the package information"""

    from compact_view import Ui_CompactView

    def __init__(self, parent=None):
        super().__init__(parent)
        self.ui = Ui_CompactView()
        self.ui.setupUi(self)


class ExpandedView(QWidget):
    """A multi-line view of the package information"""

    from expanded_view import Ui_ExpandedView

    def __init__(self, parent=None):
        super().__init__(parent)
        self.ui = Ui_ExpandedView()
        self.ui.setupUi(self)


class PackageListItemDelegate(QStyledItemDelegate):
    """Render the repo data as a formatted region"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.displayStyle = ListDisplayStyle.EXPANDED
        self.expanded = ExpandedView()
        self.compact = CompactView()
        self.widget = self.expanded

    def set_view(self, style: ListDisplayStyle) -> None:
        if not self.displayStyle == style:
            self.displayStyle = style

    def sizeHint(self, option, index):
        self.update_content(index)
        return self.widget.sizeHint()

    def update_content(self, index):
        repo = index.data(PackageListItemModel.DataAccessRole)
        if self.displayStyle == ListDisplayStyle.EXPANDED:
            self.widget = self.expanded
            self.widget.ui.labelPackageName.setText(f"<h1>{repo.display_name}</h1>")
            self.widget.ui.labelIcon.setPixmap(repo.icon.pixmap(QSize(48, 48)))
        else:
            self.widget = self.compact
            self.widget.ui.labelPackageName.setText(f"<b>{repo.display_name}</b>")
            self.widget.ui.labelIcon.setPixmap(repo.icon.pixmap(QSize(16, 16)))

        self.widget.ui.labelIcon.setText("")
        if self.displayStyle == ListDisplayStyle.EXPANDED:
            self.widget.ui.labelTags.setText("")
        if repo.metadata:
            self.widget.ui.labelDescription.setText(repo.metadata.Description)
            self.widget.ui.labelVersion.setText(f"<i>v{repo.metadata.Version}</i>")
            if self.displayStyle == ListDisplayStyle.EXPANDED:
                maintainers = repo.metadata.Maintainer
                maintainers_string = ""
                if len(maintainers) == 1:
                    maintainers_string = (
                        translate("AddonsInstaller", "Maintainer")
                        + f": {maintainers[0]['name']} <{maintainers[0]['email']}>"
                    )
                elif len(maintainers) > 1:
                    n = len(maintainers)
                    maintainers_string = translate(
                        "AddonsInstaller", "Maintainers:", "", n
                    )
                    for maintainer in maintainers:
                        maintainers_string += (
                            f"\n{maintainer['name']} <{maintainer['email']}>"
                        )
                self.widget.ui.labelMaintainer.setText(maintainers_string)
                if repo.tags:
                    self.widget.ui.labelTags.setText(
                        translate("AddonsInstaller", "Tags")
                        + ": "
                        + ", ".join(repo.tags)
                    )
        elif repo.macro and repo.macro.parsed:
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
                    self.widget.ui.labelMaintainer.setText(
                        caption + ": " + repo.macro.author
                    )
                else:
                    self.widget.ui.labelMaintainer.setText("")
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

    def get_compact_update_string(self, repo: Addon) -> str:
        """Get a single-line string listing details about the installed version and date"""

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
            style = (
                "style='color:" + utils.warning_color_string() + "; font-weight:bold;'"
            )
            result += (
                f"<span {style}> ["
                + translate("AddonsInstaller", "DISABLED")
                + "]</span>"
            )

        return result

    def get_expanded_update_string(self, repo: Addon) -> str:
        """Get a multi-line string listing details about the installed version and date"""

        result = ""

        installed_version_string = ""
        if repo.status() != Addon.Status.NOT_INSTALLED:
            if repo.installed_version:
                installed_version_string = (
                    "<br/>" + translate("AddonsInstaller", "Installed version") + ": "
                )
                installed_version_string += repo.installed_version
            else:
                installed_version_string = "<br/>" + translate(
                    "AddonsInstaller", "Unknown version"
                )

        installed_date_string = ""
        if repo.updated_timestamp:
            installed_date_string = (
                "<br/>" + translate("AddonsInstaller", "Installed on") + ": "
            )
            installed_date_string += (
                QDateTime.fromTime_t(repo.updated_timestamp)
                .date()
                .toString(Qt.SystemLocaleShortDate)
            )

        available_version_string = ""
        if repo.metadata:
            available_version_string = (
                "<br/>" + translate("AddonsInstaller", "Available version") + ": "
            )
            available_version_string += repo.metadata.Version

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
            style = (
                "style='color:" + utils.warning_color_string() + "; font-weight:bold;'"
            )
            result += (
                f"<br/><span {style}>["
                + translate("AddonsInstaller", "DISABLED")
                + "]</span>"
            )

        return result

    def paint(self, painter: QPainter, option: QStyleOptionViewItem, _: QModelIndex):
        painter.save()
        self.widget.resize(option.rect.size())
        painter.translate(option.rect.topLeft())
        self.widget.render(painter, QPoint(), QRegion(), QWidget.DrawChildren)
        painter.restore()


class PackageListFilter(QSortFilterProxyModel):
    """Handle filtering the item list on various criteria"""

    def __init__(self):
        super().__init__()
        self.package_type = 0  # Default to showing everything
        self.status = 0  # Default to showing any
        self.setSortCaseSensitivity(Qt.CaseInsensitive)
        self.hide_obsolete = False
        self.hide_py2 = False
        self.hide_newer_freecad_required = False

    def setPackageFilter(
        self, type: int
    ) -> None:  # 0=All, 1=Workbenches, 2=Macros, 3=Preference Packs
        self.package_type = type
        self.invalidateFilter()

    def setStatusFilter(
        self, status: int
    ) -> None:  # 0=Any, 1=Installed, 2=Not installed, 3=Update available
        self.status = status
        self.invalidateFilter()

    def setHidePy2(self, hide_py2: bool) -> None:
        self.hide_py2 = hide_py2
        self.invalidateFilter()

    def setHideObsolete(self, hide_obsolete: bool) -> None:
        self.hide_obsolete = hide_obsolete
        self.invalidateFilter()

    def setHideNewerFreeCADRequired(self, hide_nfr: bool) -> None:
        self.hide_newer_freecad_required = hide_nfr
        self.invalidateFilter()

    def lessThan(self, left, right) -> bool:
        l = self.sourceModel().data(left, PackageListItemModel.DataAccessRole)
        r = self.sourceModel().data(right, PackageListItemModel.DataAccessRole)

        return l.display_name.lower() < r.display_name.lower()

    def filterAcceptsRow(self, row, parent=QModelIndex()):
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
        if (
            data.status() == Addon.Status.NOT_INSTALLED
            and self.hide_py2
            and data.python2
        ):
            return False

        # If it's not installed, check to see if it's marked obsolete
        if (
            data.status() == Addon.Status.NOT_INSTALLED
            and self.hide_obsolete
            and data.obsolete
        ):
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

            first_supported_version = data.metadata.getFirstSupportedFreeCADVersion()
            if first_supported_version is not None:
                required_version = first_supported_version.split(".")
                fc_major = int(FreeCAD.Version()[0])
                fc_minor = int(FreeCAD.Version()[1])

                if int(required_version[0]) > fc_major:
                    return False
                elif int(required_version[0]) == fc_major and len(required_version) > 1:
                    if int(required_version[1]) > fc_minor:
                        return False

        name = data.display_name
        desc = data.description
        if hasattr(self, "filterRegularExpression"):  # Added in Qt 5.12
            re = self.filterRegularExpression()
            if re.isValid():
                re.setPatternOptions(QRegularExpression.CaseInsensitiveOption)
                if re.match(name).hasMatch():
                    return True
                if re.match(desc).hasMatch():
                    return True
                if (
                    data.macro
                    and data.macro.comment
                    and re.match(data.macro.comment).hasMatch()
                ):
                    return True
                for tag in data.tags:
                    if re.match(tag).hasMatch():
                        return True
                return False
            else:
                return False
        else:
            re = self.filterRegExp()
            if re.isValid():
                re.setCaseSensitivity(Qt.CaseInsensitive)
                if re.indexIn(name) != -1:
                    return True
                if re.indexIn(desc) != -1:
                    return True
                if (
                    data.macro
                    and data.macro.comment
                    and re.indexIn(data.macro.comment) != -1
                ):
                    return True
                for tag in data.tags:
                    if re.indexIn(tag) != -1:
                        return True
                return False
            else:
                return False


class Ui_PackageList(object):
    """The contents of the PackageList widget"""

    def setupUi(self, Form):
        if not Form.objectName():
            Form.setObjectName("PackageList")
        self.verticalLayout = QVBoxLayout(Form)
        self.verticalLayout.setObjectName("verticalLayout")
        self.horizontalLayout_6 = QHBoxLayout()
        self.horizontalLayout_6.setObjectName("horizontalLayout_6")
        self.buttonCompactLayout = QToolButton(Form)
        self.buttonCompactLayout.setObjectName("buttonCompactLayout")
        self.buttonCompactLayout.setCheckable(True)
        self.buttonCompactLayout.setAutoExclusive(True)
        self.buttonCompactLayout.setIcon(
            QIcon.fromTheme("expanded_view", QIcon(":/icons/compact_view.svg"))
        )

        self.horizontalLayout_6.addWidget(self.buttonCompactLayout)

        self.buttonExpandedLayout = QToolButton(Form)
        self.buttonExpandedLayout.setObjectName("buttonExpandedLayout")
        self.buttonExpandedLayout.setCheckable(True)
        self.buttonExpandedLayout.setChecked(True)
        self.buttonExpandedLayout.setAutoExclusive(True)
        self.buttonExpandedLayout.setIcon(
            QIcon.fromTheme("expanded_view", QIcon(":/icons/expanded_view.svg"))
        )

        self.horizontalLayout_6.addWidget(self.buttonExpandedLayout)

        self.labelPackagesContaining = QLabel(Form)
        self.labelPackagesContaining.setObjectName("labelPackagesContaining")

        self.horizontalLayout_6.addWidget(self.labelPackagesContaining)

        self.comboPackageType = QComboBox(Form)
        self.comboPackageType.addItem("")
        self.comboPackageType.addItem("")
        self.comboPackageType.addItem("")
        self.comboPackageType.addItem("")
        self.comboPackageType.setObjectName("comboPackageType")

        self.horizontalLayout_6.addWidget(self.comboPackageType)

        self.labelStatus = QLabel(Form)
        self.labelStatus.setObjectName("labelStatus")

        self.horizontalLayout_6.addWidget(self.labelStatus)

        self.comboStatus = QComboBox(Form)
        self.comboStatus.addItem("")
        self.comboStatus.addItem("")
        self.comboStatus.addItem("")
        self.comboStatus.addItem("")
        self.comboStatus.setObjectName("comboStatus")

        self.horizontalLayout_6.addWidget(self.comboStatus)

        self.lineEditFilter = QLineEdit(Form)
        self.lineEditFilter.setObjectName("lineEditFilter")
        self.lineEditFilter.setClearButtonEnabled(True)

        self.horizontalLayout_6.addWidget(self.lineEditFilter)

        self.labelFilterValidity = QLabel(Form)
        self.labelFilterValidity.setObjectName("labelFilterValidity")

        self.horizontalLayout_6.addWidget(self.labelFilterValidity)

        self.verticalLayout.addLayout(self.horizontalLayout_6)

        self.listPackages = QListView(Form)
        self.listPackages.setObjectName("listPackages")
        self.listPackages.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.listPackages.setProperty("showDropIndicator", False)
        self.listPackages.setSelectionMode(QAbstractItemView.NoSelection)
        self.listPackages.setResizeMode(QListView.Adjust)
        self.listPackages.setUniformItemSizes(False)
        self.listPackages.setAlternatingRowColors(True)
        self.listPackages.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        self.verticalLayout.addWidget(self.listPackages)

        self.retranslateUi(Form)

        QMetaObject.connectSlotsByName(Form)

    def retranslateUi(self, Form):
        self.labelPackagesContaining.setText(
            QCoreApplication.translate(
                "AddonsInstaller", "Show Addons containing:", None
            )
        )
        self.comboPackageType.setItemText(
            0, QCoreApplication.translate("AddonsInstaller", "All", None)
        )
        self.comboPackageType.setItemText(
            1, QCoreApplication.translate("AddonsInstaller", "Workbenches", None)
        )
        self.comboPackageType.setItemText(
            2, QCoreApplication.translate("AddonsInstaller", "Macros", None)
        )
        self.comboPackageType.setItemText(
            3, QCoreApplication.translate("AddonsInstaller", "Preference Packs", None)
        )
        self.labelStatus.setText(
            QCoreApplication.translate("AddonsInstaller", "Status:", None)
        )
        self.comboStatus.setItemText(
            StatusFilter.ANY, QCoreApplication.translate("AddonsInstaller", "Any", None)
        )
        self.comboStatus.setItemText(
            StatusFilter.INSTALLED,
            QCoreApplication.translate("AddonsInstaller", "Installed", None),
        )
        self.comboStatus.setItemText(
            StatusFilter.NOT_INSTALLED,
            QCoreApplication.translate("AddonsInstaller", "Not installed", None),
        )
        self.comboStatus.setItemText(
            StatusFilter.UPDATE_AVAILABLE,
            QCoreApplication.translate("AddonsInstaller", "Update available", None),
        )
        self.lineEditFilter.setPlaceholderText(
            QCoreApplication.translate("AddonsInstaller", "Filter", None)
        )
        self.labelFilterValidity.setText(
            QCoreApplication.translate("AddonsInstaller", "OK", None)
        )
