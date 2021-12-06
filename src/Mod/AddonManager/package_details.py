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
from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *

import os
import shutil
from datetime import date, timedelta

import FreeCAD

from addonmanager_utilities import translate  # this needs to be as is for pylupdate
from addonmanager_workers import ShowWorker, GetMacroDetailsWorker
from AddonManagerRepo import AddonManagerRepo

import inspect

class PackageDetails(QWidget):

    back = Signal()
    install = Signal(AddonManagerRepo)
    uninstall = Signal(AddonManagerRepo)
    update = Signal(AddonManagerRepo)
    execute = Signal(AddonManagerRepo)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.ui = Ui_PackageDetails()
        self.ui.setupUi(self)

        self.worker = None
        self.repo = None

        self.ui.buttonBack.clicked.connect(self.back.emit)
        self.ui.buttonRefresh.clicked.connect(self.refresh)
        self.ui.buttonExecute.clicked.connect(lambda: self.execute.emit(self.repo))
        self.ui.buttonInstall.clicked.connect(lambda: self.install.emit(self.repo))
        self.ui.buttonUninstall.clicked.connect(lambda: self.uninstall.emit(self.repo))
        self.ui.buttonUpdate.clicked.connect(lambda: self.update.emit(self.repo))

    def show_repo(self, repo:AddonManagerRepo, reload:bool = False) -> None:

        self.repo = repo

        if self.worker is not None:
            if not self.worker.isFinished():
                self.worker.requestInterruption()
                self.worker.wait()

        # Always load bare macros from scratch, we need to grab their code, which isn't cached
        force_reload = reload
        if repo.repo_type == AddonManagerRepo.RepoType.MACRO:
            force_reload = True

        self.check_and_clean_cache(force_reload)

        if repo.repo_type == AddonManagerRepo.RepoType.MACRO:
            self.show_macro(repo)
            self.ui.buttonExecute.show()
        elif repo.repo_type == AddonManagerRepo.RepoType.WORKBENCH:
            self.show_workbench(repo)
            self.ui.buttonExecute.hide()
        elif repo.repo_type == AddonManagerRepo.RepoType.PACKAGE:
            self.show_package(repo)
            self.ui.buttonExecute.hide()

        if repo.update_status == AddonManagerRepo.UpdateStatus.NOT_INSTALLED:
            self.ui.buttonInstall.show()
            self.ui.buttonUninstall.hide()
            self.ui.buttonUpdate.hide()
        elif repo.update_status == AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.hide()
        elif repo.update_status == AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.show()
        elif repo.update_status == AddonManagerRepo.UpdateStatus.UNCHECKED:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.hide()
        elif repo.update_status == AddonManagerRepo.UpdateStatus.PENDING_RESTART:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.hide()

    @classmethod
    def cache_path(self, repo:AddonManagerRepo) -> str:
        cache_path = FreeCAD.getUserCachePath()
        full_path = os.path.join(cache_path,"AddonManager",repo.name)
        return full_path

    def check_and_clean_cache(self, force:bool = False) -> None:
        cache_path = PackageDetails.cache_path(self.repo)
        readme_cache_file = os.path.join(cache_path,"README.html")
        readme_images_path = os.path.join(cache_path,"Images")
        download_interrupted_sentinel = os.path.join(readme_images_path,"download_in_progress")
        download_interrupted = os.path.isfile(download_interrupted_sentinel)
        if os.path.isfile(readme_cache_file):
            pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
            days_between_updates = pref.GetInt("DaysBetweenUpdates", 2^32)
            timestamp = os.path.getmtime(readme_cache_file)
            last_cache_update = date.fromtimestamp(timestamp)
            delta_update = timedelta(days=days_between_updates)
            if date.today() >= last_cache_update + delta_update or download_interrupted or force:
                if force:
                    FreeCAD.Console.PrintMessage(f"Forced README cache update for {self.repo.name}\n")
                elif download_interrupted:
                    FreeCAD.Console.PrintMessage(f"Restarting interrupted README download for {self.repo.name}\n")
                else:
                    FreeCAD.Console.PrintMessage(f"Cache expired, downloading README for {self.repo.name} again\n")
                os.remove(readme_cache_file)
                if os.path.isdir(readme_images_path):
                    shutil.rmtree(readme_images_path)

    def refresh(self):
        self.check_and_clean_cache(force=True)
        self.show_repo(self.repo)

    def show_cached_readme(self, repo:AddonManagerRepo) -> bool:
        """ Attempts to show a cached readme, returns true if there was a cache, or false if not """

        cache_path = PackageDetails.cache_path(repo)
        readme_cache_file = os.path.join(cache_path,"README.html")
        if os.path.isfile(readme_cache_file):
            with open(readme_cache_file,"rb") as f:
                data = f.read()
                self.ui.textBrowserReadMe.setText(data.decode())
                return True
        return False

    def show_workbench(self, repo:AddonManagerRepo) -> None:
        """loads information of a given workbench"""
        
        if not self.show_cached_readme(repo):
            self.ui.textBrowserReadMe.setText(translate("AddonsInstaller","Fetching README.md from package repository"))
            self.worker = ShowWorker(repo, PackageDetails.cache_path(repo))
            self.worker.readme_updated.connect(lambda desc: self.cache_readme(repo, desc))
            self.worker.readme_updated.connect(lambda desc: self.ui.textBrowserReadMe.setText(desc))
            self.worker.start()

    def show_package(self, repo:AddonManagerRepo) -> None:
        """ Show the details for a package (a repo with a package.xml metadata file) """
        
        if not self.show_cached_readme(repo):
            self.ui.textBrowserReadMe.setText(translate("AddonsInstaller","Fetching README.md from package repository"))
            self.worker = ShowWorker(repo,PackageDetails.cache_path(repo))
            self.worker.readme_updated.connect(lambda desc: self.cache_readme(repo, desc))
            self.worker.readme_updated.connect(lambda desc: self.ui.textBrowserReadMe.setText(desc))
            self.worker.start()

    def show_macro(self, repo:AddonManagerRepo) -> None:
        """loads information of a given macro"""
        
        if not self.show_cached_readme(repo):
            self.ui.textBrowserReadMe.setText(translate("AddonsInstaller","Fetching README.md from package repository"))
            self.worker = GetMacroDetailsWorker(repo)
            self.worker.readme_updated.connect(lambda desc: self.cache_readme(repo, desc))
            self.worker.readme_updated.connect(lambda desc: self.ui.textBrowserReadMe.setText(desc))
            self.worker.start()

    def cache_readme(self, repo:AddonManagerRepo, readme:str) -> None:
        cache_path = PackageDetails.cache_path(repo)
        readme_cache_file = os.path.join(cache_path,"README.html")
        os.makedirs(cache_path,exist_ok=True)
        with open(readme_cache_file,"wb") as f:
            f.write(readme.encode())

