# -*- coding: utf-8 -*-
#***************************************************************************
#*   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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

import math
from datetime import datetime

import FreeCAD
import DraftGeomUtils
import DraftVecUtils
import Part
import WorkingPlane

# This is roughly based on the no-fit polygon algorithm, used in
# SvgNest: https://github.com/Jack000/SVGnest
# Wikihouse plugin: https://github.com/tav/wikihouse-plugin/blob/master/wikihouse.rb

TOLERANCE = 0.0001 # smaller than this, two points are considered equal
DISCRETIZE = 4 # the number of segments in which arcs must be subdivided
ROTATIONS = [0,90,180,270] # the possible rotations to try

class Nester:


    def __init__(self,container=None,shapes=None):

        """Nester([container,shapes]): Creates a nester object with a container
           shape and a list of other shapes to nest into it. Container and
           shapes must be Part.Faces.

           Typical workflow:

           n = Nester() # creates the nester
           n.addContainer(object) # adds a doc object as the container
           n.addObjects(objects) # adds a list of doc objects as shapes
           n.run() # runs the nesting
           n.show() # creates a preview (compound) of the results
           n.apply() # applies transformations to the original objects

           Defaults (can be changed):

           Nester.TOLERANCE = 0.0001
           Nester.DISCRETIZE = 4
           Nester.ROTATIONS = [0,90,180,270]
           """

        self.objects = None
        self.container = container
        self.shapes = shapes
        self.results = [] # storage for the different results
        self.indexedFaces = None
        self.running = True
        self.progress = 0
        self.setCounter = None # optionally define a setCounter(value) function where value is a %

    def addObjects(self,objects):

        """addObjects(objects): adds FreeCAD DocumentObjects to the nester"""

        if not isinstance(objects,list):
            objects = [objects]
        if not self.objects:
            self.objects = {}
        if not self.shapes:
            self.shapes = []
        for obj in objects:
            if hasattr(obj,'Shape'):
                h = obj.Shape.hashCode()
                if not h in self.objects:
                    self.objects[h] = obj
                    self.shapes.append(obj.Shape)

    def addContainer(self,container):

        """addContainer(object): adds a FreeCAD DocumentObject as the container"""

        if hasattr(container,'Shape'):
            self.container = container.Shape

    def clear(self):

        """clear(): Removes all objects and shape from the nester"""

        self.objects = None
        self.shapes = None

    def stop(self):

        """stop((): stops the computation"""

        self.running = False

    def update(self):

        """update(): internal function to verify if computation can
        go on"""

        if self.setCounter:
            self.setCounter(self.progress)
        if FreeCAD.GuiUp:
            from PySide import QtGui
            QtGui.QApplication.processEvents()
        if not self.running:
            return False
        return True

    def run(self):

        """run(): Runs a nesting operation. Returns a list of lists of
           shapes, each primary list being one filled container, or None
           if the operation failed."""

        # reset abort mechanism and variables

        self.running = True
        self.progress = 0
        starttime = datetime.now()

        # general conformity tests

        print("Executing conformity tests ... ",end="")
        if not self.container:
            print("Empty container. Aborting")
            return
        if not self.shapes:
            print("Empty shapes. Aborting")
            return
        if not isinstance(self.container,Part.Face):
            print("Container is not a face. Aborting")
            return
        normal = self.container.normalAt(0,0)
        for s in self.shapes:
            if not self.update():
                return
            if len(s.Faces) != 1:
                print("One of the shapes does not contain exactly one face. Aborting")
                return
            # check if all faces correctly oriented (same normal)
            if s.Faces[0].normalAt(0,0).getAngle(normal) > TOLERANCE:
                # let pass faces with inverted normal
                if s.Faces[0].normalAt(0,0).getAngle(normal)-math.pi > TOLERANCE:
                    print("One of the face doesn't have the same orientation as the container. Aborting")
                    return

        # TODO
        # allow to use a non-rectangular container
        # manage margins/paddings
        # allow to prevent or force specific rotations for a piece

        # LONG-TERM TODO
        # add genetic algo to swap pieces, and check if the result is better

        # track progresses
        step = 100.0/(len(self.shapes)*len(ROTATIONS))

        # store hashCode together with the face so we can change the order
        # and still identify the original face, so we can calculate a transform afterwards
        self.indexedfaces = [[shape.hashCode(),shape] for shape in self.shapes]

        # build a clean copy so we don't touch the original
        faces = list(self.indexedfaces)

        # replace shapes by their face
        faces = [[f[0],f[1].Faces[0]] for f in faces]

        # order by area
        faces = sorted(faces,key=lambda face: face[1].Area)

        # discretize non-linear edges and remove holes
        nfaces = []
        for face in faces:
            if not self.update():
                return
            nedges = []
            allLines = True
            for edge in face[1].OuterWire.OrderedEdges:
                if isinstance(edge.Curve,(Part.LineSegment,Part.Line)):
                    nedges.append(edge)
                else:
                    allLines = False
                    last = edge.Vertexes[0].Point
                    for i in range(DISCRETIZE):
                        s = float(i+1)/DISCRETIZE
                        par = edge.FirstParameter + (edge.LastParameter-edge.FirstParameter)*s
                        new = edge.valueAt(par)
                        nedges.append(Part.LineSegment(last,new).toShape())
                        last = new
            f = Part.Face(Part.Wire(nedges))
            if not f.isValid():
                if allLines:
                    print("Invalid face found in set. Aborting")
                else:
                    print("Face distretizing failed. Aborting")
                return
            nfaces.append([face[0],f])
        faces = nfaces

        # container for sheets with a first, empty sheet
        sheets = [[]]

        print("Everything OK (",datetime.now()-starttime,")")

        # main loop

        facenumber = 1
        facesnumber = len(faces)

        #print("Vertices per face:",[len(face[1].Vertexes) for face in faces])

        while faces:

            print("Placing piece",facenumber,"/",facesnumber,"Area:",FreeCAD.Units.Quantity(faces[-1][1].Area,FreeCAD.Units.Area).getUserPreferred()[0],": ",end="")

            face = faces.pop()
            boc = self.container.BoundBox

            # this stores the available solutions for each rotation of a piece
            # contains [sheetnumber,face,xlength] lists,
            # face being [hascode,transformed face] and xlength
            # the X size of all boundboxes of placed pieces
            available = []

            # this stores the possible positions on a blank
            # sheet, in case we need to create a new one
            initials = []

            # this checks if the piece don't fit in the container
            unfit = True

            for rotation in ROTATIONS:

                if not self.update():
                    return

                self.progress += step

                print(rotation,", ",end="")
                hashcode = face[0]
                rotface = face[1].copy()
                if rotation:
                    rotface.rotate(rotface.CenterOfMass,normal,rotation)
                bof = rotface.BoundBox
                rotverts = self.order(rotface)
                #for i,v in enumerate(rotverts):
                #    Draft.makeText([str(i)],point=v)
                basepoint = rotverts[0] # leftmost point of the rotated face
                basecorner = boc.getPoint(0) # lower left corner of the container

                # See if the piece fits in the container dimensions
                if (bof.XLength < boc.XLength) and (bof.YLength < boc.YLength):
                    unfit = False

                # Get the fit polygon of the container
                # that is, the polygon inside which basepoint can
                # circulate, and the face still be fully inside the container

                v1 = basecorner.add(basepoint.sub(bof.getPoint(0)))
                v2 = v1.add(FreeCAD.Vector(0,boc.YLength-bof.YLength,0))
                v3 = v2.add(FreeCAD.Vector(boc.XLength-bof.XLength,0,0))
                v4 = v3.add(FreeCAD.Vector(0,-(boc.YLength-bof.YLength),0))
                binpol = Part.Face(Part.makePolygon([v1,v2,v3,v4,v1]))
                initials.append([binpol,[hashcode,rotface],basepoint])

                # check for available space on each existing sheet

                for sheetnumber,sheet in enumerate(sheets):
                    # Get the no-fit polygon for each already placed face in
                    # current sheet. That is, a polygon in which basepoint
                    # cannot be, if we want our face to not overlap with the
                    # placed face.
                    # To do this, we "circulate" the face around the placed face

                    if not self.update():
                        return

                    nofitpol = []
                    for placed in sheet:
                        pts = []
                        for placedvert in self.order(placed[1],right=True):
                            fpts = []
                            for i,rotvert in enumerate(rotverts):
                                if not self.update():
                                    return

                                facecopy = rotface.copy()
                                facecopy.translate(placedvert.sub(rotvert))

                                # test if all the points of the face are outside the
                                # placed face (except the base point, which is coincident)

                                outside = True
                                faceverts = self.order(facecopy)
                                for vert in faceverts:
                                    if (vert.sub(placedvert)).Length > TOLERANCE:
                                        if placed[1].isInside(vert,TOLERANCE,True):
                                            outside = False
                                            break

                                # also need to test for edge intersection, because even
                                # if all vertices are outside, the pieces could still
                                # overlap

                                if outside:
                                    for e1 in facecopy.OuterWire.Edges:
                                        for e2 in placed[1].OuterWire.Edges:
                                            if not self.update():
                                                return

                                            if True:
                                                # Draft code (SLOW)
                                                p = DraftGeomUtils.findIntersection(e1,e2)
                                                if p:
                                                    p = p[0]
                                                    p1 = e1.Vertexes[0].Point
                                                    p2 = e1.Vertexes[1].Point
                                                    p3 = e2.Vertexes[0].Point
                                                    p4 = e2.Vertexes[1].Point
                                                    if (p.sub(p1).Length > TOLERANCE) and (p.sub(p2).Length > TOLERANCE) \
                                                    and (p.sub(p3).Length > TOLERANCE) and (p.sub(p4).Length > TOLERANCE):
                                                        outside = False
                                                        break
                                            else:
                                                # alt code: using distToShape (EVEN SLOWER!)
                                                p = e1.distToShape(e2)
                                                if p:
                                                    if p[0] < TOLERANCE:
                                                        # allow vertex-to-vertex intersection
                                                        if (p[2][0][0] != "Vertex") or (p[2][0][3] != "Vertex"):
                                                            outside = False
                                                            break


                                if outside:
                                    fpts.append([faceverts[0],i])
                                    #Draft.makeText([str(i)],point=faceverts[0])

                            # reorder available solutions around a same point if needed
                            # ensure they are in the correct order

                            idxs = [p[1] for p in fpts]
                            if (0 in idxs) and (len(faceverts)-1 in idxs):
                                slicepoint = len(fpts)
                                last = len(faceverts)
                                for p in reversed(fpts):
                                    if p[1] == last-1:
                                        slicepoint -= 1
                                        last -= 1
                                    else:
                                        break
                                fpts = fpts[slicepoint:]+fpts[:slicepoint]
                                #print(fpts)
                            pts.extend(fpts)

                        # create the polygon

                        if len(pts) < 3:
                            print("Error calculating a no-fit polygon. Aborting")
                            return
                        pts = [p[0] for p in pts]
                        pol = Part.Face(Part.makePolygon(pts+[pts[0]]))

                        if not pol.isValid():

                            # fix overlapping edges

                            overlap = True
                            while overlap:
                                overlap = False
                                for i in range(len(pol.OuterWire.Edges)-1):
                                    if not self.update():
                                        return

                                    v1 = DraftGeomUtils.vec(pol.OuterWire.OrderedEdges[i])
                                    v2 = DraftGeomUtils.vec(pol.OuterWire.OrderedEdges[i+1])
                                    if abs(v1.getAngle(v2)-math.pi) <= TOLERANCE:
                                        overlap = True
                                        ne = Part.LineSegment(pol.OuterWire.OrderedEdges[i].Vertexes[0].Point,
                                                              pol.OuterWire.OrderedEdges[i+1].Vertexes[-1].Point).toShape()
                                        pol = Part.Face(Part.Wire(pol.OuterWire.OrderedEdges[:i]+[ne]+pol.OuterWire.OrderedEdges[i+2:]))
                                        break

                        if not pol.isValid():

                            # trying basic OCC fix

                            pol.fix(0,0,0)
                            if pol.isValid():
                                if pol.ShapeType == "Face":
                                    pol = Part.Face(pol.OuterWire) # discard possible inner holes
                                elif pol.Faces:
                                    # several faces after the fix, keep the biggest one
                                    a = 0
                                    ff = None
                                    for f in pol.Faces:
                                        if f.Area > a:
                                            a = f.Area
                                            ff = f
                                    if ff:
                                        pol = ff
                                else:
                                    print("Unable to fix invalid no-fit polygon. Aborting")
                                    Part.show(pol)
                                    return

                        if not pol.isValid():

                            # none of the fixes worked. Epic fail.

                            print("Invalid no-fit polygon. Aborting")
                            Part.show(pol.OuterWire)
                            for p in sheet:
                                Part.show(p[1])
                            Part.show(facecopy)
                            #for i,p in enumerate(faceverts):
                            #    Draft.makeText([str(i)],point=p)
                            return

                        if pol.isValid():
                            nofitpol.append(pol)
                            #Part.show(pol)

                    # Union all the no-fit pols into one

                    if len(nofitpol) == 1:
                        nofitpol = nofitpol[0]
                    elif len(nofitpol) > 1:
                        b = nofitpol.pop()
                        for n in nofitpol:
                            if not self.update():
                                return
                            b = b.fuse(n)
                        nofitpol = b

                        # remove internal edges (discard edges shared by 2 faces)

                        lut = {}
                        for f in fitpol.Faces:
                            for e in f.Edges:
                                h = e.hashCode()
                                if h in lut:
                                    lut[h].append(e)
                                else:
                                    lut[h] = [e]
                        edges = [e[0] for e in lut.values() if len(e) == 1]
                        try:
                            pol = Part.Face(Part.Wire(edges))
                        except Exception:
                            # above method can fail sometimes. Try a slower method
                            w = DraftGeomUtils.findWires(edges)
                            if len(w) == 1:
                                if w[0].isClosed():
                                    try:
                                        pol = Part.Face(w[0])
                                    except Exception:
                                        print("Error merging polygons. Aborting")
                                        try:
                                            Part.show(Part.Wire(edges))
                                        except Exception:
                                            for e in edges:
                                                Part.show(e)
                                        return

                    # subtract the no-fit polygon from the container's fit polygon
                    # we then have the zone where the face can be placed

                    if nofitpol:
                        fitpol = binpol.cut(nofitpol)
                    else:
                        fitpol = binpol.copy()

                    # check that we have some space on this sheet

                    if (fitpol.Area > 0) and fitpol.Vertexes:

                        # order the fitpol vertexes by smallest X
                        # and try to place the piece, making sure it doesn't
                        # intersect with already placed pieces
                        fitverts = sorted([v.Point for v in fitpol.Vertexes],key=lambda v: v.x)
                        for p in fitverts:
                            if not self.update():
                                return

                            trface = rotface.copy()
                            trface.translate(p.sub(basepoint))
                            ok = True
                            for placed in sheet:
                                if ok:
                                    for vert in trface.Vertexes:
                                        if placed[1].isInside(vert.Point,TOLERANCE,False):
                                            ok = False
                                            break
                                if ok:
                                    for e1 in trface.OuterWire.Edges:
                                        for e2 in placed[1].OuterWire.Edges:
                                            p = DraftGeomUtils.findIntersection(e1,e2)
                                            if p:
                                                p = p[0]
                                                p1 = e1.Vertexes[0].Point
                                                p2 = e1.Vertexes[1].Point
                                                p3 = e2.Vertexes[0].Point
                                                p4 = e2.Vertexes[1].Point
                                                if (p.sub(p1).Length > TOLERANCE) and (p.sub(p2).Length > TOLERANCE) \
                                                and (p.sub(p3).Length > TOLERANCE) and (p.sub(p4).Length > TOLERANCE):
                                                    ok = False
                                                    break
                                if not ok:
                                    break
                            if ok:
                                rotface = trface
                                break
                        else:
                            print("Couldn't determine location on sheet. Aborting")
                            return

                        # check the X space occupied by this solution

                        bb = rotface.BoundBox
                        for placed in sheet:
                            bb.add(placed[1].BoundBox)
                        available.append([sheetnumber,[hashcode,rotface],bb.XMax,fitpol])

            if unfit:
                print("One face doesn't fit in the container. Aborting")
                return

            if available:

                # order by smallest X size and take the first one
                available = sorted(available,key=lambda sol: sol[2])
                print("Adding piece to sheet",available[0][0]+1)
                sheets[available[0][0]].append(available[0][1])
                #Part.show(available[0][3])

            else:

                # adding to the leftmost vertex of the binpol

                sheet = []
                print("Creating new sheet, adding piece to sheet",len(sheets))
                # order initial positions by smallest X size
                initials = sorted(initials,key=lambda sol: sol[1][1].BoundBox.XLength)
                hashcode = initials[0][1][0]
                face = initials[0][1][1]
                # order binpol vertexes by X coord
                verts = sorted([v.Point for v in initials[0][0].Vertexes],key=lambda v: v.x)
                face.translate(verts[0].sub(initials[0][2]))
                sheet.append([hashcode,face])
                sheets.append(sheet)

            facenumber += 1

        print("Run time:",datetime.now()-starttime)
        self.results.append(sheets)
        return sheets


    def order(self,face,right=False):

        """order(face,[right]): returns a list of vertices
        ordered clockwise. The first vertex will be the
        lefmost one, unless right is True, in which case the
        first vertex will be the rightmost one"""

        verts = [v.Point for v in face.OuterWire.OrderedVertexes]

        # flatten the polygon on the XY plane

        wp = WorkingPlane.plane()
        wp.alignToPointAndAxis(face.CenterOfMass,face.normalAt(0,0))
        pverts = []
        for v in verts:
            vx = DraftVecUtils.project(v,wp.u)
            lx = vx.Length
            if vx.getAngle(wp.u) > 1:
                lx = -lx
            vy = DraftVecUtils.project(v,wp.v)
            ly = vy.Length
            if vy.getAngle(wp.v) > 1:
                ly = -ly
            pverts.append(FreeCAD.Vector(lx,ly,0))
        pverts.append(pverts[0])

        # https://stackoverflow.com/questions/1165647/how-to-determine-if-a-list-of-polygon-points-are-in-clockwise-order

        s = 0
        for i in range(len(pverts)-1):
            s += (pverts[i+1].x-pverts[i].x)*(pverts[i+1].y+pverts[i].y)
        if s < 0:
            verts.reverse()
        elif s == 0:
            print("error computing winding direction")
            return

        return verts


    def show(self,result=None):

        """show([result]): creates shapes in the document, showing
           the given result (list of sheets) or the last result if
           none is provided"""

        if not result:
            result = []
            if self.results:
                result = self.results[-1]
        offset = FreeCAD.Vector(0,0,0)
        feats = []
        for sheet in result:
            shapes = [self.container.OuterWire]
            shapes.extend([face[1] for face in sheet])
            comp = Part.makeCompound(shapes)
            comp.translate(offset)
            o = FreeCAD.ActiveDocument.addObject("Part::Feature","Nest")
            o.Shape = comp
            feats.append(o)
            offset = offset.add(FreeCAD.Vector(1.1*self.container.BoundBox.XLength,0,0))
        FreeCAD.ActiveDocument.recompute()
        return feats

    def getPlacements(self,result=None):

        """getPlacements([result]): returns a dictionary of hashCode:Placement
        pairs from the given result or the last computed result if none
        is given. The Placement contains a translation vector and a rotation
        to be given to the final object."""

        if not self.indexedfaces:
            print("error: shapes were not indexed. Please run() first")
            return
        if not result:
            result = []
            if self.results:
                result = self.results[-1]
        d = {}
        offset = FreeCAD.Vector(0,0,0)
        for sheet in result:
            for face in sheet:
                orig = None
                for pair in self.indexedfaces:
                    if pair[0] == face[0]:
                        orig = pair[1]
                if not orig:
                    print("error: hashCode mismatch between original and transformed face")
                    return
                shape = face[1]
                if offset.Length:
                    shape.translate(offset)
                deltav = shape.Faces[0].CenterOfMass.sub(orig.Faces[0].CenterOfMass)
                rot = FreeCAD.Rotation(orig.Vertexes[0].Point.sub(orig.Faces[0].CenterOfMass),shape.Vertexes[0].Point.sub(shape.Faces[0].CenterOfMass))
                pla = FreeCAD.Placement(deltav,rot)
                d[face[0]] = pla
            offset = offset.add(FreeCAD.Vector(1.1*self.container.BoundBox.XLength,0,0))
        return d

    def apply(self,result=None):

        """apply([result]): Applies the computed placements of the given
        result, or the last computed result if none is given, to the
        document objects given to the nester via addObjects() before
        running."""

        if not self.objects:
            print("objects list is empty")
            return
        p = self.getPlacements(result)
        if p:
            for key,pla in p.items():
                if key in self.objects:
                    sh = self.objects[key].Shape.copy()
                    sh.translate(pla.Base)
                    sh.rotate(sh.Faces[0].CenterOfMass,pla.Rotation.Axis,math.degrees(pla.Rotation.Angle))
                    self.objects[key].Placement = sh.Placement
                else:
                    print("error: hashCode mismatch with original object")


def test():

    "runs a test with selected shapes, container selected last"

    import FreeCADGui
    sel = FreeCADGui.Selection.getSelection()
    if sel:
        container = sel.pop().Shape
        shapes = [o.Shape for o in sel]
        n = Nester(container,shapes)
        result = n.run()
        if result:
            n.show()
