import FreeCAD,FreeCADGui,ArchComponent,WorkingPlane,Drawing,math
from FreeCAD import Vector
from PyQt4 import QtCore
from pivy import coin
from draftlibs import fcvec


class _CommandSectionPlane:
    "the Arch SectionPlane command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_SectionPlane',
                'Accel': "S, P",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_SectionPlane","Section Plane"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_SectionPlane","Adds a section plane object to the document")}

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        FreeCAD.ActiveDocument.openTransaction("Section Plane")
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Section")
        _SectionPlane(obj)
        _ViewProviderSectionPlane(obj.ViewObject)
        FreeCAD.ActiveDocument.commitTransaction()
        g = []
        for o in sel:
            if o.isDerivedFrom("Part::Feature"):
                g.append(o)
        obj.Objects = g
        page = FreeCAD.ActiveDocument.addObject("Drawing::FeaturePage","Page")
        template = FreeCAD.getResourceDir()+'Mod/Drawing/Templates/A3_Landscape.svg'
        page.ViewObject.HintOffsetX = 200
        page.ViewObject.HintOffsetY = 100
        page.ViewObject.HintScale = 20
        page.Template = template
        view = FreeCAD.ActiveDocument.addObject("Drawing::FeatureViewPython","View")
        page.addObject(view)
        _ArchDrawingView(view)
        view.Source = obj
        FreeCAD.ActiveDocument.recompute()

class _SectionPlane:
    "A section plane object"
    def __init__(self,obj):
        obj.Proxy = self
        obj.addProperty("App::PropertyLinkList","Objects","Base",
                        "The objects that must be considered by this section plane. Empty means all document")
        self.Type = "SectionPlane"
        
    def execute(self,obj):
        import Part
        pl = obj.Placement
        l = obj.ViewObject.DisplaySize
        p = Part.makePlane(l,l,Vector(l/2,-l/2,0),Vector(0,0,-1))
        obj.Shape = p
        obj.Placement = pl

    def onChanged(self,obj,prop):
        pass

    def getNormal(self,obj):
        return obj.Shape.Faces[0].normalAt(0,0)

class _ViewProviderSectionPlane(ArchComponent.ViewProviderComponent):
    "A View Provider for Section Planes"
    def __init__(self,vobj):
        vobj.addProperty("App::PropertyLength","DisplaySize","Base",
                        "The display size of the section plane")
        vobj.DisplaySize = 1
        vobj.Transparency = 85
        vobj.LineWidth = 1
        vobj.LineColor = (0.0,0.0,0.4,1.0)
        vobj.Proxy = self
        self.Object = vobj.Object

    def getIcon(self):
        return ":/icons/Arch_SectionPlane_Tree.svg"

    def claimChildren(self):
        return []

    def attach(self,vobj):
        self.Object = vobj.Object
        # adding arrows
        rn = vobj.RootNode
        self.col = coin.SoBaseColor()
        self.setColor()
        ds = coin.SoDrawStyle()
        ds.style = coin.SoDrawStyle.LINES
        self.lcoords = coin.SoCoordinate3()
        ls = coin.SoLineSet()
        ls.numVertices.setValues([2,4,4,2,4,4,2,4,4,2,4,4])
        pt = coin.SoAnnotation()
        pt.addChild(self.col)
        pt.addChild(ds)
        pt.addChild(self.lcoords)
        pt.addChild(ls)
        rn.addChild(pt)
        self.setVerts()

    def setColor(self):
        print self.Object.ViewObject.LineColor
        self.col.rgb.setValue(self.Object.ViewObject.LineColor[0],
                              self.Object.ViewObject.LineColor[1],
                              self.Object.ViewObject.LineColor[2])

    def setVerts(self):
        def extendVerts(x,y):
            l1 = hd/3
            l2 = l1/3
            verts.extend([[x,y,0],[x,y,-l1]])
            verts.extend([[x,y,-l1],[x-l2,y,-l1+l2],[x+l2,y,-l1+l2],[x,y,-l1]])
            verts.extend([[x,y,-l1],[x,y-l2,-l1+l2],[x,y+l2,-l1+l2],[x,y,-l1]])
        hd = self.Object.ViewObject.DisplaySize/2
        verts = []
        extendVerts(-hd,-hd)
        extendVerts(hd,-hd)
        extendVerts(hd,hd)
        extendVerts(-hd,hd)
        self.lcoords.point.setValues(verts)

    def updateData(self,obj,prop):
        if prop in ["Shape","Placement"]:
            self.setVerts()
        return

    def onChanged(self,vobj,prop):
        if prop == "LineColor":
            self.setColor()
        elif prop == "DisplaySize":
            vobj.Object.Proxy.execute(vobj.Object)
        return

