#!/usr/bin/env python

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License (LGPL)
# as published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.

import FreeCAD
import math
import re

from OpenSCADFeatures import *
from OpenSCAD2Dgeom import *
from OpenSCADUtils import *

if open.__module__ in ['__builtin__','io']:
    pythonopen = open # to distinguish python built-in open function from the one declared here


def openscadmesh(doc, scadstr, objname):
    import Part
    import Mesh
    import os
    import OpenSCADUtils
    tmpfilename = OpenSCADUtils.callopenscadstring(scadstr,'stl')
    if tmpfilename:
        #mesh1 = doc.getObject(objname) #reuse imported object
        Mesh.insert(tmpfilename)
        os.unlink(tmpfilename)
        mesh1 = doc.getObject(objname) #blog
        mesh1.ViewObject.hide()
        sh=Part.Shape()
        sh.makeShapeFromMesh(mesh1.Mesh.Topology, 0.1)
        solid = Part.Solid(sh)
        obj = doc.addObject("Part::FeaturePython", objname)
        ImportObject(obj, mesh1) #This object is not mutable from the GUI
        ViewProviderTree(obj.ViewObject)
        solid = solid.removeSplitter()
        if solid.Volume < 0:
            solid.complement()
        obj.Shape = solid#.removeSplitter()
        return obj
    else:
        print(scadstr)


