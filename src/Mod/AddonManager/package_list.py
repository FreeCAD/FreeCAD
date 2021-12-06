# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import FreeCAD

from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *

from typing import Dict, Union
from enum import IntEnum
import threading

from addonmanager_utilities import translate  # this needs to be as is for pylupdate
from AddonManagerRepo import AddonManagerRepo

from compact_view import Ui_CompactView
from expanded_view import Ui_ExpandedView

class ListDisplayStyle(IntEnum):
    COMPACT = 0
    EXPANDED = 1

class PackageList(QWidget):
    """ A widget that shows a list of packages and various widgets to control the display of the list """

    itemSelected = Signal(AddonManagerRepo)
    
    def __init__(self,parent=None):
        super().__init__(parent)
        self.ui = Ui_PackageList()
        self.ui.setupUi(self)
        
        self.item_filter = PackageListFilter()
        self.ui.listPackages.setModel (self.item_filter)
        self.item_delegate = PackageListItemDelegate(self.ui.listPackages)
        self.ui.listPackages.setItemDelegate(self.item_delegate)

        self.ui.listPackages.clicked.connect(self.on_listPackages_clicked)
        self.ui.comboPackageType.currentIndexChanged.connect(self.update_type_filter)
        self.ui.lineEditFilter.textChanged.connect(self.update_text_filter)
        self.ui.buttonCompactLayout.clicked.connect(lambda: self.set_view_style(ListDisplayStyle.COMPACT))
        self.ui.buttonExpandedLayout.clicked.connect(lambda: self.set_view_style(ListDisplayStyle.EXPANDED))
        
        # Only shows when the user types in a filter
        self.ui.labelFilterValidity.hide()

        # Set up the view the same as the last time:
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        package_type = pref.GetInt("PackageTypeSelection", 1)
        self.ui.comboPackageType.setCurrentIndex(package_type)

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

    def on_listPackages_clicked(self, index:QModelIndex):
        source_selection = self.item_filter.mapToSource (index)
        selected_repo = self.item_model.repos[source_selection.row()]
        self.itemSelected.emit(selected_repo)

    def update_type_filter(self, type_filter:int) -> None:
        """hide/show rows corresponding to the type filter
       
        type_filter is an integer: 0 for all, 1 for workbenches, 2 for macros, and 3 for preference packs
        
        """

        self.item_filter.setPackageFilter(type_filter)
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("PackageTypeSelection", type_filter)

    def update_text_filter(self, text_filter:str) -> None:
        """filter name and description by the regex specified by text_filter"""

        if text_filter:
            test_regex = QRegularExpression(text_filter)
            if test_regex.isValid():
                self.ui.labelFilterValidity.setToolTip(translate("AddonsInstaller","Filter is valid"))
                icon = QIcon.fromTheme("ok", QIcon(":/icons/edit_OK.svg"))
                self.ui.labelFilterValidity.setPixmap(icon.pixmap(16,16))
            else:
                self.ui.labelFilterValidity.setToolTip(translate("AddonsInstaller","Filter regular expression is invalid"))
                icon = QIcon.fromTheme("cancel", QIcon(":/icons/edit_Cancel.svg"))
                self.ui.labelFilterValidity.setPixmap(icon.pixmap(16,16))
            self.ui.labelFilterValidity.show()
        else:
            self.ui.labelFilterValidity.hide()
        self.item_filter.setFilterRegularExpression(text_filter)

    def set_view_style(self, style:ListDisplayStyle) -> None:
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

    def rowCount(self, parent:QModelIndex=QModelIndex()) -> int:
        if parent.isValid():
            return 0
        return len(self.repos)

    def columnCount(self, parent:QModelIndex=QModelIndex()) -> int:
        if parent.isValid():
            return 0
        return 1

    def data(self, index:QModelIndex, role:int=Qt.DisplayRole):
        if not index.isValid():
            return None
        row = index.row()
        if role == Qt.ToolTipRole:
            tooltip = ""
            if self.repos[row].repo_type == AddonManagerRepo.RepoType.PACKAGE:
                tooltip = translate("AddonsInstaller","Click for details about package") + f" '{self.repos[row].name}'"
            elif self.repos[row].repo_type == AddonManagerRepo.RepoType.WORKBENCH:
                tooltip = translate("AddonsInstaller","Click for details about workbench") + f" '{self.repos[row].name}'"
            elif self.repos[row].repo_type == AddonManagerRepo.RepoType.MACRO:
                tooltip = translate("AddonsInstaller","Click for details about macro") + f" '{self.repos[row].name}'"
            return tooltip
        elif role == PackageListItemModel.DataAccessRole:
            return self.repos[row]

    def headerData(self, section, orientation, role=Qt.DisplayRole):
        return None

    def setData(self, index:QModelIndex, value, role=Qt.EditRole) -> None:
        """ Set the data for this row. The column of the index is ignored. """

        row = index.row()
        self.write_lock.acquire()
        if role == PackageListItemModel.StatusUpdateRole:
            self.repos[row].update_status = value
            self.dataChanged.emit(self.index(row,2), self.index(row,2), [PackageListItemModel.StatusUpdateRole])
        elif role == PackageListItemModel.IconUpdateRole:
            self.repos[row].icon = value
            self.dataChanged.emit(self.index(row,0), self.index(row,0), [PackageListItemModel.IconUpdateRole]) 
        self.write_lock.release()

    def append_item(self, repo:AddonManagerRepo) -> None:
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
            self.beginRemoveRows(QModelIndex(), 0, self.rowCount()-1)
            self.repos = []
            self.endRemoveRows()
            self.write_lock.release()

    def update_item_status(self, name:str, status:AddonManagerRepo.UpdateStatus) -> None:
        for row,item in enumerate(self.repos):
            if item.name == name:
                self.setData(self.index(row,0), status, PackageListItemModel.StatusUpdateRole)
                return

    def update_item_icon(self, name:str, icon:QIcon) -> None:
        for row,item in enumerate(self.repos):
            if item.name == name:
                self.setData(self.index(row,0), icon, PackageListItemModel.IconUpdateRole)
                return

    def reload_item(self,repo:AddonManagerRepo) -> None:
        for index,item in enumerate(self.repos):
            if item.name == repo.name:
                self.write_lock.acquire()
                self.repos[index] = repo
                self.write_lock.release()
                return

