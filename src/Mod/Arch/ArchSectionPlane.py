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

import FreeCAD,FreeCADGui,ArchComponent,WorkingPlane,math,Draft,ArchCommands,DraftVecUtils
from FreeCAD import Vector
from PyQt4 import QtCore
from pivy import coin
from DraftTools import translate

def makeSectionPlane(objectslist=None):
    """makeSectionPlane([objectslist]) : Creates a Section plane objects including the
    given objects. If no object is given, the whole document will be considered."""
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Section")
    _SectionPlane(obj)
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

def makeSectionView(section):
    """makeSectionView(section) : Creates a Drawing view of the given Section Plane
    in the active Page object (a new page will be created if none exists"""
    page = None
    for o in FreeCAD.ActiveDocument.Objects:
        if o.isDerivedFrom("Drawing::FeaturePage"):
            page = o
            break
    if not page:
        page = FreeCAD.ActiveDocument.addObject("Drawing::FeaturePage",str(translate("Arch","Page")))
        page.Template = Draft.getParam("template",FreeCAD.getResourceDir()+'Mod/Drawing/Templates/A3_Landscape.svg')
        
    view = FreeCAD.ActiveDocument.addObject("Drawing::FeatureViewPython","View")
    page.addObject(view)
    _ArchDrawingView(view)
    view.Source = section
    view.Label = str(translate("Arch","View of"))+" "+section.Name
    return view

