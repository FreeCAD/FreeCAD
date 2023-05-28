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

__title__  = "FreeCAD Arch Curtain Wall"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

import math
import FreeCAD
import ArchComponent
import ArchCommands
import DraftVecUtils

if FreeCAD.GuiUp:
    import FreeCADGui
    from draftutils.translate import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

ANGLETOLERANCE = 0.67 # vectors with angles below this are considered going in same dir

## @package ArchCurtainWall
#  \ingroup ARCH
#  \brief The Curtain Wall object and tools
#
#  This module provides tools to build Curtain wall objects.

"""
Curtain wall tool

Abstract: Curtain walls need a surface to work on (base).
They then divide each face of that surface into quads,
using the face parameters grid.

The vertical lines can then receive one type of profile
(vertical mullions), the horizontal ones another
(horizontal mullions), and the quads a third (panels).

We then have two cases, depending on each quad: Either the
four corners of each quad are coplanar, in which case the
panel filling is rectangular, or they don't, in which case
the facet is triangulated and receives a third mullion
(diagonal mullion).
"""


def makeCurtainWall(baseobj=None,name=None):

    """
    makeCurtainWall([baseobj],[name]): Creates a curtain wall in the active document
    """

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","CurtainWall")
    obj.Label = name if name else translate("Arch","Curtain Wall")
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
            FreeCADGui.doCommand("base = Draft.makeLine(FreeCAD."+str(self.points[0])+",FreeCAD."+str(self.points[1])+")")
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
        vsize = 50
        hsize = 50
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        if not "Host" in pl:
            obj.addProperty("App::PropertyLink","Host","CurtainWall",QT_TRANSLATE_NOOP("App::Property","An optional host object for this curtain wall"))
        if not "Height" in pl:
            obj.addProperty("App::PropertyLength","Height","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The height of the curtain wall, if based on an edge"))
            obj.Height = p.GetFloat("WallHeight",3000)
        if not "VerticalMullionNumber" in pl:
            obj.addProperty("App::PropertyInteger","VerticalMullionNumber","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of vertical mullions"))
            obj.setEditorMode("VerticalMullionNumber",1)
        if not "VerticalMullionAlignment" in pl:
            obj.addProperty("App::PropertyBool","VerticalMullionAlignment","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","If the profile of the vertical mullions get aligned with the surface or not"))
        if not "VerticalSections" in pl:
            obj.addProperty("App::PropertyInteger","VerticalSections","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of vertical sections of this curtain wall"))
            obj.VerticalSections = 1
        if "VerticalMullionSize" in pl:
            # obsolete
            vsize = obj.VerticalMullionSize.Value
            obj.removeProperty("VerticalMullionSize")
        if not "VerticalMullionHeight" in pl:
            obj.addProperty("App::PropertyLength","VerticalMullionHeight","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The height of the vertical mullions profile, if no profile is used"))
            obj.VerticalMullionHeight = vsize
        if not "VerticalMullionWidth" in pl:
            obj.addProperty("App::PropertyLength","VerticalMullionWidth","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The width of the vertical mullions profile, if no profile is used"))
            obj.VerticalMullionWidth = vsize
        if not "VerticalMullionProfile" in pl:
            obj.addProperty("App::PropertyLink","VerticalMullionProfile","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","A profile for vertical mullions (disables vertical mullion size)"))
        if not "HorizontalMullionNumber" in pl:
            obj.addProperty("App::PropertyInteger","HorizontalMullionNumber","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of horizontal mullions"))
            obj.setEditorMode("HorizontalMullionNumber",1)
        if not "HorizontalMullionAlignment" in pl:
            obj.addProperty("App::PropertyBool","HorizontalMullionAlignment","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","If the profile of the horizontal mullions gets aligned with the surface or not"))
        if not "HorizontalSections" in pl:
            obj.addProperty("App::PropertyInteger","HorizontalSections","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of horizontal sections of this curtain wall"))
            obj.HorizontalSections = 1
        if "HorizontalMullionSize" in pl:
            # obsolete
            hsize = obj.HorizontalMullionSize.Value
            obj.removeProperty("HorizontalMullionSize")
        if not "HorizontalMullionHeight" in pl:
            obj.addProperty("App::PropertyLength","HorizontalMullionHeight","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The height of the horizontal mullions profile, if no profile is used"))
            obj.HorizontalMullionHeight = hsize
        if not "HorizontalMullionWidth" in pl:
            obj.addProperty("App::PropertyLength","HorizontalMullionWidth","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The width of the horizontal mullions profile, if no profile is used"))
            obj.HorizontalMullionWidth = hsize
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
        if not "Refine" in pl:
            obj.addProperty("App::PropertyBool","Refine","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","Perform subtractions between components so none overlap"))
        if not "CenterProfiles" in pl:
            obj.addProperty("App::PropertyBool","CenterProfiles","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","Centers the profile over the edges or not"))
            obj.CenterProfiles = True
        if not "VerticalDirection" in pl:
            obj.addProperty("App::PropertyVector","VerticalDirection","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The vertical direction reference to be used by this object to deduce vertical/horizontal directions. Keep it close to the actual vertical direction of your curtain wall"))
            obj.VerticalDirection = FreeCAD.Vector(0,0,1)

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

        facets = []

        faces = []
        if obj.Base.Shape.Faces:
            faces = obj.Base.Shape.Faces
        elif obj.Height.Value and obj.VerticalDirection.Length:
            ext = FreeCAD.Vector(obj.VerticalDirection)
            ext.normalize()
            ext = ext.multiply(obj.Height.Value)
            faces = [edge.extrude(ext) for edge in obj.Base.Shape.Edges]
        if not faces:
            FreeCAD.Console.PrintLog(obj.Label+": unable to build base faces\n")
            return

        # subdivide the faces into quads
        for face in faces:

            fp = face.ParameterRange

            # guessing horizontal/vertical directions
            vdir = obj.VerticalDirection
            if not vdir.Length:
                vdir = FreeCAD.Vector(0,0,1)
            vdir.normalize()
            basevector = face.valueAt(fp[1],fp[3]).sub(face.valueAt(fp[0],fp[2]))
            a = basevector.getAngle(vdir)
            if (a <= math.pi/2+ANGLETOLERANCE) and (a >= math.pi/2-ANGLETOLERANCE):
                facedir = True
                vertsec = obj.VerticalSections
                horizsec = obj.HorizontalSections
            else:
                facedir = False
                vertsec = obj.HorizontalSections
                horizsec = obj.VerticalSections

            hstep = (fp[1]-fp[0])
            if vertsec:
                hstep = hstep/vertsec
            vstep = (fp[3]-fp[2])
            if horizsec:
                vstep = vstep/horizsec

            # construct facets
            for i in range(vertsec or 1):
                for j in range(horizsec or 1):
                    p0 = face.valueAt(fp[0]+i*hstep,fp[2]+j*vstep)
                    p1 = face.valueAt(fp[0]+(i+1)*hstep,fp[2]+j*vstep)
                    p2 = face.valueAt(fp[0]+(i+1)*hstep,fp[2]+(j+1)*vstep)
                    p3 = face.valueAt(fp[0]+i*hstep,fp[2]+(j+1)*vstep)
                    facet = Part.Face(Part.makePolygon([p0,p1,p2,p3,p0]))
                    facets.append(facet)

        if not facets:
            FreeCAD.Console.PrintLog(obj.Label+": failed to subdivide shape\n")
            return

        baseshape = Part.makeShell(facets)

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
                n = faces[0].normalAt(0,0).add(faces[1].normalAt(0,0))
                if n.Length > 0.001:
                    n.normalize()
                else:
                    # adjacent faces have same normals
                    n = faces[0].normalAt(0,0)
                self.edgenormals[ec] = n

        # sort edges between vertical/horizontal
        hedges = []
        vedges = []
        for edge in baseshape.Edges:
            v = edge.Vertexes[-1].Point.sub(edge.Vertexes[0].Point)
            a = v.getAngle(vdir)
            if (a <= math.pi/2+ANGLETOLERANCE) and (a >= math.pi/2-ANGLETOLERANCE):
                hedges.append(edge)
            else:
                vedges.append(edge)

        # construct vertical mullions
        vmullions = []
        vprofile = self.getMullionProfile(obj,"Vertical")
        if vprofile and vertsec:
            for vedge in vedges:
                vn = self.edgenormals[vedge.hashCode()]
                if (vn.x != 0) or (vn.y != 0):
                    avn = FreeCAD.Vector(vn.x,vn.y,0)
                    rot = FreeCAD.Rotation(FreeCAD.Vector(0,-1,0),avn)
                else:
                    rot = FreeCAD.Rotation()
                if obj.VerticalMullionAlignment:
                    ev = vedge.Vertexes[-1].Point.sub(vedge.Vertexes[0].Point)
                    rot = FreeCAD.Rotation(FreeCAD.Vector(1,0,0),ev).multiply(rot)
                vmullions.append(self.makeMullion(vedge,vprofile,rot,obj.CenterProfiles))

        # construct horizontal mullions
        hmullions = []
        hprofile = self.getMullionProfile(obj,"Horizontal")
        if hprofile and horizsec:
            for hedge in hedges:
                rot = FreeCAD.Rotation(FreeCAD.Vector(0,1,0),-90)
                vn = self.edgenormals[hedge.hashCode()]
                if (vn.x != 0) or (vn.y != 0):
                    avn = FreeCAD.Vector(vn.x,vn.y,0)
                    rot = FreeCAD.Rotation(FreeCAD.Vector(0,-1,0),avn).multiply(rot)
                    if obj.HorizontalMullionAlignment:
                        rot = FreeCAD.Rotation(avn,vn).multiply(rot)
                hmullions.append(self.makeMullion(hedge,hprofile,rot,obj.CenterProfiles))

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
                for dedge in dedges:
                    rot = FreeCAD.Rotation(FreeCAD.Vector(0,0,1),dedge.Vertexes[-1].Point.sub(dedge.Vertexes[0].Point))
                    dmullions.append(self.makeMullion(dedge,dprofile,rot,obj.CenterProfiles))

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
        obj.VerticalMullionNumber = len(vmullions)
        obj.HorizontalMullionNumber = len(hmullions)
        obj.DiagonalMullionNumber = len(dmullions)
        obj.PanelNumber = len(panels)
        shape = Part.makeCompound(vmullions+hmullions+dmullions+panels)
        shape = self.processSubShapes(obj,shape,pl)
        self.applyShape(obj,shape,pl)

    def makePanel(self,verts,thickness):

        """creates a panel from face points and thickness"""

        import Part

        panel = Part.Face(Part.makePolygon(verts+[verts[0]]))
        n = panel.normalAt(0,0)
        n.multiply(thickness)
        panel = panel.extrude(n)
        return panel

    def makeMullion(self,edge,profile,rotation,recenter=False):

        """creates a mullions from an edge and a profile"""

        center = FreeCAD.Vector(0,0,0)
        if recenter:
            if hasattr(profile,"CenterOfMass"):
                center = profile.CenterOfMass
            elif hasattr(profile,"BoundBox"):
                center = profile.BoundBox.Center
        p0 = edge.Vertexes[0].Point
        p1 = edge.Vertexes[-1].Point
        mullion = profile.copy()
        if rotation:
            mullion = mullion.rotate(center,rotation.Axis,math.degrees(rotation.Angle))
        mullion = mullion.translate(p0.sub(center))
        mullion = mullion.extrude(p1.sub(p0))
        return mullion

    def getMullionProfile(self,obj,direction):

        """returns a profile shape already properly oriented, ready for extrude"""

        import Part,DraftGeomUtils

        prof = getattr(obj,direction+"MullionProfile")
        proh = getattr(obj,direction+"MullionHeight").Value
        prow = getattr(obj,direction+"MullionWidth").Value
        if prof:
            profile = prof.Shape.copy()
        else:
            if (not proh) or (not prow):
                return None
            profile = Part.Face(Part.makePlane(prow,proh,FreeCAD.Vector(-prow/2,-proh/2,0)))
        return profile

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

    def updateData(self,obj,prop):

        if prop == "Shape":
            self.colorize(obj,force=True)

    def onChanged(self,vobj,prop):

        if (prop in ["DiffuseColor","Transparency"]) and vobj.Object:
            self.colorize(vobj.Object)
        elif prop == "ShapeColor":
            self.colorize(vobj.Object,force=True)
        ArchComponent.ViewProviderComponent.onChanged(self,vobj,prop)

    def colorize(self,obj,force=False):

        "setting different part colors"

        if not obj.Shape or not obj.Shape.Solids:
            return
        basecolor = obj.ViewObject.ShapeColor
        basetransparency = obj.ViewObject.Transparency/100.0
        panelcolor = ArchCommands.getDefaultColor("WindowGlass")
        paneltransparency = 0.7
        if hasattr(obj,"Material") and obj.Material and hasattr(obj.Material,"Materials"):
            if obj.Material.Names:
                if "Frame" in obj.Material.Names:
                    mat = obj.Material.Materials[obj.Material.Names.index("Frame")]
                    if ('DiffuseColor' in mat.Material) and ("(" in mat.Material['DiffuseColor']):
                        basecolor = tuple([float(f) for f in mat.Material['DiffuseColor'].strip("()").split(",")])
                if "Glass panel" in obj.Material.Names:
                    mat = obj.Material.Materials[obj.Material.Names.index("Glass panel")]
                    if ('DiffuseColor' in mat.Material) and ("(" in mat.Material['DiffuseColor']):
                        panelcolor = tuple([float(f) for f in mat.Material['DiffuseColor'].strip("()").split(",")])
                    if ('Transparency' in mat.Material):
                        paneltransparency = float(mat.Material['Transparency'])/100.0
                elif "Solid panel" in obj.Material.Names:
                    mat = obj.Material.Materials[obj.Material.Names.index("Solid panel")]
                    if ('DiffuseColor' in mat.Material) and ("(" in mat.Material['DiffuseColor']):
                        panelcolor = tuple([float(f) for f in mat.Material['DiffuseColor'].strip("()").split(",")])
                    paneltransparency = 0
        basecolor = basecolor[:3]+(basetransparency,)
        panelcolor = panelcolor[:3]+(paneltransparency,)
        colors = []
        nmullions = obj.VerticalMullionNumber + obj.HorizontalMullionNumber + obj.DiagonalMullionNumber
        for i,solid in enumerate(obj.Shape.Solids):
            for _ in solid.Faces:
                if i < nmullions:
                    colors.append(basecolor)
                else:
                    colors.append(panelcolor)
        if self.areDifferentColors(colors,obj.ViewObject.DiffuseColor) or force:
            obj.ViewObject.DiffuseColor = colors



if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_CurtainWall',CommandArchCurtainWall())
