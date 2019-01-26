#!/usr/bin/env python
# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
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

from __future__ import print_function

__title__="FreeCAD Addon Manager Module"
__author__ = "Yorik van Havre","Jonathan Wiedemann","Kurt Kremitzki"
__url__ = "http://www.freecadweb.org"

'''
FreeCAD Addon Manager Module

It will fetch its contents from https://github.com/FreeCAD/FreeCAD-addons
You need a working internet connection, and the GitPython package
installed.
'''
import os
import re
import shutil
import stat
import sys
import tempfile

from PySide import QtCore, QtGui
import FreeCAD
import FreeCADGui
if sys.version_info.major < 3:
    import urllib2
else:
    import urllib.request as urllib2

from addonmanager_macro import Macro
from addonmanager_utilities import translate
from addonmanager_utilities import urlopen

NOGIT = False # for debugging purposes, set this to True to always use http downloads

MACROS_BLACKLIST = ["BOLTS","WorkFeatures","how to install","PartsLibrary","FCGear"]
OBSOLETE = ["assembly2"]

if sys.version_info.major < 3:
    import StringIO as io
    _stringio = io.StringIO
else:
    import io
    _stringio = io.BytesIO


def symlink(source, link_name):
    if os.path.exists(link_name):
        print("macro already exists")
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
    - macro: a addonmanager_macro.Macro instance
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
    - macro: a addonmanager_macro.Macro instance
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


