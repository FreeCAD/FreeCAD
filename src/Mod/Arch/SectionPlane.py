import FreeCAD,FreeCADGui,Part,Component,WorkingPlane,Drawing,math
from FreeCAD import Vector
from PyQt4 import QtCore
from pivy import coin
from draftlibs import fcvec,fcgeo


class CommandSectionPlane:
    "the Arch SectionPlane command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_SectionPlane',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_SectionPlane","Section Plane"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_SectionPlane","Adds a section plane object to the document")}

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        FreeCAD.ActiveDocument.openTransaction("Section Plane")
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Section")
        SectionPlane(obj)
        ViewProviderSectionPlane(obj.ViewObject)
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
        ArchDrawingView(view)
        view.Source = obj
        FreeCAD.ActiveDocument.recompute()

class SectionPlane:
    "A section plane object"
    def __init__(self,obj):
        obj.Proxy = self
        obj.addProperty("App::PropertyLinkList","Objects","Base",
                        "The objects that must be considered by this section plane. Empty means all document")
        self.Type = "Section"
        self.Object = obj
        
    def execute(self,obj):
        pl = obj.Placement
        l = obj.ViewObject.DisplaySize
        p = Part.makePlane(l,l,Vector(l/2,-l/2,0),Vector(0,0,-1))
        obj.Shape = p
        obj.Placement = pl
        self.Object = obj

    def onChanged(self,obj,prop):
        pass

    def getNormal(self):
        return self.Object.Shape.Faces[0].normalAt(0,0)

class ViewProviderSectionPlane(Component.ViewProviderComponent):
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
        return """
            /* XPM */
            static char * Arch_SectionPlane_xpm[] = {
            "16 16 3 1",
            " 	c None",
            ".	c #000000",
            "+	c #FFFFFF",
            "     ......     ",
            "   ..+++.++..   ",
            "  .+++++..+++.  ",
            " .++++++...+++. ",
            " .++.+++....++. ",
            ".++.+.++.....++.",
            ".+.+++.+......+.",
            ".+.+++.+........",
            ".+.+++.+........",
            ".+.....+......+.",
            ".+.+++.+.....++.",
            " .++++++....++. ",
            " .++++++...+++. ",
            "  .+++++..+++.  ",
            "   ..+++.++..   ",
            "     ......     "};
            """

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

