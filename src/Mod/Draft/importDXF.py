# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
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

__title__="FreeCAD Draft Workbench - DXF importer/exporter"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = ["http://www.freecadweb.org"]

'''
This script uses a DXF-parsing library created by Stani,
Kitsu and Migius for Blender

imports:
line, polylines, lwpolylines, arcs, circles, texts,
mtexts, layers (as groups), colors

exports:
lines, polylines, lwpolylines, circles, arcs,
texts, colors,layers (from groups)
'''

TEXTSCALING = 1.35 # scaling factor between autocad font sizes and coin font sizes
CURRENTDXFLIB = 1.38 # the minimal version of the dxfLibrary needed to run

import sys, FreeCAD, os, Part, math, re, string, Mesh, Draft, DraftVecUtils, DraftGeomUtils
from Draft import _Dimension, _ViewProviderDimension
from FreeCAD import Vector

gui = FreeCAD.GuiUp
draftui = None
if gui:
    import FreeCADGui
    try:
        draftui = FreeCADGui.draftToolBar
    except AttributeError:
        pass

def errorDXFLib(gui):
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    dxfAllowDownload = p.GetBool("dxfAllowDownload",False)
    if dxfAllowDownload:
        files = ['dxfColorMap.py','dxfImportObjects.py','dxfLibrary.py','dxfReader.py']
        baseurl = 'https://raw.githubusercontent.com/yorikvanhavre/Draft-dxf-importer/'+str(CURRENTDXFLIB)+"/"
        import ArchCommands
        from FreeCAD import Base
        progressbar = Base.ProgressIndicator()
        progressbar.start("Downloading files...",4)
        for f in files:
            progressbar.next()
            p = None
            p = ArchCommands.download(baseurl+f,force=True)
            if not p:
                if gui:
                    from PySide import QtGui, QtCore
                    from DraftTools import translate
                    message = translate("Draft","""Download of dxf libraries failed.
Please download them manually from:
https://github.com/yorikvanhavre/Draft-dxf-importer
and place them in your macros folder.""")
                    QtGui.QMessageBox.information(None,"",message)
                else:
                    FreeCAD.Console.PrintWarning("The DXF import/export libraries needed by FreeCAD to handle the DXF format are not installed.\n")
                    FreeCAD.Console.PrintWarning("Please check https://github.com/yorikvanhavre/Draft-dxf-importer\n")
                break
        progressbar.stop()
        sys.path.append(FreeCAD.ConfigGet("UserAppData"))
    else:
        if gui:
            from PySide import QtGui, QtCore
            from DraftTools import translate
            message = translate('draft',"""The DXF import/export libraries needed by FreeCAD to handle
the DXF format were not found on this system.
Please either enable FreeCAD to download these libraries:
  1 - Load Draft workbench
  2 - Menu Edit > Preferences > Import-Export > DXF > Enable downloads
Or download these libraries manually, as explained on
https://github.com/yorikvanhavre/Draft-dxf-importer
To enabled FreeCAD to download these libraries, answer Yes.""")
            reply = QtGui.QMessageBox.question(None,"",message,
                QtGui.QMessageBox.Yes | QtGui.QMessageBox.No, QtGui.QMessageBox.No)
            if reply == QtGui.QMessageBox.Yes:
                p.SetBool("dxfAllowDownload",True)
                errorDXFLib(gui)
            if reply == QtGui.QMessageBox.No:
                pass
        else:
            FreeCAD.Console.PrintWarning("The DXF import/export libraries needed by FreeCAD to handle the DXF format are not installed.\n")
            FreeCAD.Console.PrintWarning("Please check https://github.com/yorikvanhavre/Draft-dxf-importer\n")

# check dxfLibrary version
try:
    if FreeCAD.ConfigGet("UserAppData") not in sys.path:
        sys.path.append(FreeCAD.ConfigGet("UserAppData"))
    import dxfLibrary
    import dxfColorMap
    import dxfReader
except ImportError:
    libsok = False
    FreeCAD.Console.PrintWarning("DXF libraries not found. Trying to download...\n")
else:
    if "v"+str(CURRENTDXFLIB) in dxfLibrary.__version__:
        libsok = True
    else:
        FreeCAD.Console.PrintWarning("DXF libraries need to be updated. Trying to download...\n")
        libsok = False
if not libsok:
    errorDXFLib(gui)
    try:
        import dxfColorMap, dxfLibrary, dxfReader
    except ImportError:
        dxfReader = None
        dxfLibrary = None

if open.__module__ == '__builtin__':
    pythonopen = open # to distinguish python built-in open function from the one declared here

def prec():
    "returns the current Draft precision level"
    return Draft.getParam("precision",6)

def decodeName(name):
    "decodes encoded strings"
    try:
        decodedName = (name.decode("utf8"))
    except UnicodeDecodeError:
        try:
            decodedName = (name.decode("latin1"))
        except UnicodeDecodeError:
                print("dxf: error: couldn't determine character encoding")
                decodedName = name
    return decodedName

def deformat(text):
    "removes weird formats in texts and wipes UTF characters"
    # remove ACAD string formatation
    #t = re.sub('{([^!}]([^}]|\n)*)}', '', text)
    #print("input text: ",text)
    t = text.strip("{}")
    t = re.sub("\\\.*?;","",t)
    # replace UTF codes by utf chars
    sts = re.split("\\\\(U\+....)",t)
    ns = u""
    for ss in sts:
        #print(ss, type(ss))
        if ss.startswith("U+"):
            ucode = "0x"+ss[2:]
            ns += unichr(eval(ucode)) #Python3 - unichr doesn't exist anymore
        else:
            try:
                ns += ss.decode("utf8")
            except UnicodeError:
                try:
                    ns += ss.decode("latin1")
                except UnicodeError:
                    print("unable to decode text: ",text)
    t = ns
    # replace degrees, diameters chars
    t = re.sub('%%d','°',t)
    t = re.sub('%%c','Ø',t)
    #print("output text: ",t)
    return t

def locateLayer(wantedLayer,color=None):
    "returns layer group and creates it if needed"
    wantedLayerName = decodeName(wantedLayer)
    for l in layers:
        if wantedLayerName==l.Label:
            return l
    if dxfUseDraftVisGroups:
        newLayer = Draft.makeVisGroup(name=wantedLayer)
    else:
        newLayer = doc.addObject("App::DocumentObjectGroup",wantedLayer)
    newLayer.Label = wantedLayerName
    layers.append(newLayer)
    return newLayer

def getdimheight(style):
    "returns the dimension text height from the given dimstyle"
    for t in drawing.tables.data:
        if t.name == 'dimstyle':
            for a in t.data:
                if hasattr(a,"type"):
                    if a.type == "dimstyle":
                        if rawValue(a,2) == style:
                            return rawValue(a,140)
    return 1

def calcBulge(v1,bulge,v2):
    '''
    calculates intermediary vertex for curved segments.
    algorithm from http://www.afralisp.net/lisp/Bulges1.htm
    '''
    chord = v2.sub(v1)
    sagitta = (bulge * chord.Length)/2
    perp = chord.cross(Vector(0,0,1))
    startpoint = v1.add(chord.multiply(0.5))
    if not DraftVecUtils.isNull(perp): perp.normalize()
    endpoint = perp.multiply(sagitta)
    return startpoint.add(endpoint)

def getGroup(ob):
    "checks if the object is part of a group"
    for i in FreeCAD.ActiveDocument.Objects:
        if i.isDerivedFrom("App::DocumentObjectGroup"):
            for j in i.Group:
                if (j == ob):
                    return i.Label
    return "0"

def getACI(ob,text=False):
    "gets the ACI color closest to the objects color"
    if not gui:
        return 0
    else:
        if text:
            col=ob.ViewObject.TextColor
        else:
            col=ob.ViewObject.LineColor
        aci=[0,442]
        for i in range (255,-1,-1):
            ref=dxfColorMap.color_map[i]
            dist=((ref[0]-col[0])**2 + (ref[1]-col[1])**2 + (ref[2]-col[2])**2)
            if (dist <= aci[1]): aci=[i,dist]
        return aci[0]

def rawValue(entity,code):
    "returns the  value of a DXF code in an entity section"
    value = None
    for pair in entity.data:
        if pair[0] == code:
            value = pair[1]
    return value

def getMultiplePoints(entity):
    "scans the given entity for multiple points (paths, leaders, etc)"
    pts = []
    for d in entity.data:
        if d[0] == 10:
            pts.append([d[1]])
        elif d[0] in [20,30]:
            pts[-1].append(d[1])
    pts.reverse()
    points = []
    for p in pts:
        if len(p) == 3:
            points.append(Vector(p[0],p[1],p[2]))
        else:
            points.append(Vector(p[0],p[1],0))
    return points

