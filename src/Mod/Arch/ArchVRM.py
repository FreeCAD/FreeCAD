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

import FreeCAD,math,Part
from draftlibs import fcvec,fcgeo

DEBUG = True # if we want debug messages
MAXLOOP = 10 # the max number of loop before abort

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
        self.wp = wp
        self.faces = []
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
        "sets a Draft WorkingPlane for this renderer"
        self.wp = wp
        if DEBUG: print "Renderer set on " + str(self.wp)

    def add(self,faces):
        "add faces, shape or object to this renderer"
        if isinstance(faces,list):
            f = faces
        elif hasattr(faces,"Faces"):
            f = faces.Faces
        elif hasattr(faces,"Shape"):
            f = faces.Shape.Faces
        if DEBUG: print "adding ", len(f), " faces"
        self.faces.extend(f)
        self.oriented = False
        self.trimmed = False
        self.sorted = False
            
    def clean(self):
        "removes all faces from this renderer"
        self.faces = []
        self.oriented = False
        self.trimmed = False
        self.sorted = False

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
        for f in (self.faces):
            if self.isVisible(f):
                faces.append(f)
        if DEBUG: print len(self.faces)-len(faces) , " faces removed, ", len(faces), " faces retained"
        self.faces = faces
        self.trimmed = True

    def projectFace(self,face):
        "projects a single face on the WP"
        verts = []
        edges = fcgeo.sortEdges(face.Edges)
        for e in edges:
            v = e.Vertexes[0].Point
            v = self.wp.getLocalCoords(v)
            verts.append(v)
        verts.append(verts[0])
        try:
            sh = Part.makePolygon(verts)
            sh = Part.Face(sh)
        except:
            if DEBUG: print "Error: Unable to project face on the WP"
            return None
        else:
            return sh

    def flattenFace(self,face):
        "Returns a face where all vertices have Z = 0"
        verts = []
        edges = fcgeo.sortEdges(face.Edges)
        for e in edges:
            v = e.Vertexes[0].Point
            verts.append(FreeCAD.Vector(v.x,v.y,0))
        verts.append(verts[0])
        try:
            sh = Part.makePolygon(verts)
            sh = Part.Face(sh)
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

    def sortFaces(self,face1,face2):
        "zsorts two faces. Returns 1 if face1 is closer, 2 if face2 is closer, 0 otherwise"

        # theory from
        # http://www.siggraph.org/education/materials/HyperGraph/scanline/visibility/painter.htm
        # and practical application http://vrm.ao2.it/ (blender vector renderer)

        b1 = face1.BoundBox
        b2 = face2.BoundBox

        if DEBUG: print "comparing face1: normal ", face1.normalAt(0,0), " with face2: normal ", face2.normalAt(0,0)

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
        if DEBUG: print "passed, faces are overlapping"

        # test 2: if Z bounds dont overlap, it's easy to know the closest
        if DEBUG: print "doing test 2"
        if b1.ZMax < b2.ZMin:
            return 2
        if b2.ZMax < b1.ZMin:
            return 1
        if DEBUG: print "passed, faces Z are crossed"

        # test 3: all verts of face1 are behind the plane of face2
        if DEBUG: print "doing test 3"
        norm = face2.normalAt(0,0)
        behind = 0
        for v in face1.Vertexes:
            dv = v.Point.sub(face2.Vertexes[0].Point)
            dv = fcvec.project(dv,norm)
            if dv.Length:
                if dv.getAngle(norm) > 0.1:
                    behind += 1
        if behind == len(face1.Vertexes):
            return 2
        if DEBUG: print "passed, face 1 is not behind"

        # test 4: all verts of face2 are in front of the plane of face1
        if DEBUG: print "doing test 4"
        norm = face1.normalAt(0,0)
        front = 0
        for v in face2.Vertexes:
            dv = v.Point.sub(face1.Vertexes[0].Point)
            dv = fcvec.project(dv,norm)
            if dv.Length:
                if dv.getAngle(norm) < 0.1:
                    front += 1
        if front == len(face2.Vertexes):
            return 2
        if DEBUG: print "passed, face 2 is not in front"

        # test 5: see if faces projections don't overlap, vertexwise
        if DEBUG: print "doing test 5"
        if not self.zOverlaps(face1,face2):
            return 0
        elif not self.zOverlaps(face2,face1):
            return 0
        if DEBUG: print "passed, faces are overlapping" 

        if DEBUG: print "Houston, all tests passed, and still no results" 
        return 0

    def sort(self):
        "projects a shape on the WP"
        if not self.faces: 
            return
        if not self.trimmed:
            self.removeHidden()
        if not self.oriented:
            self.reorient()
        faces = self.faces[:]
        if DEBUG: print "sorting faces: ", faces
        sfaces = []
        safecount = 0
        while faces:
            f1 = faces[0]
            for f2 in faces[1:]:
                if DEBUG: print "Scanning %d faces" % len(faces)
                z = self.sortFaces(f1,f2)
                if z == 1:
                    faces.remove(f2)
                    sfaces.insert(0,f2)
                elif z == 2:
                    faces.remove(f1)
                    sfaces.append(f2)
                    break
            safecount += 1
            if DEBUG: print "iteration ",safecount
            if safecount > (10 * len(faces)):
                if DEBUG: print "too long loop, aborting"
                break
        if DEBUG: print "done Z sorting. ", len(faces), " faces retained, ", len(self.faces)-len(faces), " faces lost."
        self.faces = faces
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

    def getSVG(self):
        "Returns a SVG fragment"
        if not self.sorted:
            self.sort()
        svg = ''
        for f in self.faces:
            svg +='<path '
            edges = fcgeo.sortEdges(f.Edges)
            v = edges[0].Vertexes[0].Point
            svg += 'd="M '+ str(v.x) +' '+ str(v.y) + ' '
            for e in edges:
                if isinstance(e.Curve,Part.Line) or isinstance(e.Curve,Part.BSplineCurve):
                    v = e.Vertexes[-1].Point
                    svg += 'L '+ str(v.x) +' '+ str(v.y) + ' '
                elif isinstance(e.Curve,Part.Circle):
                    r = e.Curve.Radius
                    v = e.Vertexes[-1].Point
                    svg += 'A '+ str(r) + ' '+ str(r) +' 0 0 1 '+ str(v.x) +' '
                    svg += str(v.y) + ' '
            svg += '" '
            svg += 'stroke="#000000" '
            svg += 'stroke-width="0.01 px" '
            svg += 'style="stroke-width:0.01;'
            svg += 'stroke-miterlimit:1;'
            svg += 'stroke-dasharray:none;'
            svg += 'fill:#aaaaaa"'
            svg += '/>\n'
        return svg
