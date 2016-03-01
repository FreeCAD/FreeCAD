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
else:
    def translate(ctxt,txt):
        return txt

__title__="FreeCAD Structure"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

# Make some strings picked by the translator
#if FreeCAD.GuiUp:
#    QtCore.QT_TRANSLATE_NOOP("Arch","Wood")
#    QtCore.QT_TRANSLATE_NOOP("Arch","Steel")

# Possible roles for structural elements
Roles = ["Beam","Column","Slab","Wall","Curtain Wall","Roof","Foundation","Pile","Tendon"]

#Reads preset profiles and categorizes them
Categories=[None]
Presets=ArchProfile.readPresets()
for pre in Presets[1:]:
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
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Structure","Structure"),
                'Accel': "S, T",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Structure","Creates a structure object from scratch or from a selected object (sketch, wire, face or solid)")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        self.Length = p.GetFloat("StructureLength",100)
        self.Width = p.GetFloat("StructureWidth",100)
        self.Height = p.GetFloat("StructureHeight",1000)
        self.Profile = None
        self.continueCmd = False
        self.DECIMALS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt("Decimals",2)
        import DraftGui
        self.FORMAT = DraftGui.makeFormatSpec(self.DECIMALS,'Length')
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
        import DraftTrackers
        self.points = []
        self.tracker = DraftTrackers.boxTracker()
        self.tracker.width(self.Width)
        self.tracker.height(self.Height)
        self.tracker.length(self.Length)
        self.tracker.on()
        FreeCADGui.Snapper.getPoint(callback=self.getPoint,movecallback=self.update,extradlg=self.taskbox())

    def getPoint(self,point=None,obj=None):
        "this function is called by the snapper when it has a 3D point"
        self.tracker.finalize()
        if point == None:
            return
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Structure")))
        FreeCADGui.addModule("Arch")
        if self.Profile is not None:
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
        FreeCADGui.doCommand('s.Placement.Rotation=FreeCAD.DraftWorkingPlane.getRotation().Rotation')
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
        valuec.addItems([" "]+Categories[1:])
        grid.addWidget(labelc,0,0,1,1)
        grid.addWidget(valuec,0,1,1,1)

        # presets box
        labelp = QtGui.QLabel(translate("Arch","Preset").decode("utf8"))
        self.vPresets = QtGui.QComboBox()
        self.pSelect=Presets[1:]
        fpresets = [" "]+self._createItemlist(self.pSelect)
        self.vPresets.addItems(fpresets)
        grid.addWidget(labelp,1,0,1,1)
        grid.addWidget(self.vPresets,1,1,1,1)

        # length
        label1 = QtGui.QLabel(translate("Arch","Length").decode("utf8"))
        self.vLength = ui.createWidget("Gui::InputField")
        self.vLength.setText(self.FORMAT % self.Length)
        grid.addWidget(label1,2,0,1,1)
        grid.addWidget(self.vLength,2,1,1,1)

        # width
        label2 = QtGui.QLabel(translate("Arch","Width").decode("utf8"))
        self.vWidth = ui.createWidget("Gui::InputField")
        self.vWidth.setText(self.FORMAT % self.Width)
        grid.addWidget(label2,3,0,1,1)
        grid.addWidget(self.vWidth,3,1,1,1)

        # height
        label3 = QtGui.QLabel(translate("Arch","Height").decode("utf8"))
        self.vHeight = ui.createWidget("Gui::InputField")
        self.vHeight.setText(self.FORMAT % self.Height)
        grid.addWidget(label3,4,0,1,1)
        grid.addWidget(self.vHeight,4,1,1,1)

        # horizontal button
        value5 = QtGui.QPushButton(translate("Arch","Rotate").decode("utf8"))
        grid.addWidget(value5,5,0,1,2)

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
        QtCore.QObject.connect(value5,QtCore.SIGNAL("pressed()"),self.rotate)
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
        if i > 0:
            self.pSelect= [p for p in Presets[1:] if p[1] == Categories[i]]
            fpresets = [" "]+self._createItemlist(self.pSelect)
            self.vPresets.addItems(fpresets)
        else:
            self.pSelect=Presets[1:]
            fpresets = [" "]+self._createItemlist(self.pSelect)
            self.vPresets.addItems(fpresets)

    def setPreset(self,i):
        if i > 0:
            p=self.pSelect[i-1][0]
            self.vLength.setText(self.FORMAT % float(Presets[p][4]))
            self.vWidth.setText(self.FORMAT % float(Presets[p][5]))
            self.Profile = Presets[p]
        else:
            self.Profile = None

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
        obj.addProperty("App::PropertyLink","Tool","Arch",translate("Arch","An optional extrusion path for this element"))
        obj.addProperty("App::PropertyLength","Length","Arch",translate("Arch","The length of this element, if not based on a profile"))
        obj.addProperty("App::PropertyLength","Width","Arch",translate("Arch","The width of this element, if not based on a profile"))
        obj.addProperty("App::PropertyLength","Height","Arch",translate("Arch","The height or extrusion depth of this element. Keep 0 for automatic"))
        obj.addProperty("App::PropertyLinkList","Armatures","Arch",translate("Arch","Armatures contained in this element"))
        obj.addProperty("App::PropertyVector","Normal","Arch",translate("Arch","The normal extrusion direction of this object (keep (0,0,0) for automatic normal)"))
        obj.addProperty("App::PropertyVectorList","Nodes","Arch",translate("Arch","The structural nodes of this element"))
        obj.addProperty("App::PropertyString","Profile","Arch",translate("Arch","A description of the standard profile this element is based upon"))
        self.Type = "Structure"
        obj.Role = Roles

    def execute(self,obj):
        "creates the structure shape"

        import Part, DraftGeomUtils
        
        if self.clone(obj):
            return

        normal,length,width,height = self.getDefaultValues(obj)

        # creating base shape
        pl = obj.Placement
        base = None
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.isNull():
                    return
                if not obj.Base.Shape.isValid():
                    if not obj.Base.Shape.Solids:
                        # let pass invalid objects if they have solids...
                        return
                if hasattr(obj,"Tool"):
                    if obj.Tool:
                        try:
                            base = obj.Tool.Shape.copy().makePipe(obj.Base.Shape.copy())
                        except Part.OCCError:
                            FreeCAD.Console.PrintError(translate("Arch","Error: The base shape couldn't be extruded along this tool object"))
                            return
                if not base:
                    if not height:
                        return
                    if obj.Normal == Vector(0,0,0):
                        if len(obj.Base.Shape.Faces) > 0 :
                            normal=obj.Base.Shape.Faces[0].normalAt(.5,.5)
                        else:
                            normal = DraftGeomUtils.getNormal(obj.Base.Shape)
                            if not normal:
                                normal = FreeCAD.Vector(0,0,1)
                            #p = FreeCAD.Placement(obj.Base.Placement)
                            #normal = p.Rotation.multVec(normal)
                    else:
                        normal = Vector(obj.Normal)
                    normal = normal.multiply(height)
                    base = obj.Base.Shape.copy()
                    if base.Solids:
                        pass
                    elif base.Faces:
                        base = base.extrude(normal)
                    elif (len(base.Wires) == 1):
                        if base.Wires[0].isClosed():
                            try:
                                base = Part.Face(base.Wires[0])
                                base = base.extrude(normal)
                            except Part.OCCError:
                                FreeCAD.Console.PrintError(obj.Label+" : "+str(translate("Arch","Unable to extrude the base shape\n")))
                                return

            elif obj.Base.isDerivedFrom("Mesh::Feature"):
                if obj.Base.Mesh.isSolid():
                    if obj.Base.Mesh.countComponents() == 1:
                        sh = ArchCommands.getShapeFromMesh(obj.Base.Mesh)
                        if sh.isClosed() and sh.isValid() and sh.Solids and (not sh.isNull()):
                            base = sh
                        else:
                            FreeCAD.Console.PrintWarning(str(translate("Arch","This mesh is an invalid solid")))
                            obj.Base.ViewObject.show()
        else:
            base = self.getProfiles(obj)
            if base:
                if length > height:
                    normal = normal.multiply(length)
                else:
                    normal = normal.multiply(height)
                base = Part.Face(base[0])
                base = base.extrude(normal)

        base = self.processSubShapes(obj,base,pl)
        self.applyShape(obj,base,pl)

    def onChanged(self,obj,prop):
        self.hideSubobjects(obj,prop)
        if prop == "Shape":
            if hasattr(obj,"Nodes"):
                # update structural nodes
                if obj.Nodes:
                    if hasattr(self,"nodes"):
                        if self.nodes:
                            if obj.Nodes != self.nodes:
                                # nodes are set manually: don't touch them
                                return
                    else:
                        # nodes haven't been calculated yet, but are set (file load)
                        # we calculate the nodes now but don't change the property
                        axis = self.getAxis(obj)
                        if axis:
                            self.nodes = [v.Point for v in axis.Vertexes]
                            return
                # we calculate and set the nodes
                axis = self.getAxis(obj)
                if axis:
                    self.nodes = [v.Point for v in axis.Vertexes]
                    obj.Nodes = self.nodes


