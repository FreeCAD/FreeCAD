#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2012                                                    *
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

import FreeCAD, Draft, math, ArchComponent, DraftVecUtils
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

## @package ArchRoof
#  \ingroup ARCH
#  \brief The Roof object and tools
#
#  This module provides tools to build Roof objects.
#  Roofs are build from a closed contour and a series of
#  slopes.

__title__="FreeCAD Roof"
__author__ = "Yorik van Havre", "Jonathan Wiedemann"
__url__ = "http://www.freecadweb.org"


def makeRoof(baseobj=None,facenr=0, angles=[45.,], run = [], idrel = [0,],thickness = [50.,], overhang=[100.,], name="Roof"):

    '''makeRoof(baseobj,[facenr],[angle],[name]) : Makes a roof based on
    a closed wire or an object. You can provide a list of angles, run,
    idrel, thickness, overhang for each edges in the wire to define the
    roof shape. The default for angle is 45 and the list is
    automatically complete to match with number of edges in the wire.
    If the base object is a solid the roof take the shape.'''

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    import Part
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    obj.Label = translate("Arch",name)
    w = None
    _Roof(obj)
    if FreeCAD.GuiUp:
        _ViewProviderRoof(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
        if obj.Base.isDerivedFrom("Part::Feature"):
            if obj.Base.Shape.Solids:
                if FreeCAD.GuiUp:
                    obj.Base.ViewObject.hide()
            else:
                if (obj.Base.Shape.Faces and obj.Face):
                    w = obj.Base.Shape.Faces[obj.Face-1].Wires[0]
                    if FreeCAD.GuiUp:
                        obj.Base.ViewObject.hide()
                elif obj.Base.Shape.Wires:
                    w = obj.Base.Shape.Wires[0]
                    if FreeCAD.GuiUp:
                        obj.Base.ViewObject.hide()
        if w:
            if w.isClosed():
                if FreeCAD.GuiUp:
                    obj.Base.ViewObject.hide()
                edges = Part.__sortEdges__(w.Edges)
                l = len(edges)

                la = len(angles)
                alist = angles
                for i in range(l-la):
                    alist.append(angles[0])
                obj.Angles=alist

                lr = len(run)
                rlist = run
                for i in range(l-lr):
                    #rlist.append(w.Edges[i].Length/2.)
                    rlist.append(250.)
                obj.Runs = rlist

                lidrel = len(idrel)
                rellist = idrel
                for i in range(l-lidrel):
                    rellist.append(0)
                obj.IdRel = rellist

                lthick = len(thickness)
                tlist = thickness
                for i in range(l-lthick):
                    tlist.append(thickness[0])
                obj.Thickness = tlist

                lover = len(overhang)
                olist = overhang
                for i in range(l-lover):
                    olist.append(overhang[0])
                obj.Overhang = olist
    obj.Face = facenr
    return obj

class _CommandRoof:

    "the Arch Roof command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Roof',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Roof","Roof"),
                'Accel': "R, F",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Roof","Creates a roof object from the selected wire.")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        sel = FreeCADGui.Selection.getSelectionEx()
        if sel:
            sel = sel[0]
            obj = sel.Object
            FreeCADGui.Control.closeDialog()
            if sel.HasSubObjects:
                if "Face" in sel.SubElementNames[0]:
                    idx = int(sel.SubElementNames[0][4:])
                    FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Roof"))
                    FreeCADGui.addModule("Arch")
                    FreeCADGui.doCommand("obj = Arch.makeRoof(FreeCAD.ActiveDocument."+obj.Name+","+str(idx)+")")
                    FreeCADGui.addModule("Draft")
                    FreeCADGui.doCommand("Draft.autogroup(obj)")
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                    return
            if obj.isDerivedFrom("Part::Feature"):
                if obj.Shape.Wires:
                    FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Roof"))
                    FreeCADGui.addModule("Arch")
                    FreeCADGui.doCommand("obj = Arch.makeRoof(FreeCAD.ActiveDocument."+obj.Name+")")
                    FreeCADGui.addModule("Draft")
                    FreeCADGui.doCommand("Draft.autogroup(obj)")
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                    return
            else:
                FreeCAD.Console.PrintMessage(translate("Arch","Unable to create a roof"))
        else:
            FreeCAD.Console.PrintMessage(translate("Arch","Please select a base object")+"\n")
            FreeCADGui.Control.showDialog(ArchComponent.SelectionTaskPanel())
            FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(nextCommand="Arch_Roof")
            FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)


