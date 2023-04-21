#***************************************************************************
#*   Copyright (c) 2012 Sebastian Hoogen <github@sebastianhoogen.de>       *
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

__title__ = "FreeCAD OpenSCAD Workbench - 2D helper functions"
__author__ = "Sebastian Hoogen"
__url__ = ["https://www.freecad.org"]

'''
This Script includes python functions to convert imported dxf geometry to Faces
'''

from functools import reduce

class Overlappingfaces():
    '''combines overlapping faces together'''
    def __init__(self,facelist):
        self.sortedfaces = sorted(facelist,key=(lambda shape: shape.Area),reverse=True)
        self.builddepdict()
        #self.faceindex = {}
        #for idx,face in enumerate(self.sortesfaces):
        #    self.faceindex[face.hashCode()] = idx

#    def __len__(self):
#        return len(self.sortedfaces)

    @staticmethod
    def dofacesoverlapboundbox(bigface,smallface):
        return bigface.BoundBox.isIntersection(smallface.BoundBox)

    @staticmethod
    def dofacesoverlapallverts(bigface,smallface):
        def vertsinface(f1,verts,tol=0.001,inface=True):
            '''check if all given verts are inside shape f1'''
            return all([f1.isInside(vert.Point,tol,inface) for vert in verts])
        return vertsinface(bigface,smallface.Vertexes)

    @staticmethod
    def dofacesoverlapproximity(bigface,smallface):
        l1,l2 = bigface.proximity(smallface)
        return len(l1) > 0 or len(l2) > 0

    @staticmethod
    def dofacesoverlapboolean(bigface,smallface):
        #import FreeCAD,FreeCADGui
        #FreeCAD.Console.PrintLog('intersecting %d %d\n'%(bigfacei,smallfacei))
        #FreeCADGui.updateGui()
        return bigface.common(smallface).Area > 0

    def builddepdict(self):
        import Part
        import itertools
        #isinsidelist = []
        self.isinsidedict = {}
        #for bigface, smallface in itertools.combinations(sortedfaces,2):
        for bigfacei, smallfacei in\
                itertools.combinations(range(len(self.sortedfaces)),2):
            try:
                overlap = Overlappingfaces.dofacesoverlapproximity(\
                        self.sortedfaces[bigfacei],self.sortedfaces[smallfacei])
            except (NotImplementedError, Part.OCCError) as e:
                try:
                    overlap = Overlappingfaces.dofacesoverlapboolean(\
                            self.sortedfaces[bigfacei],\
                            self.sortedfaces[smallfacei])
                except Part.OCCError:
                    overlap = Overlappingfaces.dofacesoverlapallverts(\
                            self.sortedfaces[bigfacei],\
                            self.sortedfaces[smallfacei])
            if overlap:
                #isinsidelist.append((bigfacei,smallfacei))
                smallinbig = self.isinsidedict.get(bigfacei,[])
                smallinbig.append(smallfacei)
                if len(smallinbig) == 1:
                    self.isinsidedict[bigfacei] = smallinbig

    @staticmethod
    def finddepth(dict1,faceidx,curdepth=0):
        if faceidx not in dict1:
            return curdepth+1
        else:
        #print(dict1[faceidx],[(finddepth(dict1,childface,curdepth)) for childface in dict1[faceidx]])
            return max([(Overlappingfaces.finddepth(dict1,childface,curdepth+1)) for childface in dict1[faceidx]])

    def findrootdepth(self):
        return max([Overlappingfaces.finddepth(self.isinsidedict,fi) for fi in range(len(self.sortedfaces))])

    def hasnoparent(self,faceindex):
        return Overlappingfaces.hasnoparentstatic(self.isinsidedict,faceindex)

    @staticmethod
    def hasnoparentstatic(isinsidedict,faceindex):
        for smalllist in isinsidedict.values():
            if faceindex in smalllist:
                return False
        return True

    #@staticmethod
    #def subtreedict(rootface,parantdict):
    #    '''biuld a subtree dictinary'''
    #    newdict = parantdict.copy()
    #    del newdict[rootface]
    #    return newdict

    @staticmethod
    def directchildren(isinsidedict,parent):
        #return [child for child in isinsidedict.get(parent,[]) if child not in isinsidedict]
        dchildren=[]
        for child in isinsidedict.get(parent,[]):
            direct = True
            for key, value in isinsidedict.items():
                if key != parent and child in value and parent not in value:
                    direct = False
            if direct:
                dchildren.append(child)
        return dchildren

    #@staticmethod
    #def indirectchildren(isinsidedict,parent):
     #   return [child for child in isinsidedict.get(parent,[]) if child in isinsidedict]

    @staticmethod
    def printtree(isinsidedict,facenum):
        def printtreechild(isinsidedict,facenum,parent):
            children=Overlappingfaces.directchildren(isinsidedict,parent)
            print('parent %d directchild %s' % (parent,children))
            if children:
                subdict=isinsidedict.copy()
                del subdict[parent]
                for child in children:
                    printtreechild(subdict,facenum,child)

        rootitems=[fi for fi in range(facenum) if Overlappingfaces.hasnoparentstatic(isinsidedict,fi)]
        for rootitem in rootitems:
            printtreechild(isinsidedict,facenum,rootitem)

    def makefeatures(self,doc):
        import FreeCAD
        def addshape(faceindex):
            obj=doc.addObject('Part::Feature','facefromedges_%d' % faceindex)
            obj.Shape = self.sortedfaces[faceindex]
            obj.ViewObject.hide()
            return obj

        def addfeature(faceindex,isinsidedict):
            directchildren = Overlappingfaces.directchildren(isinsidedict,faceindex)
            if len(directchildren) == 0:
                obj=addshape(faceindex)
            else:
                subdict=isinsidedict.copy()
                del subdict[faceindex]
                obj=doc.addObject("Part::Cut","facesfromedges_%d" % faceindex)
                obj.Base= addshape(faceindex) #we only do subtraction
                if len(directchildren) == 1:
                    obj.Tool = addfeature(directchildren[0],subdict)
                else:
                    obj.Tool = doc.addObject("Part::MultiFuse",\
                            "facesfromedges_union")
                    obj.Tool.Shapes = [addfeature(child,subdict)\
                            for child in directchildren]
                    obj.Tool.ViewObject.hide()
            obj.ViewObject.hide()
            return obj

        rootitems = [fi for fi in range(len(self.sortedfaces)) if self.hasnoparent(fi)]
        for rootitem in rootitems:
            addfeature(rootitem,self.isinsidedict).ViewObject.show()


    def makeshape(self):
        def removefaces(rfaces):
            for tfi in directchildren[::-1]:
                finishedwith.append(tfi)
                #del faces[tfi]
                if tfi in isinsidedict:
                    del isinsidedict[tfi]
                for key,value in isinsidedict.items():
                    if tfi in value:
                        newlist=value[:] #we work on a shallow copy of isinsidedict
                        newlist.remove(tfi)
                        isinsidedict[key]=newlist

        def hasnoparent(faceindex):
            for smalllist in self.isinsidedict.values():
                if faceindex in smalllist:
                    return False
            return True

        faces=self.sortedfaces[:]
        isinsidedict=self.isinsidedict.copy()
        finishedwith=[]
        while not all([Overlappingfaces.hasnoparentstatic(isinsidedict,fi) for fi in range(len(faces))]):
            #print([(Overlappingfaces.hasnoparentstatic(isinsidedict,fi),\
                #Overlappingfaces.directchildren(isinsidedict,fi)) for fi in range(len(faces))])
            for fi in range(len(faces))[::-1]:
                directchildren = Overlappingfaces.directchildren(isinsidedict,fi)
                if not directchildren:
                    continue
                elif len(directchildren) == 1:
                    faces[fi]=faces[fi].cut(faces[directchildren[0]])
                    #print(fi,'-' ,directchildren[0], faces[fi],faces[directchildren[0]])
                    removefaces(directchildren)
                else:
                    toolface=fusefaces([faces[tfi] for tfi in directchildren])
                    faces[fi]=faces[fi].cut(toolface)
                    #print(fi, '- ()', directchildren, [faces[tfi] for tfi in directchildren])
                    removefaces(directchildren)
                #print(fi,directchildren)
        faces =[face for index,face in enumerate(faces) if index not in finishedwith]
