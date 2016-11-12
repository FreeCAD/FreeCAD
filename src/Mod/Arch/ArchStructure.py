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
#Modified 2016-01-03 JAndersM

import FreeCAD,Draft,ArchComponent,DraftVecUtils,ArchCommands
from FreeCAD import Vector
import ArchProfile

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt

__title__="FreeCAD Structure"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"


# Possible roles for structural elements
Roles = ["Beam","Column","Slab","Wall","Curtain Wall","Roof","Foundation","Pile","Tendon"]

#Reads preset profiles and categorizes them
Categories=[]
Presets=ArchProfile.readPresets()
for pre in Presets:
    if pre[1] not in Categories:
        Categories.append(pre[1])


def makeStructure(baseobj=None,length=None,width=None,height=None,name="Structure"):
    '''makeStructure([obj],[length],[width],[heigth],[swap]): creates a
    structure element based on the given profile object and the given
    extrusion height. If no base object is given, you can also specify
    length and width for a cubic object.'''
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    obj.Label = translate("Arch",name)
    _Structure(obj)
    if FreeCAD.GuiUp:
        _ViewProviderStructure(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
        if FreeCAD.GuiUp:
            obj.Base.ViewObject.hide()
    if width:
        obj.Width = width
    else:
        obj.Width = p.GetFloat("StructureWidth",100)
    if height:
        obj.Height = height
    else:
        obj.Height = p.GetFloat("StructureHeight",1000)
    if length:
        obj.Length = length
    else:
        if not baseobj:
            # don't set the length if we have a base object, otherwise the lenght X height calc
            # gets wrong
            obj.Length = p.GetFloat("StructureLength",100)
    if obj.Height > obj.Length:
        obj.Role = "Column"
    return obj

def makeStructuralSystem(objects=[],axes=[],name="StructuralSystem"):
    '''makeStructuralSystem(objects,axes): makes a structural system
    based on the given objects and axes'''
    result = []
    if not axes:
        print "At least one axis must be given"
        return
    if objects:
        if not isinstance(objects,list):
            objects = [objects]
    else:
        objects = [None]
    for o in objects:
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
        obj.Label = translate("Arch",name)
        _StructuralSystem(obj)
        if FreeCAD.GuiUp:
            _ViewProviderStructuralSystem(obj.ViewObject)
        if o:
            obj.Base = o
        obj.Axes = axes
        result.append(obj)
        if FreeCAD.GuiUp and o:
            o.ViewObject.hide()
            Draft.formatObject(obj,o)
    FreeCAD.ActiveDocument.recompute()
    if len(result) == 1:
        return result[0]
    else:
        return result

class _CommandStructure:
    "the Arch Structure command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Structure',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Structure","Structure"),
                'Accel': "S, T",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Structure","Creates a structure object from scratch or from a selected object (sketch, wire, face or solid)")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        self.Length = p.GetFloat("StructureLength",100)
        self.Width = p.GetFloat("StructureWidth",100)
        self.Height = p.GetFloat("StructureHeight",1000)
        self.Profile = None
        self.continueCmd = False
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            st = Draft.getObjectsOfType(sel,"Structure")
            ax = Draft.getObjectsOfType(sel,"Axis")
            if ax:
                FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Structural System")))
                FreeCADGui.addModule("Arch")
                if st:
                    FreeCADGui.doCommand("Arch.makeStructuralSystem(" + ArchCommands.getStringList(st) + "," + ArchCommands.getStringList(ax) + ")")
                else:
                    FreeCADGui.doCommand("Arch.makeStructuralSystem(axes=" + ArchCommands.getStringList(ax) + ")")
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
                return
            elif not(ax) and not(st):
                FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Structure")))
                FreeCADGui.addModule("Arch")
                for obj in sel:
                    FreeCADGui.doCommand("Arch.makeStructure(FreeCAD.ActiveDocument." + obj.Name + ")")
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
                return

        # interactive mode
        if hasattr(FreeCAD,"DraftWorkingPlane"):
            FreeCAD.DraftWorkingPlane.setup()
        import DraftTrackers,ArchPrecast
        self.points = []
        self.tracker = DraftTrackers.boxTracker()
        self.tracker.width(self.Width)
        self.tracker.height(self.Height)
        self.tracker.length(self.Length)
        self.tracker.on()
        self.precast = ArchPrecast._PrecastTaskPanel()
        self.dents = ArchPrecast._DentsTaskPanel()
        self.precast.Dents = self.dents
        FreeCADGui.Snapper.getPoint(callback=self.getPoint,movecallback=self.update,extradlg=[self.taskbox(),self.precast.form,self.dents.form])

    def getPoint(self,point=None,obj=None):
        "this function is called by the snapper when it has a 3D point"
        self.tracker.finalize()
        if point == None:
            return
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Structure")))
        FreeCADGui.addModule("Arch")
        if self.Profile is not None:
            if "Precast" in self.Profile:
                # precast concrete
                args = self.precast.getValues()
                args["PrecastType"] = self.Profile.split("_")[1]
                args["Length"] = self.Length
                args["Width"] = self.Width
                args["Height"] = self.Height
                argstring = ""
                for pair in args.items():
                    argstring += pair[0].lower() + "="
                    if isinstance(pair[1],str):
                        argstring += '"' + pair[1] + '",'
                    else:
                        argstring += str(pair[1]) + ","
                FreeCADGui.addModule("ArchPrecast")
                FreeCADGui.doCommand("s = ArchPrecast.makePrecast("+argstring+")")
            else:
                # metal profile
                FreeCADGui.doCommand('p = Arch.makeProfile('+str(self.Profile)+')')
                if self.Length == self.Profile[4]:
                    # vertical
                    FreeCADGui.doCommand('s = Arch.makeStructure(p,height='+str(self.Height)+')')
                else:
                    # horizontal
                    FreeCADGui.doCommand('s = Arch.makeStructure(p,height='+str(self.Length)+')')
                    FreeCADGui.doCommand('s.Placement.Rotation = FreeCAD.Rotation(-0.5,0.5,-0.5,0.5)')
                FreeCADGui.doCommand('s.Profile = "'+self.Profile[2]+'"')
        else :
            FreeCADGui.doCommand('s = Arch.makeStructure(length='+str(self.Length)+',width='+str(self.Width)+',height='+str(self.Height)+')')
        FreeCADGui.doCommand('s.Placement.Base = '+DraftVecUtils.toString(point))
        FreeCADGui.doCommand('s.Placement.Rotation=s.Placement.Rotation.multiply(FreeCAD.DraftWorkingPlane.getRotation().Rotation)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        if self.continueCmd:
            self.Activated()

    def _createItemlist(self, baselist) :
        ilist=[]
        for p in baselist:
            ilist.append(p[2]+" ("+str(p[4])+"x"+str(p[5])+"mm)")
        return ilist

    def taskbox(self):
        "sets up a taskbox widget"
        w = QtGui.QWidget()
        ui = FreeCADGui.UiLoader()
        w.setWindowTitle(translate("Arch","Structure options").decode("utf8"))
        grid = QtGui.QGridLayout(w)

        # categories box
        labelc = QtGui.QLabel(translate("Arch","Category").decode("utf8"))
        valuec = QtGui.QComboBox()
        valuec.addItems([" ","Precast concrete"]+Categories)
        grid.addWidget(labelc,0,0,1,1)
        grid.addWidget(valuec,0,1,1,1)

        # presets box
        labelp = QtGui.QLabel(translate("Arch","Preset").decode("utf8"))
        self.vPresets = QtGui.QComboBox()
        self.pSelect = [None]
        fpresets = [" "]
        self.vPresets.addItems(fpresets)
        grid.addWidget(labelp,1,0,1,1)
        grid.addWidget(self.vPresets,1,1,1,1)

        # length
        label1 = QtGui.QLabel(translate("Arch","Length").decode("utf8"))
        self.vLength = ui.createWidget("Gui::InputField")
        self.vLength.setText(FreeCAD.Units.Quantity(self.Length,FreeCAD.Units.Length).UserString)
        grid.addWidget(label1,2,0,1,1)
        grid.addWidget(self.vLength,2,1,1,1)

        # width
        label2 = QtGui.QLabel(translate("Arch","Width").decode("utf8"))
        self.vWidth = ui.createWidget("Gui::InputField")
        self.vWidth.setText(FreeCAD.Units.Quantity(self.Width,FreeCAD.Units.Length).UserString)
        grid.addWidget(label2,3,0,1,1)
        grid.addWidget(self.vWidth,3,1,1,1)

        # height
        label3 = QtGui.QLabel(translate("Arch","Height").decode("utf8"))
        self.vHeight = ui.createWidget("Gui::InputField")
        self.vHeight.setText(FreeCAD.Units.Quantity(self.Height,FreeCAD.Units.Length).UserString)
        grid.addWidget(label3,4,0,1,1)
        grid.addWidget(self.vHeight,4,1,1,1)

        # horizontal button
        value5 = QtGui.QPushButton(translate("Arch","Switch L/H").decode("utf8"))
        grid.addWidget(value5,5,0,1,1)
        value6 = QtGui.QPushButton(translate("Arch","Switch L/W").decode("utf8"))
        grid.addWidget(value6,5,1,1,1)

        # continue button
        label4 = QtGui.QLabel(translate("Arch","Con&tinue").decode("utf8"))
        value4 = QtGui.QCheckBox()
        value4.setObjectName("ContinueCmd")
        value4.setLayoutDirection(QtCore.Qt.RightToLeft)
        label4.setBuddy(value4)
        if hasattr(FreeCADGui,"draftToolBar"):
            value4.setChecked(FreeCADGui.draftToolBar.continueMode)
            self.continueCmd = FreeCADGui.draftToolBar.continueMode
        grid.addWidget(label4,6,0,1,1)
        grid.addWidget(value4,6,1,1,1)

        QtCore.QObject.connect(valuec,QtCore.SIGNAL("currentIndexChanged(int)"),self.setCategory)
        QtCore.QObject.connect(self.vPresets,QtCore.SIGNAL("currentIndexChanged(int)"),self.setPreset)
        QtCore.QObject.connect(self.vLength,QtCore.SIGNAL("valueChanged(double)"),self.setLength)
        QtCore.QObject.connect(self.vWidth,QtCore.SIGNAL("valueChanged(double)"),self.setWidth)
        QtCore.QObject.connect(self.vHeight,QtCore.SIGNAL("valueChanged(double)"),self.setHeight)
        QtCore.QObject.connect(value4,QtCore.SIGNAL("stateChanged(int)"),self.setContinue)
        QtCore.QObject.connect(value5,QtCore.SIGNAL("pressed()"),self.rotateLH)
        QtCore.QObject.connect(value6,QtCore.SIGNAL("pressed()"),self.rotateLW)
        return w

    def update(self,point,info):
        "this function is called by the Snapper when the mouse is moved"
        if FreeCADGui.Control.activeDialog():
            if self.Height >= self.Length:
                delta = Vector(0,0,self.Height/2)
            else:
                delta = Vector(self.Length/2,0,0)
            self.tracker.pos(point.add(delta))

    def setWidth(self,d):
        self.Width = d
        self.tracker.width(d)

    def setHeight(self,d):
        self.Height = d
        self.tracker.height(d)

    def setLength(self,d):
        self.Length = d
        self.tracker.length(d)

    def setContinue(self,i):
        self.continueCmd = bool(i)
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.continueMode = bool(i)

    def setCategory(self,i):
        self.vPresets.clear()
        if i > 1:
            self.precast.form.hide()
            self.pSelect = [p for p in Presets if p[1] == Categories[i-2]]
            fpresets = self._createItemlist(self.pSelect)
            self.vPresets.addItems(fpresets)
            self.setPreset(0)
        elif i == 1:
            self.precast.form.show()
            self.pSelect = self.precast.PrecastTypes
            fpresets = self.precast.PrecastTypes
            self.vPresets.addItems(fpresets)
            self.setPreset(0)
        else:
            self.precast.form.hide()
            self.pSelect = [None]
            fpresets = [" "]
            self.vPresets.addItems(fpresets)

    def setPreset(self,i):
        self.Profile = None
        elt = self.pSelect[i]
        if elt:
            if elt in self.precast.PrecastTypes:
                self.precast.setPreset(elt)
                self.Profile = "Precast_" + elt
                if elt in ["Pillar","Beam"]:
                    self.dents.form.show()
                else:
                    self.dents.form.hide()
            else:
                p=elt[0]
                self.vLength.setText(FreeCAD.Units.Quantity(float(Presets[p][4]),FreeCAD.Units.Length).UserString)
                self.vWidth.setText(FreeCAD.Units.Quantity(float(Presets[p][5]),FreeCAD.Units.Length).UserString)
                self.Profile = Presets[p]


    def rotateLH(self):
        h = self.Height
        l = self.Length
        self.vLength.setText(FreeCAD.Units.Quantity(h,FreeCAD.Units.Length).UserString)
        self.vHeight.setText(FreeCAD.Units.Quantity(l,FreeCAD.Units.Length).UserString)

    def rotateLW(self):
        w = self.Width
        l = self.Length
        self.vLength.setText(FreeCAD.Units.Quantity(w,FreeCAD.Units.Length).UserString)
        self.vWidth.setText(FreeCAD.Units.Quantity(l,FreeCAD.Units.Length).UserString)


class _Structure(ArchComponent.Component):

    "The Structure object"

    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLink","Tool","Arch",QT_TRANSLATE_NOOP("App::Property","An optional extrusion path for this element"))
        obj.addProperty("App::PropertyLength","Length","Arch",QT_TRANSLATE_NOOP("App::Property","The length of this element, if not based on a profile"))
        obj.addProperty("App::PropertyLength","Width","Arch",QT_TRANSLATE_NOOP("App::Property","The width of this element, if not based on a profile"))
        obj.addProperty("App::PropertyLength","Height","Arch",QT_TRANSLATE_NOOP("App::Property","The height or extrusion depth of this element. Keep 0 for automatic"))
        obj.addProperty("App::PropertyLinkList","Armatures","Arch",QT_TRANSLATE_NOOP("App::Property","Armatures contained in this element"))
        obj.addProperty("App::PropertyVector","Normal","Arch",QT_TRANSLATE_NOOP("App::Property","The normal extrusion direction of this object (keep (0,0,0) for automatic normal)"))
        obj.addProperty("App::PropertyVectorList","Nodes","Arch",QT_TRANSLATE_NOOP("App::Property","The structural nodes of this element"))
        obj.addProperty("App::PropertyString","Profile","Arch",QT_TRANSLATE_NOOP("App::Property","A description of the standard profile this element is based upon"))
        obj.addProperty("App::PropertyDistance","NodesOffset","Arch",QT_TRANSLATE_NOOP("App::Property","Offset distance between the centerline and the nodes line"))
        obj.addProperty("App::PropertyEnumeration","FaceMaker","Arch",QT_TRANSLATE_NOOP("App::Property","The facemaker type to use to build the profile of this object"))
        self.Type = "Structure"
        obj.FaceMaker = ["None","Simple","Cheese","Bullseye"]
        obj.Role = Roles

    def execute(self,obj):
        "creates the structure shape"

        import Part, DraftGeomUtils

        if self.clone(obj):
            return

        import Part, DraftGeomUtils
        base = None
        pl = obj.Placement
        extdata = self.getExtrusionData(obj)
        if extdata:
            base = extdata[0]
            base.Placement = extdata[2].multiply(base.Placement)
            extv = extdata[2].Rotation.multVec(extdata[1])
            if obj.Tool:
                try:
                    base = obj.Tool.Shape.copy().makePipe(obj.Base.Shape.copy())
                except Part.OCCError:
                    FreeCAD.Console.PrintError(translate("Arch","Error: The base shape couldn't be extruded along this tool object")+"\n")
                    return
            else:
                base = base.extrude(extv)
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.isNull():
                    return
                if not obj.Base.Shape.isValid():
                    if not obj.Base.Shape.Solids:
                        # let pass invalid objects if they have solids...
                        return
                elif obj.Base.Shape.Solids:
                    base = obj.Base.Shape.copy()
            elif obj.Base.isDerivedFrom("Mesh::Feature"):
                if obj.Base.Mesh.isSolid():
                    if obj.Base.Mesh.countComponents() == 1:
                        sh = ArchCommands.getShapeFromMesh(obj.Base.Mesh)
                        if sh.isClosed() and sh.isValid() and sh.Solids and (not sh.isNull()):
                            base = sh
                        else:
                            FreeCAD.Console.PrintWarning(translate("Arch","This mesh is an invalid solid")+"\n")
                            obj.Base.ViewObject.show()
        if not base:
            FreeCAD.Console.PrintError(translate("Arch","Error: Invalid base object")+"\n")
            return

        base = self.processSubShapes(obj,base,pl)
        self.applyShape(obj,base,pl)
        
    def getExtrusionData(self,obj):
        """returns (shape,extrusion vector,placement) or None"""
        import Part,DraftGeomUtils
        data = ArchComponent.Component.getExtrusionData(self,obj)
        if data:
            return data
        length  = obj.Length.Value
        width = obj.Width.Value
        height = obj.Height.Value
        normal = None
        if not height:
            for p in obj.InList:
                if Draft.getType(p) == "Floor":
                    if p.Height.Value:
                        height = p.Height.Value
        base = None
        placement = None
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape:
                    if obj.Base.Shape.Solids:
                        return None
                    elif obj.Base.Shape.Faces:
                        if not DraftGeomUtils.isCoplanar(obj.Base.Shape.Faces):
                            return None
                        else:
                            base,placement = self.rebase(obj.Base.Shape)
                            normal = obj.Base.Shape.Faces[0].normalAt(0,0)
                    elif obj.Base.Shape.Wires:
                        baseface = None
                        if hasattr(obj,"FaceMaker"):
                            if obj.FaceMaker != "None":
                                try:
                                    baseface = Part.makeFace(obj.Base.Shape.Wires,"Part::FaceMaker"+str(obj.FaceMaker))
                                except:
                                    FreeCAD.Console.PrintError(translate("Arch","Facemaker returned an error")+"\n")
                                    return None
                                if len(baseface.Faces) > 1:
                                    baseface = baseface.Faces[0]
                                normal = baseface.normalAt(0,0)
                        if not baseface:
                            for w in obj.Base.Shape.Wires:
                                w.fix(0.1,0,1) # fixes self-intersecting wires
                                f = Part.Face(w)
                                if baseface:
                                    baseface = baseface.fuse(f)
                                else:
                                    baseface = f
                                    normal = f.normalAt(0,0)
                        base,placement = self.rebase(baseface)
                    elif (len(obj.Base.Shape.Edges) == 1) and (len(obj.Base.Shape.Vertexes) == 1):
                        # closed edge
                        w = Part.Wire(obj.Base.Shape.Edges[0])
                        baseface = Part.Face(w)
                        base,placement = self.rebase(baseface)
        elif length and width and height:
            if (length > height) and (obj.Role != "Slab"):
                h2 = height/2 or 0.5
                w2 = width/2 or 0.5
                v1 = Vector(0,-w2,-h2)
                v2 = Vector(0,-w2,h2)
                v3 = Vector(0,w2,h2)
                v4 = Vector(0,w2,-h2)
            else:
                l2 = length/2 or 0.5
                w2 = width/2 or 0.5
                v1 = Vector(-l2,-w2,0)
                v2 = Vector(l2,-w2,0)
                v3 = Vector(l2,w2,0)
                v4 = Vector(-l2,w2,0)
            import Part
            baseface = Part.Face(Part.makePolygon([v1,v2,v3,v4,v1]))
            base,placement = self.rebase(baseface)
        if base and placement:
            if obj.Normal == Vector(0,0,0):
                if not normal:
                    normal = Vector(0,0,1)
            else:
                normal = Vector(obj.Normal)
            if (length > height) and (obj.Role != "Slab"):
                extrusion = normal.multiply(length)
            else:
                extrusion = normal.multiply(height)
            return (base,extrusion,placement)
        return None

    def onChanged(self,obj,prop):
        self.hideSubobjects(obj,prop)
        if prop in ["Shape","ResetNodes","NodesOffset"]:
            # ResetNodes is not a property but it allows us to use this function to force reset the nodes
            nodes = None
            extdata = self.getExtrusionData(obj)
            if extdata:
                nodes = extdata[0]
                nodes.Placement = nodes.Placement.multiply(extdata[2])
                if obj.Role not in ["Slab"]:
                    if obj.Tool:
                        nodes = obj.Tool.Shape
                    elif extdata[1].Length > 0:
                        if hasattr(nodes,"CenterOfMass"):
                            import Part
                            nodes = Part.Line(nodes.CenterOfMass,nodes.CenterOfMass.add(extdata[1])).toShape()
            offset = FreeCAD.Vector()
            if hasattr(obj,"NodesOffset"):
                offset = FreeCAD.Vector(0,0,obj.NodesOffset.Value)
            if obj.Nodes and (prop != "ResetNodes"):
                if hasattr(self,"nodes"):
                    if self.nodes:
                        if obj.Nodes != self.nodes:
                            # nodes are set manually: don't touch them
                            return
                else:
                    # nodes haven't been calculated yet, but are set (file load)
                    # we set the nodes now but don't change the property
                    if nodes:
                        self.nodes = [v.Point.add(offset) for v in nodes.Vertexes]
                        return
            # we set the nodes
            if nodes:
                self.nodes = [v.Point.add(offset) for v in nodes.Vertexes]
                obj.Nodes = self.nodes

    def getNodeEdges(self,obj):
        "returns a list of edges from stuctural nodes"
        edges = []
        if obj.Nodes:
            import Part
            for i in range(len(obj.Nodes)-1):
                edges.append(Part.Line(obj.Placement.multVec(obj.Nodes[i]),obj.Placement.multVec(obj.Nodes[i+1])).toShape())
            if hasattr(obj.ViewObject,"NodeType"):
                if (obj.ViewObject.NodeType == "Area") and (len(obj.Nodes) > 2):
                    edges.append(Part.Line(obj.Placement.multVec(obj.Nodes[-1]),obj.Placement.multVec(obj.Nodes[0])).toShape())
        return edges


class _ViewProviderStructure(ArchComponent.ViewProviderComponent):
    "A View Provider for the Structure object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        vobj.addProperty("App::PropertyBool","ShowNodes","Arch",QT_TRANSLATE_NOOP("App::Property","If the nodes are visible or not")).ShowNodes = False
        vobj.addProperty("App::PropertyFloat","NodeLine","Base",QT_TRANSLATE_NOOP("App::Property","The width of the nodes line"))
        vobj.addProperty("App::PropertyFloat","NodeSize","Base",QT_TRANSLATE_NOOP("App::Property","The size of the node points"))
        vobj.addProperty("App::PropertyColor","NodeColor","Base",QT_TRANSLATE_NOOP("App::Property","The color of the nodes line"))
        vobj.addProperty("App::PropertyEnumeration","NodeType","Arch",QT_TRANSLATE_NOOP("App::Property","The type of structural node"))
        vobj.NodeColor = (1.0,1.0,1.0,1.0)
        vobj.NodeSize = 6
        vobj.NodeType = ["Linear","Area"]
        vobj.ShapeColor = ArchCommands.getDefaultColor("Structure")

    def getIcon(self):
        import Arch_rc
        if hasattr(self,"Object"):
            if hasattr(self.Object,"CloneOf"):
                if self.Object.CloneOf:
                    return ":/icons/Arch_Structure_Clone.svg"
        return ":/icons/Arch_Structure_Tree.svg"

    def updateData(self,obj,prop):
        if prop == "Nodes":
            if obj.Nodes:
                if hasattr(self,"nodes"):
                    p = []
                    self.pointset.numPoints.setValue(0)
                    self.lineset.coordIndex.deleteValues(0)
                    self.faceset.coordIndex.deleteValues(0)
                    for n in obj.Nodes:
                        p.append([n.x,n.y,n.z])
                    self.coords.point.setValues(0,len(p),p)
                    self.pointset.numPoints.setValue(len(p))
                    self.lineset.coordIndex.setValues(0,len(p)+1,range(len(p))+[-1])
                    if hasattr(obj.ViewObject,"NodeType"):
                        if (obj.ViewObject.NodeType == "Area") and (len(p) > 2):
                            self.coords.point.set1Value(len(p),p[0][0],p[0][1],p[0][2])
                            self.lineset.coordIndex.setValues(0,len(p)+2,range(len(p)+1)+[-1])
                            self.faceset.coordIndex.setValues(0,len(p)+1,range(len(p))+[-1])
        elif prop == "Role":
            if hasattr(obj.ViewObject,"NodeType"):
                if obj.Role == "Slab":
                    obj.ViewObject.NodeType = "Area"
                else:
                    obj.ViewObject.NodeType = "Linear"
        ArchComponent.ViewProviderComponent.updateData(self,obj,prop)

    def onChanged(self,vobj,prop):
        if prop == "ShowNodes":
            if hasattr(self,"nodes"):
                vobj.Annotation.removeChild(self.nodes)
                del self.nodes
            if vobj.ShowNodes:
                from pivy import coin
                self.nodes = coin.SoAnnotation()
                self.coords = coin.SoCoordinate3()
                self.mat = coin.SoMaterial()
                self.pointstyle = coin.SoDrawStyle()
                self.pointstyle.style = coin.SoDrawStyle.POINTS
                self.pointset = coin.SoType.fromName("SoBrepPointSet").createInstance()
                self.linestyle = coin.SoDrawStyle()
                self.linestyle.style = coin.SoDrawStyle.LINES
                self.lineset = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
                self.facestyle = coin.SoDrawStyle()
                self.facestyle.style = coin.SoDrawStyle.FILLED
                self.shapehints = coin.SoShapeHints()
                self.shapehints.faceType = coin.SoShapeHints.UNKNOWN_FACE_TYPE
                self.fmat = coin.SoMaterial()
                self.fmat.transparency.setValue(0.75)
                self.faceset = coin.SoIndexedFaceSet()
                self.nodes.addChild(self.coords)
                self.nodes.addChild(self.mat)
                self.nodes.addChild(self.pointstyle)
                self.nodes.addChild(self.pointset)
                self.nodes.addChild(self.linestyle)
                self.nodes.addChild(self.lineset)
                self.nodes.addChild(self.facestyle)
                self.nodes.addChild(self.shapehints)
                self.nodes.addChild(self.fmat)
                self.nodes.addChild(self.faceset)
                vobj.Annotation.addChild(self.nodes)
                self.updateData(vobj.Object,"Nodes")
                self.onChanged(vobj,"NodeColor")
                self.onChanged(vobj,"NodeLine")
                self.onChanged(vobj,"NodeSize")
        elif prop == "NodeColor":
            if hasattr(self,"mat"):
                l = vobj.NodeColor
                self.mat.diffuseColor.setValue([l[0],l[1],l[2]])
                self.fmat.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "NodeLine":
            if hasattr(self,"linestyle"):
                self.linestyle.lineWidth = vobj.NodeLine
        elif prop == "NodeSize":
            if hasattr(self,"pointstyle"):
                self.pointstyle.pointSize = vobj.NodeSize
        elif prop == "NodeType":
            self.updateData(vobj.Object,"Nodes")
        ArchComponent.ViewProviderComponent.onChanged(self,vobj,prop)

    def setEdit(self,vobj,mode):
        if mode == 0:
            taskd = StructureTaskPanel(vobj.Object)
            taskd.obj = self.Object
            taskd.update()
            FreeCADGui.Control.showDialog(taskd)
            return True
        return False


class StructureTaskPanel(ArchComponent.ComponentTaskPanel):

    def __init__(self,obj):
        ArchComponent.ComponentTaskPanel.__init__(self)
        self.optwid = QtGui.QWidget()
        self.optwid.setWindowTitle(QtGui.QApplication.translate("Arch", "Node Tools", None, QtGui.QApplication.UnicodeUTF8))
        lay = QtGui.QVBoxLayout(self.optwid)

        self.resetButton = QtGui.QPushButton(self.optwid)
        self.resetButton.setIcon(QtGui.QIcon(":/icons/edit-undo.svg"))
        self.resetButton.setText(QtGui.QApplication.translate("Arch", "Reset nodes", None, QtGui.QApplication.UnicodeUTF8))

        lay.addWidget(self.resetButton)
        QtCore.QObject.connect(self.resetButton, QtCore.SIGNAL("clicked()"), self.resetNodes)

        self.editButton = QtGui.QPushButton(self.optwid)
        self.editButton.setIcon(QtGui.QIcon(":/icons/Draft_Edit.svg"))
        self.editButton.setText(QtGui.QApplication.translate("Arch", "Edit nodes", None, QtGui.QApplication.UnicodeUTF8))
        lay.addWidget(self.editButton)
        QtCore.QObject.connect(self.editButton, QtCore.SIGNAL("clicked()"), self.editNodes)

        self.extendButton = QtGui.QPushButton(self.optwid)
        self.extendButton.setIcon(QtGui.QIcon(":/icons/Snap_Perpendicular.svg"))
        self.extendButton.setText(QtGui.QApplication.translate("Arch", "Extend nodes", None, QtGui.QApplication.UnicodeUTF8))
        self.extendButton.setToolTip(QtGui.QApplication.translate("Arch", "Extends the nodes of this element to reach the nodes of another element", None, QtGui.QApplication.UnicodeUTF8))
        lay.addWidget(self.extendButton)
        QtCore.QObject.connect(self.extendButton, QtCore.SIGNAL("clicked()"), self.extendNodes)

        self.connectButton = QtGui.QPushButton(self.optwid)
        self.connectButton.setIcon(QtGui.QIcon(":/icons/Snap_Intersection.svg"))
        self.connectButton.setText(QtGui.QApplication.translate("Arch", "Connect nodes", None, QtGui.QApplication.UnicodeUTF8))
        self.connectButton.setToolTip(QtGui.QApplication.translate("Arch", "Connects nodes of this element with the nodes of another element", None, QtGui.QApplication.UnicodeUTF8))
        lay.addWidget(self.connectButton)
        QtCore.QObject.connect(self.connectButton, QtCore.SIGNAL("clicked()"), self.connectNodes)

        self.toggleButton = QtGui.QPushButton(self.optwid)
        self.toggleButton.setIcon(QtGui.QIcon(":/icons/dagViewVisible.svg"))
        self.toggleButton.setText(QtGui.QApplication.translate("Arch", "Toggle all nodes", None, QtGui.QApplication.UnicodeUTF8))
        self.toggleButton.setToolTip(QtGui.QApplication.translate("Arch", "Toggles all structural nodes of the document on/off", None, QtGui.QApplication.UnicodeUTF8))
        lay.addWidget(self.toggleButton)
        QtCore.QObject.connect(self.toggleButton, QtCore.SIGNAL("clicked()"), self.toggleNodes)

        self.form = [self.form,self.optwid]
        self.Object = obj
        self.observer = None
        self.nodevis = None

    def editNodes(self):
        FreeCADGui.Control.closeDialog()
        FreeCADGui.runCommand("Draft_Edit")

    def resetNodes(self):
        self.Object.Proxy.onChanged(self.Object,"ResetNodes")

    def extendNodes(self,other=None):
        if not other:
            self.observer = StructSelectionObserver(self.extendNodes)
            FreeCADGui.Selection.addObserver(self.observer)
            FreeCAD.Console.PrintMessage(translate("Arch","Pick another Structure object: "))
        else:
            FreeCADGui.Selection.removeObserver(self.observer)
            self.observer = None
            if Draft.getType(other) != "Structure":
                FreeCAD.Console.PrintError(translate("Arch","The picked object is not a Structure\n"))
            else:
                if not other.Nodes:
                    FreeCAD.Console.PrintError(translate("Arch","The picked object has no structural nodes\n"))
                else:
                    if (len(self.Object.Nodes) != 2) or (len(other.Nodes) != 2):
                        FreeCAD.Console.PrintError(translate("Arch","One of these objects has more than 2 nodes\n"))
                    else:
                        import DraftGeomUtils
                        nodes1 = [self.Object.Placement.multVec(v) for v in self.Object.Nodes]
                        nodes2 = [other.Placement.multVec(v) for v in other.Nodes]
                        intersect = DraftGeomUtils.findIntersection(nodes1[0],nodes1[1],nodes2[0],nodes2[1],True,True)
                        if not intersect:
                            FreeCAD.Console.PrintError(translate("Arch","Unable to find a suitable intersection point\n"))
                        else:
                            intersect = intersect[0]
                            FreeCAD.Console.PrintMessage(translate("Arch","Intersection found.\n"))
                            if DraftGeomUtils.findClosest(intersect,nodes1) == 0:
                                self.Object.Nodes = [self.Object.Placement.inverse().multVec(intersect),self.Object.Nodes[1]]
                            else:
                                self.Object.Nodes = [self.Object.Nodes[0],self.Object.Placement.inverse().multVec(intersect)]

    def connectNodes(self,other=None):
        if not other:
            self.observer = StructSelectionObserver(self.connectNodes)
            FreeCADGui.Selection.addObserver(self.observer)
            FreeCAD.Console.PrintMessage(translate("Arch","Pick another Structure object: "))
        else:
            FreeCADGui.Selection.removeObserver(self.observer)
            self.observer = None
            if Draft.getType(other) != "Structure":
                FreeCAD.Console.PrintError(translate("Arch","The picked object is not a Structure\n"))
            else:
                if not other.Nodes:
                    FreeCAD.Console.PrintError(translate("Arch","The picked object has no structural nodes\n"))
                else:
                    if (len(self.Object.Nodes) != 2) or (len(other.Nodes) != 2):
                        FreeCAD.Console.PrintError(translate("Arch","One of these objects has more than 2 nodes\n"))
                    else:
                        import DraftGeomUtils
                        nodes1 = [self.Object.Placement.multVec(v) for v in self.Object.Nodes]
                        nodes2 = [other.Placement.multVec(v) for v in other.Nodes]
                        intersect = DraftGeomUtils.findIntersection(nodes1[0],nodes1[1],nodes2[0],nodes2[1],True,True)
                        if not intersect:
                            FreeCAD.Console.PrintError(translate("Arch","Unable to find a suitable intersection point\n"))
                        else:
                            intersect = intersect[0]
                            FreeCAD.Console.PrintMessage(translate("Arch","Intersection found.\n"))
                            if DraftGeomUtils.findClosest(intersect,nodes1) == 0:
                                self.Object.Nodes = [self.Object.Placement.inverse().multVec(intersect),self.Object.Nodes[1]]
                            else:
                                self.Object.Nodes = [self.Object.Nodes[0],self.Object.Placement.inverse().multVec(intersect)]
                            if DraftGeomUtils.findClosest(intersect,nodes2) == 0:
                                other.Nodes = [other.Placement.inverse().multVec(intersect),other.Nodes[1]]
                            else:
                                other.Nodes = [other.Nodes[0],other.Placement.inverse().multVec(intersect)]

    def toggleNodes(self):
        if self.nodevis:
            for obj in self.nodevis:
                obj[0].ViewObject.ShowNodes = obj[1]
            self.nodevis = None
        else:
            self.nodevis = []
            for obj in FreeCAD.ActiveDocument.Objects:
                if hasattr(obj.ViewObject,"ShowNodes"):
                    self.nodevis.append([obj,obj.ViewObject.ShowNodes])
                    obj.ViewObject.ShowNodes = True

    def accept(self):
        if self.observer:
            FreeCADGui.Selection.removeObserver(self.observer)
        if self.nodevis:
            self.toggleNodes()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True


class StructSelectionObserver:

    def __init__(self,callback):
        self.callback = callback

    def addSelection(self, docName, objName, sub, pos):
        print "got ",objName
        obj = FreeCAD.getDocument(docName).getObject(objName)
        self.callback(obj)


class _StructuralSystem(ArchComponent.Component):
    "The Structural System object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLinkList","Axes","Arch",QT_TRANSLATE_NOOP("App::Property","Axes systems this structure is built on"))
        obj.addProperty("App::PropertyIntegerList","Exclude","Arch",QT_TRANSLATE_NOOP("App::Property","The element numbers to exclude when this structure is based on axes"))
        obj.addProperty("App::PropertyBool","Align","Arch",QT_TRANSLATE_NOOP("App::Property","If true the element are aligned with axes")).Align = False
        self.Type = "StructuralSystem"

    def execute(self,obj):
        "creates the structure shape"

        import Part, DraftGeomUtils

        # creating base shape
        pl = obj.Placement
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.isNull():
                    return
                if not obj.Base.Shape.Solids:
                    return

                base = None

                # applying axes
                pts = self.getAxisPoints(obj)
                if hasattr(obj,"Align"):
                    if obj.Align == False :
                        apl = self.getAxisPlacement(obj)
                    if obj.Align == True :
                        apl = None
                else :
                    apl = self.getAxisPlacement(obj)

                if pts:
                    fsh = []
                    for i in range(len(pts)):
                        sh = obj.Base.Shape.copy()
                        if hasattr(obj,"Exclude"):
                            if i in obj.Exclude:
                                continue
                        if apl:
                            sh.Placement.Rotation = sh.Placement.Rotation.multiply(apl.Rotation)
                        sh.translate(pts[i])
                        fsh.append(sh)

                    if fsh:
                        base = Part.makeCompound(fsh)
                        base = self.processSubShapes(obj,base,pl)

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

    def getAxisPoints(self,obj):
        "returns the gridpoints of linked axes"
        import DraftGeomUtils
        pts = []
        if len(obj.Axes) == 1:
            if hasattr(obj,"Align"):
                if obj.Align == True :
                    p0 = obj.Axes[0].Shape.Edges[0].Vertexes[1].Point
                    for e in obj.Axes[0].Shape.Edges:
                        p = e.Vertexes[1].Point
                        p = p.sub(p0)
                        pts.append(p)
                else:
                    for e in obj.Axes[0].Shape.Edges:
                        pts.append(e.Vertexes[0].Point)
            else:
                for e in obj.Axes[0].Shape.Edges:
                        pts.append(e.Vertexes[0].Point)
        elif len(obj.Axes) >= 2:
            set1 = obj.Axes[0].Shape.Edges
            set2 = obj.Axes[1].Shape.Edges
            for e1 in set1:
                for e2 in set2:
                    pts.extend(DraftGeomUtils.findIntersection(e1,e2))
        return pts

    def getAxisPlacement(self,obj):
        "returns an axis placement"
        if obj.Axes:
            return obj.Axes[0].Placement
        return None


class _ViewProviderStructuralSystem(ArchComponent.ViewProviderComponent):
    "A View Provider for the Structural System object"

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_StructuralSystem_Tree.svg"


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Structure',_CommandStructure())