class ArchDrawingView:
    def __init__(self, obj):
        obj.addProperty("App::PropertyLink","Source","Base","The linked object")
        obj.Proxy = self
        self.Type = "DrawingView"

    def execute(self, obj):
        print "executing"
        if obj.Source:
            obj.ViewResult = self.updateSVG(obj)

    def onChanged(self, obj, prop):
        print "prop:",prop
        if prop == "Source":
            obj.ViewResult = self.updateSVG(obj)

    def updateSVG(self, obj):
        "encapsulates a svg fragment into a transformation node"
        if obj.Source:
            if obj.Source.Objects:
                svg = ''
                for o in obj.Source.Objects:
                    svg += self.getSVG(o,obj.Source.Proxy.getNormal())
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

    def getSVG(self,obj,direction,base=None):
        """returns an svg fragment from a SectionPlane object,
        a direction vector and optionally a base point"""

        print "getting representation of ",obj.Name," at ",direction

        # using Draft WorkingPlane
        plane = None
        if direction != Vector(0,0,0):
            plane = WorkingPlane.plane()
            plane.alignToPointAndAxis(Vector(0,0,0),fcvec.neg(direction),0)
        print "plane:",plane

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

        def getProj(vec):
            if not plane:
                return vec
            nx = fcvec.project(vec,plane.u)
            lx = nx.Length
            if abs(nx.getAngle(plane.u)) > 0.1: lx = -lx
            ny = fcvec.project(vec,plane.v)
            ly = ny.Length
            if abs(ny.getAngle(plane.v)) > 0.1: ly = -ly
            return Vector(lx,ly,0)

        def getPath(face):
            svg ='<path '
            edges = fcgeo.sortEdges(face.Edges)
            v = getProj(edges[0].Vertexes[0].Point)
            svg += 'd="M '+ str(v.x) +' '+ str(v.y) + ' '
            for e in edges:
                if isinstance(e.Curve,Part.Line) or  isinstance(e.Curve,Part.BSplineCurve):
                    v = getProj(e.Vertexes[-1].Point)
                    svg += 'L '+ str(v.x) +' '+ str(v.y) + ' '
                elif isinstance(e.Curve,Part.Circle):
                    r = e.Curve.Radius
                    v = getProj(e.Vertexes[-1].Point)
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

        sortedFaces = []

        if not base:
            # getting the base point = first point from the bounding box
            bb = obj.Shape.BoundBox
            rad = bb.DiagonalLength/2
            rv = bb.Center.add(direction)
            rv = fcvec.scaleTo(rv,rad)
            rv = fcvec.neg(rv)
            base = bb.Center.add(rv)

        print "base:",base

        # getting faces rays
        unsortedFaces = obj.Shape.Faces[:]
        for face in unsortedFaces:

            # testing if normal
            normal = face.normalAt(0,0)
            if normal.getAngle(direction) <= math.pi/2:
                print "normal pointing outwards"
                continue
            
            bhash = face.hashCode()
            center = face.CenterOfMass
            ray = center.sub(base)
            ray = fcvec.project(ray,direction)
            z = ray.Length
            if ray.getAngle(direction) > 1:
                z = -z

            print "face center:",center," ray:",ray

            tempFaces = [face]

            # comparing with other faces crossed by the same ray
            for of in unsortedFaces:
                obb = of.BoundBox
                op = intersection(base,ray,of.CenterOfMass,of.normalAt(0,0))
                if obb.isInside(op):
                    oray = op.sub(base)
                    oray = fcvec.project(oray,direction)
                    oz = oray.Length
                    if oray.getAngle(direction) > 1:
                        oz = -oz
                    if oz > 0:
                        if oz < z:
                            tempFaces.insert(0,of)
                        elif oz > z:
                            tempFaces.append(of)

            print "tempFaces:",tempFaces

            # finding the position of the base face among others
            findex = 0
            if len(tempFaces) > 1:
                for i in range(len(tempFaces)):
                    if tempFaces[i].hashCode() == bhash:
                        findex = i

            print "face index in tempfaces:",findex

            # finding the right place to insert in ordered list
            oindex = 0 
            if not sortedFaces:
                sortedFaces.append(face)
            elif len(tempFaces) == 1:
                sortedFaces.append(face)
            else:
                if findex == 0: # our face is the first item
                    ni = getFirstIndex(tempFaces[1:],sortedFaces)
                    if ni == None:
                        sortedFaces.append(face)
                    else:
                        sortedFaces.insert(ni,face)
                elif findex == len(tempFaces)-1: # our face is the last item
                    ni = getLastIndex(tempFaces[:-1],sortedFaces)
                    if ni == None:
                        sortedFaces.append(face)
                    else:
                        sortedFaces.insert(ni+1,face)
                else: # there are faces before and after
                    i1 = getLastIndex(tempFaces[:findex],sortedFaces)
                    i2 = getFirstIndex(tempFaces[findex+1:],sortedFaces)
                    if i1 == None:
                        if i2 == None:
                            sortedFaces.append(face)
                        else:
                            sortedFaces.insert(i2,face)
                    else:
                            sortedFaces.insert(i1+1,face)

        print "sorted faces:",len(sortedFaces)

        # building SVG representation
        svg = ''
        for f in sortedFaces:
            svg += getPath(f)

        print "result:",svg
            
        return svg


FreeCADGui.addCommand('Arch_SectionPlane',CommandSectionPlane())
