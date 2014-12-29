# -*- coding: utf8 -*-

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

import FreeCAD,Draft,ArchComponent,DraftVecUtils
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui,QtCore
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

__title__="FreeCAD Arch Commands"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

# module functions ###############################################

def getStringList(objects):
    '''getStringList(objects): returns a string defining a list
    of objects'''
    result = "["
    for o in objects:
        if len(result) > 1:
            result += ","
        result += "FreeCAD.ActiveDocument." + o.Name
    result += "]"
    return result

def getDefaultColor(objectType):
    '''getDefaultColor(string): returns a color value for the given object
    type (Wall, Structure, Window, WindowGlass)'''
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    if objectType == "Wall":
        c = p.GetUnsigned("WallColor",4294967295)
    elif objectType == "Structure":
        c = p.GetUnsigned("StructureColor",2847259391)
    elif objectType == "WindowGlass":
        c = p.GetUnsigned("WindowGlassColor",1772731135)
    elif objectType == "Rebar":
        c = p.GetUnsigned("RebarColor",3111475967)
    elif objectType == "Panel":
        c = p.GetUnsigned("PanelColor",3416289279)
    else:
        c = p.GetUnsigned("WindowsColor",810781695)
    r = float((c>>24)&0xFF)/255.0
    g = float((c>>16)&0xFF)/255.0
    b = float((c>>8)&0xFF)/255.0
    result = (r,g,b,1.0)
    return result

def addComponents(objectsList,host):
    '''addComponents(objectsList,hostObject): adds the given object or the objects
    from the given list as components to the given host Object. Use this for
    example to add windows to a wall, or to add walls to a cell or floor.'''
    if not isinstance(objectsList,list):
        objectsList = [objectsList]
    hostType = Draft.getType(host)
    if hostType in ["Floor","Building","Site"]:
        c = host.Group
        for o in objectsList:
            if not o in c:
                c.append(o)
        host.Group = c
    elif hostType in ["Wall","Structure","Window","Roof","Stairs","StructuralSystem","Panel"]:
        import DraftGeomUtils
        a = host.Additions
        if hasattr(host,"Axes"):
            x = host.Axes
        for o in objectsList:
            if o.isDerivedFrom("Part::Feature"):
                if DraftGeomUtils.isValidPath(o.Shape) and (hostType == "Structure"):
                    if o.Support == host:
                        o.Support = None
                    host.Tool = o
                elif Draft.getType(o) == "Axis":
                    if not o in x:
                        x.append(o)
                elif not o in a:
                    if hasattr(o,"Shape"):
                        a.append(o)
        host.Additions = a
        if hasattr(host,"Axes"):
            host.Axes = x
    elif hostType in ["SectionPlane"]:
        a = host.Objects
        for o in objectsList:
            if not o in a:
                a.append(o)
        host.Objects = a
    elif host.isDerivedFrom("App::DocumentObjectGroup"):
        c = host.Group
        for o in objectsList:
            if not o in c:
                c.append(o)
        host.Group = c

def removeComponents(objectsList,host=None):
    '''removeComponents(objectsList,[hostObject]): removes the given component or
    the components from the given list from their parents. If a host object is
    specified, this function will try adding the components as holes to the host
    object instead.'''
    if not isinstance(objectsList,list):
        objectsList = [objectsList]
    if host:
        if Draft.getType(host) in ["Wall","Structure","Window","Roof","Stairs","StructuralSystem","Panel"]:
            if hasattr(host,"Tool"):
                if objectsList[0] == host.Tool:
                    host.Tool = None
            if hasattr(host,"Axes"):
                a = host.Axes
                for o in objectsList[:]:
                    if o in a:
                        a.remove(o)
                        objectsList.remove(o)
            s = host.Subtractions
            for o in objectsList:
                if not o in s:
                    s.append(o)
                    if Draft.getType(o) == "Window":
                        # fix for sketch-based windows
                        if o.Base:
                            if o.Base.Support:
                                if isinstance(o.Base.Support,tuple):
                                   if o.Base.Support[0].Name == host.Name:
                                       FreeCAD.Console.PrintMessage(translate("Arch","removing sketch support to avoid cross-referencing"))
                                       o.Base.Support = None
                                elif o.Base.Support.Name == host.Name:
                                    FreeCAD.Console.PrintMessage(translate("Arch","removing sketch support to avoid cross-referencing"))
                                    o.Base.Support = None
                            elif o.Base.ExternalGeometry:
                                for i in range(len(o.Base.ExternalGeometry)):
                                    if o.Base.ExternalGeometry[i][0].Name == host.Name:
                                        o.Base.delExternal(i)
                                        FreeCAD.Console.PrintMessage(translate("Arch","removing sketch support to avoid cross-referencing"))
                                        break
                    else:
                        if FreeCAD.GuiUp:
                            if not Draft.getType(o) == "Roof":
                                o.ViewObject.hide()
            host.Subtractions = s
    else:
        for o in objectsList:
            if o.InList:
               h = o.InList[0]
               tp = Draft.getType(h)
               if tp in ["Floor","Building","Site"]:
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
                   elif o == s.Base:
                       s.Base = None
                       o.ViewObject.show()
               elif tp in ["SectionPlane"]:
                   a = h.Objects
                   if o in a:
                       a.remove(o)
                       h.Objects = a

