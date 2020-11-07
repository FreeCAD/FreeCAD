# -*- coding: utf-8 -*-
#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
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

import os
import re
import shutil
import stat
import sys
import tempfile

from PySide import QtCore, QtGui

import FreeCAD
if FreeCAD.GuiUp:
    import FreeCADGui

import addonmanager_utilities as utils
from addonmanager_utilities import translate  # this needs to be as is for pylupdate
from addonmanager_macro import Macro

have_git = False
try:
    import git
    have_git = True
except ImportError:
    pass

have_zip = False
try:
    import zipfile
    have_zip = True
except ImportError:
    pass

have_markdown = False
try:
    import markdown
    have_markdown = True
except ImportError:
    pass

try:
    import StringIO as io
    _stringio = io.StringIO
except ImportError:  # StringIO is not available with python3
    import io
    _stringio = io.BytesIO

#  @package AddonManager_workers
#  \ingroup ADDONMANAGER
#  \brief Multithread workers for the addon manager
#  @{

# Blacklisted addons
macros_blacklist = []

# These addons will print an additional message informing the user
obsolete = []

# These addons will print an additional message informing the user Python2 only
py2only = []

NOGIT = False  # for debugging purposes, set this to True to always use http downloads

NOMARKDOWN = False  # for debugging purposes, set this to True to disable Markdown lib

"""Multithread workers for the Addon Manager"""


class UpdateWorker(QtCore.QThread):
    """This worker updates the list of available workbenches"""

    info_label = QtCore.Signal(str)
    addon_repo = QtCore.Signal(object)
    progressbar_show = QtCore.Signal(bool)
    done = QtCore.Signal()

    def __init__(self):

        QtCore.QThread.__init__(self)

    def run(self):
        "populates the list of addons"

        self.progressbar_show.emit(True)

        # update info lists
        global obsolete, macros_blacklist, py2only
        u = utils.urlopen("https://raw.githubusercontent.com/FreeCAD/FreeCAD-addons/master/addonflags.json")
        if u:
            p = u.read()
            if sys.version_info.major >= 3 and isinstance(p, bytes):
                p = p.decode("utf-8")
            u.close()
            hit = re.findall(r'"obsolete"[^\{]*?{[^\{]*?"Mod":\[(?P<obsolete>[^\[\]]+?)\]}',
                             p.replace("\n", "").replace(" ", ""))
            if hit:
                obsolete = hit[0].replace('"', "").split(",")
            hit = re.findall(r'"blacklisted"[^\{]*?{[^\{]*?"Macro":\[(?P<blacklisted>[^\[\]]+?)\]}',
                             p.replace("\n", "").replace(" ", ""))
            if hit:
                macros_blacklist = hit[0].replace('"', "").split(",")
            hit = re.findall(r'"py2only"[^\{]*?{[^\{]*?"Mod":\[(?P<py2only>[^\[\]]+?)\]}',
                             p.replace("\n", "").replace(" ", ""))
            if hit:
                py2only = hit[0].replace('"', "").split(",")
        else:
            print("Debug: addon_flags.json not found")

        u = utils.urlopen("https://raw.githubusercontent.com/FreeCAD/FreeCAD-addons/master/.gitmodules")
        if not u:
            self.progressbar_show.emit(False)
            self.done.emit()
            self.stop = True
            return
        p = u.read()
        if sys.version_info.major >= 3 and isinstance(p, bytes):
            p = p.decode("utf-8")
        u.close()
        p = re.findall((r'(?m)\[submodule\s*"(?P<name>.*)"\]\s*'
                        r"path\s*=\s*(?P<path>.+)\s*"
                        r"url\s*=\s*(?P<url>https?://.*)"), p)
        basedir = FreeCAD.getUserAppDataDir()
        moddir = basedir + os.sep + "Mod"
        repos = []
        # querying official addons
        for name, path, url in p:
            self.info_label.emit(name)
            url = url.split(".git")[0]
            addondir = moddir + os.sep + name
            if os.path.exists(addondir) and os.listdir(addondir):
                # make sure the folder exists and it contains files!
                state = 1
            else:
                state = 0
            repos.append([name, url, state])
        # querying custom addons
        customaddons = (FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
                        .GetString("CustomRepositories", "").split("\n"))
        for url in customaddons:
            if url:
                name = url.split("/")[-1]
                if name.lower().endswith(".git"):
                    name = name[:-4]
                addondir = moddir + os.sep + name
                if not os.path.exists(addondir):
                    state = 0
                else:
                    state = 1
                repos.append([name, url, state])
        if not repos:
            self.info_label.emit(translate("AddonsInstaller", "Unable to download addon list."))
        else:
            repos = sorted(repos, key=lambda s: s[0].lower())
            for repo in repos:
                self.addon_repo.emit(repo)
            self.info_label.emit(translate("AddonsInstaller", "Workbenches list was updated."))
        self.progressbar_show.emit(False)
        self.done.emit()
        self.stop = True