class AddonsInstaller(QtGui.QDialog):

    def __init__(self):
        QtGui.QDialog.__init__(self)
        self.repos = []
        self.macros = []
        self.macro_repo_dir = tempfile.mkdtemp()

        self.setObjectName("AddonsInstaller")
        self.resize(626, 404)
        self.verticalLayout = QtGui.QVBoxLayout(self)
        self.tabWidget = QtGui.QTabWidget()
        self.verticalLayout.addWidget(self.tabWidget)
        self.listWorkbenches = QtGui.QListWidget()
        self.listWorkbenches.setIconSize(QtCore.QSize(16,16))
        self.tabWidget.addTab(self.listWorkbenches,"")
        self.listMacros = QtGui.QListWidget()
        self.listMacros.setSortingEnabled(False)
        self.listMacros.setIconSize(QtCore.QSize(16,16))
        self.tabWidget.addTab(self.listMacros,"")
        self.labelDescription = QtGui.QLabel()
        self.labelDescription.setMinimumSize(QtCore.QSize(0, 75))
        self.labelDescription.setAlignment(QtCore.Qt.AlignLeading|QtCore.Qt.AlignLeft|QtCore.Qt.AlignTop)
        self.labelDescription.setWordWrap(True)
        self.verticalLayout.addWidget(self.labelDescription)
        self.doUpdate = []

        self.progressBar = QtGui.QProgressBar(self)
        #self.progressBar.setProperty("value", 24)
        self.progressBar.setObjectName("progressBar")
        #self.progressBar.hide()
        self.progressBar.setRange(0,0)
        self.verticalLayout.addWidget(self.progressBar)

        self.horizontalLayout = QtGui.QHBoxLayout()
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)

        self.buttonExecute = QtGui.QPushButton()
        icon = QtGui.QIcon.fromTheme("execute")
        self.buttonExecute.setIcon(icon)
        self.buttonExecute.setEnabled(False)
        self.horizontalLayout.addWidget(self.buttonExecute)

        self.buttonCheck = QtGui.QPushButton()
        icon = QtGui.QIcon.fromTheme("reload")
        self.buttonCheck.setIcon(icon)
        self.horizontalLayout.addWidget(self.buttonCheck)
        self.buttonCheck.hide()
        self.buttonInstall = QtGui.QPushButton()
        icon = QtGui.QIcon.fromTheme("download")
        self.buttonInstall.setIcon(icon)
        self.horizontalLayout.addWidget(self.buttonInstall)
        self.buttonRemove = QtGui.QPushButton()
        icon = QtGui.QIcon.fromTheme("edit-delete")
        self.buttonRemove.setIcon(icon)
        self.horizontalLayout.addWidget(self.buttonRemove)
        self.buttonCancel = QtGui.QPushButton()
        icon = QtGui.QIcon.fromTheme("cancel")
        self.buttonCancel.setIcon(icon)
        self.buttonCancel.setDefault(True)
        self.horizontalLayout.addWidget(self.buttonCancel)
        self.verticalLayout.addLayout(self.horizontalLayout)

        self.retranslateUi()

        QtCore.QObject.connect(self.buttonExecute, QtCore.SIGNAL("clicked()"), self.executemacro)
        QtCore.QObject.connect(self.buttonCancel, QtCore.SIGNAL("clicked()"), self.reject)
        QtCore.QObject.connect(self.buttonInstall, QtCore.SIGNAL("clicked()"), self.install)
        QtCore.QObject.connect(self.buttonRemove, QtCore.SIGNAL("clicked()"), self.remove)
        QtCore.QObject.connect(self.labelDescription, QtCore.SIGNAL("linkActivated(QString)"), self.showlink)
        QtCore.QObject.connect(self.listWorkbenches, QtCore.SIGNAL("currentRowChanged(int)"), self.show)
        QtCore.QObject.connect(self.tabWidget, QtCore.SIGNAL("currentChanged(int)"), self.switchtab)
        QtCore.QObject.connect(self.listMacros, QtCore.SIGNAL("currentRowChanged(int)"), self.show_macro)
        QtCore.QObject.connect(self.buttonCheck, QtCore.SIGNAL("clicked()"), self.check_updates)
        QtCore.QMetaObject.connectSlotsByName(self)

        self.update()

        if not NOGIT:
            try:
                import git
            except:
                self.buttonCheck.hide()
            else:
                self.buttonCheck.show()

        # center the dialog over the FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.move(mw.frameGeometry().topLeft() + mw.rect().center() - self.rect().center())


    def reject(self):
        # ensure all threads are finished before closing
        oktoclose = True
        for worker in ["update_worker","check_worker","show_worker","showmacro_worker",
                       "macro_worker","install_worker"]:
            if hasattr(self,worker):
                thread = getattr(self,worker)
                if thread:
                    if not thread.isFinished():
                        oktoclose = False
        if oktoclose:
            if hasattr(self,"install_worker"):
                QtGui.QMessageBox.information(self, translate("AddonsInstaller","Addon manager"), translate("AddonsInstaller","Please restart FreeCAD for changes to take effect."))
            shutil.rmtree(self.macro_repo_dir,onerror=self.remove_readonly)
            QtGui.QDialog.reject(self)

    def retranslateUi(self):
        self.setWindowTitle(translate("AddonsInstaller","Addon manager"))
        self.labelDescription.setText(translate("AddonsInstaller", "Downloading addon list..."))
        self.buttonExecute.setText(translate("AddonsInstaller", "Execute"))
        self.buttonExecute.setToolTip(translate("AddonsInstaller", "This button runs the selected macro (which must be installed first)"))
        self.buttonCheck.setToolTip(translate("AddonsInstaller", "Check for available updates"))
        self.buttonCancel.setText(translate("AddonsInstaller", "Close"))
        self.buttonInstall.setText(translate("AddonsInstaller", "Install / update"))
        self.buttonRemove.setText(translate("AddonsInstaller", "Remove"))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.listWorkbenches), translate("AddonsInstaller", "Workbenches"))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.listMacros), translate("AddonsInstaller", "Macros"))

    def update(self):
        self.listWorkbenches.clear()
        self.buttonExecute.setEnabled(False)
        self.repos = []
        self.info_worker = InfoWorker()
        self.info_worker.addon_repos.connect(self.update_repos)
        self.update_worker = UpdateWorker()
        self.update_worker.info_label.connect(self.set_information_label)
        self.update_worker.addon_repo.connect(self.add_addon_repo)
        self.update_worker.progressbar_show.connect(self.show_progress_bar)
        self.update_worker.start()

    def check_updates(self):
        if self.tabWidget.currentIndex() == 0:
            if not self.doUpdate:
                self.check_worker = CheckWBWorker(self.repos)
                self.check_worker.mark.connect(self.mark)
                self.check_worker.info_label.connect(self.set_information_label)
                self.check_worker.progressbar_show.connect(self.show_progress_bar)
                self.check_worker.start()
            else:
                self.install(self.doUpdate)

    def add_addon_repo(self, addon_repo):
        self.repos.append(addon_repo)
        if addon_repo[2] == 1 :
            self.listWorkbenches.addItem(QtGui.QListWidgetItem(QtGui.QIcon.fromTheme("dialog-ok"),str(addon_repo[0]) + str(" (Installed)")))
        else:
            self.listWorkbenches.addItem("        "+str(addon_repo[0]))

    def set_information_label(self, label):
        self.labelDescription.setText(label)
        if self.listWorkbenches.isVisible():
            self.listWorkbenches.setFocus()
        else:
            self.listMacros.setFocus()

    def show(self,idx):
        if self.repos and idx >= 0:
            self.show_worker = ShowWorker(self.repos, idx)
            self.show_worker.info_label.connect(self.set_information_label)
            self.show_worker.addon_repos.connect(self.update_repos)
            self.show_worker.progressbar_show.connect(self.show_progress_bar)
            self.show_worker.start()

    def show_macro(self,idx):
        if self.macros and idx >= 0:
            self.showmacro_worker = GetMacroDetailsWorker(self.macros[idx])
            self.showmacro_worker.info_label.connect(self.set_information_label)
            self.showmacro_worker.progressbar_show.connect(self.show_progress_bar)
            self.showmacro_worker.start()

    def switchtab(self,idx):
        if idx == 1:
            if not self.macros:
                self.listMacros.clear()
                self.macros = []
                self.macro_worker = FillMacroListWorker(self.macro_repo_dir)
                self.macro_worker.add_macro_signal.connect(self.add_macro)
                self.macro_worker.info_label_signal.connect(self.set_information_label)
                self.macro_worker.progressbar_show.connect(self.show_progress_bar)
                self.macro_worker.start()
            self.buttonCheck.setEnabled(False)
        else:
            self.buttonCheck.setEnabled(True)

    def update_repos(self, repos):
        self.repos = repos

    def add_macro(self, macro):
        if macro in self.macros:
            # The macro is already in the list of macros.
            old_macro = self.macros[self.macros.index(macro)]
            update_macro_details(old_macro, macro)
        else:
            self.macros.append(macro)
            if macro.is_installed():
                self.listMacros.addItem(QtGui.QListWidgetItem(QtGui.QIcon.fromTheme('dialog-ok'), macro.name + str(' (Installed)')))
            else:
                self.listMacros.addItem("        "+macro.name)

    def showlink(self,link):
        """opens a link with the system browser"""
        #print("clicked: ",link)
        QtGui.QDesktopServices.openUrl(QtCore.QUrl(link, QtCore.QUrl.TolerantMode))

    def install(self,repos=None):
        if self.tabWidget.currentIndex() == 0:
            # Tab "Workbenches".
            idx = None
            if repos:
                idx = []
                for repo in repos:
                    for i,r in enumerate(self.repos):
                        if r[0] == repo:
                            idx.append(i)
            else:
                idx = self.listWorkbenches.currentRow()
            if idx != None:
                if hasattr(self,"install_worker"):
                    if self.install_worker.isRunning():
                        return
                self.install_worker = InstallWorker(self.repos, idx)
                self.install_worker.info_label.connect(self.set_information_label)
                self.install_worker.progressbar_show.connect(self.show_progress_bar)
                self.install_worker.start()
        elif self.tabWidget.currentIndex() == 1:
            # Tab "Macros".
            macro = self.macros[self.listMacros.currentRow()]
            if install_macro(macro, self.macro_repo_dir):
                self.labelDescription.setText(translate("AddonsInstaller", "Macro successfully installed. The macro is now available from the Macros dialog."))
            else:
                self.labelDescription.setText(translate("AddonsInstaller", "Unable to install"))
        self.update_status(soft=True)

    def show_progress_bar(self, state):
        if state == True:
            self.listWorkbenches.setEnabled(False)
            self.listMacros.setEnabled(False)
            self.buttonExecute.setEnabled(False)
            self.buttonInstall.setEnabled(False)
            self.buttonRemove.setEnabled(False)
            self.buttonCheck.setEnabled(False)
            self.progressBar.show()
        else:
            self.progressBar.hide()
            self.listWorkbenches.setEnabled(True)
            self.listMacros.setEnabled(True)
            self.buttonExecute.setEnabled(False)
            self.buttonInstall.setEnabled(True)
            self.buttonRemove.setEnabled(True)
            if self.tabWidget.currentIndex() == 0:
                self.buttonCheck.setEnabled(True)
            if self.listWorkbenches.isVisible():
                self.listWorkbenches.setFocus()
            else:
                self.listMacros.setFocus()
                self.buttonExecute.setEnabled(True)

    def executemacro(self):
        if self.tabWidget.currentIndex() == 1:
            # Tab "Macros".
            macro = self.macros[self.listMacros.currentRow()]
            if not macro.is_installed():
                # Macro not installed, nothing to do.
                return
            macro_path = os.path.join(FreeCAD.getUserMacroDir(True), macro.filename)
            if os.path.exists(macro_path):
                macro_path = macro_path.replace("\\","/")

                FreeCADGui.open(str(macro_path))
                self.hide()
                FreeCADGui.SendMsgToActiveView("Run")
        else:
            self.buttonExecute.setEnabled(False)

    def remove_readonly(self, func, path, _):
        "Remove read only file."
        os.chmod(path, stat.S_IWRITE)
        func(path)

    def remove(self):
        if self.tabWidget.currentIndex() == 0:
            # Tab "Workbenches".
            idx = self.listWorkbenches.currentRow()
            basedir = FreeCAD.getUserAppDataDir()
            moddir = basedir + os.sep + "Mod"
            clonedir = moddir + os.sep + self.repos[idx][0]
            if os.path.exists(clonedir):
                shutil.rmtree(clonedir, onerror=self.remove_readonly)
                self.labelDescription.setText(translate("AddonsInstaller", "Addon successfully removed. Please restart FreeCAD"))
            else:
                self.labelDescription.setText(translate("AddonsInstaller", "Unable to remove this addon"))
        elif self.tabWidget.currentIndex() == 1:
            # Tab "Macros".
            macro = self.macros[self.listMacros.currentRow()]
            if remove_macro(macro):
                self.labelDescription.setText(translate('AddonsInstaller', 'Macro successfully removed.'))
            else:
                self.labelDescription.setText(translate('AddonsInstaller', 'Macro could not be removed.'))
        self.update_status(soft=True)

    def update_status(self,soft=False):

        "Updates the list of wbs/macros. If soft is true, items are not recreated (and therefore display text no triggered)"

        moddir = FreeCAD.getUserAppDataDir() + os.sep + "Mod"
        if soft:
            for i in range(self.listWorkbenches.count()):
                txt = self.listWorkbenches.item(i).text().strip()
                if txt.endswith(" (Installed)"):
                    txt = txt[:-12]
                elif txt.endswith(" (Update available)"):
                    txt = txt[:-19]
                if os.path.exists(os.path.join(moddir,txt)):
                    self.listWorkbenches.item(i).setText(txt+" (Installed)")
                    self.listWorkbenches.item(i).setIcon(QtGui.QIcon.fromTheme("dialog-ok"))
                else:
                    self.listWorkbenches.item(i).setText("        "+txt)
                    self.listWorkbenches.item(i).setIcon(QtGui.QIcon())
            for i in range(self.listMacros.count()):
                txt = self.listMacros.item(i).text().strip()
                if txt.endswith(" (Installed)"):
                    txt = txt[:-12]
                elif txt.endswith(" (Update available)"):
                    txt = txt[:-19]
                if os.path.exists(os.path.join(moddir,txt)):
                    self.listMacros.item(i).setText(txt+" (Installed)")
                    self.listMacros.item(i).setIcon(QtGui.QIcon.fromTheme("dialog-ok"))
                else:
                    self.listMacros.item(i).setText("        "+txt)
                    self.listMacros.item(i).setIcon(QtGui.QIcon())
        else:
            self.listWorkbenches.clear()
            self.listMacros.clear()
            for wb in self.repos:
                if os.path.exists(os.path.join(moddir,wb[0])):
                    self.listWorkbenches.addItem(QtGui.QListWidgetItem(QtGui.QIcon.fromTheme("dialog-ok"),str(wb[0]) + str(" (Installed)")))
                    wb[2] = 1
                else:
                    self.listWorkbenches.addItem("        "+str(wb[0]))
                    wb[2] = 0
            for macro in self.macros:
                if macro.is_installed():
                    self.listMacros.addItem(QtGui.QListWidgetItem(QtGui.QIcon.fromTheme('dialog-ok'), macro.name + str(' (Installed)')))
                else:
                    self.listMacros.addItem("        "+macro.name)

    def mark(self,repo):
        for i in range(self.listWorkbenches.count()):
            w = self.listWorkbenches.item(i)
            if w.text().startswith(str(repo)):
                w.setText(str(repo) + str(" (Update available)"))
                w.setIcon(QtGui.QIcon.fromTheme("reload"))
                if not repo in self.doUpdate:
                    self.doUpdate.append(repo)

