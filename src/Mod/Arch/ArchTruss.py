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

__title__  = "FreeCAD Arch Truss"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

import math
import FreeCAD
import ArchComponent

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

## @package ArchTruss
#  \ingroup ARCH
#  \brief The Truss object and tools
#
#  This module provides tools to build Truss objects.

rodmodes = ("/|/|/|",
            "/\\/\\/\\",
            "/|\\|/|\\",
           )

def makeTruss(baseobj=None,name=None):

    """
    makeTruss([baseobj],[name]): Creates a space object from the given object (a line)
    """

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Truss")
    obj.Label = name if name else translate("Arch","Truss")
    Truss(obj)
    if FreeCAD.GuiUp:
        ViewProviderTruss(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
        if FreeCAD.GuiUp:
            baseobj.ViewObject.hide()
    return obj


class CommandArchTruss:

    "the Arch Truss command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Truss',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Truss","Truss"),
                'Accel': "T, U",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Truss","Creates a truss object from selected line or from scratch")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        sel = FreeCADGui.Selection.getSelection()
        if len(sel) > 1:
            FreeCAD.Console.PrintError(translate("Arch","Please select only one base object or none")+"\n")
        elif len(sel) == 1:
            # build on selection
            FreeCADGui.Control.closeDialog()
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Truss"))
            FreeCADGui.addModule("Draft")
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("obj = Arch.makeTruss(FreeCAD.ActiveDocument."+FreeCADGui.Selection.getSelection()[0].Name+")")
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
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Truss"))
            FreeCADGui.addModule("Draft")
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("base = Draft.makeLine(FreeCAD."+str(self.points[0])+",FreeCAD."+str(self.points[1])+")")
            FreeCADGui.doCommand("obj = Arch.makeTruss(base)")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()


