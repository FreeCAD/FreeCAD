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

import FreeCAD,WorkingPlane,math,Draft,ArchCommands,DraftVecUtils,ArchComponent
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from pivy import coin
else:
    def translate(ctxt,txt):
        return txt

def makeSectionPlane(objectslist=None,name="Section"):
    """makeSectionPlane([objectslist]) : Creates a Section plane objects including the
    given objects. If no object is given, the whole document will be considered."""
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython",name)
    obj.Label = translate("Arch",name)
    _SectionPlane(obj)
    if FreeCAD.GuiUp:
        _ViewProviderSectionPlane(obj.ViewObject)
    if objectslist:
        g = []
        for o in objectslist:
            if o.isDerivedFrom("Part::Feature"):
                g.append(o)
            elif o.isDerivedFrom("App::DocumentObjectGroup"):
                g.append(o)
        obj.Objects = g
    return obj

def makeSectionView(section,name="View"):
    """makeSectionView(section) : Creates a Drawing view of the given Section Plane
    in the active Page object (a new page will be created if none exists"""
    page = None
    for o in FreeCAD.ActiveDocument.Objects:
        if o.isDerivedFrom("Drawing::FeaturePage"):
            page = o
            break
    if not page:
        page = FreeCAD.ActiveDocument.addObject("Drawing::FeaturePage",translate("Arch","Page"))
        page.Template = Draft.getParam("template",FreeCAD.getResourceDir()+'Mod/Drawing/Templates/A3_Landscape.svg')

    view = FreeCAD.ActiveDocument.addObject("Drawing::FeatureViewPython",name)
    page.addObject(view)
    _ArchDrawingView(view)
    view.Source = section
    view.Label = translate("Arch","View of")+" "+section.Name
    return view

class _CommandSectionPlane:
    "the Arch SectionPlane command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_SectionPlane',
                'Accel': "S, E",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_SectionPlane","Section Plane"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_SectionPlane","Creates a section plane object, including the selected objects")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        ss = "["
        for o in sel:
            if len(ss) > 1:
                ss += ","
            ss += "FreeCAD.ActiveDocument."+o.Name
        ss += "]"
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Section Plane"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("section = Arch.makeSectionPlane("+ss+")")
        FreeCADGui.doCommand("section.Placement = FreeCAD.DraftWorkingPlane.getPlacement()")
        #FreeCADGui.doCommand("Arch.makeSectionView(section)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

class _SectionPlane:
    "A section plane object"
    def __init__(self,obj):
        obj.Proxy = self
        obj.addProperty("App::PropertyPlacement","Placement","Base",translate("Arch","The placement of this object"))
        obj.addProperty("Part::PropertyPartShape","Shape","Base","")
        obj.addProperty("App::PropertyLinkList","Objects","Arch",translate("Arch","The objects that must be considered by this section plane. Empty means all document"))
        obj.addProperty("App::PropertyBool","OnlySolids","Arch",translate("Arch","If false, non-solids will be cut too, with possible wrong results."))
        obj.OnlySolids = True
        self.Type = "SectionPlane"

    def execute(self,obj):
        import Part
        if hasattr(obj.ViewObject,"DisplayLength"):
            l = obj.ViewObject.DisplayLength.Value
            h = obj.ViewObject.DisplayHeight.Value
        elif hasattr(obj.ViewObject,"DisplaySize"):
            # old objects
            l = obj.ViewObject.DisplaySize.Value
            h = obj.ViewObject.DisplaySize.Value
        else:
            l = 1
            h = 1
        p = Part.makePlane(l,l,Vector(l/2,-l/2,0),Vector(0,0,-1))
        # make sure the normal direction is pointing outwards, you never know what OCC will decide...
        if p.normalAt(0,0).getAngle(obj.Placement.Rotation.multVec(FreeCAD.Vector(0,0,1))) > 1:
            p.reverse()
        p.Placement = obj.Placement
        obj.Shape = p

    def onChanged(self,obj,prop):
        pass

    def getNormal(self,obj):
        return obj.Shape.Faces[0].normalAt(0,0)

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state

