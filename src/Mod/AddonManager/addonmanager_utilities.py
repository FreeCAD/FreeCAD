# -*- coding: utf-8 -*-
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Gaël Écorchard <galou_breizh@yahoo.fr>             *
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

import os
import re
import ctypes
import ssl
from typing import Union, Optional

import urllib
from urllib.request import Request
from urllib.error import URLError
from urllib.parse import urlparse
from http.client import HTTPResponse

from PySide2 import QtCore, QtWidgets

import FreeCAD
import FreeCADGui

# check for SSL support

ssl_ctx = None
try:
    import ssl
except ImportError:
    pass
else:
    try:
        # ssl_ctx = ssl.create_default_context(cafile=certifi.where())
        # ssl_ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
        ssl_ctx = ssl.create_default_context(ssl.Purpose.SERVER_AUTH)
    except AttributeError:
        pass

#  @package AddonManager_utilities
#  \ingroup ADDONMANAGER
#  \brief Utilities to work across different platforms, providers and python versions
#  @{


def translate(context, text, disambig=None):
    "Main translation function"

    try:
        _encoding = QtWidgets.QApplication.UnicodeUTF8
    except AttributeError:
        return QtWidgets.QApplication.translate(context, text, disambig)
    else:
        return QtWidgets.QApplication.translate(context, text, disambig, _encoding)


def symlink(source, link_name):
    """Creates a symlink of a file, if possible. Note that it fails on most modern Windows installations"""

    if os.path.exists(link_name) or os.path.lexists(link_name):
        pass
    else:
        os_symlink = getattr(os, "symlink", None)
        if callable(os_symlink):
            os_symlink(source, link_name)
        else:
            # NOTE: This does not work on most normal Windows 10 and later installations, unless developer
            # mode is turned on. Make sure to catch any exception thrown and have a fallback plan.
            csl = ctypes.windll.kernel32.CreateSymbolicLinkW
            csl.argtypes = (ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_uint32)
            csl.restype = ctypes.c_ubyte
            flags = 1 if os.path.isdir(source) else 0
            # set the SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE flag
            # (see https://blogs.windows.com/buildingapps/2016/12/02/symlinks-windows-10/#joC5tFKhdXs2gGml.97)
            flags += 2
            if csl(link_name, source, flags) == 0:
                raise ctypes.WinError()


def getserver(url):
    """returns the server part of an url"""

    return "{uri.scheme}://{uri.netloc}/".format(uri=urlparse(url))


def update_macro_details(old_macro, new_macro):
    """Update a macro with information from another one

    Update a macro with information from another one, supposedly the same but
    from a different source. The first source is supposed to be git, the second
    one the wiki.
    """

    if old_macro.on_git and new_macro.on_git:
        FreeCAD.Console.PrintLog(
            'The macro "{}" is present twice in github, please report'.format(
                old_macro.name
            )
        )
    # We don't report macros present twice on the wiki because a link to a
    # macro is considered as a macro. For example, 'Perpendicular To Wire'
    # appears twice, as of 2018-05-05).
    old_macro.on_wiki = new_macro.on_wiki
    for attr in ["desc", "url", "code"]:
        if not hasattr(old_macro, attr):
            setattr(old_macro, attr, getattr(new_macro, attr))


def remove_directory_if_empty(dir):
    """Remove the directory if it is empty

    Directory FreeCAD.getUserMacroDir(True) will not be removed even if empty.
    """

    if dir == FreeCAD.getUserMacroDir(True):
        return
    if not os.listdir(dir):
        os.rmdir(dir)


def restart_freecad():
    "Shuts down and restarts FreeCAD"

    args = QtWidgets.QApplication.arguments()[1:]
    if FreeCADGui.getMainWindow().close():
        QtCore.QProcess.startDetached(
            QtWidgets.QApplication.applicationFilePath(), args
        )


