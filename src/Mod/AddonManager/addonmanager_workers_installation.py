# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Lesser General Public            *
# *   License as published by the Free Software Foundation; either          *
# *   version 2.1 of the License, or (at your option) any later version.    *
# *                                                                         *
# *   This library is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with this library; if not, write to the Free Software   *
# *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
# *   02110-1301  USA                                                       *
# *                                                                         *
# ***************************************************************************

""" Worker thread classes for Addon Manager installation and removal """

# pylint: disable=c-extension-no-member,too-few-public-methods

import io
import os
import queue
import shutil
import subprocess
import time
import zipfile
from typing import Dict, List
from enum import Enum, auto

from PySide2 import QtCore

import FreeCAD
import addonmanager_utilities as utils
from Addon import Addon
import NetworkManager
from addonmanager_git import initialize_git

translate = FreeCAD.Qt.translate

#  @package AddonManager_workers
#  \ingroup ADDONMANAGER
#  \brief Multithread workers for the addon manager
#  @{


class InstallWorkbenchWorker(QtCore.QThread):
    "This worker installs a workbench"

    status_message = QtCore.Signal(str)
    progress_made = QtCore.Signal(int, int)
    success = QtCore.Signal(Addon, str)
    failure = QtCore.Signal(Addon, str)

    def __init__(self, repo: Addon, location=None):

        QtCore.QThread.__init__(self)
        self.repo = repo
        self.update_timer = QtCore.QTimer()
        self.update_timer.setInterval(100)
        self.update_timer.timeout.connect(self.update_status)
        self.update_timer.start()

        if location:
            self.clone_directory = location
        else:
            basedir = FreeCAD.getUserAppDataDir()
            self.clone_directory = os.path.join(basedir, "Mod", repo.name)

        if not os.path.exists(self.clone_directory):
            os.makedirs(self.clone_directory)

        self.git_manager = initialize_git()

        # Some stored data for the ZIP processing
        self.zip_complete = False
        self.zipdir = None
        self.bakdir = None
        self.zip_download_index = None

    def run(self):
        """Normally not called directly: instead, create an instance of this worker class and
        call start() on it to launch in a new thread. Installs or updates the selected addon"""

        if not self.repo:
            return

        if not self.git_manager:
            FreeCAD.Console.PrintLog(
                translate(
                    "AddonsInstaller",
                    "Git disabled - using ZIP file download instead.",
                )
                + "\n"
            )

        target_dir = self.clone_directory

        if self.git_manager:
            # Do the git process...
            self.run_git(target_dir)
        else:

            # The zip process uses an event loop, since the download can potentially be quite large
            self.launch_zip(target_dir)
            self.zip_complete = False
            current_thread = QtCore.QThread.currentThread()
            while not self.zip_complete:
                if current_thread.isInterruptionRequested():
                    return
                QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)

        self.repo.set_status(Addon.Status.PENDING_RESTART)

    def update_status(self) -> None:
        """Periodically emit the progress of the git download, for asynchronous operations"""
        if hasattr(self, "git_progress") and self.isRunning():
            self.progress_made.emit(self.git_progress.current, self.git_progress.total)
            self.status_message.emit(self.git_progress.message)

    def run_git(self, clonedir: str) -> None:
        """Clone or update the addon using git. Exits if git is disabled."""

        if not self.git_manager:
            FreeCAD.Console.PrintLog(
                translate(
                    "AddonsInstaller",
                    "Git disabled, skipping git operations",
                )
                + "\n"
            )
            return

        if os.path.exists(clonedir):
            self.run_git_update(clonedir)
        else:
            self.run_git_clone(clonedir)

    def run_git_update(self, clonedir: str) -> None:
        """Runs git update operation: normally a fetch and pull, but if something goew wrong it
        will revert to a clean clone."""
        self.status_message.emit("Updating module...")
        with self.repo.git_lock:
            if not os.path.exists(clonedir + os.sep + ".git"):
                self.git_manager.repair(self.repo.url, clonedir)
            try:
                self.git_manager.update(clonedir)
                if self.repo.contains_workbench():
                    # pylint: disable=line-too-long
                    answer = translate(
                        "AddonsInstaller",
                        "Workbench successfully updated. Please restart FreeCAD to apply the changes.",
                    )
                else:
                    answer = translate(
                        "AddonsInstaller",
                        "Workbench successfully updated.",
                    )
            except GitFailed as e:
                answer = (
                    translate("AddonsInstaller", "Error updating module")
                    + " "
                    + self.repo.name
                    + " - "
                    + translate("AddonsInstaller", "Please fix manually")
                    + " -- \n"
                )
                answer += str(e)
                self.failure.emit(self.repo, answer)
            self.update_metadata()
            self.success.emit(self.repo, answer)

    def run_git_clone(self, clonedir: str) -> None:
        """Clones a repo using git"""
        self.status_message.emit("Cloning module...")
        current_thread = QtCore.QThread.currentThread()

        FreeCAD.Console.PrintMessage("Cloning repo...\n")
        if self.repo.git_lock.locked():
            FreeCAD.Console.PrintMessage("Waiting for lock to be released to us...\n")
            if not self.repo.git_lock.acquire(timeout=2):
                FreeCAD.Console.PrintError(
                    "Timeout waiting for a lock on the git process, failed to clone repo\n"
                )
                return
            self.repo.git_lock.release()

        with self.repo.git_lock:
            FreeCAD.Console.PrintMessage("Lock acquired...\n")
            self.git_manager.clone(self.repo.url, clonedir)
            FreeCAD.Console.PrintMessage("Initial clone complete...\n")
            if current_thread.isInterruptionRequested():
                return

            if current_thread.isInterruptionRequested():
                return

            FreeCAD.Console.PrintMessage("Clone complete\n")

        if self.repo.contains_workbench():
            answer = translate(
                "AddonsInstaller",
                "Workbench successfully installed. Please restart FreeCAD to apply the changes.",
            )
        else:
            answer = translate(
                "AddonsInstaller",
                "Addon successfully installed.",
            )

        if self.repo.repo_type == Addon.Kind.WORKBENCH:
            # symlink any macro contained in the module to the macros folder
            macro_dir = FreeCAD.getUserMacroDir(True)
            if not os.path.exists(macro_dir):
                os.makedirs(macro_dir)
            if os.path.exists(clonedir):
                for f in os.listdir(clonedir):
                    if f.lower().endswith(".fcmacro"):
                        try:
                            utils.symlink(
                                os.path.join(clonedir, f), os.path.join(macro_dir, f)
                            )
                        except OSError:
                            # If the symlink failed (e.g. for a non-admin user on Windows), copy
                            # the macro instead
                            shutil.copy(
                                os.path.join(clonedir, f), os.path.join(macro_dir, f)
                            )
                        FreeCAD.ParamGet(
                            "User parameter:Plugins/" + self.repo.name
                        ).SetString("destination", clonedir)
                        # pylint: disable=line-too-long
                        answer += "\n\n" + translate(
                            "AddonsInstaller",
                            "A macro has been installed and is available under Macro -> Macros menu",
                        )
                        answer += ":\n<b>" + f + "</b>"
        self.update_metadata()
        self.success.emit(self.repo, answer)

    def launch_zip(self, zipdir: str) -> None:
        """Downloads and unzip a zip version from a git repo"""

        bakdir = None
        if os.path.exists(zipdir):
            bakdir = zipdir + ".bak"
            if os.path.exists(bakdir):
                shutil.rmtree(bakdir)
            os.rename(zipdir, bakdir)
        os.makedirs(zipdir)
        zipurl = utils.get_zip_url(self.repo)
        if not zipurl:
            self.failure.emit(
                self.repo,
                translate("AddonsInstaller", "Error: Unable to locate ZIP from")
                + " "
                + self.repo.name,
            )
            return

        self.zipdir = zipdir
        self.bakdir = bakdir

        NetworkManager.AM_NETWORK_MANAGER.progress_made.connect(self.update_zip_status)
        NetworkManager.AM_NETWORK_MANAGER.progress_complete.connect(self.finish_zip)
        self.zip_download_index = (
            NetworkManager.AM_NETWORK_MANAGER.submit_monitored_get(zipurl)
        )

    def update_zip_status(self, index: int, bytes_read: int, data_size: int):
        """Called periodically when downloading a zip file, emits a signal to display the
        download progress."""
        if index == self.zip_download_index:
            locale = QtCore.QLocale()
            if data_size > 10 * 1024 * 1024:  # To avoid overflows, show MB instead
                MB_read = bytes_read / 1024 / 1024
                MB_total = data_size / 1024 / 1024
                self.progress_made.emit(MB_read, MB_total)
                mbytes_str = locale.toString(MB_read)
                mbytes_total_str = locale.toString(MB_total)
                percent = int(100 * float(MB_read / MB_total))
                self.status_message.emit(
                    translate(
                        "AddonsInstaller",
                        "Downloading: {mbytes_str}MB of {mbytes_total_str}MB ({percent}%)",
                    ).format(
                        mbytes_str=mbytes_str,
                        mbytes_total_str=mbytes_total_str,
                        percent=percent,
                    )
                )
            elif data_size > 0:
                self.progress_made.emit(bytes_read, data_size)
                bytes_str = locale.toString(bytes_read)
                bytes_total_str = locale.toString(data_size)
                percent = int(100 * float(bytes_read / data_size))
                self.status_message.emit(
                    translate(
                        "AddonsInstaller",
                        "Downloading: {bytes_str} of {bytes_total_str} bytes ({percent}%)",
                    ).format(
                        bytes_str=bytes_str,
                        bytes_total_str=bytes_total_str,
                        percent=percent,
                    )
                )
            else:
                MB_read = bytes_read / 1024 / 1024
                bytes_str = locale.toString(MB_read)
                self.status_message.emit(
                    translate(
                        "AddonsInstaller",
                        "Downloading: {bytes_str}MB of unknown total",
                    ).format(bytes_str=bytes_str)
                )

    def finish_zip(self, _index: int, response_code: int, filename: os.PathLike):
        """Once the zip download is finished, unzip it into the correct location."""
        self.zip_complete = True
        if response_code != 200:
            self.failure.emit(
                self.repo,
                translate(
                    "AddonsInstaller",
                    "Error: Error while downloading ZIP file for {}",
                ).format(self.repo.display_name),
            )
            return

        with zipfile.ZipFile(filename, "r") as zfile:
            master = zfile.namelist()[0]  # github will put everything in a subfolder
            self.status_message.emit(
                translate("AddonsInstaller", "Download complete. Unzipping file...")
            )
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)
            zfile.extractall(self.zipdir)
        for extracted_filename in os.listdir(self.zipdir + os.sep + master):
            shutil.move(
                self.zipdir + os.sep + master + os.sep + extracted_filename,
                self.zipdir + os.sep + extracted_filename,
            )
        os.rmdir(self.zipdir + os.sep + master)
        if self.bakdir:
            shutil.rmtree(self.bakdir)
        self.update_metadata()
        self.success.emit(
            self.repo,
            translate(
                "AddonsInstaller",
                "Successfully installed {} from ZIP file",
            ).format(self.repo.display_name),
        )

    def update_metadata(self):
        """Loads the package metadata from the Addon's downloaded package.xml file."""
        basedir = FreeCAD.getUserAppDataDir()
        package_xml = os.path.join(basedir, "Mod", self.repo.name, "package.xml")
        if os.path.isfile(package_xml):
            self.repo.load_metadata_file(package_xml)
            self.repo.installed_version = self.repo.metadata.Version
            self.repo.updated_timestamp = os.path.getmtime(package_xml)


