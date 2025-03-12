# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
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

        from draftutils import params
        import Draft
        import WorkingPlane
        import draftguitools.gui_trackers as DraftTrackers
        self.sel = FreeCADGui.Selection.getSelection()
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

        # autobuild mode
        if FreeCADGui.Selection.getSelectionEx():
            FreeCADGui.draftToolBar.offUi()
            obj = self.sel[0]
            if hasattr(obj,'Shape'):
                if obj.Shape.Wires and (not obj.Shape.Solids) and (not obj.Shape.Shells):
                    FreeCADGui.Control.closeDialog()
                    host = None
                    if hasattr(obj,"AttachmentSupport"):
                        if obj.AttachmentSupport:
                            if isinstance(obj.AttachmentSupport,tuple):
                                host = obj.AttachmentSupport[0]
                            elif isinstance(obj.AttachmentSupport,list):
                                host = obj.AttachmentSupport[0][0]
                            else:
                                host = obj.AttachmentSupport
                            obj.AttachmentSupport = None # remove
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
        self.wp = WorkingPlane.get_working_plane()

        self.tracker = DraftTrackers.boxTracker()
        self.tracker.length(self.Width)
        self.tracker.width(self.W1)
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

        import Draft
        from draftutils import gui_utils
        from draftutils.messages import _wrn
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

    def update(self,point,info):

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

    def setPreset(self,i):

        from PySide import QtGui
        from draftutils import params
        from ArchWindowPresets import WindowPresets
        self.Preset = i
        if self.doormode:
            params.set_param_arch("DoorPreset",i)
        else:
            params.set_param_arch("WindowPreset",i)
        if i >= 0:
            FreeCADGui.Snapper.setSelectMode(False)
            self.tracker.length(self.Width)
            self.tracker.height(self.Height)
            self.tracker.width(self.W1)
            self.tracker.on()
            self.pic.hide()
            self.im.show()
            if i == 0:
                self.im.load(":/ui/ParametersWindowFixed.svg")
            elif i in [1,8]:
                self.im.load(":/ui/ParametersWindowSimple.svg")
            elif i in [2,4,7]:
                self.im.load(":/ui/ParametersWindowDouble.svg")
            elif i == 3:
                self.im.load(":/ui/ParametersWindowStash.svg")
            elif i == 5:
                self.im.load(":/ui/ParametersDoorSimple.svg")
            elif i == 6:
                self.im.load(":/ui/ParametersDoorGlass.svg")
            elif i == 9:
                self.im.load(":/ui/ParametersOpening.svg")
            else:
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
            #for param in self.wparams:
            #    getattr(self,"val"+param).setEnabled(True)
        else:
            FreeCADGui.Snapper.setSelectMode(True)
            self.tracker.off()
            self.im.hide()
            for param in self.wparams:
                getattr(self,"val"+param).setEnabled(False)


FreeCADGui.addCommand('Arch_Window', Arch_Window())