def fixWindow(obj):
    '''fixWindow(object): Fixes non-DAG problems in windows
    by removing supports and external geometry from underlying sketches'''
    if Draft.getType(obj) == "Window":
        if obj.Base:
            if hasattr(obj.Base,"Support"):
                if obj.Base.Support:
                    if isinstance(o.Base.Support,tuple):
                       if obj.Base.Support[0]:
                           FreeCAD.Console.PrintMessage(translate("Arch","removing sketch support to avoid cross-referencing"))
                           obj.Base.Support = None
                    elif obj.Base.Support:
                        FreeCAD.Console.PrintMessage(translate("Arch","removing sketch support to avoid cross-referencing"))
                        obj.Base.Support = None
            if hasattr(obj.Base,"ExternalGeometry"):
                if obj.Base.ExternalGeometry:
                    for i in range(len(obj.Base.ExternalGeometry)):
                        obj.Base.delExternal(0)
                        FreeCAD.Console.PrintMessage(translate("Arch","removing sketch external references to avoid cross-referencing"))

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

def makeFace(wires,method=2,cleanup=False):
    '''makeFace(wires): makes a face from a list of wires, finding which ones are holes'''
    #print "makeFace: start:", wires
    import Part

    if not isinstance(wires,list):
        if len(wires.Vertexes) < 3:
            raise
        return Part.Face(wires)
    elif len(wires) == 1:
        #import Draft;Draft.printShape(wires[0])
        if len(wires[0].Vertexes) < 3:
            raise
        return Part.Face(wires[0])

    wires = wires[:]

    #print "makeFace: inner wires found"
    ext = None
    max_length = 0
    # cleaning up rubbish in wires
    if cleanup:
        for i in range(len(wires)):
            wires[i] = DraftGeomUtils.removeInterVertices(wires[i])
        #print "makeFace: garbage removed"
    for w in wires:
        # we assume that the exterior boundary is that one with
        # the biggest bounding box
        if w.BoundBox.DiagonalLength > max_length:
            max_length = w.BoundBox.DiagonalLength
            ext = w
    #print "makeFace: exterior wire",ext
    wires.remove(ext)

    if method == 1:
        # method 1: reverse inner wires
        # all interior wires mark a hole and must reverse
        # their orientation, otherwise Part.Face fails
        for w in wires:
            #print "makeFace: reversing",w
            w.reverse()
            # make sure that the exterior wires comes as first in the list
        wires.insert(0, ext)
        #print "makeFace: done sorting", wires
        if wires:
            return Part.Face(wires)
    else:
        # method 2: use the cut method
        mf = Part.Face(ext)
        #print "makeFace: external face:",mf
        for w in wires:
            f = Part.Face(w)
            #print "makeFace: internal face:",f
            mf = mf.cut(f)
        #print "makeFace: final face:",mf.Faces
        return mf.Faces[0]

def closeHole(shape):
    '''closeHole(shape): closes a hole in an open shape'''
    import DraftGeomUtils, Part
    # creating an edges lookup table
    lut = {}
    for face in shape.Faces:
        for edge in face.Edges:
            hc = edge.hashCode()
            if lut.has_key(hc):
                lut[hc] = lut[hc] + 1
            else:
                lut[hc] = 1
    # filter out the edges shared by more than one face
    bound = []
    for e in shape.Edges:
        if lut[e.hashCode()] == 1:
            bound.append(e)
    bound = DraftGeomUtils.sortEdges(bound)
    try:
        nface = Part.Face(Part.Wire(bound))
        shell = Part.makeShell(shape.Faces+[nface])
        solid = Part.Solid(shell)
    except Part.OCCError:
        raise
    else:
        return solid

