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
You need a working internet connection, and optionally the GitPython package
installed.
'''

import os
import re
import shutil
import stat
import sys
import tempfile

from addonmanager_utilities import translate
from addonmanager_utilities import update_macro_details
from addonmanager_utilities import install_macro
from addonmanager_utilities import remove_macro
from addonmanager_utilities import remove_directory_if_empty
from addonmanager_utilities import restartFreeCAD
from addonmanager_workers import *

def QT_TRANSLATE_NOOP(ctx,txt):
    return txt


class CommandAddonManager:

    "The Addon Manager command"

    def GetResources(self):

        return {'Pixmap': 'AddonManager',
                'MenuText': QT_TRANSLATE_NOOP("Std_AddonMgr", '&Addon manager'),
                'ToolTip': QT_TRANSLATE_NOOP("Std_AddonMgr", 'Manage external workbenches and macros'),
                'Group': 'Tools'}

    def Activated(self):

        # display first use dialog if needed

        from PySide import QtGui
        readWarning = FreeCAD.ParamGet('User parameter:Plugins/addonsRepository').GetBool('readWarning',False)
        if not readWarning:
            if QtGui.QMessageBox.warning(None,"FreeCAD",translate("AddonsInstaller", "The addons that can be installed here are not officially part of FreeCAD, and are not reviewed by the FreeCAD team. Make sure you know what you are installing!"), QtGui.QMessageBox.Cancel | QtGui.QMessageBox.Ok) != QtGui.QMessageBox.StandardButton.Cancel:
                FreeCAD.ParamGet('User parameter:Plugins/addonsRepository').SetBool('readWarning',True)
                readWarning = True

        if readWarning:
            self.launch()

    def launch(self):

        import FreeCADGui
        from PySide import QtGui
        
        # create the dialog
        self.dialog = FreeCADGui.PySideUic.loadUi(os.path.join(os.path.dirname(__file__),"AddonManager.ui"))

        # cleanup the leftovers from previous runs
        self.repos = []
        self.macros = []
        self.macro_repo_dir = tempfile.mkdtemp()
        self.doUpdate = []
        self.addon_removed = False
        for worker in ["update_worker","check_worker","show_worker","showmacro_worker","macro_worker","install_worker"]:
            if hasattr(self,worker):
                thread = getattr(self,worker)
                if thread:
                    if thread.isFinished():
                        setattr(self,worker,None)
        self.dialog.tabWidget.setCurrentIndex(0)
        # these 2 settings to prevent loading an addon description on start (let the user click something first)
        self.firsttime = True
        self.firstmacro = True

        # restore window geometry and splitter state from stored state
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        w = pref.GetInt("WindowWidth",600)
        h = pref.GetInt("WindowHeight",480)
        self.dialog.resize(w,h)
        sl = pref.GetInt("SplitterLeft",298)
        sr = pref.GetInt("SplitterRight",274)
        self.dialog.splitter.setSizes([sl,sr])

        # set nice icons to everything, by theme with fallback to FreeCAD icons
        self.dialog.setWindowIcon(QtGui.QIcon(":/icons/AddonManager.svg"))
        self.dialog.buttonExecute.setIcon(QtGui.QIcon.fromTheme("execute",QtGui.QIcon(":/icons/button_valid.svg")))
        self.dialog.buttonUninstall.setIcon(QtGui.QIcon.fromTheme("cancel",QtGui.QIcon(":/icons/edit_Cancel.svg")))
        self.dialog.buttonInstall.setIcon(QtGui.QIcon.fromTheme("download",QtGui.QIcon(":/icons/edit_OK.svg")))
        self.dialog.buttonUpdateAll.setIcon(QtGui.QIcon(":/icons/button_valid.svg"))
        self.dialog.buttonConfigure.setIcon(QtGui.QIcon(":/icons/preferences-system.svg"))
        self.dialog.tabWidget.setTabIcon(0,QtGui.QIcon(":/icons/Group.svg"))
        self.dialog.tabWidget.setTabIcon(1,QtGui.QIcon(":/icons/applications-python.svg"))


        # enable/disable stuff
        self.dialog.buttonExecute.setEnabled(False)
        self.dialog.buttonUninstall.setEnabled(False)
        self.dialog.buttonInstall.setEnabled(False)
        self.dialog.buttonUpdateAll.setEnabled(False)

        # connect slots
        self.dialog.buttonExecute.clicked.connect(self.executemacro)
        self.dialog.rejected.connect(self.reject)
        self.dialog.buttonInstall.clicked.connect(self.install)
        self.dialog.buttonUninstall.clicked.connect(self.remove)
        self.dialog.buttonUpdateAll.clicked.connect(self.apply_updates)
        self.dialog.listWorkbenches.currentRowChanged.connect(self.show)
        self.dialog.tabWidget.currentChanged.connect(self.switchtab)
        self.dialog.listMacros.currentRowChanged.connect(self.show_macro)
        self.dialog.buttonConfigure.clicked.connect(self.show_config)
        
        # allow to open links in browser
        self.dialog.description.setOpenLinks(True)
        self.dialog.description.setOpenExternalLinks(True)

        # center the dialog over the FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.dialog.move(mw.frameGeometry().topLeft() + mw.rect().center() - self.dialog.rect().center())

        # populate the list
        self.update()

        # rock 'n roll!!!
        self.dialog.exec_()

    def reject(self):

        "called when the window has been closed"

        # save window geometry and splitter state for next use
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("WindowWidth",self.dialog.width())
        pref.SetInt("WindowHeight",self.dialog.height())
        pref.SetInt("SplitterLeft",self.dialog.splitter.sizes()[0])
        pref.SetInt("SplitterRight",self.dialog.splitter.sizes()[1])

        # ensure all threads are finished before closing
        oktoclose = True
        for worker in ["update_worker","check_worker","show_worker","showmacro_worker",
                       "macro_worker","install_worker"]:
            if hasattr(self,worker):
                thread = getattr(self,worker)
                if thread:
                    if not thread.isFinished():
                        oktoclose = False

        # all threads have finished
        if oktoclose:
            if (hasattr(self,"install_worker") and self.install_worker) or (hasattr(self,"addon_removed") and self.addon_removed):
                # display restart dialog
                from PySide import QtGui,QtCore
                m = QtGui.QMessageBox()
                m.setWindowTitle(translate("AddonsInstaller","Addon manager"))
                m.setWindowIcon(QtGui.QIcon(":/icons/AddonManager.svg"))
                m.setText(translate("AddonsInstaller","You must restart FreeCAD for changes to take effect. Press Ok to restart FreeCAD now, or Cancel to restart later."))
                m.setIcon(m.Warning)
                m.setStandardButtons(m.Ok | m.Cancel)
                m.setDefaultButton(m.Cancel)
                ret = m.exec_()
                if ret == m.Ok:
                    shutil.rmtree(self.macro_repo_dir,onerror=self.remove_readonly)
                    # restart FreeCAD after a delay to give time to this dialog to close
                    QtCore.QTimer.singleShot(1000,restartFreeCAD)
            try:
                shutil.rmtree(self.macro_repo_dir,onerror=self.remove_readonly)
            except:
                pass

        return True

    def update(self):

        "updates the list of workbenches"

        self.dialog.listWorkbenches.clear()
        self.dialog.buttonExecute.setEnabled(False)
        self.repos = []
        self.update_worker = UpdateWorker()
        self.update_worker.info_label.connect(self.show_information)
        self.update_worker.addon_repo.connect(self.add_addon_repo)
        self.update_worker.progressbar_show.connect(self.show_progress_bar)
        self.update_worker.done.connect(self.check_updates)
        self.update_worker.start()

    def check_updates(self):

        "checks every installed addon for available updates"

        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        if pref.GetBool("AutoCheck",False) and not self.doUpdate:
            if hasattr(self,"check_worker"):
                thread = self.check_worker
                if thread:
                    if not thread.isFinished():
                        return
            self.dialog.buttonUpdateAll.setText(translate("AddonsInstaller","Checking for updates..."))
            self.check_worker = CheckWBWorker(self.repos)
            self.check_worker.mark.connect(self.mark)
            self.check_worker.enable.connect(self.enable_updates)
            self.check_worker.start()

    def apply_updates(self):
        
        "apply all available updates"
        
        if self.doUpdate:
            self.install(self.doUpdate)
            self.dialog.buttonUpdateAll.setEnabled(False)

    def enable_updates(self,num):
        
        "enables the update button"
        
        if num:
            self.dialog.buttonUpdateAll.setText(translate("AddonsInstaller","Apply")+" "+str(num)+" "+translate("AddonsInstaller","update(s)"))
            self.dialog.buttonUpdateAll.setEnabled(True)
        else:
            self.dialog.buttonUpdateAll.setText(translate("AddonsInstaller","No update available"))
            self.dialog.buttonUpdateAll.setEnabled(False)

    def add_addon_repo(self, addon_repo):

        "adds a workbench to the list"

        from PySide import QtGui
        self.repos.append(addon_repo)
        if addon_repo[2] == 1 :
            self.dialog.listWorkbenches.addItem(QtGui.QListWidgetItem(QtGui.QIcon(":/icons/button_valid.svg"),str(addon_repo[0]) + str(" ("+translate("AddonsInstaller","Installed")+")")))
        else:
            self.dialog.listWorkbenches.addItem(QtGui.QListWidgetItem(QtGui.QIcon(":/icons/Group.svg"),str(addon_repo[0])))

    def show_information(self, label):

        "shows text in the information pane"

        self.dialog.description.setText(label)
        if self.dialog.listWorkbenches.isVisible():
            self.dialog.listWorkbenches.setFocus()
        else:
            self.dialog.listMacros.setFocus()

    def show(self,idx):

        "loads information of a given workbench"

        # this function is triggered also when the list is populated, prevent that here
        if idx == 0 and self.firsttime:
            self.dialog.listWorkbenches.setCurrentRow(-1)
            self.firsttime = False
            return

        if self.repos and idx >= 0:
            if hasattr(self,"show_worker"):
                # kill existing show worker (might still be busy loading images...)
                if self.show_worker:
                    self.show_worker.exit()
            self.show_worker = ShowWorker(self.repos, idx)
            self.show_worker.info_label.connect(self.show_information)
            self.show_worker.addon_repos.connect(self.update_repos)
            self.show_worker.progressbar_show.connect(self.show_progress_bar)
            self.show_worker.start()
            self.dialog.buttonInstall.setEnabled(True)
            self.dialog.buttonUninstall.setEnabled(True)

    def show_macro(self,idx):

        "loads information of a given macro"

        # this function is triggered when the list is populated, prevent that here
        if idx == 0 and self.firstmacro:
            self.dialog.listMacros.setCurrentRow(-1)
            self.firstmacro = False
            return

        if self.macros and idx >= 0:
            if hasattr(self,"showmacro_worker"):
                if self.showmacro_worker:
                    if not self.showmacro_worker.isFinished():
                        self.showmacro_worker.exit()
                    if not self.showmacro_worker.isFinished():
                        return
            self.showmacro_worker = GetMacroDetailsWorker(self.macros[idx])
            self.showmacro_worker.info_label.connect(self.show_information)
            self.showmacro_worker.progressbar_show.connect(self.show_progress_bar)
            self.showmacro_worker.start()
            self.dialog.buttonInstall.setEnabled(True)
            self.dialog.buttonUninstall.setEnabled(True)
            if self.macros[idx].is_installed():
                self.dialog.buttonExecute.setEnabled(True)
            else:
                self.dialog.buttonExecute.setEnabled(False)

    def switchtab(self,idx):

        "does what needs to be done when switching tabs"

        if idx == 1:
            if not self.macros:
                self.dialog.listMacros.clear()
                self.macros = []
                self.macro_worker = FillMacroListWorker(self.macro_repo_dir)
                self.macro_worker.add_macro_signal.connect(self.add_macro)
                self.macro_worker.info_label_signal.connect(self.show_information)
                self.macro_worker.progressbar_show.connect(self.show_progress_bar)
                self.macro_worker.start()
                self.dialog.listMacros.setCurrentRow(0)

    def update_repos(self, repos):

        "convenience function to update the internal list of workbenches"

        self.repos = repos

    def add_macro(self, macro):

        "adds a macro to the list"

        if macro.name:
            if macro in self.macros:
                # The macro is already in the list of macros.
                old_macro = self.macros[self.macros.index(macro)]
                update_macro_details(old_macro, macro)
            else:
                from PySide import QtGui
                self.macros.append(macro)
                if macro.is_installed():
                    self.dialog.listMacros.addItem(QtGui.QListWidgetItem(QtGui.QIcon(":/icons/button_valid.svg"), macro.name + str(' (Installed)')))
                else:
                    self.dialog.listMacros.addItem(QtGui.QListWidgetItem(QtGui.QIcon(":/icons/applications-python.svg"),macro.name))

    def install(self,repos=None):

        "installs a workbench or macro"

        if self.dialog.tabWidget.currentIndex() == 0:
            # Tab "Workbenches".
            idx = None
            if repos:
                idx = []
                for repo in repos:
                    for i,r in enumerate(self.repos):
                        if r[0] == repo:
                            idx.append(i)
            else:
                idx = self.dialog.listWorkbenches.currentRow()
            if idx != None:
                if hasattr(self,"install_worker") and self.install_worker:
                    if self.install_worker.isRunning():
                        return
                self.install_worker = InstallWorker(self.repos, idx)
                self.install_worker.info_label.connect(self.show_information)
                self.install_worker.progressbar_show.connect(self.show_progress_bar)
                self.install_worker.mark_recompute.connect(self.mark_recompute)
                self.install_worker.start()

        elif self.dialog.tabWidget.currentIndex() == 1:
            # Tab "Macros".
            macro = self.macros[self.dialog.listMacros.currentRow()]
            if install_macro(macro, self.macro_repo_dir):
                self.dialog.description.setText(translate("AddonsInstaller", "Macro successfully installed. The macro is now available from the Macros dialog."))
            else:
                self.dialog.description.setText(translate("AddonsInstaller", "Unable to install"))

    def show_progress_bar(self, state):

        "shows or hides the progress bar"

        if state == True:
            self.dialog.tabWidget.setEnabled(False)
            self.dialog.buttonInstall.setEnabled(False)
            self.dialog.buttonUninstall.setEnabled(False)
            self.dialog.progressBar.show()
        else:
            self.dialog.progressBar.hide()
            self.dialog.tabWidget.setEnabled(True)
            if not (self.firsttime and self.firstmacro):
                self.dialog.buttonInstall.setEnabled(True)
                self.dialog.buttonUninstall.setEnabled(True)
            if self.dialog.listWorkbenches.isVisible():
                self.dialog.listWorkbenches.setFocus()
            else:
                self.dialog.listMacros.setFocus()

    def executemacro(self):

        "executes a selected macro"

        import FreeCADGui
        if self.dialog.tabWidget.currentIndex() == 1:
            # Tab "Macros".
            macro = self.macros[self.dialog.listMacros.currentRow()]
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
            self.dialog.buttonExecute.setEnabled(False)

    def remove_readonly(self, func, path, _):

        "Remove a read-only file."

        os.chmod(path, stat.S_IWRITE)
        func(path)

    def remove(self):

        "uninstalls a macro or workbench"

        if self.dialog.tabWidget.currentIndex() == 0:
            # Tab "Workbenches".
            idx = self.dialog.listWorkbenches.currentRow()
            basedir = FreeCAD.getUserAppDataDir()
            moddir = basedir + os.sep + "Mod"
            clonedir = moddir + os.sep + self.repos[idx][0]
            if os.path.exists(clonedir):
                shutil.rmtree(clonedir, onerror=self.remove_readonly)
                self.dialog.description.setText(translate("AddonsInstaller", "Addon successfully removed. Please restart FreeCAD"))
            else:
                self.dialog.description.setText(translate("AddonsInstaller", "Unable to remove this addon"))

        elif self.tabWidget.currentIndex() == 1:
            # Tab "Macros".
            macro = self.macros[self.dialog.listMacros.currentRow()]
            if remove_macro(macro):
                self.dialog.description.setText(translate('AddonsInstaller', 'Macro successfully removed.'))
            else:
                self.dialog.description.setText(translate('AddonsInstaller', 'Macro could not be removed.'))
        self.update_status(soft=True)
        self.addon_removed = True # A value to trigger the restart message on dialog close

    def mark_recompute(self,addon):

        "marks an addon in the list as installed but needs recompute"

        for i in range(self.dialog.listWorkbenches.count()):
            txt = self.dialog.listWorkbenches.item(i).text().strip()
            if txt.endswith(" ("+translate("AddonsInstaller","Installed")+")"):
                txt = txt[:-12]
            elif txt.endswith(" ("+translate("AddonsInstaller","Update available")+")"):
                txt = txt[:-19]
            if txt == addon:
                from PySide import QtGui
                self.dialog.listWorkbenches.item(i).setText(txt+" ("+translate("AddonsInstaller","Restart required")+")")
                self.dialog.listWorkbenches.item(i).setIcon(QtGui.QIcon(":/icons/edit-undo.svg"))

    def update_status(self,soft=False):

        """Updates the list of workbenches/macros.
           If soft is true, items are not recreated (and therefore display text isn't triggered)"
        """

        moddir = FreeCAD.getUserAppDataDir() + os.sep + "Mod"
        from PySide import QtGui
        if soft:
            for i in range(self.dialog.listWorkbenches.count()):
                txt = self.dialog.listWorkbenches.item(i).text().strip()
                ext = ""
                if txt.endswith(" ("+translate("AddonsInstaller","Installed")+")"):
                    txt = txt[:-12]
                    ext = " ("+translate("AddonsInstaller","Installed")+")"
                elif txt.endswith(" ("+translate("AddonsInstaller","Update available")+")"):
                    txt = txt[:-19]
                    ext = " ("+translate("AddonsInstaller","Update available")+")"
                elif txt.endswith(" ("+translate("AddonsInstaller","Restart required")+")"):
                    txt = txt[:-19]
                    ext = " ("+translate("AddonsInstaller","Restart required")+")"
                if os.path.exists(os.path.join(moddir,txt)):
                    self.dialog.listWorkbenches.item(i).setText(txt+ext)
                else:
                    self.dialog.listWorkbenches.item(i).setText(txt)
                    self.dialog.listWorkbenches.item(i).setIcon(QtGui.QIcon(":/icons/Group.svg"))
            for i in range(self.dialog.listMacros.count()):
                txt = self.dialog.listMacros.item(i).text().strip()
                if txt.endswith(" ("+translate("AddonsInstaller","Installed")+")"):
                    txt = txt[:-12]
                elif txt.endswith(" ("+translate("AddonsInstaller","Update available")+")"):
                    txt = txt[:-19]
                if os.path.exists(os.path.join(moddir,txt)):
                    self.dialog.listMacros.item(i).setText(txt+ext)
                else:
                    self.dialog.listMacros.item(i).setText(txt)
                    self.dialog.listMacros.item(i).setIcon(QtGui.QIcon(":/icons/Group.svg"))
        else:
            self.dialog.listWorkbenches.clear()
            self.dialog.listMacros.clear()
            for wb in self.repos:
                if os.path.exists(os.path.join(moddir,wb[0])):
                    self.dialog.listWorkbenches.addItem(QtGui.QListWidgetItem(QtGui.QIcon(":/icons/button_valid.svg"),str(wb[0]) + " ("+translate("AddonsInstaller","Installed")+")"))
                    wb[2] = 1
                else:
                    self.dialog.listWorkbenches.addItem(QtGui.QListWidgetItem(QtGui.QIcon(":/icons/applications-python.svg"),str(wb[0])))
                    wb[2] = 0
            for macro in self.macros:
                if macro.is_installed():
                    self.dialog.listMacros.addItem(QtGui.QListWidgetItem(QtGui.QIcon(":/icons/button_valid.svg"), macro.name + " ("+translate("AddonsInstaller","Installed")+")"))
                else:
                    self.dialog.listMacros.addItem(QtGui.QListWidgetItem(QtGui.QIcon(":/icons/applications-python.svg"),+macro.name))

    def mark(self,repo):

        "mark a workbench as updatable"

        from PySide import QtGui
        for i in range(self.dialog.listWorkbenches.count()):
            w = self.dialog.listWorkbenches.item(i)
            if w.text().startswith(str(repo)):
                w.setText(str(repo) + str(" ("+translate("AddonsInstaller","Update available")+")"))
                w.setIcon(QtGui.QIcon(":/icons/debug-marker.svg"))
                if not repo in self.doUpdate:
                    self.doUpdate.append(repo)

    def show_config(self):

        "shows the configuration dialog"

        import FreeCADGui
        from PySide import QtGui
        self.config = FreeCADGui.PySideUic.loadUi(os.path.join(os.path.dirname(__file__),"AddonManagerOptions.ui"))

        # restore stored values
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        self.config.checkUpdates.setChecked(pref.GetBool("AutoCheck",False))
        self.config.customRepositories.setPlainText(pref.GetString("CustomRepositories",""))

        # center the dialog over the Addon Manager
        self.config.move(self.dialog.frameGeometry().topLeft() + self.dialog.rect().center() - self.config.rect().center())

        ret = self.config.exec_()
        
        if ret:
            # OK button has been pressed
            pref.SetBool("AutoCheck",self.config.checkUpdates.isChecked())
            pref.SetString("CustomRepositories",self.config.customRepositories.toPlainText())