def isBrightBackground():
    "checks if the current viewport background is bright"
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
    if p.GetBool("Gradient"):
        c1 = p.GetUnsigned("BackgroundColor2")
        c2 = p.GetUnsigned("BackgroundColor3")
        r1 = float((c1>>24)&0xFF)
        g1 = float((c1>>16)&0xFF)
        b1 = float((c1>>8)&0xFF)
        r2 = float((c2>>24)&0xFF)
        g2 = float((c2>>16)&0xFF)
        b2 = float((c2>>8)&0xFF)
        v1 = FreeCAD.Vector(r1,g1,b1)
        v2 = FreeCAD.Vector(r2,g2,b2)
        v = v2.sub(v1)
        v.multiply(0.5)
        cv = v1.add(v)
    else:
        c1 = p.GetUnsigned("BackgroundColor")
        r1 = float((c1>>24)&0xFF)
        g1 = float((c1>>16)&0xFF)
        b1 = float((c1>>8)&0xFF)
        cv = FreeCAD.Vector(r1,g1,b1)
    value = cv.x*.3 + cv.y*.59 + cv.z*.11
    if value < 128:
        return False
    else:
        return True

def getGroupColor(dxfobj,index=False):
    "get color of bylayer stuff"
    name = dxfobj.layer
    for table in drawing.tables.get_type("table"):
        if table.name == "layer":
            for l in table.get_type("layer"):
                if l.name == name:
                    if index:
                        return l.color
                    else:
                        if (l.color == 7) and dxfBrightBackground:
                            return [0.0,0.0,0.0]
                        else:
                            if isinstance(l.color,int):
                                if l.color > 0:
                                    return dxfColorMap.color_map[l.color]
    return [0.0,0.0,0.0]

def getColor():
    if gui and draftui:
        r = float(draftui.color.red()/255.0)
        g = float(draftui.color.green()/255.0)
        b = float(draftui.color.blue()/255.0)
        return (r,g,b,0.0)
    else:
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
        c = p.GetUnsigned("DefaultShapeLineColor",0)
        r = float(((c>>24)&0xFF)/255)
        g = float(((c>>16)&0xFF)/255)
        b = float(((c>>8)&0xFF)/255)
        return (r,g,b,0.0)

def formatObject(obj,dxfobj=None):
    "applies color and linetype to objects"
    if dxfGetColors and dxfobj and hasattr(dxfobj,"color_index"):
        if hasattr(obj.ViewObject,"TextColor"):
            if dxfobj.color_index == 256:
                cm = getGroupColor(dxfobj)[:3]
            else:
                cm = dxfColorMap.color_map[dxfobj.color_index]
            obj.ViewObject.TextColor = (cm[0],cm[1],cm[2])
        elif hasattr(obj.ViewObject,"LineColor"):
            if dxfobj.color_index == 256:
                cm = getGroupColor(dxfobj)
            elif (dxfobj.color_index == 7) and dxfBrightBackground:
                cm = [0.0,0.0,0.0]
            else:
                cm = dxfColorMap.color_map[dxfobj.color_index]
            obj.ViewObject.LineColor = (cm[0],cm[1],cm[2],0.0)
    else:
        if hasattr(obj.ViewObject,"TextColor"):
            obj.ViewObject.TextColor = dxfDefaultColor
        elif hasattr(obj.ViewObject,"LineColor"):
            obj.ViewObject.LineColor = dxfDefaultColor

def vec(pt):
    "returns a rounded Vector from a dxf point"
    return FreeCAD.Vector(round(pt[0],prec()),round(pt[1],prec()),round(pt[2],prec()))

def drawLine(line,forceShape=False):
    "returns a Part shape from a dxf line"
    if (len(line.points) > 1):
        v1=vec(line.points[0])
        v2=vec(line.points[1])
        if not DraftVecUtils.equals(v1,v2):
            try:
                if (dxfCreateDraft or dxfCreateSketch) and (not forceShape):
                    return Draft.makeWire([v1,v2])
                else:
                    return Part.Line(v1,v2).toShape()
            except Part.OCCError:
                warn(line)
    return None

def drawPolyline(polyline,forceShape=False,num=None):
    "returns a Part shape from a dxf polyline"
    if (len(polyline.points) > 1):
        edges = []
        curves = False
        verts = []
        for p in range(len(polyline.points)-1):
            p1 = polyline.points[p]
            p2 = polyline.points[p+1]
            v1 = vec(p1)
            v2 = vec(p2)
            verts.append(v1)
            if not DraftVecUtils.equals(v1,v2):
                if polyline.points[p].bulge:
                    curves = True
                    cv = calcBulge(v1,polyline.points[p].bulge,v2)
                    if DraftVecUtils.isColinear([v1,cv,v2]):
                        try: edges.append(Part.Line(v1,v2).toShape())
                        except Part.OCCError: warn(polyline,num)
                    else:
                        try: edges.append(Part.Arc(v1,cv,v2).toShape())
                        except Part.OCCError: warn(polyline,num)
                else:
                    try: edges.append(Part.Line(v1,v2).toShape())
                    except Part.OCCError: warn(polyline,num)
        verts.append(v2)
        if polyline.closed:
            p1 = polyline.points[len(polyline.points)-1]
            p2 = polyline.points[0]
            v1 = vec(p1)
            v2 = vec(p2)
            cv = calcBulge(v1,polyline.points[-1].bulge,v2)
            if not DraftVecUtils.equals(v1,v2):
                if DraftVecUtils.isColinear([v1,cv,v2]):
                    try:
                        edges.append(Part.Line(v1,v2).toShape())
                    except Part.OCCError:
                        warn(polyline,num)
                else:
                    try:
                        edges.append(Part.Arc(v1,cv,v2).toShape())
                    except Part.OCCError:
                        warn(polyline,num)
        if edges:
            try:
                width = rawValue(polyline,43)
                if width and dxfRenderPolylineWidth:
                    w = Part.Wire(edges)
                    w1 = w.makeOffset(width/2)
                    if polyline.closed:
                        w2 = w.makeOffset(-width/2)
                        w1 = Part.Face(w1)
                        w2 = Part.Face(w2)
                        if w1.BoundBox.DiagonalLength > w2.BoundBox.DiagonalLength:
                            return w1.cut(w2)
                        else:
                            return w2.cut(w1)
                    else:
                        return Part.Face(w1)
                elif (dxfCreateDraft or dxfCreateSketch) and (not curves) and (not forceShape):
                    ob = Draft.makeWire(verts)
                    ob.Closed = polyline.closed
                    return ob
                else:
                    if polyline.closed and dxfFillMode:
                        w = Part.Wire(edges)
                        return(Part.Face(w))
                    else:
                        return Part.Wire(edges)
            except Part.OCCError:
                warn(polyline,num)
    return None

def drawArc(arc,forceShape=False):
    "returns a Part shape from a dxf arc"
    v=vec(arc.loc)
    firstangle=round(arc.start_angle,prec())
    lastangle=round(arc.end_angle,prec())
    circle=Part.Circle()
    circle.Center=v
    circle.Radius=round(arc.radius,prec())
    try:
        if (dxfCreateDraft or dxfCreateSketch) and (not forceShape):
            pl = FreeCAD.Placement()
            pl.move(v)
            return Draft.makeCircle(arc.radius,pl,False,firstangle,lastangle)
        else:
            return circle.toShape(math.radians(firstangle),math.radians(lastangle))
    except Part.OCCError:
        warn(arc)
    return None

def drawCircle(circle,forceShape=False):
    "returns a Part shape from a dxf circle"
    v = vec(circle.loc)
    curve = Part.Circle()
    curve.Radius = round(circle.radius,prec())
    curve.Center = v
    try:
        if (dxfCreateDraft or dxfCreateSketch) and (not forceShape):
            pl = FreeCAD.Placement()
            pl.move(v)
            return Draft.makeCircle(circle.radius,pl)
        else:
            return curve.toShape()
    except Part.OCCError:
        warn(circle)
    return None

def drawEllipse(ellipse):
    "returns a Part shape from a dxf arc"
    try:
        c = vec(ellipse.loc)
        start = round(ellipse.start_angle,prec())
        end = round(ellipse.end_angle,prec())
        majv = vec(ellipse.major)
        majr = majv.Length
        minr = majr*ellipse.ratio
        el = Part.Ellipse(vec((0,0,0)),majr,minr)
        x = majv.normalize()
        z = vec(ellipse.extrusion).normalize()
        y = z.cross(x)
        m = DraftVecUtils.getPlaneRotation(x,y,z)
        pl = FreeCAD.Placement(m)
        pl.move(c)
        if (dxfCreateDraft or dxfCreateSketch) and (not forceShape):
            if (start != 0.0) or ((end != 0.0) or (end != round(math.pi/2,prec()))):
                shape = el.toShape(start,end)
                shape.Placement = pl
                return shape
            else:
                return Draft.makeEllipse(majr,minr,pl)
        else:
            shape = el.toShape(start,end)
            shape.Placement = pl
            return shape
    except Part.OCCError:
        warn(arc)
    return None

def drawFace(face):
    "returns a Part face from a list of points"
    pl = []
    for p in face.points:
        pl.append(vec(p))
    p1 = face.points[0]
    pl.append(vec(p1))
    try:
        pol = Part.makePolygon(pl)
        return Part.Face(pol)
    except Part.OCCError:
        warn(face)
    return None

