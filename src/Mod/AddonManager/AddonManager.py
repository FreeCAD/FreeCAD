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

from PySide import QtCore, QtGui
import sys, os, re, shutil, stat
import FreeCAD
if sys.version_info.major < 3:
    import urllib2
else:
    import urllib.request as urllib2
ctx = None
try:
    import ssl
except:
    pass
else:
    if hasattr(ssl,"create_default_context"):
        ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)

NOGIT = False # for debugging purposes, set this to True to always use http downloads

MACROS_BLACKLIST = ["BOLTS","WorkFeatures","how to install","PartsLibrary","FCGear"]



# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


def symlink(source, link_name):
    if os.path.exists(link_name):
        print("symlink already exists")
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


class AddonsInstaller(QtGui.QDialog):

    def __init__(self):
        QtGui.QDialog.__init__(self)
        self.repos = []
        self.macros = []
        self.setObjectName("AddonsInstaller")
        self.resize(326, 304)
        self.verticalLayout = QtGui.QVBoxLayout(self)
        self.tabWidget = QtGui.QTabWidget()
        self.verticalLayout.addWidget(self.tabWidget)
        self.listWorkbenches = QtGui.QListWidget()
        self.listWorkbenches.setIconSize(QtCore.QSize(16,16))
        self.tabWidget.addTab(self.listWorkbenches,"")
        self.listMacros = QtGui.QListWidget()
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
            QtGui.QDialog.reject(self)

    def retranslateUi(self):
        self.setWindowTitle(translate("AddonsInstaller","Addon manager"))
        self.labelDescription.setText(translate("AddonsInstaller", "Downloading addon list..."))
        self.buttonCheck.setToolTip(translate("AddonsInstaller", "Check for available updates"))
        self.buttonCancel.setText(translate("AddonsInstaller", "Close"))
        self.buttonInstall.setText(translate("AddonsInstaller", "Install / update"))
        self.buttonRemove.setText(translate("AddonsInstaller", "Remove"))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.listWorkbenches), translate("AddonsInstaller", "Workbenches"))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.listMacros), translate("AddonsInstaller", "Macros"))

    def update(self):
        self.listWorkbenches.clear()
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
            self.showmacro_worker = ShowMacroWorker(self.macros, idx)
            self.showmacro_worker.info_label.connect(self.set_information_label)
            self.showmacro_worker.update_macro.connect(self.update_macro)
            self.showmacro_worker.progressbar_show.connect(self.show_progress_bar)
            self.showmacro_worker.start()

    def switchtab(self,idx):
        if idx == 1:
            if not self.macros:
                self.listMacros.clear()
                self.macros = []
                self.macro_worker = MacroWorker()
                self.macro_worker.add_macro.connect(self.add_macro)
                self.macro_worker.info_label.connect(self.set_information_label)
                self.macro_worker.progressbar_show.connect(self.show_progress_bar)
                self.macro_worker.start()
            self.buttonCheck.setEnabled(False)
        else:
            self.buttonCheck.setEnabled(True)

    def update_repos(self, repos):
        self.repos = repos

    def add_macro(self, macro):
        self.macros.append(macro)
        if macro[1] == 1:
            self.listMacros.addItem(QtGui.QListWidgetItem(QtGui.QIcon.fromTheme("dialog-ok"),str(macro[0]) + str(" (Installed)")))
        else:
            self.listMacros.addItem("        "+str(macro[0]))

    def update_macro(self, idx, macro):
        self.macros[idx] = macro

    def showlink(self,link):
        "opens a link with the system browser"
        #print("clicked: ",link)
        QtGui.QDesktopServices.openUrl(QtCore.QUrl(link, QtCore.QUrl.TolerantMode))

    def install(self,repos=None):
        if self.tabWidget.currentIndex() == 0:
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
            macropath = FreeCAD.ParamGet('User parameter:BaseApp/Preferences/Macro').GetString("MacroPath",os.path.join(FreeCAD.ConfigGet("UserAppData"),"Macro"))
            if not os.path.isdir(macropath):
                os.makedirs(macropath)
            macro = self.macros[self.listMacros.currentRow()]
            if len(macro) < 5:
                self.labelDescription.setText(translate("AddonsInstaller", "Unable to install"))
                return
            macroname = "Macro_"+macro[0]+".FCMacro"
            macroname = macroname.replace(" ","_")
            macrofilename = os.path.join(macropath,macroname)
            macrofile = open(macrofilename,"wb")
            macrofile.write(macro[3])
            macrofile.close()
            self.labelDescription.setText(translate("AddonsInstaller", "Macro successfully installed. The macro is now available from the Macros dialog."))
        self.update_status()

    def show_progress_bar(self, state):
        if state == True:
            self.listWorkbenches.setEnabled(False)
            self.listMacros.setEnabled(False)
            self.buttonInstall.setEnabled(False)
            self.buttonRemove.setEnabled(False)
            self.buttonCheck.setEnabled(False)
            self.progressBar.show()
        else:
            self.progressBar.hide()
            self.listWorkbenches.setEnabled(True)
            self.listMacros.setEnabled(True)
            self.buttonInstall.setEnabled(True)
            self.buttonRemove.setEnabled(True)
            if self.tabWidget.currentIndex() == 0:
                self.buttonCheck.setEnabled(True)
            if self.listWorkbenches.isVisible():
                self.listWorkbenches.setFocus()
            else:
                self.listMacros.setFocus()

    def remove_readonly(self, func, path, _):
        "Remove read only file."
        os.chmod(path, stat.S_IWRITE)
        func(path)

    def remove(self):
        if self.tabWidget.currentIndex() == 0:
            idx = self.listWorkbenches.currentRow()
            basedir = FreeCAD.ConfigGet("UserAppData")
            moddir = basedir + os.sep + "Mod"
            clonedir = basedir + os.sep + "Mod" + os.sep + self.repos[idx][0]
            if os.path.exists(clonedir):
                shutil.rmtree(clonedir, onerror=self.remove_readonly)
                self.labelDescription.setText(translate("AddonsInstaller", "Addon successfully removed. Please restart FreeCAD"))
            else:
                self.labelDescription.setText(translate("AddonsInstaller", "Unable to remove this addon"))
        elif self.tabWidget.currentIndex() == 1:
            macropath = FreeCAD.ParamGet('User parameter:BaseApp/Preferences/Macro').GetString("MacroPath",os.path.join(FreeCAD.ConfigGet("UserAppData"),"Macro"))
            macro = self.macros[self.listMacros.currentRow()]
            if macro[1] != 1:
                return
            macroname = "Macro_"+macro[0]+".FCMacro"
            macroname = macroname.replace(" ","_")
            macrofilename = os.path.join(macropath,macroname)
            if os.path.exists(macrofilename):
                os.remove(macrofilename)
                self.labelDescription.setText(translate("AddonsInstaller", "Macro successfully removed."))
        self.update_status()

    def update_status(self):
        self.listWorkbenches.clear()
        self.listMacros.clear()
        moddir = FreeCAD.ConfigGet("UserAppData") + os.sep + "Mod"
        macropath = FreeCAD.ParamGet('User parameter:BaseApp/Preferences/Macro').GetString("MacroPath",os.path.join(FreeCAD.ConfigGet("UserAppData"),"Macro"))
        for wb in self.repos:
            if os.path.exists(os.path.join(moddir,wb[0])):
                self.listWorkbenches.addItem(QtGui.QListWidgetItem(QtGui.QIcon.fromTheme("dialog-ok"),str(wb[0]) + str(" (Installed)")))
                wb[2] = 1
            else:
                self.listWorkbenches.addItem("        "+str(wb[0]))
                wb[2] = 0
        for macro in self.macros:
            if os.path.exists(os.path.join(macropath,"Macro_"+macro[0].replace(" ","_")+".FCMacro")):
                self.listMacros.addItem(QtGui.QListWidgetItem(QtGui.QIcon.fromTheme("dialog-ok"),str(macro[0]) + str(" (Installed)")))
                macro[1] = 1
            else:
                self.listMacros.addItem("        "+str(macro[0]))
                macro[1] = 0

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
        if ctx:
            u = urllib2.urlopen("https://github.com/FreeCAD/FreeCAD-addons",context=ctx)
        else:
            u = urllib2.urlopen("https://github.com/FreeCAD/FreeCAD-addons")
        p = u.read()
        if sys.version_info.major >= 3 and isinstance(p, bytes):
            p = p.decode("utf-8")
        u.close()
        p = p.replace("\n"," ")
        p = re.findall("octicon-file-submodule(.*?)message",p)
        basedir = FreeCAD.ConfigGet("UserAppData")
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
            if ctx:
                u = urllib2.urlopen(url,context=ctx)
            else:
                u = urllib2.urlopen(url)
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
        basedir = FreeCAD.ConfigGet("UserAppData")
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