class Node:
    #fnmin = 12 # maximal fn for implicit polygon rendering
    fnmin = FreeCAD.ParamGet(\
        "User parameter:BaseApp/Preferences/Mod/OpenSCAD").GetInt('useMaxFN')
    planedim = 1e10 #size of the square used as x-y-plane

    def __init__(self, name, arguments=None, children=None,):
        pass
        self.name = name
        self.arguments = arguments or {}
        self.children = children or []

    def __repr__(self):
        str1 = 'Node(name=%s' % self.name
        if self.arguments:
            str1 += ',arguments=%s' % self.arguments
        if self.children:
            str1 += ',children=%s' % self.children
        return str1+')'

    def __nonzero__(self):
        '''A Node is not obsolete if doesn't have children.
        Only if as neither name children or arguments'''
        return bool(self.name or self.arguments or self.children)

    def __len__(self):
        '''return the number of children'''
        return len(self.children)

    def __getitem__(self, key):
        '''direct access to the children'''
        return self.children.__getitem__(key)

    def rlen(self, checkmultmarix=False):
        '''Total number of nodes'''
        if self.children:
            return 1+sum([ch.rlen() for ch in self.children])
        else:
            return 1

    def addtofreecad(self,doc=None,fcpar=None):
        def center(obj,x,y,z):
            obj.Placement = FreeCAD.Placement(\
                FreeCAD.Vector(-x/2.0,-y/2.0,-z/2.0),\
                FreeCAD.Rotation(0,0,0,1))

        import FreeCAD
        import Part
        if not doc:
            doc = FreeCAD.newDocument()
        obj = None
        namel = self.name.lower()
        multifeature={'union':"Part::MultiFuse",'imp_union':"Part::MultiFuse",
                      'intersection':"Part::MultiCommon"}
        if namel in multifeature:
            if len(self.children)>1:
                obj = doc.addObject(multifeature[namel],namel)
                subobjs = [child.addtofreecad(doc,obj) for child in self.children]
                obj.Shapes = subobjs
                for subobj in subobjs:
                    subobj.ViewObject.hide()
            elif len(self.children) == 1:
                obj = self.children[0].addtofreecad(doc,fcpar or True)
            else:
                obj = fcpar
        elif namel == 'difference':
            if len(self.children) == 1:
                obj = self.children[0].addtofreecad(doc,fcpar or True)
            else:
                obj = doc.addObject("Part::Cut",namel)
                base = self.children[0].addtofreecad(doc,obj)

                if len(self.children) == 2:
                    tool = self.children[1].addtofreecad(doc,obj)
                else:
                    tool = Node(name='imp_union',\
                        children=self.children[1:]).addtofreecad(doc,obj)
                obj.Base = base
                obj.Tool = tool
                base.ViewObject.hide()
                tool.ViewObject.hide()
        elif namel == 'cube':
            obj = doc.addObject('Part::Box', namel)
            x,y,z = self.arguments['size']
            obj.Length = x
            obj.Width = y
            obj.Height = z
            if self.arguments['center']:
                center(obj,x,y,z)
        elif namel == 'sphere':
            obj = doc.addObject("Part::Sphere", namel)
            obj.Radius = self.arguments['r']
        elif namel == 'cylinder':
            h = self.arguments['h']
            r1, r2 = self.arguments['r1'], self.arguments['r2']
            if '$fn' in self.arguments and self.arguments['$fn'] > 2 \
            and self.arguments['$fn']<=Node.fnmin: # polygonal
                if r1 == r2: # prismatic
                    obj = doc.addObject("Part::Prism","prism")
                    obj.Polygon = int(self.arguments['$fn'])
                    obj.Circumradius = r1
                    obj.Height = h
                    if self.arguments['center']:
                        center(obj,0,0,h)
                    #base.ViewObject.hide()
                elif False: #use Frustum Feature with makeRuledSurface
                    obj = doc.addObject("Part::FeaturePython",'frustum')
                    Frustum(obj,r1,r2,int(self.arguments['$fn']), h)
                    ViewProviderTree(obj.ViewObject)
                    if self.arguments['center']:
                        center(obj,0,0,h)
                else: #Use Part::Loft and GetWire Feature
                    obj = doc.addObject('Part::Loft', 'frustum')
                    import Draft
                    p1 = Draft.makePolygon(int(self.arguments['$fn']), r1)
                    p2 = Draft.makePolygon(int(self.arguments['$fn']), r2)
                    if self.arguments['center']:
                        p1.Placement = FreeCAD.Placement(\
                        FreeCAD.Vector(0.0,0.0,-h/2.0),FreeCAD.Rotation())
                        p2.Placement = FreeCAD.Placement(\
                        FreeCAD.Vector(0.0,0.0,h/2.0), FreeCAD.Rotation())
                    else:
                        p2.Placement = FreeCAD.Placement(\
                        FreeCAD.Vector(0.0,0.0,h),FreeCAD.Rotation())
                    w1 = doc.addObject("Part::FeaturePython",'polygonwire1')
                    w2 = doc.addObject("Part::FeaturePython",'polygonwire2')
                    GetWire(w1,p1)
                    GetWire(w2,p2)
                    ViewProviderTree(w1.ViewObject)
                    ViewProviderTree(w2.ViewObject)
                    obj.Sections = [w1,w2]
                    obj.Solid = True
                    obj.Ruled = True
                    p1.ViewObject.hide()
                    p2.ViewObject.hide()
                    w1.ViewObject.hide()
                    w2.ViewObject.hide()
            else:
                if r1 == r2:
                    obj=doc.addObject("Part::Cylinder",namel)
                    obj.Height = h
                    obj.Radius = r1
                else:
                    obj=doc.addObject("Part::Cone",'cone')
                    obj.Height = h
                    obj.Radius1, obj.Radius2 = r1, r2
                if self.arguments['center']:
                    center(obj,0,0,h)
        elif namel == 'polyhedron':
            obj = doc.addObject("Part::Feature",namel)
            points = self.arguments['points']
            faces = self.arguments['triangles']
            shell = Part.Shell([Part.Face(Part.makePolygon(\
                     [tuple(points[pointindex]) for pointindex in \
                     (face+face[0:1])])) for face in faces])
#            obj.Shape=Part.Solid(shell).removeSplitter()
            solid=Part.Solid(shell).removeSplitter()
            if solid.Volume < 0:
