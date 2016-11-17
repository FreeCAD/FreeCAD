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
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond
    
## @package ArchServer
#  \ingroup ARCH
#  \brief The Server object and tools
#
#  This module provides utility functions to connect with
#  online or local servers like BimServer or GIT

__title__="FreeCAD Arch Server commands"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"



# BIMSERVER ###########################################################



class _CommandBimserver:
    
    "the Arch Bimserver command definition"
    
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Bimserver',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Bimserver","BIM server"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Bimserver","Connects and interacts with a BIM server instance")}

    def Activated(self):
        try:
            import requests
        except:
            FreeCAD.Console.PrintError(translate("Arch","requests python module not found, aborting. Please install python-requests\n"))
            return
        try:
            import json
        except:
            FreeCAD.Console.PrintError(translate("Arch","json python module not found, aborting. Please install python-json\n"))
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
                import requests, json
                self.form.labelStatus.setText("Logging in...")
                url2 = url + "/json"
                data = {'request': {'interface': 'AuthInterface', 'method': 'login', 'parameters': {'username': login, 'password': passwd}}}
                try:
                    resp = requests.post(url2,data = json.dumps(data))
                except:
                    FreeCAD.Console.PrintError(translate("Arch","Unable to connect to BimServer at")+" "+url+"\n")
                    self.form.labelStatus.setText(translate("Arch","Connection failed."))
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
        import requests, json
        url,token = self.getPrefs()
        if url and token:
            self.form.labelStatus.setText(translate("Arch","Getting projects list..."))
            url += "/json"
            data = { "token": token, "request": { "interface": "SettingsInterface", "method": "getServerSettings", "parameters": { } } }
            try:
                resp = requests.post(url,data = json.dumps(data))
            except:
                FreeCAD.Console.PrintError(translate("Arch","Unable to connect to BimServer at")+" "+url[:-5]+"\n")
                self.form.labelStatus.setText(translate("Arch","Connection failed."))
                return
            if resp.ok:
                try:
                    name = resp.json()["response"]["result"]["name"]
                except:
                    pass # unable to get the server name
                else:
                    self.form.labelServerName.setText(name)
            data = { "token": token, "request": { "interface": "ServiceInterface", "method": "getAllProjects", "parameters": { "onlyTopLevel": "false", "onlyActive": "true" } } }
            resp = requests.post(url,data = json.dumps(data))
            if resp.ok:
                try:
                    projects = resp.json()["response"]["result"]
                except:
                    FreeCAD.Console.PrintError(translate("Arch","Unable to get projects list from BimServer\n"))
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
        import requests, json
        url,token = self.getPrefs()
        if url and token:
            url += "/json"
            if (index >= 0) and (len(self.Projects) > index):
                p = self.Projects[index]
                self.form.labelStatus.setText(translate("Arch","Getting revisions..."))
                for rev in p["revisions"]:
                    data = { "token": token, "request": { "interface": "ServiceInterface", "method": "getRevision", "parameters": { "roid": rev } } }
                    resp = requests.post(url,data = json.dumps(data))
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
            import requests, json
            url,token = self.getPrefs()
            if url and token:
                FreeCAD.Console.PrintMessage(translate("Arch","Downloading file from Bimserver...\n"))
                self.form.labelStatus.setText(translate("Arch","Checking available serializers..."))
                url += "/json"
                serializer = None
                for s in ["Ifc2x3tc1"]: # Ifc4 seems unreliable ATM, let's stick with good old Ifc2x3...
                    data = { "token": token, "request": { "interface": "ServiceInterface", "method": "getSerializerByName", "parameters": { "serializerName": s } } }
                    resp = requests.post(url,data = json.dumps(data))
                    if resp.ok:
                        try:
                            srl = resp.json()["response"]["result"]
                        except:
                            pass # unable to get this serializer
                        else:
                            serializer = srl
                            break
                if not serializer:
                    FreeCAD.Console.PrintError(translate("Arch","Unable to get a valid serializer from the BimServer\n"))
                    return
                tf = QtGui.QFileDialog.getSaveFileName(QtGui.qApp.activeWindow(), "Save the downloaded IFC file?", None, "IFC files (*.ifc)")
                if tf:
                    tf = tf[0]
                self.form.labelStatus.setText(translate("Arch","Downloading file..."))
                data = { "token": token, "request": { "interface": "ServiceInterface", "method": "downloadRevisions", "parameters": { "roids": [rev["oid"]], "serializerOid": serializer["oid"], "sync": "false" } } }
                resp = requests.post(url,data = json.dumps(data))
                if resp.ok:
                    try:
                        downloadid = resp.json()["response"]["result"]
                    except:
                        FreeCAD.Console.PrintError(translate("Arch","Unable to obtain a valid download for this revision from the BimServer\n"))
                        return
                data = { "token": token, "request": { "interface": "ServiceInterface", "method": "getDownloadData", "parameters": { "topicId": downloadid } } }
                resp = requests.post(url,data = json.dumps(data))
                if resp.ok:
                    try:
                        downloaddata = resp.json()["response"]["result"]["file"]
                    except:
                        FreeCAD.Console.PrintError(translate("Arch","Unable to download the data for this revision.\n"))
                        return
                    else:
                        FreeCAD.Console.PrintMessage(translate("Arch","Opening file...\n"))
                        self.form.labelStatus.setText(translate("Arch","Opening file..."))
                        if not tf:
                            th,tf = tempfile.mkstemp(suffix=".ifc")
                        f = open(tf,"wb")
                        f.write(base64.b64decode(downloaddata))
                        f.close()
                        os.close(th)
                        import importIFC
                        importIFC.open(tf)
                        os.remove(tf)
        self.form.labelStatus.setText("")

    def uploadFile(self):
        self.form.labelStatus.setText("")
        if (self.form.comboProjects.currentIndex() >= 0) and (len(self.Projects) > self.form.comboProjects.currentIndex()) and (self.form.comboRoot.currentIndex() >= 0):
            project = self.Projects[self.form.comboProjects.currentIndex()]
            import requests, json
            url,token = self.getPrefs()
            if url and token:
                url += "/json"
                deserializer = None
                FreeCAD.Console.PrintMessage(translate("Arch","Saving file...\n"))
                self.form.labelStatus.setText(translate("Arch","Checking available deserializers..."))
                import ifcopenshell
                schema = ifcopenshell.schema_identifier.lower()
                data = { "token": token, "request": { "interface": "PluginInterface",  "method": "getAllDeserializers", "parameters": { "onlyEnabled": "true" } } }
                resp = requests.post(url,data = json.dumps(data))
                if resp.ok:
                    try:
                        for d in resp.json()["response"]["result"]:
                            if schema in d["name"].lower():
                                deserializer = d
                                break
                    except:
                        pass
                if not deserializer:
                    FreeCAD.Console.PrintError(translate("Arch","Unable to get a valid deserializer for the schema")+" "+schema+"\n")
                    return
                tf = QtGui.QFileDialog.getSaveFileName(QtGui.qApp.activeWindow(), translate("Arch","Save the IFC file before uploading?"), None, translate("Arch","IFC files (*.ifc)"))
                if tf:
                    tf = tf[0]
                if not tf:
                    tf = os.path.join(tempfile._get_default_tempdir(),next(tempfile._get_candidate_names())+".ifc")
                import importIFC
                self.form.labelStatus.setText(translate("Arch","Saving file..."))
                importIFC.export([self.RootObjects[self.form.comboRoot.currentIndex()]],tf)
                f = open(tf,"rb")
                ifcdata = base64.b64encode(f.read())
                f.close()
                FreeCAD.Console.PrintMessage(translate("Arch","Uploading file to Bimserver...\n"))
                self.form.labelStatus.setText(translate("Arch","Uploading file..."))
                data = { "token": token, "request": { "interface": "ServiceInterface", "method": "checkin", "parameters": { "poid": project["oid"], "comment": self.form.editComment.text(), "deserializerOid": deserializer["oid"], "fileSize": os.path.getsize(tf), "fileName": os.path.basename(tf), "data": ifcdata, "merge": "false", "sync": "true" } } }
                resp = requests.post(url,data = json.dumps(data))
                if resp.ok:
                    if resp.json()["response"]["result"]:
                        FreeCAD.Console.PrintMessage(translate("Arch","File upload successful\n"))
                        self.getRevisions(self.form.comboProjects.currentIndex())
                    else:
                        FreeCAD.Console.PrintError(translate("Arch","File upload failed\n"))
        self.form.labelStatus.setText("")