class InfoWorker(QtCore.QThread):
    """This worker retrieves the description text of a workbench"""

    addon_repos = QtCore.Signal(object)

    def __init__(self):

        QtCore.QThread.__init__(self)

    def run(self):

        i = 0
        for repo in self.repos:
            url = repo[1]
            u = utils.urlopen(url)
            if not u:
                self.stop = True
                return
            p = u.read()
            if sys.version_info.major >= 3 and isinstance(p, bytes):
                p = p.decode("utf-8")
            u.close()
            desc = re.findall('<meta property="og:description" content="(.*?)"', p)
            if desc:
                desc = desc[0]
            else:
                desc = "Unable to retrieve addon description"
            self.repos[i].append(desc)
            i += 1
            self.addon_repos.emit(self.repos)
        self.stop = True


class CheckWBWorker(QtCore.QThread):
    """This worker checks for available updates for all workbenches"""

    enable = QtCore.Signal(int)
    mark = QtCore.Signal(str)
    addon_repos = QtCore.Signal(object)

    def __init__(self, repos):

        QtCore.QThread.__init__(self)
        self.repos = repos

    def run(self):

        if NOGIT or not have_git:
            self.stop = True
            return
        basedir = FreeCAD.getUserAppDataDir()
        moddir = basedir + os.sep + "Mod"
        upds = []
        gitpython_warning = False
        for repo in self.repos:
            if repo[2] == 1:  # installed
                # print("Checking for updates for", repo[0])
                clonedir = moddir + os.sep + repo[0]
                if os.path.exists(clonedir):
                    # mark as already installed AND already checked for updates
                    self.repos[self.repos.index(repo)][2] = 2
                    if not os.path.exists(clonedir + os.sep + ".git"):
                        # Repair addon installed with raw download
                        bare_repo = git.Repo.clone_from(repo[1], clonedir + os.sep + ".git", bare=True)
                        try:
                            with bare_repo.config_writer() as cw:
                                cw.set("core", "bare", False)
                        except AttributeError:
                            if not gitpython_warning:
                                FreeCAD.Console.PrintWarning(translate("AddonsInstaller",
                                                                       "Outdated GitPython detected, "
                                                                       "consider upgrading with pip.") + "\n")
                                gitpython_warning = True
                            cw = bare_repo.config_writer()
                            cw.set("core", "bare", False)
                            del cw
                        repo = git.Repo(clonedir)
                        repo.head.reset("--hard")
                    gitrepo = git.Git(clonedir)
                    try:
                        gitrepo.fetch()
                    except Exception:
                        print("AddonManager: Unable to fetch git updates for repo", repo[0])
                    else:
                        if "git pull" in gitrepo.status():
                            self.mark.emit(repo[0])
                            upds.append(repo[0])
                            # mark as already installed AND already checked for updates AND update available
                            self.repos[self.repos.index(repo)][2] = 3
        self.addon_repos.emit(self.repos)
        self.enable.emit(len(upds))
        self.stop = True


