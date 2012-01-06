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

import FreeCAD,FreeCADGui,Draft,ArchComponent
from draftlibs import fcvec
from FreeCAD import Vector
from PyQt4 import QtCore

__title__="FreeCAD Arch Commands"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

# module functions ###############################################

def addComponents(objectsList,host):
    '''addComponents(objectsList,hostObject): adds the given object or the objects
    from the given list as components to the given host Object. Use this for
    example to add windows to a wall, or to add walls to a cell or floor.'''
    if not isinstance(objectsList,list):
        objectsList = [objectsList]
    tp = Draft.getType(host)
    if tp in ["Cell","Floor","Building","Site"]:
        c = host.Components
        for o in objectsList:
            if not o in c:
                c.append(o)
        host.Components = c
    elif tp in ["Wall","Structure"]:
        a = host.Additions
        for o in objectsList:
            if not o in a:
                if hasattr(o,"Shape"):
                    a.append(o)
        host.Additions = a
    elif tp in ["SectionPlane"]:
        a = host.Objects
        for o in objectsList:
            if not o in a:
                if hasattr(o,"Shape"):
                    a.append(o)
        host.Objects = a

def removeComponents(objectsList,host=None):
    '''removeComponents(objectsList,[hostObject]): removes the given component or
    the components from the given list from their parents. If a host object is
    specified, this function will try adding the components as holes to the host
    object instead.'''
    if not isinstance(objectsList,list):
        objectsList = [objectsList]
    if host:
        if Draft.getType(host) in ["Wall","Structure"]:
            s = host.Subtractions
            for o in objectsList:
                if not o in s:
                    s.append(o)
            host.Subtractions = s
    else:
        for o in objectsList:
            if o.InList:
               h = o.InList[0]
               tp = Draft.getType(h)
               if tp in ["Cell","Floor","Building","Site"]:
                   c = h.Components
                   if o in c:
                       c.remove(o)
                       h.Components = c
                       o.ViewObject.show()
               elif tp in ["Wall","Structure"]:
                   a = h.Additions
                   s = h.Subtractions
                   if o in a:
                       a.remove(o)
                       h.Additions = a
                       o.ViewObject.show()
                   elif o in s:
                       s.remove(o)
                       h.Subtractions = s
                       o.ViewObject.show()
               elif tp in ["SectionPlane"]:
                   a = h.Objects
                   if o in a:
                       a.remove(o)
                       h.Objects = a

def copyProperties(obj1,obj2):
    '''copyProperties(obj1,obj2): Copies properties values from obj1 to obj2,
    when that property exists in both objects'''
    for prop in obj1.PropertiesList:
        if prop in obj2.PropertiesList:
            if not prop in ["Proxy","Shape"]:
                setattr(obj2,prop,getattr(obj1,prop))
    if obj1.ViewObject and obj2.ViewObject:
        for prop in obj1.ViewObject.PropertiesList:
            if prop in obj2.ViewObject.PropertiesList:
                if not prop in ["Proxy","Shape"]:
                    setattr(obj2.ViewObject,prop,getattr(obj1.ViewObject,prop))
                       
def splitMesh(obj,mark=True):
    '''splitMesh(object,[mark]): splits the given mesh object into separated components.
    If mark is False, nothing else is done. If True (default), non-manifold components
    will be painted in red.'''
    if not obj.isDerivedFrom("Mesh::Feature"): return []
    basemesh = obj.Mesh
    comps = basemesh.getSeparateComponents()
    nlist = []
    if comps:
        basename = obj.Name
        FreeCAD.ActiveDocument.removeObject(basename)
        for c in comps:
            newobj = FreeCAD.ActiveDocument.addObject("Mesh::Feature",basename)
            newobj.Mesh = c
            if mark and (not(c.isSolid()) or c.hasNonManifolds()):
                newobj.ViewObject.ShapeColor = (1.0,0.0,0.0,1.0)
            nlist.append(newobj)
        return nlist
    return [obj]