class _ViewProviderStructure(ArchComponent.ViewProviderComponent):
    "A View Provider for the Structure object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        vobj.addProperty("App::PropertyBool","ShowNodes","Arch","If the nodes are visible or not").ShowNodes = False
        vobj.addProperty("App::PropertyFloat","NodeLine","Base","The width of the nodes line")
        vobj.addProperty("App::PropertyFloat","NodeSize","Base","The size of the node points")
        vobj.addProperty("App::PropertyColor","NodeColor","Base","The color of the nodes line")
        vobj.NodeColor = (1.0,1.0,1.0,1.0)
        vobj.NodeSize = 6
        vobj.ShapeColor = ArchCommands.getDefaultColor("Structure")

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Structure_Tree.svg"

    def updateData(self,obj,prop):
        if prop == "Nodes":
            if obj.Nodes:
                if hasattr(self,"nodes"):
                    p = []
                    for n in obj.Nodes:
                        p.append([n.x,n.y,n.z])
                    self.coords.point.setValues(0,len(p),p)
                    self.pointset.numPoints.setValue(len(p))
                    self.lineset.coordIndex.setValues(0,len(p)+1,range(len(p))+[-1])
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
                self.nodes.addChild(self.coords)
                self.nodes.addChild(self.mat)
                self.nodes.addChild(self.pointstyle)
                self.nodes.addChild(self.pointset)
                self.nodes.addChild(self.linestyle)
                self.nodes.addChild(self.lineset)
                vobj.Annotation.addChild(self.nodes)
                self.updateData(vobj.Object,"Nodes")
                self.onChanged(vobj,"NodeColor")
                self.onChanged(vobj,"NodeLine")
                self.onChanged(vobj,"NodeSize")
        elif prop == "NodeColor":
            if hasattr(self,"mat"):
                l = vobj.NodeColor
                self.mat.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "NodeLine":
            if hasattr(self,"linestyle"):
                self.linestyle.lineWidth = vobj.NodeLine
        elif prop == "NodeSize":
            if hasattr(self,"pointstyle"):
                self.pointstyle.pointSize = vobj.NodeSize
        ArchComponent.ViewProviderComponent.onChanged(self,vobj,prop)


class _StructuralSystem(ArchComponent.Component):
    "The Structural System object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLinkList","Axes","Arch",translate("Arch","Axes systems this structure is built on"))
        obj.addProperty("App::PropertyIntegerList","Exclude","Arch",translate("Arch","The element numbers to exclude when this structure is based on axes"))
        obj.addProperty("App::PropertyBool","Align","Arch","If true the element are aligned with axes").Align = False 
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