def drawMesh(mesh):
    "returns a Mesh from a dxf mesh"
    md = []
    if mesh.flags == 16:
        pts = mesh.points
        udim = rawValue(mesh,71)
        vdim = rawValue(mesh,72)
        for u in range(udim-1):
            for v in range(vdim-1):
                b = u+v*udim
                p1 = pts[b]
                p2 = pts[b+1]
                p3 = pts[b+udim]
                p4 = pts[b+udim+1]
                md.append([p1,p2,p4])
                md.append([p1,p4,p3])
    elif mesh.flags == 64:
        pts = []
        fcs = []
        for p in mesh.points:
            if p.flags == 192:
                pts.append(p)
            elif p.flags == 128:
                fcs.append(p)
        for f in fcs:
            p1 = pts[rawValue(f,71)-1]
            p2 = pts[rawValue(f,72)-1]
            p3 = pts[rawValue(f,73)-1]
            md.append([p1,p2,p3])
            if rawValue(f,74) != None:
                p4 = pts[rawValue(f,74)-1]
                md.append([p1,p3,p4])
    try:
        return Mesh.Mesh(md)
    except FreeCAD.Base.FreeCADError:
        warn(mesh)
    return None

def drawSolid(solid):
    "returns a Part shape from a dxf solid"
    p4 = None
    p1x = rawValue(solid,10)
    p1y = rawValue(solid,20)
    p1z = rawValue(solid,30) or 0
    p2x = rawValue(solid,11)
    p2y = rawValue(solid,21)
    p2z = rawValue(solid,31) or p1z
    p3x = rawValue(solid,12)
    p3y = rawValue(solid,22)
    p3z = rawValue(solid,32) or p1z
    p4x = rawValue(solid,13)
    p4y = rawValue(solid,23)
    p4z = rawValue(solid,33) or p1z
    p1 = FreeCAD.Vector(p1x,p1y,p1z)
    p2 = FreeCAD.Vector(p2x,p2y,p2z)
    p3 = FreeCAD.Vector(p3x,p3y,p3z)
    if p4x != None: p4 = FreeCAD.Vector(p4x,p4y,p4z)
    if p4 and (p4 != p3) and (p4 != p2) and (p4 != p1):
        try:
            return Part.Face(Part.makePolygon([p1,p2,p4,p3,p1]))
        except Part.OCCError:
            warn(solid)
    else:
        try:
            return Part.Face(Part.makePolygon([p1,p2,p3,p1]))
        except Part.OCCError:
            warn(solid)
    return None

def drawSplineIterpolation(verts,closed=False,forceShape=False,\
        alwaysDiscretize=False):
    if (dxfCreateDraft or dxfCreateSketch) and (not forceShape):
        if dxfDiscretizeCurves or alwaysDiscretize:
            ob = Draft.makeWire(verts)
        else:
            ob = Draft.makeBSpline(verts)
        ob.Closed = closed
        return ob
    else:
        if dxfDiscretizeCurves or alwaysDiscretize:
            sh = Part.makePolygon(verts+[verts[0]])
        else:
            sp = Part.BSplineCurve()
            # print(knots)
            sp.interpolate(verts)
            sh = Part.Wire(sp.toShape())
        if closed and dxfFillMode:
            return Part.Face(sh)
        else:
            return sh

def drawSplineOld(spline,forceShape=False):
    "returns a Part Shape from a dxf spline"
    flag = rawValue(spline,70)
    if flag == 1:
        closed = True
    else:
        closed = False
    verts = []
    knots = []
    for dline in spline.data:
        if dline[0] == 10:
            cp = [dline[1]]
        elif dline[0] == 20:
            cp.append(dline[1])
        elif dline[0] == 30:
            cp.append(dline[1])
            pt = Vector(cp[0],cp[1],cp[2])
            if verts:
                if pt != verts[-1]:
                    verts.append(pt)
            else:
                verts.append(pt)
        elif dline[0] == 40:
            knots.append(dline[1])
    try:
        return drawSplineIterpolation(verts,closed,forceShape)
    except Part.OCCError:
        warn(spline)
    return None

def drawSpline(spline,forceShape=False):
    """returns a Part Shape from a dxf spline
as there is currently no Draft premitive to handle splines the result is a
non-parametric curve"""
    flags = rawValue(spline,70)
    closed   = (flags &  1) != 0
    periodic = (flags &  2) != 0 and False # workaround
    rational = (flags &  4) != 0
    planar   = (flags &  8) != 0
    linear   = (flags & 16) != 0
    degree     = rawValue(spline,71)
    nbknots    = rawValue(spline,72) or 0
    nbcontrolp = rawValue(spline,73) or 0
    nbfitp     = rawValue(spline,74) or 0
    knots = []
    weights = []
    controlpoints = []
    fitpoints = []
    # parse the knots and points
    dataremain = spline.data[:]
    while len(dataremain) >0:
        groupnumber = dataremain[0][0]
        if groupnumber == 40: #knot
            knots.append(dataremain[0][1])
            dataremain = dataremain[1:]
        elif groupnumber == 41: #weight
            weights.append(dataremain[0][1])
            dataremain = dataremain[1:]
        elif groupnumber in (10,11): # control or fit point
            x = dataremain[0][1]
            if dataremain[1][0] in (20,21):
                y=dataremain[1][1]
                if dataremain[2][0] in (30,31):
                    z=dataremain[2][1]
                    dataremain = dataremain[3:]
                else:
                    z=0.0
                    dataremain = dataremain[2:]
            else:
                y=0.0
                dataremain = dataremain[1:]
            vec = FreeCAD.Vector(x,y,z)
            if groupnumber == 10:
                controlpoints.append(vec)
            elif groupnumber == 11:
                fitpoints.append(vec)
        else:
            dataremain = dataremain[1:]
            #print groupnumber #debug

    if nbknots != len(knots):
        raise ValueError('Wrong number of knots')
    if nbcontrolp != len(controlpoints):
        raise ValueError('Wrong number of control points')
    if nbfitp != len(fitpoints):
        raise ValueError('Wrong number of fit points')
    if rational == all((w == 1.0 or w is None) for w in weights):
        raise ValueError('inconsistant rational flag')
    if len(weights) == 0:
        weights = None
    elif len(weights) != len(controlpoints):
        raise ValueError('Wrong number of weights')

    # build knotvector and multvector
    # this means to remove duplicate knots
    multvector=[]
    knotvector=[]
    mult=0
    previousknot=None
    for knotvalue in knots:
        if knotvalue == previousknot:
            mult += 1
        else:
            if mult > 0:
                multvector.append(mult)
            mult = 1
            previousknot = knotvalue
            knotvector.append(knotvalue)
    multvector.append(mult)
    # check if the multiplicities are valid
    innermults = multvector[:] if periodic else multvector[1:-1]
    if any(m>degree for m in innermults): #invalid
        if all(m == degree+1 for m in multvector):
            if not forceShape and weights is None:
                points=controlpoints[:]
                del points[degree+1::degree+1]
                return Draft.makeBezCurve(points,Degree=degree)
            else:
                poles=controlpoints[:]
                edges=[]
                while len(poles) >= degree+1:
                    #bezier segments
                    bzseg=Part.BezierCurve()
                    bzseg.increase(degree)
                    bzseg.setPoles(poles[0:degree+1])
                    poles=poles[degree+1:]
                    if weights is not None:
                        bzseg.setWeights(weights[0:degree+1])
                        weights=weights[degree+1:]
                    edges.append(bzseg.toShape())
                return Part.Wire(edges)
        else:
            warn('polygon fallback on %s' %spline)
            return drawSplineIterpolation(controlpoints,closed=closed,\
                forceShape=forceShape,alwaysDiscretize=True)
    try:
        bspline=Part.BSplineCurve()
        bspline.buildFromPolesMultsKnots(poles=controlpoints,mults=multvector,\
                knots=knotvector,degree=degree,periodic=periodic,\
                weights=weights)
        return bspline.toShape()
    except Part.OCCError:
        warn(spline)
    return None

def drawBlock(blockref,num=None,createObject=False):
    "returns a shape from a dxf block reference"
    if not dxfStarBlocks:
        if blockref.name[0] == '*':
            return None
    if len(blockref.entities.data) == 0:
        print("skipping empty block ",blockref.name)
        return None
    #print("creating block ", blockref.name, " containing ", len(blockref.entities.data), " entities")
    shapes = []
    for line in blockref.entities.get_type('line'):
        s = drawLine(line,forceShape=True)
        if s: shapes.append(s)
    for polyline in blockref.entities.get_type('polyline'):
        s = drawPolyline(polyline,forceShape=True)
        if s: shapes.append(s)
    for polyline in blockref.entities.get_type('lwpolyline'):
        s = drawPolyline(polyline,forceShape=True)
        if s: shapes.append(s)
    for arc in blockref.entities.get_type('arc'):
        s = drawArc(arc,forceShape=True)
        if s: shapes.append(s)
    for circle in blockref.entities.get_type('circle'):
        s = drawCircle(circle,forceShape=True)
        if s: shapes.append(s)
    for insert in blockref.entities.get_type('insert'):
        #print("insert ",insert," in block ",insert.block[0])
        if dxfStarBlocks or insert.block[0] != '*':
            s = drawInsert(insert)
            if s: shapes.append(s)
    for solid in blockref.entities.get_type('solid'):
        s = drawSolid(solid)
        if s: shapes.append(s)
    for spline in blockref.entities.get_type('spline'):
        s = drawSpline(spline,forceShape=True)
        if s: shapes.append(s)
    for text in blockref.entities.get_type('text'):
        if dxfImportTexts:
             if dxfImportLayouts or (not rawValue(text,67)):
                addText(text)
    for text in blockref.entities.get_type('mtext'):
        if dxfImportTexts:
             if dxfImportLayouts or (not rawValue(text,67)):
                print("adding block text",text.value, " from ",blockref)
                addText(text)
    try: shape = Part.makeCompound(shapes)
    except Part.OCCError: warn(blockref)
    if shape:
        blockshapes[blockref.name]=shape
        if createObject:
            newob=doc.addObject("Part::Feature",blockref.name)
            newob.Shape = shape
            blockobjects[blockref.name] = newob
            return newob
        return shape
    return None