def getCutVolume(cutplane,shapes):
    """getCutVolume(cutplane,shapes): returns a cut face and a cut volume
    from the given shapes and the given cutting plane"""
    if not shapes:
        return None,None,None
    import Part
    if not isinstance(shapes,list):
        shapes = [shapes]
    # building boundbox
    bb = shapes[0].BoundBox
    for sh in shapes[1:]:
        bb.add(sh.BoundBox)
    bb.enlarge(1)
    # building cutplane space
    placement = None
    um = vm = wm = 0
    try:
        if hasattr(cutplane,"Shape"):
            p = cutplane.Shape.copy().Faces[0]
        else:
            p = cutplane.copy().Faces[0]
    except Part.OCCError:
        FreeCAD.Console.PrintMessage(translate("Arch","Invalid cutplane\n"))
        return None,None,None
    ce = p.CenterOfMass
    ax = p.normalAt(0,0)
    u = p.Vertexes[1].Point.sub(p.Vertexes[0].Point).normalize()
    v = u.cross(ax)
    if not bb.isCutPlane(ce,ax):
        FreeCAD.Console.PrintMessage(translate("Arch","No objects are cut by the plane\n"))
        return None,None,None
    else:
        corners = [FreeCAD.Vector(bb.XMin,bb.YMin,bb.ZMin),
                   FreeCAD.Vector(bb.XMin,bb.YMax,bb.ZMin),
                   FreeCAD.Vector(bb.XMax,bb.YMin,bb.ZMin),
                   FreeCAD.Vector(bb.XMax,bb.YMax,bb.ZMin),
                   FreeCAD.Vector(bb.XMin,bb.YMin,bb.ZMax),
                   FreeCAD.Vector(bb.XMin,bb.YMax,bb.ZMax),
                   FreeCAD.Vector(bb.XMax,bb.YMin,bb.ZMax),
                   FreeCAD.Vector(bb.XMax,bb.YMax,bb.ZMax)]
        for c in corners:
            dv = c.sub(ce)
            um1 = DraftVecUtils.project(dv,u).Length
            um = max(um,um1)
            vm1 = DraftVecUtils.project(dv,v).Length
            vm = max(vm,vm1)
            wm1 = DraftVecUtils.project(dv,ax).Length
            wm = max(wm,wm1)
        vu = DraftVecUtils.scaleTo(u,um)
        vui = vu.negative()
        vv = DraftVecUtils.scaleTo(v,vm)
        vvi = vv.negative()
        p1 = ce.add(vu.add(vvi))
        p2 = ce.add(vu.add(vv))
        p3 = ce.add(vui.add(vv))
        p4 = ce.add(vui.add(vvi))
        cutface = Part.makePolygon([p1,p2,p3,p4,p1])
        cutface = Part.Face(cutface)
        cutnormal = DraftVecUtils.scaleTo(ax,wm)
        cutvolume = cutface.extrude(cutnormal)
        cutnormal = cutnormal.negative()
        invcutvolume = cutface.extrude(cutnormal)
        return cutface,cutvolume,invcutvolume

def getShapeFromMesh(mesh,fast=True,tolerance=0.001,flat=False,cut=True):
    import Part, MeshPart, DraftGeomUtils
    if mesh.isSolid() and (mesh.countComponents() == 1) and fast:
        # use the best method
        faces = []
        for f in mesh.Facets:
            p=f.Points+[f.Points[0]]
            pts = []
            for pp in p:
                pts.append(FreeCAD.Vector(pp[0],pp[1],pp[2]))
            faces.append(Part.Face(Part.makePolygon(pts)))
        shell = Part.makeShell(faces)
        solid = Part.Solid(shell)
        solid = solid.removeSplitter()
        return solid

    faces = []
    segments = mesh.getPlanarSegments(tolerance)
    #print len(segments)
    for i in segments:
        if len(i) > 0:
            wires = MeshPart.wireFromSegment(mesh, i)
            if wires:
                if flat:
                    nwires = []
                    for w in wires:
                        nwires.append(DraftGeomUtils.flattenWire(w))
                    wires = nwires
                try:
                    faces.append(makeFace(wires,method=int(cut)+1))
                except:
                    return None
    try:
        se = Part.makeShell(faces)
        se = se.removeSplitter()
        if flat:
            return se
    except Part.OCCError:
        try:
            cp = Part.makeCompound(faces)
        except Part.OCCError:
            return None
        else:
            return cp
    else:
        try:
            solid = Part.Solid(se)
        except Part.OCCError:
            return se
        else:
            return solid

def projectToVector(shape,vector):
    '''projectToVector(shape,vector): projects the given shape on the given
    vector'''
    projpoints = []
    minl = 10000000000
    maxl = -10000000000
    for v in shape.Vertexes:
        p = DraftVecUtils.project(v.Point,vector)
        projpoints.append(p)
        l = p.Length
        if p.getAngle(vector) > 1:
            l = -l
        if l > maxl:
            maxl = l
        if l < minl:
            minl = l
    return DraftVecUtils.scaleTo(vector,maxl-minl)