#                solid.complement()
                solid.reverse()
            obj.Shape=solid#.removeSplitter()

        elif namel == 'polygon':
            obj = doc.addObject("Part::Feature", namel)
            points = self.arguments['points']
            paths = self.arguments.get('paths')
            if not paths:
                faces = [Part.Face(Part.makePolygon([(x,y,0) for x,y in points+points[0:1]]))]
            else:
                faces = [Part.Face(Part.makePolygon([(points[pointindex][0],points[pointindex][1],0) for \
                    pointindex in (path+path[0:1])])) for path in paths]
            obj.Shape=subtractfaces(faces)
        elif namel == 'square':
            obj = doc.addObject("Part::Plane",namel)
            x,y = self.arguments['size']
            obj.Length = x
            obj.Width = y
            if self.arguments['center']:
                center(obj,x,y,0)
        elif namel == 'circle':
            r = self.arguments['r']
            import Draft
            if '$fn' in self.arguments and self.arguments['$fn'] != 0 \
            and self.arguments['$fn']<=Node.fnmin:
                obj = Draft.makePolygon(int(self.arguments['$fn']),r)
            else:
                obj = Draft.makeCircle(r) # create a Face
                #obj = doc.addObject("Part::Circle",namel);obj.Radius = r
        elif namel == 'color':
            if len(self.children) == 1:
                obj = self.children[0].addtofreecad(doc,fcpar or True)
            else:
                obj = Node(name='imp_union',\
                        children=self.children).addtofreecad(doc,fcpar or True)
            obj.ViewObject.ShapeColor = tuple([float(p) for p in self.arguments[:3]]) #RGB
            transp = 100 - int(math.floor(100*self.arguments[3])) #Alpha
            obj.ViewObject.Transparency = transp
        elif namel == 'multmatrix':
            assert(len(self.children)>0)
            m1l = [round(f,12) for f in sum(self.arguments,[])] #That's the original matrix
            m1 = FreeCAD.Matrix(*tuple(m1l)) #That's the original matrix
            if isspecialorthogonalpython(fcsubmatrix(m1)): #a Placement can represent the transformation
                if len(self.children) == 1:
                    obj = self.children[0].addtofreecad(doc,fcpar or True)
                else:
                    obj = Node(name='imp_union',\
                            children = self.children).addtofreecad(doc,fcpar or True)
                    #FreeCAD.Console.PrintMessage('obj %s\nmat %s/n' % (obj.Placement,m1))
                obj.Placement=FreeCAD.Placement(m1).multiply(obj.Placement)
            else: #we need to apply the matrix transformation to the Shape using a custom PythonFeature
                obj = doc.addObject("Part::FeaturePython",namel)
                if len(self.children) == 1:
                    child = self.children[0].addtofreecad(doc,obj)
                else:
                    child = Node(name='imp_union',\
                            children=self.children).addtofreecad(doc,obj)
                MatrixTransform(obj,m1,child) #This object is not mutable from the GUI
                ViewProviderTree(obj.ViewObject)
        #elif namel == 'import': pass #Custom Feature
        elif namel == 'linear_extrude':
            height = self.arguments['height']
            twist = self.arguments.get('twist')
            if not twist:
                obj = doc.addObject("Part::Extrusion",namel)
            else: #twist
                obj = doc.addObject("Part::FeaturePython",'twist_extrude')
            if len(self.children)==0:
                base = Node('import',self.arguments).addtofreecad(doc,obj)
            elif len(self.children)==1:
                base = self.children[0].addtofreecad(doc,obj)
            else:
                base = Node(name='imp_union',\
                            children=self.children).addtofreecad(doc,obj)
            if False and base.isDerivedFrom('Part::MultiFuse'):
                #does not solve all the problems
                newobj=doc.addObject("Part::FeaturePython",'refine')
                RefineShape(newobj,base)
                ViewProviderTree(newobj.ViewObject)
                base.ViewObject.hide()
                base=newobj
            if not twist:
                obj.Base = base
                obj.Dir = (0,0,height)
            else: #twist
                Twist(obj,base,height,-twist)
                ViewProviderTree(obj.ViewObject)
            if self.arguments['center']:
                center(obj,0,0,height)
            base.ViewObject.hide()

        elif namel == 'rotate_extrude':
            obj = doc.addObject("Part::Revolution",namel)
            if len(self.children)==0:
                base = Node('import',self.arguments).addtofreecad(doc,obj)
            elif len(self.children)==1:
                base = self.children[0].addtofreecad(doc,obj)
            else:
                base = Node(name='imp_union',\
                            children=self.children).addtofreecad(doc,obj)
            if False and base.isDerivedFrom('Part::MultiFuse'):
                #creates 'Axe and meridian are confused' Errors
                newobj=doc.addObject("Part::FeaturePython",'refine')
                RefineShape(newobj,base)
                ViewProviderTree(newobj.ViewObject)
                base.ViewObject.hide()
                base=newobj
            obj.Source= base
            obj.Axis = (0.00,1.00,0.00)
            obj.Base = (0.00,0.00,0.00)
            obj.Angle = 360.00
            base.ViewObject.hide()
            obj.Placement=FreeCAD.Placement(FreeCAD.Vector(),FreeCAD.Rotation(0,0,90))
        elif namel == 'projection':
            if self.arguments['cut']:
                planename = 'xy_plane_used_for_project_cut'
                obj = doc.addObject('Part::MultiCommon','projection_cut')
                plane = doc.getObject(planename)
                if not plane:
                    plane=doc.addObject("Part::Plane",planename)
                    plane.Length=Node.planedim*2
                    plane.Width=Node.planedim*2
                    plane.Placement = FreeCAD.Placement(FreeCAD.Vector(\
                    -Node.planedim,-Node.planedim,0),FreeCAD.Rotation(0,0,0,1))
                #plane.ViewObject.hide()
                subobjs = [child.addtofreecad(doc,obj) for child in self.children]
                subobjs.append(plane)
                obj.Shapes = subobjs
                for subobj in subobjs:
                    subobj.ViewObject.hide()
            else:
                #Do a proper projection
                raise(NotImplementedError)
        elif namel == 'import':
            filename = self.arguments.get('file')
            scale = self.arguments.get('scale')
            origin = self.arguments.get('origin')
            if filename:
                import os
                docname = os.path.split(filename)[1]
                objname,extension = docname.split('.',1)
                if not os.path.isabs(filename):
                    try:
                        global lastimportpath
                        filename = os.path.join(lastimportpath,filename)
                    except: raise #no path given
                # Check for a mesh fileformat support by the Mesh mddule
                if extension.lower() in reverseimporttypes()['Mesh']:
                    import Mesh
                    mesh1 = doc.getObject(objname) #reuse imported object
                    if not mesh1:
                        Mesh.insert(filename)
                        mesh1 = doc.getObject(objname)
                    mesh1.ViewObject.hide()
                    sh = Part.Shape()
                    sh.makeShapeFromMesh(mesh1.Mesh.Topology,0.1)
                    solid = Part.Solid(sh)
                    obj = doc.addObject("Part::FeaturePython",'import_%s_%s'%(extension,objname))
                    #obj=doc.addObject('Part::Feature',)
                    ImportObject(obj,mesh1) #This object is not mutable from the GUI
                    ViewProviderTree(obj.ViewObject)
                    solid = solid.removeSplitter()
                    if solid.Volume < 0:
                        #sh.reverse()
                        #sh = sh.copy()
                        solid.complement()
                    obj.Shape = solid#.removeSplitter()
                elif extension in ['dxf']:
                    layera = self.arguments.get('layer')
                    featname ='import_dxf_%s_%s'%(objname,layera)
                    # reusing an already imported object does not work if the
                    # shape in not yet calculated
                    import importDXF
                    global dxfcache
                    layers = dxfcache.get(id(doc),[])
                    if layers:
                        groupobj = [go for go in layers if (not layera) or go.Label == layera]
                    else:
                        groupobj = None
                    if not groupobj:
                        groupname = objname
                        layers = importDXF.processdxf(doc,filename) or importDXF.layers
                        dxfcache[id(doc)] = layers[:]
                        for l in layers:
                            for o in l.Group:
                                o.ViewObject.hide()
                            l.ViewObject.hide()
                        groupobj = [go for go in layers if (not layera) or go.Label == layera]
                    edges = []
                    for shapeobj in groupobj[0].Group:
                        edges.extend(shapeobj.Shape.Edges)
                    try:
                        f = edgestofaces(edges)
                    except Part.OCCError:
                        FreeCAD.Console.PrintError('processing of dxf import failed\nPlease rework \'%s\' manually\n' % layera)
                        f = Part.Shape() #empty Shape
                    obj = doc.addObject("Part::FeaturePython",'import_dxf_%s_%s'%(objname,layera))
                    #obj=doc.addObject('Part::Feature',)
                    ImportObject(obj,groupobj[0]) #This object is not mutable from the GUI
                    ViewProviderTree(obj.ViewObject)
                    obj.Shape=f

                else:
                    FreeCAD.Console.ErrorMessage('Filetype of %s not supported\n' % (filename))
                    raise(NotImplementedError)
                if obj: #handle origin and scale
                    if scale is not None and scale !=1:
                        if origin is not None and any([c != 0 for c in origin]):
                            raise(NotImplementedError)# order of transformations unknown
                        child = obj
                        m1 = FreeCAD.Matrix()
                        m1.scale(scale,scale,scale)
                        obj = doc.addObject("Part::FeaturePython",'scale_import')
                        MatrixTransform(obj,m1,child) #This object is not mutable from the GUI
                        ViewProviderTree(obj.ViewObject)
                    elif origin is not None and any([c != 0 for c in origin]):
                        placement = FreeCAD.Placement(FreeCAD.Vector(*[-c for c in origin]),FreeCAD.Rotation())
                        obj.Placement = placement.multiply(obj.Placement)
                else:
                    FreeCAD.Console.ErrorMessage('Import of %s failed\n' % (filename))


        elif namel == 'minkowski':
            childrennames = [child.name.lower() for child in self.children]
            if len(self.children) == 2 and \
                childrennames.count('cube') == 1 and \
                (childrennames.count('sphere') + \
                childrennames.count('cylinder')) == 1:
                if self.children[0].name.lower() == 'cube':
                    cube = self.children[0]
                    roundobj = self.children[1]
                elif self.children[1].name.lower() == 'cube':
                    cube = self.children[1]
                    roundobj = self.children[0]
                roundobjname = roundobj.name.lower()
                issphere =  roundobjname == 'sphere'
                cubeobj = doc.addObject('Part::Box','roundedcube')
                x,y,z = cube.arguments['size']
                r = roundobj.arguments.get('r') or \
                        roundobj.arguments.get('r1')
                cubeobj.Length = x+2*r
                cubeobj.Width = y+2*r
                cubeobj.Height = z+2*r*issphere
                obj = doc.addObject("Part::Fillet","%s_%s"%(namel,roundobjname))
                obj.Base = cubeobj
                cubeobj.ViewObject.hide()
                if issphere:
                    obj.Edges = [(i,r,r) for i in range(1,13)]
                else:#cylinder
                    obj.Edges = [(i,r,r) for i in [1,3,5,7]]
                if cube.arguments['center']:
                    center(cubeobj,x+2*r,y+2*r,z+2*r*issphere)
                else: #htandle a rotated cylinder
                    #OffsetShape
                    raise(NotImplementedError)
            elif childrennames.count('sphere') == 1:
                sphereindex = childrennames.index('sphere')
                sphere = self.children[sphereindex]
                offset = sphere.arguments['r']
                nonsphere = self.children[0:sphereindex]+\
                        self.sphere[sphereindex+1:]
                obj = doc.addObject("Part::FeaturePython",'Offset')
                if len(nonsphere) == 1:
                    child = nonsphere[0].addtofreecad(doc,obj)
                else:
                    child = Node(name='imp_union',\
                        children=nonsphere).addtofreecad(doc,obj)
                OffsetShape(obj,child,offset)
                ViewProviderTree(obj.ViewObject)
            elif False:
                raise(NotImplementedError)
                pass # handle rotated cylinders and select edges that
                     #radius = radius0 * m1.multiply(FreeCAD.Vector(0,0,1)).dot(edge.Curve.tangent(0)[0])
            else:
                raise(NotImplementedError)
        elif namel == 'surface':
            obj = doc.addObject("Part::Feature",namel) #include filename?
            obj.Shape,xoff,yoff=makeSurfaceVolume(self.arguments['file'])
            if self.arguments['center']:
                center(obj,xoff,yoff,0.0)
            return obj
            #import os
            #scadstr = 'surface(file = "%s", center = %s );' % \
            #    (self.arguments['file'], 'true' if self.arguments['center'] else 'false')
            #docname=os.path.split(self.arguments['file'])[1]
            #objname,extension = docname.split('.',1)
            #obj = openscadmesh(doc,scadstr,objname)

        elif namel in ['glide','hull']:
            raise(NotImplementedError)
        elif namel in ['render','subdiv'] or True:
            lenchld = len(self.children)
            if lenchld == 1:
                FreeCAD.Console.PrintMessage('Not recognized %s\n' % (self))
                obj = self.children[0].addtofreecad(doc,fcpar)
            elif lenchld >1:
                obj = Node(name='imp_union',\
                        children=self.children).addtofreecad(doc,fcpar or True)
            else:
                obj = doc.addObject("Part::Feature",'Not_Impl_%s'%namel)
        if fcpar == True: #We are the last real object, our parent is not rendered.
            return obj

        if fcpar:
            try:
                obj.ViewObject.hide()
            except: raise
            if True: #never refine the Shape, as it itroduces crashes
                return obj
            else: #refine Shape
                import Draft
                if obj.Type =='Part::Extrusion' and obj.Base.Type == 'Part::Part2DObjectPython' and \
                    isinstance(obj.Base.Proxy,Draft._Polygon) or \
                    (not obj.isDerivedFrom('Part::Extrusion') and \
                    not obj.isDerivedFrom('Part::Boolean') and \
                    not obj.isDerivedFrom('Part::Cut') and \
                    not obj.isDerivedFrom('Part::MultiCommon') and \
                    not obj.isDerivedFrom('Part::MultiFuse') and \
                    not obj.isDerivedFrom('Part::Revolution') ) \
                    or (obj.isDerivedFrom('Part::FeaturePython') and isinstance(obj.Proxy,RefineShape)):
                    return obj
                else:
                    newobj = doc.addObject("Part::FeaturePython",'refine')
                    RefineShape(newobj,obj)
                    ViewProviderTree(newobj.ViewObject)
                    obj.ViewObject.hide()
                    return newobj

        else:
            doc.recompute()

    def flattengroups(self,name='group'):
        """removes group node with only one child and no arguments and empty groups"""
        node = self
        while (node.name == name and len(node.children) == 1 and len(node.arguments) == 0):
            node = node.children[0]
        node.children = [child for child in node.children if not (len(child.children) == 0 and child.name == name)]
        if node.children:
            node.children = [child.flattengroups() for child in node.children]
        return node

    def pprint(self,level=0):
        """prints the indented tree"""
        if self.arguments:
            argstr = ' (%s)' % self.arguments
        else:
            argstr = ''
        print('%s %s%s' %('  '*level,self.name,argstr))
        for child in self.children:
            child.pprint(level+1)

    def pprint2(self,path='root',pathjust=24):
        """prints the tree. Left column contains the syntax to access a child"""
        if self.arguments:
            argstr = ' (%s)' % self.arguments
        else:
            argstr = ''
        print('%s %s%s' %(path.ljust(pathjust),self.name,argstr))
        for i,child in enumerate(self.children):
            child.pprint2('%s[%d]'%(path,i),pathjust)