#        return faces
        return fusefaces(faces)

def findConnectedEdges(edgelist,eps=1e-6,debug=False):
    '''returns a list of list of connected edges'''

    def vertequals(v1,v2,eps=1e-6):
        '''check two vertices for equality'''
        #return all([abs(c1-c2)<eps for c1,c2 in zip(v1.Point,v2.Point)])
        return v1.Point.sub(v2.Point).Length<eps

    def vertindex(forward):
        '''return index of last or first element'''
        return -1 if forward else 0

    freeedges = edgelist[:]
    retlist = []
    debuglist = []
    while freeedges:
        startwire = freeedges.pop(0)
        forward = True
        newedge = [(startwire,True)]
        for forward in (True, False):
            found = True
            while found:
                lastvert = newedge[vertindex(forward)][0].Vertexes[vertindex(forward == newedge[vertindex(forward)][1])]
                for ceindex, checkedge in enumerate(freeedges):
                    found = False
                    for cvindex, cvert in enumerate([checkedge.Vertexes[0],checkedge.Vertexes[-1]]):
                        if vertequals(lastvert,cvert,eps):
                            if forward:
                                newedge.append((checkedge,cvindex == 0))
                            else:
                                newedge.insert(0,(checkedge,cvindex == 1))
                            del freeedges[ceindex]
                            found = True
                            break
                    else:
                        found = False
                    if found:
                        break
                else:
                    found = False
        #we are finished for this edge
        debuglist.append(newedge)
        retlist.append([item[0] for item in newedge]) #strip off direction
    #print(debuglist)
    if debug:
        return retlist,debuglist
    else:
        return retlist

