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

import FreeCAD,Draft,ArchComponent,DraftVecUtils,ArchCommands,Units
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

# presets
Roles = ["Furniture", "Hydro Equipment", "Electric Equipment"]


def makeEquipment(baseobj=None,placement=None,name=translate("Arch","Equipment"),type=None):
    "makeEquipment([baseobj,placement,name,type]): creates an equipment object from the given base object"
    if type:
        if type == "Part":
            obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
        else:
            obj = FreeCAD.ActiveDocument.addObject("Mesh::FeaturePython",name)
        _Equipment(obj)
        if baseobj:
            obj.Base = baseobj
    else:
        if baseobj:
            if baseobj.isDerivedFrom("Mesh::Feature"):
                obj = FreeCAD.ActiveDocument.addObject("Mesh::FeaturePython",name)
            else:
                obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
            _Equipment(obj)
            obj.Base = baseobj
        else:
            obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
            _Equipment(obj)
    obj.Label = name
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
        #print c
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
        #print p,len(p.Vertexes),p.isClosed()
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
                print "Unable to produce a face from the outer wire."
            else:
                shape = f
                
    return shape

        
class _CommandEquipment:
    "the Arch Equipment command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Equipment',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Equipment","Equipment"),
                'Accel': "E, Q",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Equipment","Creates an equipment object from a selected object (Part or Mesh)")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self): 
        s = FreeCADGui.Selection.getSelection()
        if not s:
            FreeCAD.Console.PrintError(translate("Arch","You must select a base object first!"))
        else:
            base = s[0].Name
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Equipment")))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("Arch.makeEquipment(FreeCAD.ActiveDocument." + base + ")")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
            # get diffuse color info from base object
            if hasattr(s[0].ViewObject,"DiffuseColor"):
                FreeCADGui.doCommand("FreeCAD.ActiveDocument.Objects[-1].ViewObject.DiffuseColor = FreeCAD.ActiveDocument." + base + ".ViewObject.DiffuseColor")
        return
        
        
class _Command3Views:
    "the Arch 3Views command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_3Views',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_3Views","3 views from mesh"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_3Views","Creates 3 views (top, front, side) from a mesh-based object")}

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
        obj.addProperty("Part::PropertyPartShape","TopView","Arch",translate("Arch","an optional 2D shape representing a top view of this equipment"))
        obj.addProperty("Part::PropertyPartShape","FrontView","Arch",translate("Arch","an optional 2D shape representing a front view of this equipment"))
        obj.addProperty("Part::PropertyPartShape","SideView","Arch",translate("Arch","an optional 2D shape representing a side view of this equipment"))
        obj.addProperty("App::PropertyString","Model","Arch",translate("Arch","The model description of this equipment"))
        obj.addProperty("App::PropertyString","Url","Arch",translate("Arch","The url of the product page of this equipment"))
        self.Type = "Equipment"
        obj.Role = Roles
        obj.Proxy = self
        
    def onChanged(self,obj,prop):
        self.hideSubobjects(obj,prop)
        
    def execute(self,obj):
        pl = obj.Placement
        if obj.Base:
            if obj.isDerivedFrom("Mesh::Feature"):
                m = None
                if obj.Base.isDerivedFrom("Part::Feature"):
                    base = obj.Base.Shape.copy()
                    base = self.processSubShapes(obj,base,pl)
                    if base:
                        import Mesh
                        m = Mesh.Mesh(base.tessellate(1))
                        
                elif obj.Base.isDerivedFrom("Mesh::Feature"):
                    m = obj.Base.Mesh.copy()
                if m:
                    if not pl.isNull():
                        m.Placement = pl
                    obj.Mesh = m
            else:
                base = None
                if obj.Base.isDerivedFrom("Part::Feature"):
                    base = obj.Base.Shape.copy()
                elif obj.Base.isDerivedFrom("Mesh::Feature"):
                    import Part
                    base = Part.Shape()
                    base.makeShapeFromMesh(obj.Base.Mesh.Topology,0.05)
                    base = base.removeSplitteR()
                if base:
                    base = self.processSubShapes(obj,base,pl)
                    self.applyShape(obj,base,pl)


class _ViewProviderEquipment(ArchComponent.ViewProviderComponent):
    "A View Provider for the Equipment object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Equipment_Tree.svg"
        

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Equipment',_CommandEquipment())
    FreeCADGui.addCommand('Arch_3Views',   _Command3Views())