class _CommandSectionPlane:
    "the Arch SectionPlane command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_SectionPlane',
                'Accel': "S, E",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_SectionPlane","Section Plane"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_SectionPlane","Creates a section plane object, including the selected objects")}

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        ss = "["
        for o in sel:
            if len(ss) > 1:
                ss += ","
            ss += "FreeCAD.ActiveDocument."+o.Name
        ss += "]"
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Section Plane")))
        FreeCADGui.doCommand("import Arch")
        FreeCADGui.doCommand("section = Arch.makeSectionPlane("+ss+")")
        FreeCADGui.doCommand("Arch.makeSectionView(section)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

class _SectionPlane:
    "A section plane object"
    def __init__(self,obj):
        obj.Proxy = self
        obj.addProperty("App::PropertyPlacement","Placement","Base",
                        str(translate("Arch","The placement of this object")))
        obj.addProperty("Part::PropertyPartShape","Shape","Base","")
        obj.addProperty("App::PropertyLinkList","Objects","Arch",
                        str(translate("Arch","The objects that must be considered by this section plane. Empty means all document")))
        self.Type = "SectionPlane"
        
    def execute(self,obj):
        import Part
        l = obj.ViewObject.DisplaySize
        p = Part.makePlane(l,l,Vector(l/2,-l/2,0),Vector(0,0,-1))
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

class _ViewProviderSectionPlane(ArchComponent.ViewProviderComponent):
    "A View Provider for Section Planes"
    def __init__(self,vobj):
        vobj.addProperty("App::PropertyLength","DisplaySize","Arch",
                        str(translate("Arch","The display size of this section plane")))
        vobj.addProperty("App::PropertyPercent","Transparency","Base","")
        vobj.addProperty("App::PropertyFloat","LineWidth","Base","")
        vobj.addProperty("App::PropertyColor","LineColor","Base","")
        vobj.DisplaySize = 1
        vobj.Transparency = 85
        vobj.LineWidth = 1
        vobj.LineColor = (0.0,0.0,0.4,1.0)
        vobj.Proxy = self
        self.Object = vobj.Object

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_SectionPlane_Tree.svg"

    def claimChildren(self):
        return []

    def attach(self,vobj):
        self.mat1 = coin.SoMaterial()
        self.mat2 = coin.SoMaterial()
        self.fcoords = coin.SoCoordinate3()
        fs = coin.SoType.fromName("SoBrepFaceSet").createInstance()
        fs.coordIndex.setValues(0,7,[0,1,2,-1,0,2,3])
        self.drawstyle = coin.SoDrawStyle()
        self.drawstyle.style = coin.SoDrawStyle.LINES
        self.lcoords = coin.SoCoordinate3()
        ls = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        ls.coordIndex.setValues(0,57,[0,1,-1,2,3,4,5,-1,6,7,8,9,-1,10,11,-1,12,13,14,15,-1,16,17,18,19,-1,20,21,-1,22,23,24,25,-1,26,27,28,29,-1,30,31,-1,32,33,34,35,-1,36,37,38,39,-1,40,41,42,43,44])
        psep = coin.SoSeparator()
        fsep = coin.SoSeparator()
        fsep.addChild(self.mat2)
        fsep.addChild(self.fcoords)
        fsep.addChild(fs)
        psep.addChild(fsep)
        psep.addChild(self.mat1)
        psep.addChild(self.drawstyle)
        psep.addChild(self.lcoords)
        psep.addChild(ls)
        vobj.addDisplayMode(psep,"Default")
        self.onChanged(vobj,"DisplaySize")
        self.onChanged(vobj,"LineColor")
        self.onChanged(vobj,"Transparency")
        
    def getDisplayModes(self,vobj):
        return ["Default"]

    def getDefaultDisplayMode(self):
        return "Default"

    def setDisplayMode(self,mode):
        return mode

    def updateData(self,obj,prop):
        if prop in ["Placement"]:
            self.onChanged(obj.ViewObject,"DisplaySize")
        return

    def onChanged(self,vobj,prop):
        if prop == "LineColor":
            l = vobj.LineColor
            self.mat1.diffuseColor.setValue([l[0],l[1],l[2]])
            self.mat2.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "Transparency":
            if hasattr(vobj,"Transparency"):
                self.mat2.transparency.setValue(vobj.Transparency/100.0)
        elif prop == "DisplaySize":
            hd = vobj.DisplaySize/2
            verts = []
            fverts = []
            for v in [[-hd,-hd],[hd,-hd],[hd,hd],[-hd,hd]]:
                l1 = hd/3
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
        return

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

class _ArchDrawingView:
    def __init__(self, obj):
        obj.addProperty("App::PropertyLink","Source","Base","The linked object")
        obj.addProperty("App::PropertyEnumeration","RenderingMode","Drawing View","The rendering mode to use")
        obj.addProperty("App::PropertyBool","ShowCut","Drawing View","If cut geometry is shown or not")
        obj.addProperty("App::PropertyFloat","LineWidth","Drawing View","The line width of the rendered objects")
        obj.RenderingMode = ["Solid","Wireframe"]
        obj.RenderingMode = "Wireframe"
        obj.LineWidth = 0.35
        obj.ShowCut = False
        obj.Proxy = self
        self.Type = "ArchSectionView"

    def execute(self, obj):
        if obj.Source:
            obj.ViewResult = self.updateSVG(obj)
            
    def onChanged(self, obj, prop):
        if prop in ["Source","RenderingMode"]:
            self.buildSVG(obj)
            obj.ViewResult = self.updateSVG(obj)

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

    def getFlatShape(self):
        "returns a flat shape representation of the view"
        if hasattr(self,"baseshape"):
            import Drawing
            [V0,V1,H0,H1] = Drawing.project(self.baseshape,self.direction)
            return V0.Edges+V1.Edges
        else:
            FreeCAD.Console.PrintMessage(str(translate("Arch","No shape has been computed yet, select wireframe rendering and render again")))
            return None

    def getDXF(self):
        "returns a flat shape representation of the view"
        if hasattr(self,"baseshape"):
            import Drawing
            [V0,V1,H0,H1] = Drawing.project(self.baseshape,self.direction)
            DxfOutput = Drawing.projectToDXF(self.baseshape,self.direction)
            return DxfOutput
        else:
            FreeCAD.Console.PrintMessage(str(translate("Arch","No shape has been computed yet, select wireframe rendering and render again")))
            return None

    def buildSVG(self, obj,join=False):
        "creates a svg representation"
        import Part, DraftGeomUtils
        if hasattr(obj,"Source"):
            if obj.Source:
                if obj.Source.Objects:
                    objs = Draft.getGroupContents(obj.Source.Objects,walls=True)
                    objs = Draft.removeHidden(objs)
                    self.svg = ''

                    # generating SVG
                    if obj.RenderingMode == "Solid":
                        # render using the Arch Vector Renderer                        
                        import ArchVRM
                        render = ArchVRM.Renderer()
                        render.setWorkingPlane(obj.Source.Placement)
                        render.addObjects(objs)
                        if hasattr(obj,"ShowCut"):
                            render.cut(obj.Source.Shape,obj.ShowCut)
                        else:
                            render.cut(obj.Source.Shape)
                        self.svg += render.getViewSVG(linewidth="LWPlaceholder")
                        self.svg += render.getSectionSVG(linewidth="SWPLaceholder")
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
                                if o.Shape.isValid():
                                    shapes.extend(o.Shape.Solids)
                                else:
                                    FreeCAD.Console.PrintWarning(str(translate("Arch","Skipping invalid object: "))+o.Name)
                        cutface,cutvolume,invcutvolume = ArchCommands.getCutVolume(obj.Source.Shape.copy(),shapes)
                        if cutvolume:
                            nsh = []
                            for sh in shapes:
                                for sol in sh.Solids:
                                    if sol.Volume < 0:
                                        sol.reverse()
                                    c = sol.cut(cutvolume)
                                    s = sol.section(cutface)
                                    nsh.extend(c.Solids)
                                    sshapes.append(s)
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
                            svgh = Drawing.projectToSVG(hshapes,self.direction)
                            if svgh:
                                svgh = svgh.replace('stroke-width="0.35"','stroke-width="LWPlaceholder"')
                                svgh = svgh.replace('stroke-width="1"','stroke-width="LWPlaceholder"')
                                svgh = svgh.replace('stroke-width:0.01','stroke-width:LWPlaceholder')
                                svgh = svgh.replace('fill="none"','fill="none"\nstroke-dasharray="0.09,0.05"')                              
                                self.svg += svgh
                        if sshapes:
                            edges = []
                            for s in sshapes:
                                edges.extend(s.Edges)
                            wires = DraftGeomUtils.findWires(edges)
                            faces = []
                            for w in wires:
                                if (w.ShapeType == "Wire") and w.isClosed():
                                    faces.append(Part.Face(w))
                            sshapes = Part.makeCompound(faces)
                            svgs = Drawing.projectToSVG(sshapes,self.direction)
                            if svgs:
                                svgs = svgs.replace('stroke-width="0.35"','stroke-width="SWPlaceholder"')
                                svgs = svgs.replace('stroke-width="1"','stroke-width="SWPlaceholder"')
                                svgs = svgs.replace('stroke-width:0.01','stroke-width:SWPlaceholder')
                                self.svg += svgs

    def updateSVG(self, obj):
        "Formats and places the calculated svg stuff on the page"
        if not hasattr(self,"svg"):
            self.buildSVG(obj)
        else:
            if not self.svg:
                self.buildSVG(obj)
        if not hasattr(self,"svg"):
            return ''
        linewidth = obj.LineWidth/obj.Scale
        st = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetFloat("CutLineThickness")
        if not st:
            st = 2
        svg = self.svg.replace('LWPlaceholder', str(linewidth) + 'px')
        svg = svg.replace('SWPlaceholder', str(linewidth*st) + 'px')                
            
        result = ''
        result += '<g id="' + obj.Name + '"'
        result += ' transform="'
        result += 'rotate('+str(obj.Rotation)+','+str(obj.X)+','+str(obj.Y)+') '
        result += 'translate('+str(obj.X)+','+str(obj.Y)+') '
        result += 'scale('+str(obj.Scale)+','+str(-obj.Scale)+')'
        result += '">\n'
        result += svg
        result += '</g>\n'
        # print "complete node:",result
        return result
                
FreeCADGui.addCommand('Arch_SectionPlane',_CommandSectionPlane())