def get_zip_url(repo):
    "Returns the location of a zip file from a repo, if available"

    parsedUrl = urlparse(repo.url)
    if parsedUrl.netloc == "github.com":
        return f"{repo.url}/archive/{repo.branch}.zip"
    elif (
        parsedUrl.netloc == "framagit.org"
        or parsedUrl.netloc == "gitlab.com"
        or parsedUrl.netloc == "salsa.debian.org"
    ):
        # https://framagit.org/freecad-france/mooc-workbench/-/archive/master/mooc-workbench-master.zip
        # https://salsa.debian.org/mess42/pyrate/-/archive/master/pyrate-master.zip
        return f"{repo.url}/-/archive/{repo.branch}/{repo.name}-{repo.branch}.zip"
    else:
        FreeCAD.Console.PrintLog(
            "Debug: addonmanager_utilities.get_zip_url: Unknown git host fetching zip URL:",
            parsedUrl.netloc,
            "\n",
        )
        return None


def recognized_git_location(repo) -> bool:
    parsed_url = urlparse(repo.url)
    if (
        parsed_url.netloc == "github.com"
        or parsed_url.netloc == "framagit.com"
        or parsed_url.netloc == "gitlab.com"
        or parsed_url.netloc == "salsa.debian.org"
    ):
        return True
    else:
        return False


def construct_git_url(repo, filename):
    "Returns a direct download link to a file in an online Git repo: works with github, gitlab, and framagit"

    parsed_url = urlparse(repo.url)
    if parsed_url.netloc == "github.com" or parsed_url.netloc == "framagit.com":
        return f"{repo.url}/raw/{repo.branch}/{filename}"
    elif parsed_url.netloc == "gitlab.com":
        return f"{repo.url}/-/raw/{repo.branch}/{filename}"
    elif parsed_url.netloc == "salsa.debian.org":
        # e.g. https://salsa.debian.org/joha2/pyrate/-/raw/master/package.xml
        return f"{repo.url}/-/raw/{repo.branch}/{filename}"
    else:
        FreeCAD.Console.PrintLog(
            "Debug: addonmanager_utilities.construct_git_url: Unknown git host:"
            + parsed_url.netloc
            + f" for file {filename}\n"
        )
    return None


def get_readme_url(repo):
    "Returns the location of a readme file"

    return construct_git_url(repo, "README.md")


def get_metadata_url(url):
    "Returns the location of a package.xml metadata file"

    return construct_git_url(url, "package.xml")


def get_desc_regex(repo):
    """Returns a regex string that extracts a WB description to be displayed in the description
    panel of the Addon manager, if the README could not be found"""

    parsedUrl = urlparse(repo.url)
    if parsedUrl.netloc == "github.com":
        return r'<meta property="og:description" content="(.*?)"'
    elif (
        parsedUrl.netloc == "framagit.org"
        or parsedUrl.netloc == "gitlab.com"
        or parsedUrl.netloc == "salsa.debian.org"
    ):
        return r'<meta.*?content="(.*?)".*?og:description.*?>'
    FreeCAD.Console.PrintLog(
        "Debug: addonmanager_utilities.get_desc_regex: Unknown git host:",
        repo.url,
        "\n",
    )
    return None


def get_readme_html_url(repo):
    """Returns the location of a html file containing readme"""

    parsedUrl = urlparse(repo.url)
    if parsedUrl.netloc == "github.com":
        return f"{repo.url}/blob/{repo.branch}/README.md"
    else:
        return None


def get_readme_regex(repo):
    """Return a regex string that extracts the contents to be displayed in the description
    panel of the Addon manager, from raw HTML data (the readme's html rendering usually)"""

    parsedUrl = urlparse(repo.url)
    if parsedUrl.netloc == "github.com":
        return "<article.*?>(.*?)</article>"
    else:
        return None


def fix_relative_links(text, base_url):
    """Replace markdown image relative links with
    absolute ones using the base URL"""

    new_text = ""
    for line in text.splitlines():
        for link in re.findall(r"!\[.*?\]\((.*?)\)", line) + re.findall(
            r"src\s*=\s*[\"'](.+?)[\"']", line
        ):
            parts = link.split("/")
            if len(parts) < 2 or not re.match(r"^http|^www|^.+\.|^/", parts[0]):
                newlink = os.path.join(base_url, link.lstrip("./"))
                line = line.replace(link, newlink)
                FreeCAD.Console.PrintLog("Debug: replaced " + link + " with " + newlink)
        new_text = new_text + "\n" + line
    return new_text


