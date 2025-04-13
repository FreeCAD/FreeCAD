# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""BIM window command"""

import os

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")
ALLOWEDHOSTS = ["Wall","Structure","Roof"]

def sketch_has_width_and_height_constraint(sketch):
    width_found = False
    height_found = False

    for constr in sketch.Constraints:
        if width_found and height_found:
            break

        if constr.Name == "Width":
            width_found = True
        elif constr.Name == "Height":
            height_found = True

    return (width_found and height_found)

def get_attachment_object(obj):
    if not obj.AttachmentSupport:
        return None
    if isinstance(obj.AttachmentSupport,tuple):
        return obj.AttachmentSupport[0]
    elif isinstance(obj.AttachmentSupport,list):
        return obj.AttachmentSupport[0][0]
    else:
        return obj.AttachmentSupport

class Arch_Window:

    "the Arch Window command definition"

    def __init__(self):

        self.doormode = False

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Window',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Window","Window"),
                'Accel': "W, N",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Window","Creates a window object from a selected object (wire, rectangle or sketch)")}

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        """
        Executed when Arch Window is activated.

        Creates a window from the object selected by the user. If no objects are
        selected, enters an interactive mode to create a window using selected
        points to create a base.
        """

        from draftutils import params
        self.W1 = params.get_param_arch("WindowW1")  # thickness of the fixed frame
        if self.doormode:
            self.Width = params.get_param_arch("DoorWidth")
            self.Height = params.get_param_arch("DoorHeight")
        else:
            self.Width = params.get_param_arch("WindowWidth")
            self.Height = params.get_param_arch("WindowHeight")
        self.RemoveExternal = params.get_param_arch("archRemoveExternal")
        self.Preset = 0
        self.LibraryPreset = 0
        self.Sill = 0
        self.Include = True
        self.baseFace = None
        self.wparams = ["Width","Height","H1","H2","H3","W1","W2","O1","O2"]
        self.wp = None

        self.sel = FreeCADGui.Selection.getSelection()

        if self.sel and self.handleSelectionMode():
            return

        # Fallback to interactive mode
        self.handleInteractiveMode()

    def handleSelectionMode(self):
        import Draft

        FreeCADGui.draftToolBar.offUi()
        obj = self.sel[0]
        if not hasattr(obj,'Shape'):
            return False

        obj_only_has_wires = obj.Shape.Wires and (not obj.Shape.Solids) and (not obj.Shape.Shells)
        if obj_only_has_wires:
            FreeCADGui.Control.closeDialog()

            host = None
            if hasattr(obj,"AttachmentSupport"):
                host = get_attachment_object(obj)
            elif Draft.isClone(obj,"Window") and obj.Objects[0].Inlist:
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
            return True

        # Try to detect an object to use as a window type - TODO we must make this safer

        elif obj.Shape.Solids and (Draft.getType(obj) not in ALLOWEDHOSTS):

            # we consider the selected object as a type
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Window"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("Arch.makeWindow(FreeCAD.ActiveDocument."+obj.Name+")")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
            return True

        return False

    def handleInteractiveMode(self):
        import WorkingPlane
        import draftguitools.gui_trackers as DraftTrackers

        self.wp = WorkingPlane.get_working_plane()

        self.tracker = DraftTrackers.boxTracker()
        self.tracker.length(self.Width)
        self.tracker.width(self.W1)
        self.tracker.height(self.Height)
        self.tracker.on()

        FreeCAD.Console.PrintMessage(translate("Arch","Choose a face on an existing object or select a preset")+"\n")
        FreeCADGui.Snapper.getPoint(callback=self.onSnapperPoint,movecallback=self.onSnapperMove,extradlg=self.taskbox())
        #FreeCADGui.Snapper.setSelectMode(True)

    def getLibraryPresetWindow(self):
        import Draft
        from draftutils.messages import _wrn

        num_existing_objects = len(FreeCAD.ActiveDocument.Objects)
        path = self.librarypresets[self.LibraryPreset][1]
        FreeCADGui.doCommand("FreeCADGui.ActiveDocument.mergeProject('" + path + "')")

        for object in FreeCAD.ActiveDocument.Objects[num_existing_objects:]:
            if not Draft.getType(object) == "Window":
                continue

            if Draft.getType(object.Base) == "Sketcher::SketchObject":
                if not sketch_has_width_and_height_constraint(object.Base):
                    _wrn(translate("Arch", "No Width and/or Height constraint in window sketch. Window not resized."))
            else:
                _wrn(translate("Arch", "Window not based on sketch. Window not aligned or resized."))

            return object
        else:
            _wrn(translate("Arch", "No window found. Cannot continue."))
            return None

    def onSnapperPoint(self,point=None,obj=None):

        "this function is called by the snapper when it has a snap point"

        import Draft
        from ArchWindowPresets import WindowPresets
        self.tracker.off()
        if point is None:
            return
        # if something was selected, override the underlying object
        if self.sel:
            obj = self.sel[0]
        point = point.add(FreeCAD.Vector(0,0,self.Sill))
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Window"))

        FreeCADGui.doCommand("import math, FreeCAD, Arch, DraftGeomUtils, WorkingPlane")
        FreeCADGui.doCommand("wp = WorkingPlane.get_working_plane()")

        if self.baseFace is not None:
            FreeCADGui.doCommand("face = FreeCAD.ActiveDocument." + self.baseFace[0].Name + ".Shape.Faces[" + str(self.baseFace[1]) + "]")
            FreeCADGui.doCommand("pl = DraftGeomUtils.placement_from_face(face, vec_z = wp.axis)")
        else:
            FreeCADGui.doCommand("pl = FreeCAD.Placement()")
            FreeCADGui.doCommand("pl.Rotation = FreeCAD.Rotation(wp.u, wp.axis, -wp.v, 'XZY')")

        FreeCADGui.doCommand("pl.Base = FreeCAD.Vector(" + str(point.x) + ", " + str(point.y) + ", " + str(point.z) + ")")

        is_library_preset = self.LibraryPreset >= 0
        if is_library_preset:
            window = self.getLibraryPresetWindow()
            if window:
                FreeCADGui.doCommand("win = FreeCAD.ActiveDocument.getObject('" + window.Name + "')")
                FreeCADGui.doCommand("win.Base.Placement = pl")
                FreeCADGui.doCommand("win.Normal = pl.Rotation.multVec(FreeCAD.Vector(0, 0, -1))")
                FreeCADGui.doCommand("win.Width = " + str(self.Width))
                FreeCADGui.doCommand("win.Height = " + str(self.Height))
                FreeCADGui.doCommand("win.Base.recompute()")
            else:
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
            if Draft.getType(host) in ALLOWEDHOSTS:
                FreeCADGui.doCommand("win.Hosts = [FreeCAD.ActiveDocument." + host.Name + "]")
                siblings = host.Proxy.getSiblings(host)
                for sibling in siblings:
                    FreeCADGui.doCommand("win.Hosts = win.Hosts + [FreeCAD.ActiveDocument." + sibling.Name + "]")

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        # gui_utils.end_all_events()  # Causes a crash on Linux.
        self.tracker.finalize()
        return

    def onSnapperMove(self,point,info):

        "this function is called by the Snapper when the mouse is moved"

        delta = FreeCAD.Vector(self.Width/2,self.W1/2,self.Height/2)
        delta = delta.add(FreeCAD.Vector(0,0,self.Sill))

        if self.baseFace is None:
            rot = FreeCAD.Rotation(self.wp.u,self.wp.v,-self.wp.axis,"XZY")
            self.tracker.setRotation(rot)
        if info:
            if "Face" in info['Component']:
                import DraftGeomUtils
                o = FreeCAD.ActiveDocument.getObject(info['Object'])
                self.baseFace = [o,int(info['Component'][4:])-1]
                #print("switching to ",o.Label," face ",self.baseFace[1])
                f = o.Shape.Faces[self.baseFace[1]]
                p = DraftGeomUtils.placement_from_face(f,vec_z=self.wp.axis,rotated=True)
                rot = p.Rotation
                self.tracker.setRotation(rot)
        r = self.tracker.trans.rotation.getValue().getValue()
        if r != (0,0,0,1):
            delta = FreeCAD.Rotation(r[0],r[1],r[2],r[3]).multVec(FreeCAD.Vector(delta.x,-delta.y,-delta.z))
        self.tracker.pos(point.add(delta))

    def getLibraryPresetPath(self):
        p = FreeCAD.ParamGet("User parameter:Plugins/parts_library").GetString("destination", "")
        # librarypath should have only forward slashes already, but let's use replace() anyway just to be sure:
        return p.replace("\\", "/") + "/Architectural Parts"

    def scanLibraryPresets(self):
        # because of the use of FreeCADGui.doCommand() backslashes in the
        # paths in librarypresets need to be double escaped "\\\\", so let's
        # use forward slashes instead...

        presets = []
        librarypath = self.getLibraryPresetPath()
        presetdir = FreeCAD.getUserAppDataDir().replace("\\", "/") + "/Arch"
        for path in [librarypath, presetdir]:
            if not os.path.isdir(path):
                continue
            for wtype in ["Windows", "Doors"]:
                wdir = path + "/" + wtype
                if not os.path.isdir(wdir):
                    continue
                for subtype in os.listdir(wdir):
                    subdir = wdir + "/" + subtype
                    if not os.path.isdir(subdir):
                        continue
                for subfile in os.listdir(subdir):
                    if (os.path.isfile(subdir + "/" + subfile) and subfile.lower().endswith(".fcstd")):
                        p1 = wtype + " - " + subtype + " - " + subfile[:-6]
                        p2 = subdir + "/" + subfile
                        presets.append([p1, p2])
        return presets

    def taskbox(self):

        "sets up a taskbox widget"

        from draftutils import params
        from PySide import QtCore, QtGui, QtSvgWidgets
        from ArchWindowPresets import WindowPresets
        w = QtGui.QWidget()
        ui = FreeCADGui.UiLoader()
        w.setWindowTitle(translate("Arch","Window options"))
        grid = QtGui.QGridLayout(w)

        # include box
        include = QtGui.QCheckBox(translate("Arch","Auto include in host object"))
        include.setChecked(True)
        grid.addWidget(include,0,0,1,2)
        include.stateChanged.connect(self.setInclude)

        # sill height
        labels = QtGui.QLabel(translate("Arch","Sill height"))
        values = ui.createWidget("Gui::InputField")
        grid.addWidget(labels,1,0,1,1)
        grid.addWidget(values,1,1,1,1)
        values.valueChanged.connect(self.setSill)

        # check for Parts library and Arch presets
        self.librarypresets = self.scanLibraryPresets()

        # presets box
        labelp = QtGui.QLabel(translate("Arch","Preset"))
        valuep = QtGui.QComboBox()
        valuep.setMinimumContentsLength(6)
        valuep.setSizeAdjustPolicy(QtGui.QComboBox.AdjustToContents)
        valuep.addItems(WindowPresets)
        valuep.setCurrentIndex(self.Preset)
        grid.addWidget(labelp,2,0,1,1)
        grid.addWidget(valuep,2,1,1,1)
        valuep.currentIndexChanged.connect(self.setPreset)
        for it in self.librarypresets:
            valuep.addItem(it[0])

        # image display
        self.pic = QtGui.QLabel()
        grid.addWidget(self.pic,3,0,1,2)
        self.pic.setFixedHeight(128)
        self.pic.hide()

        # SVG display
        self.im = QtSvgWidgets.QSvgWidget(":/ui/ParametersWindowFixed.svg")
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
            if param == "W1":
                wid.setText(FreeCAD.Units.Quantity(self.W1,FreeCAD.Units.Length).UserString)
            elif param == "Width":
                wid.setText(FreeCAD.Units.Quantity(self.Width,FreeCAD.Units.Length).UserString)
            elif param == "Height":
                wid.setText(FreeCAD.Units.Quantity(self.Height,FreeCAD.Units.Length).UserString)
            else:
                n = params.get_param_arch("Window"+param)
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
            i = params.get_param_arch("DoorPreset")
            d = params.get_param_arch("DoorSill")
        else:
            i = params.get_param_arch("WindowPreset")
            d = params.get_param_arch("WindowSill")
        if i < valuep.count():
            valuep.setCurrentIndex(i)
        values.setText(FreeCAD.Units.Quantity(d,FreeCAD.Units.Length).UserString)

        return w

    def getValueChanged(self,p):

        return lambda d : self.setParams(p, d)

    def setSill(self,d):

        from draftutils import params
        self.Sill = d
        if self.doormode:
            params.set_param_arch("DoorSill",d)
        else:
            params.set_param_arch("WindowSill",d)

    def setInclude(self,i):

        self.Include = bool(i)

    def setParams(self,param,d):

        from draftutils import params
        setattr(self,param,d)
        self.tracker.length(self.Width)
        self.tracker.height(self.Height)
        self.tracker.width(self.W1)
        prefix = "Door" if self.doormode and param in ("Width","Height") else "Window"
        params.set_param_arch(prefix+param,d)

    def loadThumbnailFromFCStd(self,path):

        from PySide import QtGui
        try:
            import tempfile
            import zipfile
        except Exception:
            return None

        zfile = zipfile.ZipFile(path)
        files = zfile.namelist()
        # check for meta-file if it's really a FreeCAD document
        if files[0] == "Document.xml":
            image="thumbnails/Thumbnail.png"
            if not image in files:
                return None

            image = zfile.read(image)
            thumbfile = tempfile.mkstemp(suffix='.png')[1]
            thumb = open(thumbfile,"wb")
            thumb.write(image)
            thumb.close()
            return QtGui.QPixmap(thumbfile)

    def setPresetThumbnail(self, i):
        from ArchWindowPresets import WindowPresets, getWindowPresetIcon

        self.pic.hide()
        self.im.show()

        is_builtin_preset = i < len(WindowPresets)
        if is_builtin_preset:
            self.im.load(getWindowPresetIcon(i))
        else:
            # From Library
            self.im.hide()
            path = self.librarypresets[i - len(WindowPresets)][1]
            is_fcstd_file = path.lower().endswith(".fcstd")
            if is_fcstd_file:
                qpixmap = self.loadThumbnailFromFCStd(path)
                if qpixmap:
                    self.pic.setPixmap(qpixmap)
                    self.pic.show()
        #for param in self.wparams:
        #    getattr(self,"val"+param).setEnabled(True)

    def savePresetToParams(self,i):
        from draftutils import params
        if self.doormode:
            params.set_param_arch("DoorPreset",i)
        else:
            params.set_param_arch("WindowPreset",i)

    def setPreset(self,i):
        from ArchWindowPresets import WindowPresets

        self.Preset = i
        self.LibraryPreset = self.Preset - len(WindowPresets)

        self.savePresetToParams(i)

        is_valid_preset = i >= 0
        if not is_valid_preset:
            FreeCADGui.Snapper.setSelectMode(True)
            self.tracker.off()
            self.im.hide()
            for param in self.wparams:
                getattr(self,"val"+param).setEnabled(False)
            return

        FreeCADGui.Snapper.setSelectMode(False)
        self.tracker.length(self.Width)
        self.tracker.height(self.Height)
        self.tracker.width(self.W1)
        self.tracker.on()

        self.setPresetThumbnail(i)

FreeCADGui.addCommand('Arch_Window', Arch_Window())