class DependencyInstallationWorker(QtCore.QThread):
    """Install dependencies using Addonmanager for FreeCAD, and pip for python"""

    no_python_exe = QtCore.Signal()
    no_pip = QtCore.Signal(str)  # Attempted command
    failure = QtCore.Signal(str, str)  # Short message, detailed message
    success = QtCore.Signal()

    def __init__(
        self,
        addons: List[Addon],
        python_required: List[str],
        python_optional: List[str],
        location: os.PathLike = None,
    ):
        """Install the various types of dependencies that might be specified. If an optional
         dependency fails this is non-fatal, but other failures are considered fatal. If location
        is specified it overrides the FreeCAD user base directory setting: this is used mostly
        for testing purposes and shouldn't be set by normal code in most circumstances."""
        QtCore.QThread.__init__(self)
        self.addons = addons
        self.python_required = python_required
        self.python_optional = python_optional
        self.location = location

    def run(self):
        """Normally not called directly: create the object and call start() to launch it
        in its own thread. Installs dependencies for the Addon."""
        self._install_required_addons()
        if self.python_required or self.python_optional:
            self._install_python_packages()
        self.success.emit()

    def _install_required_addons(self):
        """Install whatever FreeCAD Addons were set as required."""
        for repo in self.addons:
            if QtCore.QThread.currentThread().isInterruptionRequested():
                return
            location = self.location
            if location:
                location = os.path.join(location, "Mod")
            worker = InstallWorkbenchWorker(repo, location=location)
            worker.start()
            while worker.isRunning():
                if QtCore.QThread.currentThread().isInterruptionRequested():
                    worker.requestInterruption()
                    worker.wait()
                    return
                time.sleep(0.1)
                QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)

    def _install_python_packages(self):
        """Install required and optional Python dependencies using pip."""
        if not self._verify_pip():
            return

        if self.location:
            vendor_path = os.path.join(self.location, "AdditionalPythonPackages")
        else:
            vendor_path = os.path.join(
                FreeCAD.getUserAppDataDir(), "AdditionalPythonPackages"
            )
        if not os.path.exists(vendor_path):
            os.makedirs(vendor_path)

        self._install_required(vendor_path)
        self._install_optional(vendor_path)

    def _verify_pip(self) -> bool:
        """Ensure that pip is working -- returns True if it is, or False if not. Also emits the
        no_pip signal if pip cannot execute."""
        python_exe = utils.get_python_exe()
        pip_failed = False
        if python_exe:
            try:
                proc = subprocess.run(
                    [python_exe, "-m", "pip", "--version"],
                    stdout=subprocess.PIPE,
                    check=True,
                )
            except subprocess.CalledProcessError:
                pip_failed = True
            if proc.returncode != 0:
                pip_failed = True
        else:
            pip_failed = True
        if pip_failed:
            self.no_pip.emit(f"{python_exe} -m pip --version")
        FreeCAD.Console.PrintMessage(proc.stdout)
        FreeCAD.Console.PrintWarning(proc.stderr)
        result = proc.stdout
        FreeCAD.Console.PrintMessage(result.decode())
        return not pip_failed

    def _install_required(self, vendor_path: os.PathLike):
        """Install the required Python package dependencies. If any fail a failure signal is
        emitted and the function exits without proceeding with any additional installs."""
        python_exe = utils.get_python_exe()
        for pymod in self.python_required:
            if QtCore.QThread.currentThread().isInterruptionRequested():
                return
            proc = subprocess.run(
                [
                    python_exe,
                    "-m",
                    "pip",
                    "install",
                    "--disable-pip-version-check",
                    "--target",
                    vendor_path,
                    pymod,
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=True,
            )
            FreeCAD.Console.PrintMessage(proc.stdout.decode())
            if proc.returncode != 0:
                self.failure.emit(
                    translate(
                        "AddonsInstaller",
                        "Installation of Python package {} failed",
                    ).format(pymod),
                    proc.stderr,
                )
                return

    def _install_optional(self, vendor_path: os.PathLike):
        """Install the optional Python package dependencies. If any fail a message is printed to
        the console, but installation of the others continues."""
        python_exe = utils.get_python_exe()
        for pymod in self.python_optional:
            if QtCore.QThread.currentThread().isInterruptionRequested():
                return
            try:
                proc = subprocess.run(
                    [
                        python_exe,
                        "-m",
                        "pip",
                        "install",
                        "--target",
                        vendor_path,
                        pymod,
                    ],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    check=True,
                )
            except subprocess.CalledProcessError as e:
                FreeCAD.Console.PrintError(str(e))
                continue
            FreeCAD.Console.PrintMessage(proc.stdout.decode())
            if proc.returncode != 0:
                FreeCAD.Console.PrintError(proc.stderr.decode())


class UpdateMetadataCacheWorker(QtCore.QThread):
    """Scan through all available packages and see if our local copy of package.xml needs to be
    updated"""

    status_message = QtCore.Signal(str)
    progress_made = QtCore.Signal(int, int)
    package_updated = QtCore.Signal(Addon)

    class RequestType(Enum):
        """The type of item being downloaded."""

        PACKAGE_XML = auto()
        METADATA_TXT = auto()
        REQUIREMENTS_TXT = auto()
        ICON = auto()

    def __init__(self, repos):

        QtCore.QThread.__init__(self)
        self.repos = repos
        self.requests: Dict[int, (Addon, UpdateMetadataCacheWorker.RequestType)] = {}
        NetworkManager.AM_NETWORK_MANAGER.completed.connect(self.download_completed)
        self.requests_completed = 0
        self.total_requests = 0
        self.store = os.path.join(
            FreeCAD.getUserCachePath(), "AddonManager", "PackageMetadata"
        )
        self.updated_repos = set()

    def run(self):
        current_thread = QtCore.QThread.currentThread()

        for repo in self.repos:
            if repo.url and utils.recognized_git_location(repo):
                # package.xml
                index = NetworkManager.AM_NETWORK_MANAGER.submit_unmonitored_get(
                    utils.construct_git_url(repo, "package.xml")
                )
                self.requests[index] = (
                    repo,
                    UpdateMetadataCacheWorker.RequestType.PACKAGE_XML,
                )
                self.total_requests += 1

                # metadata.txt
                index = NetworkManager.AM_NETWORK_MANAGER.submit_unmonitored_get(
                    utils.construct_git_url(repo, "metadata.txt")
                )
                self.requests[index] = (
                    repo,
                    UpdateMetadataCacheWorker.RequestType.METADATA_TXT,
                )
                self.total_requests += 1

                # requirements.txt
                index = NetworkManager.AM_NETWORK_MANAGER.submit_unmonitored_get(
                    utils.construct_git_url(repo, "requirements.txt")
                )
                self.requests[index] = (
                    repo,
                    UpdateMetadataCacheWorker.RequestType.REQUIREMENTS_TXT,
                )
                self.total_requests += 1

        while self.requests:
            if current_thread.isInterruptionRequested():
                NetworkManager.AM_NETWORK_MANAGER.completed.disconnect(
                    self.download_completed
                )
                for request in self.requests.keys():
                    NetworkManager.AM_NETWORK_MANAGER.abort(request)
                return
            # 50 ms maximum between checks for interruption
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)

        # This set contains one copy of each of the repos that got some kind of data in
        # this process. For those repos, tell the main Addon Manager code that it needs
        # to update its copy of the repo, and redraw its information.
        for repo in self.updated_repos:
            self.package_updated.emit(repo)

    def download_completed(
        self, index: int, code: int, data: QtCore.QByteArray
    ) -> None:
        """Callback for handling a completed metadata file download."""
        if index in self.requests:
            self.requests_completed += 1
            self.progress_made.emit(self.requests_completed, self.total_requests)
            request = self.requests.pop(index)
            if code == 200:  # HTTP success
                self.updated_repos.add(request[0])  # mark this repo as updated
                if request[1] == UpdateMetadataCacheWorker.RequestType.PACKAGE_XML:
                    self.process_package_xml(request[0], data)
                elif request[1] == UpdateMetadataCacheWorker.RequestType.METADATA_TXT:
                    self.process_metadata_txt(request[0], data)
                elif (
                    request[1] == UpdateMetadataCacheWorker.RequestType.REQUIREMENTS_TXT
                ):
                    self.process_requirements_txt(request[0], data)
                elif request[1] == UpdateMetadataCacheWorker.RequestType.ICON:
                    self.process_icon(request[0], data)

    def process_package_xml(self, repo: Addon, data: QtCore.QByteArray):
        """Process the package.xml metadata file"""
        repo.repo_type = Addon.Kind.PACKAGE  # By definition
        package_cache_directory = os.path.join(self.store, repo.name)
        if not os.path.exists(package_cache_directory):
            os.makedirs(package_cache_directory)
        new_xml_file = os.path.join(package_cache_directory, "package.xml")
        with open(new_xml_file, "wb") as f:
            f.write(data.data())
        metadata = FreeCAD.Metadata(new_xml_file)
        repo.metadata = metadata
        self.status_message.emit(
            translate("AddonsInstaller", "Downloaded package.xml for {}").format(
                repo.name
            )
        )

        # Grab a new copy of the icon as well: we couldn't enqueue this earlier because
        # we didn't know the path to it, which is stored in the package.xml file.
        icon = metadata.Icon
        if not icon:
            # If there is no icon set for the entire package, see if there are
            # any workbenches, which are required to have icons, and grab the first
            # one we find:
            content = repo.metadata.Content
            if "workbench" in content:
                wb = content["workbench"][0]
                if wb.Icon:
                    if wb.Subdirectory:
                        subdir = wb.Subdirectory
                    else:
                        subdir = wb.Name
                    repo.Icon = subdir + wb.Icon
                    icon = repo.Icon

        icon_url = utils.construct_git_url(repo, icon)
        index = NetworkManager.AM_NETWORK_MANAGER.submit_unmonitored_get(icon_url)
        self.requests[index] = (repo, UpdateMetadataCacheWorker.RequestType.ICON)
        self.total_requests += 1

    def process_metadata_txt(self, repo: Addon, data: QtCore.QByteArray):
        """Process the metadata.txt metadata file"""
        self.status_message.emit(
            translate("AddonsInstaller", "Downloaded metadata.txt for {}").format(
                repo.display_name
            )
        )
        f = io.StringIO(data.data().decode("utf8"))
        while True:
            line = f.readline()
            if not line:
                break
            if line.startswith("workbenches="):
                depswb = line.split("=")[1].split(",")
                for wb in depswb:
                    wb_name = wb.strip()
                    if wb_name:
                        repo.requires.add(wb_name)
                        FreeCAD.Console.PrintLog(
                            f"{repo.display_name} requires FreeCAD Addon '{wb_name}'\n"
                        )

            elif line.startswith("pylibs="):
                depspy = line.split("=")[1].split(",")
                for pl in depspy:
                    dep = pl.strip()
                    if dep:
                        repo.python_requires.add(dep)
                        FreeCAD.Console.PrintLog(
                            f"{repo.display_name} requires python package '{dep}'\n"
                        )

            elif line.startswith("optionalpylibs="):
                opspy = line.split("=")[1].split(",")
                for pl in opspy:
                    dep = pl.strip()
                    if dep:
                        repo.python_optional.add(dep)
                        FreeCAD.Console.PrintLog(
                            f"{repo.display_name} optionally imports python package"
                            + f" '{pl.strip()}'\n"
                        )
        # For review and debugging purposes, store the file locally
        package_cache_directory = os.path.join(self.store, repo.name)
        if not os.path.exists(package_cache_directory):
            os.makedirs(package_cache_directory)
        new_xml_file = os.path.join(package_cache_directory, "metadata.txt")
        with open(new_xml_file, "wb") as f:
            f.write(data.data())

    def process_requirements_txt(self, repo: Addon, data: QtCore.QByteArray):
        """Process the requirements.txt metadata file"""
        self.status_message.emit(
            translate(
                "AddonsInstaller",
                "Downloaded requirements.txt for {}",
            ).format(repo.display_name)
        )
        f = io.StringIO(data.data().decode("utf8"))
        lines = f.readlines()
        for line in lines:
            break_chars = " <>=~!+#"
            package = line
            for n, c in enumerate(line):
                if c in break_chars:
                    package = line[:n].strip()
                    break
            if package:
                repo.python_requires.add(package)
        # For review and debugging purposes, store the file locally
        package_cache_directory = os.path.join(self.store, repo.name)
        if not os.path.exists(package_cache_directory):
            os.makedirs(package_cache_directory)
        new_xml_file = os.path.join(package_cache_directory, "requirements.txt")
        with open(new_xml_file, "wb") as f:
            f.write(data.data())

    def process_icon(self, repo: Addon, data: QtCore.QByteArray):
        """Convert icon data into a valid icon file and store it"""
        self.status_message.emit(
            translate("AddonsInstaller", "Downloaded icon for {}").format(
                repo.display_name
            )
        )
        cache_file = repo.get_cached_icon_filename()
        with open(cache_file, "wb") as icon_file:
            icon_file.write(data.data())
            repo.cached_icon_filename = cache_file


