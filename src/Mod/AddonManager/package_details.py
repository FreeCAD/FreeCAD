# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
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
from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *

import os
import shutil
from datetime import date, timedelta

import FreeCAD

import addonmanager_utilities as utils
from addonmanager_workers import ShowWorker, GetMacroDetailsWorker
from AddonManagerRepo import AddonManagerRepo

import inspect

translate = FreeCAD.Qt.translate


class PackageDetails(QWidget):

    back = Signal()
    install = Signal(AddonManagerRepo)
    uninstall = Signal(AddonManagerRepo)
    update = Signal(AddonManagerRepo)
    execute = Signal(AddonManagerRepo)
    update_status = Signal(AddonManagerRepo)
    check_for_update = Signal(AddonManagerRepo)

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
        self.ui.buttonCheckForUpdate.clicked.connect(
            lambda: self.check_for_update.emit(self.repo)
        )

    def show_repo(self, repo: AddonManagerRepo, reload: bool = False) -> None:

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

        if repo.update_status != AddonManagerRepo.UpdateStatus.NOT_INSTALLED:

            version = repo.installed_version
            date = ""
            installed_version_string = "<h3>"
            if repo.updated_timestamp:
                date = (
                    QDateTime.fromTime_t(repo.updated_timestamp)
                    .date()
                    .toString(Qt.SystemLocaleShortDate)
                )
            if version and date:
                installed_version_string += (
                    translate(
                        "AddonsInstaller", f"Version {version} installed on {date}"
                    )
                    + ". "
                )
            elif version:
                installed_version_string += (
                    translate("AddonsInstaller", f"Version {version} installed") + ". "
                )
            elif date:
                installed_version_string += (
                    translate("AddonsInstaller", f"Installed on {date}") + ". "
                )
            else:
                installed_version_string += (
                    translate("AddonsInstaller", "Installed") + ". "
                )

            if repo.update_status == AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE:
                if repo.metadata:
                    installed_version_string += (
                        "<b>"
                        + translate("AddonsInstaller", "Update available to version")
                        + " "
                    )
                    installed_version_string += repo.metadata.Version
                    installed_version_string += ".</b>"
                elif repo.macro and repo.macro.version:
                    installed_version_string += (
                        "<b>"
                        + translate("AddonsInstaller", "Update available to version")
                        + " "
                    )
                    installed_version_string += repo.macro.version
                    installed_version_string += ".</b>"
                else:
                    installed_version_string += (
                        "<b>"
                        + translate(
                            "AddonsInstaller",
                            "An update is available",
                        )
                        + ".</b>"
                    )
            elif (
                repo.update_status == AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE
            ):
                installed_version_string += (
                    translate("AddonsInstaller", "This is the latest version available")
                    + "."
                )
            elif repo.update_status == AddonManagerRepo.UpdateStatus.PENDING_RESTART:
                installed_version_string += (
                    translate(
                        "AddonsInstaller", "Updated, please restart FreeCAD to use"
                    )
                    + "."
                )
            elif repo.update_status == AddonManagerRepo.UpdateStatus.UNCHECKED:

                pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
                autocheck = pref.GetBool("AutoCheck", False)
                if autocheck:
                    installed_version_string += (
                        translate("AddonsInstaller", "Update check in progress") + "."
                    )
                else:
                    installed_version_string += (
                        translate("AddonsInstaller", "Automatic update checks disabled")
                        + "."
                    )

            installed_version_string += "</h3>"
            self.ui.labelPackageDetails.setText(installed_version_string)
            if repo.update_status == AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE:
                self.ui.labelPackageDetails.setStyleSheet(
                    "color:" + utils.attention_color_string()
                )
            else:
                self.ui.labelPackageDetails.setStyleSheet(
                    "color:" + utils.bright_color_string()
                )
            self.ui.labelPackageDetails.show()

            if repo.macro is not None:
                moddir = FreeCAD.getUserMacroDir(True)
            else:
                basedir = FreeCAD.getUserAppDataDir()
                moddir = os.path.join(basedir, "Mod", repo.name)
            installationLocationString = (
                translate("AddonsInstaller", "Installation location") + ": " + moddir
            )

            self.ui.labelInstallationLocation.setText(installationLocationString)
            self.ui.labelInstallationLocation.show()
        else:
            self.ui.labelPackageDetails.hide()
            self.ui.labelInstallationLocation.hide()

        if repo.update_status == AddonManagerRepo.UpdateStatus.NOT_INSTALLED:
            self.ui.buttonInstall.show()
            self.ui.buttonUninstall.hide()
            self.ui.buttonUpdate.hide()
            self.ui.buttonCheckForUpdate.hide()
        elif repo.update_status == AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.hide()
            self.ui.buttonCheckForUpdate.hide()
        elif repo.update_status == AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.show()
            self.ui.buttonCheckForUpdate.hide()
        elif repo.update_status == AddonManagerRepo.UpdateStatus.UNCHECKED:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.hide()
            self.ui.buttonCheckForUpdate.show()
        elif repo.update_status == AddonManagerRepo.UpdateStatus.PENDING_RESTART:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.hide()
            self.ui.buttonCheckForUpdate.hide()

        if repo.obsolete:
            self.ui.labelWarningInfo.show()
            self.ui.labelWarningInfo.setText(
                "<h1>"
                + translate("AddonsInstaller", "WARNING: This addon is obsolete")
                + "</h1>"
            )
            self.ui.labelWarningInfo.setStyleSheet(
                "color:" + utils.warning_color_string()
            )
        elif repo.python2:
            self.ui.labelWarningInfo.show()
            self.ui.labelWarningInfo.setText(
                "<h1>"
                + translate("AddonsInstaller", "WARNING: This addon is Python 2 Only")
                + "</h1>"
            )
            self.ui.labelWarningInfo.setStyleSheet(
                "color:" + utils.warning_color_string()
            )
        else:
            self.ui.labelWarningInfo.hide()

    @classmethod
    def cache_path(self, repo: AddonManagerRepo) -> str:
        cache_path = FreeCAD.getUserCachePath()
        full_path = os.path.join(cache_path, "AddonManager", repo.name)
        return full_path

    def check_and_clean_cache(self, force: bool = False) -> None:
        cache_path = PackageDetails.cache_path(self.repo)
        readme_cache_file = os.path.join(cache_path, "README.html")
        readme_images_path = os.path.join(cache_path, "Images")
        download_interrupted_sentinel = os.path.join(
            readme_images_path, "download_in_progress"
        )
        download_interrupted = os.path.isfile(download_interrupted_sentinel)
        if os.path.isfile(readme_cache_file):
            pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
            days_between_updates = pref.GetInt("DaysBetweenUpdates", 2 ^ 32)
            timestamp = os.path.getmtime(readme_cache_file)
            last_cache_update = date.fromtimestamp(timestamp)
            delta_update = timedelta(days=days_between_updates)
            if (
                date.today() >= last_cache_update + delta_update
                or download_interrupted
                or force
            ):
                if force:
                    FreeCAD.Console.PrintLog(
                        f"Forced README cache update for {self.repo.name}\n"
                    )
                elif download_interrupted:
                    FreeCAD.Console.PrintLog(
                        f"Restarting interrupted README download for {self.repo.name}\n"
                    )
                else:
                    FreeCAD.Console.PrintLog(
                        f"Cache expired, downloading README for {self.repo.name} again\n"
                    )
                os.remove(readme_cache_file)
                if os.path.isdir(readme_images_path):
                    shutil.rmtree(readme_images_path)

    def refresh(self):
        self.check_and_clean_cache(force=True)
        self.show_repo(self.repo)

    def show_cached_readme(self, repo: AddonManagerRepo) -> bool:
        """Attempts to show a cached readme, returns true if there was a cache, or false if not"""

        cache_path = PackageDetails.cache_path(repo)
        readme_cache_file = os.path.join(cache_path, "README.html")
        if os.path.isfile(readme_cache_file):
            with open(readme_cache_file, "rb") as f:
                data = f.read()
                self.ui.textBrowserReadMe.setText(data.decode())
                return True
        return False

    def show_workbench(self, repo: AddonManagerRepo) -> None:
        """loads information of a given workbench"""

        if not self.show_cached_readme(repo):
            self.ui.textBrowserReadMe.setText(
                translate(
                    "AddonsInstaller", "Fetching README.md from package repository"
                )
            )
            self.worker = ShowWorker(repo, PackageDetails.cache_path(repo))
            self.worker.readme_updated.connect(
                lambda desc: self.cache_readme(repo, desc)
            )
            self.worker.readme_updated.connect(
                lambda desc: self.ui.textBrowserReadMe.setText(desc)
            )
            self.worker.update_status.connect(self.update_status.emit)
            self.worker.update_status.connect(self.show)
            self.worker.start()

    def show_package(self, repo: AddonManagerRepo) -> None:
        """Show the details for a package (a repo with a package.xml metadata file)"""

        if not self.show_cached_readme(repo):
            self.ui.textBrowserReadMe.setText(
                translate(
                    "AddonsInstaller", "Fetching README.md from package repository"
                )
            )
            self.worker = ShowWorker(repo, PackageDetails.cache_path(repo))
            self.worker.readme_updated.connect(
                lambda desc: self.cache_readme(repo, desc)
            )
            self.worker.readme_updated.connect(
                lambda desc: self.ui.textBrowserReadMe.setText(desc)
            )
            self.worker.update_status.connect(self.update_status.emit)
            self.worker.update_status.connect(self.show)
            self.worker.start()

    def show_macro(self, repo: AddonManagerRepo) -> None:
        """loads information of a given macro"""

        if not self.show_cached_readme(repo):
            self.ui.textBrowserReadMe.setText(
                translate(
                    "AddonsInstaller", "Fetching README.md from package repository"
                )
            )
            self.worker = GetMacroDetailsWorker(repo)
            self.worker.readme_updated.connect(
                lambda desc: self.cache_readme(repo, desc)
            )
            self.worker.readme_updated.connect(
                lambda desc: self.ui.textBrowserReadMe.setText(desc)
            )
            self.worker.start()

    def cache_readme(self, repo: AddonManagerRepo, readme: str) -> None:
        cache_path = PackageDetails.cache_path(repo)
        readme_cache_file = os.path.join(cache_path, "README.html")
        os.makedirs(cache_path, exist_ok=True)
        with open(readme_cache_file, "wb") as f:
            f.write(readme.encode())


