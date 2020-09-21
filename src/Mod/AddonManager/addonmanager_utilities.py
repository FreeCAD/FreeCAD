# -*- coding: utf-8 -*-
#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2018 Gaël Écorchard <galou_breizh@yahoo.fr>             *
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

import codecs
import os
import re
import shutil
import sys
import ctypes

if sys.version_info.major < 3:
    import urllib2
    from urllib2 import URLError
    from urlparse import urlparse
else:
    import urllib.request as urllib2
    from urllib.error import URLError
    from urllib.parse import urlparse

from PySide import QtGui, QtCore

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
        _encoding = QtGui.QApplication.UnicodeUTF8
    except AttributeError:
        return QtGui.QApplication.translate(context, text, disambig)
    else:
        return QtGui.QApplication.translate(context, text, disambig, _encoding)


def symlink(source, link_name):
    "creates a symlink of a file, if possible"

    if os.path.exists(link_name) or os.path.lexists(link_name):
        # print("macro already exists")
        pass
    else:
        os_symlink = getattr(os, "symlink", None)
        if callable(os_symlink):
            os_symlink(source, link_name)
        else:
            csl = ctypes.windll.kernel32.CreateSymbolicLinkW
            csl.argtypes = (ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_uint32)
            csl.restype = ctypes.c_ubyte
            flags = 1 if os.path.isdir(source) else 0
            # set the SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE flag
            # (see https://blogs.windows.com/buildingapps/2016/12/02/symlinks-windows-10/#joC5tFKhdXs2gGml.97)
            flags += 2
            if csl(link_name, source, flags) == 0:
                raise ctypes.WinError()


def urlopen(url):
    """Opens an url with urllib2"""

    timeout = 5

    # Proxy an ssl configuration
    pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
    if pref.GetBool("NoProxyCheck", True):
        proxies = {}
    else:
        if pref.GetBool("SystemProxyCheck", False):
            proxy = urllib2.getproxies()
            proxies = {"http": proxy.get('http'), "https": proxy.get('http')}
        elif pref.GetBool("UserProxyCheck", False):
            proxy = pref.GetString("ProxyUrl", "")
            proxies = {"http": proxy, "https": proxy}

    if ssl_ctx:
        handler = urllib2.HTTPSHandler(context=ssl_ctx)
    else:
        handler = {}
    proxy_support = urllib2.ProxyHandler(proxies)
    opener = urllib2.build_opener(proxy_support, handler)
    urllib2.install_opener(opener)

    # Url opening
    req = urllib2.Request(url,
                          headers={'User-Agent': "Magic Browser"})
    try:
        u = urllib2.urlopen(req, timeout=timeout)
    except Exception:
        return None
    else:
        return u


def getserver(url):
    """returns the server part of an url"""

    return '{uri.scheme}://{uri.netloc}/'.format(uri=urlparse(url))


def update_macro_details(old_macro, new_macro):
    """Update a macro with information from another one

    Update a macro with information from another one, supposedly the same but
    from a different source. The first source is supposed to be git, the second
    one the wiki.
    """

    if old_macro.on_git and new_macro.on_git:
        FreeCAD.Console.PrintWarning('The macro "{}" is present twice in github, please report'.format(old_macro.name))
    # We don't report macros present twice on the wiki because a link to a
    # macro is considered as a macro. For example, 'Perpendicular To Wire'
    # appears twice, as of 2018-05-05).
    old_macro.on_wiki = new_macro.on_wiki
    for attr in ['desc', 'url', 'code']:
        if not hasattr(old_macro, attr):
            setattr(old_macro, attr, getattr(new_macro, attr))


def install_macro(macro, macro_repo_dir):
    """Install a macro and all its related files

    Returns True if the macro was installed correctly.

    Parameters
    ----------
    - macro: an addonmanager_macro.Macro instance
    """

    if not macro.code:
        return False
    macro_dir = FreeCAD.getUserMacroDir(True)
    if not os.path.isdir(macro_dir):
        try:
            os.makedirs(macro_dir)
        except OSError:
            return False
    macro_path = os.path.join(macro_dir, macro.filename)
    try:
        with codecs.open(macro_path, 'w', 'utf-8') as macrofile:
            macrofile.write(macro.code)
    except IOError:
        return False
    # Copy related files, which are supposed to be given relative to
    # macro.src_filename.
    base_dir = os.path.dirname(macro.src_filename)
    for other_file in macro.other_files:
        dst_dir = os.path.join(macro_dir, os.path.dirname(other_file))
        if not os.path.isdir(dst_dir):
            try:
                os.makedirs(dst_dir)
            except OSError:
                return False
        src_file = os.path.join(base_dir, other_file)
        dst_file = os.path.join(macro_dir, other_file)
        try:
            shutil.copy(src_file, dst_file)
        except IOError:
            return False
    return True


