#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
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

import os

import FreeCAD
import ArchCommands
import ArchComponent
import Draft
import DraftVecUtils
import ArchWindowPresets
from FreeCAD import Units
from FreeCAD import Vector
from draftutils.messages import _wrn

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui, QtSvg
    from draftutils.translate import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import draftguitools.gui_trackers as DraftTrackers
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchWindow
#  \ingroup ARCH
#  \brief The Window object and tools
#
#  This module provides tools to build Window objects.
#  Windows are Arch objects obtained by extruding a series
#  of wires, and that can be inserted into other Arch objects,
#  by defining a volume that gets subtracted from them.

__title__  = "FreeCAD Window"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

# presets
WindowPartTypes = ["Frame","Solid panel","Glass panel","Louvre"]
AllowedHosts =    ["Wall","Structure","Roof"]
WindowOpeningModes = ["None","Arc 90","Arc 90 inv","Arc 45","Arc 45 inv","Arc 180",
                      "Arc 180 inv","Triangle","Triangle inv","Sliding","Sliding inv"]
WindowPresets = ArchWindowPresets.WindowPresets


def makeWindow(baseobj=None,width=None,height=None,parts=None,name=None):

    '''makeWindow(baseobj,[width,height,parts,name]): creates a window based on the
    given base 2D object (sketch or draft).'''

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    if baseobj:
        if Draft.getType(baseobj) == "Window":
            obj = Draft.clone(baseobj)
            return obj
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Window")
    _Window(obj)
    if name:
        obj.Label = name
    else:
        obj.Label = translate("Arch","Window")
    if FreeCAD.GuiUp:
        _ViewProviderWindow(obj.ViewObject)
        #obj.ViewObject.Transparency=p.GetInt("WindowTransparency",85)
    if width:
        obj.Width = width
    if height:
        obj.Height = height
    if baseobj:
        obj.Normal = baseobj.Placement.Rotation.multVec(FreeCAD.Vector(0,0,-1))
        obj.Base = baseobj
    if parts:
        obj.WindowParts = parts
    else:
        if baseobj:
            if baseobj.getLinkedObject().isDerivedFrom("Part::Part2DObject"):
                # create default component
                if baseobj.Shape.Wires:
                    tp = "Frame"
                    if len(baseobj.Shape.Wires) == 1:
                        tp = "Solid panel"
                    i = 0
                    ws = ''
                    for w in baseobj.Shape.Wires:
                        if w.isClosed():
                            if ws: ws += ","
                            ws += "Wire" + str(i)
                            i += 1
                    obj.WindowParts = ["Default",tp,ws,"1","0"]
            else:
                # bind properties from base obj if existing
                for prop in ["Height","Width","Subvolume","Tag","Description","Material"]:
                    for p in baseobj.PropertiesList:
                        if (p == prop) or p.endswith("_"+prop):
                            obj.setExpression(prop, baseobj.Name+"."+p)

    if obj.Base and FreeCAD.GuiUp:
        obj.Base.ViewObject.DisplayMode = "Wireframe"
        obj.Base.ViewObject.hide()
        from DraftGui import todo
        todo.delay(recolorize,[obj.Document.Name,obj.Name])
    return obj

def recolorize(attr): # names is [docname,objname]

    """Recolorizes an object or a [documentname,objectname] list
    This basically calls the Proxy.colorize(obj) methods of objects that
    have one."""

    if isinstance(attr,list):
        if attr[0] in FreeCAD.listDocuments():
            doc = FreeCAD.getDocument(attr[0])
            obj = doc.getObject(attr[1])
            if obj:
                if obj.ViewObject:
                    if obj.ViewObject.Proxy:
                        obj.ViewObject.Proxy.colorize(obj,force=True)
    elif hasattr(attr,"ViewObject") and attr.ViewObject:
        obj = attr
        if hasattr(obj.ViewObject,"Proxy") and hasattr(obj.ViewObject.Proxy,"colorize"):
            obj.ViewObject.Proxy.colorize(obj,force=True)





