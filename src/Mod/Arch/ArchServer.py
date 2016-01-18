#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015                                                    *
#*   Yorik van Havre <yorik@uncreated.net>                                 *
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

import FreeCAD,os
from PySide import QtCore, QtGui

if FreeCAD.GuiUp:
    import FreeCADGui
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

__title__="FreeCAD Arch Server commands"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"


class _CommandBimserver:
    "the Arch Bimserver command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Bimserver',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Bimserver","BIM server"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Bimserver","Opens a browser window and connects to a BIM server instance")}

    def Activated(self):
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        url = p.GetString("BimServerUrl","http://localhost:8082")
        FreeCADGui.addModule("WebGui")
        FreeCADGui.doCommand("WebGui.openBrowser(\""+url+"\")")
  

class _CommandGit:
    "the Arch Git Commit command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Git',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Git","Commit with Git"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Git","Commits the current document")}

    def Activated(self):
        f = FreeCAD.ActiveDocument.FileName
        if not f:
            FreeCAD.Console.PrintError(translate("Arch","This document is not saved. Please save it first"))
            return
        try:
            import git
        except:
            FreeCAD.Console.PrintError(translate("Arch","The Python Git module was not found. Please install the python-git package."))
            return
        try:
            repo = git.Repo(os.path.dirname(f))
        except:
            FreeCAD.Console.PrintError(translate("Arch","This document doesn't appear to be part of a Git repository."))
            return
        pushOK = True
        if not repo.remotes:
            FreeCAD.Console.PrintWarning(translate("Arch","Warning: no remote repositories. Unable to push"))
            pushOK = False
        modified_files = repo.git.diff("--name-only").split()
        untracked_files = repo.git.ls_files("--other","--exclude-standard").split()
        if not os.path.basename(f) in modified_files:
            if not os.path.basename(f) in untracked_files:
                FreeCAD.Console.PrintError(translate("Arch","The Git repository cannot handle this document."))
                return
            
        d = _ArchGitDialog()
        if not pushOK:
            d.checkBox.setChecked(False)
            d.checkBox.setEnabled(False)
        d.label.setText(str(len(modified_files)+len(untracked_files))+" modified file(s)")
        d.lineEdit.setText("Changed " + os.path.basename(f))
        r = d.exec_()
        if r:
            if d.radioButton_2.isChecked():
                for o in modified_files + untracked_files:
                    repo.git.add(o)
            else:
                repo.git.add(os.path.basename(f))
            repo.git.commit(m=d.lineEdit.text())
            if d.checkBox.isChecked():
                repo.git.push()


class _ArchGitDialog(QtGui.QDialog):
    def __init__(self):
        QtGui.QDialog.__init__(self)
        self.setObjectName("ArchGitOptions")
        self.resize(370, 200)
        self.verticalLayout = QtGui.QVBoxLayout(self)
        self.verticalLayout.setObjectName("verticalLayout")
        self.groupBox = QtGui.QGroupBox(self)
        self.groupBox.setObjectName("groupBox")
        self.vl3 = QtGui.QVBoxLayout(self.groupBox)
        self.vl3.setObjectName("vl3")
        self.label = QtGui.QLabel(self.groupBox)
        self.label.setObjectName("label")
        self.vl3.addWidget(self.label)
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.vl3.addLayout(self.horizontalLayout)
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.radioButton_2 = QtGui.QRadioButton(self.groupBox)
        self.radioButton_2.setChecked(True)
        self.radioButton_2.setObjectName("radioButton_2")
        self.horizontalLayout.addWidget(self.radioButton_2)
        self.radioButton = QtGui.QRadioButton(self.groupBox)
        self.radioButton.setObjectName("radioButton")
        self.horizontalLayout.addWidget(self.radioButton)
        self.verticalLayout.addWidget(self.groupBox)
        self.groupBox_2 = QtGui.QGroupBox(self)
        self.groupBox_2.setObjectName("groupBox_2")
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.groupBox_2)
        self.verticalLayout_2.setObjectName("horizontalLayout_2")
        self.lineEdit = QtGui.QLineEdit(self.groupBox_2)
        self.lineEdit.setObjectName("lineEdit")
        self.verticalLayout_2.addWidget(self.lineEdit)
        self.checkBox = QtGui.QCheckBox(self.groupBox_2)
        self.checkBox.setChecked(True)
        self.checkBox.setObjectName("checkBox")
        self.verticalLayout_2.addWidget(self.checkBox)
        self.verticalLayout.addWidget(self.groupBox_2)
        self.buttonBox = QtGui.QDialogButtonBox(self)
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.verticalLayout.addWidget(self.buttonBox)

        self.retranslateUi()
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), self.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), self.reject)
        QtCore.QMetaObject.connectSlotsByName(self)

    def retranslateUi(self):
        self.setWindowTitle(QtGui.QApplication.translate("ArchGitOptions", "Git Options", None, QtGui.QApplication.UnicodeUTF8))
        self.groupBox.setTitle(QtGui.QApplication.translate("ArchGitOptions", "What to commit", None, QtGui.QApplication.UnicodeUTF8))
        self.radioButton_2.setText(QtGui.QApplication.translate("ArchGitOptions", "All files in folder", None, QtGui.QApplication.UnicodeUTF8))
        self.radioButton.setText(QtGui.QApplication.translate("ArchGitOptions", "Only this .FcStd file", None, QtGui.QApplication.UnicodeUTF8))
        self.groupBox_2.setTitle(QtGui.QApplication.translate("ArchGitOptions", "Commit message", None, QtGui.QApplication.UnicodeUTF8))
        self.lineEdit.setText(QtGui.QApplication.translate("ArchGitOptions", "commit", None, QtGui.QApplication.UnicodeUTF8))
        self.checkBox.setText(QtGui.QApplication.translate("ArchGitOptions", "Push to default remote repository", None, QtGui.QApplication.UnicodeUTF8))



if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Bimserver',_CommandBimserver())
    FreeCADGui.addCommand('Arch_Git',_CommandGit())