class CompactView(QWidget):
    """ A single-line view of the package information """

    from compact_view import Ui_CompactView

    def __init__(self,parent=None):
        super().__init__(parent)
        self.ui = Ui_CompactView()
        self.ui.setupUi(self)

class ExpandedView(QWidget):
    """ A multi-line view of the package information """

    from expanded_view import Ui_ExpandedView

    def __init__(self,parent=None):
        super().__init__(parent)
        self.ui = Ui_ExpandedView()
        self.ui.setupUi(self)

class PackageListItemDelegate(QStyledItemDelegate):
    """ Render the repo data as a formatted region """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.displayStyle = ListDisplayStyle.EXPANDED
        self.expanded = ExpandedView()
        self.compact = CompactView()
        self.widget = self.expanded
        
    def set_view (self, style:ListDisplayStyle) -> None:
        if not self.displayStyle == style:
            self.displayStyle = style

    def sizeHint(self, option, index):
        self.update_content(index)
        return self.widget.sizeHint()

    def update_content(self, index):
        repo = index.data(PackageListItemModel.DataAccessRole)
        if self.displayStyle == ListDisplayStyle.EXPANDED:
            self.widget = self.expanded
            self.widget.ui.labelPackageName.setText(f"<h1>{repo.name}</h1>")
            self.widget.ui.labelIcon.setPixmap(repo.icon.pixmap(QSize(48,48)))
        else:
            self.widget = self.compact
            self.widget.ui.labelPackageName.setText(f"<b>{repo.name}</b>")
            self.widget.ui.labelIcon.setPixmap(repo.icon.pixmap(QSize(16,16)))

        self.widget.ui.labelIcon.setText("")
        if repo.metadata:
            self.widget.ui.labelDescription.setText(repo.metadata.Description)
            self.widget.ui.labelVersion.setText(f"<i>v{repo.metadata.Version}</i>")
            if self.displayStyle == ListDisplayStyle.EXPANDED:
                maintainers = repo.metadata.Maintainer
                maintainers_string = ""
                if len(maintainers) == 1:
                    maintainers_string = translate("AddonsInstaller","Maintainer") + f": {maintainers[0]['name']} <{maintainers[0]['email']}>"
                elif len(maintainers) > 1:
                    maintainers_string = translate("AddonsInstaller","Maintainers:")
                    for maintainer in maintainers:
                        maintainers_string += f"\n{maintainer['name']} <{maintainer['email']}>"
                self.widget.ui.labelMaintainer.setText(maintainers_string)
        else:
            self.widget.ui.labelDescription.setText("")
            self.widget.ui.labelVersion.setText("")
            if self.displayStyle == ListDisplayStyle.EXPANDED:
                self.widget.ui.labelMaintainer.setText("")

        # Update status
        if repo.update_status == AddonManagerRepo.UpdateStatus.NOT_INSTALLED:
            self.widget.ui.labelStatus.setText("")
        elif repo.update_status == AddonManagerRepo.UpdateStatus.UNCHECKED:
            self.widget.ui.labelStatus.setText(translate("AddonsInstaller","Installed"))
        elif repo.update_status == AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE:
            self.widget.ui.labelStatus.setText(translate("AddonsInstaller","Up-to-date"))
        elif repo.update_status == AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE:
            self.widget.ui.labelStatus.setText(translate("AddonsInstaller","Update available"))
        elif repo.update_status == AddonManagerRepo.UpdateStatus.PENDING_RESTART:
            self.widget.ui.labelStatus.setText(translate("AddonsInstaller","Pending restart"))
        self.widget.adjustSize()

    def paint(self, painter:QPainter, option:QStyleOptionViewItem, index:QModelIndex):
        painter.save()
        self.widget.resize(option.rect.size())
        painter.translate(option.rect.topLeft())
        self.widget.render(painter, QPoint(), QRegion(), QWidget.DrawChildren)
        painter.restore()