class FillMacroListWorker(QtCore.QThread):
    """This worker populates the list of macros"""

    add_macro_signal = QtCore.Signal(Macro)
    info_label_signal = QtCore.Signal(str)
    progressbar_show = QtCore.Signal(bool)

    def __init__(self, repo_dir):

        QtCore.QThread.__init__(self)
        self.repo_dir = repo_dir
        self.macros = []

    def run(self):
        """Populates the list of macros"""

        self.retrieve_macros_from_git()
        self.retrieve_macros_from_wiki()
        [self.add_macro_signal.emit(m) for m in sorted(self.macros, key=lambda m: m.name.lower())]
        if self.macros:
            self.info_label_signal.emit(translate("AddonsInstaller", "List of macros successfully retrieved."))
        self.progressbar_show.emit(False)
        self.stop = True

    def retrieve_macros_from_git(self):
        """Retrieve macros from FreeCAD-macros.git

        Emits a signal for each macro in
        https://github.com/FreeCAD/FreeCAD-macros.git
        """

        if not have_git:
            self.info_label_signal.emit("GitPython not installed! Cannot retrieve macros from Git")
            FreeCAD.Console.PrintWarning(translate("AddonsInstaller",
                                                   "GitPython not installed! Cannot retrieve macros from git")+"\n")
            return

        self.info_label_signal.emit("Downloading list of macros from git...")
        try:
            git.Repo.clone_from("https://github.com/FreeCAD/FreeCAD-macros.git", self.repo_dir)
        except Exception:
            FreeCAD.Console.PrintWarning(translate("AddonsInstaller",
                                                   "Something went wrong with the Git Macro Retrieval, "
                                                   "possibly the Git executable is not in the path") + "\n")
        for dirpath, _, filenames in os.walk(self.repo_dir):
            if ".git" in dirpath:
                continue
            for filename in filenames:
                if filename.lower().endswith(".fcmacro"):
                    macro = Macro(filename[:-8])  # Remove ".FCMacro".
                    macro.on_git = True
                    macro.src_filename = os.path.join(dirpath, filename)
                    self.macros.append(macro)

    def retrieve_macros_from_wiki(self):
        """Retrieve macros from the wiki

        Read the wiki and emit a signal for each found macro.
        Reads only the page https://www.freecadweb.org/wiki/Macros_recipes
        """

        self.info_label_signal.emit("Downloading list of macros from the FreeCAD wiki...")
        self.progressbar_show.emit(True)
        u = utils.urlopen("https://www.freecadweb.org/wiki/Macros_recipes")
        if not u:
            FreeCAD.Console.PrintWarning(translate("AddonsInstaller",
                                                   "Appears to be an issue connecting to the Wiki, "
                                                   "therefore cannot retrieve Wiki macro list at this time") + "\n")
            return
        p = u.read()
        u.close()
        if sys.version_info.major >= 3 and isinstance(p, bytes):
            p = p.decode("utf-8")
        macros = re.findall('title="(Macro.*?)"', p)
        macros = [mac for mac in macros if ("translated" not in mac)]
        for mac in macros:
            macname = mac[6:]  # Remove "Macro ".
            macname = macname.replace("&amp;", "&")
            if (macname not in macros_blacklist) and ("recipes" not in macname.lower()):
                macro = Macro(macname)
                macro.on_wiki = True
                self.macros.append(macro)