class UpdateAllWorker(QtCore.QThread):
    """Update all listed packages, of any kind"""

    progress_made = QtCore.Signal(int, int)
    status_message = QtCore.Signal(str)
    success = QtCore.Signal(Addon)
    failure = QtCore.Signal(Addon)

    # TODO: This should be re-written to be solidly single-threaded, some of the called code is
    # not re-entrant

    def __init__(self, repos):
        super().__init__()
        self.repos = repos

    def run(self):
        self.progress_made.emit(0, len(self.repos))
        self.repo_queue = queue.Queue()
        current_thread = QtCore.QThread.currentThread()
        for repo in self.repos:
            self.repo_queue.put(repo)
            FreeCAD.Console.PrintLog(
                f"  UPDATER: Adding '{repo.name}' to update queue\n"
            )

        # The original design called for multiple update threads at the same time, but the updater
        # itself is not thread-safe, so for the time being only spawn one update thread.
        workers = []
        for _ in range(1):
            FreeCAD.Console.PrintLog(f"  UPDATER: Starting worker\n")
            worker = UpdateSingleWorker(self.repo_queue)
            worker.success.connect(self.on_success)
            worker.failure.connect(self.on_failure)
            worker.start()
            workers.append(worker)

        while not self.repo_queue.empty():
            if current_thread.isInterruptionRequested():
                for worker in workers:
                    worker.blockSignals(True)
                    worker.requestInterruption()
                    worker.wait()
                return
            # Ensure our signals propagate out by running an internal thread-local event loop
            QtCore.QCoreApplication.processEvents()

        self.repo_queue.join()

        # Make sure all of our child threads have fully exited:
        for worker in workers:
            worker.wait()

    def on_success(self, repo: Addon) -> None:
        FreeCAD.Console.PrintLog(
            f"  UPDATER: Main thread received notice that worker successfully updated {repo.name}\n"
        )
        self.progress_made.emit(
            len(self.repos) - self.repo_queue.qsize(), len(self.repos)
        )
        self.success.emit(repo)

    def on_failure(self, repo: Addon) -> None:
        FreeCAD.Console.PrintLog(
            f"  UPDATER:  Main thread received notice that worker failed to update {repo.name}\n"
        )
        self.progress_made.emit(
            len(self.repos) - self.repo_queue.qsize(), len(self.repos)
        )
        self.failure.emit(repo)