def meshToShape(obj,mark=True):
    '''meshToShape(object,[mark]): turns a mesh into a shape, joining coplanar facets. If
    mark is True (default), non-solid objects will be marked in red'''
    name = obj.Name
    import Part,MeshPart
    from draftlibs import fcgeo
    if "Mesh" in obj.PropertiesList:
        faces = []	
        mesh = obj.Mesh
        plac = obj.Placement
        segments = mesh.getPlanes(0.001) # use rather strict tolerance here
        print len(segments)," segments ",segments
        for i in segments:
            print "treating",segments.index(i),i
            if len(i) > 0:
                wires = MeshPart.wireFromSegment(mesh, i)
                print "wire done"
                print wires
                if len(wires) > 1:
                    # a segment can have inner holes
                    print "inner wires found"
                    ext = None
                    max_length = 0
                    # cleaning up rubbish in wires
                    for i in range(len(wires)):
                        wires[i] = fcgeo.removeInterVertices(wires[i])
                    for w in wires:
                        # we assume that the exterior boundary is that one with
                        # the biggest bounding box
                        if w.BoundBox.DiagonalLength > max_length:
                            max_length = w.BoundBox.DiagonalLength
                            ext = w
                    print "exterior wire",ext
                    wires.remove(ext)
                    # all interior wires mark a hole and must reverse
                    # their orientation, otherwise Part.Face fails
                    for w in wires:
                        print "reversing",w
                        #w.reverse()
                        print "reversed"
                    # make sure that the exterior wires comes as first in the list
                    wires.insert(0, ext)
                    print "done sorting", wires
                if wires:
                    faces.append(Part.Face(wires))
                print "done facing"
            print "faces",faces

        try:
            se = Part.makeShell(faces)
            solid = Part.Solid(se)
        except:
            pass
        else:
            if solid.isClosed():
                FreeCAD.ActiveDocument.removeObject(name)
            else:
                if mark:
                    newobj.ViewObject.ShapeColor = (1.0,0.0,0.0,1.0)
            newobj = FreeCAD.ActiveDocument.addObject("Part::Feature",name)
            newobj.Shape = solid
            newobj.Placement = plac
            return newobj
    return None

def removeShape(objs,mark=True):
    '''takes an arch object (wall or structure) built on a cubic shape, and removes
    the inner shape, keeping its length, width and height as parameters.'''
    from draftlibs import fcgeo
    if not isinstance(objs,list):
        objs = [objs]
    for obj in objs:
        if fcgeo.isCubic(obj.Shape):
            dims = fcgeo.getCubicDimensions(obj.Shape)
            if dims:
                name = obj.Name
                tp = Draft.getType(obj)
                print tp
                if tp == "Structure":
                    FreeCAD.ActiveDocument.removeObject(name)
                    import Structure
                    str = Structure.makeStructure(length=dims[1],width=dims[2],height=dims[3],name=name)
                    str.Placement = dims[0]
                elif tp == "Wall":
                    FreeCAD.ActiveDocument.removeObject(name)
                    import Wall
                    length = dims[1]
                    width = dims[2]
                    v1 = Vector(length/2,0,0)
                    v2 = fcvec.neg(v1)
                    v1 = dims[0].multVec(v1)
                    v2 = dims[0].multVec(v2)
                    line = Draft.makeLine(v1,v2)
                    wal = Wall.makeWall(line,width=width,height=dims[3],name=name)
        else:
            if mark:
                obj.ViewObject.ShapeColor = (1.0,0.0,0.0,1.0)

def mergeCells(objectslist):
    '''mergeCells(objectslist): merges the objects in the given list
    into one. All objects must be of the same type and based on the Cell
    object (cells, floors, buildings, or sites).'''
    if not objectslist:
        return None
    if not isinstance(objectslist,list):
        return None
    if len(objectslist) < 2:
        return None
    typ = Draft.getType(objectslist[0])
    if not(typ in ["Cell","Floor","Building","Site"]):
               return None
    for o in objectslist:
        if Draft.getType(o) != typ:
            return None
    base = objectslist.pop(0)
    for o in objectslist:
        l = base.Components
        for c in o.Components:
            if not c in l:
                l.append(c)
        base.Components = l
        FreeCAD.ActiveDocument.removeObject(o.Name)
    FreeCAD.ActiveDocument.recompute()
    return base
    
# command definitions ###############################################
                       