class ShowWorker(QtCore.QThread):
    """This worker retrieves info of a given workbench"""

    info_label = QtCore.Signal(str)
    addon_repos = QtCore.Signal(object)
    progressbar_show = QtCore.Signal(bool)

    def __init__(self, repos, idx):

        # repos is a list of [name, url, installbit, descr]
        #   name      : Addon name
        #   url       : Addon repository location
        #   installbit: 0 = Addon is not installed
        #               1 = Addon is installed
        #               2 = Addon is installed and checked for available updates (none pending)
        #               3 = Addon is installed and has a pending update
        #   descr     : Addon description

        QtCore.QThread.__init__(self)
        self.repos = repos
        self.idx = idx

    def run(self):

        self.progressbar_show.emit(True)
        self.info_label.emit(translate("AddonsInstaller", "Retrieving description..."))
        if len(self.repos[self.idx]) == 4:
            desc = self.repos[self.idx][3]
        else:
            u = None
            url = self.repos[self.idx][1]
            self.info_label.emit(translate("AddonsInstaller", "Retrieving info from") + " " + str(url))
            desc = ""
            regex = utils.get_readme_regex(url)
            if regex:
                # extract readme from html via regex
                readmeurl = utils.get_readme_html_url(url)
                if not readmeurl:
                    print("Debug: README not found for", url)
                u = utils.urlopen(readmeurl)
                if not u:
                    print("Debug: README not found at", readmeurl)
                u = utils.urlopen(readmeurl)
                if u:
                    p = u.read()
                    if sys.version_info.major >= 3 and isinstance(p, bytes):
                        p = p.decode("utf-8")
                    u.close()
                    readme = re.findall(regex, p, flags=re.MULTILINE | re.DOTALL)
                    if readme:
                        desc = readme[0]
                else:
                    print("Debug: README not found at", readmeurl)
            else:
                # convert raw markdown using lib
                readmeurl = utils.get_readme_url(url)
                if not readmeurl:
                    print("Debug: README not found for", url)
                u = utils.urlopen(readmeurl)
                if u:
                    p = u.read()
                    if sys.version_info.major >= 3 and isinstance(p, bytes):
                        p = p.decode("utf-8")
                    u.close()
                    desc = utils.fix_relative_links(p, readmeurl.rsplit("/README.md")[0])
                    if NOMARKDOWN or not have_markdown:
                        desc = markdown.markdown(desc, extensions=["md_in_html"])
                    else:
                        message = """
<div style="width: 100%; text-align:center;background: #91bbe0;">
    <strong style="color: #FFFFFF;">
"""
                        message += translate("AddonsInstaller", "Raw markdown displayed")
                        message += "</strong><br/><br/>"
                        message += translate("AddonsInstaller", "Python Markdown library is missing.")
                        message += "<br/></div><hr/><pre>" + desc + "</pre>"
                        desc = message
                else:
                    print("Debug: README not found at", readmeurl)
            if desc == "":
                # fall back to the description text
                u = utils.urlopen(url)
                if not u:
                    self.progressbar_show.emit(False)
                    self.stop = True
                    return
                p = u.read()
                if sys.version_info.major >= 3 and isinstance(p, bytes):
                    p = p.decode("utf-8")
                u.close()
                descregex = utils.get_desc_regex(url)
                if descregex:
                    desc = re.findall(descregex, p)
                    if desc:
                        desc = desc[0]
            if not desc:
                desc = "Unable to retrieve addon description"
            self.repos[self.idx].append(desc)
            self.addon_repos.emit(self.repos)
        # Addon is installed so lets check if it has an update
        if self.repos[self.idx][2] == 1:
            upd = False
            # checking for updates
            if not NOGIT and have_git:
                repo = self.repos[self.idx]
                clonedir = FreeCAD.getUserAppDataDir() + os.sep + "Mod" + os.sep + repo[0]
                if os.path.exists(clonedir):
                    if not os.path.exists(clonedir + os.sep + ".git"):
                        # Repair addon installed with raw download
                        bare_repo = git.Repo.clone_from(repo[1], clonedir + os.sep + ".git", bare=True)
                        try:
                            with bare_repo.config_writer() as cw:
                                cw.set("core", "bare", False)
                        except AttributeError:
                            FreeCAD.Console.PrintWarning(translate("AddonsInstaller",
                                                                   "Outdated GitPython detected, "
                                                                   "consider upgrading with pip.") + "\n")
                            cw = bare_repo.config_writer()
                            cw.set("core", "bare", False)
                            del cw
                        repo = git.Repo(clonedir)
                        repo.head.reset("--hard")
                    gitrepo = git.Git(clonedir)
                    gitrepo.fetch()
                    if "git pull" in gitrepo.status():
                        upd = True
            # If there is an update pending, lets user know via the UI
            if upd:
                message = """
<div style="width: 100%;text-align: center;background: #75AFFD;">
    <br/>
    <strong style="background: #397FF7;color: #FFFFFF;">
"""
                message += translate("AddonsInstaller", "An update is available for this addon.")
                message += "</strong><br/></div><hr/>" + desc + '<br/><br/>Addon repository: <a href="'
                message += self.repos[self.idx][1] + '">' + self.repos[self.idx][1] + "</a>"
                # mark as already installed AND already checked for updates AND update is available
                self.repos[self.idx][2] = 3
            # If there isn't, indicate that this addon is already installed
            else:
                message = """
<div style="width: 100%;text-align: center;background: #C1FEB2;">
    <br/>
    <strong style="background: #00B629;color: #FFFFFF;">
"""
                message += translate("AddonsInstaller", "This addon is already installed.")
                message += "</strong><br/></div><hr/>" + desc
                message += '<br/><br/>Addon repository: <a href="'
                message += self.repos[self.idx][1] + '">' + self.repos[self.idx][1] + "</a>"
                self.repos[self.idx][2] = 2  # mark as already installed AND already checked for updates
            # Let the user know the install path for this addon
            message += "<br/>" + translate("AddonInstaller", "Installed location") + ": "
            message += FreeCAD.getUserAppDataDir() + os.sep + "Mod" + os.sep + self.repos[self.idx][0]
            self.addon_repos.emit(self.repos)
        elif self.repos[self.idx][2] == 2:
            message = """
<div style="width: 100%;text-align: center;background: #C1FEB2;">
    <br/>
    <strong style="background: #00B629;color: #FFFFFF;">
"""
            message += translate("AddonsInstaller", "This addon is already installed.")
            message += "</strong><br></div><hr/>" + desc
            message += '<br/><br/>Addon repository: <a href="'
            message += self.repos[self.idx][1] + '">' + self.repos[self.idx][1] + "</a>"
            message += "<br/>" + translate("AddonInstaller", "Installed location") + ": "
            message += FreeCAD.getUserAppDataDir() + os.sep + "Mod" + os.sep + self.repos[self.idx][0]
        elif self.repos[self.idx][2] == 3:
            message = """
<div style="width: 100%;text-align: center;background: #75AFFD;">
    <br/>
    <strong style="background: #397FF7;color: #FFFFFF;">
"""
            message += translate("AddonsInstaller", "An update is available for this addon.")
            message += "</strong><br/></div><hr/>" + desc + '<br/><br/>Addon repository: <a href="'
            message += self.repos[self.idx][1] + '">' + self.repos[self.idx][1] + "</a>"
            message += "<br/>" + translate("AddonInstaller", "Installed location") + ": "
            message += FreeCAD.getUserAppDataDir() + os.sep + "Mod" + os.sep + self.repos[self.idx][0]
        else:
            message = desc + '<br/><br/>Addon repository: <a href="'
            message += self.repos[self.idx][1] + '">' + self.repos[self.idx][1] + '</a>'

        # If the Addon is obsolete, let the user know through the Addon UI
        if self.repos[self.idx][0] in obsolete:
            message = """
<div style="width: 100%; text-align:center; background: #FFB3B3;">
    <strong style="color: #FFFFFF; background: #FF0000;">
"""
            message += translate("AddonsInstaller", "This addon is marked as obsolete") + "</strong><br/><br/>"
            message += translate("AddonsInstaller",
                                 "This usually means it is no longer maintained, "
                                 "and some more advanced addon in this list "
                                 "provides the same functionality.") + "<br/></div><hr/>" + desc

        # If the Addon is Python 2 only, let the user know through the Addon UI
        if self.repos[self.idx][0] in py2only:
            message = """
<div style="width: 100%; text-align:center; background: #ffe9b3;">
    <strong style="color: #FFFFFF; background: #ff8000;">
"""
            message += translate("AddonsInstaller", "This addon is marked as Python 2 Only") + "</strong><br/><br/>"
            message += translate("AddonsInstaller",
                                 "This workbench may no longer be maintained and "
                                 "installing it on a Python 3 system will more than "
                                 "likely result in errors at startup or while in use.")
            message += "<br/></div><hr/>" + desc

        self.info_label.emit(message)
        self.progressbar_show.emit(False)
        self.mustLoadImages = True
        label = self.loadImages(message, self.repos[self.idx][1], self.repos[self.idx][0])
        if label:
            self.info_label.emit(label)
        self.stop = True

    def stopImageLoading(self):
        "this stops the image loading process and allow the thread to terminate earlier"

        self.mustLoadImages = False

    def loadImages(self, message, url, wbName):
        "checks if the given page contains images and downloads them"

        # QTextBrowser cannot display online images. So we download them
        # here, and replace the image link in the html code with the
        # downloaded version

        imagepaths = re.findall("<img.*?src=\"(.*?)\"", message)
        if imagepaths:
            storedimages = []
            store = os.path.join(FreeCAD.getUserAppDataDir(), "AddonManager", "Images")
            if not os.path.exists(store):
                os.makedirs(store)
            for path in imagepaths:
                if not self.mustLoadImages:
                    return None
                origpath = path
                if "?" in path:
                    # remove everything after the ?
                    path = path.split("?")[0]
                if not path.startswith("http"):
                    path = utils.getserver(url) + path
                name = path.split("/")[-1]
                if name and path.startswith("http"):
                    storename = os.path.join(store, name)
                    if len(storename) >= 260:
                        remainChars = 259 - (len(store) + len(wbName) + 1)
                        storename = os.path.join(store, wbName+name[-remainChars:])
                    if not os.path.exists(storename):
                        try:
                            u = utils.urlopen(path)
                            imagedata = u.read()
                            u.close()
                        except Exception:
                            print("AddonManager: Debug: Error retrieving image from", path)
                        else:
                            f = open(storename, "wb")
                            f.write(imagedata)
                            f.close()

                            # resize the image to 300x300px if needed
                            img = QtGui.QImage(storename)
                            if (img.width() > 300) or (img.height() > 300):
                                pix = QtGui.QPixmap()
                                pix = pix.fromImage(img.scaled(300, 300,
                                                               QtCore.Qt.KeepAspectRatio,
                                                               QtCore.Qt.FastTransformation))
                                pix.save(storename, "jpeg", 100)
                    message = message.replace("src=\""+origpath, "src=\"file:///"+storename.replace("\\", "/"))
            # print(message)
            return message
        return None