def drawInsert(insert,num=None,clone=False):
    if dxfImportTexts:
        attrs = attribs(insert)
        for a in attrs:
            addText(a,attrib=True)
    if clone:
        if insert.block in blockobjects:
            newob = Draft.clone(blockobjects[insert.block])
            tsf = FreeCAD.Matrix()
            rot = math.radians(insert.rotation)
            pos = vec(insert.loc)
            tsf.move(pos)
            tsf.rotateZ(rot)
            sc = insert.scale
            sc = FreeCAD.Vector(sc[0],sc[1],0)
            newob.Placement = FreeCAD.Placement(tsf)
            newob.Scale = sc
            return newob
        else:
            shape = None
    else:
        if insert in blockshapes:
            shape = blockshapes[insert.block].copy()
        else:
            shape = None
            for b in drawing.blocks.data:
                if b.name == insert.block:
                    shape = drawBlock(b,num)
        if shape:
            pos = vec(insert.loc)
            rot = math.radians(insert.rotation)
            scale = insert.scale
            tsf = FreeCAD.Matrix()
            tsf.scale(scale[0],scale[1],0) # for some reason z must be 0 to work
            tsf.rotateZ(rot)
            shape = shape.transformGeometry(tsf)
            shape.translate(pos)
            return shape
    return None

def drawLayerBlock(objlist):
    "draws a Draft block with the given shapes or objects"
    obj = None
    if (dxfCreateDraft or dxfCreateSketch):
        try:
            obj = Draft.makeBlock(objlist)
        except Part.OCCError:
            pass
    else:
        try:
            obj = Part.makeCompound(objlist)
        except Part.OCCError:
            pass
    return obj

def attribs(insert):
    "checks if an insert has attributes, and returns the values if yes"
    atts = []
    if rawValue(insert,66) != 1: return []
    index = None
    for i in range(len(drawing.entities.data)):
        if drawing.entities.data[i] == insert:
            index = i
            break
    if index == None: return []
    j = index+1
    while True:
        ent = drawing.entities.data[j]
        if str(ent) == 'seqend':
            return atts
        elif str(ent) == 'attrib':
            atts.append(ent)
            j += 1

def addObject(shape,name="Shape",layer=None):
    "adds a new object to the document with passed arguments"
    if isinstance(shape,Part.Shape):
        newob=doc.addObject("Part::Feature",name)
        newob.Shape = shape
    else:
        newob = shape
    if layer:
        lay=locateLayer(layer)
        lay.addObject(newob)
    formatObject(newob)
    return newob

def addText(text,attrib=False):
    "adds a new text to the document"
    if attrib:
        lay = locateLayer(rawValue(text,8))
        val = rawValue(text,1)
        pos = FreeCAD.Vector(rawValue(text,10),rawValue(text,20),rawValue(text,30))
        hgt = rawValue(text,40)
    else:
        lay = locateLayer(text.layer)
        val = text.value
        pos = FreeCAD.Vector(text.loc[0],text.loc[1],text.loc[2])
        hgt = text.height
    if val:
        if attrib:
            newob = doc.addObject("App::Annotation","Attribute")
        else:
            newob = doc.addObject("App::Annotation","Text")
        lay.addObject(newob)
        val = deformat(val)
        # the following stores text as Latin1 in annotations, which
        # displays ok in coin texts, but causes errors later on.
        # better store as utf8 always.
        #try:
        #    val = val.decode("utf8").encode("Latin1")
        #except:
        #    try:
        #        val = val.encode("latin1")
        #    except:
        #        pass
        rx = rawValue(text,11)
        ry = rawValue(text,21)
        rz = rawValue(text,31)
        xv = Vector(1,0,0)
        ax = Vector(0,0,1)
        if rx or ry or rz:
            xv = Vector(rx,ry,rz)
            if not DraftVecUtils.isNull(xv):
                ax = (xv.cross(Vector(1,0,0))).negative()
                if DraftVecUtils.isNull(ax):
                    ax = Vector(0,0,1)
                ang = -math.degrees(DraftVecUtils.angle(xv,Vector(1,0,0),ax))
                Draft.rotate(newob,ang,axis=ax)
            if ax == Vector(0,0,-1): ax = Vector(0,0,1)
        elif hasattr(text,"rotation"):
            if text.rotation:
                Draft.rotate(newob,text.rotation)
        if attrib:
            attrot = rawValue(text,50)
            if attrot:
                Draft.rotate(newob,attrot)
        newob.LabelText = val.split("\n")
        if gui and draftui and dxfUseStandardSize:
            fsize = draftui.fontsize
        else:
            fsize = float(hgt)*TEXTSCALING
        if hasattr(text,"alignment"):
            yv = ax.cross(xv)
            if text.alignment in [1,2,3]:
                sup = DraftVecUtils.scaleTo(yv,fsize/TEXTSCALING).negative()
                #print(ax,sup)
                pos = pos.add(sup)
            elif text.alignment in [4,5,6]:
                sup = DraftVecUtils.scaleTo(yv,fsize/(2*TEXTSCALING)).negative()
                pos = pos.add(sup)
        newob.Position = pos
        if gui:
            newob.ViewObject.FontSize = fsize
            if hasattr(text,"alignment"):
                if text.alignment in [2,5,8]:
                    newob.ViewObject.Justification = "Center"
                elif text.alignment in [3,6,9]:
                    newob.ViewObject.Justification = "Right"
            newob.ViewObject.DisplayMode = "World"
            formatObject(newob,text)

def addToBlock(obj,layer):
    "adds given shape to the layer dict"
    if layer in layerBlocks:
        layerBlocks[layer].append(obj)
    else:
        layerBlocks[layer] = [obj]