class UpdateWorker(QtCore.QThread):

    info_label = QtCore.Signal(str)
    addon_repo = QtCore.Signal(object)
    progressbar_show = QtCore.Signal(bool)

    def __init__(self):
        QtCore.QThread.__init__(self)

    def run(self):
        "populates the list of addons"
        self.progressbar_show.emit(True)
        u = urlopen("https://github.com/FreeCAD/FreeCAD-addons")
        p = u.read()
        if sys.version_info.major >= 3 and isinstance(p, bytes):
            p = p.decode("utf-8")
        u.close()
        p = p.replace("\n"," ")
        p = re.findall("octicon-file-submodule(.*?)message",p)
        basedir = FreeCAD.getUserAppDataDir()
        moddir = basedir + os.sep + "Mod"
        repos = []
        for l in p:
            #name = re.findall("data-skip-pjax=\"true\">(.*?)<",l)[0]
            name = re.findall("title=\"(.*?) @",l)[0]
            self.info_label.emit(name)
            #url = re.findall("title=\"(.*?) @",l)[0]
            url = "https://github.com/" + re.findall("href=\"\/(.*?)\/tree",l)[0]
            addondir = moddir + os.sep + name
            #print ("found:",name," at ",url)
            if not os.path.exists(addondir):
                state = 0
            else:
                state = 1
            repos.append([name,url,state])
        if not repos:
            self.info_label.emit(translate("AddonsInstaller", "Unable to download addon list."))
        else:
            repos = sorted(repos, key=lambda s: s[0].lower())
            for repo in repos:
                self.addon_repo.emit(repo)
            self.info_label.emit(translate("AddonsInstaller", "Workbenches list was updated."))
        self.progressbar_show.emit(False)
        self.stop = True