def endpointdistance(edges):
    '''return the distance of vertices in path (list of edges) as
    maximum, minimum and distance between start and endpoint
    it expects the edges to be traversed forward starting from Vertex 0'''
    numedges=len(edges)
    if numedges == 1 and len(edges[0].Vertexes) == 1:
        return 0.0,0.0,0.0
    outerdistance = edges[0].Vertexes[0].Point.sub(\
        edges[-1].Vertexes[-1].Point).Length
    if numedges > 1:
        innerdistances=[edges[i].Vertexes[-1].Point.sub(edges[i+1].\
                Vertexes[0].Point).Length for i in range(numedges-1)]
        return max(innerdistances),min(innerdistances),outerdistance
    else:
        return 0.0,0.0,outerdistance

def endpointdistancedebuglist(debuglist):
    '''return the distance of vertices in path (list of edges) as
    maximum, minimum and distance between start and endpoint
    it expects a 'not reversed' flag for every edge'''
    numedges=len(debuglist)
    if numedges == 1 and len(debuglist[0][0].Vertexes) == 1:
        return 0.0,0.0,0.0
    outerdistance = debuglist[0][0].Vertexes[(not debuglist[0][1])*-1].\
            Point.sub(debuglist[-1][0].Vertexes[(debuglist[-1][1])*-1].\
            Point).Length
    if numedges > 1:
        innerdistances=[debuglist[i][0].Vertexes[debuglist[i][1]*-1].\
                Point.sub(debuglist[i+1][0].Vertexes[(not debuglist[i+1][1])*\
                -1].Point).Length for i in range(numedges-1)]
        return max(innerdistances),min(innerdistances),outerdistance
    else:
        return 0.0,0.0,outerdistance