class PackageListFilter(QSortFilterProxyModel):
    """ Handle filtering the item list on various criteria """

    def __init__(self):
        super().__init__()
        self.package_type = 0 # Default to showing everything
        self.setSortCaseSensitivity(Qt.CaseInsensitive)

    def setPackageFilter(self, type:int) -> None: # 0=All, 1=Workbenches, 2=Macros, 3=Preference Packs
        self.package_type = type
        self.invalidateFilter()
        
    def lessThan(self, left, right) -> bool:
        l = self.sourceModel().data(left,PackageListItemModel.DataAccessRole)
        r = self.sourceModel().data(right,PackageListItemModel.DataAccessRole)

        lname = l.name if l.metadata is None else l.metadata.Name
        rname = r.name if r.metadata is None else r.metadata.Name
        return lname.lower() < rname.lower()

    def filterAcceptsRow(self, row, parent=QModelIndex()):
        index = self.sourceModel().createIndex(row, 0)
        data = self.sourceModel().data(index,PackageListItemModel.DataAccessRole)
        if self.package_type == 1:
            if not data.contains_workbench():
                return False
        elif self.package_type == 2:
           if  not data.contains_macro():
               return False
        elif self.package_type == 3:
           if not data.contains_preference_pack():
               return False
        
        name = data.name if data.metadata is None else data.metadata.Name
        desc = data.description if not data.metadata else data.metadata.Description
        re = self.filterRegularExpression()
        if re.isValid():
            re.setPatternOptions(QRegularExpression.CaseInsensitiveOption)
            if re.match(name).hasMatch():
                return True
            if re.match(desc).hasMatch():
                return True
            return False
        else:
            return False