def meshToShape(obj,mark=True,fast=True,tol=0.001,flat=False,cut=True):
    '''meshToShape(object,[mark,fast,tol,flat,cut]): turns a mesh into a shape, joining coplanar facets. If
    mark is True (default), non-solid objects will be marked in red. Fast uses a faster algorithm by
    building a shell from the facets then removing splitter, tol is the tolerance used when converting
    mesh segments to wires, flat will force the wires to be perfectly planar, to be sure they can be
    turned into faces, but this might leave gaps in the final shell. If cut is true, holes in faces are
    made by subtraction (default)'''

    name = obj.Name
    if "Mesh" in obj.PropertiesList:
        faces = []
        mesh = obj.Mesh
        plac = obj.Placement
        solid = getShapeFromMesh(mesh,fast,tol,flat,cut)
        if solid:
            if solid.isClosed() and solid.isValid():
                FreeCAD.ActiveDocument.removeObject(name)
            newobj = FreeCAD.ActiveDocument.addObject("Part::Feature",name)
            newobj.Shape = solid
            newobj.Placement = plac
            if (not solid.isClosed()) or (not solid.isValid()):
                if mark:
                    newobj.ViewObject.ShapeColor = (1.0,0.0,0.0,1.0)
            return newobj
    return None

def removeShape(objs,mark=True):
    '''removeShape(objs,mark=True): takes an arch object (wall or structure) built on a cubic shape, and removes
    the inner shape, keeping its length, width and height as parameters. If mark is True, objects that cannot
    be processed by this function will become red.'''
    import DraftGeomUtils
    if not isinstance(objs,list):
        objs = [objs]
    for obj in objs:
        if DraftGeomUtils.isCubic(obj.Shape):
            dims = DraftGeomUtils.getCubicDimensions(obj.Shape)
            if dims:
                name = obj.Name
                tp = Draft.getType(obj)
                print tp
                if tp == "Structure":
                    FreeCAD.ActiveDocument.removeObject(name)
                    import ArchStructure
                    str = ArchStructure.makeStructure(length=dims[1],width=dims[2],height=dims[3],name=name)
                    str.Placement = dims[0]
                elif tp == "Wall":
                    FreeCAD.ActiveDocument.removeObject(name)
                    import ArchWall
                    length = dims[1]
                    width = dims[2]
                    v1 = Vector(length/2,0,0)
                    v2 = v1.negative()
                    v1 = dims[0].multVec(v1)
                    v2 = dims[0].multVec(v2)
                    line = Draft.makeLine(v1,v2)
                    wal = ArchWall.makeWall(line,width=width,height=dims[3],name=name)
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

def download(url,force=False):
    '''download(url,force=False): downloads a file from the given URL and saves it in the
    macro path. Returns the path to the saved file. If force is True, the file will be
    downloaded again evn if it already exists.'''
    import urllib2, os
    name = url.split('/')[-1]
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Macro")
    macropath = p.GetString("MacroPath","")
    if not macropath:
        macropath = FreeCAD.ConfigGet("UserAppData")
    filepath = os.path.join(macropath,name)
    if os.path.exists(filepath) and not(force):
        return filepath
    try:
        FreeCAD.Console.PrintMessage("downloading "+url+" ...\n")
        response = urllib2.urlopen(url)
        s = response.read()
        f = open(filepath,'wb')
        f.write(s)
        f.close()
    except:
        return None
    else:
        return filepath

def check(objectslist,includehidden=False):
    """check(objectslist,includehidden=False): checks if the given objects contain only solids"""
    objs = Draft.getGroupContents(objectslist)
    if not includehidden:
        objs = Draft.removeHidden(objs)
    bad = []
    for o in objs:
        if not o.isDerivedFrom("Part::Feature"):
            bad.append([o,"is not a Part-based object"])
        else:
            s = o.Shape
            if (not s.isClosed()) and (not (Draft.getType(o) == "Axis")):
                bad.append([o,translate("Arch","is not closed")])
            elif not s.isValid():
                bad.append([o,translate("Arch","is not valid")])
            elif (not s.Solids) and (not (Draft.getType(o) == "Axis")):
                bad.append([o,translate("Arch","doesn't contain any solid")])
            else:
                f = 0
                for sol in s.Solids:
                    f += len(sol.Faces)
                    if not sol.isClosed():
                        bad.append([o,translate("Arch","contains a non-closed solid")])
                if len(s.Faces) != f:
                    bad.append([o,translate("Arch","contains faces that are not part of any solid")])
    return bad