class _CommandWindow:

    "the Arch Window command definition"

    def __init__(self):

        self.doormode = False

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Window',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Window","Window"),
                'Accel': "W, N",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Window","Creates a window object from a selected object (wire, rectangle or sketch)")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        self.sel = FreeCADGui.Selection.getSelection()
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        self.Thickness = p.GetFloat("WindowThickness",50)
        self.Width = p.GetFloat("WindowWidth",1000)
        if self.doormode:
            self.Height = p.GetFloat("DoorHeight",2100)
        else:
            self.Height = p.GetFloat("WindowHeight",1000)
        self.RemoveExternal =  p.GetBool("archRemoveExternal",False)
        self.Preset = 0
        self.LibraryPreset = 0
        self.Sill = 0
        self.Include = True
        self.baseFace = None
        self.wparams = ["Width","Height","H1","H2","H3","W1","W2","O1","O2"]

        # autobuild mode
        if FreeCADGui.Selection.getSelectionEx():
            FreeCADGui.draftToolBar.offUi()
            obj = self.sel[0]
            if hasattr(obj,'Shape'):
                if obj.Shape.Wires and (not obj.Shape.Solids) and (not obj.Shape.Shells):
                    FreeCADGui.Control.closeDialog()
                    host = None
                    if hasattr(obj,"Support"):
                        if obj.Support:
                            if isinstance(obj.Support,tuple):
                                host = obj.Support[0]
                            elif isinstance(obj.Support,list):
                                host = obj.Support[0][0]
                            else:
                                host = obj.Support
                            obj.Support = None # remove
                    elif Draft.isClone(obj,"Window"):
                        if obj.Objects[0].Inlist:
                            host = obj.Objects[0].Inlist[0]

                    FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Window"))
                    FreeCADGui.addModule("Arch")
                    FreeCADGui.doCommand("win = Arch.makeWindow(FreeCAD.ActiveDocument."+obj.Name+")")
                    if host and self.Include:
                        FreeCADGui.doCommand("win.Hosts = [FreeCAD.ActiveDocument."+host.Name+"]")
                        siblings = host.Proxy.getSiblings(host)
                        sibs = [host]
                        for sibling in siblings:
                            if not sibling in sibs:
                                sibs.append(sibling)
                                FreeCADGui.doCommand("win.Hosts = win.Hosts+[FreeCAD.ActiveDocument."+sibling.Name+"]")
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                    return

                # Try to detect an object to use as a window type - TODO we must make this safer

                elif obj.Shape.Solids and (Draft.getType(obj) not in ["Wall","Structure","Roof"]):
                    # we consider the selected object as a type
                    FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Window"))
                    FreeCADGui.addModule("Arch")
                    FreeCADGui.doCommand("Arch.makeWindow(FreeCAD.ActiveDocument."+obj.Name+")")
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                    return

        # interactive mode
        if hasattr(FreeCAD,"DraftWorkingPlane"):
            FreeCAD.DraftWorkingPlane.setup()

        self.tracker = DraftTrackers.boxTracker()
        self.tracker.length(self.Width)
        self.tracker.width(self.Thickness)
        self.tracker.height(self.Height)
        self.tracker.on()
        FreeCAD.Console.PrintMessage(translate("Arch","Choose a face on an existing object or select a preset")+"\n")
        FreeCADGui.Snapper.getPoint(callback=self.getPoint,movecallback=self.update,extradlg=self.taskbox())
        #FreeCADGui.Snapper.setSelectMode(True)

    def has_width_and_height_constraint(self, sketch):
        width_found = False
        height_found = False

        for constr in sketch.Constraints:
            if constr.Name == "Width":
                width_found = True
            elif constr.Name == "Height":
                height_found = True
            elif width_found and height_found:
                break

        return (width_found and height_found)

    def getPoint(self,point=None,obj=None):

        "this function is called by the snapper when it has a 3D point"

        self.tracker.finalize()
        if point is None:
            return
        # if something was selected, override the underlying object
        if self.sel:
            obj = self.sel[0]
        point = point.add(FreeCAD.Vector(0,0,self.Sill))
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Window"))

        FreeCADGui.doCommand("import math, FreeCAD, Arch, DraftGeomUtils")
        FreeCADGui.doCommand("wp = FreeCAD.DraftWorkingPlane")

        if self.baseFace is not None:
            FreeCADGui.doCommand("face = FreeCAD.ActiveDocument." + self.baseFace[0].Name + ".Shape.Faces[" + str(self.baseFace[1]) + "]")
            FreeCADGui.doCommand("pl = DraftGeomUtils.placement_from_face(face, vec_z = wp.axis)")
        else:
            FreeCADGui.doCommand("pl = FreeCAD.Placement()")
            FreeCADGui.doCommand("pl.Rotation = FreeCAD.Rotation(wp.u, wp.axis, -wp.v, 'XZY')")

        FreeCADGui.doCommand("pl.Base = FreeCAD.Vector(" + str(point.x) + ", " + str(point.y) + ", " + str(point.z) + ")")

        if self.Preset >= len(WindowPresets):
            # library object
            col = FreeCAD.ActiveDocument.Objects
            path = self.librarypresets[self.Preset - len(WindowPresets)][1]
            FreeCADGui.doCommand("FreeCADGui.ActiveDocument.mergeProject('" + path + "')")
            # find the latest added window
            nol = FreeCAD.ActiveDocument.Objects
            for o in nol[len(col):]:
                if Draft.getType(o) == "Window":
                    if Draft.getType(o.Base) != "Sketcher::SketchObject":
                        _wrn(translate("Arch", "Window not based on sketch. Window not aligned or resized."))
                        self.Include = False
                        break
                    FreeCADGui.doCommand("win = FreeCAD.ActiveDocument.getObject('" + o.Name + "')")
                    FreeCADGui.doCommand("win.Base.Placement = pl")
                    FreeCADGui.doCommand("win.Normal = pl.Rotation.multVec(FreeCAD.Vector(0, 0, -1))")
                    FreeCADGui.doCommand("win.Width = " + str(self.Width))
                    FreeCADGui.doCommand("win.Height = " + str(self.Height))
                    FreeCADGui.doCommand("win.Base.recompute()")
                    if not self.has_width_and_height_constraint(o.Base):
                        _wrn(translate("Arch", "No Width and/or Height constraint in window sketch. Window not resized."))
                    break
            else:
                _wrn(translate("Arch", "No window found. Cannot continue."))
                self.Include = False

        else:
            # preset
            wp = ""
            for p in self.wparams:
                wp += p.lower() + "=" + str(getattr(self,p)) + ", "
            FreeCADGui.doCommand("win = Arch.makeWindowPreset('" + WindowPresets[self.Preset] + "', " + wp + "placement=pl)")

        if self.Include:
            host = None
            if self.baseFace is not None:
                host = self.baseFace[0]
            elif obj:
                host = obj
            if Draft.getType(host) in AllowedHosts:
                FreeCADGui.doCommand("win.Hosts = [FreeCAD.ActiveDocument." + host.Name + "]")
                siblings = host.Proxy.getSiblings(host)
                for sibling in siblings:
                    FreeCADGui.doCommand("win.Hosts = win.Hosts + [FreeCAD.ActiveDocument." + sibling.Name + "]")

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        return

    def update(self,point,info):

        "this function is called by the Snapper when the mouse is moved"

        delta = FreeCAD.Vector(self.Width/2,self.Thickness/2,self.Height/2)
        delta = delta.add(FreeCAD.Vector(0,0,self.Sill))

        wp = FreeCAD.DraftWorkingPlane
        if self.baseFace is None:
            rot = FreeCAD.Rotation(wp.u,wp.v,-wp.axis,"XZY")
            self.tracker.setRotation(rot)
        if info:
            if "Face" in info['Component']:
                import DraftGeomUtils
                o = FreeCAD.ActiveDocument.getObject(info['Object'])
                self.baseFace = [o,int(info['Component'][4:])-1]
                #print("switching to ",o.Label," face ",self.baseFace[1])
                f = o.Shape.Faces[self.baseFace[1]]
                p = DraftGeomUtils.placement_from_face(f,vec_z=wp.axis,rotated=True)
                rot = p.Rotation
                self.tracker.setRotation(rot)
        r = self.tracker.trans.rotation.getValue().getValue()
        if r != (0,0,0,1):
            delta = FreeCAD.Rotation(r[0],r[1],r[2],r[3]).multVec(FreeCAD.Vector(delta.x,-delta.y,-delta.z))
        self.tracker.pos(point.add(delta))

    def taskbox(self):

        "sets up a taskbox widget"

        w = QtGui.QWidget()
        ui = FreeCADGui.UiLoader()
        w.setWindowTitle(translate("Arch","Window options"))
        grid = QtGui.QGridLayout(w)

        # include box
        include = QtGui.QCheckBox(translate("Arch","Auto include in host object"))
        include.setChecked(True)
        grid.addWidget(include,0,0,1,2)
        QtCore.QObject.connect(include,QtCore.SIGNAL("stateChanged(int)"),self.setInclude)

        # sill height
        labels = QtGui.QLabel(translate("Arch","Sill height"))
        values = ui.createWidget("Gui::InputField")
        grid.addWidget(labels,1,0,1,1)
        grid.addWidget(values,1,1,1,1)
        QtCore.QObject.connect(values,QtCore.SIGNAL("valueChanged(double)"),self.setSill)

        # check for Parts library and Arch presets

        # because of the use of FreeCADGui.doCommand() backslashes in the
        # paths in librarypresets need to be double escaped "\\\\", so let's
        # use forward slashes instead...
        self.librarypresets = []
        librarypath = FreeCAD.ParamGet("User parameter:Plugins/parts_library").GetString("destination", "")
        # librarypath should have only forward slashes already, but let's use replace() anyway just to be sure:
        librarypath = librarypath.replace("\\", "/") + "/Architectural Parts"
        presetdir = FreeCAD.getUserAppDataDir().replace("\\", "/") + "/Arch"
        for path in [librarypath, presetdir]:
            if os.path.isdir(path):
                for wtype in ["Windows", "Doors"]:
                    wdir = path + "/" + wtype
                    if os.path.isdir(wdir):
                        for subtype in os.listdir(wdir):
                            subdir = wdir + "/" + subtype
                            if os.path.isdir(subdir):
                                for subfile in os.listdir(subdir):
                                    if (os.path.isfile(subdir + "/" + subfile)
                                            and subfile.lower().endswith(".fcstd")):
                                        self.librarypresets.append([wtype + " - " + subtype + " - " + subfile[:-6],
                                                                    subdir + "/" + subfile])

        # presets box
        labelp = QtGui.QLabel(translate("Arch","Preset"))
        valuep = QtGui.QComboBox()
        valuep.setMinimumContentsLength(6)
        valuep.setSizeAdjustPolicy(QtGui.QComboBox.AdjustToContents)
        valuep.addItems(WindowPresets)
        valuep.setCurrentIndex(self.Preset)
        grid.addWidget(labelp,2,0,1,1)
        grid.addWidget(valuep,2,1,1,1)
        QtCore.QObject.connect(valuep,QtCore.SIGNAL("currentIndexChanged(int)"),self.setPreset)
        for it in self.librarypresets:
            valuep.addItem(it[0])

        # image display
        self.pic = QtGui.QLabel()
        grid.addWidget(self.pic,3,0,1,2)
        self.pic.setFixedHeight(128)
        self.pic.hide()

        # SVG display
        self.im = QtSvg.QSvgWidget(":/ui/ParametersWindowFixed.svg")
        self.im.setMaximumWidth(200)
        self.im.setMinimumHeight(120)
        grid.addWidget(self.im,4,0,1,2)
        #self.im.hide()

        # parameters
        i = 5
        for param in self.wparams:
            lab = QtGui.QLabel(translate("Arch",param))
            setattr(self,"val"+param,ui.createWidget("Gui::InputField"))
            wid = getattr(self,"val"+param)
            if param == "Width":
                wid.setText(FreeCAD.Units.Quantity(self.Width,FreeCAD.Units.Length).UserString)
            elif param == "Height":
                wid.setText(FreeCAD.Units.Quantity(self.Height,FreeCAD.Units.Length).UserString)
            elif param == "O1":
                n = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetFloat("WindowO1",0.0)
                wid.setText(FreeCAD.Units.Quantity(n,FreeCAD.Units.Length).UserString)
                setattr(self,param,n)
            elif param == "W1":
                n = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetFloat("WindowW1",self.Thickness*2)
                wid.setText(FreeCAD.Units.Quantity(n,FreeCAD.Units.Length).UserString)
                setattr(self,param,n)
            else:
                n = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetFloat("Window"+param,self.Thickness)
                wid.setText(FreeCAD.Units.Quantity(n,FreeCAD.Units.Length).UserString)
                setattr(self,param,n)
            grid.addWidget(lab,i,0,1,1)
            grid.addWidget(wid,i,1,1,1)
            i += 1
            valueChanged = self.getValueChanged(param)
            FreeCAD.wid = wid
            QtCore.QObject.connect(getattr(self,"val"+param),QtCore.SIGNAL("valueChanged(double)"), valueChanged)

        # restore saved states
        if self.doormode:
            i = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetInt("DoorPreset",0)
            d = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetFloat("DoorSill",0)
        else:
            i = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetInt("WindowPreset",0)
            d = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetFloat("WindowSill",0)
        if i < valuep.count():
            valuep.setCurrentIndex(i)
        values.setText(FreeCAD.Units.Quantity(d,FreeCAD.Units.Length).UserString)

        return w

    def getValueChanged(self,p):

        return lambda d : self.setParams(p, d)

    def setSill(self,d):

        self.Sill = d
        if self.doormode:
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").SetFloat("DoorSill",d)
        else:
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").SetFloat("WindowSill",d)

    def setInclude(self,i):

        self.Include = bool(i)

    def setParams(self,param,d):

        setattr(self,param,d)
        self.tracker.length(self.Width)
        self.tracker.height(self.Height)
        self.tracker.width(self.W1)
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").SetFloat("Window"+param,d)

    def setPreset(self,i):

        self.Preset = i
        if self.doormode:
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").SetInt("DoorPreset",i)
        else:
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").SetInt("WindowPreset",i)
        if i >= 0:
            FreeCADGui.Snapper.setSelectMode(False)
            self.tracker.length(self.Width)
            self.tracker.width(self.Thickness)
            self.tracker.height(self.Height)
            self.tracker.on()
            self.pic.hide()
            self.im.show()
            if i == 0:
                self.im.load(":/ui/ParametersWindowFixed.svg")
            elif i in [1,8]:
                self.im.load(":/ui/ParametersWindowSimple.svg")
            elif i == 6:
                self.im.load(":/ui/ParametersDoorGlass.svg")
            elif i == 3:
                self.im.load(":/ui/ParametersWindowStash.svg")
            elif i == 5:
                self.im.load(":/ui/ParametersDoorSimple.svg")
            else:
                if i >= len(WindowPresets):
                    # From Library
                    self.im.hide()
                    path = self.librarypresets[i-len(WindowPresets)][1]
                    if path.lower().endswith(".fcstd"):
                        try:
                            import tempfile
                            import zipfile
                        except Exception:
                            pass
                        else:
                            zfile = zipfile.ZipFile(path)
                            files = zfile.namelist()
                            # check for meta-file if it's really a FreeCAD document
                            if files[0] == "Document.xml":
                                image="thumbnails/Thumbnail.png"
                                if image in files:
                                    image = zfile.read(image)
                                    thumbfile = tempfile.mkstemp(suffix='.png')[1]
                                    thumb = open(thumbfile,"wb")
                                    thumb.write(image)
                                    thumb.close()
                                    im = QtGui.QPixmap(thumbfile)
                                    self.pic.setPixmap(im)
                                    self.pic.show()
                else:
                    self.im.load(":/ui/ParametersWindowDouble.svg")
            #for param in self.wparams:
            #    getattr(self,"val"+param).setEnabled(True)
        else:
            FreeCADGui.Snapper.setSelectMode(True)
            self.tracker.off()
            self.im.hide()
            for param in self.wparams:
                getattr(self,"val"+param).setEnabled(False)