class _ViewProviderSectionPlane:
    "A View Provider for Section Planes"
    def __init__(self,vobj):
        vobj.addProperty("App::PropertyLength","DisplayLength","Arch",translate("Arch","The display length of this section plane"))
        vobj.addProperty("App::PropertyLength","DisplayHeight","Arch",translate("Arch","The display height of this section plane"))
        vobj.addProperty("App::PropertyLength","ArrowSize","Arch",translate("Arch","The size of the arrows of this section plane"))
        vobj.addProperty("App::PropertyPercent","Transparency","Base","")
        vobj.addProperty("App::PropertyFloat","LineWidth","Base","")
        vobj.addProperty("App::PropertyColor","LineColor","Base","")
        vobj.addProperty("App::PropertyBool","CutView","Arch",translate("Arch","Show the cut in the 3D view"))
        vobj.DisplayLength = 1000
        vobj.DisplayHeight = 1000
        vobj.ArrowSize = 50
        vobj.Transparency = 85
        vobj.LineWidth = 1
        vobj.LineColor = (0.0,0.0,0.4,1.0)
        vobj.CutView = False
        vobj.Proxy = self
        self.Object = vobj.Object

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_SectionPlane_Tree.svg"

    def claimChildren(self):
        return []

    def attach(self,vobj):
        self.clip = None
        self.mat1 = coin.SoMaterial()
        self.mat2 = coin.SoMaterial()
        self.fcoords = coin.SoCoordinate3()
        #fs = coin.SoType.fromName("SoBrepFaceSet").createInstance() # this causes a FreeCAD freeze for me
        fs = coin.SoIndexedFaceSet()
        fs.coordIndex.setValues(0,7,[0,1,2,-1,0,2,3])
        self.drawstyle = coin.SoDrawStyle()
        self.drawstyle.style = coin.SoDrawStyle.LINES
        self.lcoords = coin.SoCoordinate3()
        ls = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        ls.coordIndex.setValues(0,57,[0,1,-1,2,3,4,5,-1,6,7,8,9,-1,10,11,-1,12,13,14,15,-1,16,17,18,19,-1,20,21,-1,22,23,24,25,-1,26,27,28,29,-1,30,31,-1,32,33,34,35,-1,36,37,38,39,-1,40,41,42,43,44])
        sep = coin.SoSeparator()
        psep = coin.SoSeparator()
        fsep = coin.SoSeparator()
        fsep.addChild(self.mat2)
        fsep.addChild(self.fcoords)
        fsep.addChild(fs)
        psep.addChild(self.mat1)
        psep.addChild(self.drawstyle)
        psep.addChild(self.lcoords)
        psep.addChild(ls)
        sep.addChild(fsep)
        sep.addChild(psep)
        vobj.addDisplayMode(sep,"Default")
        self.onChanged(vobj,"DisplayLength")
        self.onChanged(vobj,"LineColor")
        self.onChanged(vobj,"Transparency")
        self.onChanged(vobj,"CutView")

    def getDisplayModes(self,vobj):
        return ["Default"]

    def getDefaultDisplayMode(self):
        return "Default"

    def setDisplayMode(self,mode):
        return mode

    def updateData(self,obj,prop):
        if prop in ["Placement"]:
            self.onChanged(obj.ViewObject,"DisplayLength")
            self.onChanged(obj.ViewObject,"CutView")
        return

    def onChanged(self,vobj,prop):
        if prop == "LineColor":
            l = vobj.LineColor
            self.mat1.diffuseColor.setValue([l[0],l[1],l[2]])
            self.mat2.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "Transparency":
            if hasattr(vobj,"Transparency"):
                self.mat2.transparency.setValue(vobj.Transparency/100.0)
        elif prop in ["DisplayLength","DisplayHeight","ArrowSize"]:
            if hasattr(vobj,"DisplayLength"):
                ld = vobj.DisplayLength.Value/2
                hd = vobj.DisplayHeight.Value/2
            elif hasattr(vobj,"DisplaySize"):
                # old objects
                ld = vobj.DisplaySize.Value/2
                hd = vobj.DisplaySize.Value/2
            else:
                ld = 1
                hd = 1
            verts = []
            fverts = []
            for v in [[-ld,-hd],[ld,-hd],[ld,hd],[-ld,hd]]:
                if hasattr(vobj,"ArrowSize"):
                    l1 = vobj.ArrowSize.Value if vobj.ArrowSize.Value > 0 else 0.1
                else:
                    l1 = 0.1
                l2 = l1/3
                pl = FreeCAD.Placement(vobj.Object.Placement)
                p1 = pl.multVec(Vector(v[0],v[1],0))
                p2 = pl.multVec(Vector(v[0],v[1],-l1))
                p3 = pl.multVec(Vector(v[0]-l2,v[1],-l1+l2))
                p4 = pl.multVec(Vector(v[0]+l2,v[1],-l1+l2))
                p5 = pl.multVec(Vector(v[0],v[1]-l2,-l1+l2))
                p6 = pl.multVec(Vector(v[0],v[1]+l2,-l1+l2))
                verts.extend([[p1.x,p1.y,p1.z],[p2.x,p2.y,p2.z]])
                fverts.append([p1.x,p1.y,p1.z])
                verts.extend([[p2.x,p2.y,p2.z],[p3.x,p3.y,p3.z],[p4.x,p4.y,p4.z],[p2.x,p2.y,p2.z]])
                verts.extend([[p2.x,p2.y,p2.z],[p5.x,p5.y,p5.z],[p6.x,p6.y,p6.z],[p2.x,p2.y,p2.z]])
            verts.extend(fverts+[fverts[0]])
            self.lcoords.point.setValues(verts)
            self.fcoords.point.setValues(fverts)
        elif prop == "LineWidth":
            self.drawstyle.lineWidth = vobj.LineWidth
        elif prop == "CutView":
            if hasattr(vobj,"CutView") and FreeCADGui.ActiveDocument.ActiveView:
                sg = FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()
                if vobj.CutView:
                    if self.clip:
                        sg.removeChild(self.clip)
                        self.clip = None
                    for o in Draft.getGroupContents(vobj.Object.Objects,walls=True):
                        if hasattr(o.ViewObject,"Lighting"):
                            o.ViewObject.Lighting = "One side"
                    self.clip = coin.SoClipPlane()
                    self.clip.on.setValue(True)
                    norm = vobj.Object.Proxy.getNormal(vobj.Object)
                    mp = vobj.Object.Shape.CenterOfMass
                    mp = DraftVecUtils.project(mp,norm)
                    dist = mp.Length + 0.1 # to not clip exactly on the section object
                    norm = norm.negative()
                    plane = coin.SbPlane(coin.SbVec3f(norm.x,norm.y,norm.z),-dist)
                    self.clip.plane.setValue(plane)
                    sg.insertChild(self.clip,0)
                else:
                    if self.clip:
                        sg.removeChild(self.clip)
                        self.clip = None
        return

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None
        
    def setEdit(self,vobj,mode):
        taskd = SectionPlaneTaskPanel()
        taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        return False
        
    def doubleClicked(self,vobj):
        self.setEdit(vobj,None)