class MacroWorker(QtCore.QThread):

    add_macro = QtCore.Signal(object)
    info_label = QtCore.Signal(str)
    progressbar_show = QtCore.Signal(bool)

    def __init__(self):
        QtCore.QThread.__init__(self)

    def run(self):
        "populates the list of addons"
        self.info_label.emit("Downloading list of macros...")
        self.progressbar_show.emit(True)
        macropath = FreeCAD.ParamGet('User parameter:BaseApp/Preferences/Macro').GetString("MacroPath",os.path.join(FreeCAD.ConfigGet("UserAppData"),"Macro"))
        if ctx:
            u = urllib2.urlopen("https://www.freecadweb.org/wiki/Macros_recipes",context=ctx)
        else:
            u = urllib2.urlopen("https://www.freecadweb.org/wiki/Macros_recipes")
        p = u.read()
        if sys.version_info.major >= 3 and isinstance(p, bytes):
            p = p.decode("utf-8")
        u.close()
        macros = re.findall("title=\"(Macro.*?)\"",p)
        macros = [mac for mac in macros if (not("translated" in mac))]
        macros.sort(key=str.lower)
        for mac in macros:
            macname = mac[6:]
            macname = macname.replace("&amp;","&")
            if (not (macname in MACROS_BLACKLIST)) and (not("recipes" in macname.lower())):
                macfile = mac.replace(" ","_")+".FCMacro"
                if os.path.exists(os.path.join(macropath,macfile)):
                    installed = 1
                else:
                    installed = 0
                self.add_macro.emit([macname,installed])
        self.info_label.emit(translate("AddonsInstaller", "List of macros successfully retrieved."))
        self.progressbar_show.emit(False)
        self.stop = True


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
            self.info_label.emit(translate("AddonsInstaller", "Retrieving info from ") + str(url))
            if ctx:
                u = urllib2.urlopen(url,context=ctx)
            else:
                u = urllib2.urlopen(url)
            p = u.read()
            if sys.version_info.major >= 3 and isinstance(p, bytes):
                p = p.decode("utf-8")
            u.close()
            desc = re.findall("<meta property=\"og:description\" content=\"(.*?)\"",p)
            if desc:
                desc = desc[0]
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
                    clonedir = FreeCAD.ConfigGet("UserAppData") + os.sep + "Mod" + os.sep + repo[0]
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