class Ui_PackageDetails(object):
    def setupUi(self, PackageDetails):
        if not PackageDetails.objectName():
            PackageDetails.setObjectName(u"PackageDetails")
        self.verticalLayout_2 = QVBoxLayout(PackageDetails)
        self.verticalLayout_2.setObjectName(u"verticalLayout_2")
        self.layoutDetailsBackButton = QHBoxLayout()
        self.layoutDetailsBackButton.setObjectName(u"layoutDetailsBackButton")
        self.buttonBack = QToolButton(PackageDetails)
        self.buttonBack.setObjectName(u"buttonBack")
        self.buttonBack.setIcon(QIcon.fromTheme("back", QIcon(":/icons/button_left.svg")))
        self.buttonRefresh = QToolButton(PackageDetails)
        self.buttonRefresh.setObjectName(u"buttonRefresh")
        self.buttonRefresh.setIcon(QIcon.fromTheme("refresh", QIcon(":/icons/view-refresh.svg")))

        self.layoutDetailsBackButton.addWidget(self.buttonBack)
        self.layoutDetailsBackButton.addWidget(self.buttonRefresh)

        self.horizontalSpacer = QSpacerItem(40, 20, QSizePolicy.Expanding, QSizePolicy.Minimum)

        self.layoutDetailsBackButton.addItem(self.horizontalSpacer)


        self.buttonInstall = QPushButton(PackageDetails)
        self.buttonInstall.setObjectName(u"buttonInstall")

        self.layoutDetailsBackButton.addWidget(self.buttonInstall)

        self.buttonUninstall = QPushButton(PackageDetails)
        self.buttonUninstall.setObjectName(u"buttonUninstall")

        self.layoutDetailsBackButton.addWidget(self.buttonUninstall)

        self.buttonUpdate = QPushButton(PackageDetails)
        self.buttonUpdate.setObjectName(u"buttonUpdate")

        self.layoutDetailsBackButton.addWidget(self.buttonUpdate)

        self.buttonExecute = QPushButton(PackageDetails)
        self.buttonExecute.setObjectName(u"buttonExecute")

        self.layoutDetailsBackButton.addWidget(self.buttonExecute)


        self.verticalLayout_2.addLayout(self.layoutDetailsBackButton)

        self.textBrowserReadMe = QTextBrowser(PackageDetails)
        self.textBrowserReadMe.setObjectName(u"textBrowserReadMe")
        self.textBrowserReadMe.setOpenExternalLinks(True)
        self.textBrowserReadMe.setOpenLinks(True)

        self.verticalLayout_2.addWidget(self.textBrowserReadMe)


        self.retranslateUi(PackageDetails)

        QMetaObject.connectSlotsByName(PackageDetails)
    # setupUi

    def retranslateUi(self, PackageDetails):
        self.buttonBack.setText("")
        self.buttonInstall.setText(QCoreApplication.translate("AddonsInstaller", u"Install", None))
        self.buttonUninstall.setText(QCoreApplication.translate("AddonsInstaller", u"Uninstall", None))
        self.buttonUpdate.setText(QCoreApplication.translate("AddonsInstaller", u"Update", None))
        self.buttonExecute.setText(QCoreApplication.translate("AddonsInstaller", u"Run Macro", None))
        self.buttonBack.setToolTip(QCoreApplication.translate("AddonsInstaller", u"Return to package list", None))
        self.buttonRefresh.setToolTip(QCoreApplication.translate("AddonsInstaller", u"Delete cached version of this README and re-download", None))
    # retranslateUi