class UpdateSingleWorker(QtCore.QThread):
    success = QtCore.Signal(Addon)
    failure = QtCore.Signal(Addon)

    def __init__(self, repo_queue: queue.Queue, location=None):
        super().__init__()
        self.repo_queue = repo_queue
        self.location = location

    def run(self):
        current_thread = QtCore.QThread.currentThread()
        while True:
            if current_thread.isInterruptionRequested():
                FreeCAD.Console.PrintLog(
                    f"  UPDATER: Interruption requested, stopping all updates\n"
                )
                return
            try:
                repo = self.repo_queue.get_nowait()
                FreeCAD.Console.PrintLog(
                    f"  UPDATER: Pulling {repo.name} from the update queue\n"
                )
            except queue.Empty:
                FreeCAD.Console.PrintLog(
                    f"  UPDATER: Worker thread queue is empty, exiting thread\n"
                )
                return
            if repo.repo_type == Addon.Kind.MACRO:
                FreeCAD.Console.PrintLog(f"  UPDATER: Updating macro '{repo.name}'\n")
                self.update_macro(repo)
            else:
                FreeCAD.Console.PrintLog(f"  UPDATER: Updating addon '{repo.name}'\n")
                self.update_package(repo)
            self.repo_queue.task_done()
            FreeCAD.Console.PrintLog(
                f"  UPDATER: Worker thread completed action for '{repo.name}' and reported result "
                + "to main thread\n"
            )

    def update_macro(self, repo: Addon):
        """Updating a macro happens in this function, in the current thread"""

        cache_path = os.path.join(
            FreeCAD.getUserCachePath(), "AddonManager", "MacroCache"
        )
        os.makedirs(cache_path, exist_ok=True)
        install_succeeded, _ = repo.macro.install(cache_path)

        if install_succeeded:
            install_succeeded, _ = repo.macro.install(FreeCAD.getUserMacroDir(True))
            utils.update_macro_installation_details(repo)

        if install_succeeded:
            self.success.emit(repo)
        else:
            self.failure.emit(repo)

    def update_package(self, repo: Addon):
        """Updating a package re-uses the package installation worker, so actually spawns another
        thread that we block on"""

        worker = InstallWorkbenchWorker(repo, location=self.location)
        worker.success.connect(lambda repo, _: self.success.emit(repo))
        worker.failure.connect(lambda repo, _: self.failure.emit(repo))
        worker.start()
        while True:
            # Ensure our signals propagate out by running an internal thread-local event loop
            QtCore.QCoreApplication.processEvents()
            if not worker.isRunning():
                break

        time.sleep(0.1)  # Give the signal a moment to propagate to the other threads
        QtCore.QCoreApplication.processEvents()


#  @}
