#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2017 - Yorik van Havre <yorik@uncreated.net>            *
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

__title__ = "Sketchfab uploader"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

import FreeCAD, FreeCADGui, WebGui, os, zipfile, requests, tempfile
from PySide import QtCore, QtGui

# \cond
try:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, QtGui.QApplication.UnicodeUTF8)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)
# \endcond

SKETCHFAB_UPLOAD_URL = "https://api.sketchfab.com/v1/models"
SKETCHFAB_TOKEN_URL = "https://sketchfab.com/settings/password"
SKETCHFAB_MODEL_URL = "https://sketchfab.com/show/"


class SketchfabTaskPanel:
    
    '''The TaskPanel for Sketchfab upload'''
    
    def __init__(self):
        
        self.url = None
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/TaskDlgSketchfab.ui")
        self.form.ProgressBar.hide()
        self.form.Button_View.hide()
        QtCore.QObject.connect(self.form.Button_Token,QtCore.SIGNAL("pressed()"),self.getToken)
        QtCore.QObject.connect(self.form.Button_Upload,QtCore.SIGNAL("pressed()"),self.upload)
        QtCore.QObject.connect(self.form.Button_View,QtCore.SIGNAL("pressed()"),self.viewModel)
        self.form.Text_Name.setText(FreeCAD.ActiveDocument.Label)
        self.form.Text_Token.setText(FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Web").GetString("SketchfabToken",""))

    def isAllowedAlterSelection(self):
        
        return True

    def isAllowedAlterView(self):
        
        return True

    def getStandardButtons(self):
        
        return int(QtGui.QDialogButtonBox.Close)

    def accept(self):
        
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def getToken(self):
        
        QtGui.QDesktopServices.openUrl(SKETCHFAB_TOKEN_URL)
        
    def saveFile(self):
        
        import FreeCADGui
        if self.form.Radio_Selection.isChecked():
            objects = FreeCADGui.Selection.getSelection()
        else:
            objects = [obj for obj in FreeCAD.ActiveDocument.Objects if obj.ViewObject.isVisible()]
        if not objects:
            QtGui.QMessageBox.critical(None,translate("Web","Nothing to upload"),translate("The selection of the document contains no object to upload"))
            return None
        filename = os.path.join(tempfile._get_default_tempdir(),next(tempfile._get_candidate_names()))
        filetype = self.form.Combo_Filetype.currentIndex()
        # 0 = obj + mtl, 1 = obj, 2 = dae, 3 = stl, 4 = IGES, 5 = iv (currently not working)
        if filetype == 0: # OBJ + MTL
            import importOBJ
            importOBJ.export(objects,filename+".obj")
            return self.packFiles(filename,[filename+".obj",filename+".mtl"])
        elif filetype == 1: # OBJ (mesh exporter)
            import Mesh
            Mesh.export(objects,filename+".obj")
            return self.packFiles(filename,[filename+".obj"])
        elif filetype == 2: # DAE
            import importDAE
            importDAE.export(objects,filename+".dae")
            return self.packFiles(filename,[filename+".dae"])
        elif filetype == 3: # STL
            import Mesh
            Mesh.export(objects,filename+".stl")
            return self.packFiles(filename,[filename+".stl"])
        elif filetype == 4: # IGES
            import Part
            Part.export(objects,filename+".iges")
            return self.packFiles(filename,[filename+".iges"])
        elif filetype == 5: # STL
            import FreeCADGui
            FreeCADGui.export(objects,filename+".iv")
            return self.packFiles(filename,[filename+".iv"])

    def packFiles(self,filename,fileslist):
        
        originalname = os.path.basename(fileslist[0])
        for f in fileslist:
            if not os.path.exists(f):
                return None
        z = zipfile.ZipFile(filename+".zip","w")
        for f in fileslist:
            z.write(f)
        z.close()
        for f in fileslist:
            os.remove(f)
        s = os.path.getsize(filename+".zip")
        if s > 1048576:
            size = str(s >> 20)+" MB"
        else:
            size = str(s >> 10)+" KB"
        return (filename+".zip",originalname,size)

    def upload(self):

        if not self.form.Text_Name.text():
            QtGui.QMessageBox.critical(None,translate("Web","Model name is empty"),translate("You must provide a name for your model"))
            return
        if not self.form.Text_Token.text():
            QtGui.QMessageBox.critical(None,translate("Web","No token provided"),translate("The token is empty. Please press the Obtain button to get your user API token from Sketchfab, then copy / paste the API token to the field below"))
            return
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Web").SetString("SketchfabToken",self.form.Text_Token.text())
        pack = self.saveFile()
        if not pack:
            QtGui.QMessageBox.critical(None,translate("Web","File packing error"),translate("Unable to save and zip a file for upload"))
            return
        data = {
            "title": self.form.Text_Name.text(),
            "description": self.form.Text_Description.text(),
            "filename": pack[1],
            "tags": "freecad,"+self.form.Text_Tags.text(),
            "private": self.form.Check_Private.isChecked(),
            "token": self.form.Text_Token.text(),
            "source": "freecad",
            }
        files = {
            "fileModel": open(pack[0], 'rb'),
            }
        self.form.Button_Upload.hide()
        # for now this is a fake progress bar, it won't move, just to show the user that the upload is in progress
        self.form.ProgressBar.setFormat(translate("Web","Uploading")+" "+pack[2]+"...")
        self.form.ProgressBar.show()
        try:
            r = requests.post(SKETCHFAB_UPLOAD_URL, data=data, files=files, verify=False)
        except requests.exceptions.RequestException as e:
            QtGui.QMessageBox.critical(None,translate("Web","Upload error"),translate("Upload failed:")+" "+str(e))
            self.form.ProgressBar.hide()
            self.form.Button_Upload.show()
            return
        result = r.json()
        if r.status_code != requests.codes.ok:
            QtGui.QMessageBox.critical(None,translate("Web","Upload error"),translate("Upload failed:")+" "+result["error"])
            self.form.ProgressBar.hide()
            self.form.Button_Upload.show()
            return
        self.url = SKETCHFAB_MODEL_URL + result["result"]["id"]
        self.form.ProgressBar.hide()
        self.form.Button_View.show()

    def viewModel(self):
        
        if self.url:
            QtGui.QDesktopServices.openUrl(self.url)
        




