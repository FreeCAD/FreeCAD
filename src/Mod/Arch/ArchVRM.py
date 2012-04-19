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

import FreeCAD,math,Part,ArchCommands
from draftlibs import fcvec,fcgeo

DEBUG = True # if we want debug messages
MAXLOOP = 10 # the max number of loop before abort

class Renderer:
    "A renderer object"
    def __init__(self,wp=None,debug=None):

        """
        Creates a renderer with a default Draft WorkingPlane
        Use like this:
        
        import ArchVRM
        p = ArchVRM.Renderer()
        p.add(App.ActiveDocument.ActiveObject)
        p.sort()
        p.buildDummy()
        """

        if debug != None: DEBUG = debug
        self.defaultFill = (0.9,0.9,0.9,1.0) # the default fill color
        self.wp = wp
        self.faces = []
        self.fills = []
        self.oriented = False
        self.trimmed = False
        self.sorted = False
        if not self.wp:
            import WorkingPlane
            self.wp = WorkingPlane.plane()
        if DEBUG: print "Renderer initialized on " + str(self.wp)

    def __str__(self):
        return "Arch Renderer: " + str(len(self.faces)) + " faces projected on " + str(self.wp)

    def setWorkingPlane(self,wp):
        "sets a Draft WorkingPlane or Placement for this renderer"
        if isinstance(wp,FreeCAD.Placement):
            self.wp.setFromPlacement(wp)
        else:
            self.wp = wp
        if DEBUG: print "Renderer set on " + str(self.wp)

    def add(self,faces,colors=None):
        "add faces, shape or object to this renderer, optionally with face colors"

        def setcolors(colors,n):
            if colors:
                if isinstance(colors,tuple) and len(colors) == 4:
                    for i in range(n):
                        self.fills.append(colors)
                elif len(colors) == n:
                    self.fills.extend(colors)
                else:
                    c = []
                    for i in range(n):
                        c.append(colors[0])
                    self.fills.extend(c)
            else:
                c = []
                for i in range(n):
                    c.append(self.defaultFill)
                self.fills.extend(c)
            
        if isinstance(faces,list):
            f = faces
            setcolors(colors,len(f))
        elif hasattr(faces,"Faces"):
            f = faces.Faces
            setcolors(colors,len(f))
        elif hasattr(faces,"Shape"):
            f = faces.Shape.Faces
            if hasattr(faces,"ViewObject") and not colors:
                colors = faces.ViewObject.DiffuseColor
            setcolors(colors,len(f))
                    
        if DEBUG: print "adding ", len(f), " faces"
        self.faces.extend(f)
        self.oriented = False
        self.trimmed = False
        self.sorted = False
            
    def clean(self):
        "removes all faces from this renderer"
        self.faces = []
        self.fills = []
        self.oriented = False
        self.trimmed = False
        self.sorted = False

    def info(self):
        "Prints info about the contents of this renderer"
        r = str(self)+"\n"
        r += "oriented: " + str(self.oriented) + "\n"
        r += "trimmed: " + str(self.trimmed) + "\n"
        r += "sorted: " + str(self.sorted) + "\n"
        r += "contains " + str(len(self.faces)) + " faces\n"
        for i in range(len(self.faces)):
            r += "  face " + str(i) + " : center " + str(self.faces[i].CenterOfMass)
            r += " : normal " + str(self.faces[i].normalAt(0,0))
            r += ", " + str(len(self.faces[i].Vertexes)) + " verts"
            r += ", color: " + self.getFill(self.fills[i]) + "\n"
        return r

    def addLabels(self):
        "Add labels on the model to identify faces"
        c = 0
        for f in self.faces:
            l = FreeCAD.ActiveDocument.addObject("App::AnnotationLabel","facelabel")
            l.BasePosition = f.CenterOfMass
            l.LabelText = str(c)
            c += 1

    def isVisible(self,face):
        "returns True if the given face points in the view direction"
        normal = face.normalAt(0,0)
        if DEBUG: print "checking face normal ", normal, " against ", self.wp.axis, " : ", math.degrees(normal.getAngle(self.wp.axis))
        if normal.getAngle(self.wp.axis) < math.pi/2:
            return True
        return False

    def reorient(self):
        "reorients the faces on the WP"
        if not self.faces: 
            return
        self.faces = [self.projectFace(f) for f in self.faces]
        self.oriented = True

    def removeHidden(self):
        "removes faces pointing outwards"
        if not self.faces: 
            return
        faces = []
        fills = []
        for i in range(len(self.faces)):
            if self.isVisible(self.faces[i]):
                faces.append(self.faces[i])
                fills.append(self.fills[i])
        if DEBUG: print len(self.faces)-len(faces) , " faces removed, ", len(faces), " faces retained"
        self.faces = faces
        self.fills = fills
        self.trimmed = True

    def projectFace(self,face):
        "projects a single face on the WP"
        wires = []
        norm = face.normalAt(0,0)
        for w in face.Wires:
            verts = []
            edges = fcgeo.sortEdges(w.Edges)
            for e in edges:
                v = e.Vertexes[0].Point
                v = self.wp.getLocalCoords(v)
                verts.append(v)
            verts.append(verts[0])
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
            return sh

    def flattenFace(self,face):
        "Returns a face where all vertices have Z = 0"
        wires = []
        for w in face.Wires:
            verts = []
            edges = fcgeo.sortEdges(w.Edges)
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
            return sh

    def isInside(self,vert,face):
        "Returns True if the vert is inside the face in Z projection"
        

    def zOverlaps(self,face1,face2):
        "Checks if face1 overlaps face2 in Z direction"

        face1 = self.flattenFace(face1)
        face2 = self.flattenFace(face2)

        # first we check if one of the verts is inside the other face
        for v in face1.Vertexes:
            if self.isInside(v,face2):
                return True

        # even so, faces can still overlap if their edges cross each other
        for e1 in face1.Edges:
            for e2 in face2.Edges:
                if fcgeo.findIntersection(e1,e2):
                    return True

        return False

    def compare(self,face1,face2):
        "zsorts two faces. Returns 1 if face1 is closer, 2 if face2 is closer, 0 otherwise"

        # theory from
        # http://www.siggraph.org/education/materials/HyperGraph/scanline/visibility/painter.htm
        # and practical application http://vrm.ao2.it/ (blender vector renderer)

        b1 = face1.BoundBox
        b2 = face2.BoundBox

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
        norm = face2.normalAt(0,0)
        behind = 0
        front = 0
        for v in face1.Vertexes:
            dv = v.Point.sub(face2.Vertexes[0].Point)
            dv = fcvec.project(dv,norm)
            if fcvec.isNull(dv):
                behind += 1
                front += 1
            else:
                if dv.getAngle(norm) > 1:
                    behind += 1
                else:
                    front += 1
        if DEBUG: print "front: ",front," behind: ",behind
        if behind == len(face1.Vertexes):
            return 2
        elif front == len(face1.Vertexes):
            return 1
        if DEBUG: print "failed, cannot say if face 1 is in front or behind"

        # test 4: all verts of face2 are in front or behind the plane of face1
        if DEBUG: print "doing test 4"
        norm = face1.normalAt(0,0)
        behind = 0
        front = 0
        for v in face2.Vertexes:
            dv = v.Point.sub(face1.Vertexes[0].Point)
            dv = fcvec.project(dv,norm)
            if fcvec.isNull(dv):
                behind += 1
                front += 1
            else:
                if dv.getAngle(norm) > 1:
                    behind += 1
                else:
                    front += 1
        if DEBUG: print "front: ",front," behind: ",behind
        if behind == len(face2.Vertexes):
            return 1
        elif front == len(face2.Vertexes):
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
        if not self.faces: 
            return
        if len(self.faces) == 1:
            return
        if not self.trimmed:
            self.removeHidden()
        if not self.oriented:
            self.reorient()
        faces = self.faces[:]
        if DEBUG: print "sorting ",len(self.faces)," faces"
        sfaces = []
        sfills = []
        loopcount = 0
        notfoundstack = 0
        while faces:
            if DEBUG: print "loop ", loopcount
            f1 = faces[0]
            fi1 = self.fills[self.faces.index(f1)]
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
                    sfills.insert(p,fi1)
                    notfoundstack = 0
            else:
                # either there is no stack, or no more face can be compared
                # find a root, 2 faces that can be compared
                if DEBUG: print "using unordered stack, notfound = ",notfoundstack
                for f2 in faces[1:]:
                    fi2 = self.fills[self.faces.index(f2)]
                    if DEBUG: print "comparing face",str(self.faces.index(f1))," with face",str(self.faces.index(f2))
                    r = self.compare(f1,f2)
                    if r == 1:
                        faces.remove(f2)
                        sfaces.append(f2)
                        sfills.append(fi2)
                        faces.remove(f1)
                        sfaces.append(f1)
                        sfills.append(fi1)
                        notfoundstack = 0
                        break
                    elif r == 2:
                        faces.remove(f1)
                        sfaces.append(f1)
                        sfills.append(fi1)
                        faces.remove(f2)
                        sfaces.append(f2)
                        sfills.append(fi2)
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
        self.fills = sfills
        self.sorted = True

    def buildDummy(self):
        "Builds a dummy object with faces spaced on the Z axis, for visual check"
        z = 0
        if not self.sorted:
            self.sort()
        faces = []
        for f in self.faces[:]:
            ff = self.flattenFace(f)
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

    def getSVG(self,linewidth=0.01):
        "Returns a SVG fragment"
        if DEBUG: print len(self.faces), " faces and ", len(self.fills), " fills."
        if not self.sorted:
            self.sort()
        svg = ''
        for i in range(len(self.faces)):
            fill = self.getFill(self.fills[i])
            svg +='<path '
            svg += 'd="'
            for w in self.faces[i].Wires:
                edges = fcgeo.sortEdges(w.Edges)
                v = edges[0].Vertexes[0].Point
                svg += 'M '+ str(v.x) +' '+ str(v.y) + ' '
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
            svg += '" '
            svg += 'stroke="#000000" '
            svg += 'stroke-width="' + str(linewidth) + '" '
            svg += 'style="stroke-width:0.01;'
            svg += 'stroke-miterlimit:1;'
            svg += 'stroke-linejoin:round;'
            svg += 'stroke-dasharray:none;'
            svg += 'fill:' + fill + ';'
            svg += 'fill-rule: evenodd'
            svg += '"/>\n'
        return svg