class _ArchDrawingView:
    def __init__(self, obj):
        obj.addProperty("App::PropertyLink","Source","Base","The linked object")
        obj.addProperty("App::PropertyEnumeration","RenderingMode","Drawing view","The rendering mode to use")
        obj.addProperty("App::PropertyBool","ShowCut","Drawing view","If cut geometry is shown or not")
        obj.addProperty("App::PropertyBool","ShowFill","Drawing view","If cut geometry is filled or not")
        obj.addProperty("App::PropertyFloat","LineWidth","Drawing view","The line width of the rendered objects")
        obj.addProperty("App::PropertyLength","FontSize","Drawing view","The size of the texts inside this object")
        obj.RenderingMode = ["Solid","Wireframe"]
        obj.RenderingMode = "Wireframe"
        obj.LineWidth = 0.35
        obj.ShowCut = False
        obj.Proxy = self
        self.Type = "ArchSectionView"
        obj.FontSize = 12

    def execute(self, obj):
        if hasattr(obj,"Source"):
            if obj.Source:
                if not hasattr(self,"svg"):
                    self.onChanged(obj,"Source")
                else:
                    if not self.svg:
                        self.onChanged(obj,"Source")
                if not hasattr(self,"svg"):
                    return ''
                if not hasattr(self,"direction"):
                    p = FreeCAD.Placement(obj.Source.Placement)
                    self.direction = p.Rotation.multVec(FreeCAD.Vector(0,0,1))
                linewidth = obj.LineWidth/obj.Scale
                st = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetFloat("CutLineThickness",2)
                da = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetString("archHiddenPattern","30,10")
                da =da.replace(" ","")
                svg = self.svg.replace('LWPlaceholder', str(linewidth) + 'px')
                svg = svg.replace('SWPlaceholder', str(linewidth*st) + 'px')
                svg = svg.replace('DAPlaceholder', str(da))
                if hasattr(self,"spaces"):
                    if self.spaces and round(self.direction.getAngle(FreeCAD.Vector(0,0,1)),Draft.precision()) in [0,round(math.pi,Draft.precision())]:
                        svg += '<g transform="scale(1,-1)">'
                        for s in self.spaces:
                            svg += Draft.getSVG(s,scale=obj.Scale,fontsize=obj.FontSize.Value,direction=self.direction)
                        svg += '</g>'
                result = ''
                result += '<g id="' + obj.Name + '"'
                result += ' transform="'
                result += 'rotate('+str(obj.Rotation)+','+str(obj.X)+','+str(obj.Y)+') '
                result += 'translate('+str(obj.X)+','+str(obj.Y)+') '
                result += 'scale('+str(obj.Scale)+','+str(obj.Scale)+')'
                result += '">\n'
                result += svg
                result += '</g>\n'
                # print "complete node:",result
                obj.ViewResult = result

    def onChanged(self, obj, prop):
        if prop in ["Source","RenderingMode","ShowCut"]:
            import Part, DraftGeomUtils
            if hasattr(obj,"Source"):
                if obj.Source:
                    if obj.Source.Objects:
                        objs = Draft.getGroupContents(obj.Source.Objects,walls=True,addgroups=True)
                        objs = Draft.removeHidden(objs)
                        # separate spaces
                        self.spaces = []
                        os = []
                        for o in objs:
                            if Draft.getType(o) == "Space":
                                self.spaces.append(o)
                            else:
                                os.append(o)
                        objs = os
                        self.svg = ''
                        fillpattern = '<pattern id="sectionfill" patternUnits="userSpaceOnUse" patternTransform="matrix(5,0,0,5,0,0)"'
                        fillpattern += ' x="0" y="0" width="10" height="10">'
                        fillpattern += '<g>'
                        fillpattern += '<rect width="10" height="10" style="stroke:none; fill:#ffffff" /><path style="stroke:#000000; stroke-width:1" d="M0,0 l10,10" /></g></pattern>'

                        # generating SVG
                        if obj.RenderingMode == "Solid":
                            # render using the Arch Vector Renderer
                            import ArchVRM, WorkingPlane
                            wp = WorkingPlane.plane()
                            wp.setFromPlacement(obj.Source.Placement)
                            wp.inverse()
                            render = ArchVRM.Renderer()
                            render.setWorkingPlane(wp)
                            render.addObjects(objs)
                            if hasattr(obj,"ShowCut"):
                                render.cut(obj.Source.Shape,obj.ShowCut)
                            else:
                                render.cut(obj.Source.Shape)
                            self.svg += render.getViewSVG(linewidth="LWPlaceholder")
                            self.svg += fillpattern
                            self.svg += render.getSectionSVG(linewidth="SWPlaceholder",fillpattern="sectionfill")
                            if hasattr(obj,"ShowCut"):
                                if obj.ShowCut:
                                    self.svg += render.getHiddenSVG(linewidth="LWPlaceholder")
                            # print render.info()

                        else:
                            # render using the Drawing module
                            import Drawing, Part
                            shapes = []
                            hshapes = []
                            sshapes = []
                            p = FreeCAD.Placement(obj.Source.Placement)
                            self.direction = p.Rotation.multVec(FreeCAD.Vector(0,0,1))
                            for o in objs:
                                if o.isDerivedFrom("Part::Feature"):
                                    if o.Shape.isNull():
                                        pass
                                        #FreeCAD.Console.PrintWarning(translate("Arch","Skipping empty object: ")+o.Name)
                                    elif o.Shape.isValid():
                                        if hasattr(obj.Source,"OnlySolids"):
                                            if obj.Source.OnlySolids:
                                                shapes.extend(o.Shape.Solids)
                                            else:
                                                shapes.append(o.Shape)
                                        else:
                                            shapes.extend(o.Shape.Solids)
                                    else:
                                        FreeCAD.Console.PrintWarning(translate("Arch","Skipping invalid object: ")+o.Name)
                            cutface,cutvolume,invcutvolume = ArchCommands.getCutVolume(obj.Source.Shape.copy(),shapes)
                            if cutvolume:
                                nsh = []
                                for sh in shapes:
                                    for sol in sh.Solids:
                                        if sol.Volume < 0:
                                            sol.reverse()
                                        c = sol.cut(cutvolume)
                                        s = sol.section(cutface)
                                        try:
                                            wires = DraftGeomUtils.findWires(s.Edges)
                                            for w in wires:
                                                f = Part.Face(w)
                                                sshapes.append(f)
                                            #s = Part.Wire(s.Edges)
                                            #s = Part.Face(s)
                                        except Part.OCCError:
                                            #print "ArchDrawingView: unable to get a face"
                                            sshapes.append(s)
                                        nsh.extend(c.Solids)
                                        #sshapes.append(s)
                                        if hasattr(obj,"ShowCut"):
                                            if obj.ShowCut:
                                                c = sol.cut(invcutvolume)
                                                hshapes.append(c)
                                shapes = nsh
                            if shapes:
                                self.shapes = shapes
                                self.baseshape = Part.makeCompound(shapes)
                                svgf = Drawing.projectToSVG(self.baseshape,self.direction)
                                if svgf:
                                    svgf = svgf.replace('stroke-width="0.35"','stroke-width="LWPlaceholder"')
                                    svgf = svgf.replace('stroke-width="1"','stroke-width="LWPlaceholder"')
                                    svgf = svgf.replace('stroke-width:0.01','stroke-width:LWPlaceholder')
                                    self.svg += svgf
                            if hshapes:
                                hshapes = Part.makeCompound(hshapes)
                                self.hiddenshape = hshapes
                                svgh = Drawing.projectToSVG(hshapes,self.direction)
                                if svgh:
                                    svgh = svgh.replace('stroke-width="0.35"','stroke-width="LWPlaceholder"')
                                    svgh = svgh.replace('stroke-width="1"','stroke-width="LWPlaceholder"')
                                    svgh = svgh.replace('stroke-width:0.01','stroke-width:LWPlaceholder')
                                    svgh = svgh.replace('fill="none"','fill="none"\nstroke-dasharray="DAPlaceholder"')
                                    self.svg += svgh
                            if sshapes:
                                svgs = ""
                                if hasattr(obj,"ShowFill"):
                                    if obj.ShowFill:
                                        svgs += fillpattern
                                        svgs += '<g transform="rotate(180)">\n'
                                        for s in sshapes:
                                            if s.Edges:
                                                f = Draft.getSVG(s,direction=self.direction.negative(),linewidth=0,fillstyle="sectionfill",color=(0,0,0))
                                                svgs += f
                                        svgs += "</g>\n"
                                sshapes = Part.makeCompound(sshapes)
                                self.sectionshape = sshapes
                                svgs += Drawing.projectToSVG(sshapes,self.direction)
                                if svgs:
                                    svgs = svgs.replace('stroke-width="0.35"','stroke-width="SWPlaceholder"')
                                    svgs = svgs.replace('stroke-width="1"','stroke-width="SWPlaceholder"')
                                    svgs = svgs.replace('stroke-width:0.01','stroke-width:SWPlaceholder')
                                    svgs = svgs.replace('stroke-width="0.35 px"','stroke-width="SWPlaceholder"')
                                    svgs = svgs.replace('stroke-width:0.35','stroke-width:SWPlaceholder')
                                    self.svg += svgs

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

    def getDXF(self,obj):
        "returns a DXF representation of the view"
        if obj.RenderingMode == "Solid":
            print "Unable to get DXF from Solid mode: ",obj.Label
            return ""
        result = []
        import Drawing
        if not hasattr(self,"baseshape"):
            self.onChanged(obj,"Source")
        if hasattr(self,"baseshape"):
            if self.baseshape:
                result.append(Drawing.projectToDXF(self.baseshape,self.direction))
        if hasattr(self,"sectionshape"):
            if self.sectionshape:
                result.append(Drawing.projectToDXF(self.sectionshape,self.direction))
        if hasattr(self,"hiddenshape"):
            if self.hiddenshape:
                result.append(Drawing.projectToDXF(self.hiddenshape,self.direction))
        return result

