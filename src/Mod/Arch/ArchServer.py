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

import FreeCAD, os, time, tempfile, base64, Draft
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



# BIMSERVER ###########################################################



class _CommandBimserver:
    
    "the Arch Bimserver command definition"
    
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Bimserver',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Bimserver","BIM server"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Bimserver","Connects and interacts with a BIM server instance")}

    def Activated(self):
        try:
            import requests
        except:
            FreeCAD.Console.PrintError("requests python module not found, aborting.\n")
        else:
            FreeCADGui.Control.showDialog(_BimServerTaskPanel())


class _BimServerTaskPanel:
    
    '''The TaskPanel for the BimServer command'''
    
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/BimServerTaskPanel.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Arch_Bimserver.svg"))
        self.form.labelStatus.setText("")
        QtCore.QObject.connect(self.form.buttonServer, QtCore.SIGNAL("clicked()"), self.login)
        QtCore.QObject.connect(self.form.buttonBrowser, QtCore.SIGNAL("clicked()"), self.browse)
        QtCore.QObject.connect(self.form.comboProjects, QtCore.SIGNAL("currentIndexChanged(int)"), self.getRevisions)
        QtCore.QObject.connect(self.form.buttonOpen, QtCore.SIGNAL("clicked()"), self.openFile)
        QtCore.QObject.connect(self.form.buttonUpload, QtCore.SIGNAL("clicked()"), self.uploadFile)
        self.prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        self.Projects = []
        self.Revisions = []
        self.RootObjects = Draft.getObjectsOfType(FreeCAD.ActiveDocument.Objects,"Site")+Draft.getObjectsOfType(FreeCAD.ActiveDocument.Objects,"Building")
        for o in self.RootObjects:
            self.form.comboRoot.addItem(o.Label)
        self.setLogged(False)
        url,token = self.getPrefs()
        if url and token:
            self.getProjects()
            
    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def accept(self):
        FreeCADGui.Selection.removeObserver(self.observer)
        FreeCADGui.Control.closeDialog()

    def getPrefs(self):
        url = self.prefs.GetString("BimServerUrl","http://localhost:8082")
        if hasattr(self,"token"):
            token = self.token
        else:
            token = self.prefs.GetString("BimServerToken","")
            if token:
                self.token = token
        return url,token
        
    def setLogged(self,logged):
        if logged:
            self.form.buttonServer.setText("Connected")
            self.form.buttonServer.setIcon(QtGui.QIcon(":/icons/edit_OK.svg"))
            self.form.buttonServer.setToolTip("Click to log out")
            self.Connected = True
        else:
            self.form.buttonServer.setText("Not connected")
            self.form.buttonServer.setIcon(QtGui.QIcon(":/icons/edit_Cancel.svg"))
            self.form.buttonServer.setToolTip("Click to log in")
            self.Connected = False

    def login(self):
        self.setLogged(False)
        self.form.labelStatus.setText("")
        if self.Connected:
            # if the user pressed logout, delete the token
            self.prefs.SetString("BimServerToken","")
        else:
            url,token = self.getPrefs()
            loginform = FreeCADGui.PySideUic.loadUi(":/ui/DialogBimServerLogin.ui")
            loginform.editUrl.setText(url)
            dlg = loginform.exec_()
            if dlg:
                url = loginform.editUrl.text()
                login = loginform.editLogin.text()
                passwd = loginform.editPassword.text()
                store = loginform.checkStore.isChecked()
                import requests
                self.form.labelStatus.setText("Logging in...")
                url2 = url + "/json"
                data = {'request': {'interface': 'AuthInterface', 'method': 'login', 'parameters': {'username': login, 'password': passwd}}}
                try:
                    resp = requests.post(url2,json = data)
                except:
                    FreeCAD.Console.PrintError("Unable to connect to BimServer at "+url+"\n")
                    self.form.labelStatus.setText("Connection failed.")
                    return
                if resp.ok:
                    try:
                        token = resp.json()["response"]["result"]
                    except:
                        return
                    else:
                        if store:
                            self.prefs.SetString("BimServerUrl",url)
                            if token:
                                self.prefs.SetString("BimServerToken",token)
                        else:
                            self.prefs.SetString("BimServerToken","")
                        if token:
                            self.token = token
                            self.getProjects()
        self.form.labelStatus.setText("")

    def browse(self):
        url = self.prefs.GetString("BimServerUrl","http://localhost:8082")+"/apps/bimviews"
        if self.prefs.GetBool("BimServerBrowser",False):
            FreeCADGui.addModule("WebGui")
            FreeCADGui.doCommand("WebGui.openBrowser(\""+url+"\")")
        else:
            QtGui.QDesktopServices.openUrl(QtCore.QUrl(url, QtCore.QUrl.TolerantMode))

    def getProjects(self):
        self.setLogged(False)
        self.Projects = []
        self.form.labelStatus.setText("")
        import requests
        url,token = self.getPrefs()
        if url and token:
            self.form.labelStatus.setText("Getting projects list...")
            url += "/json"
            data = { "token": token, "request": { "interface": "SettingsInterface", "method": "getServerSettings", "parameters": { } } }
            try:
                resp = requests.post(url,json = data)
            except:
                FreeCAD.Console.PrintError("Unable to connect to BimServer at "+url[:-5]+"\n")
                self.form.labelStatus.setText("Connection failed.")
                return
            if resp.ok:
                try:
                    name = resp.json()["response"]["result"]["name"]
                except:
                    pass # unable to get the server name
                else:
                    self.form.labelServerName.setText(name)
            data = { "token": token, "request": { "interface": "ServiceInterface", "method": "getAllProjects", "parameters": { "onlyTopLevel": "false", "onlyActive": "true" } } }
            resp = requests.post(url,json = data)
            if resp.ok:
                try:
                    projects = resp.json()["response"]["result"]
                except:
                    FreeCAD.Console.PrintError("Unable to get projects list from BimServer\n")
                else:
                    self.setLogged(True)
                    self.form.comboProjects.clear()
                    for p in projects:
                        self.form.comboProjects.addItem(p["name"])
                    self.Projects = projects
                    self.form.comboProjects.setCurrentIndex(0)
                    self.getRevisions(0)
        self.form.labelStatus.setText("")

    def getRevisions(self,index):
        self.form.labelStatus.setText("")
        self.form.listRevisions.clear()
        self.Revisions = []
        import requests
        url,token = self.getPrefs()
        if url and token:
            url += "/json"
            if (index >= 0) and (len(self.Projects) > index):
                p = self.Projects[index]
                self.form.labelStatus.setText("Getting revisions...")
                for rev in p["revisions"]:
                    data = { "token": token, "request": { "interface": "ServiceInterface", "method": "getRevision", "parameters": { "roid": rev } } }
                    resp = requests.post(url,json = data)
                    if resp.ok:
                        try:
                            name = resp.json()["response"]["result"]["comment"]
                            date = resp.json()["response"]["result"]["date"]
                        except:
                            pass # unable to get the revision
                        else:
                            date = time.strftime("%a %d %b %Y %H:%M:%S GMT", time.gmtime(int(date)/1000.0))
                            self.form.listRevisions.addItem(date+" - "+name)
                            self.Revisions.append(resp.json()["response"]["result"])
        self.form.labelStatus.setText("")

    def openFile(self):
        self.form.labelStatus.setText("")
        if (self.form.listRevisions.currentRow() >= 0) and (len(self.Revisions) > self.form.listRevisions.currentRow()):
            rev = self.Revisions[self.form.listRevisions.currentRow()]
            import requests
            url,token = self.getPrefs()
            if url and token:
                FreeCAD.Console.PrintMessage("Downloading file from Bimserver...\n")
                self.form.labelStatus.setText("Checking available serializers...")
                url += "/json"
                serializer = None
                for s in ["Ifc2x3tc1"]: # Ifc4 seems unreliable ATM, let's stick with good old Ifc2x3...
                    data = { "token": token, "request": { "interface": "ServiceInterface", "method": "getSerializerByName", "parameters": { "serializerName": s } } }
                    resp = requests.post(url,json = data)
                    if resp.ok:
                        try:
                            srl = resp.json()["response"]["result"]
                        except:
                            pass # unable to get this serializer
                        else:
                            serializer = srl
                            break
                if not serializer:
                    FreeCAD.Console.PrintError("Unable to get a valid serializer from the BimServer\n")
                    return
                tf = QtGui.QFileDialog.getSaveFileName(QtGui.qApp.activeWindow(), "Save the downloaded IFC file?", None, "IFC files (*.ifc)")
                if tf:
                    tf = tf[0]
                self.form.labelStatus.setText("Downloading file...")
                data = { "token": token, "request": { "interface": "ServiceInterface", "method": "downloadRevisions", "parameters": { "roids": [rev["oid"]], "serializerOid": serializer["oid"], "sync": "false" } } }
                resp = requests.post(url,json = data)
                if resp.ok:
                    try:
                        downloadid = resp.json()["response"]["result"]
                    except:
                        FreeCAD.Console.PrintError("Unable to obtain a valid download for this revision from the BimServer\n")
                        return
                data = { "token": token, "request": { "interface": "ServiceInterface", "method": "getDownloadData", "parameters": { "topicId": downloadid } } }
                resp = requests.post(url,json = data)
                if resp.ok:
                    try:
                        downloaddata = resp.json()["response"]["result"]["file"]
                    except:
                        FreeCAD.Console.PrintError("Unable to download the data for this revision.\n")
                        return
                    else:
                        FreeCAD.Console.PrintMessage("Opening file...\n")
                        self.form.labelStatus.setText("Opening file...")
                        if not tf:
                            tf = tempfile.mkstemp(suffix=".ifc")[1]
                        f = open(tf,"wb")
                        f.write(base64.b64decode(downloaddata))
                        f.close()
                        import importIFC
                        importIFC.open(tf)
        self.form.labelStatus.setText("")

    def uploadFile(self):
        self.form.labelStatus.setText("")
        if (self.form.comboProjects.currentIndex() >= 0) and (len(self.Projects) > self.form.comboProjects.currentIndex()) and (self.form.comboRoot.currentIndex() >= 0):
            project = self.Projects[self.form.comboProjects.currentIndex()]
            import requests
            url,token = self.getPrefs()
            if url and token:
                url += "/json"
                deserializer = None
                FreeCAD.Console.PrintMessage("Saving file...\n")
                self.form.labelStatus.setText("Checking available deserializers...")
                import ifcopenshell
                schema = ifcopenshell.schema_identifier.lower()
                data = { "token": token, "request": { "interface": "PluginInterface",  "method": "getAllDeserializers", "parameters": { "onlyEnabled": "true" } } }
                resp = requests.post(url,json = data)
                if resp.ok:
                    try:
                        for d in resp.json()["response"]["result"]:
                            if schema in d["name"].lower():
                                deserializer = d
                                break
                    except:
                        pass
                if not deserializer:
                    FreeCAD.Console.PrintError("Unable to get a valid deserializer for the "+schema+" schema\n")
                    return
                tf = QtGui.QFileDialog.getSaveFileName(QtGui.qApp.activeWindow(), "Save the IFC file before uploading?", None, "IFC files (*.ifc)")
                if tf:
                    tf = tf[0]
                if not tf:
                    tf = os.path.join(tempfile._get_default_tempdir(),next(tempfile._get_candidate_names())+".ifc")
                import importIFC
                self.form.labelStatus.setText("Saving file...")
                importIFC.export([self.RootObjects[self.form.comboRoot.currentIndex()]],tf)
                f = open(tf,"rb")
                ifcdata = base64.b64encode(f.read())
                f.close()
                FreeCAD.Console.PrintMessage("Uploading file to Bimserver...\n")
                self.form.labelStatus.setText("Uploading file...")
                data = { "token": token, "request": { "interface": "ServiceInterface", "method": "checkin", "parameters": { "poid": project["oid"], "comment": self.form.editComment.text(), "deserializerOid": deserializer["oid"], "fileSize": os.path.getsize(tf), "fileName": os.path.basename(tf), "data": ifcdata, "merge": "false", "sync": "true" } } }
                resp = requests.post(url,json = data)
                if resp.ok:
                    if resp.json()["response"]["result"]:
                        FreeCAD.Console.PrintMessage("File upload successful\n")
                        self.getRevisions(self.form.comboProjects.currentIndex())
                    else:
                        FreeCAD.Console.PrintError("File upload failed\n")
        self.form.labelStatus.setText("")



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