class InfoWorker(QtCore.QThread):
    addon_repos = QtCore.Signal(object)

    def __init__(self):
        QtCore.QThread.__init__(self)

    def run(self):
        i = 0
        for repo in self.repos:
            url = repo[1]
            u = urlopen(url)
            p = u.read()
            if sys.version_info.major >= 3 and isinstance(p, bytes):
                p = p.decode("utf-8")
            u.close()
            desc = re.findall("<meta property=\"og:description\" content=\"(.*?)\"",p)
            if desc:
                desc = desc[0]
            else:
                desc = "Unable to retrieve addon description"
            self.repos[i].append(desc)
            i += 1
            self.addon_repos.emit(self.repos)
        self.stop = True


class CheckWBWorker(QtCore.QThread):
    info_label = QtCore.Signal(str)
    mark = QtCore.Signal(str)
    progressbar_show = QtCore.Signal(bool)

    def __init__(self,repos):
        QtCore.QThread.__init__(self)
        self.repos = repos

    def run(self):
        if NOGIT:
            self.stop = True
            return
        try:
            import git
        except:
            self.stop = True
            return
        self.progressbar_show.emit(True)
        basedir = FreeCAD.getUserAppDataDir()
        moddir = basedir + os.sep + "Mod"
        self.info_label.emit(translate("AddonsInstaller", "Checking for new versions..."))
        upds = []
        gitpython_warning = False
        for repo in self.repos:
            if repo[2] == 1: #installed
                self.info_label.emit(translate("AddonsInstaller","Checking repo")+" "+repo[0]+"...")
                clonedir = moddir + os.sep + repo[0]
                if os.path.exists(clonedir):
                    if not os.path.exists(clonedir + os.sep + '.git'):
                        # Repair addon installed with raw download
                        bare_repo = git.Repo.clone_from(repo[1], clonedir + os.sep + '.git', bare=True)
                        try:
                            with bare_repo.config_writer() as cw:
                                cw.set('core', 'bare', False)
                        except AttributeError:
                            if not gitpython_warning:
                                FreeCAD.Console.PrintWarning(translate("AddonsInstaller", "Outdated GitPython detected, consider upgrading with pip.")+"\n")
                                gitpython_warning = True
                            cw = bare_repo.config_writer()
                            cw.set('core', 'bare', False)
                            del cw
                        repo = git.Repo(clonedir)
                        repo.head.reset('--hard')
                    gitrepo = git.Git(clonedir)
                    gitrepo.fetch()
                    if "git pull" in gitrepo.status():
                        self.mark.emit(repo[0])
                        upds.append(repo[0])
        self.progressbar_show.emit(False)
        if upds:
            self.info_label.emit(str(len(upds))+" "+translate("AddonsInstaller", "update(s) available")+": "+",".join(upds)+". "+translate("AddonsInstaller","Press the update button again to update them all at once."))
        else:
            self.info_label.emit(translate("AddonsInstaller","Everything is up to date"))
        self.stop = True