class _Roof(ArchComponent.Component):

    "The Roof object"

    def __init__(self,obj):

        ArchComponent.Component.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Roof"
        obj.Proxy = self

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Angles" in pl:
            obj.addProperty("App::PropertyFloatList","Angles","Roof",    QT_TRANSLATE_NOOP("App::Property","A list of angles for each roof pane"))
        if not "Runs" in pl:
            obj.addProperty("App::PropertyFloatList","Runs","Roof",      QT_TRANSLATE_NOOP("App::Property","A list of horizontal length projections for each roof pane"))
        if not "IdRel" in pl:
            obj.addProperty("App::PropertyIntegerList","IdRel","Roof",   QT_TRANSLATE_NOOP("App::Property","A list of IDs of relative profiles for each roof pane"))
        if not "Thickness" in pl:
            obj.addProperty("App::PropertyFloatList","Thickness","Roof", QT_TRANSLATE_NOOP("App::Property","A list of thicknesses for each roof pane"))
        if not "Overhang" in pl:
            obj.addProperty("App::PropertyFloatList","Overhang","Roof",  QT_TRANSLATE_NOOP("App::Property","A list of overhangs for each roof pane"))
        if not "Heights" in pl:
            obj.addProperty("App::PropertyFloatList","Heights","Roof",   QT_TRANSLATE_NOOP("App::Property","A list of calculated heights for each roof pane"))
        if not "Face" in pl:
            obj.addProperty("App::PropertyInteger","Face","Roof",        QT_TRANSLATE_NOOP("App::Property","The face number of the base object used to build this roof"))
        if not "RidgeLength" in pl:
            obj.addProperty("App::PropertyLength","RidgeLength","Roof",  QT_TRANSLATE_NOOP("App::Property","The total length of ridges and hips of this roof"))
            obj.setEditorMode("RidgeLength",1)
        if not "BorderLength" in pl:
            obj.addProperty("App::PropertyLength","BorderLength","Roof", QT_TRANSLATE_NOOP("App::Property","The total length of borders of this roof"))
            obj.setEditorMode("BorderLength",1)
        if not "Flip" in pl:
            obj.addProperty("App::PropertyBool","Flip","Roof",QT_TRANSLATE_NOOP("App::Property","Flip the roof direction if going the wrong way"))
        self.Type = "Roof"

    def onDocumentRestored(self,obj):

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def calcHeight(self, id):

        " Get the height from run and angle of the given roof profile "

        htRel = self.profilsDico[id]["run"]*(math.tan(math.radians(self.profilsDico[id]["angle"])))
        return htRel

    def calcRun(self, id):

        " Get the run from height and angle of the given roof profile "

        runRel = self.profilsDico[id]["height"]/(math.tan(math.radians(self.profilsDico[id]["angle"])))
        return runRel

    def calcAngle(self, id):

        " Get the angle from height and run of the given roof profile "

        a = math.degrees(math.atan(self.profilsDico[id]["height"]/self.profilsDico[id]["run"]))
        return a

    def getPerpendicular(self, vec, rotEdge, l):


        " Get the perpendicular vec of given edge on xy plane "
        norm = FreeCAD.Vector(0,0,1)
        if hasattr(self,"normal"):
            if self.normal:
                norm = FreeCAD.Vector(self.normal)
        perpendicular = vec.cross(norm)
        if  -180. <= rotEdge < -90.:
            perpendicular[0] = abs(perpendicular[0])*-1
            perpendicular[1] = abs(perpendicular[1])*-1
        elif   -90. <= rotEdge <= 0.:
            perpendicular[0] = abs(perpendicular[0])*-1
            perpendicular[1] = abs(perpendicular[1])
        elif 0. < rotEdge <= 90.:
            perpendicular[0] = abs(perpendicular[0])
            perpendicular[1] = abs(perpendicular[1])
        elif 90. < rotEdge <= 180.:
            perpendicular[0] = abs(perpendicular[0])
            perpendicular[1] = abs(perpendicular[1])*-1
        else:
            print("Unknown Angle")
        perpendicular[2] = abs(perpendicular[2])
        perpendicular.normalize()
        perpendicular = perpendicular.multiply(l)
        if hasattr(self,"flip"):
            if self.flip:
                return perpendicular.negative()
        return perpendicular

    def makeRoofProfilsDic(self, id, angle, run, idrel, overhang, thickness,):

        profilDico = {}
        profilDico["id"] = id
        if angle == 90.:
            profilDico["name"] = "Pignon"+str(id)
            profilDico["run"]=0.
        else:
            profilDico["name"] = "Pan"+str(id)
            profilDico["run"] = run
        profilDico["angle"] = angle
        profilDico["idrel"] = idrel
        profilDico["overhang"] = overhang
        profilDico["thickness"] = thickness
        profilDico["height"] = None
        profilDico["points"] = []
        self.profilsDico.append(profilDico)

    def calcMissingData(self, i):

        a = self.profilsDico[i]["angle"]
        run = self.profilsDico[i]["run"]
        rel = self.profilsDico[i]["idrel"]
        if a == 0.0 and run == 0.0 :
            self.profilsDico[i]["run"] = self.profilsDico[rel]["run"]
            self.profilsDico[i]["angle"] = self.profilsDico[rel]["angle"]
            self.profilsDico[i]["height"] = self.calcHeight(i)
        elif run == 0:
            if a == 90. :
                htRel = self.calcHeight(rel)
                self.profilsDico[i]["height"] = htRel
            else :
                htRel = self.calcHeight(rel)
                self.profilsDico[i]["height"] = htRel
                run = self.calcRun(i)
                self.profilsDico[i]["run"] = run
        elif a == 0. :
            htRel = self.calcHeight(rel)
            self.profilsDico[i]["height"] = htRel
            a = self.calcAngle(i)
            self.profilsDico[i]["angle"] = a
        else :
            ht = self.calcHeight(i)
            self.profilsDico[i]["height"] = ht

    def calcEdgeGeometry(self, edges, i):

        self.profilsDico[i]["edge"] = edges[i]
        vec = edges[i].Vertexes[-1].Point.sub(edges[i].Vertexes[0].Point)
        self.profilsDico[i]["vec"] = vec
        rot = math.degrees(DraftVecUtils.angle(vec))
        self.profilsDico[i]["rot"] = rot

    def calcDraftEdges(self, i):

        import DraftGeomUtils
        edge = self.profilsDico[i]["edge"]
        vec = self.profilsDico[i]["vec"]
        rot = self.profilsDico[i]["rot"]
        overhang = self.profilsDico[i]["overhang"]
        run = self.profilsDico[i]["run"]
        perpendicular = self.getPerpendicular(vec,rot,self.profilsDico[i]["overhang"]).negative()
        eave = DraftGeomUtils.offset(edge,perpendicular)
        self.profilsDico[i]["eaveD"] = eave
        perpendicular = self.getPerpendicular(vec,rot,self.profilsDico[i]["run"])
        ridge = DraftGeomUtils.offset(edge,perpendicular)
        self.profilsDico[i]["ridge"] = ridge

    def calcEave(self, i):

        import DraftGeomUtils
        pt0Eave1 = DraftGeomUtils.findIntersection(self.findProfil(i-1)["eaveD"],self.findProfil(i)["eaveD"],infinite1=True,infinite2=True,)
        pt1Eave1 = DraftGeomUtils.findIntersection(self.findProfil(i)["eaveD"],self.findProfil(i+1)["eaveD"],infinite1=True,infinite2=True,)
        eave = DraftGeomUtils.edg(FreeCAD.Vector(pt0Eave1[0]),FreeCAD.Vector(pt1Eave1[0]))
        self.profilsDico[i]["eave"] = eave

    def findProfil(self, idx):

        if 0<=idx<len(self.profilsDico):
            profil = self.profilsDico[idx]
        else:
            idx = abs(abs(idx) - len(self.profilsDico))
            profil = self.profilsDico[idx]
        return profil

    def nextPignon(self, i):

        import DraftGeomUtils
        profilCurrent = self.findProfil(i)
        profilNext1 = self.findProfil(i+1)
        profilNext2 = self.findProfil(i+2)
        point = DraftGeomUtils.findIntersection(profilCurrent["eave"],profilNext1["eave"],infinite1=True,infinite2=True,)
        self.ptsPaneProject.append(FreeCAD.Vector(point[0]))
        pt1 = DraftGeomUtils.findIntersection(profilCurrent["edge"],profilNext1["eave"],infinite1=True,infinite2=True,)
        pt2 = DraftGeomUtils.findIntersection(profilNext2["edge"],profilNext1["eave"],infinite1=True,infinite2=True,)
        eaveWithoutOverhang = DraftGeomUtils.edg(pt1[0],pt2[0])
        if profilCurrent["run"]+profilNext2["run"] != eaveWithoutOverhang.Length :
            points = [FreeCAD.Vector(0.0,0.0,0.0),]
            points.append(FreeCAD.Vector(profilCurrent["run"],profilCurrent["height"],0.0))
            rampantCurrent = DraftGeomUtils.edg(points[0],points[1])
            points = [FreeCAD.Vector(eaveWithoutOverhang.Length,0.0,0.0),]
            points.append(FreeCAD.Vector(eaveWithoutOverhang.Length-profilNext2["run"],profilNext2["height"],0.0))
            rampantNext2 = DraftGeomUtils.edg(points[0],points[1])
            point = DraftGeomUtils.findIntersection(rampantCurrent,rampantNext2,infinite1=True,infinite2=True,)
            ridgeCurrent = DraftGeomUtils.offset(profilCurrent["edge"],self.getPerpendicular(profilCurrent["vec"],profilCurrent["rot"],point[0].x))
            point = DraftGeomUtils.findIntersection(ridgeCurrent,profilNext1["eave"],infinite1=True,infinite2=True,)
        else:
            point = DraftGeomUtils.findIntersection(profilCurrent["ridge"],profilNext1["eaveD"],infinite1=True,infinite2=True,)
        self.ptsPaneProject.append(FreeCAD.Vector(point[0]))

    def backPignon(self, i):

        import DraftGeomUtils
        profilCurrent = self.findProfil(i)
        profilBack1 = self.findProfil(i-1)
        profilBack2 = self.findProfil(i-2)
        pt1 = DraftGeomUtils.findIntersection(profilCurrent["edge"],profilBack1["eave"],infinite1=True,infinite2=True,)
        pt2 = DraftGeomUtils.findIntersection(profilBack2["edge"],profilBack1["eave"],infinite1=True,infinite2=True,)
        eaveWithoutOverhang = DraftGeomUtils.edg(pt1[0],pt2[0])
        if profilCurrent["run"]+profilBack2["run"] != eaveWithoutOverhang.Length :
            points = [FreeCAD.Vector(0.0,0.0,0.0),]
            points.append(FreeCAD.Vector(profilCurrent["run"],profilCurrent["height"],0.0))
            rampantCurrent = DraftGeomUtils.edg(points[0],points[1])
            points = [FreeCAD.Vector(eaveWithoutOverhang.Length,0.0,0.0),]
            points.append(FreeCAD.Vector(eaveWithoutOverhang.Length-profilBack2["run"],profilBack2["height"],0.0))
            rampantBack2 = DraftGeomUtils.edg(points[0],points[1])
            point = DraftGeomUtils.findIntersection(rampantCurrent,rampantBack2,infinite1=True,infinite2=True,)
            ridgeCurrent = DraftGeomUtils.offset(profilCurrent["edge"],self.getPerpendicular(profilCurrent["vec"],profilCurrent["rot"],point[0].x))
            point = DraftGeomUtils.findIntersection(ridgeCurrent,profilBack1["eave"],infinite1=True,infinite2=True,)
        else:
            point = DraftGeomUtils.findIntersection(profilCurrent["ridge"],profilBack1["eave"],infinite1=True,infinite2=True,)
        self.ptsPaneProject.append(FreeCAD.Vector(point[0]))
        point = DraftGeomUtils.findIntersection(profilCurrent["eave"],profilBack1["eave"],infinite1=True,infinite2=True,)
        self.ptsPaneProject.append(FreeCAD.Vector(point[0]))

    def nextSameHeight(self, i):

        import DraftGeomUtils
        profilCurrent = self.findProfil(i)
        profilNext1 = self.findProfil(i+1)
        ptInterRidges = DraftGeomUtils.findIntersection(profilCurrent["ridge"],profilNext1["ridge"],infinite1=True,infinite2=True,)
        edgeHip = DraftGeomUtils.edg(FreeCAD.Vector(ptInterRidges[0]),profilCurrent["edge"].Vertexes[-1].Point)
        ptInterHipEave = DraftGeomUtils.findIntersection(edgeHip,profilCurrent["eave"],infinite1=True,infinite2=False,)
        if ptInterHipEave:
            self.ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave[0]))
        else:
            ptInterHipEave = DraftGeomUtils.findIntersection(edgeHip,profilNext1["eaveD"],infinite1=True,infinite2=True,)
            self.ptsPaneProject.append(FreeCAD.Vector(profilCurrent["eave"].Vertexes[-1].Point))
            self.ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave[0]))
        self.ptsPaneProject.append(FreeCAD.Vector(ptInterRidges[0]))

    def backSameHeight(self, i):

        import DraftGeomUtils
        profilCurrent = self.findProfil(i)
        profilBack1 = self.findProfil(i-1)
        ptInterRidges = DraftGeomUtils.findIntersection(profilCurrent["ridge"],profilBack1["ridge"],infinite1=True,infinite2=True,)
        self.ptsPaneProject.append(FreeCAD.Vector(ptInterRidges[0]))
        edgeHip = DraftGeomUtils.edg(FreeCAD.Vector(ptInterRidges[0]),profilCurrent["edge"].Vertexes[0].Point)
        ptInterHipEave = DraftGeomUtils.findIntersection(edgeHip,profilCurrent["eave"],infinite1=True,infinite2=False,)
        if ptInterHipEave:
            self.ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave[0]))
        else:
            ptInterHipEave = DraftGeomUtils.findIntersection(edgeHip,profilBack1["eaveD"],infinite1=True,infinite2=True,)
            self.ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave[0]))
            self.ptsPaneProject.append(FreeCAD.Vector(profilCurrent["eave"].Vertexes[0].Point))

    def nextHigher(self, i):

        import DraftGeomUtils
        profilCurrent = self.findProfil(i)
        profilNext1 = self.findProfil(i+1)
        dec = profilCurrent["height"]/math.tan(math.radians(profilNext1["angle"]))
        edgeRidgeOnPane = DraftGeomUtils.offset(profilNext1["edge"],self.getPerpendicular(profilNext1["vec"],profilNext1["rot"],dec))
        ptInter = DraftGeomUtils.findIntersection(profilCurrent["ridge"],edgeRidgeOnPane,infinite1=True,infinite2=True,)
        edgeHip = DraftGeomUtils.edg(FreeCAD.Vector(ptInter[0]),profilCurrent["edge"].Vertexes[-1].Point)
        ptInterHipEave = DraftGeomUtils.findIntersection(edgeHip,profilCurrent["eave"],infinite1=True,infinite2=False,)
        if ptInterHipEave:
            self.ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave[0]))
        else:
            ptInterHipEave = DraftGeomUtils.findIntersection(edgeHip,profilNext1["eaveD"],infinite1=True,infinite2=True,)
            self.ptsPaneProject.append(FreeCAD.Vector(profilCurrent["eave"].Vertexes[-1].Point[0]))
            self.ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave[0]))
        self.ptsPaneProject.append(FreeCAD.Vector(ptInter[0]))
        ptInterRidges = DraftGeomUtils.findIntersection(profilCurrent["ridge"],profilNext1["ridge"],infinite1=True,infinite2=True,)
        self.ptsPaneProject.append(FreeCAD.Vector(ptInterRidges[0]))

    def backHigher(self, i):

        import DraftGeomUtils
        profilCurrent = self.findProfil(i)
        profilBack1 = self.findProfil(i-1)
        dec = profilCurrent["height"]/math.tan(math.radians(profilBack1["angle"]))
        edgeRidgeOnPane = DraftGeomUtils.offset(profilBack1["edge"],self.getPerpendicular(profilBack1["vec"],profilBack1["rot"],dec))
        ptInterRidges = DraftGeomUtils.findIntersection(edgeRidgeOnPane,profilCurrent["ridge"],infinite1=True,infinite2=True,)
        self.ptsPaneProject.append(FreeCAD.Vector(ptInterRidges[0]))
        edgeHip = DraftGeomUtils.edg(FreeCAD.Vector(ptInterRidges[0]),profilCurrent["edge"].Vertexes[0].Point)
        ptInterHipEave = DraftGeomUtils.findIntersection(edgeHip,profilCurrent["eave"],infinite1=True,infinite2=False,)
        if ptInterHipEave:
            self.ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave[0]))
        else:
            ptInterHipEave = DraftGeomUtils.findIntersection(edgeHip,profilBack1["eaveD"],infinite1=True,infinite2=True,)
            self.ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave[0]))
            self.ptsPaneProject.append(FreeCAD.Vector(profilCurrent["eave"].Vertexes[0].Point[0]))

    def nextSmaller(self, i):

        import DraftGeomUtils
        profilCurrent = self.findProfil(i)
        profilNext1 = self.findProfil(i+1)
        dec = profilNext1["height"]/math.tan(math.radians(profilCurrent["angle"]))
        edgeRidgeOnPane = DraftGeomUtils.offset(profilCurrent["edge"],self.getPerpendicular(profilCurrent["vec"],profilCurrent["rot"],dec))
        ptInter = DraftGeomUtils.findIntersection(profilNext1["ridge"],edgeRidgeOnPane,infinite1=True,infinite2=True,)
        edgeHip = DraftGeomUtils.edg(FreeCAD.Vector(ptInter[0]),profilCurrent["edge"].Vertexes[-1].Point)
        ptInterHipEave = DraftGeomUtils.findIntersection(edgeHip,profilCurrent["eave"],infinite1=True,infinite2=False,)
        if ptInterHipEave:
            self.ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave[0]))
        else:
            ptInterHipEave = DraftGeomUtils.findIntersection(edgeHip,profilNext1["eaveD"],infinite1=True,infinite2=True,)
            self.ptsPaneProject.append(FreeCAD.Vector(profilCurrent["eave"].Vertexes[-1].Point[0]))
            self.ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave[0]))
        self.ptsPaneProject.append(FreeCAD.Vector(ptInter[0]))
        ptInter = edgeHip.Vertexes[0].Point
        vecInterRidges = DraftGeomUtils.findPerpendicular(ptInter, [profilCurrent["ridge"].Edges[0],], force=0)
        ptInterRidges = ptInter.add(vecInterRidges[0])
        self.ptsPaneProject.append(FreeCAD.Vector(ptInterRidges))

    def backSmaller(self, i):

        import DraftGeomUtils
        profilCurrent = self.findProfil(i)
        profilBack1 = self.findProfil(i-1)
        dec = profilBack1["height"]/math.tan(math.radians(profilCurrent["angle"]))
        edgeRidgeOnPane = DraftGeomUtils.offset(profilCurrent["edge"],self.getPerpendicular(profilCurrent["vec"],profilCurrent["rot"],dec))
        ptInter1 = DraftGeomUtils.findIntersection(edgeRidgeOnPane,profilBack1["ridge"],infinite1=True,infinite2=True,)
        edgeHip = DraftGeomUtils.edg(FreeCAD.Vector(ptInter1[0]),profilCurrent["edge"].Vertexes[0].Point)
        ptInter2 = edgeHip.Vertexes[0].Point
        vecInterRidges = DraftGeomUtils.findPerpendicular(ptInter2, [profilCurrent["ridge"].Edges[0],], force=0)
        ptInterRidges = ptInter2.add(vecInterRidges[0])
        self.ptsPaneProject.append(FreeCAD.Vector(ptInterRidges))
        self.ptsPaneProject.append(FreeCAD.Vector(ptInter1[0]))
        ptInterHipEave = DraftGeomUtils.findIntersection(edgeHip,profilCurrent["eave"],infinite1=True,infinite2=False,)
        if ptInterHipEave:
            self.ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave[0]))
        else:
            ptInterHipEave = DraftGeomUtils.findIntersection(edgeHip,profilBack1["eaveD"],infinite1=True,infinite2=True,)
            self.ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave[0]))
            self.ptsPaneProject.append(FreeCAD.Vector(profilCurrent["eave"].Vertexes[0].Point[0]))

    def getRoofPaneProject(self, i):

        self.ptsPaneProject=[]
        profilCurrent = self.findProfil(i)
        profilBack1 = self.findProfil(i-1)
        profilNext1 = self.findProfil(i+1)
        if profilCurrent["angle"] != 90.:
            if profilNext1["angle"] == 90. :
                self.nextPignon(i)
            elif profilNext1["height"] == profilCurrent["height"] :
                self.nextSameHeight(i)
            elif profilNext1["height"] < profilCurrent["height"] :
                self.nextSmaller(i)
            elif profilNext1["height"] > profilCurrent["height"] :
                self.nextHigher(i)
            else:
                print("Arch Roof : Case Not implemented")
            if profilBack1["angle"] == 90. :
                self.backPignon(i)
            elif profilBack1["height"] == profilCurrent["height"] :
                self.backSameHeight(i)
            elif profilBack1["height"] < profilCurrent["height"] :
                self.backSmaller(i)
            elif profilBack1["height"] > profilCurrent["height"] :
                self.backHigher(i)
            else:
                print("Arch Roof : Case Not implemented")
        else:
            self.ptsPaneProject=[]

        self.ptsPaneProject = DraftVecUtils.removeDoubles(self.ptsPaneProject)
        profilCurrent["points"] = self.ptsPaneProject

    def createProfilShape (self, points, midpoint, rot, vec, run, d, f):

        import Part
        lp = len(points)
        points.append(points[0])
        edgesWire = []
        for i in range(lp):
            edge = Part.makeLine(points[i],points[i+1])
            edgesWire.append(edge)
        profil = Part.Wire(edgesWire)
        profil.translate(midpoint)
        profil.rotate(midpoint,FreeCAD.Vector(0,0,1), 90. + rot * -1)
        perp = self.getPerpendicular(vec,rot,run)
        profil.rotate(midpoint,perp,90.)
        vecT = vec.normalize()
        vecT.multiply(d)
        profil.translate(vecT)
        vecE = vecT.multiply(-2.)
        profilFace = Part.Face(profil)
        profilShp = profilFace.extrude(vecE)
        profilShp = f.common(profilShp)
        #shapesList.append(profilShp)
        return profilShp

    def execute(self,obj):

        if self.clone(obj):
            return

        import Part, math, DraftGeomUtils
        pl = obj.Placement
        #self.baseface = None
        self.flip = False
        if hasattr(obj,"Flip"):
            if obj.Flip:
                self.flip = True
        base = None
        w = None
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.Solids:
                    base = obj.Base.Shape
                    #pl = obj.Base.Placement
                else:
                    if (obj.Base.Shape.Faces and obj.Face):
                        w = obj.Base.Shape.Faces[obj.Face-1].Wires[0]
                    elif obj.Base.Shape.Wires:
                        w = obj.Base.Shape.Wires[0]
        if w:
            if w.isClosed():
                self.profilsDico = []
                self.shps = []
                self.subVolshps = []
                heights = []
                edges = Part.__sortEdges__(w.Edges)
                l = len(edges)
                for i in range(l):
                    self.makeRoofProfilsDic(i, obj.Angles[i], obj.Runs[i], obj.IdRel[i], obj.Overhang[i], obj.Thickness[i])
                for i in range(l):
                    self.calcMissingData(i)
                for i in range(l):
                    self.calcEdgeGeometry(edges, i)
                for i in range(l):
                    self.calcDraftEdges(i)
                for i in range(l):
                    self.calcEave(i)
                for p in self.profilsDico:
                    heights.append(p["height"])
                obj.Heights = heights
                for i in range(l):
                    self.getRoofPaneProject(i)
                    profilCurrent = self.findProfil(i)
                    midpoint = DraftGeomUtils.findMidpoint(profilCurrent["edge"])
                    ptsPaneProject = profilCurrent["points"]
                    lp = len(ptsPaneProject)
                    if lp != 0:
                        ptsPaneProject.append(ptsPaneProject[0])
                        edgesWire = []
                        for p in range(lp):
                            edge = Part.makeLine(ptsPaneProject[p],ptsPaneProject[p+1])
                            edgesWire.append(edge)
                        wire = Part.Wire(edgesWire)
                        d = wire.BoundBox.DiagonalLength
                        thicknessV = profilCurrent["thickness"]/(math.cos(math.radians(profilCurrent["angle"])))
                        overhangV = profilCurrent["overhang"]*math.tan(math.radians(profilCurrent["angle"]))
                        if wire.isClosed():
                            f = Part.Face(wire)
                            f = f.extrude(FreeCAD.Vector(0,0,profilCurrent["height"]+1000000.0))
                            f.translate(FreeCAD.Vector(0.0,0.0,-2*overhangV))
                        ptsPaneProfil=[FreeCAD.Vector(-profilCurrent["overhang"],-overhangV,0.0),FreeCAD.Vector(profilCurrent["run"],profilCurrent["height"],0.0),FreeCAD.Vector(profilCurrent["run"],profilCurrent["height"]+thicknessV,0.0),FreeCAD.Vector(-profilCurrent["overhang"],-overhangV+thicknessV,0.0)]
                        self.shps.append(self.createProfilShape(ptsPaneProfil, midpoint, profilCurrent["rot"], profilCurrent["vec"], profilCurrent["run"], d, f))
                        ## subVolume shape
                        ptsSubVolumeProfil=[FreeCAD.Vector(-profilCurrent["overhang"],-overhangV,0.0),FreeCAD.Vector(profilCurrent["run"],profilCurrent["height"],0.0),FreeCAD.Vector(profilCurrent["run"],profilCurrent["height"]+900000.0,0.0),FreeCAD.Vector(-profilCurrent["overhang"],profilCurrent["height"]+900000.0,0.0)]
                        self.subVolshps.append(self.createProfilShape(ptsSubVolumeProfil, midpoint, profilCurrent["rot"], profilCurrent["vec"], profilCurrent["run"], d, f))
                ## SubVolume
                self.sub = self.subVolshps.pop()
                for s in self.subVolshps:
                    self.sub = self.sub.fuse(s)
                self.sub = self.sub.removeSplitter()
                if not self.sub.isNull():
                    if not DraftGeomUtils.isNull(pl):
                        self.sub.Placement = pl
                ## BaseVolume
                base = self.shps.pop()
                for s in self.shps :
                    base = base.fuse(s)
                base = self.processSubShapes(obj,base)
                self.applyShape(obj,base,pl,allownosolid=True)
        elif base :
            base = self.processSubShapes(obj,base)
            self.applyShape(obj,base,pl,allownosolid=True)
        else:
            FreeCAD.Console.PrintMessage(translate("Arch","Unable to create a roof"))

    def getSubVolume(self,obj):

        "returns a volume to be subtracted"
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.Solids:
                    return obj.Shape
                else :
                    if self.sub:
                        return self.sub
                    else :
                        self.execute(obj)
                        return self.sub
        return None

    def computeAreas(self,obj):

        "computes border and ridge roof edges length"

        if hasattr(obj,"RidgeLength") and hasattr(obj,"BorderLength"):
            rl = 0
            bl = 0
            rn = 0
            bn = 0
            import Part,math
            if obj.Shape:
                if obj.Shape.Faces:
                    fset = []
                    for f in obj.Shape.Faces:
                        if f.normalAt(0,0).getAngle(FreeCAD.Vector(0,0,1)) < math.pi/2:
                            fset.append(f)
                    if fset:
                        try:
                            shell = Part.Shell(fset)
                        except:
                            pass
                        else:
                            lut={}
                            if shell.Faces:
                                for f in shell.Faces:
                                    for e in f.Edges:
                                        hc = e.hashCode()
                                        if hc in lut:
                                            lut[hc] = lut[hc] + 1
                                        else:
                                            lut[hc] = 1
                                for e in shell.Edges:
                                    if lut[e.hashCode()] == 1:
                                        bl += e.Length
                                        bn += 1
                                    elif lut[e.hashCode()] == 2:
                                        rl += e.Length
                                        rn += 1
            if obj.RidgeLength.Value != rl:
                obj.RidgeLength = rl
                #print(str(rn)+" ridge edges in roof "+obj.Name)
            if obj.BorderLength.Value != bl:
                obj.BorderLength = bl
                #print(str(bn)+" border edges in roof "+obj.Name)
        ArchComponent.Component.computeAreas(self,obj)