class Ui_PackageList(object):
    """ The contents of the PackageList widget """

    def setupUi(self, Form):
        if not Form.objectName():
            Form.setObjectName(u"PackageList")
        self.verticalLayout = QVBoxLayout(Form)
        self.verticalLayout.setObjectName(u"verticalLayout")
        self.horizontalLayout_6 = QHBoxLayout()
        self.horizontalLayout_6.setObjectName(u"horizontalLayout_6")
        self.buttonCompactLayout = QToolButton(Form)
        self.buttonCompactLayout.setObjectName(u"buttonCompactLayout")
        self.buttonCompactLayout.setCheckable(True)
        self.buttonCompactLayout.setAutoExclusive(True)
        self.buttonCompactLayout.setIcon(QIcon.fromTheme("expanded_view", QIcon(":/icons/compact_view.svg")))

        self.horizontalLayout_6.addWidget(self.buttonCompactLayout)

        self.buttonExpandedLayout = QToolButton(Form)
        self.buttonExpandedLayout.setObjectName(u"buttonExpandedLayout")
        self.buttonExpandedLayout.setCheckable(True)
        self.buttonExpandedLayout.setChecked(True)
        self.buttonExpandedLayout.setAutoExclusive(True)
        self.buttonExpandedLayout.setIcon(QIcon.fromTheme("expanded_view", QIcon(":/icons/expanded_view.svg")))

        self.horizontalLayout_6.addWidget(self.buttonExpandedLayout)

        self.labelPackagesContaining = QLabel(Form)
        self.labelPackagesContaining.setObjectName(u"labelPackagesContaining")

        self.horizontalLayout_6.addWidget(self.labelPackagesContaining)

        self.comboPackageType = QComboBox(Form)
        self.comboPackageType.addItem("")
        self.comboPackageType.addItem("")
        self.comboPackageType.addItem("")
        self.comboPackageType.addItem("")
        self.comboPackageType.setObjectName(u"comboPackageType")

        self.horizontalLayout_6.addWidget(self.comboPackageType)

        self.lineEditFilter = QLineEdit(Form)
        self.lineEditFilter.setObjectName(u"lineEditFilter")
        self.lineEditFilter.setClearButtonEnabled(True)

        self.horizontalLayout_6.addWidget(self.lineEditFilter)

        self.labelFilterValidity = QLabel(Form)
        self.labelFilterValidity.setObjectName(u"labelFilterValidity")

        self.horizontalLayout_6.addWidget(self.labelFilterValidity)

        self.verticalLayout.addLayout(self.horizontalLayout_6)

        self.listPackages = QListView(Form)
        self.listPackages.setObjectName(u"listPackages")
        self.listPackages.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.listPackages.setProperty("showDropIndicator", False)
        self.listPackages.setSelectionMode(QAbstractItemView.NoSelection)
        self.listPackages.setLayoutMode(QListView.Batched)
        self.listPackages.setBatchSize(15)
        self.listPackages.setResizeMode(QListView.Adjust)
        self.listPackages.setUniformItemSizes(False)
        self.listPackages.setAlternatingRowColors(True)
        self.listPackages.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        self.verticalLayout.addWidget(self.listPackages)

        self.retranslateUi(Form)

        QMetaObject.connectSlotsByName(Form)

    def retranslateUi(self, Form):
        self.labelPackagesContaining.setText(QCoreApplication.translate("AddonsInstaller", u"Show packages containing:", None))
        self.comboPackageType.setItemText(0, QCoreApplication.translate("AddonsInstaller", u"All", None))
        self.comboPackageType.setItemText(1, QCoreApplication.translate("AddonsInstaller", u"Workbenches", None))
        self.comboPackageType.setItemText(2, QCoreApplication.translate("AddonsInstaller", u"Macros", None))
        self.comboPackageType.setItemText(3, QCoreApplication.translate("AddonsInstaller", u"Preference Packs", None))

        self.lineEditFilter.setPlaceholderText(QCoreApplication.translate("AddonsInstaller", u"Filter", None))
        self.labelFilterValidity.setText(QCoreApplication.translate("AddonsInstaller", u"OK", None))

