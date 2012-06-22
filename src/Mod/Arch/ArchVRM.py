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

"The FreeCAD Arch Vector Rendering Module"

import FreeCAD,math,Part,ArchCommands,DraftVecUtils,DraftGeomUtils

DEBUG = True # if we want debug messages
MAXLOOP = 10 # the max number of loop before abort

# WARNING: in this module, faces are lists whose first item is the actual OCC face, the
# other items being additional information such as color, etc.

class Renderer:
    "A renderer object"
    def __init__(self,wp=None):

        """
        Creates a renderer with a default Draft WorkingPlane
        Use like this:
        
        import ArchVRM
        p = ArchVRM.Renderer()
        p.add(App.ActiveDocument.ActiveObject)
        p.sort()
        p.buildDummy()
        """
        
        self.reset()
        if wp:
            self.wp = wp
        else:
            import WorkingPlane
            self.wp = WorkingPlane.plane()
            
        if DEBUG: print "Renderer initialized on " + str(self.wp)

    def __str__(self):
        return "Arch Renderer: " + str(len(self.faces)) + " faces projected on " + str(self.wp)

    def reset(self):
        "removes all faces from this renderer"
        self.objects = []
        self.shapes = []
        self.faces = []
        self.resetFlags()

    def resetFlags(self):
        "resets all flags of this renderer"
        self.oriented = False
        self.trimmed = False
        self.sorted = False
        self.iscut = False
        self.joined = False
        self.sections = []

    def setWorkingPlane(self,wp):
        "sets a Draft WorkingPlane or Placement for this renderer"
        if isinstance(wp,FreeCAD.Placement):
            self.wp.setFromPlacement(wp)
        else:
            self.wp = wp
        if DEBUG: print "Renderer set on " + str(self.wp)

    def addFaces(self,faces,color=(0.9,0.9,0.9,1.0)):
        "add individual faces to this renderer, optionally with a color"
        if DEBUG: print "adding ", len(faces), " faces. Warning, these will get lost if using cut() or join()"
        for f in faces:
            self.faces.append([f,color])
        self.resetFlags()

    def addObjects(self,objs):
        "add objects to this renderer"
        for o in objs:
            if o.isDerivedFrom("Part::Feature"):
                self.objects.append(o)
                color = o.ViewObject.ShapeColor
                if o.Shape.Faces:
                    self.shapes.append([o.Shape,color])
                    for f in o.Shape.Faces:
                        self.faces.append([f,color])
        self.resetFlags()
        if DEBUG: print "adding ", len(self.objects), " objects, ", len(self.faces), " faces"

    def addShapes(self,shapes,color=(0.9,0.9,0.9,1.0)):
        "add shapes to this renderer, optionally with a color. Warning, these will get lost if using join()"
        if DEBUG: print "adding ", len(shapes), " shapes"
        for s in shapes:
            if s.Faces:
                self.shapes.append([s,color])
                for f in s.Faces:
                    self.faces.append([f,color])
        self.resetFlags()

    def info(self):
        "Prints info about the contents of this renderer"
        r = str(self)+"\n"
        r += "oriented: " + str(self.oriented) + "\n"
        r += "trimmed: " + str(self.trimmed) + "\n"
        r += "sorted: " + str(self.sorted) + "\n"
        r += "contains " + str(len(self.faces)) + " faces\n"
        for i in range(len(self.faces)):
            r += "  face " + str(i) + " : center " + str(self.faces[i][0].CenterOfMass)
            r += " : normal " + str(self.faces[i][0].normalAt(0,0))
            r += ", " + str(len(self.faces[i][0].Vertexes)) + " verts"
            r += ", color: " + self.getFill(self.faces[i][1]) + "\n"
        return r

    def addLabels(self):
        "Add labels on the model to identify faces"
        c = 0
        for f in self.faces:
            l = FreeCAD.ActiveDocument.addObject("App::AnnotationLabel","facelabel")
            l.BasePosition = f[0].CenterOfMass
            l.LabelText = str(c)
            c += 1

    def isVisible(self,face):
        "returns True if the given face points in the view direction"
        normal = face[0].normalAt(0,0)
        if DEBUG: print "checking face normal ", normal, " against ", self.wp.axis, " : ", math.degrees(normal.getAngle(self.wp.axis))
        if normal.getAngle(self.wp.axis) < math.pi/2:
            return True
        return False

    def reorient(self):
        "reorients the faces on the WP"
        if not self.faces: 
            return
        self.faces = [self.projectFace(f) for f in self.faces]
        if self.sections:
            self.sections = [self.projectFace(f) for f in self.sections]
        self.oriented = True

    def removeHidden(self):
        "removes faces pointing outwards"
        if not self.faces: 
            return
        faces = []
        for f in self.faces:
            if self.isVisible(f):
                faces.append(f)
        if DEBUG: print len(self.faces)-len(faces) , " faces removed, ", len(faces), " faces retained"
        self.faces = faces
        self.trimmed = True

    def projectFace(self,face):
        "projects a single face on the WP"
        wires = []
        norm = face[0].normalAt(0,0)
        for w in face[0].Wires:
            verts = []
            edges = DraftGeomUtils.sortEdges(w.Edges)
            for e in edges:
                v = e.Vertexes[0].Point
                v = self.wp.getLocalCoords(v)
                verts.append(v)
            verts.append(verts[0])
            if len(verts) > 2:
                wires.append(Part.makePolygon(verts))
        try:
            sh = ArchCommands.makeFace(wires)
        except:
            if DEBUG: print "Error: Unable to project face on the WP"
            return None
        else:
            # restoring flipped normals
            vnorm = self.wp.getLocalCoords(norm)
            if vnorm.getAngle(sh.normalAt(0,0)) > 1:
                sh.reverse()
            return [sh]+face[1:]

    def flattenFace(self,face):
        "Returns a face where all vertices have Z = 0"
        wires = []
        for w in face[0].Wires:
            verts = []
            edges = DraftGeomUtils.sortEdges(w.Edges)
            for e in edges:
                v = e.Vertexes[0].Point
                verts.append(FreeCAD.Vector(v.x,v.y,0))
            verts.append(verts[0])
            wires.append(Part.makePolygon(verts))
        try:
            sh = Part.Face(wires)
        except:
            if DEBUG: print "Error: Unable to flatten face"
            return None
        else:
            return [sh]+face[1:]

    def cut(self,cutplane):
        "Cuts through the shapes with a given cut plane and builds section faces"
        if self.iscut:
            return
        if not self.shapes:
            if DEBUG: print "No objects to make sections"
        else:
            fill = (1.0,1.0,1.0,1.0)
            placement = FreeCAD.Placement(cutplane.Placement)

            # building boundbox
            bb = self.shapes[0][0].BoundBox 
            for sh in self.shapes[1:]:
                bb.add(sh[0].BoundBox)
            bb.enlarge(1)
            um = vm = wm = 0
            if not bb.isCutPlane(placement.Base,self.wp.axis):
                if DEBUG: print "No objects are cut by the plane"
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
                    dv = c.sub(placement.Base)
                    um1 = DraftVecUtils.project(dv,self.wp.u).Length
                    um = max(um,um1)
                    vm1 = DraftVecUtils.project(dv,self.wp.v).Length
                    vm = max(vm,vm1)
                    wm1 = DraftVecUtils.project(dv,self.wp.axis).Length
                    wm = max(wm,wm1)
                p1 = FreeCAD.Vector(-um,vm,0)
                p2 = FreeCAD.Vector(um,vm,0)
                p3 = FreeCAD.Vector(um,-vm,0)
                p4 = FreeCAD.Vector(-um,-vm,0)
                cutface = Part.makePolygon([p1,p2,p3,p4,p1])
                cutface = Part.Face(cutface)
                cutface.Placement = placement
                cutnormal = DraftVecUtils.scaleTo(self.wp.axis,wm)
                cutvolume = cutface.extrude(cutnormal)
                shapes = []
                faces = []
                sections = []
                for sh in self.shapes:
                    for sol in sh[0].Solids:
                        c = sol.cut(cutvolume)
                        shapes.append([c]+sh[1:])
                        for f in c.Faces:
                            faces.append([f]+sh[1:])
                        sec = sol.section(cutface)
                        if sec.Edges:
                            wires = DraftGeomUtils.findWires(sec.Edges)
                            for w in wires:
                                sec = Part.Face(w)
                                sections.append([sec,fill])
                self.shapes = shapes
                self.faces = faces
                self.sections = sections
                if DEBUG: print "Built ",len(self.sections)," sections, ", len(self.faces), " faces retained"
                self.iscut = True
                self.oriented = False
                self.trimmed = False
                self.sorted = False
                self.joined = False

    def isInside(self,vert,face):
        "Returns True if the vert is inside the face in Z projection"
    
        # http://paulbourke.net/geometry/insidepoly/
        count = 0
        p = self.wp.getLocalCoords(vert.Point)
        for e in face[0].Edges:
            p1 = e.Vertexes[0].Point
            p2 = e.Vertexes[-1].Point
            if p.y > min(p1.y,p2.y):
                if p.y <= max(p1.y,p2.y):
                    if p.x <= max(p1.x,p2.x):
                        if p1.y != p2.y:
                            xinters = (p.y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x
                            if (p1.x == p2.x) or (p.x <= xinters):
                                count += 1
        if count % 2 == 0:
            return False
        else:
            return True

    def zOverlaps(self,face1,face2):
        "Checks if face1 overlaps face2 in Z direction"
        face1 = self.flattenFace(face1)
        face2 = self.flattenFace(face2)
        
        # first we check if one of the verts is inside the other face
        for v in face1[0].Vertexes:
            if self.isInside(v,face2):
                return True

        # even so, faces can still overlap if their edges cross each other
        for e1 in face1[0].Edges:
            for e2 in face2[0].Edges:
                if DraftGeomUtils.findIntersection(e1,e2):
                    return True
        return False

    def compare(self,face1,face2):
        "zsorts two faces. Returns 1 if face1 is closer, 2 if face2 is closer, 0 otherwise"

        # theory from
        # http://www.siggraph.org/education/materials/HyperGraph/scanline/visibility/painter.htm
        # and practical application http://vrm.ao2.it/ (blender vector renderer)

        b1 = face1[0].BoundBox
        b2 = face2[0].BoundBox

        # test 1: if faces don't overlap, no comparison possible
        if DEBUG: print "doing test 1"
        if b1.XMax < b2.XMin:
            return 0
        if b1.XMin > b2.XMax:
            return 0
        if b1.YMax < b2.YMin:
            return 0
        if b1.YMin > b2.YMax:
            return 0
        if DEBUG: print "failed, faces bboxes are not distinct"

        # test 2: if Z bounds dont overlap, it's easy to know the closest
        if DEBUG: print "doing test 2"
        if b1.ZMax < b2.ZMin:
            return 2
        if b2.ZMax < b1.ZMin:
            return 1
        if DEBUG: print "failed, faces Z are not distinct"

        # test 3: all verts of face1 are in front or behind the plane of face2
        if DEBUG: print "doing test 3"
        norm = face2[0].normalAt(0,0)
        behind = 0
        front = 0
        for v in face1[0].Vertexes:
            dv = v.Point.sub(face2[0].Vertexes[0].Point)
            dv = DraftVecUtils.project(dv,norm)
            if DraftVecUtils.isNull(dv):
                behind += 1
                front += 1
            else:
                if dv.getAngle(norm) > 1:
                    behind += 1
                else:
                    front += 1
        if DEBUG: print "front: ",front," behind: ",behind
        if behind == len(face1[0].Vertexes):
            return 2
        elif front == len(face1[0].Vertexes):
            return 1
        if DEBUG: print "failed, cannot say if face 1 is in front or behind"

        # test 4: all verts of face2 are in front or behind the plane of face1
        if DEBUG: print "doing test 4"
        norm = face1[0].normalAt(0,0)
        behind = 0
        front = 0
        for v in face2[0].Vertexes:
            dv = v.Point.sub(face1[0].Vertexes[0].Point)
            dv = DraftVecUtils.project(dv,norm)
            if DraftVecUtils.isNull(dv):
                behind += 1
                front += 1
            else:
                if dv.getAngle(norm) > 1:
                    behind += 1
                else:
                    front += 1
        if DEBUG: print "front: ",front," behind: ",behind
        if behind == len(face2[0].Vertexes):
            return 1
        elif front == len(face2[0].Vertexes):
            return 2
        if DEBUG: print "failed, cannot say if face 2 is in front or behind"

        # test 5: see if faces projections don't overlap, vertexwise
        if DEBUG: print "doing test 5"
        if not self.zOverlaps(face1,face2):
            return 0
        elif not self.zOverlaps(face2,face1):
            return 0
        if DEBUG: print "failed, faces are overlapping" 

        if DEBUG: print "Houston, all tests passed, and still no results" 
        return 0

    def join(self,otype):
        "joins the objects of same type"
        walls = []
        structs = []
        objs = []
        for o in obj.Source.Objects:
            t = Draft.getType(o)
            if t == "Wall":
                walls.append(o)
            elif t == "Structure":
                structs.append(o)
            else:
                objs.append(o)
        for g in [walls,structs]:
            if g:
                print "group:",g
                col = g[0].ViewObject.DiffuseColor[0]
                s = g[0].Shape
                for o in g[1:]:
                    try:
                        fs = s.fuse(o.Shape)
                        fs = fs.removeSplitter()
                    except:
                        print "shape fusion failed"
                        objs.append([o.Shape,o.ViewObject.DiffuseColor[0]])
                    else:
                        s = fs
                objs.append([s,col])

    def findPosition(self,f1,faces):
        "Finds the position of a face in a list of faces"
        l = None
        h = None
        for f2 in faces:
            if DEBUG: print "comparing face",str(self.faces.index(f1))," with face",str(self.faces.index(f2))
            r = self.compare(f1,f2)
            if r == 1:
                l = faces.index(f2)
            elif r == 2:
                if h == None:
                    h = faces.index(f2)
                else:
                    if faces.index(f2) < h:
                        h = faces.index(f2)
        if l != None:
            return l + 1
        elif h != None:
            return h
        else:
            return None

    def sort(self):
        "projects a shape on the WP"
        if len(self.faces) <= 1:
            return
        if not self.trimmed:
            self.removeHidden()
        if len(self.faces) == 1:
            return
        if not self.oriented:
            self.reorient()
        faces = self.faces[:]
        if DEBUG: print "sorting ",len(self.faces)," faces"
        sfaces = []
        loopcount = 0
        notfoundstack = 0
        while faces:
            if DEBUG: print "loop ", loopcount
            f1 = faces[0]
            if sfaces and (notfoundstack < len(faces)):
                if DEBUG: print "using ordered stack, notfound = ",notfoundstack
                p = self.findPosition(f1,sfaces)
                if p == None:
                    # no position found, we move the face to the end of the pile
                    faces.remove(f1)
                    faces.append(f1)
                    notfoundstack += 1
                else:
                    # position found, we insert it
                    faces.remove(f1)
                    sfaces.insert(p,f1)
                    notfoundstack = 0
            else:
                # either there is no stack, or no more face can be compared
                # find a root, 2 faces that can be compared
                if DEBUG: print "using unordered stack, notfound = ",notfoundstack
                for f2 in faces[1:]:
                    if DEBUG: print "comparing face",str(self.faces.index(f1))," with face",str(self.faces.index(f2))
                    r = self.compare(f1,f2)
                    if r == 1:
                        faces.remove(f2)
                        sfaces.append(f2)
                        faces.remove(f1)
                        sfaces.append(f1)
                        notfoundstack = 0
                        break
                    elif r == 2:
                        faces.remove(f1)
                        sfaces.append(f1)
                        faces.remove(f2)
                        sfaces.append(f2)
                        notfoundstack = 0
                        break
                else:
                    # nothing found, move the face to the end of the pile
                    faces.remove(f1)
                    faces.append(f1)
            loopcount += 1
            if loopcount == MAXLOOP * len(self.faces):
                if DEBUG: print "Too many loops, aborting."
                break

        if DEBUG: print "done Z sorting. ", len(sfaces), " faces retained, ", len(self.faces)-len(sfaces), " faces lost."
        self.faces = sfaces
        self.sorted = True

    def buildDummy(self):
        "Builds a dummy object with faces spaced on the Z axis, for visual check"
        z = 0
        if not self.sorted:
            self.sort()
        faces = []
        for f in self.faces[:]:
            ff = self.flattenFace(f)[0]
            ff.translate(FreeCAD.Vector(0,0,z))
            faces.append(ff)
            z += 1
        if faces:
            c = Part.makeCompound(faces)
            Part.show(c)

    def getFill(self,fill):
        "Returns a SVG fill value"
        r = str(hex(int(fill[0]*255)))[2:].zfill(2)
        g = str(hex(int(fill[1]*255)))[2:].zfill(2)
        b = str(hex(int(fill[2]*255)))[2:].zfill(2)
        col = "#"+r+g+b
        return col

    def getPathData(self,w):
        "Returns a SVG path data string from a 2D wire"
        edges = DraftGeomUtils.sortEdges(w.Edges)
        v = edges[0].Vertexes[0].Point
        svg = 'M '+ str(v.x) +' '+ str(v.y) + ' '
        for e in edges:
            if isinstance(e.Curve,Part.Line) or isinstance(e.Curve,Part.BSplineCurve):
                v = e.Vertexes[-1].Point
                svg += 'L '+ str(v.x) +' '+ str(v.y) + ' '
            elif isinstance(e.Curve,Part.Circle):
                r = e.Curve.Radius
                v = e.Vertexes[-1].Point
                svg += 'A '+ str(r) + ' '+ str(r) +' 0 0 1 '+ str(v.x) +' '
                svg += str(v.y) + ' '
        svg += 'z '
        return svg

    def getViewSVG(self,linewidth=0.01):
        "Returns a SVG fragment from viewed faces"
        if DEBUG: print "Printing ", len(self.faces), " faces"
        if not self.sorted:
            self.sort()
        svg = ''
        for f in self.faces:
            fill = self.getFill(f[1])
            svg +='<path '
            svg += 'd="'
            for w in f[0].Wires:
                svg += self.getPathData(w)
            svg += '" '
            svg += 'stroke="#000000" '
            svg += 'stroke-width="' + str(linewidth) + '" '
            svg += 'style="stroke-width:' + str(linewidth) + ';'
            svg += 'stroke-miterlimit:1;'
            svg += 'stroke-linejoin:round;'
            svg += 'stroke-dasharray:none;'
            svg += 'fill:' + fill + ';'
            svg += 'fill-rule: evenodd'
            svg += '"/>\n'
        return svg

    def getSectionSVG(self,linewidth=0.02):
        "Returns a SVG fragment from cut faces"
        if DEBUG: print "Printing ", len(self.sections), " cutfaces"
        if not self.oriented:
            self.reorient()
        svg = ''
        for f in self.sections:
            fill = self.getFill(f[1])
            svg +='<path '
            svg += 'd="'
            for w in f[0].Wires:
                svg += self.getPathData(w)
            svg += '" '
            svg += 'stroke="#000000" '
            svg += 'stroke-width="' + str(linewidth) + '" '
            svg += 'style="stroke-width:' + str(linewidth) + ';'
            svg += 'stroke-miterlimit:1;'
            svg += 'stroke-linejoin:round;'
            svg += 'stroke-dasharray:none;'
            svg += 'fill:' + fill + ';'
            svg += 'fill-rule: evenodd'
            svg += '"/>\n'
        return svg
        