class _ViewProviderRoof(ArchComponent.ViewProviderComponent):

    "A View Provider for the Roof object"

    def __init__(self,vobj):

        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_Roof_Tree.svg"

    def attach(self,vobj):

        self.Object = vobj.Object
        return

    def unsetEdit(self,vobj,mode):

        FreeCADGui.Control.closeDialog()
        return

    def setEdit(self,vobj,mode=0):

        if vobj.Object.Base.Shape.Solids :
            taskd = ArchComponent.ComponentTaskPanel()
            taskd.obj = self.Object
            taskd.update()
            FreeCADGui.Control.showDialog(taskd)
        else:
            taskd = _RoofTaskPanel()
            taskd.obj = self.Object
            taskd.update()
            FreeCADGui.Control.showDialog(taskd)
        return True


class _RoofTaskPanel:

    '''The editmode TaskPanel for Roof objects'''

    def __init__(self):

        self.updating = False

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
        self.tree.setColumnCount(7)
        self.tree.header().resizeSection(0,30)
        self.tree.header().resizeSection(1,50)
        self.tree.header().resizeSection(2,60)
        self.tree.header().resizeSection(3,30)
        self.tree.header().resizeSection(4,50)
        self.tree.header().resizeSection(5,50)
        self.tree.header().resizeSection(6,50)

        QtCore.QObject.connect(self.tree, QtCore.SIGNAL("itemChanged(QTreeWidgetItem *, int)"), self.edit)
        self.update()

    def isAllowedAlterSelection(self):
        return False

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def update(self):
        'fills the treewidget'
        self.updating = True
        self.tree.clear()
        if self.obj:
            for i in range(len(self.obj.Angles)):
                item = QtGui.QTreeWidgetItem(self.tree)
                item.setText(0,str(i))
                item.setText(1,str(self.obj.Angles[i]))
                item.setText(2,str(self.obj.Runs[i]))
                item.setText(3,str(self.obj.IdRel[i]))
                item.setText(4,str(self.obj.Thickness[i]))
                item.setText(5,str(self.obj.Overhang[i]))
                item.setText(6,str(self.obj.Heights[i]))
                item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
                item.setTextAlignment(0,QtCore.Qt.AlignLeft)
        self.retranslateUi(self.form)
        self.updating = False

    def edit(self,item,column):
        if not self.updating:
            self.resetObject()

    def resetObject(self,remove=None):
        "transfers the values from the widget to the object"
        a = []
        run = []
        rel = []
        thick = []
        over = []
        for i in range(self.tree.topLevelItemCount()):
            it = self.tree.findItems(str(i),QtCore.Qt.MatchExactly,0)[0]
            a.append(float(it.text(1)))
            run.append(float(it.text(2)))
            rel.append(int(it.text(3)))
            thick.append(float(it.text(4)))
            over.append(float(it.text(5)))
        self.obj.Runs = run
        self.obj.Angles = a
        self.obj.IdRel = rel
        self.obj.Thickness = thick
        self.obj.Overhang = over
        self.obj.touch()
        FreeCAD.ActiveDocument.recompute()
        self.update()

    def reject(self):
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Roof", None))
        self.title.setText(QtGui.QApplication.translate("Arch", "Parameters of the profiles of the roof:\n* Angle : slope in degrees compared to the horizontal one.\n* Run : outdistance between the wall and the ridge sheathing.\n* Thickness : thickness of the side of roof.\n* Overhang : outdistance between the sewer and the wall.\n* Height : height of the ridge sheathing (calculated automatically)\n* IdRel : Relative Id for calculations automatic.\n---\nIf Angle = 0 and Run = 0 then profile is identical to the relative profile.\nIf Angle = 0 then angle is calculated so that the height is the same one as the relative profile.\nIf Run = 0 then Run is calculated so that the height is the same one as the relative profile.", None))
        self.tree.setHeaderLabels([QtGui.QApplication.translate("Arch", "Id", None),
                                    QtGui.QApplication.translate("Arch", "Angle (deg)", None),
                                    QtGui.QApplication.translate("Arch", "Run (mm)", None),
                                    QtGui.QApplication.translate("Arch", "IdRel", None),
                                    QtGui.QApplication.translate("Arch", "Thickness (mm)", None),
                                    QtGui.QApplication.translate("Arch", "Overhang (mm)", None),
                                    QtGui.QApplication.translate("Arch", "Height (mm)", None)])

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Roof',_CommandRoof())