class ShowMacroWorker(QtCore.QThread):

    info_label = QtCore.Signal(str)
    update_macro = QtCore.Signal(int,object)
    progressbar_show = QtCore.Signal(bool)

    def __init__(self, macros, idx):
        QtCore.QThread.__init__(self)
        self.macros = macros
        self.idx = idx

    def run(self):
        self.progressbar_show.emit(True)
        self.info_label.emit(translate("AddonsInstaller", "Retrieving description..."))
        if len(self.macros[self.idx]) > 2:
            desc = self.macros[self.idx][2]
            url = self.macros[self.idx][4]
        else:
            mac = self.macros[self.idx][0].replace(" ","_")
            mac = mac.replace("&","%26")
            mac = mac.replace("+","%2B")
            url = "https://www.freecadweb.org/wiki/Macro_"+mac
            self.info_label.emit("Retrieving info from " + str(url))
            if ctx:
                u = urllib2.urlopen(url,context=ctx)
            else:
                u = urllib2.urlopen(url)
            p = u.read()
            if sys.version_info.major >= 3 and isinstance(p, bytes):
                p = p.decode("utf-8")
            u.close()
            code = re.findall("<pre>(.*?)<\/pre>",p.replace("\n","--endl--"))
            if code:
                # code = code[0]
                # take the biggest code block
                code = sorted(code,key=len)[-1]
                code = code.replace("--endl--","\n")
            else:
                self.info_label.emit(translate("AddonsInstaller", "Unable to fetch the code of this macro."))
                self.progressbar_show.emit(False)
                self.stop = True
                return
            desc = re.findall("<td class=\"ctEven left macro-description\">(.*?)<\/td>",p.replace("\n"," "))
            if desc:
                desc = desc[0]
            else:
                self.info_label.emit(translate("AddonsInstaller", "Unable to retrieve a description for this macro."))
                desc = "No description available"
            # clean HTML escape codes
            try:
                from HTMLParser import HTMLParser
            except ImportError:
                from html.parser import HTMLParser
            try:
                code = code.decode("utf8")
                code = HTMLParser().unescape(code)
                code = code.encode("utf8")
                code = code.replace("\xc2\xa0", " ")
            except:
                FreeCAD.Console.PrintWarning(translate("AddonsInstaller", "Unable to clean macro code: ")+mac+"\n")
            self.update_macro.emit(self.idx,self.macros[self.idx]+[desc,code,url])
        if self.macros[self.idx][1] == 1 :
            message = "<strong>" + translate("AddonsInstaller", "<strong>This addon is already installed.") + "</strong><br>" + desc + ' - <a href="' + url + '"><span style="word-wrap: break-word;width:15em;text-decoration: underline; color:#0000ff;">' + url + '</span></a>'
        else:
            message = desc + ' - <a href="' + url + '"><span style="word-wrap: break-word;width:15em;text-decoration: underline; color:#0000ff;">' + url + '</span></a>'
        self.info_label.emit( message )
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
        except:
            self.info_label.emit("GitPython not found.")
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
            basedir = FreeCAD.ConfigGet("UserAppData")
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
                    macrodir = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Macro").GetString("MacroPath",os.path.join(FreeCAD.ConfigGet("UserAppData"),"Macro"))
                    if not os.path.exists(macrodir):
                        os.makedirs(macrodir)
                    for f in os.listdir(clonedir):
                        if f.lower().endswith(".fcmacro"):
                            symlink(clonedir+os.sep+f,macrodir+os.sep+f)
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
            if ctx:
                mu = urllib2.urlopen(depsurl,context=ctx)
            else:
                mu = urllib2.urlopen(depsurl)
        except urllib2.HTTPError:
            # no metadata.txt, we just continue without deps checking
            pass
        else:
            # metadata.txt found
            depsfile = mu.read()
            mu.close()
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
        try:
            import StringIO as io
        except ImportError: # StringIO is not available with python3
            import io
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
            if ctx:
                u = urllib2.urlopen(zipurl,context=ctx)
            else:
                u = urllib2.urlopen(zipurl)
        except:
            return translate("AddonsInstaller", "Error: Unable to download") + " " + zipurl
        zfile = io.StringIO()
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
