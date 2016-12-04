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

import FreeCAD,Draft,ArchComponent,DraftVecUtils,ArchCommands,math
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond
    
## @package ArchPanel
#  \ingroup ARCH
#  \brief The Panel object and tools
#
#  This module provides tools to build Panel objects.
#  Panels consist of a closed shape that gets extruded to
#  produce a flat object.


__title__="FreeCAD Panel"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

#           Description                 l    w    t

Presets = [None,
           ["Plywood 12mm, 1220 x 2440",1200,2400,12],
           ["Plywood 15mm, 1220 x 2440",1200,2400,15],
           ["Plywood 18mm, 1220 x 2440",1200,2400,18],
           ["Plywood 25mm, 1220 x 2440",1200,2400,25],
           ["MDF 3mm, 900 x 600",       900, 600, 3],
           ["MDF 6mm, 900 x 600",       900, 600, 6]]

def makePanel(baseobj=None,length=0,width=0,thickness=0,placement=None,name="Panel"):
    '''makePanel([obj],[length],[width],[thickness],[placement]): creates a
    panel element based on the given profile object and the given
    extrusion thickness. If no base object is given, you can also specify
    length and width for a simple cubic object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    obj.Label = translate("Arch",name)
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
    return obj


def makePanelView(panel,page=None,name="PanelView"):
    """makePanelView(panel,[page]) : Creates a Drawing view of the given panel
    in the given or active Page object (a new page will be created if none exists)."""
    if not page:
        for o in FreeCAD.ActiveDocument.Objects:
            if o.isDerivedFrom("Drawing::FeaturePage"):
                page = o
                break
        if not page:
            page = FreeCAD.ActiveDocument.addObject("Drawing::FeaturePage",translate("Arch","Page"))
            page.Template = Draft.getParam("template",FreeCAD.getResourceDir()+'Mod/Drawing/Templates/A3_Landscape.svg')
    view = FreeCAD.ActiveDocument.addObject("Drawing::FeatureViewPython",name)
    page.addObject(view)
    _PanelView(view)
    view.Source = panel
    view.Label = translate("Arch","View of")+" "+panel.Name
    return view


class _CommandPanel:
    "the Arch Panel command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Panel',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Panel","Panel"),
                'Accel': "P, A",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Panel","Creates a panel object from scratch or from a selected object (sketch, wire, face or solid)")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        self.Length = p.GetFloat("PanelLength",1000)
        self.Width = p.GetFloat("PanelWidth",1000)
        self.Thickness = p.GetFloat("PanelThickness",10)
        self.Profile = None
        self.continueCmd = False
        self.rotated = False
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            if len(sel) == 1:
                if Draft.getType(sel[0]) == "Panel":
                    return
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Panel")))
            FreeCADGui.addModule("Arch")
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
        FreeCADGui.addModule("Arch")
        if self.Profile:
            pr = Presets[self.Profile]
            FreeCADGui.doCommand('p = Arch.makeProfile('+str(pr[2])+','+str(pr[3])+','+str(pr[4])+','+str(pr[5])+')')
            FreeCADGui.doCommand('s = Arch.makePanel(p,thickness='+str(self.Thickness)+')')
            #FreeCADGui.doCommand('s.Placement.Rotation = FreeCAD.Rotation(-0.5,0.5,-0.5,0.5)')
        else:
            FreeCADGui.doCommand('s = Arch.makePanel(length='+str(self.Length)+',width='+str(self.Width)+',thickness='+str(self.Thickness)+')')
        FreeCADGui.doCommand('s.Placement.Base = '+DraftVecUtils.toString(point))
        if self.rotated:
            FreeCADGui.doCommand('s.Placement.Rotation = FreeCAD.Rotation(FreeCAD.Vector(1.00,0.00,0.00),90.00)')
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
            fpresets.append(translate("Arch",p[0]))
        valuep.addItems(fpresets)
        grid.addWidget(labelp,0,0,1,1)
        grid.addWidget(valuep,0,1,1,1)

        # length
        label1 = QtGui.QLabel(translate("Arch","Length").decode("utf8"))
        self.vLength = ui.createWidget("Gui::InputField")
        self.vLength.setText(FreeCAD.Units.Quantity(self.Length,FreeCAD.Units.Length).UserString)
        grid.addWidget(label1,1,0,1,1)
        grid.addWidget(self.vLength,1,1,1,1)

        # width
        label2 = QtGui.QLabel(translate("Arch","Width").decode("utf8"))
        self.vWidth = ui.createWidget("Gui::InputField")
        self.vWidth.setText(FreeCAD.Units.Quantity(self.Width,FreeCAD.Units.Length).UserString)
        grid.addWidget(label2,2,0,1,1)
        grid.addWidget(self.vWidth,2,1,1,1)

        # height
        label3 = QtGui.QLabel(translate("Arch","Thickness").decode("utf8"))
        self.vHeight = ui.createWidget("Gui::InputField")
        self.vHeight.setText(FreeCAD.Units.Quantity(self.Thickness,FreeCAD.Units.Length).UserString)
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
            self.tracker.pos(point)
            if self.rotated:
                self.tracker.width(self.Thickness)
                self.tracker.height(self.Width)
                self.tracker.length(self.Length)
            else:
                self.tracker.width(self.Width)
                self.tracker.height(self.Thickness)
                self.tracker.length(self.Length)

    def setWidth(self,d):
        self.Width = d

    def setThickness(self,d):
        self.Thickness = d

    def setLength(self,d):
        self.Length = d

    def setContinue(self,i):
        self.continueCmd = bool(i)
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.continueMode = bool(i)

    def setPreset(self,i):
        if i > 0:
            self.vLength.setText(FreeCAD.Units.Quantity(float(Presets[i][1]),FreeCAD.Units.Length).UserString)
            self.vWidth.setText(FreeCAD.Units.Quantity(float(Presets[i][2]),FreeCAD.Units.Length).UserString)
            self.vHeight.setText(FreeCAD.Units.Quantity(float(Presets[i][3]),FreeCAD.Units.Length).UserString)

    def rotate(self):
        self.rotated = not self.rotated


class _Panel(ArchComponent.Component):
    "The Panel object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLength","Length","Arch",   QT_TRANSLATE_NOOP("App::Property","The length of this element, if not based on a profile"))
        obj.addProperty("App::PropertyLength","Width","Arch",    QT_TRANSLATE_NOOP("App::Property","The width of this element, if not based on a profile"))
        obj.addProperty("App::PropertyLength","Thickness","Arch",QT_TRANSLATE_NOOP("App::Property","The thickness or extrusion depth of this element"))
        obj.addProperty("App::PropertyInteger","Sheets","Arch",  QT_TRANSLATE_NOOP("App::Property","The number of sheets to use"))
        obj.addProperty("App::PropertyDistance","Offset","Arch",   QT_TRANSLATE_NOOP("App::Property","The offset between this panel and its baseline"))
        obj.addProperty("App::PropertyLength","WaveLength","Arch", QT_TRANSLATE_NOOP("App::Property","The length of waves for corrugated elements"))
        obj.addProperty("App::PropertyLength","WaveHeight","Arch", QT_TRANSLATE_NOOP("App::Property","The height of waves for corrugated elements"))
        obj.addProperty("App::PropertyAngle","WaveDirection","Arch", QT_TRANSLATE_NOOP("App::Property","The direction of waves for corrugated elements"))
        obj.addProperty("App::PropertyEnumeration","WaveType","Arch", QT_TRANSLATE_NOOP("App::Property","The type of waves for corrugated elements"))
        obj.addProperty("App::PropertyArea","Area","Arch",       QT_TRANSLATE_NOOP("App::Property","The area of this panel"))
        obj.addProperty("App::PropertyEnumeration","FaceMaker","Arch",QT_TRANSLATE_NOOP("App::Property","The facemaker type to use to build the profile of this object"))
        obj.addProperty("App::PropertyVector","Normal","Arch",QT_TRANSLATE_NOOP("App::Property","The normal extrusion direction of this object (keep (0,0,0) for automatic normal)"))
        obj.Sheets = 1
        self.Type = "Panel"
        obj.WaveType = ["Curved","Trapezoidal"]
        obj.FaceMaker = ["None","Simple","Cheese","Bullseye"]
        obj.setEditorMode("VerticalArea",2)
        obj.setEditorMode("HorizontalArea",2)

    def execute(self,obj):
        "creates the panel shape"
        
        if self.clone(obj):
            return

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
                if not obj.Base.Shape.Solids:
                    return

        # creating base shape
        pl = obj.Placement
        base = None
        normal = None
        if hasattr(obj,"Normal"):
            if obj.Normal.Length > 0:
                normal = Vector(obj.Normal)
                normal.normalize()
                normal.multiply(thickness)
        baseprofile = None
        if obj.Base:
            base = obj.Base.Shape.copy()
            if not base.Solids:
                p = FreeCAD.Placement(obj.Base.Placement)
                if base.Faces:
                    baseprofile = base
                    if not normal:
                        normal = baseprofile.Faces[0].normalAt(0,0).multiply(thickness)
                    base = base.extrude(normal)
                elif base.Wires:
                    fm = False
                    if hasattr(obj,"FaceMaker"):
                        if obj.FaceMaker != "None":
                            try:
                                base = Part.makeFace(base.Wires,"Part::FaceMaker"+str(obj.FaceMaker))
                                fm = True
                            except:
                                FreeCAD.Console.PrintError(translate("Arch","Facemaker returned an error")+"\n")
                                return
                    if not fm:
                        closed = True
                        for w in base.Wires:
                            if not w.isClosed():
                                closed = False
                        if closed:
                            baseprofile = ArchCommands.makeFace(base.Wires)
                            if not normal:
                                normal = baseprofile.normalAt(0,0).multiply(thickness)
                            base = baseprofile.extrude(normal)
                elif obj.Base.isDerivedFrom("Mesh::Feature"):
                    if obj.Base.Mesh.isSolid():
                        if obj.Base.Mesh.countComponents() == 1:
                            sh = ArchCommands.getShapeFromMesh(obj.Base.Mesh)
                            if sh.isClosed() and sh.isValid() and sh.Solids:
                                base = sh
        else:
            if not normal:
                normal = Vector(0,0,1).multiply(thickness)
            l2 = length/2 or 0.5
            w2 = width/2 or 0.5
            v1 = Vector(-l2,-w2,0)
            v2 = Vector(l2,-w2,0)
            v3 = Vector(l2,w2,0)
            v4 = Vector(-l2,w2,0)
            base = Part.makePolygon([v1,v2,v3,v4,v1])
            baseprofile = Part.Face(base)
            base = baseprofile.extrude(normal)
            
        if hasattr(obj,"Area"):
            if baseprofile:
                obj.Area = baseprofile.Area
            
        if hasattr(obj,"WaveLength"):
            if baseprofile and obj.WaveLength.Value and obj.WaveHeight.Value:
                # corrugated element
                bb = baseprofile.BoundBox
                bb.enlarge(bb.DiagonalLength)
                p1 = Vector(bb.getPoint(0).x,bb.getPoint(0).y,bb.Center.z)
                if obj.WaveType == "Curved":
                    p2 = p1.add(Vector(obj.WaveLength.Value/2,0,obj.WaveHeight.Value))
                    p3 = p2.add(Vector(obj.WaveLength.Value/2,0,-obj.WaveHeight.Value))
                    e1 = Part.Arc(p1,p2,p3).toShape()
                    p4 = p3.add(Vector(obj.WaveLength.Value/2,0,-obj.WaveHeight.Value))
                    p5 = p4.add(Vector(obj.WaveLength.Value/2,0,obj.WaveHeight.Value))
                    e2 = Part.Arc(p3,p4,p5).toShape()
                else:
                    if obj.WaveHeight.Value < obj.WaveLength.Value:
                        p2 = p1.add(Vector(obj.WaveHeight.Value,0,obj.WaveHeight.Value))
                        p3 = p2.add(Vector(obj.WaveLength.Value-2*obj.WaveHeight.Value,0,0))
                        p4 = p3.add(Vector(obj.WaveHeight.Value,0,-obj.WaveHeight.Value))
                        e1 = Part.makePolygon([p1,p2,p3,p4])
                        p5 = p4.add(Vector(obj.WaveHeight.Value,0,-obj.WaveHeight.Value))
                        p6 = p5.add(Vector(obj.WaveLength.Value-2*obj.WaveHeight.Value,0,0))
                        p7 = p6.add(Vector(obj.WaveHeight.Value,0,obj.WaveHeight.Value))
                        e2 = Part.makePolygon([p4,p5,p6,p7])
                    else:
                        p2 = p1.add(Vector(obj.WaveLength.Value/2,0,obj.WaveHeight.Value))
                        p3 = p2.add(Vector(obj.WaveLength.Value/2,0,-obj.WaveHeight.Value))
                        e1 = Part.makePolygon([p1,p2,p3])
                        p4 = p3.add(Vector(obj.WaveLength.Value/2,0,-obj.WaveHeight.Value))
                        p5 = p4.add(Vector(obj.WaveLength.Value/2,0,obj.WaveHeight.Value))
                        e2 = Part.makePolygon([p3,p4,p5])
                edges = [e1,e2]
                for i in range(int(bb.XLength/(obj.WaveLength.Value*2))):
                    e1 = e1.copy()
                    e1.translate(Vector(obj.WaveLength.Value*2,0,0))
                    e2 = e2.copy()
                    e2.translate(Vector(obj.WaveLength.Value*2,0,0))
                    edges.extend([e1,e2])
                basewire = Part.Wire(edges)
                baseface = basewire.extrude(Vector(0,bb.YLength,0))
                base = baseface.extrude(Vector(0,0,thickness))
                rot = FreeCAD.Rotation(FreeCAD.Vector(0,0,1),normal)
                base.rotate(bb.Center,rot.Axis,math.degrees(rot.Angle))
                if obj.WaveDirection.Value:
                    base.rotate(bb.Center,normal,obj.WaveDirection.Value)
                n1 = normal.negative().normalize().multiply(obj.WaveHeight.Value*2)
                self.vol = baseprofile.copy()
                self.vol.translate(n1)
                self.vol = self.vol.extrude(n1.negative().multiply(2))
                base = self.vol.common(base)
                base = base.removeSplitter()
                if not base:
                    FreeCAD.Console.PrintError(transpate("Arch","Error computing shape of ")+obj.Label+"\n")
                    return False

        if base and (obj.Sheets > 1) and normal and thickness:
            bases = [base]
            for i in range(1,obj.Sheets):
                n = FreeCAD.Vector(normal).normalize().multiply(i*thickness)
                b = base.copy()
                b.translate(n)
                bases.append(b)
            base = Part.makeCompound(bases)

        if base and normal and hasattr(obj,"Offset"):
            if obj.Offset.Value:
                v = DraftVecUtils.scaleTo(normal,obj.Offset.Value)
                base.translate(v)

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
        vobj.ShapeColor = ArchCommands.getDefaultColor("Panel")

    def getIcon(self):
        import Arch_rc
        if hasattr(self,"Object"):
            if hasattr(self.Object,"CloneOf"):
                if self.Object.CloneOf:
                    return ":/icons/Arch_Panel_Clone.svg"
        return ":/icons/Arch_Panel_Tree.svg"


class _PanelView:
    "A Drawing view for Arch Panels"

    def __init__(self, obj):
        obj.addProperty("App::PropertyLink","Source","Base",QT_TRANSLATE_NOOP("App::Property","The linked object"))
        obj.addProperty("App::PropertyFloat","LineWidth","Drawing view",QT_TRANSLATE_NOOP("App::Property","The line width of the rendered objects"))
        obj.addProperty("App::PropertyColor","LineColor","Drawing view",QT_TRANSLATE_NOOP("App::Property","The color of the panel outline"))
        obj.addProperty("App::PropertyLength","FontSize","Tag view",QT_TRANSLATE_NOOP("App::Property","The size of the tag text"))
        obj.addProperty("App::PropertyColor","TextColor","Tag view",QT_TRANSLATE_NOOP("App::Property","The color of the tag text"))
        obj.addProperty("App::PropertyFloat","TextX","Tag view",QT_TRANSLATE_NOOP("App::Property","The X offset of the tag text"))
        obj.addProperty("App::PropertyFloat","TextY","Tag view",QT_TRANSLATE_NOOP("App::Property","The Y offset of the tag text"))
        obj.addProperty("App::PropertyString","FontName","Tag view",QT_TRANSLATE_NOOP("App::Property","The font of the tag text"))
        obj.Proxy = self
        self.Type = "PanelView"
        obj.LineWidth = 0.35
        obj.LineColor = (0.0,0.0,0.0)
        obj.TextColor = (0.0,0.0,1.0)
        obj.X = 10
        obj.Y = 10
        obj.TextX = 10
        obj.TextY = 10
        obj.FontName = "sans"

    def execute(self, obj):
        if obj.Source:
            if hasattr(obj.Source.Proxy,"BaseProfile"):
                p = obj.Source.Proxy.BaseProfile
                n = obj.Source.Proxy.ExtrusionVector
                import Drawing
                svg1 = ""
                svg2 = ""
                result = ""
                svg1 = Drawing.projectToSVG(p,DraftVecUtils.neg(n))
                if svg1:
                    w = str(obj.LineWidth/obj.Scale) #don't let linewidth be influenced by the scale...
                    svg1 = svg1.replace('stroke-width="0.35"','stroke-width="'+w+'"')
                    svg1 = svg1.replace('stroke-width="1"','stroke-width="'+w+'"')
                    svg1 = svg1.replace('stroke-width:0.01','stroke-width:'+w)
                    svg1 = svg1.replace('scale(1,-1)','scale('+str(obj.Scale)+',-'+str(obj.Scale)+')')
                if obj.Source.Tag:
                    svg2 = '<text id="tag'+obj.Name+'"'
                    svg2 += ' fill="'+Draft.getrgb(obj.TextColor)+'"'
                    svg2 += ' font-size="'+str(obj.FontSize)+'"'
                    svg2 += ' style="text-anchor:start;text-align:left;'
                    svg2 += ' font-family:'+obj.FontName+'" '
                    svg2 += ' transform="translate(' + str(obj.TextX) + ',' + str(obj.TextY) + ')">'
                    svg2 += '<tspan>'+obj.Source.Tag+'</tspan></text>\n'
                result += '<g id="' + obj.Name + '"'
                result += ' transform="'
                result += 'rotate('+str(obj.Rotation)+','+str(obj.X)+','+str(obj.Y)+') '
                result += 'translate('+str(obj.X)+','+str(obj.Y)+')'
                result += '">\n  '
                result += svg1
                result += svg2
                result += '</g>'
                obj.ViewResult = result

    def onChanged(self, obj, prop):
        pass

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state

    def getDisplayModes(self,vobj):
        modes=["Default"]
        return modes

    def setDisplayMode(self,mode):
        return mode

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Panel',_CommandPanel())
