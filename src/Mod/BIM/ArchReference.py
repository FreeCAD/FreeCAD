# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__  = "FreeCAD Arch External Reference"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

## @package ArchReference
#  \ingroup ARCH
#  \brief The Reference object and tools
#
#  This module provides tools to build Reference objects.
#  References can take a shape from a Part-based object in
#  another file.

import os
import re
import zipfile

import FreeCAD

from draftutils import params

if FreeCAD.GuiUp:
    from PySide import QtCore, QtGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    from draftutils.translate import translate
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond


class ArchReference:

    """The Arch Reference object"""

    def __init__(self, obj):

        obj.Proxy = self
        self.Type = "Reference"
        ArchReference.setProperties(self, obj)
        self.reload = True


    def setProperties(self, obj):

        pl = obj.PropertiesList
        if not "File" in pl:
            t = QT_TRANSLATE_NOOP("App::Property","The base file this component is built upon")
            obj.addProperty("App::PropertyFile","File","Reference",t, locked=True)
        if not "Part" in pl:
            t = QT_TRANSLATE_NOOP("App::Property","The part to use from the base file")
            obj.addProperty("App::PropertyString","Part","Reference",t, locked=True)
        if not "ReferenceMode" in pl:
            t = QT_TRANSLATE_NOOP("App::Property","The way the referenced objects are included in the current document. 'Normal' includes the shape, 'Transient' discards the shape when the object is switched off (smaller filesize), 'Lightweight' does not import the shape but only the OpenInventor representation")
            obj.addProperty("App::PropertyEnumeration","ReferenceMode","Reference",t, locked=True)
            obj.ReferenceMode = ["Normal","Transient","Lightweight"]
            if "TransientReference" in pl:
                if obj.TransientReference:
                    obj.ReferenceMode = "Transient"
                obj.removeProperty("TransientReference")
                t = translate("Arch", "TransientReference property to ReferenceMode")
                FreeCAD.Console.PrintMessage(translate("Arch","Upgrading")+" "+obj.Label+" "+t+"\n")
        if not "FuseArch" in pl:
            t = QT_TRANSLATE_NOOP("App::Property","Fuse objects of same material")
            obj.addProperty("App::PropertyBool","FuseArch", "Reference", t, locked=True)


    def onDocumentRestored(self, obj):

        ArchReference.setProperties(self, obj)
        self.reload = False
        if obj.ReferenceMode == "Lightweight":
            if obj.ViewObject and obj.ViewObject.Proxy:
                obj.ViewObject.Proxy.loadInventor(obj)


    def dumps(self):

        return None


    def loads(self, state):

        self.Type = "Reference"


    def onChanged(self, obj, prop):

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


    def execute(self, obj):

        import Part
        pl = obj.Placement
        filename = self.getFile(obj)
        if filename and self.reload and obj.ReferenceMode in ["Normal","Transient"]:
            self.parts = self.getPartsList(obj)
            if self.parts:
                if filename.lower().endswith(".fcstd"):
                    zdoc = zipfile.ZipFile(filename)
                    if zdoc:
                        if obj.Part:
                            if obj.Part in self.parts:
                                if self.parts[obj.Part][1] in zdoc.namelist():
                                    f = zdoc.open(self.parts[obj.Part][1])
                                    shapedata = f.read()
                                    f.close()
                                    shapedata = shapedata.decode("utf8")
                                    shape = self.cleanShape(shapedata,obj,self.parts[obj.Part][2])
                                    obj.Shape = shape
                                    if not pl.isIdentity():
                                        obj.Placement = pl
                                else:
                                    t = translate("Arch","Part not found in file")
                                    FreeCAD.Console.PrintError(t+"\n")
                        else:
                            shapes = []
                            for part in self.parts.values():
                                f = zdoc.open(part[1])
                                shapedata = f.read()
                                f.close()
                                shapedata = shapedata.decode("utf8")
                                shape = self.cleanShape(shapedata,obj)
                                shapes.append(shape)
                            if shapes:
                                obj.Shape = Part.makeCompound(shapes)
                elif filename.lower().endswith(".ifc"):
                    ifcfile = self.getIfcFile(filename)
                    if not ifcfile:
                        return
                    try:
                        from nativeifc import ifc_tools
                        from nativeifc import ifc_generator
                    except:
                        t = translate("Arch","NativeIFC not available - unable to process IFC files")
                        FreeCAD.Console.PrintError(t+"\n")
                        return
                    elements = self.getIFCElements(obj, ifcfile)
                    shape, colors = ifc_generator.generate_shape(ifcfile, elements, cached=True)
                    if shape:
                        placement = shape.Placement
                        obj.Shape = shape
                        obj.Placement = placement
                        if colors:
                            ifc_tools.set_colors(obj, colors)
            elif filename.lower().endswith(".dxf"):
                # create a special parameter set to control the DXF importer
                loc = "User parameter:BaseApp/Preferences/Mod/Arch"
                hGrp = FreeCAD.ParamGet(loc).GetGroup("RefDxfImport")
                hGrp.SetBool("dxfUseDraftVisGroups", False)
                hGrp.SetBool("dxfGetOriginalColors", False)
                hGrp.SetBool("groupLayers", True)
                hGrp.SetFloat("dxfScaling", 1.0)
                hGrp.SetBool("dxftext", False)
                hGrp.SetBool("dxfImportPoints", True)
                hGrp.SetBool("dxflayout", False)
                hGrp.SetBool("dxfstarblocks", False)
                doc = obj.Document
                oldobjs = list(doc.Objects)
                import Import
                Import.readDXF(filename, doc.Name, True, loc + "/RefDxfImport")
                newobjs = [o for o in doc.Objects if o not in oldobjs]
                shapes = [o.Shape for o in newobjs if o.isDerivedFrom("Part::Feature")]
                if len(shapes) == 1:
                    obj.Shape = shapes[0]
                elif len(shapes) > 1:
                    obj.Shape = Part.makeCompound(shapes)
                names = [o.Name for o in newobjs]
                for n in names:
                    doc.removeObject(n)
            self.reload = False


    def getIFCElements(self, obj, ifcfile):

        """returns IFC elements for this object"""

        try:
            from nativeifc import ifc_generator
        except:
            t = translate("Arch","NativeIFC not available - unable to process IFC files")
            FreeCAD.Console.PrintError(t+"\n")
            return
        if obj.Part:
            element = ifcfile[int(obj.Part)]
        else:
            element = ifcfile.by_type("IfcProject")[0]
        elements = ifc_generator.get_decomposed_elements(element)
        elements = ifc_generator.filter_types(elements)
        return elements


    def cleanShape(self, shapedata, obj, materials=None):

        """cleans the imported shape"""

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
            #print("solids:",len(shape.Solids),"mattable:",materials)
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
                t = translate("Arch","Error removing splitter")
                FreeCAD.Console.PrintError(obj.Label+": "+t+"\n")
        return shape


    def exists(self,filepath):

        """case-insensitive version of os.path.exists. Returns the actual file path or None"""

        if os.path.exists(filepath):
            return filepath
        # check for uppercase/lowercase extensions
        p, e = os.path.splitext(filepath)
        if os.path.exists(p + e.lower()):
            return p + e.lower()
        if os.path.exists(p + e.upper()):
            return p + e.upper()
        return None


    def getFile(self,obj,filename=None):

        """gets a valid file, if possible"""

        if not filename:
            filename = obj.File
        if not filename:
            return None
        if not filename.lower().endswith(".fcstd"):
            if not filename.lower().endswith(".ifc"):
                if not filename.lower().endswith(".dxf"):
                    return None
        if not self.exists(filename):
            # search for the file in the current directory if not found
            basename = os.path.basename(filename)
            currentdir = os.path.dirname(obj.Document.FileName)
            altfile = os.path.join(currentdir,basename)
            if altfile == obj.Document.FileName:
                return None
            elif self.exists(altfile):
                return self.exists(altfile)
            else:
                # search for subpaths in current folder
                altfile = None
                subdirs = self.splitall(os.path.dirname(filename))
                for i in range(len(subdirs)):
                    subpath = [currentdir]+subdirs[-i:]+[basename]
                    altfile = os.path.join(*subpath)
                    if self.exists(altfile):
                        return self.exists(altfile)
                return None
        return self.exists(filename)


    def getPartsList(self, obj, filename=None):

        """returns a list of Part-based objects in a file"""

        filename = self.getFile(obj, filename)
        if not filename:
            return None
        if filename.lower().endswith(".fcstd"):
            return self.getPartsListFCSTD(obj, filename)
        elif filename.lower().endswith(".ifc"):
            return self.getPartsListIFC(obj, filename)
        elif filename.lower().endswith(".dxf"):
            return self.getPartsListDXF(obj, filename)


    def getPartsListDXF(self, obj, filename):

        """returns a list of Part-based objects in a DXF file"""

        # support layers
        #with open(filename) as f:
        #    txt = f.read()
        return {}


    def getPartsListIFC(self, obj, filename):

        """returns a list of Part-based objects in a IFC file"""

        ifcfile = self.getIfcFile(filename)
        if not ifcfile:
            return None
        structs = ifcfile.by_type("IfcSpatialElement")
        res = {}
        for s in structs:
            n = s.Name
            if not n:
                n = ""
            name = "#" + str(s.id()) + " " + n + "(" + s.is_a() + ")"
            res[str(s.id())] = [name, s, None]
        return res


    def getPartsListFCSTD(self, obj, filename):

        """returns a list of Part-based objects in a FCStd file"""

        parts = {}
        materials = {}
        zdoc = zipfile.ZipFile(filename)
        with zdoc.open("Document.xml") as docf:
            name = None
            label = None
            part = None
            materials = {}
            writemode = False
            for line in docf:
                line = line.decode("utf8")
                if "<Object name=" in line:
                    n = re.findall(r'name=\"(.*?)\"',line)
                    if n:
                        name = n[0]
                elif "<Property name=\"Label\"" in line:
                    writemode = True
                elif writemode and "<String value=" in line:
                    n = re.findall(r'value=\"(.*?)\"',line)
                    if n:
                        label = n[0]
                        writemode = False
                elif "<Property name=\"Shape\" type=\"Part::PropertyPartShape\"" in line:
                    writemode = True
                elif writemode and "<Part" in line and "file=" in line:
                    n = re.findall(r'file=\"(.*?)\"',line)
                    if n:
                        part = n[0]
                        writemode = False
                elif "<Property name=\"MaterialsTable\" type=\"App::PropertyMap\"" in line:
                    writemode = True
                elif writemode and "<Item key=" in line:
                    n = re.findall(r'key=\"(.*?)\"',line)
                    v = re.findall(r'value=\"(.*?)\"',line)
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


    def getIfcFile(self, filename):

        """Gets an IfcOpenShell object"""

        try:
            import ifcopenshell
        except:
            t = translate("Arch","NativeIFC not available - unable to process IFC files")
            FreeCAD.Console.PrintError(t+"\n")
            return None
        if not getattr(self, "ifcfile", None):
            self.ifcfile = ifcopenshell.open(filename)
        return self.ifcfile


    def getColors(self, obj):

        """returns the DiffuseColor of the referenced object"""

        filename = self.getFile(obj)
        if not filename:
            return None
        part = obj.Part
        if not obj.Part:
            return None
        colors = None
        if filename.lower().endswith(".fcstd"):
            zdoc = zipfile.ZipFile(filename)
            if not "GuiDocument.xml" in zdoc.namelist():
                return None
            colorfile = None
            with zdoc.open("GuiDocument.xml") as docf:
                writemode1 = False
                writemode2 = False
                for line in docf:
                    line = line.decode("utf8")
                    if ("<ViewProvider name=" in line) and (part in line):
                        writemode1 = True
                    elif writemode1 and ("<Property name=\"DiffuseColor\"" in line):
                        writemode1 = False
                        writemode2 = True
                    elif writemode2 and ("<ColorList file=" in line):
                        n = re.findall(r'file=\"(.*?)\"',line)
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
                colors.append((buf[i*4+3]/255.0,buf[i*4+2]/255.0,buf[i*4+1]/255.0,buf[i*4]/255.0))
        return colors


    def splitall(self,path):

        """splits a path between its components"""

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


    """A View Provider for the Arch Reference object"""


    def __init__(self,vobj):

        vobj.Proxy = self
        self.setProperties(vobj)


    def setProperties(self,vobj):

        pl = vobj.PropertiesList
        if not "TimeStamp" in pl:
            t = QT_TRANSLATE_NOOP("App::Property","The latest time stamp of the linked file")
            vobj.addProperty("App::PropertyFloat","TimeStamp","Reference",t, locked=True)
            vobj.setEditorMode("TimeStamp",2)
        if not "UpdateColors" in pl:
            t = QT_TRANSLATE_NOOP("App::Property","If true, the colors from the linked file will be kept updated")
            vobj.addProperty("App::PropertyBool","UpdateColors","Reference",t, locked=True)
            vobj.UpdateColors = True


    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_Reference.svg"


    def attach(self,vobj):

        self.Object = vobj.Object
        # Check for file change every minute
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.checkChanges)
        s = params.get_param_arch("ReferenceCheckInterval")
        self.timer.start(1000*s)


    def dumps(self):

        return None


    def loads(self,state):

        return None


    def updateData(self,obj,prop):

        if (prop == "Shape"):
            if hasattr(obj.ViewObject,"UpdateColors") and obj.ViewObject.UpdateColors:
                if obj.Shape and not obj.Shape.isNull():
                    colors = obj.Proxy.getColors(obj)
                    if colors:
                        obj.ViewObject.DiffuseColor = colors
                    from draftutils import todo
                    todo.ToDo.delay(self.recolorize,obj.ViewObject)


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
            # prevent ShapeColor from overriding DiffuseColor
            if hasattr(vobj,"DiffuseColor") and hasattr(vobj,"UpdateColors"):
                if vobj.DiffuseColor and vobj.UpdateColors:
                    vobj.DiffuseColor = vobj.DiffuseColor
        elif prop == "Visibility":
            if vobj.Visibility:
                if (not vobj.Object.Shape) or vobj.Object.Shape.isNull():
                    vobj.Object.Proxy.reload = True
                    vobj.Object.Proxy.execute(vobj.Object)
            else:
                if hasattr(vobj.Object,"ReferenceMode"):
                    if vobj.Object.ReferenceMode == "Transient":
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


    def setEdit(self, vobj, mode):
        if mode != 0:
            return None

        taskd = ArchReferenceTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True


    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None

        FreeCADGui.Control.closeDialog()
        from draftutils import todo
        todo.ToDo.delay(vobj.Proxy.recolorize,vobj)
        return True


    def setupContextMenu(self, vobj, menu):

        if FreeCADGui.activeWorkbench().name() != 'BIMWorkbench':
            return

        actionEdit = QtGui.QAction(translate("Arch", "Edit"),
                                   menu)
        QtCore.QObject.connect(actionEdit,
                               QtCore.SIGNAL("triggered()"),
                               self.edit)
        menu.addAction(actionEdit)

        actionOnReload = QtGui.QAction(QtGui.QIcon(":/icons/view-refresh.svg"),
                                       translate("Arch", "Reload reference"),
                                       menu)
        QtCore.QObject.connect(actionOnReload,
                               QtCore.SIGNAL("triggered()"),
                               self.onReload)
        menu.addAction(actionOnReload)

        actionOnOpen = QtGui.QAction(QtGui.QIcon(":/icons/document-open.svg"),
                                     translate("Arch", "Open reference"),
                                     menu)
        QtCore.QObject.connect(actionOnOpen,
                               QtCore.SIGNAL("triggered()"),
                               self.onOpen)
        menu.addAction(actionOnOpen)


    def edit(self):

        FreeCADGui.ActiveDocument.setEdit(self.Object, 0)


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


    def loadInventor(self, obj):

        "loads an openinventor file and replace the root node of this object"

        filename = obj.Proxy.getFile(obj)
        if not filename:
            return None
        if filename.lower().endswith(".ifc"):
            self.setIFCNode(obj, filename)
            return

        # check inventor contents
        ivstring = self.getInventorString(obj)
        if not ivstring:
            t = translate("Arch","Unable to get lightWeight node for object referenced in")
            FreeCAD.Console.PrintWarning(t+" "+obj.Label+"\n")
            return
        from pivy import coin
        inputnode = coin.SoInput()
        inputnode.setBuffer(ivstring)
        lwnode = coin.SoDB.readAll(inputnode)
        if not isinstance(lwnode,coin.SoSeparator):
            t = translate("Arch","Invalid lightWeight node for object referenced in")
            FreeCAD.Console.PrintError(t+" "+obj.Label+"\n")
            return
        if lwnode.getNumChildren() < 2:
            t = translate("Arch","Invalid lightWeight node for object referenced in")
            FreeCAD.Console.PrintError(t+" "+obj.Label+"\n")
            return
        flatlines = lwnode
        shaded = lwnode.getChild(0)
        wireframe = lwnode.getChild(1)

        # check node contents
        rootnode = obj.ViewObject.RootNode
        if rootnode.getNumChildren() < 3:
            FreeCAD.Console.PrintError(translate("Arch","Invalid root node in")+" "+obj.Label+"\n")
            return
        switch = rootnode.getChild(2)
        if switch.getNumChildren() != 4:
            FreeCAD.Console.PrintError(translate("Arch","Invalid root node in")+" "+obj.Label+"\n")
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
            FreeCAD.Console.PrintError(translate("Arch","Invalid root node in")+" "+obj.Label+"\n")
            return
        switch = rootnode.getChild(2)
        if switch.getNumChildren() != 4:
            FreeCAD.Console.PrintError(translate("Arch","Invalid root node in")+" "+obj.Label+"\n")
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
                line = line.decode("utf8")
                if ("<Object name=" in line) and (part in line):
                    writemode1 = True
                elif writemode1 and ("<Property name=\"SavedInventor\"" in line):
                    writemode1 = False
                    writemode2 = True
                elif writemode2 and ("<FileIncluded file=" in line):
                    n = re.findall(r'file=\"(.*?)\"',line)
                    if n:
                        ivfile = n[0]
                        break
        if not ivfile:
            return None
        if not ivfile in zdoc.namelist():
            return None
        f = zdoc.open(ivfile)
        buf = f.read()
        buf = buf.decode("utf8")
        f.close()
        buf = buf.replace("lineWidth 2","lineWidth "+str(int(obj.ViewObject.LineWidth)))
        return buf


    def setIFCNode(self, obj, filename):

        """Sets the coin node of this object from an IFC file"""

        try:
            from nativeifc import ifc_tools
            from nativeifc import ifc_generator
        except:
            t = translate("Arch","NativeIFC not available - unable to process IFC files")
            FreeCAD.Console.PrintError(t+"\n")
            return
        ifcfile = obj.Proxy.getIfcFile(filename)
        elements = obj.Proxy.getIFCElements(obj, ifcfile)
        node, placement = ifc_generator.generate_coin(ifcfile, elements, cached=True)
        if node:
            ifc_generator.set_representation(obj.ViewObject, node)
            colors = node[0]
            if colors:
                ifc_tools.set_colors(obj, colors)
        else:
            ifc_generator.set_representation(obj.ViewObject, None)
        if placement:
            obj.Placement = placement