class Ui_PackageDetails(object):
    def setupUi(self, PackageDetails):
        if not PackageDetails.objectName():
            PackageDetails.setObjectName("PackageDetails")
        self.verticalLayout_2 = QVBoxLayout(PackageDetails)
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        self.layoutDetailsBackButton = QHBoxLayout()
        self.layoutDetailsBackButton.setObjectName("layoutDetailsBackButton")
        self.buttonBack = QToolButton(PackageDetails)
        self.buttonBack.setObjectName("buttonBack")
        self.buttonBack.setIcon(
            QIcon.fromTheme("back", QIcon(":/icons/button_left.svg"))
        )
        self.buttonRefresh = QToolButton(PackageDetails)
        self.buttonRefresh.setObjectName("buttonRefresh")
        self.buttonRefresh.setIcon(
            QIcon.fromTheme("refresh", QIcon(":/icons/view-refresh.svg"))
        )

        self.layoutDetailsBackButton.addWidget(self.buttonBack)
        self.layoutDetailsBackButton.addWidget(self.buttonRefresh)

        self.horizontalSpacer = QSpacerItem(
            40, 20, QSizePolicy.Expanding, QSizePolicy.Minimum
        )

        self.layoutDetailsBackButton.addItem(self.horizontalSpacer)

        self.buttonInstall = QPushButton(PackageDetails)
        self.buttonInstall.setObjectName("buttonInstall")

        self.layoutDetailsBackButton.addWidget(self.buttonInstall)

        self.buttonUninstall = QPushButton(PackageDetails)
        self.buttonUninstall.setObjectName("buttonUninstall")

        self.layoutDetailsBackButton.addWidget(self.buttonUninstall)

        self.buttonUpdate = QPushButton(PackageDetails)
        self.buttonUpdate.setObjectName("buttonUpdate")

        self.layoutDetailsBackButton.addWidget(self.buttonUpdate)

        self.buttonCheckForUpdate = QPushButton(PackageDetails)
        self.buttonCheckForUpdate.setObjectName("buttonCheckForUpdate")

        self.layoutDetailsBackButton.addWidget(self.buttonCheckForUpdate)

        self.buttonExecute = QPushButton(PackageDetails)
        self.buttonExecute.setObjectName("buttonExecute")

        self.layoutDetailsBackButton.addWidget(self.buttonExecute)

        self.verticalLayout_2.addLayout(self.layoutDetailsBackButton)

        self.labelPackageDetails = QLabel(PackageDetails)
        self.labelPackageDetails.hide()

        self.verticalLayout_2.addWidget(self.labelPackageDetails)

        self.labelInstallationLocation = QLabel(PackageDetails)
        self.labelInstallationLocation.setTextInteractionFlags(Qt.TextSelectableByMouse)
        self.labelInstallationLocation.hide()

        self.verticalLayout_2.addWidget(self.labelInstallationLocation)

        self.labelWarningInfo = QLabel(PackageDetails)
        self.labelWarningInfo.hide()

        self.verticalLayout_2.addWidget(self.labelWarningInfo)

        self.textBrowserReadMe = QTextBrowser(PackageDetails)
        self.textBrowserReadMe.setObjectName("textBrowserReadMe")
        self.textBrowserReadMe.setOpenExternalLinks(True)
        self.textBrowserReadMe.setOpenLinks(True)

        self.verticalLayout_2.addWidget(self.textBrowserReadMe)

        self.retranslateUi(PackageDetails)

        QMetaObject.connectSlotsByName(PackageDetails)

    # setupUi

    def retranslateUi(self, PackageDetails):
        self.buttonBack.setText("")
        self.buttonInstall.setText(
            QCoreApplication.translate("AddonsInstaller", "Install", None)
        )
        self.buttonUninstall.setText(
            QCoreApplication.translate("AddonsInstaller", "Uninstall", None)
        )
        self.buttonUpdate.setText(
            QCoreApplication.translate("AddonsInstaller", "Update", None)
        )
        self.buttonCheckForUpdate.setText(
            QCoreApplication.translate("AddonsInstaller", "Check for Update", None)
        )
        self.buttonExecute.setText(
            QCoreApplication.translate("AddonsInstaller", "Run Macro", None)
        )
        self.buttonBack.setToolTip(
            QCoreApplication.translate(
                "AddonsInstaller", "Return to package list", None
            )
        )
        self.buttonRefresh.setToolTip(
            QCoreApplication.translate(
                "AddonsInstaller",
                "Delete cached version of this README and re-download",
                None,
            )
        )

    # retranslateUi