def getHost(obj,strict=True):
    """getHost(obj,[strict]): returns the host of the current object. If strict is true (default),
    the host can only be an object of a higher level than the given one, or in other words, if a wall
    is contained in another wall which is part of a floor, the floor is returned instead of the parent wall"""
    import Draft
    t = Draft.getType(obj)
    for par in obj.InList:
        if par.isDerivedFrom("Part::Feature") or par.isDerivedFrom("App::DocumentObjectGroup"):
            if strict:
                if Draft.getType(par) != t:
                    return par
                else:
                    return getHost(par,strict)
            else:
                return par
    return None

def pruneIncluded(objectslist):
    """pruneIncluded(objectslist): removes from a list of Arch objects, those that are subcomponents of
    another shape-based object, leaving only the top-level shapes."""
    import Draft
    newlist = []
    for obj in objectslist:
        toplevel = True
        if obj.isDerivedFrom("Part::Feature"):
            if not (Draft.getType(obj) in ["Window","Clone"]):
                for parent in obj.InList:
                    if parent.isDerivedFrom("Part::Feature"):
                        if not parent.isDerivedFrom("Part::Part2DObject"):
                            # don't consider 2D objects based on arch elements
                            toplevel = False
        if toplevel:
            newlist.append(obj)
    return newlist

class _SurveyObserver:
    "an observer for the survey() function"
    def __init__(self,callback):
        self.callback = callback
        self.cancellable = False
        self.selection = []
        self.labels = []

    def addSelection(self,document, object, element, position):
        self.cancellable = False
        self.callback(True)

    def clearSelection(self,document):
        if self.cancellable:
            self.callback(True)
        else:
            self.cancellable = True