def processdxf(document,filename,getShapes=False):
    "this does the translation of the dxf contents into FreeCAD Part objects"
    global drawing # for debugging - so drawing is still accessible to python after the script ran
    FreeCAD.Console.PrintMessage("opening "+filename+"...\n")
    drawing = dxfReader.readDXF(filename)
    global layers
    layers = []
    global doc
    doc = document
    global blockshapes
    blockshapes = {}
    global blockobjects
    blockobjects = {}
    global badobjects
    badobjects = []
    global layerBlocks
    layerBlocks = {}
    sketch = None
    shapes = []

    # drawing lines

    lines = drawing.entities.get_type("line")
    if lines: FreeCAD.Console.PrintMessage("drawing "+str(len(lines))+" lines...\n")
    for line in lines:
        if dxfImportLayouts or (not rawValue(line,67)):
            shape = drawLine(line)
            if shape:
                if dxfCreateSketch:
                    if dxfMakeBlocks or dxfJoin:
                        if sketch:
                            shape = Draft.makeSketch(shape,autoconstraints=True,addTo=sketch)
                        else:
                            shape = Draft.makeSketch(shape,autoconstraints=True)
                            sketch = shape
                    else:
                        shape = Draft.makeSketch(shape,autoconstraints=True)
                elif dxfJoin or getShapes:
                    if isinstance(shape,Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                elif dxfMakeBlocks:
                    addToBlock(shape,line.layer)
                else:
                    newob = addObject(shape,"Line",line.layer)
                    if gui: formatObject(newob,line)

    # drawing polylines

    pls = drawing.entities.get_type("lwpolyline")
    pls.extend(drawing.entities.get_type("polyline"))
    polylines = []
    meshes = []
    for p in pls:
        if hasattr(p,"flags"):
            if p.flags in [16,64]:
                meshes.append(p)
            else:
                polylines.append(p)
        else:
            polylines.append(p)
    if polylines:
        FreeCAD.Console.PrintMessage("drawing "+str(len(polylines))+" polylines...\n")
    num = 0
    for polyline in polylines:
        if dxfImportLayouts or (not rawValue(polyline,67)):
            shape = drawPolyline(polyline,num)
            if shape:
                if dxfCreateSketch:
                    if isinstance(shape,Part.Shape):
                        t = FreeCAD.ActiveDocument.addObject("Part::Feature","Shape")
                        t.Shape = shape
                        shape = t
                    if dxfMakeBlocks or dxfJoin:
                        if sketch:
                            shape = Draft.makeSketch(shape,autoconstraints=True,addTo=sketch)
                        else:
                            shape = Draft.makeSketch(shape,autoconstraints=True)
                            sketch = shape
                    else:
                        shape = Draft.makeSketch(shape,autoconstraints=True)
                elif dxfJoin or getShapes:
                    if isinstance(shape,Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                elif dxfMakeBlocks:
                    addToBlock(shape,polyline.layer)
                else:
                    newob = addObject(shape,"Polyline",polyline.layer)
                    if gui: formatObject(newob,polyline)
            num += 1

    # drawing arcs

    arcs = drawing.entities.get_type("arc")
    if arcs: FreeCAD.Console.PrintMessage("drawing "+str(len(arcs))+" arcs...\n")
    for arc in arcs:
        if dxfImportLayouts or (not rawValue(arc,67)):
            shape = drawArc(arc)
            if shape:
                if dxfCreateSketch:
                    if dxfMakeBlocks or dxfJoin:
                        if sketch:
                            shape = Draft.makeSketch(shape,autoconstraints=True,addTo=sketch)
                        else:
                            shape = Draft.makeSketch(shape,autoconstraints=True)
                            sketch = shape
                    else:
                        shape = Draft.makeSketch(shape,autoconstraints=True)
                elif dxfJoin or getShapes:
                    if isinstance(shape,Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                elif dxfMakeBlocks:
                    addToBlock(shape,arc.layer)
                else:
                    newob = addObject(shape,"Arc",arc.layer)
                    if gui: formatObject(newob,arc)

    # joining lines, polylines and arcs if needed

    if dxfJoin and shapes:
        FreeCAD.Console.PrintMessage("Joining geometry...\n")
        edges = []
        for s in shapes:
            edges.extend(s.Edges)
        if len(edges) > (100):
            FreeCAD.Console.PrintMessage(str(len(edges))+" edges to join\n")
            from PySide import QtGui
            d = QtGui.QMessageBox()
            d.setText("Warning: High number of entities to join (>100)")
            d.setInformativeText("This might take a long time or even freeze your computer. Are you sure? You can also disable the \"join geometry\" setting in DXF import preferences")
            d.setStandardButtons(QtGui.QMessageBox.Ok | QtGui.QMessageBox.Cancel)
            d.setDefaultButton(QtGui.QMessageBox.Cancel)
            res = d.exec_()
            if res == QtGui.QMessageBox.Cancel:
                FreeCAD.Console.PrintMessage("Aborted\n")
                return
        shapes = DraftGeomUtils.findWires(edges)
        for s in shapes:
            newob = addObject(s)

    # drawing circles

    circles = drawing.entities.get_type("circle")
    if circles: FreeCAD.Console.PrintMessage("drawing "+str(len(circles))+" circles...\n")
    for circle in circles:
        if dxfImportLayouts or (not rawValue(circle,67)):
            shape = drawCircle(circle)
            if shape:
                if dxfCreateSketch:
                    if dxfMakeBlocks or dxfJoin:
                        if sketch:
                            shape = Draft.makeSketch(shape,autoconstraints=True,addTo=sketch)
                        else:
                            shape = Draft.makeSketch(shape,autoconstraints=True)
                            sketch = shape
                    else:
                        shape = Draft.makeSketch(shape,autoconstraints=True)
                elif dxfMakeBlocks:
                    addToBlock(shape,circle.layer)
                elif getShapes:
                    if isinstance(shape,Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                else:
                    newob = addObject(shape,"Circle",circle.layer)
                    if gui: formatObject(newob,circle)

    # drawing solids

    solids = drawing.entities.get_type("solid")
    if solids: FreeCAD.Console.PrintMessage("drawing "+str(len(circles))+" solids...\n")
    for solid in solids:
        lay = rawValue(solid,8)
        if dxfImportLayouts or (not rawValue(solid,67)):
            shape = drawSolid(solid)
            if shape:
                if dxfMakeBlocks:
                    addToBlock(shape,lay)
                elif getShapes:
                    if isinstance(shape,Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                else:
                    newob = addObject(shape,"Solid",lay)
                    if gui: formatObject(newob,solid)

    # drawing splines

    splines = drawing.entities.get_type("spline")
    if splines: FreeCAD.Console.PrintMessage("drawing "+str(len(splines))+" splines...\n")
    for spline in splines:
        lay = rawValue(spline,8)
        if dxfImportLayouts or (not rawValue(spline,67)):
            shape = drawSpline(spline)
            if shape:
                if dxfMakeBlocks:
                    addToBlock(shape,lay)
                elif getShapes:
                    if isinstance(shape,Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                else:
                    newob = addObject(shape,"Spline",lay)
                    if gui: formatObject(newob,spline)

    # drawing ellipses

    ellipses = drawing.entities.get_type("ellipse")
    if ellipses: FreeCAD.Console.PrintMessage("drawing "+str(len(ellipses))+" ellipses...\n")
    for ellipse in ellipses:
        lay = rawValue(ellipse,8)
        if dxfImportLayouts or (not rawValue(ellipse,67)):
            shape = drawEllipse(ellipse)
            if shape:
                if dxfMakeBlocks:
                    addToBlock(shape,lay)
                elif getShapes:
                    if isinstance(shape,Part.Shape):
                        shapes.append(shape)
                    else:
                        shapes.append(shape.Shape)
                else:
                    newob = addObject(shape,"Ellipse",lay)
                    if gui: formatObject(newob,ellipse)

    # drawing texts

    if dxfImportTexts:
        texts = drawing.entities.get_type("mtext")
        texts.extend(drawing.entities.get_type("text"))
        if texts:
            FreeCAD.Console.PrintMessage("drawing "+str(len(texts))+" texts...\n")
        for text in texts:
            if dxfImportLayouts or (not rawValue(text,67)):
                addText(text)

    else: FreeCAD.Console.PrintMessage("skipping texts...\n")

    # drawing 3D objects

    faces3d = drawing.entities.get_type("3dface")
    if faces3d: FreeCAD.Console.PrintMessage("drawing "+str(len(faces3d))+" 3dfaces...\n")
    for face3d in faces3d:
        shape = drawFace(face3d)
        if shape:
            if getShapes:
                if isinstance(shape,Part.Shape):
                    shapes.append(shape)
                else:
                    shapes.append(shape.Shape)
            else:
                newob = addObject(shape,"Face",face3d.layer)
                if gui: formatObject(newob,face3d)
    if meshes: FreeCAD.Console.PrintMessage("drawing "+str(len(meshes))+" 3dmeshes...\n")
    for mesh in meshes:
        me = drawMesh(mesh)
        if me:
            newob = doc.addObject("Mesh::Feature","Mesh")
            lay = locateLayer(rawValue(mesh,8))
            lay.addObject(newob)
            newob.Mesh = me
            if gui: formatObject(newob,mesh)

    # end of shape-based objects, return if we are just getting shapes

    if getShapes and shapes:
        return(shapes)

    # drawing dims

    if dxfImportTexts:
        dims = drawing.entities.get_type("dimension")
        FreeCAD.Console.PrintMessage("drawing "+str(len(dims))+" dimensions...\n")
        for dim in dims:
            if dxfImportLayouts or (not rawValue(dim,67)):
                try:
                    layer = rawValue(dim,8)
                    x1 = float(rawValue(dim,10))
                    y1 = float(rawValue(dim,20))
                    z1 = float(rawValue(dim,30))
                    x2 = float(rawValue(dim,13))
                    y2 = float(rawValue(dim,23))
                    z2 = float(rawValue(dim,33))
                    x3 = float(rawValue(dim,14))
                    y3 = float(rawValue(dim,24))
                    z3 = float(rawValue(dim,34))
                    d = rawValue(dim,70)
                    if d: align = int(d)
                    else: align = 0
                    d = rawValue(dim,50)
                    if d: angle = float(d)
                    else: angle = 0
                except ValueError:
                    warn(dim)
                else:
                    lay=locateLayer(layer)
                    pt = FreeCAD.Vector(x1,y1,z1)
                    p1 = FreeCAD.Vector(x2,y2,z2)
                    p2 = FreeCAD.Vector(x3,y3,z3)
                    if align >= 128:
                        align -= 128
                    elif align >= 64:
                        align -= 64
                    elif align >= 32:
                        align -= 32
                    if align == 0:
                        if angle in [0,180]:
                            p2 = FreeCAD.Vector(x3,y2,z2)
                        elif angle in [90,270]:
                            p2 = FreeCAD.Vector(x2,y3,z2)
                    newob = doc.addObject("App::FeaturePython","Dimension")
                    lay.addObject(newob)
                    _Dimension(newob)
                    _ViewProviderDimension(newob.ViewObject)
                    newob.Start = p1
                    newob.End = p2
                    newob.Dimline = pt
                    if gui:
                        dim.layer = layer
                        dim.color_index = 256
                        formatObject (newob,dim)
                        if dxfUseStandardSize and draftui:
                            newob.ViewObject.FontSize = draftui.fontsize
                        else:
                            st = rawValue(dim,3)
                            newob.ViewObject.FontSize = float(getdimheight(st))*TEXTSCALING
    else:
        FreeCAD.Console.PrintMessage("skipping dimensions...\n")

    # drawing points

    if dxfImportPoints:
        points = drawing.entities.get_type("point")
        if points: FreeCAD.Console.PrintMessage("drawing "+str(len(points))+" points...\n")
        for point in points:
                x = rawValue(point,10)
                y = rawValue(point,20)
                z = rawValue(point,30)
                lay = rawValue(point,8)
                if dxfImportLayouts or (not rawValue(point,67)):
                    if dxfMakeBlocks:
                        shape = Part.Vertex(x,y,z)
                        addToBlock(shape,lay)
                    else:
                        newob = Draft.makePoint(x,y,z)
                        lay = locateLayer(lay)
                        lay.addObject(newob)
                        if gui:
                            formatObject(newob,point)
    else:
        FreeCAD.Console.PrintMessage("skipping points...\n")

    # drawing leaders

    if dxfImportTexts:
        leaders = drawing.entities.get_type("leader")
        if leaders:
            FreeCAD.Console.PrintMessage("drawing "+str(len(leaders))+" leaders...\n")
        for leader in leaders:
            if dxfImportLayouts or (not rawValue(leader,67)):
                points = getMultiplePoints(leader)
                newob = Draft.makeWire(points)
                lay = locateLayer(rawValue(leader,8))
                lay.addObject(newob)
                if gui:
                    newob.ViewObject.EndArrow = True
                    formatObject(newob,leader)
    else:
        FreeCAD.Console.PrintMessage("skipping leaders...\n")

    # drawing hatches

    if dxfImportHatches:
        hatches = drawing.entities.get_type("hatch")
        if hatches:
            FreeCAD.Console.PrintMessage("drawing "+str(len(hatches))+" hatches...\n")
        for hatch in hatches:
            if dxfImportLayouts or (not rawValue(hatch,67)):
                points = getMultiplePoints(hatch)
                if len(points) > 1:
                    lay = rawValue(hatch,8)
                    points = points[:-1]
                    newob = None
                    if dxfCreatePart or dxfMakeBlocks:
                        points.append(points[0])
                        s = Part.makePolygon(points)
                        if dxfMakeBlocks:
                            addToBlock(s,lay)
                        else:
                            newob = addObject(s,"Hatch",lay)
                            if gui:
                                formatObject(newob,hatch)
                    else:
                        newob = Draft.makeWire(points)
                        locateLayer(lay).addObject(newob)
                        if gui:
                            formatObject(newob,hatch)
    else:
        FreeCAD.Console.PrintMessage("skipping hatches...\n")

    # drawing blocks

    inserts = drawing.entities.get_type("insert")
    if not dxfStarBlocks:
        FreeCAD.Console.PrintMessage("skipping *blocks...\n")
        newinserts = []
        for i in inserts:
            if dxfImportLayouts or (not rawValue(i,67)):
                if i.block[0] != '*':
                    newinserts.append(i)
        inserts = newinserts
    if inserts:
        FreeCAD.Console.PrintMessage("drawing "+str(len(inserts))+" blocks...\n")
        blockrefs = drawing.blocks.data
        for ref in blockrefs:
            if dxfCreateDraft or dxfCreateSketch:
                drawBlock(ref,createObject=True)
            else:
                drawBlock(ref,createObject=False)
        num = 0
        for insert in inserts:
            if (dxfCreateDraft or dxfCreateSketch) and not(dxfMakeBlocks):
                shape = drawInsert(insert,num,clone=True)
            else:
                shape = drawInsert(insert,num)
            if shape:
                if dxfMakeBlocks:
                    addToBlock(shape,insert.layer)
                else:
                    newob = addObject(shape,"Block."+insert.block,insert.layer)
                    if gui: formatObject(newob,insert)
            num += 1

    # make blocks, if any

    if dxfMakeBlocks:
        print("creating layerblocks...")
        for k,l in layerBlocks.items():
            shape = drawLayerBlock(l)
            if shape:
                newob = addObject(shape,k)
    del layerBlocks

    # hide block objects, if any

    for k,o in blockobjects.items():
        if o.ViewObject:
            o.ViewObject.hide()
    del blockobjects

    # finishing

    print("done processing")

    doc.recompute()
    FreeCAD.Console.PrintMessage("successfully imported "+filename+"\n")
    if badobjects:
        print("dxf: ",len(badobjects)," objects were not imported")
    del doc
    del blockshapes

def warn(dxfobject,num=None):
    "outputs a warning if a dxf object couldn't be imported"
    print("dxf: couldn't import ", dxfobject, " (",num,")")
    badobjects.append(dxfobject)

def open(filename):
    "called when freecad opens a file."
    readPreferences()
    if dxfReader:
        docname = os.path.splitext(os.path.basename(filename))[0]
        doc = FreeCAD.newDocument(docname)
        doc.Label = decodeName(docname)
        processdxf(doc,filename)
        return doc
    else:
        errorDXFLib(gui)

def insert(filename,docname):
    "called when freecad imports a file"
    readPreferences()
    if dxfReader:
        groupname = os.path.splitext(os.path.basename(filename))[0]
        try:
            doc=FreeCAD.getDocument(docname)
        except NameError:
            doc=FreeCAD.newDocument(docname)
        FreeCAD.setActiveDocument(docname)
        importgroup = doc.addObject("App::DocumentObjectGroup",groupname)
        importgroup.Label = decodeName(groupname)
        processdxf(doc,filename)
        for l in layers:
            importgroup.addObject(l)
    else:
        errorDXFLib(gui)

def getShapes(filename):
    "reads a dxf file and returns a list of shapes from its contents"
    if dxfReader:
        return processdxf(None,filename,getShapes=True)


# EXPORT ########################################################################

def projectShape(shape,direction):
    import Drawing
    edges = []
    try:
        groups = Drawing.projectEx(shape,direction)
    except Part.OCCError:
        print("unable to project shape on direction ",direction)
        return shape
    else:
        for g in groups[0:5]:
            if g:
                edges.append(g)
        return DraftGeomUtils.cleanProjection(Part.makeCompound(edges))

def getArcData(edge):
    "returns center, radius, start and end angles of a circle-based edge"
    ce = edge.Curve.Center
    radius = edge.Curve.Radius
    if len(edge.Vertexes) == 1:
        # closed circle
        return DraftVecUtils.tup(ce), radius, 0, 0
    else:
        # new method: recalculate ourselves - cannot trust edge.Curve.Axis or XAxis
        p1 = edge.Vertexes[0].Point
        p2 = edge.Vertexes[-1].Point
        v1 = p1.sub(ce)
        v2 = p2.sub(ce)
        #print v1.cross(v2)
        #print edge.Curve.Axis
        #print p1
        #print p2
        # we can use Z check since arcs getting here will ALWAYS be in XY plane
        # Z can be 0 if the arc is 180 deg
        if (v1.cross(v2).z >= 0) or (edge.Curve.Axis.z > 0):
            #clockwise
            ang1 = -DraftVecUtils.angle(v1)
            ang2 = -DraftVecUtils.angle(v2)
        else:
            #counterclockwise
            ang2 = -DraftVecUtils.angle(v1)
            ang1 = -DraftVecUtils.angle(v2)

        # obsolete method - fails a lot
        #if round(edge.Curve.Axis.dot(FreeCAD.Vector(0,0,1))) == 1:
        #    ang1,ang2=edge.ParameterRange
        #else:
        #    ang2,ang1=edge.ParameterRange
        #if edge.Curve.XAxis != FreeCAD.Vector(1,0,0):
        #    ang1 -= DraftVecUtils.angle(edge.Curve.XAxis)
        #    ang2 -= DraftVecUtils.angle(edge.Curve.XAxis)

        return DraftVecUtils.tup(ce), radius, math.degrees(ang1),\
                math.degrees(ang2)

def getSplineSegs(edge):
    "returns an array of vectors from a Spline or Bezier edge"
    params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    seglength = params.GetFloat("maxsegmentlength",5.0)
    points = []
    if seglength == 0:
        points.append(edge.Vertexes[0].Point)
        points.append(edge.Vertexes[-1].Point)
    else:
        points.append(edge.valueAt(edge.FirstParameter))
        if (edge.Length > seglength):
            nbsegs = int(math.ceil(edge.Length/seglength))
            step = (edge.LastParameter-edge.FirstParameter)/nbsegs
            for nv in range(1,nbsegs):
                #print("value at",nv*step,"=",edge.valueAt(nv*step))
                v = edge.valueAt(edge.FirstParameter+(nv*step))
                points.append(v)
        points.append(edge.valueAt(edge.LastParameter))
    return points

def getWire(wire,nospline=False,lw=True):
    "returns an array of dxf-ready points and bulges from a wire"
    def fmt(v,b=0.0):
        if lw:
            # LWpolyline format
            return (v.x,v.y,v.z,None,None,b)
        else:
            # Polyline format
            return ((v.x,v.y,v.z),None,[None,None],b)
    edges = DraftGeomUtils.sortEdges(wire.Edges)
    points = []
    # print("processing wire ",wire.Edges)
    for edge in edges:
        v1 = edge.Vertexes[0].Point
        if DraftGeomUtils.geomType(edge) == "Circle":
            # polyline bulge -> negative makes the arc go clockwise
            angle = edge.LastParameter-edge.FirstParameter
            bul = math.tan(angle/4)
            #if cross1[2] < 0:
                # polyline bulge -> negative makes the arc go clockwise
                #bul = -bul
            if edge.Curve.Axis.dot(FreeCAD.Vector(0,0,1)) < 0:
                bul = -bul
            points.append(fmt(v1,bul))
        elif (DraftGeomUtils.geomType(edge) in ["BSplineCurve","BezierCurve","Ellipse"]) and (not nospline):
            spline = getSplineSegs(edge)
            spline.pop()
            for p in spline:
                points.append(fmt(p))
        else:
            points.append(fmt(v1))
    if not DraftGeomUtils.isReallyClosed(wire):
        v = edges[-1].Vertexes[-1].Point
        points.append(fmt(v))
    # print("wire verts: ",points)
    return points

def getBlock(sh,obj,lwPoly=False):
    "returns a dxf block with the contents of the object"
    block = dxfLibrary.Block(name=obj.Name,layer=getGroup(obj))
    writeShape(sh,obj,block,lwPoly)
    return block

def writeShape(sh,ob,dxfobject,nospline=False,lwPoly=False):
    "writes the object's shape contents in the given dxf object"
    processededges = []
    for wire in sh.Wires: # polylines
        for e in wire.Edges:
            processededges.append(e.hashCode())
        if (len(wire.Edges) == 1) and (DraftGeomUtils.geomType(wire.Edges[0]) == "Circle"):
            center, radius, ang1, ang2 = getArcData(wire.Edges[0])
            if center != None:
                if len(wire.Edges[0].Vertexes) == 1: # circle
                    dxfobject.append(dxfLibrary.Circle(center, radius,
                                                       color=getACI(ob),
                                                       layer=getGroup(ob)))
                else: # arc
                    dxfobject.append(dxfLibrary.Arc(center, radius,
                                                    ang1, ang2, color=getACI(ob),
                                                    layer=getGroup(ob)))
        else:
            if (lwPoly):
                if hasattr(dxfLibrary,"LwPolyLine"):
                    dxfobject.append(dxfLibrary.LwPolyLine(getWire(wire,nospline), [0.0,0.0],
                                                           int(DraftGeomUtils.isReallyClosed(wire)), color=getACI(ob),
                                                           layer=getGroup(ob)))
                else:
                    FreeCAD.Console.PrintWarning("LwPolyLine support not found. Please delete dxfLibrary.py from your FreeCAD user directory to force auto-update\n")
            else :
                dxfobject.append(dxfLibrary.PolyLine(getWire(wire,nospline,lw=False), [0.0,0.0,0.0],
                                                     int(DraftGeomUtils.isReallyClosed(wire)), color=getACI(ob),
                                                     layer=getGroup(ob)))
    if len(processededges) < len(sh.Edges): # lone edges
        loneedges = []
        for e in sh.Edges:
            if not(e.hashCode() in processededges): loneedges.append(e)
        # print("lone edges ",loneedges)
        for edge in loneedges:
            if (DraftGeomUtils.geomType(edge) in ["BSplineCurve","BezierCurve"]): # splines
                if (len(edge.Vertexes) == 1) and (edge.Curve.isClosed()) and (edge.Area > 0):
                    # special case: 1-vert closed spline, approximate as a circle
                    c = DraftGeomUtils.getCircleFromSpline(edge)
                    if c:
                        dxfobject.append(dxfLibrary.Circle(DraftVecUtils.tup(c.Curve.Center), c.Curve.Radius,
                                                           color=getACI(ob),
                                                           layer=getGroup(ob)))
                else:
                    points = []
                    spline = getSplineSegs(edge)
                    for p in spline:
                        points.append(((p.x,p.y,p.z),None,[None,None],0.0))
                    dxfobject.append(dxfLibrary.PolyLine(points, [0.0,0.0,0.0],
                                                         0, color=getACI(ob),
                                                         layer=getGroup(ob)))
            elif DraftGeomUtils.geomType(edge) == "Circle": # curves
                center, radius, ang1, ang2 = getArcData(edge)
                if center != None:
                    if not isinstance(center,tuple):
                        center = DraftVecUtils.tup(center)
                    if len(edge.Vertexes) == 1: # circles
                        dxfobject.append(dxfLibrary.Circle(center, radius,
                                                           color=getACI(ob),
                                                           layer=getGroup(ob)))
                    else : # arcs
                        dxfobject.append(dxfLibrary.Arc(center, radius,
                                                        ang1, ang2, color=getACI(ob),
                                                        layer=getGroup(ob)))
            elif DraftGeomUtils.geomType(edge) == "Ellipse": # ellipses:
                if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetBool("DiscretizeEllipses",True):
                    points = []
                    spline = getSplineSegs(edge)
                    for p in spline:
                        points.append(((p.x,p.y,p.z),None,[None,None],0.0))
                    dxfobject.append(dxfLibrary.PolyLine(points, [0.0,0.0,0.0],
                                                         0, color=getACI(ob),
                                                         layer=getGroup(ob)))
                else:
                    if hasattr(dxfLibrary,"Ellipse"):
                        center = DraftVecUtils.tup(edge.Curve.Center)
                        norm = DraftVecUtils.tup(edge.Curve.Axis)
                        start = edge.FirstParameter
                        end = edge.LastParameter
                        ax = edge.Curve.Focus1.sub(edge.Curve.Center)
                        major = DraftVecUtils.tup(DraftVecUtils.scaleTo(ax,edge.Curve.MajorRadius))
                        minor = edge.Curve.MinorRadius/edge.Curve.MajorRadius
                        # print("exporting ellipse: ",center,norm,start,end,major,minor)
                        dxfobject.append(dxfLibrary.Ellipse(center=center,majorAxis=major,normalAxis=norm,
                                                            minorAxisRatio=minor,startParameter=start,
                                                            endParameter=end,
                                                            color=getACI(ob),
                                                            layer=getGroup(ob)))
                    else:
                        FreeCAD.Console.PrintWarning("Ellipses support not found. Please delete dxfLibrary.py from your FreeCAD user directory to force auto-update\n")
            else: # anything else is treated as lines
                if len(edge.Vertexes) > 1:
                    ve1=edge.Vertexes[0].Point
                    ve2=edge.Vertexes[1].Point
                    dxfobject.append(dxfLibrary.Line([DraftVecUtils.tup(ve1), DraftVecUtils.tup(ve2)],
                                                     color=getACI(ob),
                                                     layer=getGroup(ob)))

def writeMesh(ob,dxfobject):
    "export a shape as a polyface mesh"
    meshdata = ob.Shape.tessellate(0.5)
    # print(meshdata)
    points = []
    faces = []
    for p in meshdata[0]:
        points.append([p.x,p.y,p.z])
    for f in meshdata[1]:
        faces.append([f[0]+1,f[1]+1,f[2]+1])
    # print(len(points),len(faces))
    dxfobject.append(dxfLibrary.PolyLine([points,faces], [0.0,0.0,0.0],
                                         64, color=getACI(ob),
                                         layer=getGroup(ob)))

def export(objectslist,filename,nospline=False,lwPoly=False):
    "called when freecad exports a file. If nospline=True, bsplines are exported as straight segs lwPoly=True for OpenSCAD DXF"
    readPreferences()
    if dxfLibrary:
        global exportList
        exportList = objectslist
        exportList = Draft.getGroupContents(exportList)

        if (len(exportList) == 1) and (Draft.getType(exportList[0]) == "ArchSectionView"):
            # arch view: export it "as is"
            dxf = exportList[0].Proxy.getDXF()
            if dxf:
                f = open(filename,"w")
                f.write(dxf)
                f.close()

        elif (len(exportList) == 1) and (exportList[0].isDerivedFrom("Drawing::FeaturePage")):
            # page: special hack-export! (see below)
            exportPage(exportList[0],filename)

        else:
            # other cases, treat edges
            dxf = dxfLibrary.Drawing()
            for ob in exportList:
                print("processing "+str(ob.Name))
                if ob.isDerivedFrom("Part::Feature"):
                    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetBool("dxfmesh"):
                        sh = None
                        if not ob.Shape.isNull():
                            writeMesh(ob,dxf)
                    elif gui and FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetBool("dxfproject"):
                        direction = FreeCADGui.ActiveDocument.ActiveView.\
                                getViewDirection().multiply(-1)
                        sh = projectShape(ob.Shape,direction)
                    else:
                        if ob.Shape.Volume > 0:
                            sh = projectShape(ob.Shape,Vector(0,0,1))
                        else:
                            sh = ob.Shape
                    if sh:
                        if not sh.isNull():
                            if sh.ShapeType == 'Compound':
                                if (len(sh.Wires) == 1):
                                    # only one wire in this compound, no lone edge -> polyline
                                    if (len(sh.Wires[0].Edges) == len(sh.Edges)):
                                        writeShape(sh,ob,dxf,nospline,lwPoly)
                                    else:
                                        # 1 wire + lone edges -> block
                                        block = getBlock(sh,ob,lwPoly)
                                        dxf.blocks.append(block)
                                        dxf.append(dxfLibrary.Insert(name=ob.Name.upper()))
                                else:
                                    # all other cases: block
                                    block = getBlock(sh,ob,lwPoly)
                                    dxf.blocks.append(block)
                                    dxf.append(dxfLibrary.Insert(name=ob.Name.upper()))
                            else:
                                writeShape(sh,ob,dxf,nospline,lwPoly)

                elif Draft.getType(ob) == "Annotation":
                    # texts

                    # temporary - as dxfLibrary doesn't support mtexts well, we use several single-line texts
                    # well, anyway, at the moment, Draft only writes single-line texts, so...
                    for text in ob.LabelText:
                        point = DraftVecUtils.tup(FreeCAD.Vector(ob.Position.x,
                                                         ob.Position.y-ob.LabelText.index(text),
                                                         ob.Position.z))
                        if gui: height = float(ob.ViewObject.FontSize)
                        else: height = 1
                        dxf.append(dxfLibrary.Text(text,point,height=height,
                                                   color=getACI(ob,text=True),
                                                   style='STANDARD',
                                                   layer=getGroup(ob)))

                elif Draft.getType(ob) == "Dimension":
                    p1 = DraftVecUtils.tup(ob.Start)
                    p2 = DraftVecUtils.tup(ob.End)
                    base = Part.Line(ob.Start,ob.End).toShape()
                    proj = DraftGeomUtils.findDistance(ob.Dimline,base)
                    if not proj:
                        pbase = DraftVecUtils.tup(ob.End)
                    else:
                        pbase = DraftVecUtils.tup(ob.End.add(proj.negative()))
                    dxf.append(dxfLibrary.Dimension(pbase,p1,p2,color=getACI(ob),
                                                    layer=getGroup(ob)))

            dxf.saveas(filename)
        FreeCAD.Console.PrintMessage("successfully exported "+filename+"\r\n")
    else:
        errorDXFLib(gui)

def exportPage(page,filename):
    "special export for pages"
    template = os.path.splitext(page.Template)[0]+".dxf"
    global dxfhandle
    dxfhandle = 1
    if os.path.exists(template):
        f = pythonopen(template,"rb")
        template = f.read()
        f.close()
        # find & replace editable texts
        import re
        f = pythonopen(page.Template,"rb")
        svgtemplate = f.read()
        f.close()
        editables = re.findall("freecad:editable=\"(.*?)\"",svgtemplate)
        values = page.EditableTexts
        for i in range(len(editables)):
            if len(values) > i:
                template = template.replace(editables[i],values[i])
    else:
        # dummy default template
        print("DXF version of the template not found. Creating a default empty template.")
        template = "999\nFreeCAD DXF exporter v"+FreeCAD.Version()[0]+"."+FreeCAD.Version()[1]+"-"+FreeCAD.Version()[2]+"\n"
        template += "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n"
        template += "0\nSECTION\n2\nBLOCKS\n$blocks\n0\nENDSEC\n"
        template += "0\nSECTION\n2\nENTITIES\n$entities\n0\nENDSEC\n"
        template += "0\nEOF"
    blocks = ""
    entities = ""
    for view in page.Group:
        b,e = getViewDXF(view)
        blocks += b
        entities += e
    result = template.replace("$blocks",blocks[:-1])
    result = result.replace("$entities",entities[:-1])
    f = pythonopen(filename,"wb")
    f.write(result)
    f.close()


def getViewDXF(view):
    "returns a DXF fragment from a Drawing View"
    global dxfhandle
    block = ""
    insert = ""

    if view.isDerivedFrom("App::DocumentObjectGroup"):
        for child in view.Group:
            b,e = getViewDXF(child)
            block += b
            insert += e

    elif view.isDerivedFrom("Drawing::FeatureViewPython"):
        if hasattr(view.Proxy,"getDXF"):
            r = view.Rotation
            if r != 0: r = -r # fix rotation direction
            count = 0
            block = ""
            insert = ""
            geom = view.Proxy.getDXF(view)
            if not isinstance(geom,list): geom = [geom]
            for g in geom: # getDXF returns a list of entities
                g = g.replace("sheet_layer\n","0\n6\nBYBLOCK\n62\n0\n") # change layer and set color and ltype to BYBLOCK (0)
                block += "0\nBLOCK\n8\n0\n2\n"+view.Name+str(count)+"\n70\n0\n10\n0\n20\n0\n3\n"+view.Name+str(count)+"\n1\n\n"
                block += g
                block += "0\nENDBLK\n8\n0\n"
                insert += "0\nINSERT\n5\naaaa"+hex(dxfhandle)[2:]+"\n8\n0\n6\nBYLAYER\n62\n256\n2\n"+view.Name+str(count)
                insert += "\n10\n"+str(view.X)+"\n20\n"+str(-view.Y)
                insert += "\n30\n0\n41\n"+str(view.Scale)+"\n42\n"+str(view.Scale)+"\n43\n"+str(view.Scale)
                insert += "\n50\n"+str(r)+"\n"
                dxfhandle += 1
                count += 1

    elif view.isDerivedFrom("Drawing::FeatureViewPart"):
        r = view.Rotation
        if r != 0: r = -r # fix rotation direction
        import Drawing
        proj = Drawing.projectToDXF(view.Source.Shape,view.Direction)
        proj = proj.replace("sheet_layer\n","0\n6\nBYBLOCK\n62\n0\n") # change layer and set color and ltype to BYBLOCK (0)
        block = "0\nBLOCK\n8\n0\n2\n"+view.Name+"\n70\n0\n10\n0\n20\n0\n3\n"+view.Name+"\n1\n\n"
        block += proj
        block += "0\nENDBLK\n8\n0\n"
        insert = "0\nINSERT\n5\naaaa"+hex(dxfhandle)[2:]+"\n8\n0\n6\nBYLAYER\n62\n256\n2\n"+view.Name
        insert += "\n10\n"+str(view.X)+"\n20\n"+str(-view.Y)
        insert += "\n30\n0\n41\n"+str(view.Scale)+"\n42\n"+str(view.Scale)+"\n43\n"+str(view.Scale)
        insert += "\n50\n"+str(r)+"\n"
        dxfhandle += 1

    elif view.isDerivedFrom("Drawing::FeatureViewAnnotation"):
        r = view.Rotation
        if r != 0: r = -r # fix rotation direction
        insert ="0\nTEXT\n5\n"+hex(dxfhandle)[2:]+"\n8\n0"
        insert += "\n10\n"+str(view.X)+"\n20\n"+str(-view.Y)
        insert += "\n30\n0\n40\n"+str(view.Scale/2)
        insert += "\n50\n"+str(r)
        insert += "\n1\n"+view.Text[0]+"\n"
        dxfhandle += 1

    else:
        print("Unable to get DXF representation from view: ",view.Label)
    return block,insert


def exportPageLegacy(page,filename):
    "exports the given page the old way, by converting its SVG code to DXF with the Draft module"
    import importSVG
    tempdoc = importSVG.open(page.PageResult)
    tempobj = tempdoc.Objects
    export(tempobj,filename,nospline=True,lwPoly=False)
    FreeCAD.closeDocument(tempdoc.Name)

def readPreferences():
    # reading parameters
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    global dxfCreatePart, dxfCreateDraft, dxfCreateSketch, dxfDiscretizeCurves, dxfStarBlocks
    global dxfMakeBlocks, dxfJoin, dxfRenderPolylineWidth, dxfImportTexts, dxfImportLayouts
    global dxfImportPoints, dxfImportHatches, dxfUseStandardSize, dxfGetColors, dxfUseDraftVisGroups
    global dxfFillMode, dxfBrightBackground, dxfDefaultColor
    dxfCreatePart = p.GetBool("dxfCreatePart",True)
    dxfCreateDraft = p.GetBool("dxfCreateDraft",False)
    dxfCreateSketch = p.GetBool("dxfCreateSketch",False)
    dxfDiscretizeCurves = p.GetBool("DiscretizeEllipses",True)
    dxfStarBlocks = p.GetBool("dxfstarblocks",False)
    dxfMakeBlocks = p.GetBool("groupLayers",False)
    dxfJoin = p.GetBool("joingeometry",False)
    dxfRenderPolylineWidth = p.GetBool("renderPolylineWidth",False)
    dxfImportTexts = p.GetBool("dxftext",False)
    dxfImportLayouts = p.GetBool("dxflayouts",False)
    dxfImportPoints = p.GetBool("dxfImportPoints",False)
    dxfImportHatches = p.GetBool("importDxfHatches",False)
    dxfUseStandardSize = p.GetBool("dxfStdSize",False)
    dxfGetColors = p.GetBool("dxfGetOriginalColors",False)
    dxfUseDraftVisGroups = p.GetBool("dxfUseDraftVisGroups",False)
    dxfFillMode = p.GetBool("fillmode",True)
    dxfBrightBackground = isBrightBackground()
    dxfDefaultColor = getColor()
