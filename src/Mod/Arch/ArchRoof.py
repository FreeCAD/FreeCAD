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
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

def makeRoof2(baseobj=None,facenr=1, angles=[45.,], run = [], idrel = [0,],thickness = [1.,], overhang=[0.,], name=translate("Arch","Roof")):
    '''makeRoof(baseobj,[facenr],[angle],[name]) : Makes a roof based on a
    face from an existing object. You can provide the number of the face
    to build the roof on (default = 1), the angle (default=45) and a name (default
    = roof).'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
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
                olist.append(2.)
            obj.Overhang = olist

    obj.Face = facenr
    return obj

def makeRoof(baseobj=None,facenr=1,angle=45,name=translate("Arch","Roof")):
    '''makeRoof(baseobj,[facenr],[angle],[name]) : Makes a roof based on a
    face from an existing object. You can provide the number of the face
    to build the roof on (default = 1), the angle (default=45) and a name (default
    = roof).'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Roof(obj)
    if FreeCAD.GuiUp:
        _ViewProviderRoof(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
    obj.Face = facenr
    obj.Angle = angle
    return obj

class _CommandRoof:
    "the Arch Roof command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Roof',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Roof","Roof"),
                'Accel': "R, F",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Roof","Creates a roof object from the selected face of an object")}

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
                    FreeCADGui.doCommand("Arch.makeRoof2(FreeCAD.ActiveDocument."+obj.Name+","+str(idx)+")")
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                    return
            if obj.isDerivedFrom("Part::Feature"):
                if obj.Shape.Wires:
                    FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Roof"))
                    FreeCADGui.addModule("Arch")
                    FreeCADGui.doCommand("Arch.makeRoof2(FreeCAD.ActiveDocument."+obj.Name+")")
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
        obj.addProperty("App::PropertyFloatList","Overhang","Arch", translate("Arch","TODO:The Overhang of the roof pane."))
        obj.addProperty("App::PropertyFloatList","Heights","Arch", translate("Arch","The calculated height of the roof pane list."))
        obj.addProperty("App::PropertyInteger","Face","Base",translate("Arch","The face number of the base object used to build this roof"))
        self.Type = "Roof"
        obj.Proxy = self

    def calcHeight(self, id):
        htRel = self.profilsDico[id]["run"]*(math.tan(math.radians(self.profilsDico[id]["angle"])))
        return htRel

    def calcRun(self, id):
        lgRel = self.profilsDico[id]["height"]/(math.tan(math.radians(self.profilsDico[id]["angle"])))
        return lgRel

    def calcAngle(self, id):
        a = math.degrees(math.atan(self.profilsDico[id]["height"]/self.profilsDico[id]["run"]))
        return a

    def getPerpendicular(self, vec, angleEdge, l):
        norm = FreeCAD.Vector(0,0,1)
        perpendicular = vec.cross(norm)
        if  -180. <= angleEdge < -90.:
            perpendicular[0] = abs(perpendicular[0])*-1
            perpendicular[1] = abs(perpendicular[1])*-1
        elif   -90. <= angleEdge <= 0.:
            perpendicular[0] = abs(perpendicular[0])*-1
            perpendicular[1] = abs(perpendicular[1])
        elif 0. < angleEdge <= 90.:
            perpendicular[0] = abs(perpendicular[0])
            perpendicular[1] = abs(perpendicular[1])
        elif 90. < angleEdge <= 180.:
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
        lg = self.profilsDico[i]["run"]
        rel = self.profilsDico[i]["idrel"]
        if a == 0.0 and lg == 0.0 :
            self.profilsDico[i]["run"] = self.profilsDico[rel]["run"]
            self.profilsDico[i]["angle"] = self.profilsDico[rel]["angle"]
            self.profilsDico[i]["height"] = self.calcHeight(i)
        elif lg == 0:
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
                    shps = []
                    heights = []
                    edges = DraftGeomUtils.sortEdges(w.Edges)
                    l = len(edges)
                    for i in range(l):
                        self.makeRoofProfilsDic(i, obj.Angles[i], obj.Runs[i], obj.IdRel[i], obj.Overhang[i], obj.Thickness[i])
                    for i in range(l):
                        self.calcMissingData(i)
                    for p in self.profilsDico:
                        heights.append(p["height"])
                    obj.Heights = heights
                    """
                    for p in self.profilsDico:
                        print p

                    print obj.Angles
                    print obj.Runs
                    print obj.IdRel
                    print obj.Overhang
                    print obj.Thickness
                    print obj.Heights
                    """
                    for i in range(l):
                        edgesForward = edges[:]
                        edgesForward.append(edges[0])
                        edgesBack = edges[:]
                        edgesBack.insert(0,edges[-1])
                        points=[]
                        profil0 =self.profilsDico[i-1]
                        profil1 =self.profilsDico[i]
                        if i == l-1:
                            profil2 =self.profilsDico[0]
                        else:
                            profil2 =self.profilsDico[i+1]
                        vec0 = edges[i-1].Vertexes[-1].Point.sub(edges[i-1].Vertexes[0].Point)
                        vec1 = edges[i].Vertexes[-1].Point.sub(edges[i].Vertexes[0].Point)
                        vec2 = edgesForward[i+1].Vertexes[-1].Point.sub(edgesForward[i+1].Vertexes[0].Point)
                        angleEdge0 = math.degrees(DraftVecUtils.angle(vec0))
                        angleEdge1 = math.degrees(DraftVecUtils.angle(vec1))
                        angleEdge2 = math.degrees(DraftVecUtils.angle(vec2))
                        points=[edges[i].Vertexes[0].Point,edges[i].Vertexes[-1].Point]
                        if profil1["angle"] != 90.:
                            lg = profil1["run"]
                            faitage = DraftGeomUtils.offset(edges[i],self.getPerpendicular(vec1,angleEdge1,lg))
                            midpoint = DraftGeomUtils.findMidpoint(edges[i])
                            if profil2["angle"] == 90. :
                                edge = DraftGeomUtils.offset(edgesForward[i+1],FreeCAD.Vector(0,0,0))
                                point = DraftGeomUtils.findIntersection(faitage,edge,infinite1=True,infinite2=True,)
                                points.append(FreeCAD.Vector(point[0]))
                            elif profil2["height"] == profil1["height"]:
                                edge = DraftGeomUtils.offset(edgesForward[i+1],self.getPerpendicular(vec2,angleEdge2,profil2["run"]))
                                point = DraftGeomUtils.findIntersection(faitage,edge,infinite1=True,infinite2=True,)
                                points.append(FreeCAD.Vector(point[0]))
                            elif profil1["height"] > profil2["height"]:
                                edge = DraftGeomUtils.offset(edgesForward[i+1],self.getPerpendicular(vec2,angleEdge2,profil2["run"]))
                                dec = profil2["height"]/math.tan(math.radians(profil1["angle"]))
                                edge1 = DraftGeomUtils.offset(edges[i],self.getPerpendicular(vec1,angleEdge1,dec))
                                pointR = DraftGeomUtils.findIntersection(edge,edge1,infinite1=True,infinite2=True,)
                                points.append(FreeCAD.Vector(pointR[0]))
                                point = DraftGeomUtils.findIntersection(faitage,edge,infinite1=True,infinite2=True,)
                                points.append(FreeCAD.Vector(point[0]))
                            elif profil1["height"] < profil2["height"]:
                                dec = profil1["height"]/math.tan(math.radians(profil2["angle"]))
                                edge1 = DraftGeomUtils.offset(edgesForward[i+1],self.getPerpendicular(vec2,angleEdge2,dec))
                                point = DraftGeomUtils.findIntersection(faitage,edge1,infinite1=True,infinite2=True,)
                                points.append(FreeCAD.Vector(point[0]))
                            else:
                                print("Cas de figure non pris en charge")
                            if profil0["angle"] == 90. :
                                edge = edgesBack[i]
                                point = DraftGeomUtils.findIntersection(faitage,edge,infinite1=True,infinite2=True,)
                                points.append(FreeCAD.Vector(point[0]))
                            elif profil0["height"] == profil1["height"]:
                                edge = DraftGeomUtils.offset(edgesBack[i],self.getPerpendicular(vec0,angleEdge0,profil0["run"]))
                                point = DraftGeomUtils.findIntersection(faitage,edge,infinite1=True,infinite2=True,)
                                points.append(FreeCAD.Vector(point[0]))
                            elif profil1["height"] > profil0["height"]:
                                dec = profil0["height"]/math.tan(math.radians(profil1["angle"]))
                                edge1 = DraftGeomUtils.offset(edges[i],self.getPerpendicular(vec1,angleEdge1,dec))
                                edge = DraftGeomUtils.offset(edges[i-1],self.getPerpendicular(vec0,angleEdge0,profil0["run"]))
                                point = DraftGeomUtils.findIntersection(edge,faitage,infinite1=True,infinite2=True,)
                                points.append(FreeCAD.Vector(point[0]))
                                point = DraftGeomUtils.findIntersection(edge1,edge,infinite1=True,infinite2=True,)
                                points.append(FreeCAD.Vector(point[0]))
                            elif profil1["height"] < profil0["height"]:
                                dec = profil1["height"]/math.tan(math.radians(profil0["angle"]))
                                edge1 = DraftGeomUtils.offset(edges[i-1],self.getPerpendicular(vec0,angleEdge0,dec))
                                point = DraftGeomUtils.findIntersection(faitage,edge1,infinite1=True,infinite2=True,)
                                points.append(FreeCAD.Vector(point[0]))
                            else:
                                print("Cas de figure non pris en charge")
                            points = DraftVecUtils.removeDoubles(points)
                            self.profilsDico[i]["points"] = points
                            lp = len(points)
                            points.append(points[0])
                            edgesWire = []
                            for i in range(lp):
                                edge = Part.makeLine(points[i],points[i+1])
                                edgesWire.append(edge)
                            wire = Part.Wire(edgesWire)
                            d = wire.BoundBox.DiagonalLength
                            thicknessV = profil1["thickness"]/(math.cos(math.radians(profil1["angle"])))
                            overhangV = profil1["overhang"]*math.tan(math.radians(profil1["angle"]))
                            if wire.isClosed():
                                f = Part.Face(wire)
                                f = f.extrude(FreeCAD.Vector(0,0,profil1["height"]+2*thicknessV))
                            points=[FreeCAD.Vector(-profil1["overhang"],-overhangV,0.0),FreeCAD.Vector(profil1["run"],profil1["height"],0.0),FreeCAD.Vector(profil1["run"],profil1["height"]+thicknessV,0.0),FreeCAD.Vector(-profil1["overhang"],-overhangV+thicknessV,0.0)]
                            lp = len(points)
                            points.append(points[0])
                            edgesWire = []
                            for i in range(lp):
                                edge = Part.makeLine(points[i],points[i+1])
                                edgesWire.append(edge)
                            profilCouv = Part.Wire(edgesWire)
                            profilCouv.translate(midpoint)
                            profilCouv.rotate(midpoint,FreeCAD.Vector(0,0,1), 90. + angleEdge1 * -1)
                            perp = self.getPerpendicular(vec1,angleEdge1,profil1["run"])
                            profilCouv.rotate(midpoint,perp,90.)
                            vecT = vec1.normalize()
                            vecT.multiply(d)
                            profilCouv.translate(vecT)
                            vecE = vecT.multiply(-2.)
                            pC = Part.Face(profilCouv)
                            profilVol = pC.extrude(vecE)
                            #Part.show(profilVol)
                            panVol = f.common(profilVol)
                            #Part.show(panVol)
                            shps.append(panVol)
                        else:
                            #TODO PIGNON
                            pass
                    base = shps.pop()
                    for s in shps:
                        base = base.fuse(s)
                    base = base.removeSplitter()
                    if not base.isNull():
                        if not DraftGeomUtils.isNull(pl):
                            base.Placement = pl

        base = self.processSubShapes(obj,base)
        if base:
            if not base.isNull():
                obj.Shape = base

    def getSubVolume(self,obj,extension=10000):
        "returns a volume to be subtracted"
        if hasattr(self,"baseface"):
            if self.baseface:
                norm = self.baseface.normalAt(0,0)
                norm = DraftVecUtils.scaleTo(norm,extension)
                return self.baseface.extrude(norm)
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
            if float(it.text(4)) == 0.:
                thick.append(1.)
            else:
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
        self.title.setText(QtGui.QApplication.translate("Arch", "Parameters of the profiles of the roof:\n* Angle : slope in degrees compared to the horizontal one.\n* Run : outdistance between the wall and the ridge sheathing.\n* Thickness : thickness of the side of roof.\n* Overhang : outdistance between the sewer and the wall.\n* Height : height of the ridge sheathing (calculated automatically)\n* IdRel : Relative Id for calculations automatic.\n---\nIf Angle = 0 and Run = the 0 then profile is identical to the relative profile.\nIf Angle = the 0 then angle is calculated so that the height is the same one as the relative profile.\nIf Run = 0 then Run is calculated so that the height is the same one as the relative profile.", None, QtGui.QApplication.UnicodeUTF8))
        self.tree.setHeaderLabels([QtGui.QApplication.translate("Arch", "Id", None, QtGui.QApplication.UnicodeUTF8),
                                    QtGui.QApplication.translate("Arch", "Angle", None, QtGui.QApplication.UnicodeUTF8),
                                    QtGui.QApplication.translate("Arch", "Run", None, QtGui.QApplication.UnicodeUTF8),
                                    QtGui.QApplication.translate("Arch", "IdRel", None, QtGui.QApplication.UnicodeUTF8),
                                    QtGui.QApplication.translate("Arch", "Thickness", None, QtGui.QApplication.UnicodeUTF8),
                                    QtGui.QApplication.translate("Arch", "Overhang", None, QtGui.QApplication.UnicodeUTF8),
                                    QtGui.QApplication.translate("Arch", "Height", None, QtGui.QApplication.UnicodeUTF8)])

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Roof',_CommandRoof())