def survey(callback=False):
    """survey(): starts survey mode, where you can click edges and faces to get their lengths or area.
    Clicking on no object (on an empty area) stops survey mode."""
    if not callback:
        if hasattr(FreeCAD,"SurveyObserver"):
            for label in FreeCAD.SurveyObserver.labels:
                FreeCAD.ActiveDocument.removeObject(label)
            FreeCADGui.Selection.removeObserver(FreeCAD.SurveyObserver)
            del FreeCAD.SurveyObserver
            if FreeCAD.GuiUp:
                if hasattr(FreeCADGui,"draftToolBar"):
                    FreeCADGui.draftToolBar.offUi()
        else:
            FreeCAD.SurveyObserver = _SurveyObserver(callback=survey)
            FreeCADGui.Selection.addObserver(FreeCAD.SurveyObserver)
            if FreeCAD.GuiUp:
                if hasattr(FreeCADGui,"draftToolBar"):
                    FreeCADGui.draftToolBar.selectUi(callback=survey)
    else:
        sel = FreeCADGui.Selection.getSelectionEx()
        if not sel:
            if hasattr(FreeCAD,"SurveyObserver"):
                for label in FreeCAD.SurveyObserver.labels:
                    FreeCAD.ActiveDocument.removeObject(label)
                FreeCADGui.Selection.removeObserver(FreeCAD.SurveyObserver)
                del FreeCAD.SurveyObserver
                if FreeCAD.GuiUp:
                    if hasattr(FreeCADGui,"draftToolBar"):
                        FreeCADGui.draftToolBar.offUi()
        else:
            if hasattr(FreeCAD,"SurveyObserver"):
                basesel = FreeCAD.SurveyObserver.selection
                newsels = []
                for o in sel:
                    found = False
                    for eo in basesel:
                        if o.ObjectName == eo.ObjectName:
                            if o.SubElementNames == eo.SubElementNames:
                                found = True
                    if not found:
                        newsels.append(o)
                if newsels:
                    for o in newsels:
                        if o.Object.isDerivedFrom("Part::Feature"):
                            n = o.Object.Label
                            if not o.HasSubObjects:
                                # entire object
                                anno = FreeCAD.ActiveDocument.addObject("App::AnnotationLabel","surveyLabel")
                                if hasattr(o.Object.Shape,"CenterOfMass"):
                                    anno.BasePosition = o.Object.Shape.CenterOfMass
                                else:
                                    anno.BasePosition = o.Object.Shape.BoundBox.Center
                                FreeCAD.SurveyObserver.labels.append(anno.Name)
                                t = ""
                                if o.Object.Shape.Solids:
                                    t = FreeCAD.Units.Quantity(o.Object.Shape.Volume,FreeCAD.Units.Volume)
                                    t = t.getUserPreferred()[0]
                                    t = t.encode("utf8").replace("^3","³")
                                    anno.LabelText = "v " + t
                                    FreeCAD.Console.PrintMessage("Object: " + n + ", Element: Whole, Volume: " + t.decode("utf8") + "\n")
                                elif o.Object.Shape.Faces:
                                    t = FreeCAD.Units.Quantity(o.Object.Shape.Area,FreeCAD.Units.Area)
                                    t = t.getUserPreferred()[0]
                                    t = t.encode("utf8").replace("^2","²")
                                    anno.LabelText = "a " + t
                                    FreeCAD.Console.PrintMessage("Object: " + n + ", Element: Whole, Area: " + t.decode("utf8") + "\n")
                                else:
                                    t = FreeCAD.Units.Quantity(o.Object.Shape.Length,FreeCAD.Units.Length)
                                    t = t.getUserPreferred()[0]
                                    t = t.encode("utf8")
                                    anno.LabelText = "l " + t
                                    FreeCAD.Console.PrintMessage("Object: " + n + ", Element: Whole, Length: " + t.decode("utf8") + "\n")
                                if FreeCAD.GuiUp and t:
                                    QtGui.qApp.clipboard().setText(t)
                            else:
                                # single element(s)
                                for el in o.SubElementNames:
                                    e = getattr(o.Object.Shape,el)
                                    anno = FreeCAD.ActiveDocument.addObject("App::AnnotationLabel","surveyLabel")
                                    if "Vertex" in el:
                                        anno.BasePosition = e.Point
                                    else:
                                        if hasattr(e,"CenterOfMass"):
                                            anno.BasePosition = e.CenterOfMass
                                        else:
                                            anno.BasePosition = e.BoundBox.Center
                                    FreeCAD.SurveyObserver.labels.append(anno.Name)
                                    t = ""
                                    if "Face" in el:
                                        t = FreeCAD.Units.Quantity(e.Area,FreeCAD.Units.Area)
                                        t = t.getUserPreferred()[0]
                                        t = t.encode("utf8").replace("^2","²")
                                        anno.LabelText = "a " + t
                                        FreeCAD.Console.PrintMessage("Object: " + n + ", Element: " + el + ", Area: "+ t.decode("utf8")  + "\n")
                                    elif "Edge" in el:
                                        t = FreeCAD.Units.Quantity(e.Length,FreeCAD.Units.Length)
                                        t = t.getUserPreferred()[0]
                                        t = t.encode("utf8")
                                        anno.LabelText = "l " + t
                                        FreeCAD.Console.PrintMessage("Object: " + n + ", Element: " + el + ", Length: " + t.decode("utf8") + "\n")
                                    elif "Vertex" in el:
                                        t = FreeCAD.Units.Quantity(e.Z,FreeCAD.Units.Length)
                                        t = t.getUserPreferred()[0]
                                        t = t.encode("utf8")
                                        anno.LabelText = "z " + t
                                        FreeCAD.Console.PrintMessage("Object: " + n + ", Element: " + el + ", Zcoord: " + t.decode("utf8") + "\n")
                                    if FreeCAD.GuiUp and t:
                                        QtGui.qApp.clipboard().setText(t)

                    FreeCAD.SurveyObserver.selection.extend(newsels)


def toggleIfcBrepFlag(obj):
    """toggleIfcBrepFlag(obj): toggles the IFC brep flag of the given object, forcing it
    to be exported as brep geometry or not."""
    if not hasattr(obj,"IfcAttributes"):
        FreeCAD.Console.PrintMessage(translate("Arch","Object doesn't have settable IFC Attributes"))
    else:
        d = obj.IfcAttributes
        if "FlagForceBrep" in d.keys():
            if d["FlagForceBrep"] == "True":
                d["FlagForceBrep"] = "False"
                FreeCAD.Console.PrintMessage(translate("Arch","Disabling Brep force flag of object")+" "+obj.Label+"\n")
            else:
                d["FlagForceBrep"] = "True"
                FreeCAD.Console.PrintMessage(translate("Arch","Enabling Brep force flag of object")+" "+obj.Label+"\n")
        else:
            d["FlagForceBrep"] = "True"
            FreeCAD.Console.PrintMessage(translate("Arch","Enabling Brep force flag of object")+" "+obj.Label+"\n")
        obj.IfcAttributes = d


def makeCompoundFromSelected(objects=None):
    """makeCompoundFromSelected([objects]): Creates a new compound object from the given
    subobjects (faces, edges) or from the the selection if objects is None"""
    import FreeCADGui,Part
    so = []
    if not objects:
        objects = FreeCADGui.Selection.getSelectionEx()
    if not isinstance(objects,list):
        objects = [objects]
    for o in objects:
        so.extend(o.SubObjects)
    if so:
        c = Part.makeCompound(so)
        Part.show(c)