class FillMacroListWorker(QtCore.QThread):
    """Populates the list of macros
    """
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
            self.info_label_signal.emit(translate('AddonsInstaller', 'List of macros successfully retrieved.'))
        self.progressbar_show.emit(False)
        self.stop = True

    def retrieve_macros_from_git(self):
        """Retrieve macros from FreeCAD-macros.git

        Emits a signal for each macro in
        https://github.com/FreeCAD/FreeCAD-macros.git
        """
        try:
            import git
        except ImportError:
            self.info_label_signal.emit("GitPython not installed! Cannot retrieve macros from git")
            FreeCAD.Console.PrintWarning(translate('AddonInstaller', 'GitPython not installed! Cannot retrieve macros from git')+"\n")
            return

        self.info_label_signal.emit('Downloading list of macros for git...')
        git.Repo.clone_from('https://github.com/FreeCAD/FreeCAD-macros.git', self.repo_dir)
        for dirpath, _, filenames in os.walk(self.repo_dir):
             if '.git' in dirpath:
                 continue
             for filename in filenames:
                 if filename.lower().endswith('.fcmacro'):
                    macro = Macro(filename[:-8])  # Remove ".FCMacro".
                    macro.on_git = True
                    macro.src_filename = os.path.join(dirpath, filename)
                    self.macros.append(macro)

    def retrieve_macros_from_wiki(self):
        """Retrieve macros from the wiki

        Read the wiki and emit a signal for each found macro.
        Reads only the page https://www.freecadweb.org/wiki/Macros_recipes.
        """
        self.info_label_signal.emit("Downloading list of macros...")
        self.progressbar_show.emit(True)
        u = urlopen("https://www.freecadweb.org/wiki/Macros_recipes")
        p = u.read()
        u.close()
        if sys.version_info.major >= 3 and isinstance(p, bytes):
            p = p.decode("utf-8")
        macros = re.findall('title="(Macro.*?)"', p)
        macros = [mac for mac in macros if ('translated' not in mac)]
        for mac in macros:
            macname = mac[6:]  # Remove "Macro ".
            macname = macname.replace("&amp;","&")
            if (macname not in MACROS_BLACKLIST) and ('recipes' not in macname.lower()):
                macro = Macro(macname)
                macro.on_wiki = True
                self.macros.append(macro)