def remove_macro(macro):
    """Remove a macro and all its related files

    Returns True if the macro was removed correctly.

    Parameters
    ----------
    - macro: an addonmanager_macro.Macro instance
    """

    if not macro.is_installed():
        # Macro not installed, nothing to do.
        return True
    macro_dir = FreeCAD.getUserMacroDir(True)
    macro_path = os.path.join(macro_dir, macro.filename)
    macro_path_with_macro_prefix = os.path.join(macro_dir, 'Macro_' + macro.filename)
    if os.path.exists(macro_path):
        os.remove(macro_path)
    elif os.path.exists(macro_path_with_macro_prefix):
        os.remove(macro_path_with_macro_prefix)
    # Remove related files, which are supposed to be given relative to
    # macro.src_filename.
    for other_file in macro.other_files:
        dst_file = os.path.join(macro_dir, other_file)
        remove_directory_if_empty(os.path.dirname(dst_file))
        os.remove(dst_file)
    return True


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

    args = QtGui.QApplication.arguments()[1:]
    if FreeCADGui.getMainWindow().close():
        QtCore.QProcess.startDetached(QtGui.QApplication.applicationFilePath(), args)


def get_zip_url(baseurl):
    "Returns the location of a zip file from a repo, if available"

    url = getserver(baseurl).strip("/")
    if url.endswith("github.com"):
        return baseurl+"/archive/master.zip"
    elif url.endswith("framagit.org") or url.endswith("gitlab.com"):
        # https://framagit.org/freecad-france/mooc-workbench/-/archive/master/mooc-workbench-master.zip
        reponame = baseurl.strip("/").split("/")[-1]
        return baseurl+"/-/archive/master/"+reponame+"-master.zip"
    else:
        print("Debug: addonmanager_utilities.get_zip_url: Unknown git host:", url)
        return None


def get_readme_url(url):
    "Returns the location of a readme file"

    if "github" in url or "framagit" in url or "gitlab" in url:
        return url+"/raw/master/README.md"
    else:
        print("Debug: addonmanager_utilities.get_readme_url: Unknown git host:", url)
    return None


def get_desc_regex(url):
    """Returns a regex string that extracts a WB description to be displayed in the description
    panel of the Addon manager, if the README could not be found"""

    if "github" in url:
        return r'<meta property="og:description" content="(.*?)"'
    elif "framagit" in url or "gitlab" in url:
        return r'<meta.*?content="(.*?)".*?og:description.*?>'
    print("Debug: addonmanager_utilities.get_desc_regex: Unknown git host:", url)
    return None


def get_readme_html_url(url):
    """Returns the location of a html file containing readme"""

    if "github" in url:
        return url + "/blob/master/README.md"
    else:
        print("Debug: addonmanager_utilities.get_readme_html_url: Unknown git host:", url)
        return None


def get_readme_regex(url):
    """Return a regex string that extracts the contents to be displayed in the description
    panel of the Addon manager, from raw HTML data (the readme's html rendering usually)"""

    if ("github" in url):
        return "<article.*?>(.*?)</article>"
    else:
        print("Debug: addonmanager_utilities.get_readme_regex: Unknown git host:", url)
        return None


def fix_relative_links(text, base_url):
    """Replace markdown image relative links with
    absolute ones using the base URL"""

    new_text = ""
    for line in text.splitlines():
        for link in (re.findall(r"!\[.*?\]\((.*?)\)", line) +
                     re.findall(r"src\s*=\s*[\"'](.+?)[\"']", line)):
            parts = link.split('/')
            if len(parts) < 2 or not re.match(r"^http|^www|^.+\.|^/", parts[0]):
                newlink = os.path.join(base_url, link.lstrip('./'))
                line = line.replace(link, newlink)
                print("Debug: replaced " + link + " with " + newlink)
        new_text = new_text + '\n' + line
    return new_text

#  @}
