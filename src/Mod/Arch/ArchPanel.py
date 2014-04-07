#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
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

import FreeCAD,Draft,ArchComponent,DraftVecUtils,ArchCommands
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

__title__="FreeCAD Panel"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"


Presets = [None]


def makePanel(baseobj=None,length=0,width=0,thickness=0,placement=None,name=translate("Arch","Panel")):
    '''makePanel([obj],[length],[width],[thickness],[placement]): creates a
    panel element based on the given profile object and the given
    extrusion thickness. If no base object is given, you can also specify
    length and width for a simple cubic object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Panel(obj)
    _ViewProviderPanel(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
        obj.Base.ViewObject.hide()
    if width:
        obj.Width = width
    if thickness:
        obj.Thickness = thickness
    if length:
        obj.Length = length
    obj.ViewObject.ShapeColor = ArchCommands.getDefaultColor("Panel")
    return obj


class _CommandPanel:
    "the Arch Panel command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Panel',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Panel","Panel"),
                'Accel': "P, A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Panel","Creates a panel object from scratch or from a selected object (sketch, wire, face or solid)")}
        
    def Activated(self):    
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        self.Length = p.GetFloat("PanelLength",1000)
        self.Width = p.GetFloat("PanelWidth",1000)
        self.Thickness = p.GetFloat("PanelThickness",10)
        self.Profile = None
        self.continueCmd = False
        self.DECIMALS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt("Decimals",2)
        self.FORMAT = "%." + str(self.DECIMALS) + "f mm"
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Panel")))
            FreeCADGui.doCommand("import Arch")
            for obj in sel:
                FreeCADGui.doCommand("Arch.makePanel(FreeCAD.ActiveDocument." + obj.Name + ",thickness=" + str(self.Thickness) + ")")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
            return

        # interactive mode
        if hasattr(FreeCAD,"DraftWorkingPlane"):
            FreeCAD.DraftWorkingPlane.setup()
        import DraftTrackers
        self.points = []
        self.tracker = DraftTrackers.boxTracker()
        self.tracker.width(self.Width)
        self.tracker.height(self.Thickness)
        self.tracker.length(self.Length)
        self.tracker.on()
        FreeCADGui.Snapper.getPoint(callback=self.getPoint,movecallback=self.update,extradlg=self.taskbox())
            
    def getPoint(self,point=None,obj=None):
        "this function is called by the snapper when it has a 3D point"
        self.tracker.finalize()
        if point == None:
            return
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Panel")))
        FreeCADGui.doCommand('import Arch')
        if self.Profile:
            pr = Presets[self.Profile]
            FreeCADGui.doCommand('p = Arch.makeProfile('+str(pr[2])+','+str(pr[3])+','+str(pr[4])+','+str(pr[5])+')')
            FreeCADGui.doCommand('s = Arch.makePanel(p,thickness='+str(self.Thickness)+')')
            #FreeCADGui.doCommand('s.Placement.Rotation = FreeCAD.Rotation(-0.5,0.5,-0.5,0.5)')
        else:
            FreeCADGui.doCommand('s = Arch.makePanel(length='+str(self.Length)+',width='+str(self.Width)+',thickness='+str(self.Thickness)+')')
        FreeCADGui.doCommand('s.Placement.Base = '+DraftVecUtils.toString(point))
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        if self.continueCmd:
            self.Activated()

    def taskbox(self):
        "sets up a taskbox widget"
        w = QtGui.QWidget()
        ui = FreeCADGui.UiLoader()
        w.setWindowTitle(translate("Arch","Panel options").decode("utf8"))
        grid = QtGui.QGridLayout(w)
        
        # presets box
        labelp = QtGui.QLabel(translate("Arch","Preset").decode("utf8"))
        valuep = QtGui.QComboBox()
        fpresets = [" "]
        for p in Presets[1:]:
            fpresets.append(str(translate("Arch",p[0]))+" "+p[1]+" ("+str(p[2])+"x"+str(p[3])+"mm)")
        valuep.addItems(fpresets)
        grid.addWidget(labelp,0,0,1,1)
        grid.addWidget(valuep,0,1,1,1)
        
        # length
        label1 = QtGui.QLabel(translate("Arch","Length").decode("utf8"))
        self.vLength = ui.createWidget("Gui::InputField")
        self.vLength.setText(self.FORMAT % self.Length)
        grid.addWidget(label1,1,0,1,1)
        grid.addWidget(self.vLength,1,1,1,1)
        
        # width
        label2 = QtGui.QLabel(translate("Arch","Width").decode("utf8"))
        self.vWidth = ui.createWidget("Gui::InputField")
        self.vWidth.setText(self.FORMAT % self.Width)
        grid.addWidget(label2,2,0,1,1)
        grid.addWidget(self.vWidth,2,1,1,1)

        # height
        label3 = QtGui.QLabel(translate("Arch","Thickness").decode("utf8"))
        self.vHeight = ui.createWidget("Gui::InputField")
        self.vHeight.setText(self.FORMAT % self.Thickness)
        grid.addWidget(label3,3,0,1,1)
        grid.addWidget(self.vHeight,3,1,1,1)
        
        # horizontal button
        value5 = QtGui.QPushButton(translate("Arch","Rotate").decode("utf8"))
        grid.addWidget(value5,4,0,1,2)

        # continue button
        label4 = QtGui.QLabel(translate("Arch","Con&tinue").decode("utf8"))
        value4 = QtGui.QCheckBox()
        value4.setObjectName("ContinueCmd")
        value4.setLayoutDirection(QtCore.Qt.RightToLeft)
        label4.setBuddy(value4)
        if hasattr(FreeCADGui,"draftToolBar"):
            value4.setChecked(FreeCADGui.draftToolBar.continueMode)
            self.continueCmd = FreeCADGui.draftToolBar.continueMode
        grid.addWidget(label4,5,0,1,1)
        grid.addWidget(value4,5,1,1,1)

        QtCore.QObject.connect(valuep,QtCore.SIGNAL("currentIndexChanged(int)"),self.setPreset)
        QtCore.QObject.connect(self.vLength,QtCore.SIGNAL("valueChanged(double)"),self.setLength)
        QtCore.QObject.connect(self.vWidth,QtCore.SIGNAL("valueChanged(double)"),self.setWidth)
        QtCore.QObject.connect(self.vHeight,QtCore.SIGNAL("valueChanged(double)"),self.setThickness)
        QtCore.QObject.connect(value4,QtCore.SIGNAL("stateChanged(int)"),self.setContinue)
        QtCore.QObject.connect(value5,QtCore.SIGNAL("pressed()"),self.rotate)
        return w
        
    def update(self,point,info):
        "this function is called by the Snapper when the mouse is moved"
        if FreeCADGui.Control.activeDialog():
            delta = Vector(self.Length/2,0,0)
            self.tracker.pos(point.add(delta))
        
    def setWidth(self,d):
        self.Width = d
        self.tracker.width(d)

    def setThickness(self,d):
        self.Thickness = d
        self.tracker.height(d)

    def setLength(self,d):
        self.Length = d
        self.tracker.length(d)

    def setContinue(self,i):
        self.continueCmd = bool(i)
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.continueMode = bool(i)
        
    def setPreset(self,i):
        if i > 0:
            self.vLength.setText(self.FORMAT % float(Presets[i][2]))
            self.vWidth.setText(self.FORMAT % float(Presets[i][3]))
        if len(Presets[i]) == 6:
            self.Profile = i
        else:
            self.Profile = 0
            
    def rotate(self):
        l = self.Length
        w = self.Width
        h = self.Height
        self.vLength.setText(self.FORMAT % h)
        self.vHeight.setText(self.FORMAT % w)
        self.vWidth.setText(self.FORMAT % l)


class _Structure(ArchComponent.Component):
    "The Structure object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLength","Length","Arch",translate("Arch","The length of this element, if not based on a profile"))
        obj.addProperty("App::PropertyLength","Width","Arch",translate("Arch","The width of this element, if not based on a profile"))
        obj.addProperty("App::PropertyLength","Thickness","Arch",translate("Arch","The thickness or extrusion depth of this element"))
        self.Type = "Panel"
        
    def execute(self,obj):
        "creates the panel shape"
        
        import Part, DraftGeomUtils
        
        # base tests
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.isNull():
                    return
            elif obj.Base.isDerivedFrom("Mesh::Feature"):
                if not obj.Base.Mesh.isSolid():
                    return
        else:
            if obj.Length.Value:
                length = obj.Length.Value
            else:
                return
            if obj.Width.Value:
                width = obj.Width.Value
            else:
                return
        if obj.Thickness.Value:
            thickness = obj.Thickness.Value
        else:
            if not obj.Base:
                return
            elif obj.Base.isDerivedFrom("Part::Feature"):
                if not obj.Base.Solids:
                    return                

        # creating base shape
        pl = obj.Placement
        base = None
        if obj.Base:
            p = FreeCAD.Placement(obj.Base.Placement)
            normal = p.Rotation.multVec(Vector(0,0,1))
            normal = normal.multiply(thickness)
            base = obj.Base.Shape.copy()
            if base.Solids:
                pass
            elif base.Faces:
                self.BaseProfile = base
                self.ExtrusionVector = normal
                base = base.extrude(normal)
            elif (len(base.Wires) == 1):
                if base.Wires[0].isClosed():
                    base = Part.Face(base.Wires[0])
                    self.BaseProfile = base
                    self.ExtrusionVector = normal
                    base = base.extrude(normal)               
            elif obj.Base.isDerivedFrom("Mesh::Feature"):
                if obj.Base.Mesh.isSolid():
                    if obj.Base.Mesh.countComponents() == 1:
                        sh = ArchCommands.getShapeFromMesh(obj.Base.Mesh)
                        if sh.isClosed() and sh.isValid() and sh.Solids:
                            base = sh
        else:
            normal = Vector(0,0,1).multiply(thickness)
            self.ExtrusionVector = normal
            l2 = length/2 or 0.5
            w2 = width/2 or 0.5
            v1 = Vector(-l2,-w2,0)
            v2 = Vector(l2,-w2,0)
            v3 = Vector(l2,w2,0)
            v4 = Vector(-l2,w2,0)
            base = Part.makePolygon([v1,v2,v3,v4,v1])
            base = Part.Face(base)
            self.BaseProfile = base
            base = base.extrude(self.ExtrusionVector)

        # process subshapes
        base = self.processSubShapes(obj,base,pl)
            
        # applying
        if base:
            if not base.isNull():
                if base.isValid() and base.Solids:
                    if base.Volume < 0:
                        base.reverse()
                    if base.Volume < 0:
                        FreeCAD.Console.PrintError(translate("Arch","Couldn't compute a shape"))
                        return
                    base = base.removeSplitter()
                    obj.Shape = base
                    if not pl.isNull():
                        obj.Placement = pl


class _ViewProviderPanel(ArchComponent.ViewProviderComponent):
    "A View Provider for the Panel object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Panel_Tree.svg"

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Panel',_CommandPanel())
