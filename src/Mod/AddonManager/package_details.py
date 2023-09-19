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

""" Provides the PackageDetails widget. """

import os
from typing import Optional

from PySide import QtCore, QtGui, QtWidgets

import addonmanager_freecad_interface as fci

import addonmanager_utilities as utils
from addonmanager_metadata import Version, UrlType, get_first_supported_freecad_version
from addonmanager_workers_startup import GetMacroDetailsWorker, CheckSingleUpdateWorker
from Addon import Addon
from change_branch import ChangeBranchDialog

have_git = False
try:
    import git

    if hasattr(git, "Repo"):
        have_git = True
except ImportError:
    git = None


translate = fci.translate

show_javascript_console_output = False

try:
    from PySide import QtWebEngineWidgets

    HAS_QTWEBENGINE = True
except ImportError:
    fci.Console.PrintWarning(
        translate(
            "AddonsInstaller",
            "Addon Manager Warning: Could not import QtWebEngineWidgets -- README data will display as text-only",
        )
        + "\n"
    )
    HAS_QTWEBENGINE = False


class PackageDetails(QtWidgets.QWidget):
    """The PackageDetails QWidget shows package README information and provides
    install, uninstall, and update buttons."""

    back = QtCore.Signal()
    install = QtCore.Signal(Addon)
    uninstall = QtCore.Signal(Addon)
    update = QtCore.Signal(Addon)
    execute = QtCore.Signal(Addon)
    update_status = QtCore.Signal(Addon)
    check_for_update = QtCore.Signal(Addon)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.ui = Ui_PackageDetails()
        self.ui.setupUi(self)

        self.worker = None
        self.repo = None
        self.status_update_thread = None

        self.ui.buttonBack.clicked.connect(self.back.emit)
        self.ui.buttonExecute.clicked.connect(lambda: self.execute.emit(self.repo))
        self.ui.buttonInstall.clicked.connect(lambda: self.install.emit(self.repo))
        self.ui.buttonUninstall.clicked.connect(lambda: self.uninstall.emit(self.repo))
        self.ui.buttonUpdate.clicked.connect(lambda: self.update.emit(self.repo))
        self.ui.buttonCheckForUpdate.clicked.connect(lambda: self.check_for_update.emit(self.repo))
        self.ui.buttonChangeBranch.clicked.connect(self.change_branch_clicked)
        self.ui.buttonEnable.clicked.connect(self.enable_clicked)
        self.ui.buttonDisable.clicked.connect(self.disable_clicked)
        if HAS_QTWEBENGINE:
            self.ui.webView.loadStarted.connect(self.load_started)
            self.ui.webView.loadProgress.connect(self.load_progress)
            self.ui.webView.loadFinished.connect(self.load_finished)

            loading_html_file = os.path.join(os.path.dirname(__file__), "loading.html")
            with open(loading_html_file, "r", errors="ignore", encoding="utf-8") as f:
                html = f.read()
                self.ui.loadingLabel.setHtml(html)
                self.ui.loadingLabel.show()
                self.ui.webView.hide()

    def show_repo(self, repo: Addon, reload: bool = False) -> None:
        """The main entry point for this class, shows the package details and related buttons
        for the provided repo. If reload is true, then even if this is already the current repo
        the data is reloaded."""

        # If this is the same repo we were already showing, we do not have to do the
        # expensive refetch unless reload is true
        if self.repo != repo or reload:
            self.repo = repo

            if HAS_QTWEBENGINE:
                self.ui.loadingLabel.show()
                self.ui.slowLoadLabel.hide()
                self.ui.webView.setHtml("<html><body>Loading...</body></html>")
                self.ui.webView.hide()
                self.ui.progressBar.show()
                self.timeout = QtCore.QTimer.singleShot(6000, self.long_load_running)  # Six seconds
            else:
                self.ui.missingWebViewLabel.setStyleSheet("color:" + utils.warning_color_string())

            if self.worker is not None:
                if not self.worker.isFinished():
                    self.worker.requestInterruption()
                    self.worker.wait()

            if repo.repo_type == Addon.Kind.MACRO:
                self.show_macro(repo)
                self.ui.buttonExecute.show()
            elif repo.repo_type == Addon.Kind.WORKBENCH:
                self.show_workbench(repo)
                self.ui.buttonExecute.hide()
            elif repo.repo_type == Addon.Kind.PACKAGE:
                self.show_package(repo)
                self.ui.buttonExecute.hide()

        if repo.status() == Addon.Status.UNCHECKED:
            if not self.status_update_thread:
                self.status_update_thread = QtCore.QThread()
            self.status_create_addon_list_worker = CheckSingleUpdateWorker(repo)
            self.status_create_addon_list_worker.moveToThread(self.status_update_thread)
            self.status_update_thread.finished.connect(
                self.status_create_addon_list_worker.deleteLater
            )
            self.check_for_update.connect(self.status_create_addon_list_worker.do_work)
            self.status_create_addon_list_worker.update_status.connect(self.display_repo_status)
            self.status_update_thread.start()
            self.check_for_update.emit(self.repo)

        self.display_repo_status(self.repo.update_status)

    def display_repo_status(self, status):
        """Updates the contents of the widget to display the current install status of the widget."""
        repo = self.repo
        self.set_change_branch_button_state()
        self.set_disable_button_state()
        if status != Addon.Status.NOT_INSTALLED:
            version = repo.installed_version
            date = ""
            installed_version_string = "<h3>"
            if repo.updated_timestamp:
                date = (
                    QtCore.QDateTime.fromTime_t(repo.updated_timestamp)
                    .date()
                    .toString(QtCore.Qt.SystemLocaleShortDate)
                )
            if version and date:
                installed_version_string += (
                    translate("AddonsInstaller", "Version {version} installed on {date}").format(
                        version=version, date=date
                    )
                    + ". "
                )
            elif version:
                installed_version_string += (
                    translate("AddonsInstaller", "Version {version} installed") + ". "
                ).format(version=version)
            elif date:
                installed_version_string += (
                    translate("AddonsInstaller", "Installed on {date}") + ". "
                ).format(date=date)
            else:
                installed_version_string += translate("AddonsInstaller", "Installed") + ". "

            if status == Addon.Status.UPDATE_AVAILABLE:
                if repo.metadata:
                    installed_version_string += (
                        "<b>"
                        + translate(
                            "AddonsInstaller",
                            "On branch {}, update available to version",
                        ).format(repo.branch)
                        + " "
                    )
                    installed_version_string += str(repo.metadata.version)
                    installed_version_string += ".</b>"
                elif repo.macro and repo.macro.version:
                    installed_version_string += (
                        "<b>" + translate("AddonsInstaller", "Update available to version") + " "
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
            elif status == Addon.Status.NO_UPDATE_AVAILABLE:
                detached_head = False
                branch = repo.branch
                if have_git and repo.repo_type != Addon.Kind.MACRO:
                    basedir = fci.getUserAppDataDir()
                    moddir = os.path.join(basedir, "Mod", repo.name)
                    if os.path.exists(os.path.join(moddir, ".git")):
                        gitrepo = git.Repo(moddir)
                        branch = gitrepo.head.ref.name
                        detached_head = gitrepo.head.is_detached

                if detached_head:
                    installed_version_string += (
                        translate(
                            "AddonsInstaller",
                            "Git tag '{}' checked out, no updates possible",
                        ).format(branch)
                        + "."
                    )
                else:
                    installed_version_string += (
                        translate(
                            "AddonsInstaller",
                            "This is the latest version available for branch {}",
                        ).format(branch)
                        + "."
                    )
            elif status == Addon.Status.PENDING_RESTART:
                installed_version_string += (
                    translate("AddonsInstaller", "Updated, please restart FreeCAD to use") + "."
                )
            elif status == Addon.Status.UNCHECKED:
                pref = fci.ParamGet("User parameter:BaseApp/Preferences/Addons")
                autocheck = pref.GetBool("AutoCheck", False)
                if autocheck:
                    installed_version_string += (
                        translate("AddonsInstaller", "Update check in progress") + "."
                    )
                else:
                    installed_version_string += (
                        translate("AddonsInstaller", "Automatic update checks disabled") + "."
                    )

            installed_version_string += "</h3>"
            self.ui.labelPackageDetails.setText(installed_version_string)
            if repo.status() == Addon.Status.UPDATE_AVAILABLE:
                self.ui.labelPackageDetails.setStyleSheet("color:" + utils.attention_color_string())
            else:
                self.ui.labelPackageDetails.setStyleSheet("color:" + utils.bright_color_string())
            self.ui.labelPackageDetails.show()

            if repo.macro is not None:
                moddir = fci.getUserMacroDir(True)
            else:
                basedir = fci.getUserAppDataDir()
                moddir = os.path.join(basedir, "Mod", repo.name)
            installationLocationString = (
                translate("AddonsInstaller", "Installation location")
                + ": "
                + os.path.normpath(moddir)
            )

            self.ui.labelInstallationLocation.setText(installationLocationString)
            self.ui.labelInstallationLocation.show()
        else:
            self.ui.labelPackageDetails.hide()
            self.ui.labelInstallationLocation.hide()

        if status == Addon.Status.NOT_INSTALLED:
            self.ui.buttonInstall.show()
            self.ui.buttonUninstall.hide()
            self.ui.buttonUpdate.hide()
            self.ui.buttonCheckForUpdate.hide()
        elif status == Addon.Status.NO_UPDATE_AVAILABLE:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.hide()
            self.ui.buttonCheckForUpdate.hide()
        elif status == Addon.Status.UPDATE_AVAILABLE:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.show()
            self.ui.buttonCheckForUpdate.hide()
        elif status == Addon.Status.UNCHECKED:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.hide()
            self.ui.buttonCheckForUpdate.show()
        elif status == Addon.Status.PENDING_RESTART:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.hide()
            self.ui.buttonCheckForUpdate.hide()
        elif status == Addon.Status.CANNOT_CHECK:
            self.ui.buttonInstall.hide()
            self.ui.buttonUninstall.show()
            self.ui.buttonUpdate.show()
            self.ui.buttonCheckForUpdate.hide()

        required_version = self.requires_newer_freecad()
        if repo.obsolete:
            self.ui.labelWarningInfo.show()
            self.ui.labelWarningInfo.setText(
                "<h1>" + translate("AddonsInstaller", "WARNING: This addon is obsolete") + "</h1>"
            )
            self.ui.labelWarningInfo.setStyleSheet("color:" + utils.warning_color_string())
        elif repo.python2:
            self.ui.labelWarningInfo.show()
            self.ui.labelWarningInfo.setText(
                "<h1>"
                + translate("AddonsInstaller", "WARNING: This addon is Python 2 Only")
                + "</h1>"
            )
            self.ui.labelWarningInfo.setStyleSheet("color:" + utils.warning_color_string())
        elif required_version:
            self.ui.labelWarningInfo.show()
            self.ui.labelWarningInfo.setText(
                "<h1>"
                + translate("AddonsInstaller", "WARNING: This addon requires FreeCAD ")
                + required_version
                + "</h1>"
            )
            self.ui.labelWarningInfo.setStyleSheet("color:" + utils.warning_color_string())
        elif repo.is_disabled():
            self.ui.labelWarningInfo.show()
            self.ui.labelWarningInfo.setText(
                "<h2>"
                + translate(
                    "AddonsInstaller",
                    "WARNING: This addon is currently installed, but disabled. Use the 'enable' button to re-enable.",
                )
                + "</h2>"
            )
            self.ui.labelWarningInfo.setStyleSheet("color:" + utils.warning_color_string())

        else:
            self.ui.labelWarningInfo.hide()

    def requires_newer_freecad(self) -> Optional[Version]:
        """If the current package is not installed, returns the first supported version of
        FreeCAD, if one is set, or None if no information is available (or if the package is
        already installed)."""

        # If it's not installed, check to see if it's for a newer version of FreeCAD
        if self.repo.status() == Addon.Status.NOT_INSTALLED and self.repo.metadata:
            # Only hide if ALL content items require a newer version, otherwise
            # it's possible that this package actually provides versions of itself
            # for newer and older versions

            first_supported_version = get_first_supported_freecad_version(self.repo.metadata)
            if first_supported_version is not None:
                fc_version = Version(from_list=fci.Version())
                if first_supported_version > fc_version:
                    return first_supported_version
        return None

    def set_change_branch_button_state(self):
        """The change branch button is only available for installed Addons that have a .git directory
        and in runs where the git is available."""

        self.ui.buttonChangeBranch.hide()

        pref = fci.ParamGet("User parameter:BaseApp/Preferences/Addons")
        show_switcher = pref.GetBool("ShowBranchSwitcher", False)
        if not show_switcher:
            return

        # Is this repo installed? If not, return.
        if self.repo.status() == Addon.Status.NOT_INSTALLED:
            return

        # Is it a Macro? If so, return:
        if self.repo.repo_type == Addon.Kind.MACRO:
            return

        # Can we actually switch branches? If not, return.
        if not have_git:
            return

        # Is there a .git subdirectory? If not, return.
        basedir = fci.getUserAppDataDir()
        path_to_git = os.path.join(basedir, "Mod", self.repo.name, ".git")
        if not os.path.isdir(path_to_git):
            return

        # If all four above checks passed, then it's possible for us to switch
        # branches, if there are any besides the one we are on: show the button
        self.ui.buttonChangeBranch.show()

    def set_disable_button_state(self):
        """Set up the enable/disable button based on the enabled/disabled state of the addon"""
        self.ui.buttonEnable.hide()
        self.ui.buttonDisable.hide()
        status = self.repo.status()
        if status != Addon.Status.NOT_INSTALLED:
            disabled = self.repo.is_disabled()
            if disabled:
                self.ui.buttonEnable.show()
            else:
                self.ui.buttonDisable.show()

    def show_workbench(self, repo: Addon) -> None:
        """loads information of a given workbench"""
        url = utils.get_readme_html_url(repo)
        if HAS_QTWEBENGINE:
            self.ui.webView.load(QtCore.QUrl(url))
            self.ui.urlBar.setText(url)
        else:
            readme_data = utils.blocking_get(url)
            text = readme_data.decode("utf8")
            self.ui.textBrowserReadMe.setHtml(text)

    def show_package(self, repo: Addon) -> None:
        """Show the details for a package (a repo with a package.xml metadata file)"""

        readme_url = None
        if repo.metadata:
            for url in repo.metadata.url:
                if url.type == UrlType.readme:
                    readme_url = url.location
                    break
        if not readme_url:
            readme_url = utils.get_readme_html_url(repo)
        if HAS_QTWEBENGINE:
            self.ui.webView.load(QtCore.QUrl(readme_url))
            self.ui.urlBar.setText(readme_url)
        else:
            readme_data = utils.blocking_get(readme_url)
            text = readme_data.decode("utf8")
            self.ui.textBrowserReadMe.setHtml(text)

    def show_macro(self, repo: Addon) -> None:
        """loads information of a given macro"""

        if not repo.macro.url:
            # We need to populate the macro information... may as well do it while the user reads the wiki page
            self.worker = GetMacroDetailsWorker(repo)
            self.worker.readme_updated.connect(self.macro_readme_updated)
            self.worker.start()
        else:
            self.macro_readme_updated()

    def macro_readme_updated(self):
        """Update the display of a Macro's README data."""
        url = self.repo.macro.wiki
        if not url:
            url = self.repo.macro.url

        if HAS_QTWEBENGINE:
            if url:
                self.ui.webView.load(QtCore.QUrl(url))
                self.ui.urlBar.setText(url)
            else:
                self.ui.urlBar.setText(
                    "("
                    + translate("AddonsInstaller", "No URL or wiki page provided by this macro")
                    + ")"
                )
        else:
            if url:
                readme_data = utils.blocking_get(url)
                text = readme_data.decode("utf8")
                self.ui.textBrowserReadMe.setHtml(text)
            else:
                self.ui.textBrowserReadMe.setHtml(
                    "("
                    + translate("AddonsInstaller", "No URL or wiki page provided by this macro")
                    + ")"
                )

    def run_javascript(self):
        """Modify the page for a README to optimize for viewing in a smaller window"""

        s = """
( function() {
    const url = new URL (window.location);
    const body = document.getElementsByTagName("body")[0];
    if (url.hostname === "github.com") {
        const articles = document.getElementsByTagName("article");
        if (articles.length > 0) {
            const article = articles[0];
            body.appendChild (article);
            body.style.padding = "1em";
            let sibling = article.previousSibling;
            while (sibling) {
                sibling.remove();
                sibling = article.previousSibling;
            }
        }
    } else if (url.hostname === "gitlab.com" ||
               url.hostname === "framagit.org" ||
               url.hostname === "salsa.debian.org") {
        // These all use the GitLab page display...
        const articles = document.getElementsByTagName("article");
        if (articles.length > 0) {
            const article = articles[0];
            body.appendChild (article);
            body.style.padding = "1em";
            let sibling = article.previousSibling;
            while (sibling) {
                sibling.remove();
                sibling = article.previousSibling;
            }
        }
    } else if (url.hostname === "wiki.freecad.org" ||
               url.hostname === "wiki.freecad.org") {
        const first_heading = document.getElementById('firstHeading');
        const body_content = document.getElementById('bodyContent');
        const new_node = document.createElement("div");
        new_node.appendChild(first_heading);
        new_node.appendChild(body_content);
        body.appendChild(new_node);
        let sibling = new_node.previousSibling;
        while (sibling) {
            sibling.remove();
            sibling = new_node.previousSibling;
        }
    }
}
) ()
"""
        self.ui.webView.page().runJavaScript(s)

    def load_started(self):
        """Called when loading is started: sets up the progress bar"""
        self.ui.progressBar.show()
        self.ui.progressBar.setValue(0)

    def load_progress(self, progress: int):
        """Called during load to update the progress bar"""
        self.ui.progressBar.setValue(progress)

    def load_finished(self, load_succeeded: bool):
        """Once loading is complete, update the display of the progress bar and loading widget."""
        self.ui.loadingLabel.hide()
        self.ui.slowLoadLabel.hide()
        self.ui.webView.show()
        self.ui.progressBar.hide()
        url = self.ui.webView.url()
        if (
            hasattr(self, "timeout")
            and hasattr(self.timeout, "isActive")
            and self.timeout.isActive()
        ):
            self.timeout.stop()
        if load_succeeded:
            # It says it succeeded, but it might have only succeeded in loading a
            # "Page not found" page!
            title = self.ui.webView.title()
            path_components = url.path().split("/")
            expected_content = path_components[-1]
            if url.host() == "github.com" and expected_content not in title:
                self.show_error_for(url)
            elif title == "":
                self.show_error_for(url)
            else:
                self.run_javascript()
        else:
            self.show_error_for(url)

    def long_load_running(self):
        """Displays a message about loading taking a long time."""
        if hasattr(self.ui, "webView") and self.ui.webView.isHidden():
            self.ui.slowLoadLabel.show()
            self.ui.loadingLabel.hide()
            self.ui.webView.show()

    def show_error_for(self, url: QtCore.QUrl) -> None:
        """Displays error information."""
        m = translate("AddonsInstaller", "Could not load README data from URL {}").format(
            url.toString()
        )
        html = f"<html><body><p>{m}</p></body></html>"
        self.ui.webView.setHtml(html)

    def change_branch_clicked(self) -> None:
        """Loads the branch-switching dialog"""
        basedir = fci.getUserAppDataDir()
        path_to_repo = os.path.join(basedir, "Mod", self.repo.name)
        change_branch_dialog = ChangeBranchDialog(path_to_repo, self)
        change_branch_dialog.branch_changed.connect(self.branch_changed)
        change_branch_dialog.exec()

    def enable_clicked(self) -> None:
        """Called by the Enable button, enables this Addon and updates GUI to reflect
        that status."""
        self.repo.enable()
        self.repo.set_status(Addon.Status.PENDING_RESTART)
        self.set_disable_button_state()
        self.update_status.emit(self.repo)
        self.ui.labelWarningInfo.show()
        self.ui.labelWarningInfo.setText(
            "<h3>"
            + translate(
                "AddonsInstaller",
                "This Addon will be enabled next time you restart FreeCAD.",
            )
            + "</h3>"
        )
        self.ui.labelWarningInfo.setStyleSheet("color:" + utils.bright_color_string())

    def disable_clicked(self) -> None:
        """Called by the Disable button, disables this Addon and updates the GUI to
        reflect that status."""
        self.repo.disable()
        self.repo.set_status(Addon.Status.PENDING_RESTART)
        self.set_disable_button_state()
        self.update_status.emit(self.repo)
        self.ui.labelWarningInfo.show()
        self.ui.labelWarningInfo.setText(
            "<h3>"
            + translate(
                "AddonsInstaller",
                "This Addon will be disabled next time you restart FreeCAD.",
            )
            + "</h3>"
        )
        self.ui.labelWarningInfo.setStyleSheet("color:" + utils.attention_color_string())

    def branch_changed(self, name: str) -> None:
        """Displays a dialog confirming the branch changed, and tries to access the
        metadata file from that branch."""
        QtWidgets.QMessageBox.information(
            self,
            translate("AddonsInstaller", "Success"),
            translate(
                "AddonsInstaller",
                "Branch change succeeded, please restart to use the new version.",
            ),
        )
        # See if this branch has a package.xml file:
        basedir = fci.getUserAppDataDir()
        path_to_metadata = os.path.join(basedir, "Mod", self.repo.name, "package.xml")
        if os.path.isfile(path_to_metadata):
            self.repo.load_metadata_file(path_to_metadata)
            self.repo.installed_version = self.repo.metadata.version
        else:
            self.repo.repo_type = Addon.Kind.WORKBENCH
            self.repo.metadata = None
            self.repo.installed_version = None
        self.repo.updated_timestamp = QtCore.QDateTime.currentDateTime().toSecsSinceEpoch()
        self.repo.branch = name
        self.repo.set_status(Addon.Status.PENDING_RESTART)

        installed_version_string = "<h3>"
        installed_version_string += translate(
            "AddonsInstaller", "Changed to git ref '{}' -- please restart to use Addon."
        ).format(name)
        installed_version_string += "</h3>"
        self.ui.labelPackageDetails.setText(installed_version_string)
        self.ui.labelPackageDetails.setStyleSheet("color:" + utils.attention_color_string())
        self.update_status.emit(self.repo)


if HAS_QTWEBENGINE:

    class RestrictedWebPage(QtWebEngineWidgets.QWebEnginePage):
        """A class that follows links to FreeCAD wiki pages, but opens all other
        clicked links in the system web browser"""

        def __init__(self, parent):
            super().__init__(parent)
            self.settings().setAttribute(
                QtWebEngineWidgets.QWebEngineSettings.ErrorPageEnabled, False
            )
            self.stored_url = None

        def acceptNavigationRequest(self, requested_url, _type, isMainFrame):
            """A callback for navigation requests: this widget will only display
            navigation requests to the FreeCAD Wiki (for translation purposes) --
            anything else will open in a new window.
            """
            if _type == QtWebEngineWidgets.QWebEnginePage.NavigationTypeLinkClicked:
                # See if the link is to a FreeCAD Wiki page -- if so, follow it,
                # otherwise ask the OS to open it
                if (
                    requested_url.host() == "wiki.freecad.org"
                    or requested_url.host() == "wiki.freecad.org"
                ):
                    return super().acceptNavigationRequest(requested_url, _type, isMainFrame)
                QtGui.QDesktopServices.openUrl(requested_url)
                self.stored_url = self.url()
                QtCore.QTimer.singleShot(0, self._reload_stored_url)
                return False
            return super().acceptNavigationRequest(requested_url, _type, isMainFrame)

        def javaScriptConsoleMessage(self, level, message, lineNumber, _):
            """Handle JavaScript console messages by optionally outputting them to
            the FreeCAD Console. This must be manually enabled in this Python file by
            setting the global show_javascript_console_output to true."""
            global show_javascript_console_output
            if show_javascript_console_output:
                tag = translate("AddonsInstaller", "Page JavaScript reported")
                if level == QtWebEngineWidgets.QWebEnginePage.InfoMessageLevel:
                    fci.Console.PrintMessage(f"{tag} {lineNumber}: {message}\n")
                elif level == QtWebEngineWidgets.QWebEnginePage.WarningMessageLevel:
                    fci.Console.PrintWarning(f"{tag} {lineNumber}: {message}\n")
                elif level == QtWebEngineWidgets.QWebEnginePage.ErrorMessageLevel:
                    fci.Console.PrintError(f"{tag} {lineNumber}: {message}\n")

        def _reload_stored_url(self):
            if self.stored_url:
                self.load(self.stored_url)


class Ui_PackageDetails(object):
    """The generated UI from the Qt Designer UI file"""

    def setupUi(self, PackageDetails):
        if not PackageDetails.objectName():
            PackageDetails.setObjectName("PackageDetails")
        self.verticalLayout_2 = QtWidgets.QVBoxLayout(PackageDetails)
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        self.layoutDetailsBackButton = QtWidgets.QHBoxLayout()
        self.layoutDetailsBackButton.setObjectName("layoutDetailsBackButton")
        self.buttonBack = QtWidgets.QToolButton(PackageDetails)
        self.buttonBack.setObjectName("buttonBack")
        self.buttonBack.setIcon(
            QtGui.QIcon.fromTheme("back", QtGui.QIcon(":/icons/button_left.svg"))
        )

        self.layoutDetailsBackButton.addWidget(self.buttonBack)

        self.horizontalSpacer = QtWidgets.QSpacerItem(
            40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum
        )

        self.layoutDetailsBackButton.addItem(self.horizontalSpacer)

        self.buttonInstall = QtWidgets.QPushButton(PackageDetails)
        self.buttonInstall.setObjectName("buttonInstall")

        self.layoutDetailsBackButton.addWidget(self.buttonInstall)

        self.buttonUninstall = QtWidgets.QPushButton(PackageDetails)
        self.buttonUninstall.setObjectName("buttonUninstall")

        self.layoutDetailsBackButton.addWidget(self.buttonUninstall)

        self.buttonUpdate = QtWidgets.QPushButton(PackageDetails)
        self.buttonUpdate.setObjectName("buttonUpdate")

        self.layoutDetailsBackButton.addWidget(self.buttonUpdate)

        self.buttonCheckForUpdate = QtWidgets.QPushButton(PackageDetails)
        self.buttonCheckForUpdate.setObjectName("buttonCheckForUpdate")

        self.layoutDetailsBackButton.addWidget(self.buttonCheckForUpdate)

        self.buttonChangeBranch = QtWidgets.QPushButton(PackageDetails)
        self.buttonChangeBranch.setObjectName("buttonChangeBranch")

        self.layoutDetailsBackButton.addWidget(self.buttonChangeBranch)

        self.buttonExecute = QtWidgets.QPushButton(PackageDetails)
        self.buttonExecute.setObjectName("buttonExecute")

        self.layoutDetailsBackButton.addWidget(self.buttonExecute)

        self.buttonDisable = QtWidgets.QPushButton(PackageDetails)
        self.buttonDisable.setObjectName("buttonDisable")

        self.layoutDetailsBackButton.addWidget(self.buttonDisable)

        self.buttonEnable = QtWidgets.QPushButton(PackageDetails)
        self.buttonEnable.setObjectName("buttonEnable")

        self.layoutDetailsBackButton.addWidget(self.buttonEnable)

        self.verticalLayout_2.addLayout(self.layoutDetailsBackButton)

        self.labelPackageDetails = QtWidgets.QLabel(PackageDetails)
        self.labelPackageDetails.hide()

        self.verticalLayout_2.addWidget(self.labelPackageDetails)

        self.labelInstallationLocation = QtWidgets.QLabel(PackageDetails)
        self.labelInstallationLocation.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)
        self.labelInstallationLocation.hide()

        self.verticalLayout_2.addWidget(self.labelInstallationLocation)

        self.labelWarningInfo = QtWidgets.QLabel(PackageDetails)
        self.labelWarningInfo.hide()

        self.verticalLayout_2.addWidget(self.labelWarningInfo)

        sizePolicy1 = QtWidgets.QSizePolicy(
            QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding
        )
        sizePolicy1.setHorizontalStretch(0)
        sizePolicy1.setVerticalStretch(0)

        if HAS_QTWEBENGINE:
            self.webView = QtWebEngineWidgets.QWebEngineView(PackageDetails)
            self.webView.setObjectName("webView")
            self.webView.setSizePolicy(sizePolicy1)
            self.webView.setPage(RestrictedWebPage(PackageDetails))

            self.verticalLayout_2.addWidget(self.webView)

            self.loadingLabel = QtWebEngineWidgets.QWebEngineView(PackageDetails)
            self.loadingLabel.setObjectName("loadingLabel")
            self.loadingLabel.setSizePolicy(sizePolicy1)

            self.verticalLayout_2.addWidget(self.loadingLabel)

            self.slowLoadLabel = QtWidgets.QLabel(PackageDetails)
            self.slowLoadLabel.setObjectName("slowLoadLabel")

            self.verticalLayout_2.addWidget(self.slowLoadLabel)

            self.progressBar = QtWidgets.QProgressBar(PackageDetails)
            self.progressBar.setObjectName("progressBar")
            self.progressBar.setTextVisible(False)

            self.verticalLayout_2.addWidget(self.progressBar)

            self.urlBar = QtWidgets.QLineEdit(PackageDetails)
            self.urlBar.setObjectName("urlBar")
            self.urlBar.setReadOnly(True)

            self.verticalLayout_2.addWidget(self.urlBar)
        else:
            self.missingWebViewLabel = QtWidgets.QLabel(PackageDetails)
            self.missingWebViewLabel.setObjectName("missingWebViewLabel")
            self.missingWebViewLabel.setWordWrap(True)
            self.verticalLayout_2.addWidget(self.missingWebViewLabel)

            self.textBrowserReadMe = QtWidgets.QTextBrowser(PackageDetails)
            self.textBrowserReadMe.setObjectName("textBrowserReadMe")
            self.textBrowserReadMe.setOpenExternalLinks(True)
            self.textBrowserReadMe.setOpenLinks(True)

            self.verticalLayout_2.addWidget(self.textBrowserReadMe)

        self.retranslateUi(PackageDetails)

        QtCore.QMetaObject.connectSlotsByName(PackageDetails)

    # setupUi

    def retranslateUi(self, _):
        self.buttonBack.setText("")
        self.buttonInstall.setText(
            QtCore.QCoreApplication.translate("AddonsInstaller", "Install", None)
        )
        self.buttonUninstall.setText(
            QtCore.QCoreApplication.translate("AddonsInstaller", "Uninstall", None)
        )
        self.buttonUpdate.setText(
            QtCore.QCoreApplication.translate("AddonsInstaller", "Update", None)
        )
        self.buttonCheckForUpdate.setText(
            QtCore.QCoreApplication.translate("AddonsInstaller", "Check for Update", None)
        )
        self.buttonExecute.setText(
            QtCore.QCoreApplication.translate("AddonsInstaller", "Run Macro", None)
        )
        self.buttonChangeBranch.setText(
            QtCore.QCoreApplication.translate("AddonsInstaller", "Change Branch", None)
        )
        self.buttonEnable.setText(
            QtCore.QCoreApplication.translate("AddonsInstaller", "Enable", None)
        )
        self.buttonDisable.setText(
            QtCore.QCoreApplication.translate("AddonsInstaller", "Disable", None)
        )
        self.buttonBack.setToolTip(
            QtCore.QCoreApplication.translate("AddonsInstaller", "Return to package list", None)
        )
        if not HAS_QTWEBENGINE:
            self.missingWebViewLabel.setText(
                "<h3>"
                + QtCore.QCoreApplication.translate(
                    "AddonsInstaller",
                    "QtWebEngine Python bindings not installed -- using fallback README display.",
                    None,
                )
                + "</h3>"
            )
        else:
            self.slowLoadLabel.setText(
                QtCore.QCoreApplication.translate(
                    "AddonsInstaller",
                    "The page is taking a long time to load... showing the data we have so far...",
                )
            )

    # retranslateUi
