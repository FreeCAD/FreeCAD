#***************************************************************************
#*   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
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

__title__  = "FreeCAD Arch External Reference"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecadweb.org"


import FreeCAD
import os
import zipfile
import re
import sys
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt, utf8_decode=False):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchReference
#  \ingroup ARCH
#  \brief The Reference object and tools
#
#  This module provides tools to build Reference objects.
#  References can take a shape from a Part-based object in
#  another file.



def makeReference(filepath=None,partname=None,name="External Reference"):


    "makeReference([filepath,partname]): Creates an Arch Reference object"

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","ArchReference")
    obj.Label = name
    ArchReference(obj)
    if FreeCAD.GuiUp:
        ViewProviderArchReference(obj.ViewObject)
    if filepath:
        obj.File = filepath
    if partname:
        obj.Part = partname
    import Draft
    Draft.select(obj)
    return obj



class ArchReference:


    "The Arch Reference object"

    def __init__(self,obj):

        obj.Proxy = self
        ArchReference.setProperties(self,obj)
        self.Type = "Reference"
        self.reload = True

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "File" in pl:
            obj.addProperty("App::PropertyFile","File","Reference",QT_TRANSLATE_NOOP("App::Property","The base file this component is built upon"))
        if not "Part" in pl:
            obj.addProperty("App::PropertyString","Part","Reference",QT_TRANSLATE_NOOP("App::Property","The part to use from the base file"))
        if not "ReferenceMode" in pl:
            obj.addProperty("App::PropertyEnumeration","ReferenceMode","Reference",QT_TRANSLATE_NOOP("App::Property","The way the referenced objects are included in the current document. 'Normal' includes the shape, 'Transient' discards the shape when the object is switched off (smaller filesize), 'Lightweight' does not import the shape but only the OpenInventor representation"))
            obj.ReferenceMode = ["Normal","Transient","Lightweight"]
            if "TransientReference" in pl:
                if obj.TransientReference:
                    obj.ReferenceMode = "Transient"
                obj.removeProperty("TransientReference")
                FreeCAD.Console.PrintMessage("Upgrading "+obj.Label+" TransientReference property to ReferenceMode\n")
        if not "FuseArch" in pl:
            obj.addProperty("App::PropertyBool","FuseArch", "Reference", QT_TRANSLATE_NOOP("App::Property","Fuse objects of same material"))
        self.Type = "Reference"

    def onDocumentRestored(self,obj):

        ArchReference.setProperties(self,obj)
        self.reload = False
        if obj.ReferenceMode == "Lightweight":
            if obj.ViewObject and obj.ViewObject.Proxy:
                obj.ViewObject.Proxy.loadInventor(obj)

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

    def onChanged(self,obj,prop):

        if prop in ["File","Part"]:
            self.reload = True
        elif prop == "ReferenceMode":
            if obj.ReferenceMode == "Normal":
                if obj.ViewObject and obj.ViewObject.Proxy:
                    obj.ViewObject.Proxy.unloadInventor(obj)
                if (not obj.Shape) or obj.Shape.isNull():
                    self.reload = True
                    obj.touch()
            elif obj.ReferenceMode == "Transient":
                if obj.ViewObject and obj.ViewObject.Proxy:
                    obj.ViewObject.Proxy.unloadInventor(obj)
                self.reload = False
            elif obj.ReferenceMode == "Lightweight":
                self.reload = False
                import Part
                pl = obj.Placement
                obj.Shape = Part.Shape()
                obj.Placement = pl
                if obj.ViewObject and obj.ViewObject.Proxy:
                    obj.ViewObject.Proxy.loadInventor(obj)

    def execute(self,obj):

        pl = obj.Placement
        filename = self.getFile(obj)
        if filename and obj.Part and self.reload and obj.ReferenceMode in ["Normal","Transient"]:
            self.parts = self.getPartsList(obj)
            if self.parts:
                zdoc = zipfile.ZipFile(filename)
                if zdoc:
                    if obj.Part in self.parts:
                        if self.parts[obj.Part][1] in zdoc.namelist():
                            f = zdoc.open(self.parts[obj.Part][1])
                            shapedata = f.read()
                            f.close()
                            if sys.version_info.major >= 3:
                                shapedata = shapedata.decode("utf8")
                            shape = self.cleanShape(shapedata,obj,self.parts[obj.Part][2])
                            obj.Shape = shape
                            if not pl.isIdentity():
                                obj.Placement = pl
                        else:
                            print("Part not found in file")
            self.reload = False

    def cleanShape(self,shapedata,obj,materials):

        "cleans the imported shape"

        import Part
        shape = Part.Shape()
        shape.importBrepFromString(shapedata)
        if obj.FuseArch and materials:
            # separate lone edges
            shapes = []
            for edge in shape.Edges:
                found = False
                for solid in shape.Solids:
                    for soledge in solid.Edges:
                        if edge.hashCode() == soledge.hashCode():
                            found = True
                            break
                    if found:
                        break
                if found:
                    break
            else:
                shapes.append(edge)
            print("solids:",len(shape.Solids),"mattable:",materials)
            for key,solindexes in materials.items():
                if key == "Undefined":
                    # do not join objects with no defined material
                    for solindex in [int(i) for i in solindexes.split(",")]:
                        shapes.append(shape.Solids[solindex])
                else:
                    fusion = None
                    for solindex in [int(i) for i in solindexes.split(",")]:
                        if not fusion:
                            fusion = shape.Solids[solindex]
                        else:
                            fusion = fusion.fuse(shape.Solids[solindex])
                    if fusion:
                        shapes.append(fusion)
            shape = Part.makeCompound(shapes)
            try:
                shape = shape.removeSplitter()
            except Exception:
                print(obj.Label,": error removing splitter")
        return shape

    def getFile(self,obj,filename=None):

        "gets a valid file, if possible"

        if not filename:
            filename = obj.File
        if not filename:
            return None
        if not filename.lower().endswith(".fcstd"):
            return None
        if not os.path.exists(filename):
            # search for the file in the current directory if not found
            basename = os.path.basename(filename)
            currentdir = os.path.dirname(obj.Document.FileName)
            altfile = os.path.join(currentdir,basename)
            if altfile == obj.Document.FileName:
                return None
            elif os.path.exists(altfile):
                return altfile
            else:
                # search for subpaths in current folder
                altfile = None
                subdirs = self.splitall(os.path.dirname(filename))
                for i in range(len(subdirs)):
                    subpath = [currentdir]+subdirs[-i:]+[basename]
                    altfile = os.path.join(*subpath)
                    if os.path.exists(altfile):
                        return altfile
                return None
        return filename

    def getPartsList(self,obj,filename=None):

        "returns a list of Part-based objects in a FCStd file"

        parts = {}
        materials = {}
        filename = self.getFile(obj,filename)
        if not filename:
            return parts
        zdoc = zipfile.ZipFile(filename)
        with zdoc.open("Document.xml") as docf:
            name = None
            label = None
            part = None
            materials = {}
            writemode = False
            for line in docf:
                if sys.version_info.major >= 3:
                    line = line.decode("utf8")
                if "<Object name=" in line:
                    n = re.findall('name=\"(.*?)\"',line)
                    if n:
                        name = n[0]
                elif "<Property name=\"Label\"" in line:
                    writemode = True
                elif writemode and "<String value=" in line:
                    n = re.findall('value=\"(.*?)\"',line)
                    if n:
                        label = n[0]
                        writemode = False
                elif "<Property name=\"Shape\" type=\"Part::PropertyPartShape\"" in line:
                    writemode = True
                elif writemode and "<Part file=" in line:
                    n = re.findall('file=\"(.*?)\"',line)
                    if n:
                        part = n[0]
                        writemode = False
                elif "<Property name=\"MaterialsTable\" type=\"App::PropertyMap\"" in line:
                    writemode = True
                elif writemode and "<Item key=" in line:
                    n = re.findall('key=\"(.*?)\"',line)
                    v = re.findall('value=\"(.*?)\"',line)
                    if n and v:
                        materials[n[0]] = v[0]
                elif writemode and "</Map>" in line:
                    writemode = False
                elif "</Object>" in line:
                    if name and label and part:
                        parts[name] = [label,part,materials]
                    name = None
                    label = None
                    part = None
                    materials = {}
                    writemode = False
        return parts

    def getColors(self,obj):

        "returns the DiffuseColor of the referenced object"

        filename = self.getFile(obj)
        if not filename:
            return None
        part = obj.Part
        if not obj.Part:
            return None
        zdoc = zipfile.ZipFile(filename)
        if not "GuiDocument.xml" in zdoc.namelist():
            return None
        colorfile = None
        with zdoc.open("GuiDocument.xml") as docf:
            writemode1 = False
            writemode2 = False
            for line in docf:
                if sys.version_info.major >= 3:
                    line = line.decode("utf8")
                if ("<ViewProvider name=" in line) and (part in line):
                    writemode1 = True
                elif writemode1 and ("<Property name=\"DiffuseColor\"" in line):
                    writemode1 = False
                    writemode2 = True
                elif writemode2 and ("<ColorList file=" in line):
                    n = re.findall('file=\"(.*?)\"',line)
                    if n:
                        colorfile = n[0]
                        break
        if not colorfile:
            return None
        if not colorfile in zdoc.namelist():
            return None
        colors = []
        cf = zdoc.open(colorfile)
        buf = cf.read()
        cf.close()
        for i in range(1,int(len(buf)/4)):
            if sys.version_info.major >= 3:
                colors.append((buf[i*4+3]/255.0,buf[i*4+2]/255.0,buf[i*4+1]/255.0,buf[i*4]/255.0))
            else:
                colors.append((ord(buf[i*4+3])/255.0,ord(buf[i*4+2])/255.0,ord(buf[i*4+1])/255.0,ord(buf[i*4])/255.0))
        if colors:
            return colors
        return None

    def splitall(self,path):

        "splits a path between its components"

        allparts = []
        while 1:
            parts = os.path.split(path)
            if parts[0] == path:  # sentinel for absolute paths
                allparts.insert(0, parts[0])
                break
            elif parts[1] == path: # sentinel for relative paths
                allparts.insert(0, parts[1])
                break
            else:
                path = parts[0]
                allparts.insert(0, parts[1])
        return allparts


