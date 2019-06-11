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

import os
import sys

# check for SSL support

ssl_ctx = None
try:
    import ssl
except ImportError:
    pass
else:
    try:
        ssl_ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    except AttributeError:
        pass



def translate(context, text, disambig=None):
    
    "Main translation function"
    
    from PySide import QtGui
    try:
        _encoding = QtGui.QApplication.UnicodeUTF8
    except AttributeError:
        return QtGui.QApplication.translate(context, text, disambig)
    else:
        return QtGui.QApplication.translate(context, text, disambig, _encoding)


def symlink(source, link_name):

    "creates a symlink of a file, if possible"

    if os.path.exists(link_name) or os.path.lexists(link_name):
        #print("macro already exists")
        pass
    else:
        os_symlink = getattr(os, "symlink", None)
        if callable(os_symlink):
            os_symlink(source, link_name)
        else:
            import ctypes
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

    if sys.version_info.major < 3:
        import urllib2
    else:
        import urllib.request as urllib2

    try:
        if ssl_ctx:
            u = urllib2.urlopen(url, context=ssl_ctx)
        else:
            u = urllib2.urlopen(url)
    except:
        return None
    else:
        return u


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
    if sys.version_info.major < 3:
        # In python2 the code is a bytes object.
        mode = 'wb'
    else:
        mode = 'w'
    try:
        with open(macro_path, mode) as macrofile:
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


def restartFreeCAD():

    "Shuts down and restarts FreeCAD"

    import FreeCADGui
    from PySide import QtGui,QtCore
    args = QtGui.QApplication.arguments()[1:]
    if FreeCADGui.getMainWindow().close():
        QtCore.QProcess.startDetached(QtGui.QApplication.applicationFilePath(),args)