def cleanArchSplitter(objets=None):
    """cleanArchSplitter([objects]): removes the splitters from the base shapes
    of the given Arch objects or selected Arch objects if objects is None"""
    import FreeCAD,FreeCADGui
    if not objects:
        objects = FreeCADGui.Selection.getSelection()
    if not isinstance(objects,list):
        objects = [objects]
    for obj in objects:
        if obj.isDerivedFrom("Part::Feature"):
            if hasattr(obj,"Base"):
                if obj.Base:
                    print "Attempting to clean splitters from ",obj.Label
                    if obj.Base.isDerivedFrom("Part::Feature"):
                        if not obj.Base.Shape.isNull():
                            obj.Base.Shape = obj.Base.Shape.removeSplitter()
    FreeCAD.ActiveDocument.recompute()


def rebuildArchShape(objects=None):
    """rebuildArchShape([objects]): takes the faces from the base shape of the given (
    or selected if objects is None) Arch objects, and tries to rebuild a valid solid from them."""
    import FreeCAD,FreeCADGui,Part
    if not objects:
        objects = FreeCADGui.Selection.getSelection()
    if not isinstance(objects,list):
        objects = [objects]
    for obj in objects:
        success = False
        if obj.isDerivedFrom("Part::Feature"):
            if hasattr(obj,"Base"):
                if obj.Base:
                    try:
                        print "Attempting to rebuild ",obj.Label
                        if obj.Base.isDerivedFrom("Part::Feature"):
                            if not obj.Base.Shape.isNull():
                                faces = []
                                for f in obj.Base.Shape.Faces:
                                    f2 = Part.Face(f.Wires)
                                    #print "rebuilt face: isValid is ",f2.isValid()
                                    faces.append(f2)
                                if faces:
                                    shell = Part.Shell(faces)
                                    if shell:
                                        #print "rebuilt shell: isValid is ",shell.isValid()
                                        solid = Part.Solid(shell)
                                        if solid:
                                            if not solid.isValid():
                                                solid.sewShape()
                                                solid = Part.Solid(solid)
                                            #print "rebuilt solid: isValid is ",solid.isValid()
                                            if solid.isValid():
                                                print "Success"
                                                obj.Base.Shape = solid
                                                success = True
                    except:
                        pass
        if not success:
            print "Failed"
    FreeCAD.ActiveDocument.recompute()

# command definitions ###############################################
class _CommandAdd:
    "the Arch Add command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Add',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Add","Add component"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Add","Adds the selected components to the active object")}

    def IsActive(self):
        return len(FreeCADGui.Selection.getSelection()) > 1

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if Draft.getType(sel[-1]) == "Space":
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Add space boundary")))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("Arch.addSpaceBoundaries( FreeCAD.ActiveDocument."+sel[-1].Name+", FreeCADGui.Selection.getSelectionEx() )")
        else:
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Grouping")))
            if not mergeCells(sel):
                host = sel.pop()
                ss = "["
                for o in sel:
                    if len(ss) > 1:
                        ss += ","
                    ss += "FreeCAD.ActiveDocument."+o.Name
                ss += "]"
                FreeCADGui.addModule("Arch")
                FreeCADGui.doCommand("Arch.addComponents("+ss+",FreeCAD.ActiveDocument."+host.Name+")")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class _CommandRemove:
    "the Arch Add command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Remove',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Remove","Remove component"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Remove","Remove the selected components from their parents, or create a hole in a component")}

    def IsActive(self):
        return bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if Draft.getType(sel[-1]) == "Space":
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Remove space boundary")))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("Arch.removeSpaceBoundaries( FreeCAD.ActiveDocument."+sel[-1].Name+", FreeCADGui.Selection.getSelection() )")
        else:
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Ungrouping")))
            if (Draft.getType(sel[-1]) in ["Wall","Structure","Stairs","Roof","Window","Panel"]) and (len(sel) > 1):
                host = sel.pop()
                ss = "["
                for o in sel:
                    if len(ss) > 1:
                        ss += ","
                    ss += "FreeCAD.ActiveDocument."+o.Name
                ss += "]"
                FreeCADGui.addModule("Arch")
                FreeCADGui.doCommand("Arch.removeComponents("+ss+",FreeCAD.ActiveDocument."+host.Name+")")
            else:
                FreeCADGui.addModule("Arch")
                FreeCADGui.doCommand("Arch.removeComponents(FreeCAD.ActiveDocument."+sel[-1].Name+")")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class _CommandSplitMesh:
    "the Arch SplitMesh command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_SplitMesh',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_SplitMesh","Split Mesh"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_SplitMesh","Splits selected meshes into independent components")}

    def IsActive(self):
        return bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        if FreeCADGui.Selection.getSelection():
            sel = FreeCADGui.Selection.getSelection()
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Split Mesh")))
            for obj in sel:
                n = obj.Name
                nobjs = splitMesh(obj)
                if len(nobjs) > 1:
                    g = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup",n)
                    for o in nobjs:
                        g.addObject(o)
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()


