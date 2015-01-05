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

import FreeCAD, Draft, math, ArchComponent, DraftVecUtils, DraftGeomUtils
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

__title__="FreeCAD Roof"
__author__ = "Yorik van Havre", "Jonathan Wiedemann"
__url__ = "http://www.freecadweb.org"

def makeRoof(baseobj=None,facenr=1, angles=[45.,], run = [], idrel = [0,],thickness = [1.,], overhang=[2.,], name="Roof"):
    '''makeRoof(baseobj,[facenr],[angle],[name]) : Makes a roof based on a closed wire.
    face from an existing object. You can provide a list of angles, run, idrel, thickness,
    overhang for each edges in the wire to define the roof shape. The default for angle is 45
    and the list is automatically complete to match with number of edges in the wire.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    obj.Label = translate("Arch",name)
    _Roof(obj)
    if FreeCAD.GuiUp:
        _ViewProviderRoof(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
    if obj.Base.isDerivedFrom("Part::Feature"):
        if (obj.Base.Shape.Faces and obj.Face):
            w = obj.Base.Shape.Faces[obj.Face-1].Wires[0]
        elif obj.Base.Shape.Wires:
            w = obj.Base.Shape.Wires[0]
    if w:
        if w.isClosed():
            edges = DraftGeomUtils.sortEdges(w.Edges)
            l = len(edges)

            la = len(angles)
            alist = angles
            for i in range(l-la):
                alist.append(angles[0])
            obj.Angles=alist

            lr = len(run)
            rlist = run
            for i in range(l-lr):
                rlist.append(w.Edges[i].Length/2.)
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
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Roof","Roof"),
                'Accel': "R, F",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Roof","Creates a roof object from the selected wire.")}

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
                    FreeCADGui.doCommand("Arch.makeRoof(FreeCAD.ActiveDocument."+obj.Name+","+str(idx)+")")
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                    return
            if obj.isDerivedFrom("Part::Feature"):
                if obj.Shape.Wires:
                    FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Roof"))
                    FreeCADGui.addModule("Arch")
                    FreeCADGui.doCommand("Arch.makeRoof(FreeCAD.ActiveDocument."+obj.Name+")")
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                    return
            else:
                FreeCAD.Console.PrintMessage(translate("Arch","Unable to create a roof"))
        else:
            FreeCAD.Console.PrintMessage(translate("Arch","Please select a base object\n"))
            FreeCADGui.Control.showDialog(ArchComponent.SelectionTaskPanel())
            FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(nextCommand="Arch_Roof")
            FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)

class _Roof(ArchComponent.Component):
    "The Roof object"

    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyFloatList","Angles","Arch", translate("Arch","The angles of each slope."))
        obj.addProperty("App::PropertyFloatList","Runs","Arch", translate("Arch","The horizontal lenght projection of each crawling."))
        obj.addProperty("App::PropertyIntegerList","IdRel","Arch", translate("Arch","The pane Id of relative profil."))
        obj.addProperty("App::PropertyFloatList","Thickness","Arch", translate("Arch","The thickness of the roof pane."))
        obj.addProperty("App::PropertyFloatList","Overhang","Arch", translate("Arch","The Overhang of the roof pane."))
        obj.addProperty("App::PropertyFloatList","Heights","Arch", translate("Arch","The calculated height of the roof pane list."))
        obj.addProperty("App::PropertyInteger","Face","Base",translate("Arch","The face number of the base object used to build this roof"))
        self.Type = "Roof"
        obj.Proxy = self

    def calcHeight(self, id):
        htRel = self.profilsDico[id]["run"]*(math.tan(math.radians(self.profilsDico[id]["angle"])))
        return htRel

    def calcRun(self, id):
        runRel = self.profilsDico[id]["height"]/(math.tan(math.radians(self.profilsDico[id]["angle"])))
        return runRel

    def calcAngle(self, id):
        a = math.degrees(math.atan(self.profilsDico[id]["height"]/self.profilsDico[id]["run"]))
        return a

    def getPerpendicular(self, vec, rotEdge, l):
        norm = FreeCAD.Vector(0,0,1)
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
            print("Angle inconnue")
        perpendicular[2] = abs(perpendicular[2])
        perpendicular.normalize()
        perpendicular = perpendicular.multiply(l)
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

    def createProfilShape (self, points, midpoint, rotEdge1, vec1, run, d, shapesList, f):
        import Part
        lp = len(points)
        points.append(points[0])
        edgesWire = []
        for i in range(lp):
            edge = Part.makeLine(points[i],points[i+1])
            edgesWire.append(edge)
        profil = Part.Wire(edgesWire)
        profil.translate(midpoint)
        profil.rotate(midpoint,FreeCAD.Vector(0,0,1), 90. + rotEdge1 * -1)
        perp = self.getPerpendicular(vec1,rotEdge1,run)
        profil.rotate(midpoint,perp,90.)
        vecT = vec1.normalize()
        vecT.multiply(d)
        profil.translate(vecT)
        vecE = vecT.multiply(-2.)
        profilFace = Part.Face(profil)
        profilShp = profilFace.extrude(vecE)
        #Part.show(profilVol)
        profilShp = f.common(profilShp)
        #Part.show(panVol)
        shapesList.append(profilShp)

    def execute(self,obj):
        import Part, math, DraftGeomUtils
        pl = obj.Placement
        self.baseface = None

        base = None
        if obj.Base and obj.Angles:
            w = None
            if obj.Base.isDerivedFrom("Part::Feature"):
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
                    edges = DraftGeomUtils.sortEdges(w.Edges)
                    l = len(edges)
                    print("le contour contient "+str(l)+" aretes")
                    for i in range(l):
                        self.makeRoofProfilsDic(i, obj.Angles[i], obj.Runs[i], obj.IdRel[i], obj.Overhang[i], obj.Thickness[i])
                    for i in range(l):
                        self.calcMissingData(i)
                    for p in self.profilsDico:
                        heights.append(p["height"])
                    obj.Heights = heights
                    for i in range(l):
                        edgesForward = edges[:]
                        edgesForward.append(edges[0])
                        ptsPaneProject=[]
                        profil0 =self.profilsDico[i-1]
                        profil1 =self.profilsDico[i]
                        if i == l-1:
                            profil2 =self.profilsDico[0]
                        else:
                            profil2 =self.profilsDico[i+1]
                        vec0 = edges[i-1].Vertexes[-1].Point.sub(edges[i-1].Vertexes[0].Point)
                        vec1 = edges[i].Vertexes[-1].Point.sub(edges[i].Vertexes[0].Point)
                        vec2 = edgesForward[i+1].Vertexes[-1].Point.sub(edgesForward[i+1].Vertexes[0].Point)
                        rotEdge0 = math.degrees(DraftVecUtils.angle(vec0))
                        rotEdge1 = math.degrees(DraftVecUtils.angle(vec1))
                        rotEdge2 = math.degrees(DraftVecUtils.angle(vec2))
                        edgeEave0 = DraftGeomUtils.offset(edges[i-1],self.getPerpendicular(vec0,rotEdge0,profil0["overhang"]).negative())
                        edgeEave1 = DraftGeomUtils.offset(edges[i],self.getPerpendicular(vec1,rotEdge1,profil1["overhang"]).negative())
                        edgeEave2 = DraftGeomUtils.offset(edgesForward[i+1],self.getPerpendicular(vec2,rotEdge2,profil2["overhang"]).negative())
                        pt0Eave1 = DraftGeomUtils.findIntersection(edgeEave0,edgeEave1,infinite1=True,infinite2=True,)
                        pt1Eave1 = DraftGeomUtils.findIntersection(edgeEave1,edgeEave2,infinite1=True,infinite2=True,)
                        edgeEave1 = DraftGeomUtils.edg(FreeCAD.Vector(pt0Eave1[0]),FreeCAD.Vector(pt1Eave1[0]))
                        edgeRidge0 = DraftGeomUtils.offset(edges[i-1],self.getPerpendicular(vec0,rotEdge0,profil0["run"]))
                        edgeRidge1 = DraftGeomUtils.offset(edges[i],self.getPerpendicular(vec1,rotEdge1,profil1["run"]))
                        edgeRidge2 = DraftGeomUtils.offset(edgesForward[i+1],self.getPerpendicular(vec2,rotEdge2,profil2["run"]))
                        midpoint = DraftGeomUtils.findMidpoint(edges[i])
                        pt0Edge1 = edges[i].Vertexes[0].Point
                        pt1Edge1 = edges[i].Vertexes[-1].Point
                        print("Analyse profil " + str(i))
                        if profil1["angle"] != 90.:
                            if profil2["angle"] == 90. :
                                print("situation a droite : pignon")
                                ptsPaneProject.append(FreeCAD.Vector(pt1Eave1[0]))
                                point = DraftGeomUtils.findIntersection(edgeRidge1,edgeEave2,infinite1=True,infinite2=True,)
                                ptsPaneProject.append(FreeCAD.Vector(point[0]))
                            elif profil1["height"] == profil2["height"] :
                                print("situation a droite : ht1 = ht2")
                                ptInterRidges = DraftGeomUtils.findIntersection(edgeRidge1,edgeRidge2,infinite1=True,infinite2=True,)
                                edgeHip = DraftGeomUtils.edg(FreeCAD.Vector(ptInterRidges[0]),pt1Edge1)
                                ptInterHipEave1 = DraftGeomUtils.findIntersection(edgeHip,edgeEave1,infinite1=True,infinite2=False,)
                                if ptInterHipEave1:
                                    ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave1[0]))
                                else:
                                    ptInterHipEave2 = DraftGeomUtils.findIntersection(edgeHip,edgeEave2,infinite1=True,infinite2=True,)
                                    ptsPaneProject.append(FreeCAD.Vector(pt1Eave1[0]))
                                    ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave2[0]))
                                ptsPaneProject.append(FreeCAD.Vector(ptInterRidges[0]))
                            elif profil1["height"] > profil2["height"]:
                                print("situation a droite : ht1 > ht2")
                                dec = profil2["height"]/math.tan(math.radians(profil1["angle"]))
                                edgeRidge2OnPane = DraftGeomUtils.offset(edges[i],self.getPerpendicular(vec1,rotEdge1,dec))
                                ptInter1 = DraftGeomUtils.findIntersection(edgeRidge2,edgeRidge2OnPane,infinite1=True,infinite2=True,)
                                edgeHip = DraftGeomUtils.edg(FreeCAD.Vector(ptInter1[0]),pt1Edge1)
                                ptInterHipEave1 = DraftGeomUtils.findIntersection(edgeHip,edgeEave1,infinite1=True,infinite2=False,)
                                if ptInterHipEave1:
                                    ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave1[0]))
                                else:
                                    ptInterHipEave2 = DraftGeomUtils.findIntersection(edgeHip,edgeEave2,infinite1=True,infinite2=True,)
                                    ptsPaneProject.append(FreeCAD.Vector(pt1Eave1[0]))
                                    ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave2[0]))
                                ptsPaneProject.append(FreeCAD.Vector(ptInter1[0]))
                                ptInter2 = edgeHip.Vertexes[0].Point
                                vecInterRidges = DraftGeomUtils.findPerpendicular(ptInter2, [edgeRidge1.Edges[0],], force=0)
                                ptInterRidges = ptInter2.add(vecInterRidges[0])
                                ptsPaneProject.append(FreeCAD.Vector(ptInterRidges))
                            elif profil1["height"] < profil2["height"]:
                                print("situation a droite : ht1 < ht2")
                                dec = profil1["height"]/math.tan(math.radians(profil2["angle"]))
                                edgeRidge2OnPane = DraftGeomUtils.offset(edgesForward[i+1],self.getPerpendicular(vec2,rotEdge2,dec))
                                ptInter1 = DraftGeomUtils.findIntersection(edgeRidge1,edgeRidge2OnPane,infinite1=True,infinite2=True,)
                                edgeHip = DraftGeomUtils.edg(FreeCAD.Vector(ptInter1[0]),pt1Edge1)
                                ptInterHipEave1 = DraftGeomUtils.findIntersection(edgeHip,edgeEave1,infinite1=True,infinite2=False,)
                                if ptInterHipEave1:
                                    ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave1[0]))
                                else:
                                    ptInterHipEave2 = DraftGeomUtils.findIntersection(edgeHip,edgeEave2,infinite1=True,infinite2=True,)
                                    ptsPaneProject.append(FreeCAD.Vector(pt1Eave1[0]))
                                    ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave2[0]))
                                ptsPaneProject.append(FreeCAD.Vector(ptInter1[0]))
                                ptInterRidges = DraftGeomUtils.findIntersection(edgeRidge1,edgeRidge2,infinite1=True,infinite2=True,)
                                ptsPaneProject.append(FreeCAD.Vector(ptInterRidges[0]))
                            else:
                                print("Cas de figure non pris en charge")
                            if profil0["angle"] == 90. :
                                print("situation a gauche : pignon")
                                point = DraftGeomUtils.findIntersection(edgeRidge1,edgeEave0,infinite1=True,infinite2=True,)
                                ptsPaneProject.append(FreeCAD.Vector(point[0]))
                                ptsPaneProject.append(FreeCAD.Vector(pt0Eave1[0]))
                            elif profil0["height"] == profil1["height"]:
                                print("situation a gauche : ht1 = ht0")
                                edgeRidge0 = DraftGeomUtils.offset(edges[i-1],self.getPerpendicular(vec0,rotEdge0,profil0["run"]))
                                ptInterRidges = DraftGeomUtils.findIntersection(edgeRidge1,edgeRidge0,infinite1=True,infinite2=True,)
                                ptsPaneProject.append(FreeCAD.Vector(ptInterRidges[0]))
                                edgeHip = DraftGeomUtils.edg(FreeCAD.Vector(ptInterRidges[0]),pt0Edge1)
                                ptInterHipEave3 = DraftGeomUtils.findIntersection(edgeHip,edgeEave1,infinite1=True,infinite2=False,)
                                if ptInterHipEave3:
                                    ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave3[0]))
                                else:
                                    ptInterHipEave4 = DraftGeomUtils.findIntersection(edgeHip,edgeEave0,infinite1=True,infinite2=True,)
                                    ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave4[0]))
                                    ptsPaneProject.append(FreeCAD.Vector(pt0Eave1[0]))
                            elif profil1["height"] > profil0["height"]:
                                print("situation a gauche : ht1 > ht0")
                                dec = profil0["height"]/math.tan(math.radians(profil1["angle"]))
                                edgeRidge0OnPane = DraftGeomUtils.offset(edges[i],self.getPerpendicular(vec1,rotEdge1,dec))
                                ptInter1 = DraftGeomUtils.findIntersection(edgeRidge0OnPane,edgeRidge0,infinite1=True,infinite2=True,)
                                edgeHip = DraftGeomUtils.edg(FreeCAD.Vector(ptInter1[0]),pt0Edge1)
                                ptInter2 = edgeHip.Vertexes[0].Point
                                vecInterRidges = DraftGeomUtils.findPerpendicular(ptInter2, [edgeRidge1.Edges[0],], force=0)
                                ptInterRidges = ptInter2.add(vecInterRidges[0])
                                ptsPaneProject.append(FreeCAD.Vector(ptInterRidges))
                                ptsPaneProject.append(FreeCAD.Vector(ptInter1[0]))
                                ptInterHipEave3 = DraftGeomUtils.findIntersection(edgeHip,edgeEave1,infinite1=True,infinite2=False,)
                                if ptInterHipEave3:
                                    ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave3[0]))
                                else:
                                    ptInterHipEave4 = DraftGeomUtils.findIntersection(edgeHip,edgeEave0,infinite1=True,infinite2=True,)
                                    ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave4[0]))
                                    ptsPaneProject.append(FreeCAD.Vector(pt0Eave1[0]))
                            elif profil1["height"] < profil0["height"]:
                                print("situation a gauche : ht1 < ht0")
                                dec = profil1["height"]/math.tan(math.radians(profil0["angle"]))
                                edgeRidge0OnPane = DraftGeomUtils.offset(edges[i-1],self.getPerpendicular(vec0,rotEdge0,dec))
                                ptInterRidges = DraftGeomUtils.findIntersection(edgeRidge0OnPane,edgeRidge1,infinite1=True,infinite2=True,)
                                ptsPaneProject.append(FreeCAD.Vector(ptInterRidges[0]))
                                edgeHip = DraftGeomUtils.edg(FreeCAD.Vector(ptInterRidges[0]),pt0Edge1)
                                ptInterHipEave3 = DraftGeomUtils.findIntersection(edgeHip,edgeEave1,infinite1=True,infinite2=False,)
                                if ptInterHipEave3:
                                    ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave3[0]))
                                else:
                                    ptInterHipEave4 = DraftGeomUtils.findIntersection(edgeHip,edgeEave0,infinite1=True,infinite2=True,)
                                    ptsPaneProject.append(FreeCAD.Vector(ptInterHipEave4[0]))
                                    ptsPaneProject.append(FreeCAD.Vector(pt0Eave1[0]))
                            else:
                                print("Cas de figure non pris en charge")
                            ptsPaneProject = DraftVecUtils.removeDoubles(ptsPaneProject)
                            print("ptsPaneProject",ptsPaneProject)
                            print("Fin Analyse profil " + str(i))
                            self.profilsDico[i]["points"] = ptsPaneProject
                            lp = len(ptsPaneProject)
                            ptsPaneProject.append(ptsPaneProject[0])
                            edgesWire = []
                            for i in range(lp):
                                edge = Part.makeLine(ptsPaneProject[i],ptsPaneProject[i+1])
                                edgesWire.append(edge)
                            wire = Part.Wire(edgesWire)
                            d = wire.BoundBox.DiagonalLength
                            thicknessV = profil1["thickness"]/(math.cos(math.radians(profil1["angle"])))
                            overhangV = profil1["overhang"]*math.tan(math.radians(profil1["angle"]))
                            if wire.isClosed():
                                f = Part.Face(wire)
                                f = f.extrude(FreeCAD.Vector(0,0,profil1["height"]+2*thicknessV+2*overhangV))
                                f.translate(FreeCAD.Vector(0.0,0.0,-2*overhangV))
                            ptsPaneProfil=[FreeCAD.Vector(-profil1["overhang"],-overhangV,0.0),FreeCAD.Vector(profil1["run"],profil1["height"],0.0),FreeCAD.Vector(profil1["run"],profil1["height"]+thicknessV,0.0),FreeCAD.Vector(-profil1["overhang"],-overhangV+thicknessV,0.0)]
                            self.createProfilShape (ptsPaneProfil, midpoint, rotEdge1, vec1, profil1["run"], d, self.shps, f)
                            ## subVolume shape
                            ptsSubVolumeProfil=[FreeCAD.Vector(-profil1["overhang"],-overhangV,0.0),FreeCAD.Vector(profil1["run"],profil1["height"],0.0),FreeCAD.Vector(profil1["run"],profil1["height"]+10000,0.0),FreeCAD.Vector(0.0,profil1["height"]+10000,0.0)]
                            self.createProfilShape (ptsSubVolumeProfil, midpoint, rotEdge1, vec1, profil1["run"], d, self.subVolshps, f)
                        else:
                            #TODO PIGNON
                            pass

                    ## SubVolume
                    self.sub = self.subVolshps.pop()
                    for s in self.subVolshps:
                        self.sub = self.sub.fuse(s)
                    self.sub = self.sub.removeSplitter()
                    if not self.sub.isNull():
                        if not DraftGeomUtils.isNull(pl):
                            self.sub.Placement = pl
                    ## BaseVolume
                    base = Part.makeCompound(self.shps)
                    if not base.isNull():
                        if not DraftGeomUtils.isNull(pl):
                            base.Placement = pl
        base = self.processSubShapes(obj,base)
        if base:
            if not base.isNull():
                obj.Shape = base

    def getSubVolume(self,obj):
        "returns a volume to be subtracted"
        if self.sub:
            return self.sub
        else :
            self.execute(obj)
            return self.sub
        return None

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
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Roof", None, QtGui.QApplication.UnicodeUTF8))
        self.title.setText(QtGui.QApplication.translate("Arch", "Parameters of the profiles of the roof:\n* Angle : slope in degrees compared to the horizontal one.\n* Run : outdistance between the wall and the ridge sheathing.\n* Thickness : thickness of the side of roof.\n* Overhang : outdistance between the sewer and the wall.\n* Height : height of the ridge sheathing (calculated automatically)\n* IdRel : Relative Id for calculations automatic.\n---\nIf Angle = 0 and Run = 0 then profile is identical to the relative profile.\nIf Angle = 0 then angle is calculated so that the height is the same one as the relative profile.\nIf Run = 0 then Run is calculated so that the height is the same one as the relative profile.", None, QtGui.QApplication.UnicodeUTF8))
        self.tree.setHeaderLabels([QtGui.QApplication.translate("Arch", "Id", None, QtGui.QApplication.UnicodeUTF8),
                                    QtGui.QApplication.translate("Arch", "Angle", None, QtGui.QApplication.UnicodeUTF8),
                                    QtGui.QApplication.translate("Arch", "Run", None, QtGui.QApplication.UnicodeUTF8),
                                    QtGui.QApplication.translate("Arch", "IdRel", None, QtGui.QApplication.UnicodeUTF8),
                                    QtGui.QApplication.translate("Arch", "Thickness", None, QtGui.QApplication.UnicodeUTF8),
                                    QtGui.QApplication.translate("Arch", "Overhang", None, QtGui.QApplication.UnicodeUTF8),
                                    QtGui.QApplication.translate("Arch", "Height", None, QtGui.QApplication.UnicodeUTF8)])

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Roof',_CommandRoof())
