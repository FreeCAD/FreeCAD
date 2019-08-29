# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014                                                    *
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

__title__="FreeCAD Equipment"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

import FreeCAD,Draft,ArchComponent,DraftVecUtils,ArchCommands
from FreeCAD import Units
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

## @package ArchEquipment
#  \ingroup ARCH
#  \brief The Equipment object and tools
#
#  This module provides tools to build equipment objects.
#  Equipment is used to represent furniture and all kinds of electrical
#  or hydraulic appliances in a building


def makeEquipment(baseobj=None,placement=None,name="Equipment"):

    "makeEquipment([baseobj,placement,name]): creates an equipment object from the given base object."
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Equipment")
    _Equipment(obj)
    if baseobj:
        if baseobj.isDerivedFrom("Mesh::Feature"):
            obj.Mesh = baseobj
        else:
            obj.Base = baseobj
    obj.Label = translate("Arch",name)
    if placement:
        obj.Placement = placement
    if FreeCAD.GuiUp:
        _ViewProviderEquipment(obj.ViewObject)
        if baseobj:
            baseobj.ViewObject.hide()
    return obj

def createMeshView(obj,direction=FreeCAD.Vector(0,0,-1),outeronly=False,largestonly=False):

    """createMeshView(obj,[direction,outeronly,largestonly]): creates a flat shape that is the
    projection of the given mesh object in the given direction (default = on the XY plane). If
    outeronly is True, only the outer contour is taken into consideration, discarding the inner
    holes. If largestonly is True, only the largest segment of the given mesh will be used."""

    import Mesh, math, Part, DraftGeomUtils
    if not obj.isDerivedFrom("Mesh::Feature"):
        return
    mesh = obj.Mesh

    # 1. Flattening the mesh
    proj = []
    for f in mesh.Facets:
        nf = []
        for v in f.Points:
            v = FreeCAD.Vector(v)
            a = v.negative().getAngle(direction)
            l = math.cos(a)*v.Length
            p = v.add(FreeCAD.Vector(direction).multiply(l))
            p = DraftVecUtils.rounded(p)
            nf.append(p)
        proj.append(nf)
    flatmesh = Mesh.Mesh(proj)

    # 2. Removing wrong faces
    facets = []
    for f in flatmesh.Facets:
        if f.Normal.getAngle(direction) < math.pi:
            facets.append(f)
    cleanmesh = Mesh.Mesh(facets)

    #Mesh.show(cleanmesh)

    # 3. Getting the bigger mesh from the planar segments
    if largestonly:
        c = cleanmesh.getSeparateComponents()
        #print(c)
        cleanmesh = c[0]
        segs = cleanmesh.getPlanarSegments(1)
        meshes = []
        for s in segs:
            f = [cleanmesh.Facets[i] for i in s]
            meshes.append(Mesh.Mesh(f))
        a = 0
        for m in meshes:
            if m.Area > a:
                boundarymesh = m
                a = m.Area
        #Mesh.show(boundarymesh)
        cleanmesh = boundarymesh

    # 4. Creating a Part and getting the contour

    shape = None
    for f in cleanmesh.Facets:
        p = Part.makePolygon(f.Points+[f.Points[0]])
        #print(p,len(p.Vertexes),p.isClosed())
        try:
            p = Part.Face(p)
            if shape:
                shape = shape.fuse(p)
            else:
                shape = p
        except Part.OCCError:
            pass
    shape = shape.removeSplitter()

    # 5. Extracting the largest wire

    if outeronly:
        count = 0
        largest = None
        for w in shape.Wires:
            if len(w.Vertexes) > count:
                count = len(w.Vertexes)
                largest = w
        if largest:
            try:
                f = Part.Face(w)
            except Part.OCCError:
                print("Unable to produce a face from the outer wire.")
            else:
                shape = f

    return shape


class _CommandEquipment:

    "the Arch Equipment command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Equipment',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Equipment","Equipment"),
                'Accel': "E, Q",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Equipment","Creates an equipment object from a selected object (Part or Mesh)")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        s = FreeCADGui.Selection.getSelection()
        if not s:
            FreeCAD.Console.PrintError(translate("Arch","You must select a base shape object and optionally a mesh object"))
        else:
            base = ""
            mesh = ""
            if len(s) == 2:
                if s[0].isDerivedFrom("Part::Feature"):
                    base = s[0].Name
                elif s[0].isDerivedFrom("Mesh::Feature"):
                    mesh = s[0].Name
                if s[1].isDerivedFrom("Part::Feature"):
                    if mesh:
                        base = s[1].Name
                elif s[1].isDerivedFrom("Mesh::Feature"):
                    if base:
                        mesh = s[1].Name
            else:
                if s[0].isDerivedFrom("Part::Feature"):
                    base = s[0].Name
                elif s[0].isDerivedFrom("Mesh::Feature"):
                    mesh = s[0].Name
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Equipment")))
            FreeCADGui.addModule("Arch")
            if base:
                base = "FreeCAD.ActiveDocument." + base
            FreeCADGui.doCommand("obj = Arch.makeEquipment(" + base + ")")
            if mesh:
                FreeCADGui.doCommand("obj.Mesh = FreeCAD.ActiveDocument." + mesh)
            FreeCADGui.addModule("Draft")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
            # get diffuse color info from base object
            if base and hasattr(s[0].ViewObject,"DiffuseColor"):
                FreeCADGui.doCommand("FreeCAD.ActiveDocument.Objects[-1].ViewObject.DiffuseColor = " + base + ".ViewObject.DiffuseColor")
        return