class _ArchDrawingView:
    def __init__(self, obj):
        obj.addProperty("App::PropertyLink","Source","Base","The linked object")
        obj.addProperty("App::PropertyEnumeration","RenderingMode","Base","The rendering mode to use")
        obj.RenderingMode = ["Z-sorted","Wireframe","Wireframe + shade"]
        obj.RenderingMode = "Z-sorted"
        obj.Proxy = self
        self.Type = "DrawingView"

    def execute(self, obj):
        if obj.Source:
            obj.ViewResult = self.updateSVG(obj)
            
    def onChanged(self, obj, prop):
        if prop in ["Source","RenderingMode"]:
            obj.ViewResult = self.updateSVG(obj)

    def updateSVG(self, obj):
        "encapsulates a svg fragment into a transformation node"
        if obj.Source:
            if obj.Source.Objects:
                svg = ''
                if obj.RenderingMode == "Z-sorted":
                    svg += self.renderClassicSVG(obj.Source.Objects,obj.Source.Proxy.getNormal(obj.Source))
                elif obj.RenderingMode == "Wireframe":
                    svg += self.renderWireframeSVG(obj.Source.Objects,obj.Source.Proxy.getNormal(obj.Source))
                elif obj.RenderingMode == "Wireframe + shade":
                    svg += self.renderOutlineSVG(obj.Source.Objects,obj.Source.Proxy.getNormal(obj.Source))
                    svg += self.renderWireframeSVG(obj.Source.Objects,obj.Source.Proxy.getNormal(obj.Source))
                result = ''
                result += '<g id="' + obj.Name + '"'
                result += ' transform="'
                result += 'rotate('+str(obj.Rotation)+','+str(obj.X)+','+str(obj.Y)+') '
                result += 'translate('+str(obj.X)+','+str(obj.Y)+') '
                result += 'scale('+str(obj.Scale)+','+str(-obj.Scale)+')'
                result += '">\n'
                result += svg
                result += '</g>\n'
                print "complete node:",result
                return result
        return ''

    def getProj(self,vec,plane):
        "returns a vector in working plane space from the given vector"
        if not plane:
            return vec
        nx = fcvec.project(vec,plane.u)
        lx = nx.Length
        if abs(nx.getAngle(plane.u)) > 0.1: lx = -lx
        ny = fcvec.project(vec,plane.v)
        ly = ny.Length
        if abs(ny.getAngle(plane.v)) > 0.1: ly = -ly
        return Vector(lx,ly,0)

    def getPath(self,face,plane):
        import Part
        from draftlibs import fcgeo
        "returns a svg path from a face"
        svg ='<path '
        edges = fcgeo.sortEdges(face.Edges)
        v = self.getProj(edges[0].Vertexes[0].Point,plane)
        svg += 'd="M '+ str(v.x) +' '+ str(v.y) + ' '
        for e in edges:
            if isinstance(e.Curve,Part.Line) or isinstance(e.Curve,Part.BSplineCurve):
                v = self.getProj(e.Vertexes[-1].Point,plane)
                svg += 'L '+ str(v.x) +' '+ str(v.y) + ' '
            elif isinstance(e.Curve,Part.Circle):
                r = e.Curve.Radius
                v = self.getProj(e.Vertexes[-1].Point,plane)
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

    def renderWireframeSVG(self,objs,direction):
        os = objs[:]
        if os:
            sh = os.pop().Shape
            for o in os:
                sh = sh.fuse(o.Shape)
        result = Drawing.projectToSVG(sh,fcvec.neg(direction))
        if result:
            result = result.replace('stroke-width="0.35"','stroke-width="0.01 px"')
            return result
        return ''

    def renderOutlineSVG(self,objs,direction):
        plane = None
        plane = WorkingPlane.plane()
        if direction != Vector(0,0,0):
            plane.alignToPointAndAxis(Vector(0,0,0),fcvec.neg(direction),0)
        else:
            direction = Vector(0,0,-1)
        faces = []
        for obj in objs:
            for face in obj.Shape.Faces:
                normal = face.normalAt(0,0)
                if normal.getAngle(direction) > math.pi/2:
                    faces.append(face)
        print "faces:",faces
        if faces:
            base = faces.pop()
            for face in faces:
                base = base.oldFuse(face)
        result = self.getPath(base,plane)
        return result        

    def renderClassicSVG(self,objs,direction,base=None):
        """returns an svg fragment from a SectionPlane object,
        a direction vector and optionally a base point"""

        def intersection(p1,p2,p3,p4):
            "returns the intersection of line (p1,p2) with plane (p3,p4)"
            # http://paulbourke.net/geometry/planeline/
            dn = p4.dot(p2.sub(p1))
            if dn != 0:
                u = (p4.dot(p3.sub(p1))) / dn 
                p = p1.add((p2.sub(p1)).scale(u,u,u))
                return p
            else:
                # line is parallel to normal
                vp = fcvec.project(p3.sub(p1),p2.sub(p1))
                l = vp.Length
                if vp.getAngle(p2.sub(p1)) > 1:
                    l = -l
                return fcvec.scaleTo(p2.sub(p1),l)

        def getFirstIndex(list1,list2):
            "returns the first index from list2 where there is an item of list1"
            for i1 in range(len(list1)):
                for i2 in range(len(list2)):
                            if list1[i1].hashCode() == list2[i2].hashCode():
                                return i2
            return None

        def getLastIndex(list1,list2):
            "returns the last index from list2 where there is an item of list1"
            i = None
            for i1 in range(len(list1)):
                for i2 in range(len(list2)):
                            if list1[i1].hashCode() == list2[i2].hashCode():
                                i = i2
            return i

        def findPrevious(base,dir,faces):
            "returns the highest index in faces that is crossed by the given line"
            for i in range(len(faces)-1,-1,-1):
                print "p1:",base," p2: ",base.add(dir)
                obb = faces[i].BoundBox
                print "bo: ",obb
                op = intersection(base,base.add(dir),faces[i].CenterOfMass,faces[i].normalAt(0,0))
                print "int:", op
                if obb.isInside(op):
                    dv = op.sub(base)
                    if dv.getAngle(dir) < math.pi/2:
                        return i
            return None
            
        def findNext(base,dir,faces):
            "returns the lowest index in faces that is crossed by the given line"
            for i in range(len(faces)):
                obb = faces[i].BoundBox
                op = intersection(base,base.add(dir),faces[i].CenterOfMass,faces[i].normalAt(0,0))
                if obb.isInside(op):
                    dv = op.sub(base)
                    if dv.getAngle(dir) > math.pi/2:
                        return i
            return None
       
        print "getting representation at ",direction," =======================================>"

        # using Draft WorkingPlane
        plane = None
        plane = WorkingPlane.plane()
        if direction != Vector(0,0,0):
            plane.alignToPointAndAxis(Vector(0,0,0),fcvec.neg(direction),0)
        else:
            direction = Vector(0,0,-1)
        print "plane:",plane
                
        sortedFaces = []

        if not base:
            # getting the base point = first point from the bounding box
            bb = FreeCAD.BoundBox()
            for o in objs:
                bb.add(o.Shape.BoundBox)
            rad = bb.DiagonalLength/2
            rv = bb.Center.add(direction)
            rv = fcvec.scaleTo(rv,rad)
            rv = fcvec.neg(rv)
            base = bb.Center.add(rv)

        print "base:",base

        # getting faces
        unsortedFaces = []
        notFoundFaces = []
        for o in objs:
            unsortedFaces.append(o.Name)
            unsortedFaces.extend(o.Shape.Faces[:])
        print "analyzing ",len(unsortedFaces)," faces"
        
        for face in unsortedFaces:

            if isinstance(face,str):
                print "OBJECT ",face," =======================================>"
                continue

            print "testing face ",unsortedFaces.index(face)

            # testing if normal points outwards
            normal = face.normalAt(0,0)
            if normal.getAngle(direction) <= math.pi/2:
                print "normal pointing outwards"
                continue

            fprev = 0
            fnext = len(sortedFaces)
            notFound = True

            print "checking ",len(face.Vertexes)," verts"

            for v in face.Vertexes:
                vprev = findPrevious(v.Point,direction,sortedFaces)
                vnext = findNext(v.Point,direction,sortedFaces)
                print "temp indexes:",vprev,vnext
                if (vprev != None):
                    notfound = False
                    if (vprev > fprev):
                        fprev = vprev
                if (vnext != None):
                    notfound = False
                    if (vnext < fnext):
                        fnext = vnext

            print "fprev:",fprev
            print "fnext:",fnext
            print "notFound",notFound

            if fnext < fprev:
                raise "Error, impossible index"
            elif fnext == fprev:
                sortedFaces.insert(fnext,face)
            else:
                sortedFaces.insert(fnext,face)

            print len(sortedFaces)," sorted faces:",sortedFaces

        # building SVG representation in correct order
        svg = ''
        for f in sortedFaces:
            svg += self.getPath(f,plane)
            
        return svg

FreeCADGui.addCommand('Arch_SectionPlane',_CommandSectionPlane())