def parseexpression(e):
    e = e.strip()
    el = e.lower()
    if len(el) == 0: return None
    if el == 'true': return True
    elif el == 'false': return False
    elif el == 'undef': return None
    elif e[0].isdigit() or e[0] == '-' and len(e)>1 and e[1].isdigit():
        try:
            return float(e)
        except ValueError:
            import FreeCAD
            FreeCAD.Console.PrintMessage('%s\n' % (el))
            return 1.0

    elif el.startswith('"'): return e.strip('"') #string literal
    elif el.startswith('['):
        bopen, bclose = e.count('['), e.count(']')
        if bopen == bclose:
            return eval(el)
        else:
            import FreeCAD
            FreeCAD.Console.PrintMessage('%s\n' % (el))
            #return eval(el)
            #assert(False) #Malformed
    else:
        return e #Return the string

def parseargs(argstring):
    if '=' in argstring:
        level = 0
        tok = []
        a = []
        for i,char in enumerate(argstring):
            if char == '[': level += 1
            elif char ==']': level -= 1
            if level == 0 and (char == '=' or char == ','):
                tok.append(''.join(a).strip())
                a= []
            else:
                a.append(char)
        tok.append(''.join(a).strip())
        #print(tok)
        argdict = dict(zip(tok[0::2],[parseexpression(argstring) for argstring in tok[1::2]]))