class GetMacroDetailsWorker(QtCore.QThread):
    """Retrieve the macro details for a macro"""

    info_label = QtCore.Signal(str)
    progressbar_show = QtCore.Signal(bool)

    def __init__(self, macro):

        QtCore.QThread.__init__(self)
        self.macro = macro

    def run(self):

        self.progressbar_show.emit(True)
        self.info_label.emit(translate("AddonsInstaller", "Retrieving description..."))
        if not self.macro.parsed and self.macro.on_git:
            self.info_label.emit(translate("AddonsInstaller", "Retrieving info from git"))
            self.macro.fill_details_from_file(self.macro.src_filename)
        if not self.macro.parsed and self.macro.on_wiki:
            self.info_label.emit(translate("AddonsInstaller", "Retrieving info from wiki"))
            mac = self.macro.name.replace(" ", "_")
            mac = mac.replace("&", "%26")
            mac = mac.replace("+", "%2B")
            url = "https://www.freecadweb.org/wiki/Macro_" + mac
            self.macro.fill_details_from_wiki(url)
        if self.macro.is_installed():
            already_installed_msg = ('<strong style=\"background: #00B629;\">'
                                     + translate("AddonsInstaller", "This macro is already installed.")
                                     + '</strong><br>')
        else:
            already_installed_msg = ""
        message = (already_installed_msg
                   + "<h1>"+self.macro.name+"</h1>"
                   + self.macro.desc
                   + "<br/><br/>Macro location: <a href=\""
                   + self.macro.url
                   + "\">"
                   + self.macro.url
                   + "</a>")
        self.info_label.emit(message)
        self.progressbar_show.emit(False)
        self.stop = True