def edgestowires(edgelist,eps=0.001):
    '''takes list of edges and returns a list of wires'''
    import Part
    import Draft
    # TODO remove double edges
    wirelist=[]
    #for path in findConnectedEdges(edgelist,eps=eps):
    for path,debug in zip(*findConnectedEdges(edgelist,eps=eps,debug=True)):
        maxd,mind,outerd = endpointdistancedebuglist(debug)
        assert(maxd <= eps*2) # Assume the input to be broken
        if maxd < eps*2 and maxd > 0.000001: # OCC won't like it if maxd > 0.02:
            print('endpointdistance max:%f min:%f, ends:%f' %(maxd,mind,outerd))

            if True:
                tobeclosed = outerd < eps*2
                # OpenSCAD uses 0.001 for corase grid
                #from draftlibs import fcvec, fcgeo
                #w2=fcgeo.superWire(path,tobeclosed)
                w2=superWireReverse(debug,tobeclosed)
                wirelist.append(w2)
            else:#this locks up FreeCAD
                comp=Part.Compound(path)
                wirelist.append(comp.connectEdgesToWires(False,eps).Wires[0])
                #wirelist.append(comp.connectEdgesToWires(False,0.1).Wires[0])
        else:
            done = False
            try:
                wire=Part.Wire(path)
                #if not close or wire.isClosed or outerd > 0.0001:
                wirelist.append(Part.Wire(path))
                done = True
            except Part.OCCError:
                pass
            if not done:
                comp=Part.Compound(path)
                wirelist.append(comp.connectEdgesToWires(False,eps).Wires[0])
    return wirelist

def subtractfaces(faces):
    '''searches for the biggest face and subtracts all smaller ones from the
    first. Only makes sense if all faces overlap.'''
    if len(faces)==1:
        return faces[0]
    else:
        facelist=sorted(faces,key=(lambda shape: shape.Area),reverse=True)
        base=facelist[0]
        tool=reduce(lambda p1,p2: p1.fuse(p2),facelist[1:])
        return base.cut(tool)

def fusefaces(faces):
    if len(faces)==1:
        return faces[0]
    else:
        return reduce(lambda p1,p2: p1.fuse(p2),faces)

def subtractfaces2(faces):
    '''Sort faces, check if they overlap. Subtract overlapping face and fuse
    nonoverlapping groups.'''
    return fusefaces([subtractfaces(facegroup) for facegroup in findoverlappingfaces(faces)])

def edgestofaces(edges,algo=3,eps=0.001):
    #edges=[]
    #for shapeobj in (objs):
    #    edges.extend(shapeobj.Shape.Edges)
    #taken from Drafttools
    #from draftlibs import fcvec, fcgeo
    import Part
    #wires = fcgeo.findWires(edges)
    wires = edgestowires(edges,eps)
    facel=[]
    for w in wires:
        #assert(len(w.Edges)>1)
        if not w.isClosed():
            p0 = w.Vertexes[0].Point
            p1 = w.Vertexes[-1].Point
            edges2 = w.Edges[:]
            try:
                edges2.append(Part.LineSegment(p1,p0).toShape())
                w = Part.Wire(edges2)
                #w = Part.Wire(fcgeo.sortEdges(edges2))
            except OCCError:
                comp=Part.Compound(edges2)
                w = comp.connectEdgesToWires(False,eps).Wires[0]
        facel.append(Part.Face(w))
        #if w.isValid: #debugging
        #    facel.append(Part.Face(w))
        #else:
        #    Part.show(w)
    if algo is None:
        return facel
    elif algo == 1: #stable behavior
        return subtractfaces(facel)
    elif algo == 0: #return all faces
        return Part.Compound(facel)
    elif algo == 2:
        return subtractfaces2(facel)
    elif algo == 3:
        return Overlappingfaces(facel).makeshape()