#        argdict = {}
#        for key, value in re.findall(r"(\$?\w+)\s*=\s*(\[?\w+]?),?\s*",argstring):
#            argdict[key] = parseexpression(value)
        return argdict
    else:
        return parseexpression(argstring)

def parsenode(str1):
    name, str2 = str1.strip().split('(',1)
    assert('}' not in name)
    name = name.strip('#!%* ')#remove/ignore modifiers
    args, str3 = str2.split(')',1)
    str4 = str3.lstrip()
    if str4.startswith(';'):
        #has no children
        nextelement = str4[1:].lstrip()
        return Node(name,parseargs(args)),nextelement
    elif str4.startswith('{'):
        #has children
        level = 0
        for index,char in enumerate(str4):
            if char == '{': level += 1
            elif char == '}': level -= 1
            if level == 0:
                break
                #end of children
        childstr = str4[1:index].strip()
        nextelement = str4[index+1:].lstrip()
        bopen,bclose = childstr.count('{'),childstr.count('}')
        assert(bopen == bclose)
        children= []
        while childstr:
            try:
                childnode,childstr = parsenode(childstr)
                children.append(childnode)
            except ValueError:
                raise
        if args:
            args = parseargs(args)
        return Node(name,args,children),nextelement

def readfile(filename):
    import os
    global lastimportpath
    lastimportpath,relname = os.path.split(filename)
    isopenscad = relname.lower().endswith('.scad')
    if isopenscad:
        tmpfile=callopenscad(filename)
        if OpenSCADUtils.workaroundforissue128needed():
            lastimportpath = os.getcwd() #https://github.com/openscad/openscad/issues/128
        f = pythonopen(tmpfile)
    else:
        f = pythonopen(filename)
    rootnode =  parsenode(f.read())[0]
    f.close()
    if isopenscad and tmpfile:
        try:
            os.unlink(tmpfile)
        except OSError:
            pass
    return rootnode.flattengroups()

def open(filename):
    import os
    docname = os.path.split(filename)[1]
    doc = FreeCAD.newDocument(docname)
    doc.Label = (docname.split('.',1)[0])
    readfile(filename).addtofreecad(doc)
    #doc.recompute()
    return doc

def insert(filename,docname):
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    readfile(filename).addtofreecad(doc)
    #doc.recompute()



global dxfcache
dxfcache = {}
