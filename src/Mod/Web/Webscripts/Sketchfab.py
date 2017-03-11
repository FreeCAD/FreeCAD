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

import FreeCAD, FreeCADGui, WebGui, os, zipfile, requests, tempfile, json, time
from PySide import QtCore, QtGui

# \cond
try:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, QtGui.QApplication.UnicodeUTF8)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)
# \endcond

SKETCHFAB_UPLOAD_URL = "https://api.sketchfab.com/v3/models"
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
        
    def get_request_payload(self, token, data={}, files={}, json_payload=False):
        
        """Helper method that returns the authentication token and proper content
        type depending on whether or not we use JSON payload."""
        headers = {'Authorization': 'Token {}'.format(token)}
        if json_payload:
            headers.update({'Content-Type': 'application/json'})
            data = json.dumps(data)
        return {'data': data, 'files': files, 'headers': headers}
        
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
        return (filename+".zip",size)

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
            "name": self.form.Text_Name.text(),
            "description": self.form.Text_Description.text(),
            "tags": ["freecad"]+[t.strip() for t in self.form.Text_Tags.text().split(",")],
            "private": self.form.Check_Private.isChecked(),
            }
        files = {
            "modelFile": open(pack[0], 'rb')
            }
        self.form.Button_Upload.hide()
        # for now this is a fake progress bar, it won't move, just to show the user that the upload is in progress
        self.form.ProgressBar.setFormat(translate("Web","Uploading")+" "+pack[1]+"...")
        self.form.ProgressBar.show()
        try:
            r = requests.post(SKETCHFAB_UPLOAD_URL, **self.get_request_payload(self.form.Text_Token.text(), data, files=files))
        except requests.exceptions.RequestException as e:
            QtGui.QMessageBox.critical(None,translate("Web","Upload error"),translate("Upload failed:")+" "+str(e))
            self.form.ProgressBar.hide()
            self.form.Button_Upload.show()
            return
        if r.status_code != requests.codes.created:
            QtGui.QMessageBox.critical(None,translate("Web","Upload error"),translate("Upload failed:")+" "+r.json())
            self.form.ProgressBar.hide()
            self.form.Button_Upload.show()
            return
        self.url = r.headers['Location']
        if self.form.Combo_Filetype.currentIndex() in [0,1]: # OBJ format, sketchfab expects inverted Y/Z axes
            self.form.ProgressBar.setFormat(translate("Web","Awaiting confirmation..."))
            self.form.ProgressBar.setValue(75)
            if self.poll(self.url):
                self.form.ProgressBar.setFormat(translate("Web","Fixing model..."))
                self.patch(self.url)
            else:
                QtGui.QMessageBox.warning(None,translate("Web","Patch error"),translate("Web","Patching failed. The model was successfully uploaded, but might still require manual adjustments:"))
        self.form.ProgressBar.hide()
        self.form.Button_View.show()
        
    def poll(self,url):
        
        """GET the model endpoint to check the processing status."""
        max_errors = 10
        errors = 0
        retry = 0
        max_retries = 50
        retry_timeout = 5  # seconds
        while (retry < max_retries) and (errors < max_errors):
            try:
                r = requests.get(url, **self.get_request_payload(self.form.Text_Token.text()))
            except requests.exceptions.RequestException as e:
                print ('Sketchfab: Polling failed with error {}'.format(e))
                errors += 1
                retry += 1
                continue
            result = r.json()
            if r.status_code != requests.codes.ok:
                print ('Sketchfab: Polling failed with error: {}'.format(result['error']))
                errors += 1
                retry += 1
                continue
            processing_status = result['status']['processing']
            if processing_status == 'PENDING':
                retry += 1
                time.sleep(retry_timeout)
                continue
            elif processing_status == 'PROCESSING':
                retry += 1
                time.sleep(retry_timeout)
                continue
            elif processing_status == 'FAILED':
                print ('Sketchfab: Polling failed: {}'.format(result['error']))
                return False
            elif processing_status == 'SUCCEEDED':
                return True
            retry += 1
        print ('Sketchfab: Stopped polling after too many retries or too many errors')
        return False
        
    def patch(self,url):
        
        "applies different fixes to the uploaded model"
        options_url = os.path.join(url, 'options')
        data = {
            'orientation': '{"axis": [1, 0, 0], "angle": 270}'
            }
        try:
            r = requests.patch(options_url, **self.get_request_payload(self.form.Text_Token.text(), data, json_payload=True))
        except requests.exceptions.RequestException as e:
            QtGui.QMessageBox.warning(None,translate("Web","Patch error"),translate("Web","Patching failed. The model was successfully uploaded, but might still require manual adjustments:")+" "+str(e))
        else:
            if r.status_code != 204:
                QtGui.QMessageBox.warning(None,translate("Web","Patch error"),translate("Web","Patching failed. The model was successfully uploaded, but might still require manual adjustments:")+" "+str(r.content))

    def viewModel(self):
        
        if self.url:
            url = self.url.replace("api","www")
            url = url.replace("/v3","")
            QtGui.QDesktopServices.openUrl(url)
        