def superWireReverse(debuglist,closed=False):
    '''superWireReverse(debuglist,[closed]): forces a wire between edges
    that don't necessarily have coincident endpoints. If closed=True, wire
    will always be closed. debuglist has a tuple for every edge.The first
    entry is the edge, the second is the flag 'does not need to be inverted'
    '''
    #taken from draftlibs
    def median(v1,v2):
        vd = v2.sub(v1)
        vd.scale(.5,.5,.5)
        return v1.add(vd)
    try:
        from DraftGeomUtils import findMidpoint
    except ImportError: #workaround for Version 0.12
        from draftlibs.fcgeo import findMidpoint #workaround for Version 0.12
    import Part
    #edges = sortEdges(edgeslist)
    print(debuglist)
    newedges = []
    for i in range(len(debuglist)):
        curr = debuglist[i]
        if i == 0:
            if closed:
                prev = debuglist[-1]
            else:
                prev = None
        else:
            prev = debuglist[i-1]
        if i == (len(debuglist)-1):
            if closed:
                nexte = debuglist[0]
            else:
                nexte = None
        else:
            nexte = debuglist[i+1]
        print(i,prev,curr,nexte)
        if prev:
            if curr[0].Vertexes[-1*(not curr[1])].Point == \
                    prev[0].Vertexes[-1*prev[1]].Point:
                p1 = curr[0].Vertexes[-1*(not curr[1])].Point
            else:
                p1 = median(curr[0].Vertexes[-1*(not curr[1])].Point,\
                        prev[0].Vertexes[-1*prev[1]].Point)
        else:
            p1 = curr[0].Vertexes[-1*(not curr[1])].Point
        if nexte:
            if curr[0].Vertexes[-1*curr[1]].Point == \
                nexte[0].Vertexes[-1*(not nexte[1])].Point:
                p2 = nexte[0].Vertexes[-1*(not nexte[1])].Point
            else:
                p2 = median(curr[0].Vertexes[-1*(curr[1])].Point,\
                        nexte[0].Vertexes[-1*(not nexte[1])].Point)
        else:
            p2 = curr[0].Vertexes[-1*(curr[1])].Point
        if isinstance(curr[0].Curve,(Part.LineSegment, Part.Line)):
            print("line",p1,p2)
            newedges.append(Part.LineSegment(p1,p2).toShape())
        elif isinstance(curr[0].Curve,Part.Circle):
            p3 = findMidpoint(curr[0])
            print("arc",p1,p3,p2)
            newedges.append(Part.Arc(p1,p3,p2).toShape())
        else:
            print("Cannot superWire edges that are not lines or arcs")
            return None
    print(newedges)
    return Part.Wire(newedges)

def importDXFface(filename,layer=None,doc=None):
    import FreeCAD
    import importDXF
    importDXF.readPreferences()
    importDXF.getDXFlibs()
    importDXF.dxfMakeBlocks = False
    doc = doc or FreeCAD.activeDocument()
    layers = importDXF.processdxf(doc,filename,False,False) or importDXF.layers
    for l in layers:
        if FreeCAD.GuiUp:
            for o in l.Group:
                o.ViewObject.hide()
            l.ViewObject.hide()
    groupobj=[go for go in layers if (not layer) or go.Label == layer]
    edges=[]
    if not groupobj:
        raise ValueError('import of layer %s failed' % layer)
    for shapeobj in groupobj[0].Group:
        edges.extend(shapeobj.Shape.Edges)
    faces = edgestofaces(edges)
    # in order to allow multiple import with the same layer name
    # we need to remove used objects from the layer group
    container = None
    for layer in layers: #remove everything that has been imported
        if container is None:
            container = layer.getParentGroup()
        removeOp = getattr(layer, "removeObjectsFromDocument", None)
        if callable(removeOp):
            layer.removeObjectsFromDocument()
        for obj in layer.Group:
            obj.Document.removeObject(obj.Name)
        layer.Document.removeObject(layer.Name)
    container.Document.removeObject(container.Name)
    return faces