class _CommandAdd:
    "the Arch Add command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Add',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Add","Add component"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Add","Adds the selected components to the active object")}

    def IsActive(self):
        if len(FreeCADGui.Selection.getSelection()) > 1:
            return True
        else:
            return False
        
    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        FreeCAD.ActiveDocument.openTransaction("Grouping")
        if not mergeCells(sel):
            host = sel.pop()
            addComponents(sel,host)
        FreeCAD.ActiveDocument.commitTransaction()
        
class _CommandRemove:
    "the Arch Add command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Remove',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Remove","Remove component"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Remove","Remove the selected components from their parents, or create a hole in a component")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False
        
    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        FreeCAD.ActiveDocument.openTransaction("Ungrouping")
        if Draft.getType(sel[-1]) in ["Wall","Structure"]:
            host = sel.pop()
            removeComponents(sel,host)
        else:
            removeComponents(sel)
        FreeCAD.ActiveDocument.commitTransaction()


class _CommandSplitMesh:
    "the Arch SplitMesh command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_SplitMesh',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_SplitMesh","Split Mesh"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_SplitMesh","Splits selected meshes into independent components")}

    def IsActive(self):
        if len(FreeCADGui.Selection.getSelection()):
            return True
        else:
            return False
        
    def Activated(self):
        if FreeCADGui.Selection.getSelection():
            sel = FreeCADGui.Selection.getSelection()
            FreeCAD.ActiveDocument.openTransaction("Split Mesh")
            for obj in sel:
                n = obj.Name
                nobjs = splitMesh(obj)
                if len(nobjs) > 1:
                    g = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup",n)
                    for o in nobjs:
                        g.addObject(o)
            FreeCAD.ActiveDocument.commitTransaction()

            
class _CommandMeshToShape:
    "the Arch MeshToShape command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_MeshToShape',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_MeshToShape","Mesh to Shape"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_MeshToPart","Turns selected meshes into Part Shape objects")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False
        
    def Activated(self):
        if FreeCADGui.Selection.getSelection():
            f = FreeCADGui.Selection.getSelection()[0]
            g = None
            if f.isDerivedFrom("App::DocumentObjectGroup"):
                g = f
                FreeCADGui.Selection.clearSelection()
                for o in f.OutList:
                    FreeCADGui.Selection.addSelection(o)
            else:
                if f.InList:
                    if f.InList[0].isDerivedFrom("App::DocumentObjectGroup"):
                        g = f.InList[0]
            FreeCAD.ActiveDocument.openTransaction("Mesh to Shape")
            for obj in FreeCADGui.Selection.getSelection():
                newobj = meshToShape(obj)
                if g and newobj:
                    g.addObject(newobj)
            FreeCAD.ActiveDocument.commitTransaction()

class _CommandSelectNonSolidMeshes:
    "the Arch SelectNonSolidMeshes command definition"
    def GetResources(self):
        return {'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_SelectNonSolidMeshes","Select non-manifold meshes"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_SelectNonSolidMeshes","Selects all non-manifold meshes from the document or from the selected groups")}
        
    def Activated(self):
        msel = []
        if FreeCADGui.Selection.getSelection():
            for o in FreeCADGui.Selection.getSelection():
                if o.isDerivedFrom("App::DocumentObjectGroup"):
                    msel.extend(o.OutList)
        if not msel:
            msel = FreeCAD.ActiveDocument.Objects
        sel = []
        for o in msel:
            if o.isDerivedFrom("Mesh::Feature"):
                if (not o.Mesh.isSolid()) or o.Mesh.hasNonManifolds():
                    sel.append(o)
        if sel:
            FreeCADGui.Selection.clearSelection()
            for o in sel:
                FreeCADGui.Selection.addSelection(o)

class _CommandRemoveShape:
    "the Arch RemoveShape command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_RemoveShape',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_RemoveShape","Remove Shape from Arch"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_RemoveShape","Removes cubic shapes from Arch components")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False
        
    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        removeShape(sel)

FreeCADGui.addCommand('Arch_Add',_CommandAdd())
FreeCADGui.addCommand('Arch_Remove',_CommandRemove())
FreeCADGui.addCommand('Arch_SplitMesh',_CommandSplitMesh())
FreeCADGui.addCommand('Arch_MeshToShape',_CommandMeshToShape())
FreeCADGui.addCommand('Arch_SelectNonSolidMeshes',_CommandSelectNonSolidMeshes())
FreeCADGui.addCommand('Arch_RemoveShape',_CommandRemoveShape())