class ViewProviderArchReference:


    "A View Provider for the Arch Reference object"

    def __init__(self,vobj):

        vobj.Proxy = self
        self.setProperties(vobj)

    def setProperties(self,vobj):

        pl = vobj.PropertiesList
        if not "TimeStamp" in pl:
            vobj.addProperty("App::PropertyFloat","TimeStamp","Reference",QT_TRANSLATE_NOOP("App::Property","The latest time stamp of the linked file"))
            vobj.setEditorMode("TimeStamp",2)
        if not "UpdateColors" in pl:
            vobj.addProperty("App::PropertyBool","UpdateColors","Reference",QT_TRANSLATE_NOOP("App::Property","If true, the colors from the linked file will be kept updated"))
            vobj.UpdateColors = True

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_Reference.svg"

    def setEdit(self,vobj,mode=0):

        taskd = ArchReferenceTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self,vobj,mode):

        FreeCADGui.Control.closeDialog()
        from DraftGui import todo
        todo.delay(vobj.Proxy.recolorize,vobj)
        return

    def attach(self,vobj):

        self.Object = vobj.Object
        # Check for file change every minute
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.checkChanges)
        s = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetInt("ReferenceCheckInterval",60)
        self.timer.start(1000*s)

    def doubleClicked(self,vobj):

        self.setEdit(vobj)

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

    def updateData(self,obj,prop):

        if (prop == "Shape") and hasattr(obj.ViewObject,"UpdateColors") and obj.ViewObject.UpdateColors:
            if obj.Shape and not obj.Shape.isNull():
                colors = obj.Proxy.getColors(obj)
                if colors:
                    obj.ViewObject.DiffuseColor = colors
                from DraftGui import todo
                todo.delay(self.recolorize,obj.ViewObject)

    def recolorize(self,vobj):

        if hasattr(vobj,"DiffuseColor") and hasattr(vobj,"UpdateColors") and vobj.UpdateColors:
            vobj.DiffuseColor = vobj.DiffuseColor

    def checkChanges(self):

        "checks if the linked file has changed"

        if hasattr(self,"Object") and self.Object:
            try:
                f = self.Object.File
            except ReferenceError:
                f = None
                if hasattr(self,"timer"):
                    self.timer.stop()
                    del self.timer
            if f:
                filename = self.Object.Proxy.getFile(self.Object)
                if filename:
                    st_mtime = os.stat(filename).st_mtime
                    if hasattr(self.Object.ViewObject,"TimeStamp"):
                        if self.Object.ViewObject.TimeStamp:
                            if self.Object.ViewObject.TimeStamp != st_mtime:
                                self.Object.Proxy.reload = True
                                self.Object.touch()
                        self.Object.ViewObject.TimeStamp = st_mtime

    def onChanged(self,vobj,prop):

        if prop == "ShapeColor":
            # prevent ShapeColor to override DiffuseColor
            if hasattr(vobj,"DiffuseColor") and hasattr(vobj,"UpdateColors"):
                if vobj.DiffuseColor and vobj.UpdateColors:
                    vobj.DiffuseColor = vobj.DiffuseColor
        elif prop == "Visibility":
            if vobj.Visibility == True:
                if (not vobj.Object.Shape) or vobj.Object.Shape.isNull():
                    vobj.Object.Proxy.reload = True
                    vobj.Object.Proxy.execute(vobj.Object)
            else:
                if hasattr(vobj.Object,"ReferenceMode") and vobj.Object.ReferenceMode == "Transient":
                    vobj.Object.Proxy.reload = False
                    import Part
                    pl = vobj.Object.Placement
                    vobj.Object.Shape = Part.Shape()
                    vobj.Object.Placement = pl

    def onDelete(self,obj,doc):

        if hasattr(self,"timer"):
            self.timer.stop()
            del self.timer
            return True

    def setupContextMenu(self,vobj,menu):

        action1 = QtGui.QAction(QtGui.QIcon(":/icons/view-refresh.svg"),"Reload reference",menu)
        QtCore.QObject.connect(action1,QtCore.SIGNAL("triggered()"),self.onReload)
        menu.addAction(action1)
        action2 = QtGui.QAction(QtGui.QIcon(":/icons/document-open.svg"),"Open reference",menu)
        QtCore.QObject.connect(action2,QtCore.SIGNAL("triggered()"),self.onOpen)
        menu.addAction(action2)

    def onReload(self):

        "reloads the reference object"

        if hasattr(self,"Object") and self.Object:
            self.Object.Proxy.reload = True
            self.Object.touch()
            FreeCAD.ActiveDocument.recompute()

    def onOpen(self):

        "opens the reference file"

        if hasattr(self,"Object") and self.Object:
            if self.Object.File:
                FreeCAD.openDocument(self.Object.File)

    def loadInventor(self,obj):

        "loads an openinventor file and replace the root node of this object"

        # check inventor contents
        ivstring = self.getInventorString(obj)
        if not ivstring:
            FreeCAD.Console.PrintWarning("Unable to get lightWeight node for object referenced in "+obj.Label+"\n")
            return
        from pivy import coin
        inputnode = coin.SoInput()
        inputnode.setBuffer(ivstring)
        lwnode = coin.SoDB.readAll(inputnode)
        if not isinstance(lwnode,coin.SoSeparator):
            FreeCAD.Console.PrintError("Invalid lightWeight node for object referenced in "+obj.Label+"\n")
            return
        if lwnode.getNumChildren() < 2:
            FreeCAD.Console.PrintError("Invalid lightWeight node for object referenced in "+obj.Label+"\n")
            return
        flatlines = lwnode
        shaded = lwnode.getChild(0)
        wireframe = lwnode.getChild(1)

        # check node contents
        rootnode = obj.ViewObject.RootNode
        if rootnode.getNumChildren() < 3:
            FreeCAD.Console.PrintError("Invalid root node in "+obj.Label+"\n")
            return
        switch = rootnode.getChild(2)
        if switch.getNumChildren() != 4:
            FreeCAD.Console.PrintError("Invalid root node in "+obj.Label+"\n")
            return

        # keep a copy of the original nodes
        self.orig_flatlines = switch.getChild(0).copy()
        self.orig_shaded = switch.getChild(1).copy()
        self.orig_wireframe = switch.getChild(2).copy()

        # replace root node of object
        switch.replaceChild(0,flatlines)
        switch.replaceChild(1,shaded)
        switch.replaceChild(2,wireframe)

    def unloadInventor(self,obj):

        "restore original nodes"

        if (not hasattr(self,"orig_flatlines")) or (not self.orig_flatlines):
            return
        if (not hasattr(self,"orig_shaded")) or (not self.orig_shaded):
            return
        if (not hasattr(self,"orig_wireframe")) or (not self.orig_wireframe):
            return

        # check node contents
        rootnode = obj.ViewObject.RootNode
        if rootnode.getNumChildren() < 3:
            FreeCAD.Console.PrintError("Invalid root node in "+obj.Label+"\n")
            return
        switch = rootnode.getChild(2)
        if switch.getNumChildren() != 4:
            FreeCAD.Console.PrintError("Invalid root node in "+obj.Label+"\n")
            return

        # replace root node of object
        switch.replaceChild(0,self.orig_flatlines)
        switch.replaceChild(1,self.orig_shaded)
        switch.replaceChild(2,self.orig_wireframe)

        # discard old content
        self.orig_flatlines = None
        self.orig_shaded = None
        self.orig_wireframe = None

    def getInventorString(self,obj):

        "locates and loads an iv file saved together with an object, if existing"

        filename = obj.Proxy.getFile(obj)
        if not filename:
            return None
        part = obj.Part
        if not obj.Part:
            return None
        zdoc = zipfile.ZipFile(filename)
        if not "Document.xml" in zdoc.namelist():
            return None
        ivfile = None
        with zdoc.open("Document.xml") as docf:
            writemode1 = False
            writemode2 = False
            for line in docf:
                if sys.version_info.major >= 3:
                    line = line.decode("utf8")
                if ("<Object name=" in line) and (part in line):
                    writemode1 = True
                elif writemode1 and ("<Property name=\"SavedInventor\"" in line):
                    writemode1 = False
                    writemode2 = True
                elif writemode2 and ("<FileIncluded file=" in line):
                    n = re.findall('file=\"(.*?)\"',line)
                    if n:
                        ivfile = n[0]
                        break
        if not ivfile:
            return None
        if not ivfile in zdoc.namelist():
            return None
        f = zdoc.open(ivfile)
        buf = f.read()
        if sys.version_info.major >= 3:
            buf = buf.decode("utf8")
        f.close()
        buf = buf.replace("lineWidth 2","lineWidth "+str(int(obj.ViewObject.LineWidth)))
        return buf


