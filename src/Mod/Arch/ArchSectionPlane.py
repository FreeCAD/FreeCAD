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

import FreeCAD,FreeCADGui,ArchComponent,WorkingPlane,Drawing,math,Draft
from FreeCAD import Vector
from PyQt4 import QtCore
from pivy import coin
from draftlibs import fcvec


class _CommandSectionPlane:
    "the Arch SectionPlane command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_SectionPlane',
                'Accel': "S, P",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_SectionPlane","Section Plane"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_SectionPlane","Adds a section plane object to the document")}

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        FreeCAD.ActiveDocument.openTransaction("Section Plane")
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Section")
        _SectionPlane(obj)
        _ViewProviderSectionPlane(obj.ViewObject)
        FreeCAD.ActiveDocument.commitTransaction()
        g = []
        for o in sel:
            if o.isDerivedFrom("Part::Feature"):
                g.append(o)
        obj.Objects = g
        page = FreeCAD.ActiveDocument.addObject("Drawing::FeaturePage","Page")
        template = Draft.getParam("template")
        if not template:
            template = FreeCAD.getResourceDir()+'Mod/Drawing/Templates/A3_Landscape.svg'
        page.ViewObject.HintOffsetX = 200
        page.ViewObject.HintOffsetY = 100
        page.ViewObject.HintScale = 20
        page.Template = template
        view = FreeCAD.ActiveDocument.addObject("Drawing::FeatureViewPython","View")
        page.addObject(view)
        _ArchDrawingView(view)
        view.Source = obj
        FreeCAD.ActiveDocument.recompute()

class _SectionPlane:
    "A section plane object"
    def __init__(self,obj):
        obj.Proxy = self
        obj.addProperty("App::PropertyLinkList","Objects","Base",
                        "The objects that must be considered by this section plane. Empty means all document")
        self.Type = "SectionPlane"
        
    def execute(self,obj):
        import Part
        pl = obj.Placement
        l = obj.ViewObject.DisplaySize
        p = Part.makePlane(l,l,Vector(l/2,-l/2,0),Vector(0,0,-1))
        obj.Shape = p
        obj.Placement = pl

    def onChanged(self,obj,prop):
        pass

    def getNormal(self,obj):
        return obj.Shape.Faces[0].normalAt(0,0)

class _ViewProviderSectionPlane(ArchComponent.ViewProviderComponent):
    "A View Provider for Section Planes"
    def __init__(self,vobj):
        vobj.addProperty("App::PropertyLength","DisplaySize","Base",
                        "The display size of the section plane")
        vobj.DisplaySize = 1
        vobj.Transparency = 85
        vobj.LineWidth = 1
        vobj.LineColor = (0.0,0.0,0.4,1.0)
        vobj.Proxy = self
        self.Object = vobj.Object

    def getIcon(self):
        return ":/icons/Arch_SectionPlane_Tree.svg"

    def claimChildren(self):
        return []

    def attach(self,vobj):
        self.Object = vobj.Object
        # adding arrows
        rn = vobj.RootNode
        self.col = coin.SoBaseColor()
        self.setColor()
        ds = coin.SoDrawStyle()
        ds.style = coin.SoDrawStyle.LINES
        self.lcoords = coin.SoCoordinate3()
        ls = coin.SoLineSet()
        ls.numVertices.setValues([2,4,4,2,4,4,2,4,4,2,4,4])
        pt = coin.SoAnnotation()
        pt.addChild(self.col)
        pt.addChild(ds)
        pt.addChild(self.lcoords)
        pt.addChild(ls)
        rn.addChild(pt)
        self.setVerts()

    def setColor(self):
        print self.Object.ViewObject.LineColor
        self.col.rgb.setValue(self.Object.ViewObject.LineColor[0],
                              self.Object.ViewObject.LineColor[1],
                              self.Object.ViewObject.LineColor[2])

    def setVerts(self):
        def extendVerts(x,y):
            l1 = hd/3
            l2 = l1/3
            verts.extend([[x,y,0],[x,y,-l1]])
            verts.extend([[x,y,-l1],[x-l2,y,-l1+l2],[x+l2,y,-l1+l2],[x,y,-l1]])
            verts.extend([[x,y,-l1],[x,y-l2,-l1+l2],[x,y+l2,-l1+l2],[x,y,-l1]])
        hd = self.Object.ViewObject.DisplaySize/2
        verts = []
        extendVerts(-hd,-hd)
        extendVerts(hd,-hd)
        extendVerts(hd,hd)
        extendVerts(-hd,hd)
        self.lcoords.point.setValues(verts)

    def updateData(self,obj,prop):
        if prop in ["Shape","Placement"]:
            self.setVerts()
        return

    def onChanged(self,vobj,prop):
        if prop == "LineColor":
            self.setColor()
        elif prop == "DisplaySize":
            vobj.Object.Proxy.execute(vobj.Object)
        return

class _ArchDrawingView:
    def __init__(self, obj):
        obj.addProperty("App::PropertyLink","Source","Base","The linked object")
        obj.addProperty("App::PropertyEnumeration","RenderingMode","Base","The rendering mode to use")
        obj.RenderingMode = ["Solid","Wireframe"]
        obj.RenderingMode = "Solid"
        obj.Proxy = self
        self.Type = "DrawingView"

    def execute(self, obj):
        if obj.Source:
            obj.ViewResult = self.updateSVG(obj)
            
    def onChanged(self, obj, prop):
        if prop in ["Source","RenderingMode"]:
            obj.ViewResult = self.updateSVG(obj)

    def updateSVG(self, obj):
        "encapsulates a svg fragment into a transformation node"
        if obj.Source:
            if obj.Source.Objects:
                svg = ''
                if obj.RenderingMode == "Solid":
                    svg += self.renderVRM(obj.Source.Objects,obj.Source.Placement)
                else:
                    svg += self.renderOCC(obj.Source.Objects,obj.Source.Proxy.getNormal(obj.Source))
                result = ''
                result += '<g id="' + obj.Name + '"'
                result += ' transform="'
                result += 'rotate('+str(obj.Rotation)+','+str(obj.X)+','+str(obj.Y)+') '
                result += 'translate('+str(obj.X)+','+str(obj.Y)+') '
                result += 'scale('+str(obj.Scale)+','+str(-obj.Scale)+')'
                result += '">\n'
                result += svg
                result += '</g>\n'
                print "complete node:",result
                return result
        return ''

    def renderOCC(self,objs,direction):
        "renders an SVG fragment with the OCC method"
        os = objs[:]
        if os:
            sh = os.pop().Shape
            for o in os:
                sh = sh.fuse(o.Shape)
        result = Drawing.projectToSVG(sh,fcvec.neg(direction))
        if result:
            result = result.replace('stroke-width="0.35"','stroke-width="0.01 px"')
            return result
        return ''

    def renderVRM(self,objs,placement):
        "renders an SVG fragment with the ArchVRM method"
        import ArchVRM
        render = ArchVRM.Renderer()
        render.setWorkingPlane(FreeCAD.Placement(placement))
        for o in objs:
            render.add(o)
        svg = render.getSVG()
        return svg

FreeCADGui.addCommand('Arch_SectionPlane',_CommandSectionPlane())
