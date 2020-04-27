# -*- coding: utf8 -*-

#***************************************************************************
#*   Copyright (c) 2020 Yorik van Havre <yorik@uncreated.net>              *
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

__title__="FreeCAD Arch Curtain Wall"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

import math,sys
import FreeCAD
import Draft
import ArchComponent
import ArchCommands
import DraftVecUtils

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

## @package ArchCurtainWall
#  \ingroup ARCH
#  \brief The Curtain Wall object and tools
#
#  This module provides tools to build Curtain wall objects.

"""
Curtain wall tool

Abstract: Curtain walls need a surface to work on (base).
They then divide that surface into a grid, by intersecting
it with a grid of planes, forming pseudorectangular facets.

The vertical lines can then receive one type of profile
(vertical mullions), the horizontal ones another
(horizontal mullions), and the facets a third (panels).

The surface can be prepared before applying the curtain wall
tool on it, which allow for more complex panel/mullion
configuration.

We then have two cases, depending on the surface: Either the
four corners of each facet form a plane, in which case the
panel filling is rectangular, or they don't, in which case
the facet is triangulated and receives a third mullion
(diagonal mullion).
"""


def makeCurtainWall(baseobj=None,name="Curtain Wall"):

    """
    makeCurtainWall([baseobj]): Creates a curtain wall in the active document
    """

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","CurtainWall")
    obj.Label = translate("Arch","Curtain Wall")
    CurtainWall(obj)
    if FreeCAD.GuiUp:
        ViewProviderCurtainWall(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
        if FreeCAD.GuiUp:
            baseobj.ViewObject.hide()
    return obj


class CommandArchCurtainWall:


    "the Arch Curtain Wall command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_CurtainWall',
                'MenuText': QT_TRANSLATE_NOOP("Arch_CurtainWall","Curtain Wall"),
                'Accel': "C, W",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_CurtainWall","Creates a curtain wall object from selected line or from scratch")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        sel = FreeCADGui.Selection.getSelection()
        if len(sel) > 1:
            FreeCAD.Console.PrintError(translate("Arch","Please select only one base object or none")+"\n")
        elif len(sel) == 1:
            # build on selection
            FreeCADGui.Control.closeDialog()
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Curtain Wall"))
            FreeCADGui.addModule("Draft")
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("obj = Arch.makeCurtainWall(FreeCAD.ActiveDocument."+FreeCADGui.Selection.getSelection()[0].Name+")")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        else:
            # interactive line drawing
            self.points = []
            if hasattr(FreeCAD,"DraftWorkingPlane"):
                FreeCAD.DraftWorkingPlane.setup()
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.getPoint(callback=self.getPoint)

    def getPoint(self,point=None,obj=None):

        """Callback for clicks during interactive mode"""

        if point is None:
            # cancelled
            return
        self.points.append(point)
        if len(self.points) == 1:
            FreeCADGui.Snapper.getPoint(last=self.points[0],callback=self.getPoint)
        elif len(self.points) == 2:
            FreeCADGui.Control.closeDialog()
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Curtain Wall"))
            FreeCADGui.addModule("Draft")
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("baseline = Draft.makeLine(FreeCAD."+str(self.points[0])+",FreeCAD."+str(self.points[1])+")")
            FreeCADGui.doCommand("base = FreeCAD.ActiveDocument.addObject('Part::Extrusion','Extrude')")
            FreeCADGui.doCommand("base.Base = baseline")
            FreeCADGui.doCommand("base.DirMode = 'Custom'")
            FreeCADGui.doCommand("base.Dir = App.Vector(FreeCAD.DraftWorkingPlane.axis)")
            FreeCADGui.doCommand("base.LengthFwd = 1000")
            FreeCADGui.doCommand("obj = Arch.makeCurtainWall(base)")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()


class CurtainWall(ArchComponent.Component):


    "The curtain wall object"

    def __init__(self,obj):

        ArchComponent.Component.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Curtain Wall"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "VerticalMullionNumber" in pl:
            obj.addProperty("App::PropertyInteger","VerticalMullionNumber","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of vertical mullions"))
            obj.setEditorMode("VerticalMullionNumber",1)
        if not "VerticalDirection" in pl:
            obj.addProperty("App::PropertyVector","VerticalDirection","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The vertical direction of this curtain wall"))
            obj.VerticalDirection = FreeCAD.Vector(0,0,1)
        if not "VerticalSections" in pl:
            obj.addProperty("App::PropertyInteger","VerticalSections","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of vertical sections of this curtain wall"))
            obj.VerticalSections = 4
        if not "VerticalMullionSize" in pl:
            obj.addProperty("App::PropertyLength","VerticalMullionSize","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The size of the vertical mullions, if no profile is used"))
            obj.VerticalMullionSize = 100
        if not "VerticalMullionProfile" in pl:
            obj.addProperty("App::PropertyLink","VerticalMullionProfile","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","A profile for vertical mullions (disables vertical mullion size)"))
        if not "HorizontalMullionNumber" in pl:
            obj.addProperty("App::PropertyInteger","HorizontalMullionNumber","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of horizontal mullions"))
            obj.setEditorMode("HorizontalMullionNumber",1)
        if not "HorizontalDirection" in pl:
            obj.addProperty("App::PropertyVector","HorizontalDirection","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The horizontal direction of this curtain wall"))
        if not "HorizontalSections" in pl:
            obj.addProperty("App::PropertyInteger","HorizontalSections","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of horizontal sections of this curtain wall"))
            obj.HorizontalSections = 4
        if not "HorizontalMullionSize" in pl:
            obj.addProperty("App::PropertyLength","HorizontalMullionSize","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The size of the horizontal mullions, if no profile is used"))
            obj.HorizontalMullionSize = 50
        if not "HorizontalMullionProfile" in pl:
            obj.addProperty("App::PropertyLink","HorizontalMullionProfile","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","A profile for horizontal mullions (disables horizontal mullion size)"))
        if not "DiagonalMullionNumber" in pl:
            obj.addProperty("App::PropertyInteger","DiagonalMullionNumber","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of diagonal mullions"))
            obj.setEditorMode("DiagonalMullionNumber",1)
        if not "DiagonalMullionSize" in pl:
            obj.addProperty("App::PropertyLength","DiagonalMullionSize","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The size of the diagonal mullions, if any, if no profile is used"))
            obj.DiagonalMullionSize = 50
        if not "DiagonalMullionProfile" in pl:
            obj.addProperty("App::PropertyLink","DiagonalMullionProfile","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","A profile for diagonal mullions, if any (disables horizontal mullion size)"))
        if not "PanelNumber" in pl:
            obj.addProperty("App::PropertyInteger","PanelNumber","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of panels"))
            obj.setEditorMode("PanelNumber",1)
        if not "PanelThickness" in pl:
            obj.addProperty("App::PropertyLength","PanelThickness","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The thickness of the panels"))
            obj.PanelThickness = 20
        if not "SwapHorizontalVertical" in pl:
            obj.addProperty("App::PropertyBool","SwapHorizontalVertical","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","Swaps horizontal and vertical lines"))
        if not "Normal" in pl:
            obj.addProperty("App::PropertyVector","Normal","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The normal direction of this curtain wall"))
        if not "Refine" in pl:
            obj.addProperty("App::PropertyBool","Refine","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","Perform subtractions between components so none overlap"))


    def onDocumentRestored(self,obj):

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def onChanged(self,obj,prop):

        ArchComponent.Component.onChanged(self,obj,prop)

    def execute(self,obj):

        if self.clone(obj):
            return

        import Part,DraftGeomUtils

        pl = obj.Placement

        # test properties
        if not obj.Base:
            FreeCAD.Console.PrintLog(obj.Label+": no base\n")
            return
        if not hasattr(obj.Base,"Shape"):
            FreeCAD.Console.PrintLog(obj.Label+": invalid base\n")
            return
        if not obj.Base.Shape.Faces:
            FreeCAD.Console.PrintLog(obj.Label+": no faces in base\n")
            return
        if obj.VerticalMullionProfile:
            if not hasattr(obj.VerticalMullionProfile,"Shape"):
                FreeCAD.Console.PrintLog(obj.Label+": invalid vertical mullion profile\n")
                return
        if obj.HorizontalMullionProfile:
            if not hasattr(obj.HorizontalMullionProfile,"Shape"):
                FreeCAD.Console.PrintLog(obj.Label+": invalid horizontal mullion profile\n")
                return
        if obj.DiagonalMullionProfile:
            if not hasattr(obj.DiagonalMullionProfile,"Shape"):
                FreeCAD.Console.PrintLog(obj.Label+": invalid diagonal mullion profile\n")
                return

        # identify normal, vdir and hdir directions
        normal = obj.Normal
        if not normal.Length:
            normal = DraftGeomUtils.getNormal(obj.Base.Shape)
            if not normal:
                FreeCAD.Console.PrintLog(obj.Label+": unable to calculate normal\n")
                return
            else:
                # set the normal if not yet set
                obj.Normal = normal
        normal.normalize()
        if obj.VerticalDirection.Length:
            vdir = obj.VerticalDirection
        else:
            FreeCAD.Console.PrintLog(obj.Label+": vertical direction not set\n")
            return
        vdir.normalize()
        if obj.HorizontalDirection.Length:
            hdir = obj.HorizontalDirection
        else:
            hdir = normal.cross(obj.VerticalDirection)
            if hdir and hdir.Length:
                obj.HorizontalDirection = hdir
            else:
                FreeCAD.Console.PrintLog(obj.Label+": unable to calculate horizontal direction\n")
                return
        hdir.normalize()
        #print(hdir,vdir,normal)

        # calculate boundbox points
        verts = [v.Point for v in obj.Base.Shape.Vertexes]
        hverts = [self.getProjectedLength(v,hdir) for v in verts]
        vverts = [self.getProjectedLength(v,vdir) for v in verts]
        nverts = [self.getProjectedLength(v,normal) for v in verts]
        #print(hverts,vverts,nverts)
        MinH = min(hverts)
        MaxH = max(hverts)
        MinV = min(vverts)
        MaxV = max(vverts)
        MinN = min(nverts)
        MaxN = max(nverts)
        
        # also define extended bbox to better boolean ops results
        ExtMinH = MinH-5
        ExtMaxH = MaxH+5
        ExtMinV = MinV-5
        ExtMaxV = MaxV+5
        ExtMinN = MinN-5
        ExtMaxN = MaxN+5
        
        # construct vertical planes
        vplanes = []
        if obj.VerticalSections > 1:
            p0 = FreeCAD.Vector(normal).multiply(ExtMinN)
            p0 = p0.add(FreeCAD.Vector(hdir).multiply(MinH))
            p0 = p0.add(FreeCAD.Vector(vdir).multiply(ExtMinV))
            p1 = p0.add(FreeCAD.Vector(normal).multiply(ExtMaxN-ExtMinN))
            p2 = p1.add(FreeCAD.Vector(vdir).multiply(ExtMaxV-ExtMinV))
            p3 = p0.add(FreeCAD.Vector(vdir).multiply(ExtMaxV-ExtMinV))
            vplane = Part.Face(Part.makePolygon([p0,p1,p2,p3,p0]))
            vstep = FreeCAD.Vector(hdir).multiply((MaxH-MinH)/obj.VerticalSections)
            for i in range(1,obj.VerticalSections):
                vplane = vplane.translate(vstep)
                vplanes.append(vplane.copy())

        # construct horizontal planes
        hplanes = []
        if obj.HorizontalSections > 1:
            p4 = FreeCAD.Vector(normal).multiply(ExtMinN)
            p4 = p4.add(FreeCAD.Vector(hdir).multiply(ExtMinH))
            p4 = p4.add(FreeCAD.Vector(vdir).multiply(MinV))
            p5 = p4.add(FreeCAD.Vector(normal).multiply(ExtMaxN-ExtMinN))
            p6 = p5.add(FreeCAD.Vector(hdir).multiply(ExtMaxH-ExtMinH))
            p7 = p4.add(FreeCAD.Vector(hdir).multiply(ExtMaxH-ExtMinH))
            hplane = Part.Face(Part.makePolygon([p4,p5,p6,p7,p4]))
            hstep = FreeCAD.Vector(vdir).multiply((MaxV-MinV)/obj.HorizontalSections)
            for i in range(1,obj.HorizontalSections):
                hplane = hplane.translate(hstep)
                hplanes.append(hplane.copy())

        # apply sections
        baseshape = obj.Base.Shape.copy()
        for plane in vplanes+hplanes:
            baseshape = baseshape.cut(plane)

        # make edge/normal relation table
        edgetable = {}
        for face in baseshape.Faces:
            for edge in face.Edges:
                ec = edge.hashCode()
                if ec in edgetable:
                    edgetable[ec].append(face)
                else:
                    edgetable[ec] = [face]
        self.edgenormals = {}
        for ec,faces in edgetable.items():
            if len(faces) == 1:
                self.edgenormals[ec] = faces[0].normalAt(0,0)
            else:
                n = faces[0].normalAt(0,0).cross(faces[1].normalAt(0,0))
                if n.Length:
                    n.normalize()
                else:
                    n = faces[0].normalAt(0,0)
                self.edgenormals[ec] = n

        # construct vertical mullions
        vmullions = []
        vprofile = self.getMullionProfile(obj,"Vertical")
        if vprofile:
            vedges = self.getNormalEdges(baseshape.Edges,hdir)
            vmullions = self.makeMullions(vedges,vprofile,normal)

        # construct horizontal mullions
        hmullions = []
        hprofile = self.getMullionProfile(obj,"Horizontal")
        if hprofile:
            hedges = self.getNormalEdges(baseshape.Edges,vdir)
            hmullions = self.makeMullions(hedges,hprofile,normal)

        # construct panels
        panels = []
        dedges = []
        if obj.PanelThickness.Value:
            for face in baseshape.Faces:
                verts = [v.Point for v in face.OuterWire.OrderedVertexes]
                if len(verts) == 4:
                    if DraftGeomUtils.isPlanar(verts):
                        panel = self.makePanel(verts,obj.PanelThickness.Value)
                        panels.append(panel)
                    else:
                        verts1 = [verts[0],verts[1],verts[2]]
                        panel = self.makePanel(verts1,obj.PanelThickness.Value)
                        panels.append(panel)
                        verts2 = [verts[0],verts[2],verts[3]]
                        panel = self.makePanel(verts2,obj.PanelThickness.Value)
                        panels.append(panel)
                        dedges.append(Part.makeLine(verts[0],verts[2]))

        # construct diagonal mullions
        dmullions = []
        if dedges:
            n = (dedges[0].Vertexes[-1].Point.sub(dedges[0].Point))
            dprofile = self.getMullionProfile(obj,"Diagonal")
            if dprofile:
                dmullions = self.makeMullions(dedges,dprofile,normal)

        # perform subtractions
        if obj.Refine:
            subvmullion = None
            subhmullion = None
            subdmullion = None
            if vmullions:
                subvmullion = vmullions[0].copy()
                for m in vmullions[1:]:
                    subvmullion = subvmullion.fuse(m)
            if hmullions:
                subhmullion = hmullions[0].copy()
                for m in hmullions[1:]:
                    subhmullion = subhmullion.fuse(m)
            if dmullions:
                subdmullion = dmullions[0].copy()
                for m in dmullions[1:]:
                    subdmullion = subdmullion.fuse(m)
            if subvmullion:
                hmullions = [m.cut(subvmullion) for m in hmullions]
                if subhmullion:
                    dmullions = [m.cut(subvmullion) for m in dmullions]
                    dmullions = [m.cut(subhmullion) for m in dmullions]
                    panels = [m.cut(subvmullion) for m in panels]
                    panels = [m.cut(subhmullion) for m in panels]
                    if subdmullion:
                        panels = [m.cut(subdmullion) for m in panels]

        # mount shape
        shape = Part.makeCompound(vmullions+hmullions+dmullions+panels)
        shape = self.processSubShapes(obj,shape,pl)
        self.applyShape(obj,shape,pl)
        obj.VerticalMullionNumber = len(vmullions)
        obj.HorizontalMullionNumber = len(hmullions)
        obj.DiagonalMullionNumber = len(dmullions)
        obj.PanelNumber = len(panels)

    def makePanel(self,verts,thickness):

        """creates a panel from face points and thickness"""

        import Part

        panel = Part.Face(Part.makePolygon(verts+[verts[0]]))
        n = panel.normalAt(0,0)
        n.multiply(thickness)
        panel = panel.extrude(n)
        return panel

    def makeMullions(self,edges,profile,upvec):

        """creates a list of mullions from a list of edges and a profile"""

        mullions = []
        pcenter = FreeCAD.Vector(0,0,0)
        if hasattr(profile,"CenterOfMass"):
            center = profile.CenterOfMass
        for edge in edges:
            p0 = edge.Vertexes[0].Point
            p1 = edge.Vertexes[-1].Point
            axis = p1.sub(p0)
            if edge.hashCode() in self.edgenormals:
                normal = self.edgenormals[edge.hashCode()]
            else:
                normal = self.normal
            mullion = self.rotateProfile(profile,axis,normal)
            mullion = mullion.translate(p0.sub(center))
            mullion = mullion.extrude(p1.sub(p0))
            mullions.append(mullion)
        return mullions

    def getMullionProfile(self,obj,direction):

        """returns a profile shape already properly oriented, ready for extrude"""

        import Part,DraftGeomUtils

        prop1 = getattr(obj,direction+"MullionProfile")
        prop2 = getattr(obj,direction+"MullionSize").Value
        if prop1:
            profile = prop1.Shape.copy()
        else:
            if not prop2:
                return None
            profile = Part.Face(Part.makePlane(prop2,prop2,FreeCAD.Vector(-prop2/2,-prop2/2,0)))
        return profile

    def rotateProfile(self,profile,axis,normal):
        
        """returns a rotated profile"""
        
        import Part,DraftGeomUtils

        oaxis = DraftGeomUtils.getNormal(profile)
        if len(profile.Edges[0].Vertexes) > 1:
            oxvec = profile.Edges[0].Vertexes[-1].Point.sub(profile.Edges[0].Vertexes[0].Point)
        elif hasattr(profile.Curve,"Center"):
            oxvec = profile.Edges[0].Vertexes[-1].Point.sub(profile.Curve.Center)
        orot = FreeCAD.Rotation(oxvec,oaxis.cross(oxvec),oaxis,"XYZ")
        nrot = FreeCAD.Rotation(normal,axis.cross(normal),axis,"XYZ")
        r = nrot.multiply(orot.inverted())
        if hasattr(profile,"CenterOfMass"):
            c = profile.CenterOfMass
        elif hasattr(profile,"BoundBox"):
            c = profile.BoundBox.Center
        else:
            c = FreeCAD.Vector(0,0,0)
        rprofile = profile.copy().rotate(c,r.Axis,math.degrees(r.Angle))
        return rprofile

    def getNormalEdges(self,edges,reference):

        """returns a list of edges normal to the given reference"""

        result = []
        tolerance = 0.67 # we try to get all edges with angle > 45deg
        for edge in edges:
            if len(edge.Vertexes) > 1:
                v = edge.Vertexes[-1].Point.sub(edge.Vertexes[0].Point)
                a = v.getAngle(reference)
                if (a <= math.pi/2+tolerance) and (a >= math.pi/2-tolerance):
                    result.append(edge)
        return result

    def getProjectedLength(self,v,ref):

        """gets a signed length from projecting a vector on another"""

        proj = DraftVecUtils.project(v,ref)
        if proj.getAngle(ref) < 1:
            return proj.Length
        else:
            return -proj.Length


class ViewProviderCurtainWall(ArchComponent.ViewProviderComponent):


    "A View Provider for the CurtainWall object"

    def __init__(self,vobj):

        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_CurtainWall_Tree.svg"


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_CurtainWall',CommandArchCurtainWall())