class ArchReferenceTaskPanel:


    '''The editmode TaskPanel for Reference objects'''

    def __init__(self,obj):

        self.obj = obj
        self.filename = None
        self.form = QtGui.QWidget()
        self.form.setWindowTitle("External reference")
        layout = QtGui.QVBoxLayout(self.form)
        label1 = QtGui.QLabel("External file:")
        layout.addWidget(label1)
        self.fileButton = QtGui.QPushButton(self.form)
        self.openButton = QtGui.QPushButton(self.form)
        self.openButton.setText("Open")
        if not self.obj.File:
            self.openButton.setEnabled(False)
        l2 = QtGui.QHBoxLayout(self.form)
        layout.addLayout(l2)
        l2.addWidget(self.fileButton)
        l2.addWidget(self.openButton)
        label2 = QtGui.QLabel("Part to use:")
        layout.addWidget(label2)
        if self.obj.File:
            self.fileButton.setText(os.path.basename(self.obj.File))
        else:
            self.fileButton.setText("Choose file...")
        self.partCombo = QtGui.QComboBox(self.form)
        layout.addWidget(self.partCombo)
        if hasattr(self.obj.Proxy,"parts"):
            parts = self.obj.Proxy.parts
        else:
            parts = self.obj.Proxy.getPartsList(self.obj)
        for k in sorted(parts.keys()):
            self.partCombo.addItem(parts[k][0],k)
        if self.obj.Part:
            if self.obj.Part in parts.keys():
                self.partCombo.setCurrentIndex(sorted(parts.keys()).index(self.obj.Part))
        QtCore.QObject.connect(self.fileButton, QtCore.SIGNAL("clicked()"), self.chooseFile)
        QtCore.QObject.connect(self.openButton, QtCore.SIGNAL("clicked()"), self.openFile)

    def accept(self):

        if self.filename:
            if self.filename != self.obj.File:
                self.obj.File = self.filename
                FreeCAD.ActiveDocument.recompute()
        if self.partCombo.currentText():
            i = self.partCombo.currentIndex()
            if self.partCombo.itemData(i) != self.obj.Part:
                self.obj.Part = self.partCombo.itemData(i)
                if self.obj.Label == "External Reference":
                    self.obj.Label = self.partCombo.itemText(i)
                FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def reject(self):

        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def chooseFile(self):

        loc = QtCore.QDir.homePath()
        if self.obj.File:
            loc = os.path.dirname(self.obj.File)
        f = QtGui.QFileDialog.getOpenFileName(self.form,'Choose reference file',loc,"FreeCAD standard files (*.FCStd)")
        if f:
            self.filename = f[0]
            self.fileButton.setText(os.path.basename(self.filename))
            parts = self.obj.Proxy.getPartsList(self.obj,self.filename)
            if parts:
                self.partCombo.clear()
                for k in sorted(parts.keys()):
                    self.partCombo.addItem(parts[k][0],k)
                if self.obj.Part:
                    if self.obj.Part in parts.keys():
                        self.partCombo.setCurrentIndex(sorted(parts.keys()).index(self.obj.Part))

    def openFile(self):

        if self.obj.File:
            FreeCAD.openDocument(self.obj.File)
            FreeCADGui.Control.closeDialog()
            FreeCADGui.ActiveDocument.resetEdit()


class ArchReferenceCommand:


    "the Arch Reference command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Reference',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Reference","External reference"),
                'Accel': "E, X",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Reference","Creates an external reference object")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create external reference"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.addModule("Draft")
        FreeCADGui.doCommand("obj = Arch.makeReference()")
        FreeCADGui.doCommand("Draft.autogroup(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.doCommand("obj.ViewObject.Document.setEdit(obj.ViewObject, 0)")



if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Reference', ArchReferenceCommand())