class ShowWorker(QtCore.QThread):

    info_label = QtCore.Signal(str)
    addon_repos = QtCore.Signal(object)
    progressbar_show = QtCore.Signal(bool)

    def __init__(self, repos, idx):
        QtCore.QThread.__init__(self)
        self.repos = repos
        self.idx = idx

    def run(self):
        self.progressbar_show.emit(True)
        self.info_label.emit(translate("AddonsInstaller", "Retrieving description..."))
        if len(self.repos[self.idx]) == 4:
            desc = self.repos[self.idx][3]
        else:
            url = self.repos[self.idx][1]
            self.info_label.emit(translate("AddonsInstaller", "Retrieving info from") + ' ' + str(url))
            u = urlopen(url)
            p = u.read()
            if sys.version_info.major >= 3 and isinstance(p, bytes):
                p = p.decode("utf-8")
            u.close()
            desc = re.findall("<meta property=\"og:description\" content=\"(.*?)\"",p)
            if desc:
                desc = desc[0]
                if self.repos[self.idx][0] in OBSOLETE:
                    desc += " <b>This add-on is marked as obsolete</b> - This usually means it is no longer maintained, and some more advanced add-on in this list provides the same functionality."
            else:
                desc = "Unable to retrieve addon description"
            self.repos[self.idx].append(desc)
            self.addon_repos.emit(self.repos)
        if self.repos[self.idx][2] == 1:
            upd = False
            # checking for updates
            if not NOGIT:
                try:
                    import git
                except:
                    pass
                else:
                    repo = self.repos[self.idx]
                    clonedir = FreeCAD.getUserAppDataDir() + os.sep + "Mod" + os.sep + repo[0]
                    if os.path.exists(clonedir):
                        if not os.path.exists(clonedir + os.sep + '.git'):
                            # Repair addon installed with raw download
                            bare_repo = git.Repo.clone_from(repo[1], clonedir + os.sep + '.git', bare=True)
                            try:
                                with bare_repo.config_writer() as cw:
                                    cw.set('core', 'bare', False)
                            except AttributeError:
                                FreeCAD.Console.PrintWarning(translate("AddonsInstaller", "Outdated GitPython detected, consider upgrading with pip.")+"\n")
                                cw = bare_repo.config_writer()
                                cw.set('core', 'bare', False)
                                del cw
                            repo = git.Repo(clonedir)
                            repo.head.reset('--hard')
                        gitrepo = git.Git(clonedir)
                        gitrepo.fetch()
                        if "git pull" in gitrepo.status():
                            upd = True
            if upd:
                message = "<strong>" + translate("AddonsInstaller", "An update is available for this addon.") + "</strong><br>" + desc + ' - <a href="' + self.repos[self.idx][1] + '"><span style="word-wrap: break-word;width:15em;text-decoration: underline; color:#0000ff;">' + self.repos[self.idx][1] + '</span></a>'
            else:
                message = "<strong>" + translate("AddonsInstaller", "This addon is already installed.") + "</strong><br>" + desc + ' - <a href="' + self.repos[self.idx][1] + '"><span style="word-wrap: break-word;width:15em;text-decoration: underline; color:#0000ff;">' + self.repos[self.idx][1] + '</span></a>'
        else:
            message = desc + ' - <a href="' + self.repos[self.idx][1] + '"><span style="word-wrap: break-word;width:15em;text-decoration: underline; color:#0000ff;">' + self.repos[self.idx][1] + '</span></a>'
        self.info_label.emit( message )
        self.progressbar_show.emit(False)
        self.stop = True


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
            self.info_label.emit(translate('AddonsInstaller', 'Retrieving info from git'))
            self.macro.fill_details_from_file(self.macro.src_filename)
        if not self.macro.parsed and self.macro.on_wiki:
            self.info_label.emit(translate('AddonsInstaller', 'Retrieving info from wiki'))
            mac = self.macro.name.replace(' ', '_')
            mac = mac.replace('&', '%26')
            mac = mac.replace('+', '%2B')
            url = 'https://www.freecadweb.org/wiki/Macro_' + mac
            self.macro.fill_details_from_wiki(url)
        if self.macro.is_installed():
            already_installed_msg = ('<strong>'
                    + translate("AddonsInstaller", "This addon is already installed.")
                    + '</strong><br>')
        else:
            already_installed_msg = ''
        message = (already_installed_msg
                + self.macro.desc
                + ' - <a href="'
                + self.macro.url
                + '"><span style="word-wrap: break-word;width:15em;text-decoration: underline; color:#0000ff;">'
                + self.macro.url
                + '</span></a>')
        self.info_label.emit(message)
        self.progressbar_show.emit(False)
        self.stop = True