class _Command3Views:

    "the Arch 3Views command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_3Views',
                'MenuText': QT_TRANSLATE_NOOP("Arch_3Views","3 views from mesh"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_3Views","Creates 3 views (top, front, side) from a mesh-based object")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        s = FreeCADGui.Selection.getSelection()
        if len(s) != 1:
            FreeCAD.Console.PrintError(translate("Arch","You must select exactly one base object"))
        else:
            obj = s[0]
            if not obj.isDerivedFrom("Mesh::Feature"):
                FreeCAD.Console.PrintError(translate("Arch","The selected object must be a mesh"))
            else:
                if obj.Mesh.CountFacets > 1000:
                    msgBox = QtGui.QMessageBox()
                    msgBox.setText(translate("Arch","This mesh has more than 1000 facets."))
                    msgBox.setInformativeText(translate("Arch","This operation can take a long time. Proceed?"))
                    msgBox.setStandardButtons(QtGui.QMessageBox.Ok | QtGui.QMessageBox.Cancel)
                    msgBox.setDefaultButton(QtGui.QMessageBox.Cancel)
                    ret = msgBox.exec_()
                    if ret == QtGui.QMessageBox.Cancel:
                        return
                elif obj.Mesh.CountFacets >= 500:
                    FreeCAD.Console.PrintWarning(translate("Arch","The mesh has more than 500 facets. This will take a couple of minutes..."))
                FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create 3 views")))
                FreeCADGui.addModule("Arch")
                FreeCADGui.addModule("Part")
                FreeCADGui.doCommand("s1 = Arch.createMeshView(FreeCAD.ActiveDocument." + obj.Name + ",FreeCAD.Vector(0,0,-1),outeronly=False,largestonly=False)")
                FreeCADGui.doCommand("Part.show(s1)")
                FreeCADGui.doCommand("s2 = Arch.createMeshView(FreeCAD.ActiveDocument." + obj.Name + ",FreeCAD.Vector(1,0,0),outeronly=False,largestonly=False)")
                FreeCADGui.doCommand("Part.show(s2)")
                FreeCADGui.doCommand("s3 = Arch.createMeshView(FreeCAD.ActiveDocument." + obj.Name + ",FreeCAD.Vector(0,1,0),outeronly=False,largestonly=False)")
                FreeCADGui.doCommand("Part.show(s3)")
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
        return


class _Equipment(ArchComponent.Component):

    "The Equipment object"

    def __init__(self,obj):

        ArchComponent.Component.__init__(self,obj)
        obj.Proxy = self
        self.setProperties(obj)
        obj.IfcType = "Furniture"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Model" in pl:
            obj.addProperty("App::PropertyString","Model","Equipment",QT_TRANSLATE_NOOP("App::Property","The model description of this equipment"))
        if not "ProductURL" in pl:
            obj.addProperty("App::PropertyString","ProductURL","Equipment",QT_TRANSLATE_NOOP("App::Property","The URL of the product page of this equipment"))
        if not "StandardCode" in pl:
            obj.addProperty("App::PropertyString","StandardCode","Equipment",QT_TRANSLATE_NOOP("App::Property","A standard code (MasterFormat, OmniClass,...)"))
        if not "SnapPoints" in pl:
            obj.addProperty("App::PropertyVectorList","SnapPoints","Equipment",QT_TRANSLATE_NOOP("App::Property","Additional snap points for this equipment"))
        if not "EquipmentPower" in pl:
            obj.addProperty("App::PropertyFloat","EquipmentPower","Equipment",QT_TRANSLATE_NOOP("App::Property","The electric power needed by this equipment in Watts"))
        obj.setEditorMode("VerticalArea",2)
        obj.setEditorMode("HorizontalArea",2)
        obj.setEditorMode("PerimeterLength",2)
        self.Type = "Equipment"

    def onDocumentRestored(self,obj):

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def onChanged(self,obj,prop):

        self.hideSubobjects(obj,prop)
        ArchComponent.Component.onChanged(self,obj,prop)

    def execute(self,obj):

        if self.clone(obj):
            return

        pl = obj.Placement
        if obj.Base:
            base = None
            if obj.Base.isDerivedFrom("Part::Feature"):
                base = obj.Base.Shape.copy()
                base = self.processSubShapes(obj,base,pl)
                self.applyShape(obj,base,pl,allowinvalid=False,allownosolid=True)

    def computeAreas(self,obj):
        return


class _ViewProviderEquipment(ArchComponent.ViewProviderComponent):

    "A View Provider for the Equipment object"

    def __init__(self,vobj):

        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):

        import Arch_rc
        if hasattr(self,"Object"):
            if hasattr(self.Object,"CloneOf"):
                if self.Object.CloneOf:
                    return ":/icons/Arch_Equipment_Clone.svg"
        return ":/icons/Arch_Equipment_Tree.svg"

    def attach(self, vobj):

        self.Object = vobj.Object
        from pivy import coin
        sep = coin.SoSeparator()
        self.coords = coin.SoCoordinate3()
        sep.addChild(self.coords)
        self.coords.point.deleteValues(0)
        symbol = coin.SoMarkerSet()
        symbol.markerIndex = FreeCADGui.getMarkerIndex("", 5)
        sep.addChild(symbol)
        rn = vobj.RootNode
        rn.addChild(sep)
        ArchComponent.ViewProviderComponent.attach(self,vobj)

    def updateData(self, obj, prop):

        if prop == "SnapPoints":
            if obj.SnapPoints:
                self.coords.point.setNum(len(obj.SnapPoints))
                self.coords.point.setValues([[p.x,p.y,p.z] for p in obj.SnapPoints])
            else:
                self.coords.point.deleteValues(0)


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Equipment',_CommandEquipment())
    FreeCADGui.addCommand('Arch_3Views',   _Command3Views())