# GIT ###########################################################



class _CommandGit:
    "the Arch Git Commit command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Git',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Git","Git"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Git","Manages the current document with Git")}

    def Activated(self):
        f = FreeCAD.ActiveDocument.FileName
        if not f:
            FreeCAD.Console.PrintError(translate("Arch","This document is not saved. Please save it first.\n"))
            return
        try:
            import git
        except:
            FreeCAD.Console.PrintError(translate("Arch","The Python Git module was not found. Please install the python-git package.\n"))
            return
        try:
            repo = git.Repo(os.path.dirname(f))
        except:
            FreeCAD.Console.PrintError(translate("Arch","This document doesn't appear to be part of a Git repository.\n"))
            return
        else:
            FreeCADGui.Control.showDialog(_GitTaskPanel(repo))


class _GitTaskPanel:
    
    '''The TaskPanel for the Git command'''
    
    def __init__(self,repo):
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/GitTaskPanel.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Git.svg"))
        self.form.labelStatus.setText("")
        QtCore.QObject.connect(self.form.buttonRefresh, QtCore.SIGNAL("clicked()"), self.getFiles)
        QtCore.QObject.connect(self.form.buttonLog, QtCore.SIGNAL("clicked()"), self.getLog)
        QtCore.QObject.connect(self.form.buttonSelectAll, QtCore.SIGNAL("clicked()"), self.form.listFiles.selectAll)
        QtCore.QObject.connect(self.form.buttonDiff, QtCore.SIGNAL("clicked()"), self.getDiff)
        QtCore.QObject.connect(self.form.buttonCommit, QtCore.SIGNAL("clicked()"), self.commit)
        QtCore.QObject.connect(self.form.buttonPush, QtCore.SIGNAL("clicked()"), self.push)
        QtCore.QObject.connect(self.form.buttonPull, QtCore.SIGNAL("clicked()"), self.pull)
        self.repo = repo
        self.getRemotes()
        self.getFiles()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def accept(self):
        FreeCADGui.Control.closeDialog()
        
    def getFiles(self):
        self.form.labelStatus.setText("")
        self.form.listFiles.clear()
        self.modified = self.repo.git.diff("--name-only").split()
        self.untracked = self.repo.git.ls_files("--other","--exclude-standard").split()
        for f in self.modified:
            self.form.listFiles.addItem(f)
        for f in self.untracked:
            self.form.listFiles.addItem(f+" *")
        self.form.labelStatus.setText(translate("Arch","Branch")+": "+self.repo.active_branch.name)

    def getLog(self):
        textform = FreeCADGui.PySideUic.loadUi(":/ui/DialogDisplayText.ui")
        textform.setWindowTitle("Git log")
        textform.browserText.setPlainText(self.repo.git.log())
        textform.exec_()
        
    def getDiff(self):
        if (self.form.listFiles.currentRow() >= 0):
            f = (self.modified+self.untracked)[self.form.listFiles.currentRow()]
            textform = FreeCADGui.PySideUic.loadUi(":/ui/DialogDisplayText.ui")
            textform.setWindowTitle("Diff: "+f)
            textform.browserText.setPlainText(self.repo.git.diff(f))
            textform.exec_()
            
    def getRemotes(self):
        self.form.listRepos.clear()
        if self.repo.remotes:
            for r in self.repo.remotes:
                self.form.listRepos.addItem(r.name+": "+r.url)
        else:
            FreeCAD.Console.PrintWarning(translate("Arch","Warning: no remote repositories.\n"))
            
    def commit(self):
        if not self.form.listFiles.selectedItems():
            FreeCAD.Console.PrintError(translate("Arch","Please select file(s) to commit.\n"))
            self.form.labelStatus.setText(translate("Arch","No file selected"))
            return
        if not self.form.editMessage.text():
            FreeCAD.Console.PrintError(translate("Arch","Please write a commit message.\n"))
            self.form.labelStatus.setText(translate("Arch","No commit message"))
            return
        for it in self.form.listFiles.selectedItems():
            f = it.text()
            if f[-2:] == " *":
                f = f[:-2]
            self.repo.git.add(f)
        s = self.repo.git.commit(m=self.form.editMessage.text())
        FreeCAD.Console.PrintMessage(translate("Arch","Successfully committed %i files.\n") % len(self.form.listFiles.selectedItems()))
        self.form.labelStatus.setText(translate("Arch","Files committed."))
        if s:
            FreeCAD.Console.PrintMessage(s+"\n")
        self.getFiles()
        
    def push(self):
        if len(self.form.listRepos.selectedItems()) != 1:
            FreeCAD.Console.PrintError(translate("Arch","Please select a repo to push to.\n"))
            self.form.labelStatus.setText(translate("Arch","No repo selected"))
            return
        self.form.labelStatus.setText(translate("Arch","Pushing files..."))
        r = self.form.listRepos.selectedItems()[0].text().split(":")[0]
        s = self.repo.git.push(r)
        FreeCAD.Console.PrintMessage(translate("Arch","Successfully pushed to")+" "+r+"\n")
        self.form.labelStatus.setText(translate("Arch","Files pushed."))
        if s:
            FreeCAD.Console.PrintMessage(s+"\n")
        self.getFiles()
        
    def pull(self):
        if len(self.form.listRepos.selectedItems()) != 1:
            FreeCAD.Console.PrintError(translate("Arch","Please select a repo to pull from.\n"))
            self.form.labelStatus.setText(translate("Arch","No repo selected"))
            return
        self.form.labelStatus.setText(translate("Arch","Pulling files..."))
        r = self.form.listRepos.selectedItems()[0].text().split(":")[0]
        s = self.repo.git.pull(r)
        FreeCAD.Console.PrintMessage(translate("Arch","Successfully pulled from")+" "+r+"\n")
        self.form.labelStatus.setText(translate("Arch","Files pulled."))
        if s:
            FreeCAD.Console.PrintMessage(s+"\n")
        if os.path.basename(FreeCAD.ActiveDocument.FileName) in s:
            FreeCAD.Console.PrintWarning(translate("Arch","Warning: the current document file has been changed by this pull. Please save your document to keep your changes.\n"))



if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Bimserver',_CommandBimserver())
    FreeCADGui.addCommand('Arch_Git',_CommandGit())