class Truss(ArchComponent.Component):

    "The truss object"

    def __init__(self,obj):

        ArchComponent.Component.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Beam"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "TrussAngle" in pl:
            obj.addProperty("App::PropertyAngle","TrussAngle","Truss",
                            QT_TRANSLATE_NOOP("App::Property","The angle of the truss"))
            obj.setEditorMode("TrussAngle",1)
        if not "SlantType" in pl:
            obj.addProperty("App::PropertyEnumeration","SlantType","Truss",
                            QT_TRANSLATE_NOOP("App::Property","The slant type of this truss"))
            obj.SlantType = ["Simple","Double"]
        if not "Normal" in pl:
            obj.addProperty("App::PropertyVector","Normal","Truss",
                            QT_TRANSLATE_NOOP("App::Property","The normal direction of this truss"))
            obj.Normal = FreeCAD.Vector(0,0,1)
        if not "HeightStart" in pl:
            obj.addProperty("App::PropertyLength","HeightStart","Truss",
                            QT_TRANSLATE_NOOP("App::Property","The height of the truss at the start position"))
            obj.HeightStart = 100
        if not "HeightEnd" in pl:
            obj.addProperty("App::PropertyLength","HeightEnd","Truss",
                            QT_TRANSLATE_NOOP("App::Property","The height of the truss at the end position"))
            obj.HeightEnd = 200
        if not "StrutStartOffset" in pl:
            obj.addProperty("App::PropertyDistance","StrutStartOffset","Truss",
                            QT_TRANSLATE_NOOP("App::Property","An optional start offset for the top strut"))
        if not "StrutEndOffset" in pl:
            obj.addProperty("App::PropertyDistance","StrutEndOffset","Truss",
                            QT_TRANSLATE_NOOP("App::Property","An optional end offset for the top strut"))
        if not "StrutHeight" in pl:
            obj.addProperty("App::PropertyLength","StrutHeight","Truss",
                            QT_TRANSLATE_NOOP("App::Property","The height of the main top and bottom elements of the truss"))
            obj.StrutHeight = 10
        if not "StrutWidth" in pl:
            obj.addProperty("App::PropertyLength","StrutWidth","Truss",
                            QT_TRANSLATE_NOOP("App::Property","The width of the main top and bottom elements of the truss"))
            obj.StrutWidth = 5
        if not "RodType" in pl:
            obj.addProperty("App::PropertyEnumeration","RodType","Truss",
                            QT_TRANSLATE_NOOP("App::Property","The type of the middle element of the truss"))
            obj.RodType = ["Round","Square"]
        if not "RodDirection" in pl:
            obj.addProperty("App::PropertyEnumeration","RodDirection","Truss",
                            QT_TRANSLATE_NOOP("App::Property","The direction of the rods"))
            obj.RodDirection = ["Forward","Backward"]
        if not "RodSize" in pl:
            obj.addProperty("App::PropertyLength","RodSize","Truss",
                            QT_TRANSLATE_NOOP("App::Property","The diameter or side of the rods"))
            obj.RodSize = 2
        if not "RodSections" in pl:
            obj.addProperty("App::PropertyInteger","RodSections","Truss",
                            QT_TRANSLATE_NOOP("App::Property","The number of rod sections"))
            obj.RodSections = 3
        if not "RodEnd" in pl:
            obj.addProperty("App::PropertyBool","RodEnd","Truss",
                            QT_TRANSLATE_NOOP("App::Property","If the truss has a rod at its endpoint or not"))
        if not "RodMode" in pl:
            obj.addProperty("App::PropertyEnumeration","RodMode","Truss",
                            QT_TRANSLATE_NOOP("App::Property","How to draw the rods"))
            obj.RodMode = rodmodes
        self.Type = "Truss"

    def onDocumentRestored(self,obj):

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def onChanged(self,obj,prop):

        ArchComponent.Component.onChanged(self,obj,prop)

    def execute(self,obj):

        if self.clone(obj):
            return

        import Part

        pl = obj.Placement

        # test properties
        if not obj.StrutWidth.Value or not obj.StrutHeight.Value:
            FreeCAD.Console.PrintLog(obj.Label+": Invalid strut size\n")
            return
        if not obj.HeightStart.Value or not obj.HeightEnd.Value:
            FreeCAD.Console.PrintLog(obj.Label+": Invalid truss heights\n")
            return
        if not obj.RodSections or not obj.RodSize.Value:
            FreeCAD.Console.PrintLog(obj.Label+": Invalid rod config\n")
            return
        if not obj.Base:
            FreeCAD.Console.PrintLog(obj.Label+": No base\n")
            return
        if (not hasattr(obj.Base,"Shape")) or (len(obj.Base.Shape.Vertexes) > 2):
            FreeCAD.Console.PrintLog(obj.Label+": No base edge\n")
            return

        # baseline
        v0 = obj.Base.Shape.Vertexes[0].Point
        v1 = obj.Base.Shape.Vertexes[-1].Point

        if obj.SlantType == "Simple":
            topstrut,bottomstrut,rods,angle = self.makeTruss(obj,v0,v1)
        else:
            v3 = v0.add((v1.sub(v0)).multiply(0.5))
            topstrut1,bottomstrut1,rods1,angle = self.makeTruss(obj,v0,v3)
            topstrut2,bottomstrut2,rods2,angle = self.makeTruss(obj,v1,v3)
            topstrut = topstrut1.fuse(topstrut2)
            topstrut = topstrut.removeSplitter()
            bottomstrut = bottomstrut1.fuse(bottomstrut2)
            bottomstrut = bottomstrut.removeSplitter()
            if obj.RodEnd:
                rods2 = rods2[:-1]
            rods = rods1 + rods2

        # mount shape
        shape = Part.makeCompound([topstrut,bottomstrut]+rods)
        shape = self.processSubShapes(obj,shape,pl)
        self.applyShape(obj,shape,pl)
        obj.TrussAngle = angle

    def makeTruss(self,obj,v0,v1):

        import Part

        # get normal direction
        normal = obj.Normal
        if not normal.Length:
            normal = FreeCAD.Vector(0,0,1)

        # create base profile
        maindir = v1.sub(v0)
        sidedir = normal.cross(maindir)
        if not sidedir.Length:
            FreeCAD.Console.PrintLog(obj.Label+": normal and base are parallel\n")
            return
        sidedir.normalize()
        p0 = v0.add(FreeCAD.Vector(sidedir).negative().multiply(obj.StrutWidth.Value/2))
        p1 = p0.add(FreeCAD.Vector(sidedir).multiply(obj.StrutWidth.Value/2).multiply(2))
        p2 = p1.add(FreeCAD.Vector(normal).multiply(obj.StrutHeight))
        p3 = p0.add(FreeCAD.Vector(normal).multiply(obj.StrutHeight))
        trussprofile = Part.Face(Part.makePolygon([p0,p1,p2,p3,p0]))

        # create bottom strut
        bottomstrut = trussprofile.extrude(maindir)

        # create top strut
        v2 = v0.add(FreeCAD.Vector(normal).multiply(obj.HeightStart.Value))
        v3 = v1.add(FreeCAD.Vector(normal).multiply(obj.HeightEnd.Value))
        topdir = v3.sub(v2)
        if obj.StrutStartOffset.Value:
            v2f = v2.add((v2.sub(v3)).normalize().multiply(obj.StrutStartOffset.Value))
        else:
            v2f = v2
        if obj.StrutEndOffset.Value:
            v3f = v3.add((v3.sub(v2)).normalize().multiply(obj.StrutEndOffset.Value))
        else:
            v3f = v3
        offtopdir = v3f.sub(v2f)
        topstrut = trussprofile.extrude(offtopdir)
        topstrut.translate(v2f.sub(v0))
        topstrut.translate(FreeCAD.Vector(normal).multiply(-obj.StrutHeight.Value))
        angle = math.degrees(topdir.getAngle(maindir))

        # create rod profile on the XY plane
        if obj.RodType == "Round":
            rodprofile = Part.Face(Part.Wire([Part.makeCircle(obj.RodSize/2)]))
        else:
            rodprofile = Part.Face(Part.makePlane(obj.RodSize,obj.RodSize,FreeCAD.Vector(-obj.RodSize/2,-obj.RodSize/2,0)))

        # create rods
        rods = []
        bottomrodstart = v0.add(FreeCAD.Vector(normal).multiply(obj.StrutHeight.Value/2))
        toprodstart = v2.add(FreeCAD.Vector(normal).multiply(obj.StrutHeight.Value/2).negative())
        bottomrodvec = FreeCAD.Vector(maindir).multiply(1/obj.RodSections)
        toprodvec = FreeCAD.Vector(topdir).multiply(1/obj.RodSections)
        bottomrodpos = [bottomrodstart]
        toprodpos = [toprodstart]

        if obj.RodMode == rodmodes[0]:
            # /|/|/|
            for i in range(1,obj.RodSections+1):
                if (i > 1) or (obj.StrutStartOffset.Value >= 0):
                    # do not add first vert rod if negative offset
                    rods.append(self.makeRod(rodprofile,bottomrodpos[-1],toprodpos[-1]))
                bottomrodpos.append(bottomrodpos[-1].add(bottomrodvec))
                toprodpos.append(toprodpos[-1].add(toprodvec))
                if obj.RodDirection == "Forward":
                    rods.append(self.makeRod(rodprofile,bottomrodpos[-2],toprodpos[-1]))
                else:
                    rods.append(self.makeRod(rodprofile,bottomrodpos[-1],toprodpos[-2]))

        elif obj.RodMode == rodmodes[1]:
            # /\/\/\
            fw = True
            for i in range(1,obj.RodSections+1):
                bottomrodpos.append(bottomrodpos[-1].add(bottomrodvec))
                toprodpos.append(toprodpos[-1].add(toprodvec))
                if obj.RodDirection == "Forward":
                    if fw:
                        rods.append(self.makeRod(rodprofile,bottomrodpos[-2],toprodpos[-1]))
                    else:
                        rods.append(self.makeRod(rodprofile,bottomrodpos[-1],toprodpos[-2]))
                else:
                    if fw:
                        rods.append(self.makeRod(rodprofile,bottomrodpos[-1],toprodpos[-2]))
                    else:
                        rods.append(self.makeRod(rodprofile,bottomrodpos[-2],toprodpos[-1]))
                fw = not fw

        elif obj.RodMode == rodmodes[2]:
            # /|\|/|\
            fw = True
            for i in range(1,obj.RodSections+1):
                if (i > 1) or (obj.StrutStartOffset.Value >= 0):
                    # do not add first vert rod if negative offset
                    rods.append(self.makeRod(rodprofile,bottomrodpos[-1],toprodpos[-1]))
                bottomrodpos.append(bottomrodpos[-1].add(bottomrodvec))
                toprodpos.append(toprodpos[-1].add(toprodvec))
                if obj.RodDirection == "Forward":
                    if fw:
                        rods.append(self.makeRod(rodprofile,bottomrodpos[-2],toprodpos[-1]))
                    else:
                        rods.append(self.makeRod(rodprofile,bottomrodpos[-1],toprodpos[-2]))
                else:
                    if fw:
                        rods.append(self.makeRod(rodprofile,bottomrodpos[-1],toprodpos[-2]))
                    else:
                        rods.append(self.makeRod(rodprofile,bottomrodpos[-2],toprodpos[-1]))
                fw = not fw

        # add end rod
        if obj.RodEnd:
            rods.append(self.makeRod(rodprofile,bottomrodpos[-1],toprodpos[-1]))

        # trim rods
        rods = [rod.cut(topstrut).cut(bottomstrut) for rod in rods]

        return topstrut,bottomstrut,rods,angle

    def makeRod(self,profile,p1,p2):

        """makes a rod by extruding profile between p1 and p2"""

        rot = FreeCAD.Rotation(FreeCAD.Vector(0,0,1),p2.sub(p1))
        rod = profile.copy().translate(p1)
        rod = rod.rotate(p1,rot.Axis,math.degrees(rot.Angle))
        rod = rod.extrude(p2.sub(p1))
        return rod


class ViewProviderTruss(ArchComponent.ViewProviderComponent):


    "A View Provider for the Truss object"

    def __init__(self,vobj):

        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_Truss_Tree.svg"


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Truss',CommandArchTruss())
