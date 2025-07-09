# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__  = "FreeCAD Arch Curtain Wall"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

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

import math

import FreeCAD
import ArchCommands
import ArchComponent
import DraftVecUtils

from draftutils import params

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    from draftutils.translate import translate
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

ANGLETOLERANCE = 0.67 # vectors with angles below this are considered going in same dir


class CurtainWall(ArchComponent.Component):


    "The curtain wall object"

    def __init__(self,obj):

        ArchComponent.Component.__init__(self,obj)
        self.Type = "CurtainWall"
        self.setProperties(obj)
        obj.IfcType = "Curtain Wall"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        vsize = 50
        hsize = 50
        if not "Host" in pl:
            obj.addProperty("App::PropertyLink","Host","CurtainWall",QT_TRANSLATE_NOOP("App::Property","An optional host object for this curtain wall"), locked=True)
        if not "Height" in pl:
            obj.addProperty("App::PropertyLength","Height","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The height of the curtain wall, if based on an edge"), locked=True)
            obj.Height = params.get_param_arch("WallHeight")
        if not "VerticalMullionNumber" in pl:
            obj.addProperty("App::PropertyInteger","VerticalMullionNumber","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of vertical mullions"), locked=True)
            obj.setEditorMode("VerticalMullionNumber",1)
        if not "VerticalMullionAlignment" in pl:
            obj.addProperty("App::PropertyBool","VerticalMullionAlignment","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","If the profile of the vertical mullions get aligned with the surface or not"), locked=True)
        if not "VerticalSections" in pl:
            obj.addProperty("App::PropertyInteger","VerticalSections","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of vertical sections of this curtain wall"), locked=True)
            obj.VerticalSections = 1
        if "VerticalMullionSize" in pl:
            # obsolete
            vsize = obj.VerticalMullionSize.Value
            obj.removeProperty("VerticalMullionSize")
        if not "VerticalMullionHeight" in pl:
            obj.addProperty("App::PropertyLength","VerticalMullionHeight","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The height of the vertical mullions profile, if no profile is used"), locked=True)
            obj.VerticalMullionHeight = vsize
        if not "VerticalMullionWidth" in pl:
            obj.addProperty("App::PropertyLength","VerticalMullionWidth","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The width of the vertical mullions profile, if no profile is used"), locked=True)
            obj.VerticalMullionWidth = vsize
        if not "VerticalMullionProfile" in pl:
            obj.addProperty("App::PropertyLink","VerticalMullionProfile","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","A profile for vertical mullions (disables vertical mullion size)"), locked=True)
        if not "HorizontalMullionNumber" in pl:
            obj.addProperty("App::PropertyInteger","HorizontalMullionNumber","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of horizontal mullions"), locked=True)
            obj.setEditorMode("HorizontalMullionNumber",1)
        if not "HorizontalMullionAlignment" in pl:
            obj.addProperty("App::PropertyBool","HorizontalMullionAlignment","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","If the profile of the horizontal mullions gets aligned with the surface or not"), locked=True)
        if not "HorizontalSections" in pl:
            obj.addProperty("App::PropertyInteger","HorizontalSections","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of horizontal sections of this curtain wall"), locked=True)
            obj.HorizontalSections = 1
        if "HorizontalMullionSize" in pl:
            # obsolete
            hsize = obj.HorizontalMullionSize.Value
            obj.removeProperty("HorizontalMullionSize")
        if not "HorizontalMullionHeight" in pl:
            obj.addProperty("App::PropertyLength","HorizontalMullionHeight","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The height of the horizontal mullions profile, if no profile is used"), locked=True)
            obj.HorizontalMullionHeight = hsize
        if not "HorizontalMullionWidth" in pl:
            obj.addProperty("App::PropertyLength","HorizontalMullionWidth","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The width of the horizontal mullions profile, if no profile is used"), locked=True)
            obj.HorizontalMullionWidth = hsize
        if not "HorizontalMullionProfile" in pl:
            obj.addProperty("App::PropertyLink","HorizontalMullionProfile","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","A profile for horizontal mullions (disables horizontal mullion size)"), locked=True)
        if not "DiagonalMullionNumber" in pl:
            obj.addProperty("App::PropertyInteger","DiagonalMullionNumber","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of diagonal mullions"), locked=True)
            obj.setEditorMode("DiagonalMullionNumber",1)
        if not "DiagonalMullionSize" in pl:
            obj.addProperty("App::PropertyLength","DiagonalMullionSize","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The size of the diagonal mullions, if any, if no profile is used"), locked=True)
            obj.DiagonalMullionSize = 50
        if not "DiagonalMullionProfile" in pl:
            obj.addProperty("App::PropertyLink","DiagonalMullionProfile","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","A profile for diagonal mullions, if any (disables horizontal mullion size)"), locked=True)
        if not "PanelNumber" in pl:
            obj.addProperty("App::PropertyInteger","PanelNumber","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The number of panels"), locked=True)
            obj.setEditorMode("PanelNumber",1)
        if not "PanelThickness" in pl:
            obj.addProperty("App::PropertyLength","PanelThickness","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The thickness of the panels"), locked=True)
            obj.PanelThickness = 20
        if not "SwapHorizontalVertical" in pl:
            obj.addProperty("App::PropertyBool","SwapHorizontalVertical","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","Swaps horizontal and vertical lines"), locked=True)
        if not "Refine" in pl:
            obj.addProperty("App::PropertyBool","Refine","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","Perform subtractions between components so none overlap"), locked=True)
        if not "CenterProfiles" in pl:
            obj.addProperty("App::PropertyBool","CenterProfiles","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","Centers the profile over the edges or not"), locked=True)
            obj.CenterProfiles = True
        if not "VerticalDirection" in pl:
            obj.addProperty("App::PropertyVector","VerticalDirection","CurtainWall",
                            QT_TRANSLATE_NOOP("App::Property","The vertical direction reference to be used by this object to deduce vertical/horizontal directions. Keep it close to the actual vertical direction of your curtain wall"), locked=True)
            obj.VerticalDirection = FreeCAD.Vector(0,0,1)
        if not "OverrideEdges" in pl:  # PropertyStringList
            obj.addProperty("App::PropertyStringList","OverrideEdges","CurtainWall",QT_TRANSLATE_NOOP("App::Property","Input are index numbers of edges of Base ArchSketch/Sketch geometries (in Edit mode).  Selected edges are used to create the shape of this Arch Curtain Wall (instead of using all edges by default).  [ENHANCED by ArchSketch] GUI 'Edit Curtain Wall' Tool is provided in external Add-on ('SketchArch') to let users to select the edges interactively.  'Toponaming-Tolerant' if ArchSketch is used in Base (and SketchArch Add-on is installed).  Warning : Not 'Toponaming-Tolerant' if just Sketch is used. Property is ignored if Base ArchSketch provided the selected edges."), locked=True)

    def onDocumentRestored(self,obj):

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def loads(self,state):

        self.Type = "CurtainWall"

    def onChanged(self,obj,prop):

        ArchComponent.Component.onChanged(self,obj,prop)

    def execute(self,obj):

        if self.clone(obj):
            return
        if not self.ensureBase(obj):
            return

        import Part
        import DraftGeomUtils

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

        curtainWallBaseShapeEdges = None
        curtainWallEdges = None
        if obj.Base.Shape.Faces:
            faces = obj.Base.Shape.Faces
        elif obj.Height.Value and obj.VerticalDirection.Length:
            ext = FreeCAD.Vector(obj.VerticalDirection)
            ext.normalize()
            ext = ext.multiply(obj.Height.Value)
            if hasattr(obj.Base, 'Proxy'):
                if hasattr(obj.Base.Proxy, 'getCurtainWallBaseShapeEdgesInfo'):
                    curtainWallBaseShapeEdges = obj.Base.Proxy.getCurtainWallBaseShapeEdgesInfo(obj.Base)
                    # returned a {dict}
            # get curtain wall edges (not wires); use original edges if getCurtainWallBaseShapeEdges() provided none
            if curtainWallBaseShapeEdges:  # would be false (none) if SketchArch Add-on is not installed, or base ArchSketch does not have the edges stored / input by user
                curtainWallEdges = curtainWallBaseShapeEdges.get('curtainWallEdges')
            elif obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                skGeom = obj.Base.GeometryFacadeList
                skGeomEdges = []
                skPlacement = obj.Base.Placement  # Get Sketch's placement to restore later
                for ig, geom  in enumerate(skGeom):
                    if (not obj.OverrideEdges and not geom.Construction) or str(ig) in obj.OverrideEdges:
                        # support Line, Arc, Circle, Ellipse for Sketch
                        # as Base at the moment
                        if isinstance(geom.Geometry, (Part.LineSegment,
                                      Part.Circle, Part.ArcOfCircle)):
                            skGeomEdgesI = geom.Geometry.toShape()
                            skGeomEdges.append(skGeomEdgesI)
                curtainWallEdges = []
                for edge in skGeomEdges:
                    edge.Placement = edge.Placement.multiply(skPlacement)
                    curtainWallEdges.append(edge)
            if not curtainWallEdges:
                curtainWallEdges = obj.Base.Shape.Edges
            if curtainWallEdges:
                faces = [edge.extrude(ext) for edge in curtainWallEdges]
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

            # Check if face if vertical in the first place
            # Fix issue in 'Curtain wall vertical/horizontal mullion mix-up'
            # https://github.com/FreeCAD/FreeCAD/issues/21845
            #
            face_plane = face.findPlane()  # Curve face (surface) seems return no Plane
            if face_plane:
                if -0.001 < face_plane.Axis[2] < 0.001:  # i.e. face is vertical (normal pointing horizon)
                    faceVert = True
                    # Support 'Swap Horizontal Vertical'
                    # See issue 'Swap Horizontal Vertical does not work'
                    # https://github.com/FreeCAD/FreeCAD/issues/21866
                    if obj.SwapHorizontalVertical:
                        vertsec = obj.HorizontalSections
                        horizsec = obj.VerticalSections
                    else:
                        vertsec = obj.VerticalSections
                        horizsec = obj.HorizontalSections
                else:
                    faceVert = False

            # Guess algorithm if face is not vertical
            if not faceVert:
                # TODO 2025.6.15 : Need a more robust algorithm below
                # See issue 'Curtain wall vertical/horizontal mullion mix-up'
                # https://github.com/FreeCAD/FreeCAD/issues/21845
                # Partially improved by checking 'if face is vertical' above
                #
                basevector = face.valueAt(fp[1],fp[3]).sub(face.valueAt(fp[0],fp[2]))
                bv_angle = basevector.getAngle(vdir)
                if (bv_angle <= math.pi/2+ANGLETOLERANCE) and (bv_angle >= math.pi/2-ANGLETOLERANCE):
                    facedir = True
                    if obj.SwapHorizontalVertical:
                        vertsec = obj.HorizontalSections
                        horizsec = obj.VerticalSections
                    else:
                        vertsec = obj.VerticalSections
                        horizsec = obj.HorizontalSections
                else:
                    facedir = False
                    if obj.SwapHorizontalVertical:
                        vertsec = obj.VerticalSections
                        horizsec = obj.HorizontalSections
                    else:
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
                if DraftGeomUtils.isPlanar(verts):
                    panel = self.makePanel(verts,obj.PanelThickness.Value)
                    panels.append(panel)
                elif len(verts) == 4:
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

        import Part
        import DraftGeomUtils

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
        if not obj.ViewObject:
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
        basecolor = basecolor[:3]+(1.0 - basetransparency,)
        panelcolor = panelcolor[:3]+(1.0 - paneltransparency,)
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