class _Window(ArchComponent.Component):

    "The Window object"

    def __init__(self,obj):

        ArchComponent.Component.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Window"
        obj.MoveWithHost = True

        # Add features in the SketchArch External Add-on
        self.addSketchArchFeatures(obj)

    def addSketchArchFeatures(self,obj,linkObj=None,mode=None):
        '''
           To add features in the SketchArch External Add-on  (https://github.com/paullee0/FreeCAD_SketchArch)
           -  import ArchSketchObject module, and
           -  set properties that are common to ArchObjects (including Links) and ArchSketch
              to support the additional features

           To install SketchArch External Add-on, see https://github.com/paullee0/FreeCAD_SketchArch#iv-install
        '''

        try:
            import ArchSketchObject
            ArchSketchObject.ArchSketch.setPropertiesLinkCommon(self, obj, linkObj, mode)
        except:
            pass

    def setProperties(self,obj):

        lp = obj.PropertiesList
        if not "Hosts" in lp:
            obj.addProperty("App::PropertyLinkList","Hosts","Window",QT_TRANSLATE_NOOP("App::Property","The objects that host this window"))
        if not "WindowParts" in lp:
            obj.addProperty("App::PropertyStringList","WindowParts","Window",QT_TRANSLATE_NOOP("App::Property","The components of this window"))
            obj.setEditorMode("WindowParts",2)
        if not "HoleDepth" in lp:
            obj.addProperty("App::PropertyLength","HoleDepth","Window",QT_TRANSLATE_NOOP("App::Property","The depth of the hole that this window makes in its host object. If 0, the value will be calculated automatically."))
        if not "Subvolume" in lp:
            obj.addProperty("App::PropertyLink","Subvolume","Window",QT_TRANSLATE_NOOP("App::Property","An optional object that defines a volume to be subtracted from hosts of this window"))
        if not "Width" in lp:
            obj.addProperty("App::PropertyLength","Width","Window",QT_TRANSLATE_NOOP("App::Property","The width of this window"))
        if not "Height" in lp:
            obj.addProperty("App::PropertyLength","Height","Window",QT_TRANSLATE_NOOP("App::Property","The height of this window"))
        if not "Normal" in lp:
            obj.addProperty("App::PropertyVector","Normal","Window",QT_TRANSLATE_NOOP("App::Property","The normal direction of this window"))
        if not "Preset" in lp:
            obj.addProperty("App::PropertyInteger","Preset","Window",QT_TRANSLATE_NOOP("App::Property","The preset number this window is based on"))
            obj.setEditorMode("Preset",2)
        if not "Frame" in lp:
            obj.addProperty("App::PropertyLength","Frame","Window",QT_TRANSLATE_NOOP("App::Property","The frame size of this window"))
        if not "Offset" in lp:
            obj.addProperty("App::PropertyLength","Offset","Window",QT_TRANSLATE_NOOP("App::Property","The offset size of this window"))
        if not "Area" in lp:
            obj.addProperty("App::PropertyArea","Area","Window",QT_TRANSLATE_NOOP("App::Property","The area of this window"))
        if not "LouvreWidth" in lp:
            obj.addProperty("App::PropertyLength","LouvreWidth","Window",QT_TRANSLATE_NOOP("App::Property","The width of louvre elements"))
        if not "LouvreSpacing" in lp:
            obj.addProperty("App::PropertyLength","LouvreSpacing","Window",QT_TRANSLATE_NOOP("App::Property","The space between louvre elements"))
        if not "Opening" in lp:
            obj.addProperty("App::PropertyPercent","Opening","Window",QT_TRANSLATE_NOOP("App::Property","Opens the subcomponents that have a hinge defined"))
        if not "HoleWire" in lp:
            obj.addProperty("App::PropertyInteger","HoleWire","Window",QT_TRANSLATE_NOOP("App::Property","The number of the wire that defines the hole. If 0, the value will be calculated automatically"))
        if not "SymbolPlan" in lp:
            obj.addProperty("App::PropertyBool","SymbolPlan","Window",QT_TRANSLATE_NOOP("App::Property","Shows plan opening symbols if available"))
        if not "SymbolElevation" in lp:
            obj.addProperty("App::PropertyBool","SymbolElevation","Window",QT_TRANSLATE_NOOP("App::Property","Show elevation opening symbols if available"))
        obj.setEditorMode("VerticalArea",2)
        obj.setEditorMode("HorizontalArea",2)
        obj.setEditorMode("PerimeterLength",2)
        self.Type = "Window"

    def onDocumentRestored(self,obj):

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

        # Add features in the SketchArch External Add-on
        self.addSketchArchFeatures(obj, mode='ODR')

    def onBeforeChange(self,obj,prop):

        if prop in ["Base","WindowParts","Placement","HoleDepth","Height","Width","Hosts"]:
            setattr(self,prop,getattr(obj,prop))

    def onChanged(self,obj,prop):

        self.hideSubobjects(obj,prop)
        if not "Restore" in obj.State:
            if prop in ["Base","WindowParts","Placement","HoleDepth","Height","Width","Hosts"]:
                # anti-recursive loops, bc the base sketch will touch the Placement all the time
                touchhosts = False
                if hasattr(self,prop):
                    if getattr(self,prop) != getattr(obj,prop):
                        touchhosts = True
                if touchhosts and hasattr(self, "Hosts") and hasattr(obj, "Hosts"):
                    for host in set(self.Hosts + obj.Hosts): # use set to remove duplicates
                        # mark host to recompute so it can detect this object
                        host.touch()
            if prop in ["Width","Height","Frame"]:
                if obj.Base:
                    if hasattr(obj.Base,"Constraints") and (prop in [c.Name for c in obj.Base.Constraints]):
                        val = getattr(obj,prop).Value
                        if val > 0:
                            obj.Base.setDatum(prop,val)
            else:
                ArchComponent.Component.onChanged(self,obj,prop)


    def buildShapes(self,obj):

        import Part
        import DraftGeomUtils
        import math
        self.sshapes = []
        self.vshapes = []
        shapes = []
        rotdata = None
        for i in range(int(len(obj.WindowParts)/5)):
            wires = []
            hinge = None
            omode = None
            ssymbols = []
            vsymbols = []
            wstr = obj.WindowParts[(i*5)+2].split(',')
            for s in wstr:
                if "Wire" in s:
                    j = int(s[4:])
                    if obj.Base.Shape.Wires:
                        if len(obj.Base.Shape.Wires) >= j:
                            wires.append(obj.Base.Shape.Wires[j])
                elif "Edge" in s:
                    hinge = int(s[4:])-1
                elif "Mode" in s:
                    omode = int(s[4:])
                    if omode >= len(WindowOpeningModes):
                        # Ignore modes not listed in WindowOpeningModes
                        omode = None
            if wires:
                max_length = 0
                for w in wires:
                    if w.BoundBox.DiagonalLength > max_length:
                        max_length = w.BoundBox.DiagonalLength
                        ext = w
                wires.remove(ext)
                shape = Part.Face(ext)
                norm = shape.normalAt(0,0)
                if hasattr(obj,"Normal"):
                    if obj.Normal:
                        if not DraftVecUtils.isNull(obj.Normal):
                            norm = obj.Normal
                if hinge and omode:
                    opening = None
                    if hasattr(obj,"Opening"):
                        if obj.Opening:
                            opening = obj.Opening/100.0
                    e = obj.Base.Shape.Edges[hinge]
                    ev1 = e.Vertexes[0].Point
                    ev2 = e.Vertexes[-1].Point
                    # choose the one with lowest z to draw the symbol
                    if ev2.z < ev1.z:
                        ev1,ev2 = ev2,ev1
                    # find the point most distant from the hinge
                    p = None
                    d = 0
                    for v in shape.Vertexes:
                        dist = v.Point.distanceToLine(ev1,ev2.sub(ev1))
                        if dist > d:
                            d = dist
                            p = v.Point
                    if p:
                        # bring that point to the level of ev1 if needed
                        chord = p.sub(ev1)
                        enorm = ev2.sub(ev1)
                        proj = DraftVecUtils.project(chord,enorm)
                        v1 = ev1
                        if proj.Length > 0:
                            #chord = p.sub(ev1.add(proj))
                            #p = v1.add(chord)
                            p = p.sub(proj)
                            chord = p.sub(ev1)
                        # calculate symbols
                        v4 = p.add(DraftVecUtils.scale(enorm,0.5))
                        if omode == 1: # Arc 90
                            v2 = v1.add(DraftVecUtils.rotate(chord,math.pi/4,enorm))
                            v3 = v1.add(DraftVecUtils.rotate(chord,math.pi/2,enorm))
                            ssymbols.append(Part.Arc(p,v2,v3).toShape())
                            ssymbols.append(Part.LineSegment(v3,v1).toShape())
                            vsymbols.append(Part.LineSegment(v1,v4).toShape())
                            vsymbols.append(Part.LineSegment(v4,ev2).toShape())
                            if opening:
                                rotdata = [v1,ev2.sub(ev1),90*opening]
                        elif omode == 2: # Arc -90
                            v2 = v1.add(DraftVecUtils.rotate(chord,-math.pi/4,enorm))
                            v3 = v1.add(DraftVecUtils.rotate(chord,-math.pi/2,enorm))
                            ssymbols.append(Part.Arc(p,v2,v3).toShape())
                            ssymbols.append(Part.LineSegment(v3,v1).toShape())
                            vsymbols.append(Part.LineSegment(v1,v4).toShape())
                            vsymbols.append(Part.LineSegment(v4,ev2).toShape())
                            if opening:
                                rotdata = [v1,ev2.sub(ev1),-90*opening]
                        elif omode == 3: # Arc 45
                            v2 = v1.add(DraftVecUtils.rotate(chord,math.pi/8,enorm))
                            v3 = v1.add(DraftVecUtils.rotate(chord,math.pi/4,enorm))
                            ssymbols.append(Part.Arc(p,v2,v3).toShape())
                            ssymbols.append(Part.LineSegment(v3,v1).toShape())
                            vsymbols.append(Part.LineSegment(v1,v4).toShape())
                            vsymbols.append(Part.LineSegment(v4,ev2).toShape())
                            if opening:
                                rotdata = [v1,ev2.sub(ev1),45*opening]
                        elif omode == 4: # Arc -45
                            v2 = v1.add(DraftVecUtils.rotate(chord,-math.pi/8,enorm))
                            v3 = v1.add(DraftVecUtils.rotate(chord,-math.pi/4,enorm))
                            ssymbols.append(Part.Arc(p,v2,v3).toShape())
                            ssymbols.append(Part.LineSegment(v3,v1).toShape())
                            vsymbols.append(Part.LineSegment(v1,v4).toShape())
                            vsymbols.append(Part.LineSegment(v4,ev2).toShape())
                            if opening:
                                rotdata = [v1,ev2.sub(ev1),-45*opening]
                        elif omode == 5: # Arc 180
                            v2 = v1.add(DraftVecUtils.rotate(chord,math.pi/2,enorm))
                            v3 = v1.add(DraftVecUtils.rotate(chord,math.pi,enorm))
                            ssymbols.append(Part.Arc(p,v2,v3).toShape())
                            ssymbols.append(Part.LineSegment(v3,v1).toShape())
                            vsymbols.append(Part.LineSegment(v1,v4).toShape())
                            vsymbols.append(Part.LineSegment(v4,ev2).toShape())
                            if opening:
                                rotdata = [v1,ev2.sub(ev1),180*opening]
                        elif omode == 6: # Arc -180
                            v2 = v1.add(DraftVecUtils.rotate(chord,-math.pi/2,enorm))
                            v3 = v1.add(DraftVecUtils.rotate(chord,-math.pi,enorm))
                            ssymbols.append(Part.Arc(p,v2,v3).toShape())
                            ssymbols.append(Part.LineSegment(v3,v1).toShape())
                            vsymbols.append(Part.LineSegment(v1,v4).toShape())
                            vsymbols.append(Part.LineSegment(v4,ev2).toShape())
                            if opening:
                                rotdata = [ev1,ev2.sub(ev1),-180*opening]
                        elif omode == 7: # tri
                            v2 = v1.add(DraftVecUtils.rotate(chord,math.pi/2,enorm))
                            ssymbols.append(Part.LineSegment(p,v2).toShape())
                            ssymbols.append(Part.LineSegment(v2,v1).toShape())
                            vsymbols.append(Part.LineSegment(v1,v4).toShape())
                            vsymbols.append(Part.LineSegment(v4,ev2).toShape())
                            if opening:
                                rotdata = [v1,ev2.sub(ev1),90*opening]
                        elif omode == 8: # -tri
                            v2 = v1.add(DraftVecUtils.rotate(chord,-math.pi/2,enorm))
                            ssymbols.append(Part.LineSegment(p,v2).toShape())
                            ssymbols.append(Part.LineSegment(v2,v1).toShape())
                            vsymbols.append(Part.LineSegment(v1,v4).toShape())
                            vsymbols.append(Part.LineSegment(v4,ev2).toShape())
                            if opening:
                                rotdata = [v1,ev2.sub(ev1),-90*opening]
                        elif omode == 9: # sliding
                            pass
                        elif omode == 10: # -sliding
                            pass
                exv = FreeCAD.Vector()
                zov = FreeCAD.Vector()
                V = 0
                thk = obj.WindowParts[(i*5)+3]
                if "+V" in thk:
                    thk = thk[:-2]
                    V = obj.Frame.Value
                thk = float(thk) + V
                if thk:
                    exv = DraftVecUtils.scaleTo(norm,thk)
                    shape = shape.extrude(exv)
                    for w in wires:
                        f = Part.Face(w)
                        f = f.extrude(exv)
                        shape = shape.cut(f)
                if obj.WindowParts[(i*5)+4]:
                    V = 0
                    zof = obj.WindowParts[(i*5)+4]
                    if "+V" in zof:
                        zof = zof[:-2]
                        V = obj.Offset.Value
                    zof = float(zof) + V
                    if zof:
                        zov = DraftVecUtils.scaleTo(norm,zof)
                        shape.translate(zov)
                if hinge and omode and 0 < omode < 9:
                    if DraftVecUtils.angle(chord, norm, enorm) < 0:
                        if omode%2 == 0:
                            zov = zov.add(exv)
                    else:
                        if omode%2 == 1:
                            zov = zov.add(exv)
                    for symb in ssymbols:
                        symb.translate(zov)
                    for symb in vsymbols:
                        symb.translate(zov)
                    if rotdata:
                        rotdata[0] = rotdata[0].add(zov)
                if obj.WindowParts[(i*5)+1] == "Louvre":
                    if hasattr(obj,"LouvreWidth"):
                        if obj.LouvreWidth and obj.LouvreSpacing:
                            bb = shape.BoundBox
                            bb.enlarge(10)
                            step = obj.LouvreWidth.Value+obj.LouvreSpacing.Value
                            if step < bb.ZLength:
                                box = Part.makeBox(bb.XLength,bb.YLength,obj.LouvreSpacing.Value)
                                boxes = []
                                for i in range(int(bb.ZLength/step)+1):
                                    b = box.copy()
                                    b.translate(FreeCAD.Vector(bb.XMin,bb.YMin,bb.ZMin+i*step))
                                    boxes.append(b)
                                self.boxes = Part.makeCompound(boxes)
                                #rot = obj.Base.Placement.Rotation
                                #self.boxes.rotate(self.boxes.BoundBox.Center,rot.Axis,math.degrees(rot.Angle))
                                self.boxes.translate(shape.BoundBox.Center.sub(self.boxes.BoundBox.Center))
                                shape = shape.cut(self.boxes)
                if rotdata:
                    shape.rotate(rotdata[0],rotdata[1],rotdata[2])
                shapes.append(shape)
                self.sshapes.extend(ssymbols)
                self.vshapes.extend(vsymbols)
        return shapes

    def execute(self,obj):

        if self.clone(obj):
            clonedProxy = obj.CloneOf.Proxy
            if not (hasattr(clonedProxy, "sshapes") and hasattr(clonedProxy, "vshapes")):
                clonedProxy.buildShapes(obj.CloneOf)
            self.sshapes = clonedProxy.sshapes
            self.vshapes = clonedProxy.vshapes
            if hasattr(clonedProxy, "boxes"):
                self.boxes = clonedProxy.boxes
            return

        import Part
        import DraftGeomUtils
        import math
        pl = obj.Placement
        base = None
        self.sshapes = []
        self.vshapes = []
        if obj.Base:
            if hasattr(obj,'Shape'):
                if hasattr(obj,"WindowParts"):
                    if obj.WindowParts and (len(obj.WindowParts)%5 == 0):
                        shapes = self.buildShapes(obj)
                        if shapes:
                            base = Part.makeCompound(shapes)
                    elif not obj.WindowParts:
                        if not obj.Base.Shape.isNull():
                            base = obj.Base.Shape.copy()
                            # obj placement is already added by applyShape() below
                            #if not DraftGeomUtils.isNull(pl):
                            #    base.Placement = base.Placement.multiply(pl)
                    else:
                        print("Arch: Bad formatting of window parts definitions")

        base = self.processSubShapes(obj,base)
        if base:
            if not base.isNull():
                b = []
                if self.sshapes:
                    if hasattr(obj,"SymbolPlan"):
                        if obj.SymbolPlan:
                            b.extend(self.sshapes)
                    else:
                        b.extend(self.sshapes)
                if self.vshapes:
                    if hasattr(obj,"SymbolElevation"):
                        if obj.SymbolElevation:
                            b.extend(self.vshapes)
                    else:
                        b.extend(self.vshapes)
                if b:
                    base = Part.makeCompound([base]+b)
                    #base = Part.makeCompound([base]+self.sshapes+self.vshapes)
                self.applyShape(obj,base,pl,allowinvalid=True,allownosolid=True)
                obj.Placement = pl
        if hasattr(obj,"Area"):
            obj.Area = obj.Width.Value * obj.Height.Value

        self.executeSketchArchFeatures(obj)

    def executeSketchArchFeatures(self, obj, linkObj=None, index=None, linkElement=None):
        '''
           To execute features in the SketchArch External Add-on  (https://github.com/paullee0/FreeCAD_SketchArch)
           -  import ArchSketchObject module, and
           -  execute features that are common to ArchObjects (including Links) and ArchSketch

           To install SketchArch External Add-on, see https://github.com/paullee0/FreeCAD_SketchArch#iv-install
        '''

        # To execute features in SketchArch External Add-on
        try:
            import ArchSketchObject  # Why needed ? Should have try: addSketchArchFeatures() before !  Need 'per method' ?
            # Execute SketchArch Feature - Intuitive Automatic Placement for Arch Windows/Doors, Equipment etc.
            # see https://forum.freecad.org/viewtopic.php?f=23&t=50802
            ArchSketchObject.updateAttachmentOffset(obj, linkObj)
        except:
            pass

    def appLinkExecute(self, obj, linkObj, index, linkElement):
        '''
            Default Link Execute method() -
            See https://forum.freecad.org/viewtopic.php?f=22&t=42184&start=10#p361124
            @realthunder added support to Links to run Linked Scripted Object's methods()
        '''

        # Add features in the SketchArch External Add-on
        self.addSketchArchFeatures(obj, linkObj)

        # Execute features in the SketchArch External Add-on
        self.executeSketchArchFeatures(obj, linkObj)

    def getSubVolume(self,obj,plac=None):

        "returns a subvolume for cutting in a base object"

        # check if we have a custom subvolume
        if hasattr(obj,"Subvolume"):
            if obj.Subvolume:
                if hasattr(obj.Subvolume,'Shape'):
                    if not obj.Subvolume.Shape.isNull():
                        sh = obj.Subvolume.Shape.copy()
                        pl = FreeCAD.Placement(sh.Placement)
                        pl = pl.multiply(obj.Placement)
                        if plac:
                            pl = pl.multiply(plac)
                        sh.Placement = pl
                        return sh

        # getting extrusion depth
        base = None
        if obj.Base:
            base = obj.Base
        width = 0
        if hasattr(obj,"HoleDepth"):  # the code have not checked whether this is a clone and use the original's HoleDepth; if HoleDepth is set in this object, even it is a clone, the original's HoleDepth is overridden
            if obj.HoleDepth.Value:
                width = obj.HoleDepth.Value
        if not width:
            if base:
                b = base.Shape.BoundBox
                width = max(b.XLength,b.YLength,b.ZLength)
        if not width:
            if Draft.isClone(obj,"Window"):  # check whether this is a clone and use the original's HoleDepth or Shape's Boundbox
                if hasattr(obj,"CloneOf"):
                    orig = obj.CloneOf
                else:
                    orig = obj.Objects[0]
                if orig.Base:
                    base = orig.Base

                if hasattr(orig,"HoleDepth"):
                    if orig.HoleDepth.Value:
                        width = orig.HoleDepth.Value
                if not width:
                    if base:
                        b = base.Shape.BoundBox
                        width = max(b.XLength,b.YLength,b.ZLength)
        if not width:
            width = 1.1112 # some weird value to have little chance to overlap with an existing face

        if not base:
            if Draft.isClone(obj,"Window"):  # if this object has not base, check whether this is a clone and use the original's base
                if hasattr(obj,"CloneOf"):
                    orig = obj.CloneOf
                else:
                    orig = obj.Objects[0]  # not sure what is this exactly
                if orig.Base:
                    base = orig.Base
                else:
                    return None

        # finding which wire to use to drill the hole

        f = None
        if hasattr(obj,"HoleWire"):  # the code have not checked whether this is a clone and use the original's HoleWire; if HoleWire is set in this object, even it is a clone, the original's BoundBox/HoleWire is overridden
            if obj.HoleWire > 0:
                if obj.HoleWire <= len(base.Shape.Wires):
                    f = base.Shape.Wires[obj.HoleWire-1]

        if not f:
            if Draft.isClone(obj,"Window"):
                # check original HoleWire then
                if orig.HoleWire > 0:
                    if orig.HoleWire <= len(base.Shape.Wires):
                        f = base.Shape.Wires[obj.HoleWire-1]

        if not f:
            # finding biggest wire in the base shape
            max_length = 0
            for w in base.Shape.Wires:
                if w.BoundBox.DiagonalLength > max_length:
                    max_length = w.BoundBox.DiagonalLength
                    f = w
        if f:
            import Part
            f = Part.Face(f)
            norm = f.normalAt(0,0)
            if hasattr(obj,"Normal"):
                if obj.Normal:
                    if not DraftVecUtils.isNull(obj.Normal):
                        norm = obj.Normal
            v1 = DraftVecUtils.scaleTo(norm,width)
            f.translate(v1)
            v2 = v1.negative()
            v2 = Vector(v1).multiply(-2)
            f = f.extrude(v2)
            if plac:
                f.Placement = plac
            else:
                f.Placement = obj.Placement
            return f
        return None

    def computeAreas(self,obj):
        return