class InstallWorker(QtCore.QThread):

    info_label = QtCore.Signal(str)
    progressbar_show = QtCore.Signal(bool)

    def __init__(self, repos, idx):
        QtCore.QThread.__init__(self)
        self.idx = idx
        self.repos = repos

    def run(self):
        "installs or updates the selected addon"
        git = None
        try:
            import git
        except Exception as e:
            self.info_label.emit("GitPython not found.")
            print(e)
            FreeCAD.Console.PrintWarning(translate("AddonsInstaller","GitPython not found. Using standard download instead.")+"\n")
            try:
                import zipfile
            except:
                self.info_label.emit("no zip support.")
                FreeCAD.Console.PrintError(translate("AddonsInstaller","Your version of python doesn't appear to support ZIP files. Unable to proceed.")+"\n")
                return
            try:
                import StringIO as io
            except ImportError: # StringIO is not available with python3
                import io
        if not isinstance(self.idx,list):
            self.idx = [self.idx]
        for idx in self.idx:
            if idx < 0:
                return
            if not self.repos:
                return
            if NOGIT:
                git = None
            basedir = FreeCAD.getUserAppDataDir()
            moddir = basedir + os.sep + "Mod"
            if not os.path.exists(moddir):
                os.makedirs(moddir)
            clonedir = moddir + os.sep + self.repos[idx][0]
            self.progressbar_show.emit(True)
            if os.path.exists(clonedir):
                self.info_label.emit("Updating module...")
                if git:
                    if not os.path.exists(clonedir + os.sep + '.git'):
                        # Repair addon installed with raw download
                        bare_repo = git.Repo.clone_from(self.repos[idx][1], clonedir + os.sep + '.git', bare=True)
                        try:
                            with bare_repo.config_writer() as cw:
                                cw.set('core', 'bare', False)
                        except AttributeError:
                            FreeCAD.Console.PrintWarning(translate("AddonsInstaller", "Outdated GitPython detected, consider upgrading with pip.")+"\n")
                            cw = bare_repo.config_writer()
                            cw.set('core', 'bare', False)
                            del cw
                        repo = git.Repo(clonedir)
                        repo.head.reset('--hard')
                    repo = git.Git(clonedir)
                    answer = repo.pull()

                    # Update the submodules for this repository
                    repo_sms = git.Repo(clonedir)
                    for submodule in repo_sms.submodules:
                        submodule.update(init=True, recursive=True)
                else:
                    answer = self.download(self.repos[idx][1],clonedir)
            else:
                self.info_label.emit("Checking module dependencies...")
                depsok,answer = self.checkDependencies(self.repos[idx][1])
                if depsok:
                    if git:
                        self.info_label.emit("Cloning module...")
                        repo = git.Repo.clone_from(self.repos[idx][1], clonedir, branch='master')

                        # Make sure to clone all the submodules as well
                        if repo.submodules:
                            repo.submodule_update(recursive=True)
                    else:
                        self.info_label.emit("Downloading module...")
                        self.download(self.repos[idx][1],clonedir)
                    answer = translate("AddonsInstaller", "Workbench successfully installed. Please restart FreeCAD to apply the changes.")
            # symlink any macro contained in the module to the macros folder
            macro_dir = FreeCAD.getUserMacroDir(True)
            if not os.path.exists(macro_dir):
                os.makedirs(macro_dir)
            if os.path.exists(clonedir):
                for f in os.listdir(clonedir):
                    if f.lower().endswith(".fcmacro"):
                        print("copying macro:",f)
                        symlink(os.path.join(clonedir, f), os.path.join(macro_dir, f))
                        FreeCAD.ParamGet('User parameter:Plugins/'+self.repos[idx][0]).SetString("destination",clonedir)
                        answer += translate("AddonsInstaller", "A macro has been installed and is available the Macros menu") + ": <b>"
                        answer += f + "</b>"
            self.progressbar_show.emit(False)
            self.info_label.emit(answer)
        self.stop = True

    def checkDependencies(self,baseurl):
        "checks if the repo contains a metadata.txt and check its contents"
        import FreeCADGui
        ok = True
        message = ""
        depsurl = baseurl.replace("github.com","raw.githubusercontent.com")
        if not depsurl.endswith("/"):
            depsurl += "/"
        depsurl += "master/metadata.txt"
        try:
            mu = urlopen(depsurl)
        except urllib2.HTTPError:
            # no metadata.txt, we just continue without deps checking
            pass
        else:
            # metadata.txt found
            depsfile = mu.read()
            mu.close()

            # urllib2 gives us a bytelike object instead of a string. Have to consider that
            try:
                depsfile = depsfile.decode('utf-8')
            except AttributeError:
                pass

            deps = depsfile.split("\n")
            for l in deps:
                if l.startswith("workbenches="):
                    depswb = l.split("=")[1].split(",")
                    for wb in depswb:
                        if wb.strip():
                            if not wb.strip() in FreeCADGui.listWorkbenches().keys():
                                if not wb.strip()+"Workbench" in FreeCADGui.listWorkbenches().keys():
                                    ok = False
                                    message += translate("AddonsInstaller","Missing workbench") + ": " + wb + ", "
                elif l.startswith("pylibs="):
                    depspy = l.split("=")[1].split(",")
                    for pl in depspy:
                        if pl.strip():
                            try:
                                __import__(pl.strip())
                            except:
                                ok = False
                                message += translate("AddonsInstaller","Missing python module") +": " + pl + ", "
                elif l.startswith("optionalpylibs="):
                    opspy = l.split("=")[1].split(",")
                    for pl in opspy:
                        if pl.strip():
                            try:
                                __import__(pl.strip())
                            except:
                                message += translate("AddonsInstaller","Missing optional python module (doesn't prevent installing)") +": " + pl + ", "
        if message and (not ok):
            message = translate("AddonsInstaller", "Some errors were found that prevent to install this workbench") + ": <b>" + message + "</b>. "
            message += translate("AddonsInstaller","Please install the missing components first.")
        return ok, message

    def download(self,giturl,clonedir):
        "downloads and unzip from github"
        import zipfile
        bakdir = None
        if os.path.exists(clonedir):
            bakdir = clonedir+".bak"
            if os.path.exists(bakdir):
                shutil.rmtree(bakdir)
            os.rename(clonedir,bakdir)
        os.makedirs(clonedir)
        zipurl = giturl+"/archive/master.zip"
        try:
            print("Downloading "+zipurl)
            u = urlopen(zipurl)
        except:
            return translate("AddonsInstaller", "Error: Unable to download") + " " + zipurl
        zfile = _stringio()
        zfile.write(u.read())
        zfile = zipfile.ZipFile(zfile)
        master = zfile.namelist()[0] # github will put everything in a subfolder
        zfile.extractall(clonedir)
        u.close()
        zfile.close()
        for filename in os.listdir(clonedir+os.sep+master):
            shutil.move(clonedir+os.sep+master+os.sep+filename, clonedir+os.sep+filename)
        os.rmdir(clonedir+os.sep+master)
        if bakdir:
            shutil.rmtree(bakdir)
        return translate("AddonsInstaller", "Successfully installed") + " " + zipurl


def launchAddonMgr():
    # first use dialog
    readWarning = FreeCAD.ParamGet('User parameter:Plugins/addonsRepository').GetBool('readWarning',False)
    if not readWarning:
        if QtGui.QMessageBox.warning(None,"FreeCAD",translate("AddonsInstaller", "The addons that can be installed here are not officially part of FreeCAD, and are not reviewed by the FreeCAD team. Make sure you know what you are installing!"), QtGui.QMessageBox.Cancel | QtGui.QMessageBox.Ok) != QtGui.QMessageBox.StandardButton.Cancel:
            FreeCAD.ParamGet('User parameter:Plugins/addonsRepository').SetBool('readWarning',True)
            readWarning = True

    if readWarning:
        dialog = AddonsInstaller()
        dialog.exec_()