class InstallWorker(QtCore.QThread):
    "This worker installs a workbench"

    info_label = QtCore.Signal(str)
    progressbar_show = QtCore.Signal(bool)
    mark_recompute = QtCore.Signal(str)

    def __init__(self, repos, idx):

        QtCore.QThread.__init__(self)
        self.idx = idx
        self.repos = repos

    def run(self):
        "installs or updates the selected addon"

        if not have_git:
            self.info_label.emit("GitPython not found.")
            FreeCAD.Console.PrintWarning(translate("AddonsInstaller",
                                                   "GitPython not found. Using standard download instead.") + "\n")
            if not have_zip:
                self.info_label.emit("no zip support.")
                FreeCAD.Console.PrintError(translate("AddonsInstaller",
                                                     "Your version of python doesn't appear to support ZIP "
                                                     "files. Unable to proceed.") + "\n")
                return

        if not isinstance(self.idx, list):
            self.idx = [self.idx]
        for idx in self.idx:
            if idx < 0:
                return
            if not self.repos:
                return
            basedir = FreeCAD.getUserAppDataDir()
            moddir = basedir + os.sep + "Mod"
            if not os.path.exists(moddir):
                os.makedirs(moddir)
            clonedir = moddir + os.sep + self.repos[idx][0]
            self.progressbar_show.emit(True)
            if os.path.exists(clonedir):
                self.info_label.emit("Updating module...")
                if sys.version_info.major > 2 and str(self.repos[idx][0]) in py2only:
                    FreeCAD.Console.PrintWarning(translate("AddonsInstaller",
                                                           "User requested updating a Python 2 workbench on "
                                                           "a system running Python 3 - ") +
                                                 str(self.repos[idx][0])+"\n")
                if have_git:
                    if not os.path.exists(clonedir + os.sep + ".git"):
                        # Repair addon installed with raw download
                        bare_repo = git.Repo.clone_from(self.repos[idx][1], clonedir + os.sep + ".git", bare=True)
                        try:
                            with bare_repo.config_writer() as cw:
                                cw.set("core", "bare", False)
                        except AttributeError:
                            FreeCAD.Console.PrintWarning(translate("AddonsInstaller",
                                                                   "Outdated GitPython detected, consider "
                                                                   "upgrading with pip.") + "\n")
                            cw = bare_repo.config_writer()
                            cw.set("core", "bare", False)
                            del cw
                        repo = git.Repo(clonedir)
                        repo.head.reset("--hard")
                    repo = git.Git(clonedir)
                    try:
                        answer = repo.pull() + "\n\n" + translate("AddonsInstaller",
                                                                  "Workbench successfully updated. "
                                                                  "Please restart FreeCAD to apply the changes.")
                    except Exception:
                        print("Error updating module", self.repos[idx][1], " - Please fix manually")
                        answer = repo.status()
                        print(answer)
                    else:
                        # Update the submodules for this repository
                        repo_sms = git.Repo(clonedir)
                        for submodule in repo_sms.submodules:
                            submodule.update(init=True, recursive=True)
                else:
                    answer = self.download(self.repos[idx][1], clonedir) + "\n\n"
                    answer += translate("AddonsInstaller",
                                        "Workbench successfully updated. Please "
                                        "restart FreeCAD to apply the changes.")
            else:
                self.info_label.emit("Checking module dependencies...")
                depsok, answer = self.check_dependencies(self.repos[idx][1])
                if depsok:
                    if sys.version_info.major > 2 and str(self.repos[idx][0]) in py2only:
                        FreeCAD.Console.PrintWarning(translate("AddonsInstaller",
                                                               "User requested installing a Python 2 "
                                                               "workbench on a system running Python 3 - ") +
                                                     str(self.repos[idx][0]) + "\n")
                    if have_git:
                        self.info_label.emit("Cloning module...")
                        repo = git.Repo.clone_from(self.repos[idx][1], clonedir, branch="master")

                        # Make sure to clone all the submodules as well
                        if repo.submodules:
                            repo.submodule_update(recursive=True)
                    else:
                        self.info_label.emit("Downloading module...")
                        self.download(self.repos[idx][1], clonedir)
                    answer = translate("AddonsInstaller",
                                       "Workbench successfully installed. Please restart "
                                       "FreeCAD to apply the changes.")
            # symlink any macro contained in the module to the macros folder
            macro_dir = FreeCAD.getUserMacroDir(True)
            if not os.path.exists(macro_dir):
                os.makedirs(macro_dir)
            if os.path.exists(clonedir):
                for f in os.listdir(clonedir):
                    if f.lower().endswith(".fcmacro"):
                        print("copying macro:", f)
                        utils.symlink(os.path.join(clonedir, f), os.path.join(macro_dir, f))
                        FreeCAD.ParamGet('User parameter:Plugins/' +
                                         self.repos[idx][0]).SetString("destination", clonedir)
                        answer += "\n\n" + translate("AddonsInstaller",
                                                     "A macro has been installed and is available "
                                                     "under Macro -> Macros menu")
                        answer += ":\n<b>" + f + "</b>"
            self.progressbar_show.emit(False)
            self.info_label.emit(answer)
            self.mark_recompute.emit(self.repos[idx][0])
        self.stop = True

    def check_dependencies(self, baseurl):
        "checks if the repo contains a metadata.txt and check its contents"

        ok = True
        message = ""
        depsurl = baseurl.replace("github.com", "raw.githubusercontent.com")
        if not depsurl.endswith("/"):
            depsurl += "/"
        depsurl += "master/metadata.txt"
        mu = utils.urlopen(depsurl)
        if mu:
            # metadata.txt found
            depsfile = mu.read()
            mu.close()

            # urllib2 gives us a bytelike object instead of a string. Have to consider that
            try:
                depsfile = depsfile.decode("utf-8")
            except AttributeError:
                pass

            deps = depsfile.split("\n")
            for line in deps:
                if line.startswith("workbenches="):
                    depswb = line.split("=")[1].split(",")
                    for wb in depswb:
                        if wb.strip():
                            if not wb.strip() in FreeCADGui.listWorkbenches().keys():
                                if not wb.strip()+"Workbench" in FreeCADGui.listWorkbenches().keys():
                                    ok = False
                                    message += translate("AddonsInstaller", "Missing workbench") + ": " + wb + ", "
                elif line.startswith("pylibs="):
                    depspy = line.split("=")[1].split(",")
                    for pl in depspy:
                        if pl.strip():
                            try:
                                __import__(pl.strip())
                            except ImportError:
                                ok = False
                                message += translate("AddonsInstaller", "Missing python module") + ": " + pl + ", "
                elif line.startswith("optionalpylibs="):
                    opspy = line.split("=")[1].split(",")
                    for pl in opspy:
                        if pl.strip():
                            try:
                                __import__(pl.strip())
                            except ImportError:
                                message += translate("AddonsInstaller",
                                                     "Missing optional python module (doesn't prevent installing)")
                                message += ": " + pl + ", "
        if message and (not ok):
            message = translate("AddonsInstaller", "Some errors were found that prevent to install this workbench")
            message += ": <b>" + message + "</b>. "
            message += translate("AddonsInstaller", "Please install the missing components first.")
        return ok, message

    def download(self, baseurl, clonedir):
        "downloads and unzip a zip version from a git repo"

        bakdir = None
        if os.path.exists(clonedir):
            bakdir = clonedir+".bak"
            if os.path.exists(bakdir):
                shutil.rmtree(bakdir)
            os.rename(clonedir, bakdir)
        os.makedirs(clonedir)
        zipurl = utils.get_zip_url(baseurl)
        if not zipurl:
            return translate("AddonsInstaller", "Error: Unable to locate zip from") + " " + baseurl
        try:
            print("Downloading "+zipurl)
            u = utils.urlopen(zipurl)
        except Exception:
            return translate("AddonsInstaller", "Error: Unable to download") + " " + zipurl
        if not u:
            return translate("AddonsInstaller", "Error: Unable to download") + " " + zipurl
        zfile = _stringio()
        zfile.write(u.read())
        zfile = zipfile.ZipFile(zfile)
        master = zfile.namelist()[0]  # github will put everything in a subfolder
        zfile.extractall(clonedir)
        u.close()
        zfile.close()
        for filename in os.listdir(clonedir+os.sep+master):
            shutil.move(clonedir+os.sep+master+os.sep+filename, clonedir+os.sep+filename)
        os.rmdir(clonedir+os.sep+master)
        if bakdir:
            shutil.rmtree(bakdir)
        return translate("AddonsInstaller", "Successfully installed") + " " + zipurl


class CheckSingleWorker(QtCore.QThread):
    """Worker to check for updates for a single addon"""

    updateAvailable = QtCore.Signal(bool)

    def __init__(self, name):

        QtCore.QThread.__init__(self)
        self.name = name

    def run(self):

        if not have_git:
            return
        FreeCAD.Console.PrintLog("Checking for available updates of the "+self.name+" addon\n")
        addondir = os.path.join(FreeCAD.getUserAppDataDir(), "Mod", self.name)
        if os.path.exists(addondir):
            if os.path.exists(addondir + os.sep + ".git"):
                gitrepo = git.Git(addondir)
                try:
                    gitrepo.fetch()
                    if "git pull" in gitrepo.status():
                        self.updateAvailable.emit(True)
                        return
                except Exception:
                    # can fail for any number of reasons, ex. not being online
                    pass
        self.updateAvailable.emit(False)

#  @}