class ArchReferenceTaskPanel:


    '''The editmode TaskPanel for Reference objects'''

    def __init__(self,obj):

        self.obj = obj
        self.filename = None
        self.form = QtGui.QWidget()
        self.form.setWindowTitle(translate("Arch","External reference"))
        layout = QtGui.QVBoxLayout(self.form)
        label1 = QtGui.QLabel(translate("Arch","External file")+":")
        layout.addWidget(label1)
        self.fileButton = QtGui.QPushButton(self.form)
        self.openButton = QtGui.QPushButton(self.form)
        self.openButton.setText(translate("Arch","Open"))
        if not self.obj.File:
            self.openButton.setEnabled(False)
        l2 = QtGui.QHBoxLayout()
        layout.addLayout(l2)
        l2.addWidget(self.fileButton)
        l2.addWidget(self.openButton)
        label2 = QtGui.QLabel(translate("Arch","Part to use:"))
        layout.addWidget(label2)
        if self.obj.File:
            self.fileButton.setText(os.path.basename(self.obj.File))
        else:
            self.fileButton.setText(translate("Arch","Choose File"))
        self.partCombo = QtGui.QComboBox(self.form)
        self.partCombo.setEnabled(False)
        layout.addWidget(self.partCombo)
        if hasattr(self.obj.Proxy,"parts"):
            parts = self.obj.Proxy.parts
        else:
            parts = self.obj.Proxy.getPartsList(self.obj)
        if parts:
            self.partCombo.setEnabled(True)
            sortedkeys = sorted(parts)
            self.partCombo.addItem(translate("Arch","None (Use whole object)"),"")
            for k in sortedkeys:
                self.partCombo.addItem(parts[k][0],k)
            if self.obj.Part:
                if self.obj.Part in sortedkeys:
                    self.partCombo.setCurrentIndex(sortedkeys.index(self.obj.Part))
        else:
            self.partCombo.setEnabled(False)
        QtCore.QObject.connect(self.fileButton, QtCore.SIGNAL("clicked()"), self.chooseFile)
        QtCore.QObject.connect(self.openButton, QtCore.SIGNAL("clicked()"), self.openFile)

    def accept(self):

        from PySide import QtCore

        if self.filename:
            if self.filename != self.obj.File:
                self.obj.File = self.filename
                FreeCAD.ActiveDocument.recompute()
        if self.partCombo.currentText():
            i = self.partCombo.currentIndex()
            if i >= 1:
                if self.partCombo.itemData(i) != self.obj.Part:
                    self.obj.Part = self.partCombo.itemData(i)
            else:
                self.obj.Part = ""
            QtCore.QTimer.singleShot(0,FreeCAD.ActiveDocument.recompute)
        if self.filename and self.obj.Label == "External Reference":
            self.obj.Label = os.path.basename(self.filename)
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
        filters = "*.FCStd *.dxf"
        # enable IFC support if NativeIFC is present
        try:
            from nativeifc import ifc_tools
        except:
            pass
        else:
            filters += " *.ifc"
        filters = translate("Arch","Reference files")+" ("+filters+")"
        f = QtGui.QFileDialog.getOpenFileName(self.form,
                                              translate("Arch","Choose reference file"),
                                              loc,
                                              filters)
        if f:
            self.filename = f[0]
            self.fileButton.setText(os.path.basename(self.filename))
            parts = self.obj.Proxy.getPartsList(self.obj,self.filename)
            self.partCombo.clear()
            if parts:
                self.partCombo.setEnabled(True)
                sortedkeys = sorted(parts)
                self.partCombo.addItem(translate("Arch","None (Use whole object)"),"")
                for k in sortedkeys:
                    self.partCombo.addItem(parts[k][0],k)
                if self.obj.Part:
                    if self.obj.Part in sortedkeys:
                        self.partCombo.setCurrentIndex(sortedkeys.index(self.obj.Part))
            else:
                self.partCombo.setEnabled(False)

    def openFile(self):

        if self.obj.File:
            if self.obj.File.lower().endswith(".fcstd"):
                FreeCAD.openDocument(self.obj.File)
            else:
                FreeCAD.loadFile(self.obj.File)
            FreeCADGui.Control.closeDialog()
            FreeCADGui.ActiveDocument.resetEdit()