class _CommandMeshToShape:
    "the Arch MeshToShape command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_MeshToShape',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_MeshToShape","Mesh to Shape"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_MeshToShape","Turns selected meshes into Part Shape objects")}

    def IsActive(self):
        return bool(FreeCADGui.Selection.getSelection())

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
            p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
            fast = p.GetBool("ConversionFast",True)
            tol = p.GetFloat("ConversionTolerance",0.001)
            flat = p.GetBool("ConversionFlat",False)
            cut = p.GetBool("ConversionCut",False)
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Mesh to Shape")))
            for obj in FreeCADGui.Selection.getSelection():
                newobj = meshToShape(obj,True,fast,tol,flat,cut)
                if g and newobj:
                    g.addObject(newobj)
            FreeCAD.ActiveDocument.commitTransaction()

class _CommandSelectNonSolidMeshes:
    "the Arch SelectNonSolidMeshes command definition"
    def GetResources(self):
        return {'Pixmap': 'Arch_SelectNonManifold.svg',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_SelectNonSolidMeshes","Select non-manifold meshes"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_SelectNonSolidMeshes","Selects all non-manifold meshes from the document or from the selected groups")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

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
        return bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        removeShape(sel)

class _CommandCloseHoles:
    "the Arch CloseHoles command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_CloseHoles',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_CloseHoles","Close holes"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_CloseHoles","Closes holes in open shapes, turning them solids")}

    def IsActive(self):
        return bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        for o in FreeCADGui.Selection.getSelection():
            s = closeHole(o.Shape)
            if s:
                o.Shape = s

class _CommandCheck:
    "the Arch Check command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Check',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Check","Check"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Check","Checks the selected objects for problems")}

    def IsActive(self):
        return bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        result = check(FreeCADGui.Selection.getSelection())
        if not result:
            FreeCAD.Console.PrintMessage(str(translate("Arch","All good! no problems found")))
        else:
            FreeCADGui.Selection.clearSelection()
            for i in result:
                FreeCAD.Console.PrintWarning("Object "+i[0].Name+" ("+i[0].Label+") "+i[1])
                FreeCADGui.Selection.addSelection(i[0])


class _CommandIfcExplorer:
    "the Arch Ifc Explorer command definition"
    def GetResources(self):
        return {'Pixmap'  : 'IFC',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_IfcExplorer","Ifc Explorer"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Check","Explore the contents of an Ifc file")}

    def Activated(self):
        if hasattr(self,"dialog"):
            del self.dialog
        import importIFC
        self.dialog = importIFC.explore()


class _CommandSurvey:
    "the Arch Survey command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Survey',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Survey","Survey"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Survey","Starts survey")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommandGui("Arch.survey()")


class _ToggleIfcBrepFlag:
    "the Toggle IFC Brep flag command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_ToggleIfcBrepFlag',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_ToggleIfcBrepFlag","Toggle IFC Brep flag"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_ToggleIfcBrepFlag","Force an object to be exported as Brep or not")}

    def IsActive(self):
        return bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        for o in FreeCADGui.Selection.getSelection():
            toggleIfcBrepFlag(o)


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Add',_CommandAdd())
    FreeCADGui.addCommand('Arch_Remove',_CommandRemove())
    FreeCADGui.addCommand('Arch_SplitMesh',_CommandSplitMesh())
    FreeCADGui.addCommand('Arch_MeshToShape',_CommandMeshToShape())
    FreeCADGui.addCommand('Arch_SelectNonSolidMeshes',_CommandSelectNonSolidMeshes())
    FreeCADGui.addCommand('Arch_RemoveShape',_CommandRemoveShape())
    FreeCADGui.addCommand('Arch_CloseHoles',_CommandCloseHoles())
    FreeCADGui.addCommand('Arch_Check',_CommandCheck())
    FreeCADGui.addCommand('Arch_IfcExplorer',_CommandIfcExplorer())
    FreeCADGui.addCommand('Arch_Survey',_CommandSurvey())
    FreeCADGui.addCommand('Arch_ToggleIfcBrepFlag',_ToggleIfcBrepFlag())