class SectionPlaneTaskPanel:
    '''A TaskPanel for all the section plane object'''
    def __init__(self):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.

        self.obj = None
        self.form = QtGui.QWidget()
        self.form.setObjectName("TaskPanel")
        self.grid = QtGui.QGridLayout(self.form)
        self.grid.setObjectName("grid")
        self.title = QtGui.QLabel(self.form)
        self.grid.addWidget(self.title, 0, 0, 1, 2)

        # tree
        self.tree = QtGui.QTreeWidget(self.form)
        self.grid.addWidget(self.tree, 1, 0, 1, 2)
        self.tree.setColumnCount(1)
        self.tree.header().hide()

        # buttons
        self.addButton = QtGui.QPushButton(self.form)
        self.addButton.setObjectName("addButton")
        self.addButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.addButton, 3, 0, 1, 1)

        self.delButton = QtGui.QPushButton(self.form)
        self.delButton.setObjectName("delButton")
        self.delButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.grid.addWidget(self.delButton, 3, 1, 1, 1)

        QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        QtCore.QObject.connect(self.delButton, QtCore.SIGNAL("clicked()"), self.removeElement)
        self.update()

    def isAllowedAlterSelection(self):
        return True

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def getIcon(self,obj):
        if hasattr(obj.ViewObject,"Proxy"):
            return QtGui.QIcon(obj.ViewObject.Proxy.getIcon())
        elif obj.isDerivedFrom("Sketcher::SketchObject"):
            return QtGui.QIcon(":/icons/Sketcher_Sketch.svg")
        else:
            return QtGui.QIcon(":/icons/Tree_Part.svg")

    def update(self):
        'fills the treewidget'
        self.tree.clear()
        if self.obj:
            for o in self.obj.Objects:
                item = QtGui.QTreeWidgetItem(self.tree)
                item.setText(0,o.Name)
                item.setIcon(0,self.getIcon(o))
        self.retranslateUi(self.form)

    def addElement(self):
        if self.obj:
            for o in FreeCADGui.Selection.getSelection():
                ArchComponent.addToComponent(self.obj,o,"Objects")
            self.update()

    def removeElement(self):
        if self.obj:
            it = self.tree.currentItem()
            if it:
                comp = FreeCAD.ActiveDocument.getObject(str(it.text(0)))
                ArchComponent.removeFromComponent(self.obj,comp)
            self.update()

    def accept(self):
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Objects", None, QtGui.QApplication.UnicodeUTF8))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None, QtGui.QApplication.UnicodeUTF8))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add", None, QtGui.QApplication.UnicodeUTF8))
        self.title.setText(QtGui.QApplication.translate("Arch", "Objects seen by this section plane", None, QtGui.QApplication.UnicodeUTF8))

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_SectionPlane',_CommandSectionPlane())