def repair_git_repo(repo_url: str, clone_dir: str) -> None:
    # Repair addon installed with raw download by adding the .git
    # directory to it

    try:
        import git

        # If GitPython is not installed, but the user has a directory named "git" in their Python path, they
        # may have the import succeed, but it will not be a real GitPython installation
        have_git = hasattr(git, "Repo")
        if not have_git:
            return
    except ImportError:
        return

    try:
        bare_repo = git.Repo.clone_from(
            repo_url, clone_dir + os.sep + ".git", bare=True
        )
        with bare_repo.config_writer() as cw:
            cw.set("core", "bare", False)
    except AttributeError:
        FreeCAD.Console.PrintLog(
            translate(
                "AddonsInstaller",
                "Outdated GitPython detected, consider upgrading with pip.",
            )
            + "\n"
        )
        cw = bare_repo.config_writer()
        cw.set("core", "bare", False)
        del cw
    except Exception as e:
        FreeCAD.Console.PrintWarning(
            translate("AddonsInstaller", "Failed to repair missing .git directory")
            + "\n"
        )
        FreeCAD.Console.PrintWarning(
            translate("AddonsInstaller", "Repository URL") + f": {repo_url}\n"
        )
        FreeCAD.Console.PrintWarning(
            translate("AddonsInstaller", "Clone directory") + f": {clone_dir}\n"
        )
        FreeCAD.Console.PrintWarning(e)
        return
    repo = git.Repo(clone_dir)
    repo.head.reset("--hard")


def warning_color_string() -> str:
    """A shade of red, adapted to darkmode if possible. Targets a minimum 7:1 contrast ratio."""

    warningColorString = "rgb(255,0,0)"
    if hasattr(QtWidgets.QApplication.instance(), "styleSheet"):
        # Qt 5.9 doesn't give a QApplication instance, so can't give the stylesheet info
        if "dark" in QtWidgets.QApplication.instance().styleSheet().lower():
            warningColorString = "rgb(255,105,97)"
        else:
            warningColorString = "rgb(215,0,21)"
    return warningColorString


def bright_color_string() -> str:
    """A shade of green, adapted to darkmode if possible. Targets a minimum 7:1 contrast ratio."""

    brightColorString = "rgb(0,255,0)"
    if hasattr(QtWidgets.QApplication.instance(), "styleSheet"):
        # Qt 5.9 doesn't give a QApplication instance, so can't give the stylesheet info
        if "dark" in QtWidgets.QApplication.instance().styleSheet().lower():
            brightColorString = "rgb(48,219,91)"
        else:
            brightColorString = "rgb(36,138,61)"
    return brightColorString


def attention_color_string() -> str:
    """A shade of orange, adapted to darkmode if possible. Targets a minimum 7:1 contrast ratio."""

    attentionColorString = "rgb(255,149,0)"
    if hasattr(QtWidgets.QApplication.instance(), "styleSheet"):
        # Qt 5.9 doesn't give a QApplication instance, so can't give the stylesheet info
        if "dark" in QtWidgets.QApplication.instance().styleSheet().lower():
            attentionColorString = "rgb(255,179,64)"
        else:
            attentionColorString = "rgb(255,149,0)"
    return attentionColorString


def get_macro_version_from_file(filename: str) -> str:
    re_version = re.compile(r"^__Version__\s*=\s*(['\"])(.*)\1", flags=re.IGNORECASE)
    with open(filename, "r", errors="ignore") as f:
        line_counter = 0
        max_lines_to_scan = 50
        while line_counter < max_lines_to_scan:
            line_counter += 1
            line = f.readline()
            if line.startswith("__"):
                match = re.match(re_version, line)
                if match:
                    return match.group(2)
    return ""


def update_macro_installation_details(repo) -> None:
    if repo is None or not hasattr(repo, "macro") or repo.macro is None:
        FreeCAD.Console.PrintLog(f"Requested macro details for non-macro object\n")
        return
    test_file_one = os.path.join(FreeCAD.getUserMacroDir(True), repo.macro.filename)
    test_file_two = os.path.join(
        FreeCAD.getUserMacroDir(True), "Macro_" + repo.macro.filename
    )
    if os.path.exists(test_file_one):
        repo.updated_timestamp = os.path.getmtime(test_file_one)
        repo.installed_version = get_macro_version_from_file(test_file_one)
    elif os.path.exists(test_file_two):
        repo.updated_timestamp = os.path.getmtime(test_file_two)
        repo.installed_version = get_macro_version_from_file(test_file_two)
    else:
        return


#  @}