class _ViewProviderWindow(ArchComponent.ViewProviderComponent):

    "A View Provider for the Window object"

    def __init__(self,vobj):

        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):

        import Arch_rc
        if hasattr(self,"Object"):
            if hasattr(self.Object,"CloneOf"):
                if self.Object.CloneOf:
                    return ":/icons/Arch_Window_Clone.svg"
        return ":/icons/Arch_Window_Tree.svg"

    def updateData(self,obj,prop):

        if prop == "Shape":
            if obj.Base:
                if obj.Base.isDerivedFrom("Part::Compound"):
                    if obj.ViewObject.DiffuseColor != obj.Base.ViewObject.DiffuseColor:
                        if len(obj.Base.ViewObject.DiffuseColor) > 1:
                            obj.ViewObject.DiffuseColor = obj.Base.ViewObject.DiffuseColor
                            obj.ViewObject.update()
            self.colorize(obj)
        elif prop == "CloneOf":
            if hasattr(obj,"CloneOf") and obj.CloneOf:
                mat = None
                if hasattr(obj,"Material"):
                    if obj.Material:
                        mat = obj.Material
                if not mat:
                    if obj.ViewObject.DiffuseColor != obj.CloneOf.ViewObject.DiffuseColor:
                        if len(obj.CloneOf.ViewObject.DiffuseColor) > 1:
                            obj.ViewObject.DiffuseColor = obj.CloneOf.ViewObject.DiffuseColor
                            obj.ViewObject.update()

    def onDelete(self,vobj,subelements):

        for o in vobj.Object.Hosts:
            o.touch()
        return True

    def onChanged(self,vobj,prop):

        if (prop in ["DiffuseColor","Transparency"]) and vobj.Object:
            self.colorize(vobj.Object)
        elif prop == "ShapeColor":
            self.colorize(vobj.Object,force=True)
        ArchComponent.ViewProviderComponent.onChanged(self,vobj,prop)

    def colorize(self,obj,force=False):

        "setting different part colors"
        if hasattr(obj,"CloneOf") and obj.CloneOf:
            if self.areDifferentColors(obj.ViewObject.DiffuseColor,obj.CloneOf.ViewObject.DiffuseColor) or force:
                obj.ViewObject.DiffuseColor = obj.CloneOf.ViewObject.DiffuseColor
            return
        if not obj.Shape:
            return
        if not obj.Shape.Solids:
            return
        solids = obj.Shape.copy().Solids
        #print("Colorizing ", solids)
        colors = []
        base = obj.ViewObject.ShapeColor
        for i in range(len(solids)):
            ccol = None
            if obj.WindowParts and len(obj.WindowParts) > i*5:
                # WindowParts-based window
                name = obj.WindowParts[(i*5)]
                mtype = obj.WindowParts[(i*5)+1]
                ccol = self.getSolidMaterial(obj,name,mtype)
            elif obj.Base and hasattr(obj.Base,"Shape"):
                # Type-based window: obj.Base furnishes the window solids
                sol1 = self.getSolidSignature(solids[i])
                # here we look for all the ways to retrieve a name for each
                # solid. Currently we look for similar solids in the
                if hasattr(obj.Base,"Group"):
                    for child in obj.Base.Group:
                        if hasattr(child,"Shape") and child.Shape and child.Shape.Solids:
                            sol2 = self.getSolidSignature(child.Shape)
                            if sol1 == sol2:
                                ccol = self.getSolidMaterial(obj,child.Label)
                                break
            if not ccol:
                typeidx = (i*5)+1
                if typeidx < len(obj.WindowParts):
                    typ = obj.WindowParts[typeidx]
                    if typ == WindowPartTypes[2]: # transparent parts
                        ccol = ArchCommands.getDefaultColor("WindowGlass")
            if not ccol:
                ccol = base
            colors.extend([ccol for f in solids[i].Faces])
        #print("colors: ",colors)
        if self.areDifferentColors(colors,obj.ViewObject.DiffuseColor) or force:
            obj.ViewObject.DiffuseColor = colors

    def getSolidSignature(self,solid):

        """Returns a tuple defining as uniquely as possible a solid"""

        return (solid.ShapeType,solid.Volume,solid.Area,solid.Length)

    def getSolidMaterial(self,obj,name,mtype=None):

        ccol = None
        if hasattr(obj,"Material"):
            if obj.Material:
                if hasattr(obj.Material,"Materials"):
                    if obj.Material.Names:
                        mat = None
                        if name in obj.Material.Names:
                            mat = obj.Material.Materials[obj.Material.Names.index(name)]
                        elif mtype and (mtype in obj.Material.Names):
                            mat = obj.Material.Materials[obj.Material.Names.index(mtype)]
                        if mat:
                            if 'DiffuseColor' in mat.Material:
                                if "(" in mat.Material['DiffuseColor']:
                                    ccol = tuple([float(f) for f in mat.Material['DiffuseColor'].strip("()").split(",")])
                            if ccol and ('Transparency' in mat.Material):
                                t = float(mat.Material['Transparency'])/100.0
                                ccol = (ccol[0],ccol[1],ccol[2],t)
        return ccol

    def getHingeEdgeIndices(self):

        """returns a list of hinge edge indices (0-based)"""

        # WindowParts example:
        # ["OuterFrame", "Frame",       "Wire0,Wire1",             "100.0+V", "0.00+V",
        #  "InnerFrame", "Frame",       "Wire2,Wire3,Edge8,Mode1", "100.0",   "100.0+V",
        #  "InnerGlass", "Glass panel", "Wire3",                   "10.0",    "150.0+V"]

        idxs = []
        parts = self.Object.WindowParts
        for i in range(len(parts) // 5):
            for s in parts[(i * 5) + 2].split(","):
                if "Edge" in s:
                    idxs.append(int(s[4:]) - 1) # Edge indices in string are 1-based.
        return idxs

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None

        taskd = _ArchWindowTaskPanel()
        taskd.obj = self.Object
        self.sets = [vobj.DisplayMode,vobj.Transparency]
        vobj.DisplayMode = "Shaded"
        vobj.Transparency = 80
        if self.Object.Base:
            self.Object.Base.ViewObject.show()
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None

        vobj.DisplayMode = self.sets[0]
        vobj.Transparency = self.sets[1]
        vobj.DiffuseColor = vobj.DiffuseColor # reset face colors
        if self.Object.Base:
            self.Object.Base.ViewObject.hide()
        FreeCADGui.Control.closeDialog()
        return True

    def setupContextMenu(self, vobj, menu):
        hingeIdxs = self.getHingeEdgeIndices()

        super().contextMenuAddEdit(menu)

        if len(hingeIdxs) > 0:
            actionInvertOpening = QtGui.QAction(QtGui.QIcon(":/icons/Arch_Window_Tree.svg"),
                                                translate("Arch", "Invert opening direction"),
                                                menu)
            QtCore.QObject.connect(actionInvertOpening,
                                   QtCore.SIGNAL("triggered()"),
                                   self.invertOpening)
            menu.addAction(actionInvertOpening)

        if len(hingeIdxs) == 1:
            actionInvertHinge = QtGui.QAction(QtGui.QIcon(":/icons/Arch_Window_Tree.svg"),
                                              translate("Arch", "Invert hinge position"),
                                              menu)
            QtCore.QObject.connect(actionInvertHinge,
                                   QtCore.SIGNAL("triggered()"),
                                   self.invertHinge)
            menu.addAction(actionInvertHinge)

        super().contextMenuAddToggleSubcomponents(menu)

    def invertOpening(self):

        """swaps the opening modes found in this window"""

        pairs = [["Mode"+str(i),"Mode"+str(i+1)] for i in range(1,len(WindowOpeningModes),2)]
        self.invertPairs(pairs)

    def invertHinge(self):

        """swaps the hinge edge of a single hinge edge window"""

        idxs = self.getHingeEdgeIndices()
        if len(idxs) != 1:
            return

        idx = idxs[0]
        end = 0
        for wire in self.Object.Base.Shape.Wires:
            sta = end
            end += len(wire.Edges)
            if sta <= idx < end:
                new = idx + 2 # A rectangular wire is assumed.
                if not (sta <= new < end):
                    new = idx - 2
                break

        pairs = [["Edge" + str(idx + 1), "Edge" + str(new + 1)]]
        self.invertPairs(pairs)
        # Also invert opening direction, so the door still opens towards
        # the same side of the wall
        self.invertOpening()

    def invertPairs(self,pairs):

        """scans the WindowParts of this window and swaps the two elements of each pair, if found"""

        if hasattr(self,"Object"):
            windowparts = self.Object.WindowParts
            nparts = []
            for part in windowparts:
                for pair in pairs:
                    if pair[0] in part:
                        part = part.replace(pair[0],pair[1])
                        break
                    elif pair[1] in part:
                        part = part.replace(pair[1],pair[0])
                        break
                nparts.append(part)
            if nparts != self.Object.WindowParts:
                self.Object.WindowParts = nparts
                FreeCAD.ActiveDocument.recompute()
            else:
                FreeCAD.Console.PrintWarning(translate("Arch","This window has no defined opening")+"\n")



class _ArchWindowTaskPanel:

    '''The TaskPanel for Arch Windows'''

    def __init__(self):

        self.obj = None
        self.baseform = QtGui.QWidget()
        self.baseform.setObjectName("TaskPanel")
        self.grid = QtGui.QGridLayout(self.baseform)
        self.grid.setObjectName("grid")
        self.title = QtGui.QLabel(self.baseform)
        self.grid.addWidget(self.title, 0, 0, 1, 7)
        self.basepanel = ArchComponent.ComponentTaskPanel()
        self.form = [self.baseform,self.basepanel.baseform]

        # base object
        self.tree = QtGui.QTreeWidget(self.baseform)
        self.grid.addWidget(self.tree, 1, 0, 1, 7)
        self.tree.setColumnCount(1)
        self.tree.setMaximumSize(QtCore.QSize(500,24))
        self.tree.header().hide()

        # hole
        self.holeLabel = QtGui.QLabel(self.baseform)
        self.grid.addWidget(self.holeLabel, 2, 0, 1, 1)

        self.holeNumber = QtGui.QLineEdit(self.baseform)
        self.grid.addWidget(self.holeNumber, 2, 2, 1, 3)

        self.holeButton = QtGui.QPushButton(self.baseform)
        self.grid.addWidget(self.holeButton, 2, 6, 1, 1)
        self.holeButton.setEnabled(True)

        # trees
        self.wiretree = QtGui.QTreeWidget(self.baseform)
        self.grid.addWidget(self.wiretree, 3, 0, 1, 3)
        self.wiretree.setColumnCount(1)
        self.wiretree.setSelectionMode(QtGui.QAbstractItemView.MultiSelection)

        self.comptree = QtGui.QTreeWidget(self.baseform)
        self.grid.addWidget(self.comptree, 3, 4, 1, 3)
        self.comptree.setColumnCount(1)

        # buttons
        self.addButton = QtGui.QPushButton(self.baseform)
        self.addButton.setObjectName("addButton")
        self.addButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.addButton, 4, 0, 1, 1)
        self.addButton.setMaximumSize(QtCore.QSize(70,40))

        self.editButton = QtGui.QPushButton(self.baseform)
        self.editButton.setObjectName("editButton")
        self.editButton.setIcon(QtGui.QIcon(":/icons/Draft_Edit.svg"))
        self.grid.addWidget(self.editButton, 4, 2, 1, 3)
        self.editButton.setMaximumSize(QtCore.QSize(60,40))
        self.editButton.setEnabled(False)

        self.delButton = QtGui.QPushButton(self.baseform)
        self.delButton.setObjectName("delButton")
        self.delButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.grid.addWidget(self.delButton, 4, 6, 1, 1)
        self.delButton.setMaximumSize(QtCore.QSize(70,40))
        self.delButton.setEnabled(False)

        # invert buttons
        self.invertOpeningButton = QtGui.QPushButton(self.baseform)
        self.invertOpeningButton.setIcon(QtGui.QIcon(":/icons/Arch_Window_Tree.svg"))
        self.invertOpeningButton.clicked.connect(self.invertOpening)
        self.grid.addWidget(self.invertOpeningButton, 5, 0, 1, 7)
        self.invertOpeningButton.setEnabled(False)
        self.invertHingeButton = QtGui.QPushButton(self.baseform)
        self.invertHingeButton.setIcon(QtGui.QIcon(":/icons/Arch_Window_Tree.svg"))
        self.invertHingeButton.clicked.connect(self.invertHinge)
        self.grid.addWidget(self.invertHingeButton, 6, 0, 1, 7)
        self.invertHingeButton.setEnabled(False)

        # add new

        ui = FreeCADGui.UiLoader()
        self.newtitle = QtGui.QLabel(self.baseform)
        self.new1 = QtGui.QLabel(self.baseform)
        self.new2 = QtGui.QLabel(self.baseform)
        self.new3 = QtGui.QLabel(self.baseform)
        self.new4 = QtGui.QLabel(self.baseform)
        self.new5 = QtGui.QLabel(self.baseform)
        self.new6 = QtGui.QLabel(self.baseform)
        self.new7 = QtGui.QLabel(self.baseform)
        self.field1 = QtGui.QLineEdit(self.baseform)
        self.field2 = QtGui.QComboBox(self.baseform)
        self.field3 = QtGui.QLineEdit(self.baseform)
        self.field4 = ui.createWidget("Gui::InputField")
        self.field5 = ui.createWidget("Gui::InputField")
        self.field6 = QtGui.QPushButton(self.baseform)
        self.field7 = QtGui.QComboBox(self.baseform)
        self.addp4 = QtGui.QCheckBox(self.baseform)
        self.addp5 = QtGui.QCheckBox(self.baseform)
        self.createButton = QtGui.QPushButton(self.baseform)
        self.createButton.setObjectName("createButton")
        self.createButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.newtitle, 7, 0, 1, 7)
        self.grid.addWidget(self.new1, 8, 0, 1, 1)
        self.grid.addWidget(self.field1, 8, 2, 1, 5)
        self.grid.addWidget(self.new2, 9, 0, 1, 1)
        self.grid.addWidget(self.field2, 9, 2, 1, 5)
        self.grid.addWidget(self.new3, 10, 0, 1, 1)
        self.grid.addWidget(self.field3, 10, 2, 1, 5)
        self.grid.addWidget(self.new4, 11, 0, 1, 1)
        self.grid.addWidget(self.field4, 11, 2, 1, 4)
        self.grid.addWidget(self.addp4, 11, 6, 1, 1)
        self.grid.addWidget(self.new5, 12, 0, 1, 1)
        self.grid.addWidget(self.field5, 12, 2, 1, 4)
        self.grid.addWidget(self.addp5, 12, 6, 1, 1)
        self.grid.addWidget(self.new6, 13, 0, 1, 1)
        self.grid.addWidget(self.field6, 13, 2, 1, 5)
        self.grid.addWidget(self.new7, 14, 0, 1, 1)
        self.grid.addWidget(self.field7, 14, 2, 1, 5)
        self.grid.addWidget(self.createButton, 15, 0, 1, 7)
        self.newtitle.setVisible(False)
        self.new1.setVisible(False)
        self.new2.setVisible(False)
        self.new3.setVisible(False)
        self.new4.setVisible(False)
        self.new5.setVisible(False)
        self.new6.setVisible(False)
        self.new7.setVisible(False)
        self.field1.setVisible(False)
        self.field2.setVisible(False)
        for t in WindowPartTypes:
            self.field2.addItem("")
        self.field3.setVisible(False)
        self.field3.setReadOnly(True)
        self.field4.setVisible(False)
        self.field5.setVisible(False)
        self.field6.setVisible(False)
        self.field7.setVisible(False)
        self.addp4.setVisible(False)
        self.addp5.setVisible(False)
        for t in WindowOpeningModes:
            self.field7.addItem("")
        self.createButton.setVisible(False)

        QtCore.QObject.connect(self.holeButton, QtCore.SIGNAL("clicked()"), self.selectHole)
        QtCore.QObject.connect(self.holeNumber, QtCore.SIGNAL("textEdited(QString)"), self.setHoleNumber)
        QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        QtCore.QObject.connect(self.delButton, QtCore.SIGNAL("clicked()"), self.removeElement)
        QtCore.QObject.connect(self.editButton, QtCore.SIGNAL("clicked()"), self.editElement)
        QtCore.QObject.connect(self.createButton, QtCore.SIGNAL("clicked()"), self.create)
        QtCore.QObject.connect(self.comptree, QtCore.SIGNAL("itemClicked(QTreeWidgetItem*,int)"), self.check)
        QtCore.QObject.connect(self.wiretree, QtCore.SIGNAL("itemClicked(QTreeWidgetItem*,int)"), self.select)
        QtCore.QObject.connect(self.field6, QtCore.SIGNAL("clicked()"), self.addEdge)
        self.update()

        FreeCADGui.Selection.clearSelection()

    def isAllowedAlterSelection(self):

        return True

    def isAllowedAlterView(self):

        return True

    def getStandardButtons(self):

        return int(QtGui.QDialogButtonBox.Close)

    def check(self,wid,col):

        self.editButton.setEnabled(True)
        self.delButton.setEnabled(True)

    def select(self,wid,col):

        FreeCADGui.Selection.clearSelection()
        ws = ''
        for it in self.wiretree.selectedItems():
            if ws: ws += ","
            ws += str(it.text(0))
            w = int(str(it.text(0)[4:]))
            if self.obj:
                if self.obj.Base:
                    edges = self.obj.Base.Shape.Wires[w].Edges
                    for e in edges:
                        for i in range(len(self.obj.Base.Shape.Edges)):
                            if e.hashCode() == self.obj.Base.Shape.Edges[i].hashCode():
                                FreeCADGui.Selection.addSelection(self.obj.Base,"Edge"+str(i+1))
        self.field3.setText(ws)

    def selectHole(self):

        "takes a selected edge to determine current Hole Wire"

        s = FreeCADGui.Selection.getSelectionEx()
        if s and self.obj:
            if s[0].SubElementNames:
                if "Edge" in s[0].SubElementNames[0]:
                    for i,w in enumerate(self.obj.Base.Shape.Wires):
                        for e in w.Edges:
                            if e.hashCode() == s[0].SubObjects[0].hashCode():
                                self.holeNumber.setText(str(i+1))
                                self.setHoleNumber(str(i+1))
                                break

    def setHoleNumber(self,val):

        "sets the HoleWire obj property"

        if val.isdigit():
            val = int(val)
            if self.obj:
                if not hasattr(self.obj,"HoleWire"):
                    self.obj.addProperty("App::PropertyInteger","HoleWire","Arch",QT_TRANSLATE_NOOP("App::Property","The number of the wire that defines the hole. A value of 0 means automatic"))
                self.obj.HoleWire = val

    def getIcon(self,obj):

        if hasattr(obj.ViewObject,"Proxy"):
            if hasattr(obj.ViewObject.Proxy,"getIcon"):
                return QtGui.QIcon(obj.ViewObject.Proxy.getIcon())
        elif obj.isDerivedFrom("Sketcher::SketchObject"):
            return QtGui.QIcon(":/icons/Sketcher_Sketch.svg")
        elif hasattr(obj.ViewObject, "Icon"):
            return QtGui.QIcon(obj.ViewObject.Icon)
        return QtGui.QIcon(":/icons/Part_3D_object.svg")

    def update(self):

        'fills the tree widgets'

        self.tree.clear()
        self.wiretree.clear()
        self.comptree.clear()
        if self.obj:
            if self.obj.Base:
                item = QtGui.QTreeWidgetItem(self.tree)
                item.setText(0,self.obj.Base.Name)
                item.setIcon(0,self.getIcon(self.obj.Base))
                if hasattr(self.obj.Base,'Shape'):
                    i = 0
                    for w in self.obj.Base.Shape.Wires:
                        if w.isClosed():
                            item = QtGui.QTreeWidgetItem(self.wiretree)
                            item.setText(0,"Wire" + str(i))
                            item.setIcon(0,QtGui.QIcon(":/icons/Draft_Draft.svg"))
                        i += 1
                if self.obj.WindowParts:
                    for p in range(0,len(self.obj.WindowParts),5):
                        item = QtGui.QTreeWidgetItem(self.comptree)
                        item.setText(0,self.obj.WindowParts[p])
                        item.setIcon(0, QtGui.QIcon(":/icons/Part_3D_object.svg"))
                if hasattr(self.obj,"HoleWire"):
                    self.holeNumber.setText(str(self.obj.HoleWire))
                else:
                    self.holeNumber.setText("0")

            self.retranslateUi(self.baseform)
            self.basepanel.obj = self.obj
            self.basepanel.update()
            for wp in self.obj.WindowParts:
                if ("Edge" in wp) and ("Mode" in wp):
                    self.invertOpeningButton.setEnabled(True)
                    self.invertHingeButton.setEnabled(True)
                    break

    def addElement(self):

        'opens the component creation dialog'

        self.field1.setText('')
        self.field3.setText('')
        self.field4.setText('')
        self.field5.setText('')
        self.field6.setText(QtGui.QApplication.translate("Arch", "Get selected edge", None))
        self.field7.setCurrentIndex(0)
        self.addp4.setChecked(False)
        self.addp5.setChecked(False)
        self.newtitle.setVisible(True)
        self.new1.setVisible(True)
        self.new2.setVisible(True)
        self.new3.setVisible(True)
        self.new4.setVisible(True)
        self.new5.setVisible(True)
        self.new6.setVisible(True)
        self.new7.setVisible(True)
        self.field1.setVisible(True)
        self.field2.setVisible(True)
        self.field3.setVisible(True)
        self.field4.setVisible(True)
        self.field5.setVisible(True)
        self.field6.setVisible(True)
        self.field7.setVisible(True)
        self.addp4.setVisible(True)
        self.addp5.setVisible(True)
        self.createButton.setVisible(True)
        self.addButton.setEnabled(False)
        self.editButton.setEnabled(False)
        self.delButton.setEnabled(False)

    def removeElement(self):

        for it in self.comptree.selectedItems():
            comp = str(it.text(0))
            if self.obj:
                p = self.obj.WindowParts
                if comp in self.obj.WindowParts:
                    ind = self.obj.WindowParts.index(comp)
                    for i in range(5):
                        p.pop(ind)
                    self.obj.WindowParts = p
                    self.update()
                    self.editButton.setEnabled(False)
                    self.delButton.setEnabled(False)

    def editElement(self):

        for it in self.comptree.selectedItems():
            self.addElement()
            comp = str(it.text(0))
            if self.obj:
                if comp in self.obj.WindowParts:
                    ind = self.obj.WindowParts.index(comp)
                    self.field6.setText(QtGui.QApplication.translate("Arch", "Get selected edge", None))
                    self.field7.setCurrentIndex(0)
                    for i in range(5):
                        f = getattr(self,"field"+str(i+1))
                        t = self.obj.WindowParts[ind+i]
                        if i == 1:
                            # special behaviour for types
                            if t in WindowPartTypes:
                                f.setCurrentIndex(WindowPartTypes.index(t))
                            else:
                                f.setCurrentIndex(0)
                        elif i == 2:
                            wires = []
                            for l in t.split(","):
                                if "Wire" in l:
                                    wires.append(l)
                                elif "Edge" in l:
                                    self.field6.setText(l)
                                elif "Mode" in l:
                                    if int(l[4:]) < len(WindowOpeningModes):
                                        self.field7.setCurrentIndex(int(l[4:]))
                                    else:
                                        # Ignore modes not listed in WindowOpeningModes
                                        self.field7.setCurrentIndex(0)
                            if wires:
                                f.setText(",".join(wires))

                        elif i in [3,4]:
                            if "+V" in t:
                                t = t[:-2]
                                if i == 3:
                                    self.addp4.setChecked(True)
                                else:
                                    self.addp5.setChecked(True)
                            else:
                                if i == 3:
                                    self.addp4.setChecked(False)
                                else:
                                    self.addp5.setChecked(False)
                            f.setProperty("text",FreeCAD.Units.Quantity(float(t),FreeCAD.Units.Length).UserString)
                        else:
                            f.setText(t)

    def create(self):

        'adds a new component'

        # testing if fields are ok
        ok = True
        ar = []
        for i in range(5):
            if i == 1: # type (1)
                n = getattr(self,"field"+str(i+1)).currentIndex()
                if n in range(len(WindowPartTypes)):
                    t = WindowPartTypes[n]
                else:
                    # if type was not specified or is invalid, we set a default
                    t = WindowPartTypes[0]
            else: # name (0)
                t = str(getattr(self,"field"+str(i+1)).property("text"))
                if t in WindowPartTypes:
                    t = t + "_" # avoiding part names similar to types
            if t == "":
                if not(i in [1,5]):
                    ok = False
            else:
                if i > 2: # thickness (3), offset (4)
                    try:
                        q = FreeCAD.Units.Quantity(t)
                        t = str(q.Value)
                        if i == 3:
                            if self.addp4.isChecked():
                                t += "+V"
                        if i == 4:
                            if self.addp5.isChecked():
                                t += "+V"
                    except (ValueError,TypeError):
                        ok = False
                elif i == 2:
                    # check additional opening parameters
                    hinge = self.field6.property("text")
                    n = self.field7.currentIndex()
                    if (hinge.startswith("Edge")) and (n > 0):
                        # remove accelerator added by Qt
                        hinge = hinge.replace("&","")
                        t += "," + hinge + ",Mode" + str(n)
            ar.append(t)

        if ok:
            if self.obj:
                parts = self.obj.WindowParts
                if ar[0] in parts:
                    b = parts.index(ar[0])
                    for i in range(5):
                        parts[b+i] = ar[i]
                else:
                    parts.extend(ar)
                self.obj.WindowParts = parts
                self.update()
        else:
            FreeCAD.Console.PrintWarning(translate("Arch", "Unable to create component")+"\n")

        self.newtitle.setVisible(False)
        self.new1.setVisible(False)
        self.new2.setVisible(False)
        self.new3.setVisible(False)
        self.new4.setVisible(False)
        self.new5.setVisible(False)
        self.new6.setVisible(False)
        self.new7.setVisible(False)
        self.field1.setVisible(False)
        self.field2.setVisible(False)
        self.field3.setVisible(False)
        self.field4.setVisible(False)
        self.field5.setVisible(False)
        self.field6.setVisible(False)
        self.field7.setVisible(False)
        self.addp4.setVisible(False)
        self.addp5.setVisible(False)
        self.createButton.setVisible(False)
        self.addButton.setEnabled(True)

    def addEdge(self):

        for sel in FreeCADGui.Selection.getSelectionEx():
            for sub in sel.SubElementNames:
                if "Edge" in sub:
                    self.field6.setText(sub)
                    return

    def reject(self):

        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def retranslateUi(self, TaskPanel):

        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Window elements", None))
        self.holeLabel.setText(QtGui.QApplication.translate("Arch", "Hole wire", None))
        self.holeNumber.setToolTip(QtGui.QApplication.translate("Arch", "The number of the wire that defines a hole in the host object. A value of zero will automatically adopt the largest wire", None))
        self.holeButton.setText(QtGui.QApplication.translate("Arch", "Pick selected", None))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add", None))
        self.editButton.setText(QtGui.QApplication.translate("Arch", "Edit", None))
        self.createButton.setText(QtGui.QApplication.translate("Arch", "Create/update component", None))
        self.title.setText(QtGui.QApplication.translate("Arch", "Base 2D object", None))
        self.wiretree.setHeaderLabels([QtGui.QApplication.translate("Arch", "Wires", None)])
        self.comptree.setHeaderLabels([QtGui.QApplication.translate("Arch", "Components", None)])
        self.newtitle.setText(QtGui.QApplication.translate("Arch", "Create new component", None))
        self.new1.setText(QtGui.QApplication.translate("Arch", "Name", None))
        self.new2.setText(QtGui.QApplication.translate("Arch", "Type", None))
        self.new3.setText(QtGui.QApplication.translate("Arch", "Wires", None))
        self.new4.setText(QtGui.QApplication.translate("Arch", "Thickness", None))
        self.new5.setText(QtGui.QApplication.translate("Arch", "Offset", None))
        self.new6.setText(QtGui.QApplication.translate("Arch", "Hinge", None))
        self.new7.setText(QtGui.QApplication.translate("Arch", "Opening mode", None))
        self.addp4.setText(QtGui.QApplication.translate("Arch", "+ default", None))
        self.addp4.setToolTip(QtGui.QApplication.translate("Arch", "If this is checked, the default Frame value of this window will be added to the value entered here", None))
        self.addp5.setText(QtGui.QApplication.translate("Arch", "+ default", None))
        self.addp5.setToolTip(QtGui.QApplication.translate("Arch", "If this is checked, the default Offset value of this window will be added to the value entered here", None))
        self.field6.setText(QtGui.QApplication.translate("Arch", "Get selected edge", None))
        self.field6.setToolTip(QtGui.QApplication.translate("Arch", "Press to retrieve the selected edge", None))
        self.invertOpeningButton.setText(QtGui.QApplication.translate("Arch", "Invert opening direction", None))
        self.invertHingeButton.setText(QtGui.QApplication.translate("Arch", "Invert hinge position", None))
        for i in range(len(WindowPartTypes)):
            self.field2.setItemText(i, QtGui.QApplication.translate("Arch", WindowPartTypes[i], None))
        for i in range(len(WindowOpeningModes)):
            self.field7.setItemText(i, QtGui.QApplication.translate("Arch", WindowOpeningModes[i], None))

    def invertOpening(self):

        if self.obj:
            self.obj.ViewObject.Proxy.invertOpening()

    def invertHinge(self):

        if self.obj:
            self.obj.ViewObject.Proxy.invertHinge()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Window',_CommandWindow())
