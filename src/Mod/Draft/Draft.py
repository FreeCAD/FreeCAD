#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009, 2010                                              *  
#*   Yorik van Havre <yorik@uncreated.net>, Ken Cline <cline@frii.com>     *  
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

from __future__ import division

__title__="FreeCAD Draft Workbench"
__author__ = "Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, Dmitry Chigrin, Daniel Falck"
__url__ = "http://free-cad.sourceforge.net"

'''
General description:

    The Draft module is a FreeCAD module for drawing/editing 2D entities.
    The aim is to give FreeCAD basic 2D-CAD capabilities (similar
    to Autocad and other similar software). This modules is made to be run
    inside FreeCAD and needs the PyQt4 and pivy modules available.

User manual:

    http://sourceforge.net/apps/mediawiki/free-cad/index.php?title=2d_Drafting_Module

How it works / how to extend:

    This module is written entirely in python. If you know a bit of python
    language, you are welcome to modify this module or to help us to improve it.
    Suggestions are also welcome on the FreeCAD discussion forum.
    
    If you want to have a look at the code, here is a general explanation. The
    Draft module is divided in several files:

    - Draft.py: Hosts the functions that are useful for scripting outside of
    the Draft module, it is the "Draft API"
    - DraftGui.py: Creates and manages the special Draft toolbar
    - DraftTools.py: Contains the user tools of the Draft module (the commands
    from the Draft menu), and a couple of helpers such as the "Trackers"
    (temporary geometry used while drawing)
    - DraftVecUtils.py: a vector math library, contains functions that are not
    implemented in the standard FreeCAD vector
    - DraftGeomUtils.py: a library of misc functions to manipulate shapes.
        
    The Draft.py contains everything to create geometry in the scene. You
    should start there if you intend to modify something. Then, the DraftTools
    are where the FreeCAD commands are defined, while in DraftGui.py
    you have the ui part, ie. the draft command bar. Both DraftTools and
    DraftGui are loaded at module init by InitGui.py, which is called directly by FreeCAD.
    The tools all have an Activated() function, which is called by FreeCAD when the
    corresponding FreeCAD command is invoked. Most tools then create the trackers they
    will need during operation, then place a callback mechanism, which will detect
    user input and do the necessary cad operations. They also send commands to the
    command bar, which will display the appropriate controls. While the scene event
    callback watches mouse events, the keyboard is being watched by the command bar.
'''

# import FreeCAD modules
import FreeCAD, math, sys, os, DraftVecUtils
from FreeCAD import Vector
from pivy import coin

if FreeCAD.GuiUp:
    import FreeCADGui, WorkingPlane
    gui = True
else:
    print "FreeCAD Gui not present. Draft module will have some features disabled."
    gui = False

#---------------------------------------------------------------------------
# General functions
#---------------------------------------------------------------------------

def typecheck (args_and_types, name="?"):
    "typecheck([arg1,type),(arg2,type),...]): checks arguments types"
    for v,t in args_and_types:
        if not isinstance (v,t):
            w = "typecheck[" + str(name) + "]: "
            w += str(v) + " is not " + str(t) + "\n"
            FreeCAD.Console.PrintWarning(w)
            raise TypeError("Draft." + str(name))

def getParamType(param):
    if param in ["dimsymbol","dimPrecision","dimorientation","precision","defaultWP",
                 "snapRange","gridEvery","linewidth","UiMode","modconstrain","modsnap",
                 "modalt"]:
        return "int"
    elif param in ["constructiongroupname","textfont","patternFile","template","maxSnapEdges",
                   "snapModes"]:
        return "string"
    elif param in ["textheight","tolerance","gridSpacing"]:
        return "float"
    elif param in ["selectBaseObjects","alwaysSnap","grid","fillmode","saveonexit","maxSnap",
                   "SvgLinesBlack","dxfStdSize","showSnapBar","hideSnapBar","alwaysShowGrid",
                   "renderPolylineWidth","showPlaneTracker"]:
        return "bool"
    elif param in ["color","constructioncolor","snapcolor"]:
        return "unsigned"
    else:
        return None

def getParam(param):
    "getParam(parameterName): returns a Draft parameter value from the current config"
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    t = getParamType(param)
    if t == "int": return p.GetInt(param)
    elif t == "string": return p.GetString(param)
    elif t == "float": return p.GetFloat(param)
    elif t == "bool": return p.GetBool(param)
    elif t == "unsigned": return p.GetUnsigned(param)
    else: return None

def setParam(param,value):
    "setParam(parameterName,value): sets a Draft parameter with the given value"
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    t = getParamType(param)
    if t == "int": p.SetInt(param,value)
    elif t == "string": p.SetString(param,value)
    elif t == "float": p.SetFloat(param,value)
    elif t == "bool": p.SetBool(param,value)
    elif t == "unsigned": p.SetUnsigned(param,value)
              
def precision():
    "precision(): returns the precision value from Draft user settings"
    return getParam("precision")

def tolerance():
    "tolerance(): returns the tolerance value from Draft user settings"
    return getParam("tolerance")
        
def getRealName(name):
    "getRealName(string): strips the trailing numbers from a string name"
    for i in range(1,len(name)):
        if not name[-i] in '1234567890':
            return name[:len(name)-(i-1)]
    return name

def getType(obj):
    "getType(object): returns the Draft type of the given object"
    import Part
    if isinstance(obj,Part.Shape):
        return "Shape"
    if "Proxy" in obj.PropertiesList:
        if hasattr(obj.Proxy,"Type"):
            return obj.Proxy.Type
    if obj.isDerivedFrom("Sketcher::SketchObject"):
        return "Sketch"
    if obj.isDerivedFrom("Part::Feature"):
        return "Part"
    if (obj.Type == "App::Annotation"):
        return "Annotation"
    if obj.isDerivedFrom("Mesh::Feature"):
        return "Mesh"
    if obj.isDerivedFrom("Points::Feature"):
        return "Points"
    if (obj.Type == "App::DocumentObjectGroup"):
        return "Group"
    return "Unknown"

def get3DView():
    "get3DView(): returns the current view if it is 3D, or the first 3D view found, or None"
    v = FreeCADGui.ActiveDocument.ActiveView
    if str(type(v)) == "<type 'View3DInventorPy'>":
        return v
    v = FreeCADGui.ActiveDocument.mdiViewsOfType("Gui::View3DInventor")
    if v:
        return v[0]
    return None

def isClone(obj,objtype):
    """isClone(obj,objtype): returns True if the given object is 
    a clone of an object of the given type"""
    if getType(obj) == "Clone":
        if len(obj.Objects) == 1:
            if getType(obj.Objects[0]) == objtype:
                return True
    return False

def getGroupNames():
    "returns a list of existing groups in the document"
    glist = []
    doc = FreeCAD.ActiveDocument
    for obj in doc.Objects:
        if obj.Type == "App::DocumentObjectGroup":
            glist.append(obj.Name)
    return glist

def ungroup(obj):
    "removes the current object from any group it belongs to"
    for g in getGroupNames():
        grp = FreeCAD.ActiveDocument.getObject(g)
        if grp.hasObject(obj):
            grp.removeObject(obj)
      
def dimSymbol():
    "returns the current dim symbol from the preferences as a pivy SoMarkerSet"
    s = getParam("dimsymbol")
    marker = coin.SoMarkerSet()
    if s == 0: marker.markerIndex = coin.SoMarkerSet.CIRCLE_FILLED_5_5
    elif s == 1: marker.markerIndex = coin.SoMarkerSet.CIRCLE_FILLED_7_7
    elif s == 2: marker.markerIndex = coin.SoMarkerSet.CIRCLE_FILLED_9_9
    elif s == 3: marker.markerIndex = coin.SoMarkerSet.CIRCLE_LINE_5_5
    elif s == 4: marker.markerIndex = coin.SoMarkerSet.CIRCLE_LINE_7_7
    elif s == 5: marker.markerIndex = coin.SoMarkerSet.CIRCLE_LINE_9_9
    elif s == 6: marker.markerIndex = coin.SoMarkerSet.SLASH_5_5
    elif s == 7: marker.markerIndex = coin.SoMarkerSet.SLASH_7_7
    elif s == 8: marker.markerIndex = coin.SoMarkerSet.SLASH_9_9
    elif s == 9: marker.markerIndex = coin.SoMarkerSet.BACKSLASH_5_5
    elif s == 10: marker.markerIndex = coin.SoMarkerSet.BACKSLASH_7_7
    elif s == 11: marker.markerIndex = coin.SoMarkerSet.BACKSLASH_9_9
    return marker

def shapify(obj):
    '''shapify(object): transforms a parametric shape object into
    non-parametric and returns the new object'''
    if not (obj.isDerivedFrom("Part::Feature")): return None
    if not "Shape" in obj.PropertiesList: return None
    shape = obj.Shape
    if len(shape.Faces) == 1:
        name = "Face"
    elif len(shape.Solids) > 0:
        name = "Solid"
    elif len(shape.Faces) > 1:
        name = "Shell"
    elif len(shape.Wires) == 1:
        name = "Wire"
    elif len(shape.Edges) == 1:
        if isinstance(shape.Edges[0].Curve,Part.Line):
            name = "Line"
        else:
            name = "Circle"
    else:
        name = getRealName(obj.Name)
    FreeCAD.ActiveDocument.removeObject(obj.Name)
    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature",name)
    newobj.Shape = shape
    FreeCAD.ActiveDocument.recompute()
    return newobj

def getGroupContents(objectslist):
    '''getGroupContents(objectlist): if any object of the given list
    is a group, its content is appened to the list, which is returned'''
    newlist = []
    for obj in objectslist:
        if obj.isDerivedFrom("App::DocumentObjectGroup"):
            newlist.extend(getGroupContents(obj.Group))
        else:
            newlist.append(obj)
    return newlist

def printShape(shape):
    """prints detailed information of a shape"""
    print "solids: ", len(shape.Solids)
    print "faces: ", len(shape.Faces)
    print "wires: ",len(shape.Wires)
    if shape.Faces:
        for f in range(len(shape.Faces)):
            print "face ",f,":"
            for v in shape.Faces[f].Vertexes:
                print "    ",v.Point
    elif shape.Wires:
        for w in range(len(shape.Wires)):
            print "wire ",w,":"
            for v in shape.Wires[w].Vertexes:
                print "    ",v.Point
    else:
        for v in shape.Vertexes:
            print "    ",v.Point

def formatObject(target,origin=None):
    '''
    formatObject(targetObject,[originObject]): This function applies
    to the given target object the current properties 
    set on the toolbar (line color and line width),
    or copies the properties of another object if given as origin.
    It also places the object in construction group if needed.
    '''
    obrep = target.ViewObject
    ui = None
    if gui:
        if hasattr(FreeCADGui,"draftToolBar"):
            ui = FreeCADGui.draftToolBar
    if ui:
        doc = FreeCAD.ActiveDocument
        if ui.isConstructionMode():
            col = fcol = ui.getDefaultColor("constr")
            gname = getParam("constructiongroupname")
            if not gname:
                gname = "Construction"
            grp = doc.getObject(gname)
            if not grp:
                grp = doc.addObject("App::DocumentObjectGroup",gname) 
            grp.addObject(target)
            obrep.Transparency = 80
        else:
            col = ui.getDefaultColor("ui")
            fcol = ui.getDefaultColor("face")
        col = (float(col[0]),float(col[1]),float(col[2]),0.0)
        fcol = (float(fcol[0]),float(fcol[1]),float(fcol[2]),0.0)
        lw = ui.linewidth
        fs = ui.fontsize
        if not origin:
            if "FontSize" in obrep.PropertiesList: obrep.FontSize = fs
            if "TextColor" in obrep.PropertiesList: obrep.TextColor = col
            if "LineWidth" in obrep.PropertiesList: obrep.LineWidth = lw
            if "PointColor" in obrep.PropertiesList: obrep.PointColor = col
            if "LineColor" in obrep.PropertiesList: obrep.LineColor = col
            if "ShapeColor" in obrep.PropertiesList: obrep.ShapeColor = fcol
        else:
            matchrep = origin.ViewObject
            for p in matchrep.PropertiesList:
                if not p in ["DisplayMode","BoundingBox","Proxy","RootNode","Visibility"]:
                    if p in obrep.PropertiesList:
                        val = getattr(matchrep,p)
                        setattr(obrep,p,val)
            if matchrep.DisplayMode in obrep.listDisplayModes():
                obrep.DisplayMode = matchrep.DisplayMode

def getSelection():
    "getSelection(): returns the current FreeCAD selection"
    if gui:
        return FreeCADGui.Selection.getSelection()
    return None

def select(objs=None):
    "select(object): deselects everything and selects only the passed object or list"
    if gui:
        FreeCADGui.Selection.clearSelection()
        if objs:
            if not isinstance(objs,list):
                objs = [objs]
            for obj in objs:
                FreeCADGui.Selection.addSelection(obj)

def loadTexture(filename):
    "loadTexture(filename): returns a SoSFImage from a file"
    if gui:
        from pivy import coin
        from PyQt4 import QtGui
        try:
            p = QtGui.QImage(filename)
            size = coin.SbVec2s(p.width(), p.height())
            buffersize = p.numBytes()
            numcomponents = int (buffersize / ( size[0] * size[1] ))

            img = coin.SoSFImage()
            width = size[0]
            height = size[1]
            bytes = ""
           
            for y in range(height):
                #line = width*numcomponents*(height-(y));
                for x in range(width):
                    rgb = p.pixel(x,y)
                    if numcomponents == 1:
                        bytes = bytes + chr(QtGui.qGray( rgb ))
                    elif numcomponents == 2:
                        bytes = bytes + chr(QtGui.qGray( rgb ))
                        bytes = bytes + chr(QtGui.qAlpha( rgb ))
                    elif numcomponents == 3:
                        bytes = bytes + chr(QtGui.qRed( rgb ))
                        bytes = bytes + chr(QtGui.qGreen( rgb ))
                        bytes = bytes + chr(QtGui.qBlue( rgb ))
                    elif numcomponents == 4:
                        bytes = bytes + chr(QtGui.qRed( rgb ))
                        bytes = bytes + chr(QtGui.qGreen( rgb ))
                        bytes = bytes + chr(QtGui.qBlue( rgb ))
                        bytes = bytes + chr(QtGui.qAlpha( rgb ))
                    #line += numcomponents

            img.setValue(size, numcomponents, bytes)
        except:
            return None
        else:
            return img
    return None

def makeCircle(radius, placement=None, face=True, startangle=None, endangle=None, support=None):
    '''makeCircle(radius,[placement,face,startangle,endangle])
    or makeCircle(edge,[face]):
    Creates a circle object with given radius. If placement is given, it is
    used. If face is False, the circle is shown as a
    wireframe, otherwise as a face. If startangle AND endangle are given
    (in degrees), they are used and the object appears as an arc. If an edge
    is passed, its Curve must be a Part.Circle'''
    import Part
    if placement: typecheck([(placement,FreeCAD.Placement)], "makeCircle")
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Circle")
    _Circle(obj)
    if isinstance(radius,Part.Edge):
        edge = radius
        if isinstance(edge.Curve,Part.Circle):
            obj.Radius = edge.Curve.Radius
            placement = FreeCAD.Placement(edge.Placement)
            delta = edge.Curve.Center.sub(placement.Base)
            placement.move(delta)
            if len(edge.Vertexes) > 1:
                ref = placement.multVec(FreeCAD.Vector(1,0,0))
                v1 = (edge.Vertexes[0].Point).sub(edge.Curve.Center)
                v2 = (edge.Vertexes[-1].Point).sub(edge.Curve.Center)
                a1 = -math.degrees(DraftVecUtils.angle(v1,ref))
                a2 = -math.degrees(DraftVecUtils.angle(v2,ref))
                obj.FirstAngle = a1
                obj.LastAngle = a2
    else:    
        obj.Radius = radius
        if (startangle != None) and (endangle != None):
            if startangle == -0: startangle = 0
            obj.FirstAngle = startangle
            obj.LastAngle = endangle
    obj.Support = support
    if placement: obj.Placement = placement
    if gui:
        _ViewProviderDraft(obj.ViewObject)
        if not face: obj.ViewObject.DisplayMode = "Wireframe"
        formatObject(obj)
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj
    
def makeRectangle(length, height, placement=None, face=True, support=None):
    '''makeRectangle(length,width,[placement],[face]): Creates a Rectangle
    object with length in X direction and height in Y direction.
    If a placement is given, it is used. If face is False, the
    rectangle is shown as a wireframe, otherwise as a face.'''
    if placement: typecheck([(placement,FreeCAD.Placement)], "makeRectangle")
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Rectangle")
    _Rectangle(obj)
    
    obj.Length = length
    obj.Height = height
    obj.Support = support
    if placement: obj.Placement = placement
    if gui:
        _ViewProviderRectangle(obj.ViewObject)
        if not face: obj.ViewObject.DisplayMode = "Wireframe"
        formatObject(obj)
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj
        
def makeDimension(p1,p2,p3=None,p4=None):
    '''makeDimension(p1,p2,[p3]) or makeDimension(object,i1,i2,p3)
    or makeDimension(objlist,indices,p3): Creates a Dimension object with
    the dimension line passign through p3.The current line width and color
    will be used. There are multiple  ways to create a dimension, depending on
    the arguments you pass to it:
    - (p1,p2,p3): creates a standard dimension from p1 to p2
    - (object,i1,i2,p3): creates a linked dimension to the given object,
    measuring the distance between its vertices indexed i1 and i2
    - (object,i1,mode,p3): creates a linked dimension
    to the given object, i1 is the index of the (curved) edge to measure,
    and mode is either "radius" or "diameter".
    '''
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Dimension")
    _Dimension(obj)
    if gui:
        _ViewProviderDimension(obj.ViewObject)
    if isinstance(p1,Vector) and isinstance(p2,Vector):
        obj.Start = p1
        obj.End = p2
        if not p3:
            p3 = p2.sub(p1)
            p3.multiply(0.5)
            p3 = p1.add(p3)
    elif isinstance(p2,int) and isinstance(p3,int):
        obj.Base = p1
        obj.LinkedVertices = idx = [p2,p3]
        p3 = p4
        if not p3:
            v1 = obj.Base.Shape.Vertexes[idx[0]].Point
            v2 = obj.Base.Shape.Vertexes[idx[1]].Point
            p3 = v2.sub(v1)
            p3.multiply(0.5)
            p3 = v1.add(p3)
    elif isinstance(p3,str):
        obj.Base = p1
        if p3 == "radius":
            obj.LinkedVertices = [p2,1,1]
            obj.ViewObject.Override = "rdim"
        elif p3 == "diameter":
            obj.LinkedVertices = [p2,2,1]
            obj.ViewObject.Override = "ddim"
        p3 = p4
        if not p3:
            p3 = obj.Base.Shape.Edges[0].Curve.Center.add(Vector(1,0,0))
    obj.Dimline = p3
    if gui:
        formatObject(obj)
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj

def makeAngularDimension(center,angles,p3):
    '''makeAngularDimension(center,[angle1,angle2],p3): creates an angular Dimension
    from the given center, with the given list of angles, passing through p3.
    '''
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Dimension")
    _AngularDimension(obj)
    obj.Center = center
    for a in range(len(angles)):
        if angles[a] > 2*math.pi:
            angles[a] = angles[a]-(2*math.pi)
    obj.FirstAngle = math.degrees(angles[1])
    obj.LastAngle = math.degrees(angles[0])
    obj.Dimline = p3
    if gui:
        _ViewProviderAngularDimension(obj.ViewObject)
        formatObject(obj)
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj

def makeWire(pointslist,closed=False,placement=None,face=True,support=None):
    '''makeWire(pointslist,[closed],[placement]): Creates a Wire object
    from the given list of vectors. If closed is True or first
    and last points are identical, the wire is closed. If face is
    true (and wire is closed), the wire will appear filled. Instead of
    a pointslist, you can also pass a Part Wire.'''
    import DraftGeomUtils, Part
    if not isinstance(pointslist,list):
        e = pointslist.Wires[0].Edges
        pointslist = Part.Wire(DraftGeomUtils.sortEdges(e))
        nlist = []
        for v in pointslist.Vertexes:
            nlist.append(v.Point)
        if DraftGeomUtils.isReallyClosed(pointslist):
            closed = True
        pointslist = nlist
    print pointslist
    print closed
    if placement: typecheck([(placement,FreeCAD.Placement)], "makeWire")
    if len(pointslist) == 2: fname = "Line"
    else: fname = "DWire"
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",fname)
    _Wire(obj)
    obj.Points = pointslist
    obj.Closed = closed
    obj.Support = support
    if placement: obj.Placement = placement
    if gui:
        _ViewProviderWire(obj.ViewObject)
        if not face: obj.ViewObject.DisplayMode = "Wireframe"
        formatObject(obj)
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj

def makePolygon(nfaces,radius=1,inscribed=True,placement=None,face=True,support=None):
    '''makePolgon(nfaces,[radius],[inscribed],[placement],[face]): Creates a
    polygon object with the given number of faces and the radius.
    if inscribed is False, the polygon is circumscribed around a circle
    with the given radius, otherwise it is inscribed. If face is True,
    the resulting shape is displayed as a face, otherwise as a wireframe.
    '''
    if nfaces < 3: return None
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Polygon")
    _Polygon(obj)
    obj.FacesNumber = nfaces
    obj.Radius = radius
    if inscribed:
        obj.DrawMode = "inscribed"
    else:
        obj.DrawMode = "circumscribed"
    obj.Support = support
    if placement: obj.Placement = placement
    if gui:
        _ViewProviderDraft(obj.ViewObject)
        if not face: obj.ViewObject.DisplayMode = "Wireframe"
        formatObject(obj)
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj

def makeLine(p1,p2):
    '''makeLine(p1,p2): Creates a line between p1 and p2.'''
    obj = makeWire([p1,p2])
    return obj

def makeBSpline(pointslist,closed=False,placement=None,face=True,support=None):
    '''makeBSpline(pointslist,[closed],[placement]): Creates a B-Spline object
    from the given list of vectors. If closed is True or first
    and last points are identical, the wire is closed. If face is
    true (and wire is closed), the wire will appear filled. Instead of
    a pointslist, you can also pass a Part Wire.'''
    if not isinstance(pointslist,list):
        nlist = []
        for v in pointslist.Vertexes:
            nlist.append(v.Point)
        pointslist = nlist
    if placement: typecheck([(placement,FreeCAD.Placement)], "makeBSpline")
    if len(pointslist) == 2: fname = "Line"
    else: fname = "BSpline"
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",fname)
    _BSpline(obj)
    obj.Points = pointslist
    obj.Closed = closed
    obj.Support = support
    if placement: obj.Placement = placement
    if gui:
        _ViewProviderBSpline(obj.ViewObject)
        if not face: obj.ViewObject.DisplayMode = "Wireframe"
        formatObject(obj)
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj

def makeText(stringslist,point=Vector(0,0,0),screen=False):
    '''makeText(strings,[point],[screen]): Creates a Text object at the given point,
    containing the strings given in the strings list, one string by line (strings
    can also be one single string). The current color and text height and font
    specified in preferences are used.
    If screen is True, the text always faces the view direction.'''
    typecheck([(point,Vector)], "makeText")
    if not isinstance(stringslist,list): stringslist = [stringslist]
    textbuffer = []
    for l in stringslist: textbuffer.append(unicode(l).encode('utf-8'))
    obj=FreeCAD.ActiveDocument.addObject("App::Annotation","Text")
    obj.LabelText=textbuffer
    obj.Position=point
    if not screen: obj.ViewObject.DisplayMode="World"
    h = getParam("textheight")
    if screen: h = h*10
    obj.ViewObject.FontSize = h
    obj.ViewObject.FontName = getParam("textfont")
    obj.ViewObject.LineSpacing = 0.6
    formatObject(obj)
    select(obj)
    return obj

def makeCopy(obj,force=None,reparent=False):
    '''makeCopy(object): returns an exact copy of an object'''
    if (getType(obj) == "Rectangle") or (force == "Rectangle"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.Type,getRealName(obj.Name))
        _Rectangle(newobj)
        if gui:
            _ViewProviderRectangle(newobj.ViewObject)
    elif (getType(obj) == "Dimension") or (force == "Dimension"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.Type,getRealName(obj.Name))
        _Dimension(newobj)
        if gui:
            _ViewProviderDimension(newobj.ViewObject)
    elif (getType(obj) == "Wire") or (force == "Wire"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.Type,getRealName(obj.Name))
        _Wire(newobj)
        if gui:
            _ViewProviderWire(newobj.ViewObject)
    elif (getType(obj) == "Circle") or (force == "Circle"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.Type,getRealName(obj.Name))
        _Circle(newobj)
        if gui:
            _ViewProviderDraft(newobj.ViewObject)
    elif (getType(obj) == "Polygon") or (force == "Polygon"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.Type,getRealName(obj.Name))
        _Polygon(newobj)
        if gui:
            _ViewProviderPolygon(newobj.ViewObject)
    elif (getType(obj) == "BSpline") or (force == "BSpline"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.Type,getRealName(obj.Name))
        _BSpline(newobj)
        if gui:
            _ViewProviderBSpline(newobj.ViewObject)
    elif (getType(obj) == "Block") or (force == "BSpline"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.Type,getRealName(obj.Name))
        _Block(newobj)
        if gui:
            _ViewProviderDraftPart(newobj.ViewObject)
    elif (getType(obj) == "Structure") or (force == "Structure"):
        import ArchStructure
        newobj = FreeCAD.ActiveDocument.addObject(obj.Type,getRealName(obj.Name))
        ArchStructure._Structure(newobj)
        if gui:
            ArchStructure._ViewProviderStructure(newobj.ViewObject)
    elif (getType(obj) == "Wall") or (force == "Wall"):
        import ArchWall
        newobj = FreeCAD.ActiveDocument.addObject(obj.Type,getRealName(obj.Name))
        ArchWall._Wall(newobj)
        if gui:
            ArchWall._ViewProviderWall(newobj.ViewObject)
    elif (getType(obj) == "Window") or (force == "Window"):
        import ArchWindow
        newobj = FreeCAD.ActiveDocument.addObject(obj.Type,getRealName(obj.Name))
        ArchWindow._Window(newobj)
        if gui:
            Archwindow._ViewProviderWindow(newobj.ViewObject)
    elif (getType(obj) == "Cell") or (force == "Cell"):
        import ArchCell
        newobj = FreeCAD.ActiveDocument.addObject(obj.Type,getRealName(obj.Name))
        ArchCell._Cell(newobj)
        if gui:
            ArchCell._ViewProviderCell(newobj.ViewObject)
    elif (getType(obj) == "Sketch") or (force == "Sketch"):
        newobj = FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject",getRealName(obj.Name))
        for geo in obj.Geometries:
            newobj.addGeometry(geo)
        for con in obj.constraints:
            newobj.addConstraint(con)
    elif obj.isDerivedFrom("Part::Feature"):
        newobj = FreeCAD.ActiveDocument.addObject("Part::Feature",getRealName(obj.Name))
        newobj.Shape = obj.Shape
    else:
        print "Error: Object type cannot be copied"
        return None
    for p in obj.PropertiesList:
        if not p in ["Proxy"]:
            if p in newobj.PropertiesList:
                setattr(newobj,p,obj.getPropertyByName(p))
    if reparent:
        parents = obj.InList
        if parents:
            for par in parents:
                if par.Type == "App::DocumentObjectGroup":
                    par.addObject(newobj)
                else:
                    for prop in par.PropertiesList:
                        if getattr(par,prop) == obj:
                            setattr(par,prop,newobj)
    formatObject(newobj,obj)
    return newobj

def makeBlock(objectslist):
    '''makeBlock(objectslist): Creates a Draft Block from the given objects'''
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Block")
    _Block(obj)
    obj.Components = objectslist
    if gui:
        _ViewProviderDraftPart(obj.ViewObject)
        for o in objectslist:
            o.ViewObject.Visibility = False
        select(obj)
    return obj

def makeArray(baseobject,arg1,arg2,arg3,arg4=None):
    '''makeArray(object,xvector,yvector,xnum,ynum) for rectangular array, or
    makeArray(object,center,totalangle,totalnum) for polar array: Creates an array
    of the given object
    with, in case of rectangular array, xnum of iterations in the x direction
    at xvector distance between iterations, and same for y direction with yvector
    and ynum. In case of polar array, center is a vector, totalangle is the angle
    to cover (in degrees) and totalnum is the number of objects, including the original.
    The result is a parametric Draft Array.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Array")
    _Array(obj)
    obj.Base = baseobject
    if arg4:
        obj.ArrayType = "ortho"
        obj.IntervalX = arg1
        obj.IntervalY = arg2
        obj.NumberX = arg3
        obj.NumberY = arg4
    else:
        obj.ArrayType = "polar"
        obj.Center = arg1
        obj.Angle = arg2
        obj.NumberPolar = arg3
    if gui:
        _ViewProviderDraftPart(obj.ViewObject)  
        baseobject.ViewObject.hide()
        select(obj)
    return obj

def extrude(obj,vector):
    '''makeExtrusion(object,vector): extrudes the given object
    in the direction given by the vector. The original object
    gets hidden.'''
    newobj = FreeCAD.ActiveDocument.addObject("Part::Extrusion","Extrusion")
    newobj.Base = obj
    newobj.Dir = vector
    obj.ViewObject.Visibility = False
    formatObject(newobj,obj)
    FreeCAD.ActiveDocument.recompute()
    return newobj

def fuse(object1,object2):
    '''fuse(oject1,object2): returns an object made from
    the union of the 2 given objects. If the objects are
    coplanar, a special Draft Wire is used, otherwise we use
    a standard Part fuse.'''
    import DraftGeomUtils, Part
    # testing if we have holes:
    holes = False
    fshape = object1.Shape.fuse(object2.Shape)
    fshape = fshape.removeSplitter()
    for f in fshape.Faces:
        if len(f.Wires) > 1:
            holes = True
    if DraftGeomUtils.isCoplanar(object1.Shape.fuse(object2.Shape).Faces) and not holes:
        obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Fusion")
        _Wire(obj)
        if gui:
            _ViewProviderWire(obj.ViewObject)
        obj.Base = object1
        obj.Tool = object2
    elif holes:
        # temporary hack, since Part::Fuse objects don't remove splitters
        obj = FreeCAD.ActiveDocument.addObject("Part::Feature","Fusion")
        obj.Shape = fshape
    else:
        obj = FreeCAD.ActiveDocument.addObject("Part::Fuse","Fusion")
        obj.Base = object1
        obj.Tool = object2
    if gui:
        object1.ViewObject.Visibility = False
        object2.ViewObject.Visibility = False
        formatObject(obj,object1)
    FreeCAD.ActiveDocument.recompute()
    return obj

def cut(object1,object2):
    '''cut(oject1,object2): returns a cut object made from
    the difference of the 2 given objects.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::Cut","Cut")
    obj.Base = object1
    obj.Tool = object2
    object1.ViewObject.Visibility = False
    object2.ViewObject.Visibility = False
    formatObject(obj,object1)
    FreeCAD.ActiveDocument.recompute()
    return obj

def move(objectslist,vector,copy=False):
    '''move(objects,vector,[copy]): Moves the objects contained
    in objects (that can be an object or a list of objects)
    in the direction and distance indicated by the given
    vector. If copy is True, the actual objects are not moved, but copies
    are created instead.he objects (or their copies) are returned.'''
    typecheck([(vector,Vector), (copy,bool)], "move")
    if not isinstance(objectslist,list): objectslist = [objectslist]
    newobjlist = []
    for obj in objectslist:
        if (obj.isDerivedFrom("Part::Feature")):
            if copy:
                newobj = makeCopy(obj)
            else:
                newobj = obj
            pla = newobj.Placement
            pla.move(vector)
        elif getType(obj) == "Annotation":
            if copy:
                newobj = FreeCAD.ActiveDocument.addObject("App::Annotation",getRealName(obj.Name))
                newobj.LabelText = obj.LabelText
            else:
                newobj = obj
            newobj.Position = obj.Position.add(vector)
        elif getType(obj) == "Dimension":
            if copy:
                newobj = FreeCAD.ActiveDocument.addObject("App::FeaturePython",getRealName(obj.Name))
                _Dimension(newobj)
                if gui:
                    _ViewProviderDimension(newobj.ViewObject)
            else:
                newobj = obj
            newobj.Start = obj.Start.add(vector)
            newobj.End = obj.End.add(vector)
            newobj.Dimline = obj.Dimline.add(vector)
        else:
            if copy: print "Mesh copy not supported at the moment" # TODO
            newobj = obj
            if "Placement" in obj.PropertiesList:
                pla = obj.Placement
                pla.move(vector)
        newobjlist.append(newobj)
    if copy and getParam("selectBaseObjects"):
        select(objectslist)
    else:
        select(newobjlist)
    if len(newobjlist) == 1: return newobjlist[0]
    return newobjlist

def array(objectslist,arg1,arg2,arg3,arg4=None):
    '''array(objectslist,xvector,yvector,xnum,ynum) for rectangular array, or
    array(objectslist,center,totalangle,totalnum) for polar array: Creates an array
    of the objects contained in list (that can be an object or a list of objects)
    with, in case of rectangular array, xnum of iterations in the x direction
    at xvector distance between iterations, and same for y direction with yvector
    and ynum. In case of polar array, center is a vector, totalangle is the angle
    to cover (in degrees) and totalnum is the number of objects, including the original.'''
    
    def rectArray(objectslist,xvector,yvector,xnum,ynum):
        typecheck([(xvector,Vector), (yvector,Vector), (xnum,int), (ynum,int)], "rectArray")
        if not isinstance(objectslist,list): objectslist = [objectslist]
        for xcount in range(xnum):
            currentxvector=DraftVecUtils.scale(xvector,xcount)
            if not xcount==0:
                move(objectslist,currentxvector,True)
            for ycount in range(ynum):
                currentxvector=FreeCAD.Base.Vector(currentxvector)
                currentyvector=currentxvector.add(DraftVecUtils.scale(yvector,ycount))
                if not ycount==0:
                    move(objectslist,currentyvector,True)
    def polarArray(objectslist,center,angle,num):
        typecheck([(center,Vector), (num,int)], "polarArray")
        if not isinstance(objectslist,list): objectslist = [objectslist]
        fraction = angle/num
        for i in range(num):
            currangle = fraction + (i*fraction)
            rotate(objectslist,currangle,center,copy=True)

    if arg4:
        rectArray(objectslist,arg1,arg2,arg3,arg4)
    else:
        polarArray(objectslist,arg1,arg2,arg3)
                
def rotate(objectslist,angle,center=Vector(0,0,0),axis=Vector(0,0,1),copy=False):
    '''rotate(objects,angle,[center,axis,copy]): Rotates the objects contained
    in objects (that can be a list of objects or an object) of the given angle
    (in degrees) around the center, using axis as a rotation axis. If axis is
    omitted, the rotation will be around the vertical Z axis.
    If copy is True, the actual objects are not moved, but copies
    are created instead. The objects (or their copies) are returned.'''
    import Part
    typecheck([(copy,bool)], "rotate")
    if not isinstance(objectslist,list): objectslist = [objectslist]
    newobjlist = []
    for obj in objectslist:
        if copy:
            newobj = makeCopy(obj)
        else:
            newobj = obj
        if (obj.isDerivedFrom("Part::Feature")):
            shape = obj.Shape.copy()
            shape.rotate(DraftVecUtils.tup(center), DraftVecUtils.tup(axis), angle)
            newobj.Shape = shape
        elif (obj.isDerivedFrom("App::Annotation")):
            if axis.normalize() == Vector(1,0,0):
                newobj.ViewObject.RotationAxis = "X"
                newobj.ViewObject.Rotation = angle
            elif axis.normalize() == Vector(0,1,0):
                newobj.ViewObject.RotationAxis = "Y"
                newobj.ViewObject.Rotation = angle
            elif axis.normalize() == Vector(0,-1,0):
                newobj.ViewObject.RotationAxis = "Y"
                newobj.ViewObject.Rotation = -angle
            elif axis.normalize() == Vector(0,0,1):
                newobj.ViewObject.RotationAxis = "Z"
                newobj.ViewObject.Rotation = angle
            elif axis.normalize() == Vector(0,0,-1):
                newobj.ViewObject.RotationAxis = "Z"
                newobj.ViewObject.Rotation = -angle
        elif hasattr(obj,"Placement"):
            shape = Part.Shape()
            shape.Placement = obj.Placement
            shape.rotate(DraftVecUtils.tup(center), DraftVecUtils.tup(axis), angle)
            newobj.Placement = shape.Placement
        if copy:
            formatObject(newobj,obj)
        newobjlist.append(newobj)
    if copy and getParam("selectBaseObjects"):
        select(objectslist)
    else:
        select(newobjlist)
    if len(newobjlist) == 1: return newobjlist[0]
    return newobjlist

def scale(objectslist,delta=Vector(1,1,1),center=Vector(0,0,0),copy=False,legacy=False):
    '''scale(objects,vector,[center,copy,legacy]): Scales the objects contained
    in objects (that can be a list of objects or an object) of the given scale
    factors defined by the given vector (in X, Y and Z directions) around
    given center. If legacy is True, direct (old) mode is used, otherwise
    a parametric copy is made. If copy is True, the actual objects are not moved,
    but copies are created instead. The objects (or their copies) are returned.'''
    if not isinstance(objectslist,list): objectslist = [objectslist]
    if legacy: 
        newobjlist = []
        for obj in objectslist:
            if copy:
                newobj = makeCopy(obj)
            else:
                newobj = obj
            sh = obj.Shape.copy()
            m = FreeCAD.Matrix()
            m.scale(delta)
            sh = sh.transformGeometry(m)
            corr = Vector(center.x,center.y,center.z)
            corr.scale(delta.x,delta.y,delta.z)
            corr = DraftVecUtils.neg(corr.sub(center))
            sh.translate(corr)
            if getType(obj) == "Rectangle":
                p = []
                for v in sh.Vertexes: p.append(v.Point)
                pl = obj.Placement.copy()
                pl.Base = p[0]
                diag = p[2].sub(p[0])
                bb = p[1].sub(p[0])
                bh = p[3].sub(p[0])
                nb = DraftVecUtils.project(diag,bb)
                nh = DraftVecUtils.project(diag,bh)
                if obj.Length < 0: l = -nb.Length
                else: l = nb.Length
                if obj.Height < 0: h = -nh.Length
                else: h = nh.Length
                newobj.Length = l
                newobj.Height = h
                tr = p[0].sub(obj.Shape.Vertexes[0].Point)
                newobj.Placement = pl
            elif getType(obj) == "Wire":
                p = []
                for v in sh.Vertexes: p.append(v.Point)
                newobj.Points = p
            elif (obj.isDerivedFrom("Part::Feature")):
                newobj.Shape = sh
            elif (obj.Type == "App::Annotation"):
                factor = delta.x * delta.y * delta.z * obj.ViewObject.FontSize
                obj.ViewObject.Fontsize = factor
            if copy: formatObject(newobj,obj)
            newobjlist.append(newobj)
        if copy and getParam("selectBaseObjects"):
            select(objectslist)
        else:
            select(newobjlist)
        if len(newobjlist) == 1: return newobjlist[0]
        return newobjlist
    else:
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Scale")
        _Clone(obj)
        obj.Objects = objectslist
        obj.Scale = delta
        corr = Vector(center.x,center.y,center.z)
        corr.scale(delta.x,delta.y,delta.z)
        corr = DraftVecUtils.neg(corr.sub(center))
        p = obj.Placement
        p.move(corr)
        obj.Placement = p
        if not copy:
            for o in objectslist:
                o.ViewObject.hide()
        if gui:
            _ViewProviderDraftPart(obj.ViewObject)
            formatObject(obj,objectslist[-1])
            select(obj)
        return obj

def offset(obj,delta,copy=False,bind=False,sym=False,occ=False):
    '''offset(object,Vector,[copymode],[bind]): offsets the given wire by
    applying the given Vector to its first vertex. If copymode is
    True, another object is created, otherwise the same object gets
    offsetted. If bind is True, and provided the wire is open, the original
    and the offsetted wires will be bound by their endpoints, forming a face
    if sym is True, bind must be true too, and the offset is made on both
    sides, the total width being the given delta length.'''
    import Part, DraftGeomUtils

    def getRect(p,obj):
        "returns length,heigh,placement"
        pl = obj.Placement.copy()
        pl.Base = p[0]
        diag = p[2].sub(p[0])
        bb = p[1].sub(p[0])
        bh = p[3].sub(p[0])
        nb = DraftVecUtils.project(diag,bb)
        nh = DraftVecUtils.project(diag,bh)
        if obj.Length < 0: l = -nb.Length
        else: l = nb.Length
        if obj.Height < 0: h = -nh.Length
        else: h = nh.Length
        return l,h,pl

    def getRadius(obj,delta):
        "returns a new radius for a regular polygon"
        an = math.pi/obj.FacesNumber
        nr = DraftVecUtils.rotate(delta,-an)
        nr.multiply(1/math.cos(an))
        nr = obj.Shape.Vertexes[0].Point.add(nr)
        nr = nr.sub(obj.Placement.Base)
        nr = nr.Length
        if obj.DrawMode == "inscribed":
            return nr
        else:
            return nr * math.cos(math.pi/obj.FacesNumber)

    if getType(obj) == "Circle":
        pass
    elif getType(obj) == "BSpline":
        pass
    else:
        if sym:
            d1 = delta.multiply(0.5)
            d2 = DraftVecUtils.neg(d1)
            n1 = DraftGeomUtils.offsetWire(obj.Shape,d1)
            n2 = DraftGeomUtils.offsetWire(obj.Shape,d2)
        else:
            newwire = DraftGeomUtils.offsetWire(obj.Shape,delta)
            p = DraftGeomUtils.getVerts(newwire)
    if occ:
        newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
        newobj.Shape = DraftGeomUtils.offsetWire(obj.Shape,delta,occ=True)
        formatObject(newobj,obj)
    elif bind:
        if not DraftGeomUtils.isReallyClosed(obj.Shape):
            if sym:
                s1 = n1
                s2 = n2
            else:
                s1 = obj.Shape
                s2 = newwire
            w1 = s1.Edges
            w2 = s2.Edges
            w3 = Part.Line(s1.Vertexes[0].Point,s2.Vertexes[0].Point).toShape()
            w4 = Part.Line(s1.Vertexes[-1].Point,s2.Vertexes[-1].Point).toShape()
            newobj = Part.Face(Part.Wire(w1+[w3]+w2+[w4]))
        else:
            newobj = Part.Face(obj.Shape.Wires[0])
    elif copy:
        if sym: return None
        if getType(obj) == "Wire":
            newobj = makeWire(p)
            newobj.Closed = obj.Closed
        elif getType(obj) == "Rectangle":
            length,height,plac = getRect(p,obj)
            newobj = makeRectangle(length,height,plac)
        elif getType(obj) == "Circle":
            pl = obj.Placement
            newobj = makeCircle(delta)
            newobj.FirstAngle = obj.FirstAngle
            newobj.LastAngle = obj.LastAngle
            newobj.Placement = pl
        elif getType(obj) == "Polygon":
            pl = obj.Placement
            newobj = makePolygon(obj.FacesNumber)
            newobj.Radius = getRadius(obj,delta)
            newobj.DrawMode = obj.DrawMode
            newobj.Placement = pl
        elif getType(obj) == "Part":
            newobj = makeWire(p)
            newobj.Closed = obj.Shape.isClosed()
        elif getType(obj) == "BSpline":
            newobj = makeBSpline(delta)
            newobj.Closed = obj.Closed
        formatObject(newobj,obj)
    else:
        if sym: return None
        if getType(obj) == "Wire":
            if obj.Base or obj.Tool:
                FreeCAD.Console.PrintWarning("Warning: object history removed\n")
                obj.Base = None
                obj.Tool = None
            obj.Points = p
        elif getType(obj) == "BSpline":
            print delta
            obj.Points = delta
            print "done"
        elif getType(obj) == "Rectangle":
            length,height,plac = getRect(p,obj)
            obj.Placement = plac
            obj.Length = length
            obj.Height = height
        elif getType(obj) == "Circle":
            obj.Radius = delta
        elif getType(obj) == "Polygon":
            obj.Radius = getRadius(obj,delta)
        elif getType(obj) == 'Part':
            print "unsupported object" # TODO
        newobj = obj
    if copy and getParam("selectBaseObjects"):
        select(newobj)
    else:
        select(obj)
    return newobj

def draftify(objectslist,makeblock=False):
    '''draftify(objectslist,[makeblock]): turns each object of the given list
    (objectslist can also be a single object) into a Draft parametric
    wire. If makeblock is True, multiple objects will be grouped in a block'''
    import DraftGeomUtils, Part

    if not isinstance(objectslist,list):
        objectslist = [objectslist]
    newobjlist = []
    for obj in objectslist:
        if obj.isDerivedFrom('Part::Feature'):
            for w in obj.Shape.Wires:
                if DraftGeomUtils.hasCurves(w):
                    if (len(w.Edges) == 1) and isinstance(w.Edges[0].Curve,Part.Circle):
                        nobj = makeCircle(w.Edges[0])
                    else:
                        nobj = FreeCAD.ActiveDocument.addObject("Part::Feature",obj.Name)
                        nobj.Shape = w
                else:
                    nobj = makeWire(w)
                if obj.Shape.Faces:
                    nobj.ViewObject.DisplayMode = "Flat Lines"
                else:
                    nobj.ViewObject.DisplayMode = "Wireframe"
                newobjlist.append(nobj)
                formatObject(nobj,obj)
            FreeCAD.ActiveDocument.removeObject(obj.Name)
    FreeCAD.ActiveDocument.recompute()
    if makeblock:
        return makeBlock(newobjlist)
    else:
        if len(newobjlist) == 1:
            return newobjlist[0]
        return newobjlist

def getSVG(obj,scale=1,linewidth=0.35,fontsize=12,fillstyle="shape color",direction=None):
    '''getSVG(object,[scale], [linewidth],[fontsize],[fillstyle],[direction]):
    returns a string containing a SVG representation of the given object,
    with the given linewidth and fontsize (used if the given object contains
    any text). You can also supply an arbitrary projection vector. the
    scale parameter allows to scale linewidths down, so they are resolution-independant.'''
    import Part, DraftGeomUtils
    svg = ""
    linewidth = linewidth/scale
    fontsize = (fontsize/scale)/2
    plane = None
    if direction:
        if isinstance(direction,FreeCAD.Vector):
            if direction != Vector(0,0,0):
                plane = WorkingPlane.plane()
                plane.alignToPointAndAxis(Vector(0,0,0),DraftVecUtils.neg(direction),0)
        elif isinstance(direction,WorkingPlane.plane):
            plane = direction

    def getLineStyle(obj):
        "returns a linestyle pattern for a given object"
        if obj.ViewObject:
            if hasattr(obj.ViewObject,"DrawStyle"):
                ds = obj.ViewObject.DrawStyle
                if ds == "Dashed":
                    return "0.09,0.05"
                elif ds == "Dashdot":
                    return "0.09,0.05,0.02,0.05"
                elif ds == "Dotted":
                    return "0.02,0.02"
        return "none"

    def getrgb(color):
        "getRGB(color): returns a rgb value #000000 from a freecad color"
        r = str(hex(int(color[0]*255)))[2:].zfill(2)
        g = str(hex(int(color[1]*255)))[2:].zfill(2)
        b = str(hex(int(color[2]*255)))[2:].zfill(2)
        col = "#"+r+g+b
        if col == "#ffffff":
            print getParam('SvgLinesBlack')
            if getParam('SvgLinesBlack'):
                col = "#000000"
        return col

    def getProj(vec):
        if not plane: return vec
        nx = DraftVecUtils.project(vec,plane.u)
        lx = nx.Length
        if abs(nx.getAngle(plane.u)) > 0.1: lx = -lx
        ny = DraftVecUtils.project(vec,plane.v)
        ly = ny.Length
        if abs(ny.getAngle(plane.v)) > 0.1: ly = -ly
        return Vector(lx,ly,0)

    def getPattern(pat):
        if pat in FreeCAD.svgpatterns:
            return FreeCAD.svgpatterns[pat]
        return ''

    def getPath(edges):
        svg ='<path id="' + name + '" '
        edges = DraftGeomUtils.sortEdges(edges)
        v = getProj(edges[0].Vertexes[0].Point)
        svg += 'd="M '+ str(v.x) +' '+ str(v.y) + ' '
        for e in edges:
            if isinstance(e.Curve,Part.Line) or  isinstance(e.Curve,Part.BSplineCurve):
                v = getProj(e.Vertexes[-1].Point)
                svg += 'L '+ str(v.x) +' '+ str(v.y) + ' '
            elif isinstance(e.Curve,Part.Circle):
                if len(e.Vertexes) == 1:
                    # complete circle
                    svg = getCircle(e)
                    return svg
                r = e.Curve.Radius
                drawing_plane_normal = FreeCAD.DraftWorkingPlane.axis
                if plane: drawing_plane_normal = plane.axis
                flag_large_arc = (((e.ParameterRange[1] - e.ParameterRange[0]) / math.pi) % 2) > 1
                flag_sweep = e.Curve.Axis * drawing_plane_normal >= 0
                v = getProj(e.Vertexes[-1].Point)
                svg += 'A ' + str(r) + ' ' + str(r) + ' '
                svg += '0 ' + str(int(flag_large_arc)) + ' ' + str(int(flag_sweep)) + ' '
                svg += str(v.x) + ' ' + str(v.y) + ' '
        if fill != 'none': svg += 'Z'
        svg += '" '
        svg += 'stroke="' + stroke + '" '
        svg += 'stroke-width="' + str(linewidth) + ' px" '
        svg += 'style="stroke-width:'+ str(linewidth)
        svg += ';stroke-miterlimit:4'
        svg += ';stroke-dasharray:' + lstyle
        svg += ';fill:' + fill + '"'
        svg += '/>\n'
        return svg

    def getCircle(edge):
        cen = getProj(edge.Curve.Center)
        rad = edge.Curve.Radius
        svg = '<circle cx="' + str(cen.x)
        svg += '" cy="' + str(cen.y)
        svg += '" r="' + str(rad)+'" '
        svg += 'stroke="' + stroke + '" '
        svg += 'stroke-width="' + str(linewidth) + ' px" '
        svg += 'style="stroke-width:'+ str(linewidth)
        svg += ';stroke-miterlimit:4'
        svg += ';stroke-dasharray:' + lstyle
        svg += ';fill:' + fill + '"'
        svg += '/>\n'
        return svg

    if getType(obj) == "Dimension":
        p1,p2,p3,p4,tbase,norm,rot = obj.ViewObject.Proxy.calcGeom(obj)
        dimText = getParam("dimPrecision")
        dimText = "%."+str(dimText)+"f"
        p1 = getProj(p1)
        p2 = getProj(p2)
        p3 = getProj(p3)
        p4 = getProj(p4)
        tbase = getProj(tbase)
        svg = '<g id="'+obj.Name+'"><path '
        if obj.ViewObject.DisplayMode == "2D":
            m = FreeCAD.Placement()
            m.Rotation.Q = rot
            m = m.toMatrix()
            if plane:
                vtext = m.multiply(plane.u)
            else:
                vtext = m.multiply(Vector(1,0,0))
            angle = -DraftVecUtils.angle(vtext)
            svg += 'd="M '+str(p1.x)+' '+str(p1.y)+' '
            svg += 'L '+str(p2.x)+' '+str(p2.y)+' '
            svg += 'L '+str(p3.x)+' '+str(p3.y)+' '
            svg += 'L '+str(p4.x)+' '+str(p4.y)+'" '
        else:
            ts = (len(dimText)*obj.ViewObject.FontSize)/4
            rm = ((p3.sub(p2)).Length/2)-ts
            p2a = getProj(p2.add(DraftVecUtils.scaleTo(p3.sub(p2),rm)))
            p2b = getProj(p3.add(DraftVecUtils.scaleTo(p2.sub(p3),rm)))
            angle = 0
            svg += 'd="M '+str(p1.x)+' '+str(p1.y)+' '
            svg += 'L '+str(p2.x)+' '+str(p2.y)+' '
            svg += 'L '+str(p2a.x)+' '+str(p2a.y)+' '
            svg += 'M '+str(p2b.x)+' '+str(p2b.y)+' '
            svg += 'L '+str(p3.x)+' '+str(p3.y)+' '
            svg += 'L '+str(p4.x)+' '+str(p4.y)+'" '                        
        svg += 'fill="none" stroke="'
        svg += getrgb(obj.ViewObject.LineColor) + '" '
        svg += 'stroke-width="' + str(linewidth) + ' px" '
        svg += 'style="stroke-width:'+ str(linewidth)
        svg += ';stroke-miterlimit:4;stroke-dasharray:none" '
        svg += 'freecad:basepoint1="'+str(p1.x)+' '+str(p1.y)+'" '
        svg += 'freecad:basepoint2="'+str(p4.x)+' '+str(p4.y)+'" '
        svg += 'freecad:dimpoint="'+str(p2.x)+' '+str(p2.y)+'"'
        svg += '/>\n'
        svg += '<circle cx="'+str(p2.x)+'" cy="'+str(p2.y)
        svg += '" r="'+str(fontsize)+'" '
        svg += 'fill="'+ getrgb(obj.ViewObject.LineColor) +'" stroke="none" '
        svg += 'style="stroke-miterlimit:4;stroke-dasharray:none" '
        svg += 'freecad:skip="1"'
        svg += '/>\n'
        svg += '<circle cx="'+str(p3.x)+'" cy="'+str(p3.y)
        svg += '" r="'+str(fontsize)+'" '
        svg += 'fill="#000000" stroke="none" '
        svg += 'style="stroke-miterlimit:4;stroke-dasharray:none" '
        svg += 'freecad:skip="1"'
        svg += '/>\n'
        svg += '<text id="' + obj.Name + '" fill="'
        svg += getrgb(obj.ViewObject.LineColor) +'" font-size="'
        svg += str(fontsize)+'" '
        svg += 'style="text-anchor:middle;text-align:center;'
        svg += 'font-family:'+obj.ViewObject.FontName+'" '
        svg += 'transform="rotate('+str(math.degrees(angle))
        svg += ','+ str(tbase.x) + ',' + str(tbase.y) + ') '
        svg += 'translate(' + str(tbase.x) + ',' + str(tbase.y) + ') '
        #svg += 'scale('+str(tmod/2000)+',-'+str(tmod/2000)+') '
        svg += 'scale(1,-1) '
        svg += '" freecad:skip="1"'
        svg += '>\n'
        svg += dimText % p3.sub(p2).Length
        svg += '</text>\n</g>\n'

    elif getType(obj) == "Annotation":
        "returns an svg representation of a document annotation"
        p = getProj(obj.Position)
        svg = '<text id="' + obj.Name + '" fill="'
        svg += getrgb(obj.ViewObject.TextColor)
        svg += '" font-size="'
        svg += str(fontsize)+'" '
        svg += 'style="text-anchor:middle;text-align:center;'
        svg += 'font-family:'+obj.ViewObject.FontName+'" '
        svg += 'transform="'
        if obj.ViewObject.RotationAxis == 'Z':
            if obj.ViewObject.Rotation != 0:
                svg += 'rotate('+str(obj.ViewObject.Rotation)
                svg += ','+ str(p.x) + ',' + str(p.y) + ') '
        svg += 'translate(' + str(p.x) + ',' + str(p.y) + ') '
        #svg +='scale('+str(tmod/2000)+','+str(-tmod/2000)+')'
        svg += 'scale(1,-1) '
        svg += '">\n'
        for l in obj.LabelText:
            svg += '<tspan>'+l+'</tspan>\n'
        svg += '</text>\n'

    elif getType(obj) == "Axis":
        "returns the SVG representation of an Arch Axis system"
        color = getrgb(obj.ViewObject.LineColor)
        lorig = getLineStyle(obj)
        name = obj.Name
        stroke = getrgb(obj.ViewObject.LineColor)
        fill = 'none'
        invpl = obj.Placement.inverse()
        n = 0
        for e in obj.Shape.Edges:
            lstyle = lorig
            svg += getPath([e])
            p1 = invpl.multVec(e.Vertexes[0].Point)
            p2 = invpl.multVec(e.Vertexes[1].Point)
            dv = p2.sub(p1)
            dv.normalize()
            rad = obj.ViewObject.BubbleSize
            center = p2.add(dv.scale(rad,rad,rad))
            lstyle = "none"
            svg += getCircle(Part.makeCircle(rad,center))
            svg += '<text fill="' + color + '" '
            svg += 'font-size="' + str(rad) + '" '
            svg += 'style="text-anchor:middle;'
            svg += 'text-align:center;'
            svg += 'font-family: Arial,sans;" '
            svg += 'transform="translate(' + str(center.x+rad/4) + ',' + str(center.y-rad/3) + ') '
            svg += 'scale(1,-1)"> '
            svg += '<tspan>' + obj.ViewObject.Proxy.getNumber(n) + '</tspan>\n'
            svg += '</text>\n'
            n += 1

    elif obj.isDerivedFrom('Part::Feature'):
        if obj.Shape.isNull(): return ''
        color = getrgb(obj.ViewObject.LineColor)
        # setting fill
        if obj.Shape.Faces and (obj.ViewObject.DisplayMode != "Wireframe"):
            if fillstyle == "shape color":
                fill = getrgb(obj.ViewObject.ShapeColor)
            else:
                fill = 'url(#'+fillstyle+')'
                svg += getPattern(fillstyle)
        else:
            fill = 'none'
        lstyle = getLineStyle(obj)
        name = obj.Name
        if obj.ViewObject.DisplayMode == "Shaded":
            stroke = "none"
        else:
            stroke = getrgb(obj.ViewObject.LineColor)
        
        if len(obj.Shape.Vertexes) > 1:
            wiredEdges = []
            if obj.Shape.Faces:
                for f in obj.Shape.Faces:
                    svg += getPath(f.Edges)
                    wiredEdges.extend(f.Edges)
            else:
                for w in obj.Shape.Wires:
                    svg += getPath(w.Edges)
                    wiredEdges.extend(w.Edges)
            if len(wiredEdges) != len(obj.Shape.Edges):
                for e in obj.Shape.Edges:
                    if (DraftGeomUtils.findEdge(e,wiredEdges) == None):
                        svg += getPath([e])
        else:
            svg = getCircle(obj.Shape.Edges[0])
    return svg

def makeDrawingView(obj,page,lwmod=None,tmod=None):
    '''
    makeDrawingView(object,page,[lwmod,tmod]) - adds a View of the given object to the
    given page. lwmod modifies lineweights (in percent), tmod modifies text heights
    (in percent). The Hint scale, X and Y of the page are used.
    '''
    viewobj = FreeCAD.ActiveDocument.addObject("Drawing::FeatureViewPython","View"+obj.Name)
    _DrawingView(viewobj)
    page.addObject(viewobj)
    viewobj.Scale = page.ViewObject.HintScale
    viewobj.X = page.ViewObject.HintOffsetX
    viewobj.Y = page.ViewObject.HintOffsetY
    viewobj.Source = obj
    if lwmod: viewobj.LineweightModifier = lwmod
    if tmod: viewobj.TextModifier = tmod
    return viewobj

def makeShape2DView(baseobj,projectionVector=None):
    '''
    makeShape2DView(object,[projectionVector]) - adds a 2D shape to the document, which is a
    2D projection of the given object. A specific projection vector can also be given.
    '''
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Shape2DView")
    _Shape2DView(obj)
    if gui:
        _ViewProviderDraft(obj.ViewObject)
    obj.Base = baseobj
    if projectionVector:
        obj.Projection = projectionVector
    select(obj)
    return obj

def makeSketch(objectslist,autoconstraints=False,addTo=None,name="Sketch"):
    '''makeSketch(objectslist,[autoconstraints],[addTo],[name]): makes a Sketch
    objectslist with the given Draft objects. If autoconstraints is True,
    constraints will be automatically added to wire nodes, rectangles
    and circles. If addTo is an existing sketch, geometry will be added to it instead of
    creating a new one.'''
    import Part, DraftGeomUtils
    from Sketcher import Constraint

    StartPoint = 1
    EndPoint = 2
    MiddlePoint = 3
    
    if not isinstance(objectslist,list):
        objectslist = [objectslist]
    if addTo:
        nobj = addTo
    else:
        nobj = FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject",name)
        nobj.ViewObject.Autoconstraints = False
    for obj in objectslist:
        ok = False
        tp = getType(obj)
        if tp == "BSpline":
            pass
        elif tp == "Circle":
            if obj.FirstAngle == obj.LastAngle:
                nobj.addGeometry(obj.Shape.Edges[0].Curve)
            else:
                nobj.addGeometry(Part.ArcOfCircle(obj.Shape.Edges[0].Curve,math.radians(obj.FirstAngle),math.radians(obj.LastAngle)))
            # TODO add Radius constraits
            ok = True
        elif tp == "Rectangle":
            if obj.FilletRadius == 0:
                for edge in obj.Shape.Edges:
                    nobj.addGeometry(edge.Curve)
                if autoconstraints:
                    last = nobj.GeometryCount - 1
                    segs = [last-3,last-2,last-1,last]
                    if obj.Placement.Rotation.Q == (0,0,0,1):
                        nobj.addConstraint(Constraint("Coincident",last-3,EndPoint,last-2,StartPoint))
                        nobj.addConstraint(Constraint("Coincident",last-2,EndPoint,last-1,StartPoint))
                        nobj.addConstraint(Constraint("Coincident",last-1,EndPoint,last,StartPoint))
                        nobj.addConstraint(Constraint("Coincident",last,EndPoint,last-3,StartPoint))
                    nobj.addConstraint(Constraint("Horizontal",last-3))
                    nobj.addConstraint(Constraint("Vertical",last-2))
                    nobj.addConstraint(Constraint("Horizontal",last-1))
                    nobj.addConstraint(Constraint("Vertical",last))
                ok = True
        elif tp in ["Wire","Polygon"]:
            closed = False
            if tp == "Polygon":
                closed = True
            elif hasattr(obj,"Closed"):
                closed = obj.Closed
            for edge in obj.Shape.Edges:
                nobj.addGeometry(edge.Curve)
            if autoconstraints:
                last = nobj.GeometryCount
                segs = range(last-len(obj.Shape.Edges),last-1)
                for seg in segs:
                    nobj.addConstraint(Constraint("Coincident",seg,EndPoint,seg+1,StartPoint))
                    if DraftGeomUtils.isAligned(nobj.Geometry[seg],"x"):
                        nobj.addConstraint(Constraint("Vertical",seg))
                    elif DraftGeomUtils.isAligned(nobj.Geometry[seg],"y"):
                        nobj.addConstraint(Constraint("Horizontal",seg))
                if closed:
                    nobj.addConstraint(Constraint("Coincident",last-1,EndPoint,segs[0],StartPoint))
            ok = True
        if (not ok) and obj.isDerivedFrom("Part::Feature"):
            if DraftGeomUtils.hasOnlyWires(obj.Shape):
                for w in obj.Shape.Wires:
                    for edge in DraftGeomUtils.sortEdges(w.Edges):
                        g = DraftGeomUtils.geom(edge)
                        if g:
                            nobj.addGeometry(g)  
                    if autoconstraints:
                        last = nobj.GeometryCount
                        segs = range(last-len(w.Edges),last-1)
                        for seg in segs:
                            nobj.addConstraint(Constraint("Coincident",seg,EndPoint,seg+1,StartPoint))
                            if DraftGeomUtils.isAligned(nobj.Geometry[seg],"x"):
                                nobj.addConstraint(Constraint("Vertical",seg))
                            elif DraftGeomUtils.isAligned(nobj.Geometry[seg],"y"):
                                nobj.addConstraint(Constraint("Horizontal",seg))
                        if w.isClosed:
                            nobj.addConstraint(Constraint("Coincident",last-1,EndPoint,segs[0],StartPoint))
            else:
                for edge in obj.Shape.Edges:
                    nobj.addGeometry(DraftGeomUtils.geom(edge))
                    if autoconstraints:
                        last = nobj.GeometryCount - 1
                        if DraftGeomUtils.isAligned(nobj.Geometry[last],"x"):
                            nobj.addConstraint(Constraint("Vertical",last))
                        elif DraftGeomUtils.isAligned(nobj.Geometry[last],"y"):
                            nobj.addConstraint(Constraint("Horizontal",last))
            ok = True
        if ok:
            FreeCAD.ActiveDocument.removeObject(obj.Name)
    FreeCAD.ActiveDocument.recompute()
    return nobj

def makePoint(X=0, Y=0, Z=0,color=None,name = "Point", point_size= 5):
    ''' make a point (at coordinates x,y,z ,color(r,g,b),point_size)
        example usage: 
        p1 = makePoint()
        p1.ViewObject.Visibility= False # make it invisible
        p1.ViewObject.Visibility= True  # make it visible
        p1 = makePoint(-1,0,0) #make a point at -1,0,0
        p1 = makePoint(1,0,0,(1,0,0)) # color = red
        p1.X = 1 #move it in x
        p1.ViewObject.PointColor =(0.0,0.0,1.0) #change the color-make sure values are floats
    '''
    obj=FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Point(obj,X,Y,Z)
    obj.X = X
    obj.Y = Y
    obj.Z = Z
    if gui:
        _ViewProviderPoint(obj.ViewObject)
        if not color:
            color = FreeCADGui.draftToolBar.getDefaultColor('ui')
        obj.ViewObject.PointColor = (float(color[0]), float(color[1]), float(color[2]))
        obj.ViewObject.PointSize = point_size
        obj.ViewObject.Visibility = True
    FreeCAD.ActiveDocument.recompute()
    return obj

def clone(obj,delta=None):
    '''clone(obj,[delta]): makes a clone of the given object(s). The clone is an exact,
    linked copy of the given object. If the original object changes, the final object
    changes too. Optionally, you can give a delta Vector to move the clone from the
    original position.'''
    if not isinstance(obj,list):
        obj = [obj]
    cl = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Clone")
    cl.Label = "Clone of " + obj[0].Label
    _Clone(cl)
    if gui:
        _ViewProviderClone(cl.ViewObject)
        formatObject(cl,obj[0])
    cl.Objects = obj
    if delta:
        cl.Placement.move(delta)
    return cl

def heal(objlist=None,delete=True,reparent=True):
    '''heal([objlist],[delete],[reparent]) - recreates Draft objects that are damaged,
    for example if created from an earlier version. If delete is True,
    the damaged objects are deleted (default). If ran without arguments, all the objects
    in the document will be healed if they are damaged. If reparent is True (default),
    new objects go at the very same place in the tree than their original.'''

    if not objlist:
        objlist = FreeCAD.ActiveDocument.Objects
        print "Healing whole document..."

    if not isinstance(objlist,list):
        objlist = [objlist]

    dellist = []
    got = False
    
    for obj in objlist:
        dtype = getType(obj)
        ftype = obj.Type
        if ftype in ["Part::FeaturePython","App::FeaturePython"]:
            if obj.ViewObject.Proxy == 1 and dtype in ["Unknown","Part"]:
                got = True
                dellist.append(obj.Name)
                props = obj.PropertiesList
                if ("Dimline" in props) and ("Start" in props):
                    print "Healing " + obj.Name + " of type Dimension"
                    nobj = makeCopy(obj,force="Dimension",reparent=reparent)
                elif ("Height" in props) and ("Length" in props):
                    print "Healing " + obj.Name + " of type Rectangle"
                    nobj = makeCopy(obj,force="Rectangle",reparent=reparent)
                elif ("Points" in props) and ("Closed" in props):
                    print "Healing " + obj.Name + " of type Wire"
                    nobj = makeCopy(obj,force="Wire",reparent=reparent)
                elif ("Radius" in props) and ("FirstAngle" in props):
                    print "Healing " + obj.Name + " of type Circle"
                    nobj = makeCopy(obj,force="Circle",reparent=reparent)
                else:
                    dellist.pop()
                    print "Object " + obj.Name + " is not healable"

    if not got:
        print "No object seems to need healing"
    else:
        print "Healed ",len(dellist)," objects"

    if dellist and delete:
        for n in dellist:
            FreeCAD.ActiveDocument.removeObject(n)

			
#---------------------------------------------------------------------------
# Python Features definitions
#---------------------------------------------------------------------------

class _ViewProviderDraft:
    "A generic View Provider for Draft objects"
        
    def __init__(self, obj):
        obj.Proxy = self
        self.Object = obj.Object
        
    def attach(self, obj):
        self.Object = obj.Object
        return

    def updateData(self, fp, prop):
        return

    def getDisplayModes(self,obj):
        modes=[]
        return modes

    def setDisplayMode(self,mode):
        return mode

    def onChanged(self, vp, prop):
        return

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def setEdit(self,vp,mode):
        FreeCADGui.runCommand("Draft_Edit")
        return True

    def unsetEdit(self,vp,mode):
        if FreeCAD.activeDraftCommand:
            FreeCAD.activeDraftCommand.finish()
        return
    
    def getIcon(self):
        return(":/icons/Draft_Draft.svg")

    def claimChildren(self):
        objs = []
        if hasattr(self.Object,"Base"):
            objs.append(self.Object.Base)
        if hasattr(self.Object,"Objects"):
            objs.extend(self.Object.Objects)
        if hasattr(self.Object,"Components"):
            objs.extend(self.Object.Components)
        return objs
		
class _Dimension:
    "The Dimension object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyVector","Start","Base",
                        "Startpoint of dimension")
        obj.addProperty("App::PropertyVector","End","Base",
                        "Endpoint of dimension")
        obj.addProperty("App::PropertyVector","Dimline","Base",
                        "Point through which the dimension line passes")
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object this dimension is linked to")
        obj.addProperty("App::PropertyIntegerList","LinkedVertices","Base",
                        "The indices of the vertices from the base object to measure")
        obj.Start = FreeCAD.Vector(0,0,0)
        obj.End = FreeCAD.Vector(1,0,0)
        obj.Dimline = FreeCAD.Vector(0,1,0)
        obj.Proxy = self
        self.Type = "Dimension"
		
    def onChanged(self, obj, prop):
        pass

    def execute(self, obj):
        if obj.ViewObject:
            obj.ViewObject.update()
        
class _ViewProviderDimension:
    "A View Provider for the Dimension object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyLength","FontSize","Base","Font size")
        obj.addProperty("App::PropertyString","FontName","Base","Font name")
        obj.addProperty("App::PropertyLength","LineWidth","Base","Line width")
        obj.addProperty("App::PropertyColor","LineColor","Base","Line color")
        obj.addProperty("App::PropertyLength","ExtLines","Base","Ext lines")
        obj.addProperty("App::PropertyVector","TextPosition","Base","The position of the text. Leave (0,0,0) for automatic position")
        obj.addProperty("App::PropertyString","Override","Base","Text override. Use 'dim' to insert the dimension length")
        obj.Proxy = self
        obj.FontSize=getParam("textheight")
        obj.FontName=getParam("textfont")
        obj.ExtLines=0.3
        obj.Override = ''

    def calcGeom(self,obj):
        import Part, DraftGeomUtils
        p1 = obj.Start
        p4 = obj.End
        base = Part.Line(p1,p4).toShape()
        proj = DraftGeomUtils.findDistance(obj.Dimline,base)
        if not proj:
            p2 = p1
            p3 = p4
        else:
            p2 = p1.add(DraftVecUtils.neg(proj))
            p3 = p4.add(DraftVecUtils.neg(proj))
            dmax = obj.ViewObject.ExtLines
            if dmax and (proj.Length > dmax):
                p1 = p2.add(DraftVecUtils.scaleTo(proj,dmax))
                p4 = p3.add(DraftVecUtils.scaleTo(proj,dmax))
        midpoint = p2.add(DraftVecUtils.scale(p3.sub(p2),0.5))
        if not proj:
            ed = DraftGeomUtils.vec(base)
            proj = ed.cross(Vector(0,0,1))
        if not proj: norm = Vector(0,0,1)
        else: norm = DraftVecUtils.neg(p3.sub(p2).cross(proj))
        if not DraftVecUtils.isNull(norm):
            norm.normalize()
        va = get3DView().getViewDirection()
        if va.getAngle(norm) < math.pi/2:
            norm = DraftVecUtils.neg(norm)
        u = p3.sub(p2)
        u.normalize()
        c = get3DView().getCameraNode()
        r = c.orientation.getValue()
        ru = Vector(r.multVec(coin.SbVec3f(1,0,0)).getValue())
        if ru.getAngle(u) > math.pi/2: u = DraftVecUtils.neg(u)
        v = norm.cross(u)
        offset = DraftVecUtils.scaleTo(v,obj.ViewObject.FontSize*.2)
        if obj.ViewObject:
            if hasattr(obj.ViewObject,"DisplayMode"):
                if obj.ViewObject.DisplayMode == "3D":
                    offset = DraftVecUtils.neg(offset)
            if hasattr(obj.ViewObject,"TextPosition"):
                if obj.ViewObject.TextPosition == Vector(0,0,0):
                    tbase = midpoint.add(offset)
                else:
                    tbase = obj.ViewObject.TextPosition
            else:
                tbase = midpoint.add(offset)
        else:
            tbase = midpoint
        rot = FreeCAD.Placement(DraftVecUtils.getPlaneRotation(u,v,norm)).Rotation.Q
        return p1,p2,p3,p4,tbase,norm,rot

    def attach(self, obj):
        self.Object = obj.Object
        p1,p2,p3,p4,tbase,norm,rot = self.calcGeom(obj.Object)
        self.color = coin.SoBaseColor()
        self.color.rgb.setValue(obj.LineColor[0],
                                obj.LineColor[1],
                                obj.LineColor[2])
        self.font = coin.SoFont()
        self.font3d = coin.SoFont()
        self.text = coin.SoAsciiText()
        self.text3d = coin.SoText2()
        self.text.justification = self.text3d.justification = coin.SoAsciiText.CENTER
        self.text.string = self.text3d.string = ''
        self.textpos = coin.SoTransform()
        self.textpos.translation.setValue([tbase.x,tbase.y,tbase.z])
        tm = DraftVecUtils.getPlaneRotation(p3.sub(p2),norm)
        rm = coin.SbRotation()
        self.textpos.rotation = rm
        label = coin.SoSeparator()
        label.addChild(self.textpos)
        label.addChild(self.color)
        label.addChild(self.font)
        label.addChild(self.text)
        label3d = coin.SoSeparator()
        label3d.addChild(self.textpos)
        label3d.addChild(self.color)
        label3d.addChild(self.font3d)
        label3d.addChild(self.text3d)
        self.coord1 = coin.SoCoordinate3()
        self.coord1.point.setValue((p2.x,p2.y,p2.z))
        self.coord2 = coin.SoCoordinate3()
        self.coord2.point.setValue((p3.x,p3.y,p3.z))
        marks = coin.SoAnnotation()
        marks.addChild(self.color)
        marks.addChild(self.coord1)
        marks.addChild(dimSymbol())
        marks.addChild(self.coord2)
        marks.addChild(dimSymbol())       
        self.drawstyle = coin.SoDrawStyle()
        self.drawstyle.lineWidth = 1       
        self.line = coin.SoLineSet()
        self.coords = coin.SoCoordinate3()
        selnode=coin.SoType.fromName("SoFCSelection").createInstance()
        selnode.documentName.setValue(FreeCAD.ActiveDocument.Name)
        selnode.objectName.setValue(obj.Object.Name)
        selnode.subElementName.setValue("Line")
        selnode.addChild(self.line)
        self.node = coin.SoGroup()
        self.node.addChild(self.color)
        self.node.addChild(self.drawstyle)
        self.node.addChild(self.coords)
        self.node.addChild(selnode)
        self.node.addChild(marks)
        self.node.addChild(label)
        self.node3d = coin.SoGroup()
        self.node3d.addChild(self.color)
        self.node3d.addChild(self.drawstyle)
        self.node3d.addChild(self.coords)
        self.node3d.addChild(selnode)
        self.node3d.addChild(marks)
        self.node3d.addChild(label3d)
        obj.addDisplayMode(self.node,"2D")
        obj.addDisplayMode(self.node3d,"3D")
        self.onChanged(obj,"FontSize")
        self.onChanged(obj,"FontName")
            
    def updateData(self, obj, prop):
        try:
            dm = obj.ViewObject.DisplayMode
        except:
            dm = "2D"
        text = None
        if obj.Base and obj.LinkedVertices:
            if "Shape" in obj.Base.PropertiesList:
                if len(obj.LinkedVertices) == 3:
                    # arc linked dimension
                    e = obj.Base.Shape.Edges[obj.LinkedVertices[0]]
                    c = e.Curve.Center
                    bray = DraftVecUtils.scaleTo(obj.Dimline.sub(c),e.Curve.Radius)
                    if obj.LinkedVertices[1] == 1:
                        v1 = c
                    else:
                        v1 = c.add(DraftVecUtils.neg(bray))
                    v2 = c.add(bray)
                else:
                    # linear linked dimension
                    v1 = obj.Base.Shape.Vertexes[obj.LinkedVertices[0]].Point
                    v2 = obj.Base.Shape.Vertexes[obj.LinkedVertices[1]].Point
                if v1 != obj.Start: obj.Start = v1
                if v2 != obj.End: obj.End = v2
        p1,p2,p3,p4,tbase,norm,rot = self.calcGeom(obj)
        if 'Override' in obj.ViewObject.PropertiesList:
            text = str(obj.ViewObject.Override)
        dtext = getParam("dimPrecision")
        dtext = "%."+str(dtext)+"f"
        dtext = (dtext % p3.sub(p2).Length)
        if text:
            text = text.replace("dim",dtext)
        else:
            text = dtext
        if hasattr(self,"text"):
            self.text.string = self.text3d.string = text
            self.textpos.rotation = coin.SbRotation(rot[0],rot[1],rot[2],rot[3])
            self.textpos.translation.setValue([tbase.x,tbase.y,tbase.z])
            if dm == "2D":
                self.coords.point.setValues([[p1.x,p1.y,p1.z],
                                             [p2.x,p2.y,p2.z],
                                             [p3.x,p3.y,p3.z],
                                             [p4.x,p4.y,p4.z]])
                self.line.numVertices.setValues([4])
            else:
                ts = (len(text)*obj.ViewObject.FontSize)/4
                rm = ((p3.sub(p2)).Length/2)-ts
                p2a = p2.add(DraftVecUtils.scaleTo(p3.sub(p2),rm))
                p2b = p3.add(DraftVecUtils.scaleTo(p2.sub(p3),rm))
                self.coords.point.setValues([[p1.x,p1.y,p1.z],
                                             [p2.x,p2.y,p2.z],
                                             [p2a.x,p2a.y,p2a.z],
                                             [p2b.x,p2b.y,p2b.z],
                                             [p3.x,p3.y,p3.z],
                                             [p4.x,p4.y,p4.z]])
                self.line.numVertices.setValues([3,3])
            self.coord1.point.setValue((p2.x,p2.y,p2.z))
            self.coord2.point.setValue((p3.x,p3.y,p3.z))

    def onChanged(self, vp, prop):
        self.Object = vp.Object
        if prop == "FontSize":
            self.font.size = vp.FontSize
            self.font3d.size = vp.FontSize*100
        elif prop == "FontName":
            self.font.name = self.font3d.name = str(vp.FontName)
        elif prop == "LineColor":
            c = vp.LineColor
            self.color.rgb.setValue(c[0],c[1],c[2])
        elif prop == "LineWidth":
            self.drawstyle.lineWidth = vp.LineWidth
        else:
            self.drawstyle.lineWidth = vp.LineWidth
            self.updateData(vp.Object, None)

    def getDisplayModes(self,obj):
        return ["2D","3D"]

    def getDefaultDisplayMode(self):
        if hasattr(self,"defaultmode"):
            return self.defaultmode
        else:
            return "2D"

    def setDisplayMode(self,mode):
        return mode

    def getIcon(self):
        if self.Object.Base:
            return """
                /* XPM */
                static char * dim_xpm[] = {
                "16 16 6 1",
                " 	c None",
                ".	c #000000",
                "+	c #FFFF00",
                "@	c #FFFFFF",
                "$	c #141010",
                "#	c #615BD2",
                "        $$$$$$$$",
                "        $##$$#$$",
                "     .  $##$$##$",
                "    ..  $##$$##$",
                "   .+.  $######$",
                "  .++.  $##$$##$",
                " .+++. .$##$$##$",
                ".++++. .$######$",
                " .+++. .$$$$$$$$"
                "  .++.    .++.  ",
                "   .+.    .+.   ",
                "    ..    ..    ",
                "     .    .     ",
                "                ",
                "                ",
                "                "};
                """
        else:
            return """
                /* XPM */
                static char * dim_xpm[] = {
                "16 16 4 1",
                " 	c None",
                ".	c #000000",
                "+	c #FFFF00",
                "@	c #FFFFFF",
                "                ",
                "                ",
                "     .    .     ",
                "    ..    ..    ",
                "   .+.    .+.   ",
                "  .++.    .++.  ",
                " .+++. .. .+++. ",
                ".++++. .. .++++.",
                " .+++. .. .+++. ",
                "  .++.    .++.  ",
                "   .+.    .+.   ",
                "    ..    ..    ",
                "     .    .     ",
                "                ",
                "                ",
                "                "};
                """

    def __getstate__(self):
        return self.Object.ViewObject.DisplayMode
    
    def __setstate__(self,state):
        if state:
            self.defaultmode = state
            self.setDisplayMode(state)

class _AngularDimension:
    "The AngularDimension object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyAngle","FirstAngle","Base",
                        "Start angle of the dimension")
        obj.addProperty("App::PropertyAngle","LastAngle","Base",
                        "End angle of the dimension")
        obj.addProperty("App::PropertyVector","Dimline","Base",
                        "Point through which the dimension line passes")
        obj.addProperty("App::PropertyVector","Center","Base",
                        "The center point of this dimension")
        obj.FirstAngle = 0
        obj.LastAngle = 90
        obj.Dimline = FreeCAD.Vector(0,1,0)
        obj.Center = FreeCAD.Vector(0,0,0)
        obj.Proxy = self
        self.Type = "AngularDimension"
        
    def onChanged(self, fp, prop):
        pass

    def execute(self, fp):
        if fp.ViewObject:
            fp.ViewObject.update()

class _ViewProviderAngularDimension:
    "A View Provider for the Angular Dimension object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyLength","FontSize","Base","Font size")
        obj.addProperty("App::PropertyString","FontName","Base","Font name")
        obj.addProperty("App::PropertyLength","LineWidth","Base","Line width")
        obj.addProperty("App::PropertyColor","LineColor","Base","Line color")
        obj.addProperty("App::PropertyVector","TextPosition","Base","The position of the text. Leave (0,0,0) for automatic position")
        obj.addProperty("App::PropertyString","Override","Base","Text override. Use 'dim' to insert the dimension length")
        obj.Proxy = self
        obj.FontSize=getParam("textheight")
        obj.FontName=getParam("textfont")
        obj.Override = ''

    def attach(self, vobj):
        self.Object = vobj.Object
        self.arc = None
        c,tbase,trot,p2,p3 = self.calcGeom(vobj.Object)
        self.color = coin.SoBaseColor()
        self.color.rgb.setValue(vobj.LineColor[0],
                                vobj.LineColor[1],
                                vobj.LineColor[2])
        self.font = coin.SoFont()
        self.font3d = coin.SoFont()
        self.text = coin.SoAsciiText()
        self.text3d = coin.SoText2()
        self.text.justification = self.text3d.justification = coin.SoAsciiText.CENTER
        self.text.string = self.text3d.string = ''
        self.textpos = coin.SoTransform()
        self.textpos.translation.setValue([tbase.x,tbase.y,tbase.z])
        self.textpos.rotation = coin.SbRotation()
        label = coin.SoSeparator()
        label.addChild(self.textpos)
        label.addChild(self.color)
        label.addChild(self.font)
        label.addChild(self.text)
        label3d = coin.SoSeparator()
        label3d.addChild(self.textpos)
        label3d.addChild(self.color)
        label3d.addChild(self.font3d)
        label3d.addChild(self.text3d)
        self.coord1 = coin.SoCoordinate3()
        self.coord1.point.setValue((p2.x,p2.y,p2.z))
        self.coord2 = coin.SoCoordinate3()
        self.coord2.point.setValue((p3.x,p3.y,p3.z))
        marks = coin.SoAnnotation()
        marks.addChild(self.color)
        marks.addChild(self.coord1)
        marks.addChild(dimSymbol())
        marks.addChild(self.coord2)
        marks.addChild(dimSymbol())  
        self.drawstyle = coin.SoDrawStyle()
        self.drawstyle.lineWidth = 1       
        self.coords = coin.SoCoordinate3()
        self.selnode=coin.SoType.fromName("SoFCSelection").createInstance()
        self.selnode.documentName.setValue(FreeCAD.ActiveDocument.Name)
        self.selnode.objectName.setValue(vobj.Object.Name)
        self.selnode.subElementName.setValue("Arc")
        self.node = coin.SoGroup()
        self.node.addChild(self.color)
        self.node.addChild(self.drawstyle)
        self.node.addChild(self.coords)
        self.node.addChild(self.selnode)
        self.node.addChild(marks)
        self.node.addChild(label)
        self.node3d = coin.SoGroup()
        self.node3d.addChild(self.color)
        self.node3d.addChild(self.drawstyle)
        self.node3d.addChild(self.coords)
        self.node3d.addChild(self.selnode)
        self.node3d.addChild(marks)
        self.node3d.addChild(label3d)
        vobj.addDisplayMode(self.node,"2D")
        vobj.addDisplayMode(self.node3d,"3D")
        self.onChanged(vobj,"FontSize")
        self.onChanged(vobj,"FontName")

    def calcGeom(self,obj):
        import Part, DraftGeomUtils
        rad = (obj.Dimline.sub(obj.Center)).Length
        cir = Part.makeCircle(rad,obj.Center,Vector(0,0,1),obj.FirstAngle,obj.LastAngle)
        cp = DraftGeomUtils.findMidpoint(cir.Edges[0])
        rv = cp.sub(obj.Center)
        rv = DraftVecUtils.scaleTo(rv,rv.Length + obj.ViewObject.FontSize*.2)
        tbase = obj.Center.add(rv)
        trot = DraftVecUtils.angle(rv)-math.pi/2
        if (trot > math.pi/2) or (trot < -math.pi/2):
            trot = trot + math.pi
        s = getParam("dimorientation")
        if s == 0:
            if round(trot,precision()) == round(-math.pi/2,precision()):
                trot = math.pi/2
        return cir, tbase, trot, cir.Vertexes[0].Point, cir.Vertexes[-1].Point

    def updateData(self, obj, prop):
        text = None
        ivob = None
        c,tbase,trot,p2,p3 = self.calcGeom(obj)
        buf=c.writeInventor(2,0.01)
        ivin = coin.SoInput()
        ivin.setBuffer(buf)
        ivob = coin.SoDB.readAll(ivin)
        arc = ivob.getChildren()[1]
        # In case reading from buffer failed
        if ivob and ivob.getNumChildren() > 1:
            arc = ivob.getChild(1).getChild(0)
            arc.removeChild(arc.getChild(0))
            arc.removeChild(arc.getChild(0))                
        if self.arc:
            self.selnode.removeChild(self.arc)
        self.arc = arc
        self.selnode.addChild(self.arc)
        if 'Override' in obj.ViewObject.PropertiesList:
            text = str(obj.ViewObject.Override)
        dtext = getParam("dimPrecision")
        dtext = "%."+str(dtext)+"f"
        if obj.LastAngle > obj.FirstAngle:
            dtext = (dtext % (obj.LastAngle-obj.FirstAngle))+'\xb0'
        else:
            dtext = (dtext % ((360-obj.FirstAngle)+obj.LastAngle))+'\xb0'
        if text:
            text = text.replace("dim",dtext)
        else:
            text = dtext
        self.text.string = self.text3d.string = text
        self.textpos.translation.setValue([tbase.x,tbase.y,tbase.z])
        m = FreeCAD.Matrix()
        m.rotateZ(trot)
        tm = FreeCAD.Placement(m).Rotation.Q
        self.textpos.rotation = coin.SbRotation(tm[0],tm[1],tm[2],tm[3])
        self.coord1.point.setValue((p2.x,p2.y,p2.z))
        self.coord2.point.setValue((p3.x,p3.y,p3.z))

    def onChanged(self, vobj, prop):
        if prop == "FontSize":
            self.font.size = vobj.FontSize
            self.font3d.size = vobj.FontSize*100
        elif prop == "FontName":
            self.font.name = self.font3d.name = str(vobj.FontName)
        elif prop == "LineColor":
            c = vobj.LineColor
            self.color.rgb.setValue(c[0],c[1],c[2])
        elif prop == "LineWidth":
            self.drawstyle.lineWidth = vobj.LineWidth
        elif prop == "DisplayMode":
            pass
        else:
            self.updateData(vobj.Object, None)

    def getDisplayModes(self,obj):
        modes=[]
        modes.extend(["2D","3D"])
        return modes

    def getDefaultDisplayMode(self):
        return "2D"

    def getIcon(self):
        return """
                        /* XPM */
                        static char * dim_xpm[] = {
                        "16 16 4 1",
                        " 	c None",
                        ".	c #000000",
                        "+	c #FFFF00",
                        "@	c #FFFFFF",
                        "                ",
                        "                ",
                        "     .    .     ",
                        "    ..    ..    ",
                        "   .+.    .+.   ",
                        "  .++.    .++.  ",
                        " .+++. .. .+++. ",
                        ".++++. .. .++++.",
                        " .+++. .. .+++. ",
                        "  .++.    .++.  ",
                        "   .+.    .+.   ",
                        "    ..    ..    ",
                        "     .    .     ",
                        "                ",
                        "                ",
                        "                "};
                        """

class _Rectangle:
    "The Rectangle object"
        
    def __init__(self, obj):
        obj.addProperty("App::PropertyDistance","Length","Base","Length of the rectangle")
        obj.addProperty("App::PropertyDistance","Height","Base","Height of the rectange")
        obj.addProperty("App::PropertyDistance","FilletRadius","Base","Radius to use to fillet the corners")
        obj.Proxy = self
        obj.Length=1
        obj.Height=1
        self.Type = "Rectangle"

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        if prop in ["Length","Height"]:
            self.createGeometry(fp)
                        
    def createGeometry(self,fp):
        import Part, DraftGeomUtils
        plm = fp.Placement
        p1 = Vector(0,0,0)
        p2 = Vector(p1.x+fp.Length,p1.y,p1.z)
        p3 = Vector(p1.x+fp.Length,p1.y+fp.Height,p1.z)
        p4 = Vector(p1.x,p1.y+fp.Height,p1.z)
        shape = Part.makePolygon([p1,p2,p3,p4,p1])
        if "FilletRadius" in fp.PropertiesList:
            if fp.FilletRadius != 0:
                w = DraftGeomUtils.filletWire(shape,fp.FilletRadius)
                if w:
                    shape = w
        shape = Part.Face(shape)
        fp.Shape = shape
        fp.Placement = plm

class _ViewProviderRectangle(_ViewProviderDraft):
    "A View Provider for the Rectangle object"
    def __init__(self, vobj):
        _ViewProviderDraft.__init__(self,vobj)
        vobj.addProperty("App::PropertyFile","TextureImage",
                        "Base","Uses an image as a texture map")

    def attach(self,vobj):
        self.texture = None
        self.Object = vobj.Object

    def onChanged(self, vp, prop):
        if prop == "TextureImage":
            r = vp.RootNode
            if os.path.exists(vp.TextureImage):
                im = loadTexture(vp.TextureImage)
                if im:
                    self.texture = coin.SoTexture2()
                    self.texture.image = im
                    r.insertChild(self.texture,1)
            else:
                if self.texture:
                    r.removeChild(self.texture)
                    self.texture = None
        return
        
class _Circle:
    "The Circle object"
        
    def __init__(self, obj):
        obj.addProperty("App::PropertyAngle","FirstAngle","Base",
                        "Start angle of the arc")
        obj.addProperty("App::PropertyAngle","LastAngle","Base",
                        "End angle of the arc (for a full circle, give it same value as First Angle)")
        obj.addProperty("App::PropertyDistance","Radius","Base",
                        "Radius of the circle")
        obj.Proxy = self
        self.Type = "Circle"

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        if prop in ["Radius","FirstAngle","LastAngle"]:
            self.createGeometry(fp)
                        
    def createGeometry(self,fp):
        import Part
        plm = fp.Placement
        shape = Part.makeCircle(fp.Radius,Vector(0,0,0),
                                Vector(0,0,1),fp.FirstAngle,fp.LastAngle)
        if fp.FirstAngle == fp.LastAngle:
            shape = Part.Wire(shape)
            shape = Part.Face(shape)
        fp.Shape = shape
        fp.Placement = plm

class _Wire:
    "The Wire object"
        
    def __init__(self, obj):
        obj.addProperty("App::PropertyVectorList","Points","Base",
                        "The vertices of the wire")
        obj.addProperty("App::PropertyBool","Closed","Base",
                        "If the wire is closed or not")
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object is the wire is formed from 2 objects")
        obj.addProperty("App::PropertyLink","Tool","Base",
                        "The tool object is the wire is formed from 2 objects")
        obj.addProperty("App::PropertyVector","Start","Base",
                        "The start point of this line")
        obj.addProperty("App::PropertyVector","End","Base",
                        "The end point of this line")
        obj.addProperty("App::PropertyDistance","FilletRadius","Base","Radius to use to fillet the corners")
        obj.Proxy = self
        obj.Closed = False
        self.Type = "Wire"

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        if prop in ["Points","Closed","Base","Tool","FilletRadius"]:
            self.createGeometry(fp)
            if prop == "Points":
                if fp.Start != fp.Points[0]:
                    fp.Start = fp.Points[0]
                if fp.End != fp.Points[-1]:
                    fp.End = fp.Points[-1]
                if len(fp.Points) > 2:
                    fp.setEditorMode('Start',2)
                    fp.setEditorMode('End',2)
        elif prop == "Start":
            pts = fp.Points
            if pts:
                if pts[0] != fp.Start:
                    pts[0] = fp.Start
                    fp.Points = pts
        elif prop == "End":
            pts = fp.Points
            if len(pts) > 1:
                if pts[-1] != fp.End:
                    pts[-1] = fp.End
                    fp.Points = pts
                        
    def createGeometry(self,fp):
        import Part, DraftGeomUtils
        plm = fp.Placement
        if fp.Base and (not fp.Tool):
            if fp.Base.isDerivedFrom("Sketcher::SketchObject"):
                shape = fp.Base.Shape.copy()
                if fp.Base.Shape.isClosed():
                    shape = Part.Face(shape)
                fp.Shape = shape
                p = []
                for v in shape.Vertexes: p.append(v.Point)
                if fp.Points != p: fp.Points = p
        elif fp.Base and fp.Tool:
            if fp.Base.isDerivedFrom("Part::Feature") and fp.Tool.isDerivedFrom("Part::Feature"):
                if (not fp.Base.Shape.isNull()) and (not fp.Tool.Shape.isNull()):
                    sh1 = fp.Base.Shape.copy()
                    sh2 = fp.Tool.Shape.copy()
                    shape = sh1.fuse(sh2)
                    if DraftGeomUtils.isCoplanar(shape.Faces):
                        shape = DraftGeomUtils.concatenate(shape)
                        fp.Shape = shape
                        p = []
                        for v in shape.Vertexes: p.append(v.Point)
                        if fp.Points != p: fp.Points = p
        elif fp.Points:
            if fp.Points[0] == fp.Points[-1]:
                if not fp.Closed: fp.Closed = True
                fp.Points.pop()
            if fp.Closed and (len(fp.Points) > 2):
                shape = Part.makePolygon(fp.Points+[fp.Points[0]])
                if "FilletRadius" in fp.PropertiesList:
                    if fp.FilletRadius != 0:
                        w = DraftGeomUtils.filletWire(shape,fp.FilletRadius)
                        if w:
                            shape = w
                shape = Part.Face(shape)
            else:
                edges = []
                pts = fp.Points[1:]
                lp = fp.Points[0]
                for p in pts:
                    edges.append(Part.Line(lp,p).toShape())
                    lp = p
                shape = Part.Wire(edges)
                if "FilletRadius" in fp.PropertiesList:
                    if fp.FilletRadius != 0:
                        w = DraftGeomUtils.filletWire(shape,fp.FilletRadius)
                        if w:
                            shape = w
            fp.Shape = shape
        fp.Placement = plm

class _ViewProviderWire(_ViewProviderDraft):
    "A View Provider for the Wire object"
    def __init__(self, obj):
        _ViewProviderDraft.__init__(self,obj)
        obj.addProperty("App::PropertyBool","EndArrow","Base",
                        "Displays a dim symbol at the end of the wire")

    def attach(self, obj):
        self.Object = obj.Object
        col = coin.SoBaseColor()
        col.rgb.setValue(obj.LineColor[0],
                         obj.LineColor[1],
                         obj.LineColor[2])
        self.coords = coin.SoCoordinate3()
        self.pt = coin.SoAnnotation()
        self.pt.addChild(col)
        self.pt.addChild(self.coords)
        self.pt.addChild(dimSymbol())
        
    def updateData(self, obj, prop):
        if prop == "Points":
            if obj.Points:
                p = obj.Points[-1]
                self.coords.point.setValue((p.x,p.y,p.z))
        return

    def onChanged(self, vp, prop):
        if prop == "EndArrow":
            rn = vp.RootNode
            if vp.EndArrow:
                rn.addChild(self.pt)
            else:
                rn.removeChild(self.pt)
        return

    def claimChildren(self):
        return [self.Object.Base,self.Object.Tool]
        
class _Polygon:
    "The Polygon object"
        
    def __init__(self, obj):
        obj.addProperty("App::PropertyInteger","FacesNumber","Base","Number of faces")
        obj.addProperty("App::PropertyDistance","Radius","Base","Radius of the control circle")
        obj.addProperty("App::PropertyEnumeration","DrawMode","Base","How the polygon must be drawn from the control circle")
        obj.addProperty("App::PropertyDistance","FilletRadius","Base","Radius to use to fillet the corners")
        obj.DrawMode = ['inscribed','circumscribed']
        obj.FacesNumber = 3
        obj.Radius = 1
        obj.Proxy = self
        self.Type = "Polygon"

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        if prop in ["FacesNumber","Radius","DrawMode"]:
            self.createGeometry(fp)
                        
    def createGeometry(self,fp):
        import Part, DraftGeomUtils
        plm = fp.Placement
        angle = (math.pi*2)/fp.FacesNumber
        if fp.DrawMode == 'inscribed':
            delta = fp.Radius
        else:
            delta = fp.Radius/math.cos(angle/2)
        pts = [Vector(delta,0,0)]
        for i in range(fp.FacesNumber-1):
            ang = (i+1)*angle
            pts.append(Vector(delta*math.cos(ang),delta*math.sin(ang),0))
        pts.append(pts[0])
        shape = Part.makePolygon(pts)
        if "FilletRadius" in fp.PropertiesList:
            if fp.FilletRadius != 0:
                w = DraftGeomUtils.filletWire(shape,fp.FilletRadius)
                if w:
                    shape = w
        shape = Part.Face(shape)
        fp.Shape = shape
        fp.Placement = plm

class _DrawingView:
    def __init__(self, obj):
        obj.addProperty("App::PropertyVector","Direction","Shape View","Projection direction")
        obj.addProperty("App::PropertyFloat","LineWidth","Drawing View","The width of the lines inside this object")
        obj.addProperty("App::PropertyFloat","FontSize","Drawing View","The size of the texts inside this object")
        obj.addProperty("App::PropertyLink","Source","Base","The linked object")
        obj.addProperty("App::PropertyEnumeration","FillStyle","Drawing View","Shape Fill Style")
        fills = ['shape color']
        for f in FreeCAD.svgpatterns.keys():
            fills.append(f)
        obj.FillStyle = fills
        obj.Proxy = self
        obj.LineWidth = 0.35
        obj.FontSize = 12
        self.Type = "DrawingView"

    def execute(self, obj):
        if obj.Source:
            obj.ViewResult = self.updateSVG(obj)

    def onChanged(self, obj, prop):
        if prop in ["X","Y","Scale","LineWidth","FontSize","FillStyle","Direction"]:
            obj.ViewResult = self.updateSVG(obj)

    def updateSVG(self, obj):
        "encapsulates a svg fragment into a transformation node"
        svg = getSVG(obj.Source,obj.Scale,obj.LineWidth,obj.FontSize,obj.FillStyle,obj.Direction)
        result = ''
        result += '<g id="' + obj.Name + '"'
        result += ' transform="'
        result += 'rotate('+str(obj.Rotation)+','+str(obj.X)+','+str(obj.Y)+') '
        result += 'translate('+str(obj.X)+','+str(obj.Y)+') '
        result += 'scale('+str(obj.Scale)+','+str(-obj.Scale)+')'
        result += '">'
        result += svg
        result += '</g>'
        return result

class _BSpline:
    "The BSpline object"
        
    def __init__(self, obj):
        obj.addProperty("App::PropertyVectorList","Points","Base",
                        "The points of the b-spline")
        obj.addProperty("App::PropertyBool","Closed","Base",
                        "If the b-spline is closed or not")
        obj.Proxy = self
        obj.Closed = False
        self.Type = "BSpline"

    def execute(self, fp):
        self.createGeometry(fp)
        
    def onChanged(self, fp, prop):
        if prop in ["Points","Closed"]:
            self.createGeometry(fp)
                        
    def createGeometry(self,fp):
        import Part
        plm = fp.Placement
        if fp.Points:
            if fp.Points[0] == fp.Points[-1]:
                if not fp.Closed: fp.Closed = True
                fp.Points.pop()
            if fp.Closed and (len(fp.Points) > 2):
                spline = Part.BSplineCurve()
                spline.interpolate(fp.Points, True)
                # DNC: bug fix: convert to face if closed
                shape = Part.Wire(spline.toShape())
                shape = Part.Face(shape)
                fp.Shape = shape
            else:   
                spline = Part.BSplineCurve()
                spline.interpolate(fp.Points, False)
                fp.Shape = spline.toShape()
        fp.Placement = plm

class _ViewProviderBSpline(_ViewProviderDraft):
    "A View Provider for the BSPline object"
    def __init__(self, obj):
        _ViewProviderDraft.__init__(self,obj)
        obj.addProperty("App::PropertyBool","EndArrow",
                        "Base","Displays a dim symbol at the end of the wire")
        col = coin.SoBaseColor()
        col.rgb.setValue(obj.LineColor[0],
                         obj.LineColor[1],
                         obj.LineColor[2])
        self.coords = coin.SoCoordinate3()
        self.pt = coin.SoAnnotation()
        self.pt.addChild(col)
        self.pt.addChild(self.coords)
        self.pt.addChild(dimSymbol())
        
    def updateData(self, obj, prop):
        if prop == "Points":
            if obj.Points:
                p = obj.Points[-1]
                if hasattr(self,"coords"):
                    self.coords.point.setValue((p.x,p.y,p.z))
        return

    def onChanged(self, vp, prop):
        if prop == "EndArrow":
            rn = vp.RootNode
            if vp.EndArrow:
                rn.addChild(self.pt)
            else:
                rn.removeChild(self.pt)
        return

class _Block:
    "The Block object"
    
    def __init__(self, obj):
        obj.addProperty("App::PropertyLinkList","Components","Base",
                        "The components of this block")
        obj.Proxy = self
        self.Type = "Block"

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        if prop in ["Components"]:
            self.createGeometry(fp)
                        
    def createGeometry(self,fp):
        import Part
        plm = fp.Placement
        shps = []
        for c in fp.Components:
            shps.append(c.Shape)
        if shps:
            shape = Part.makeCompound(shps)
            fp.Shape = shape
        fp.Placement = plm

class _Shape2DView:
    "The Shape2DView object"

    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object this 2D view must represent")
        obj.addProperty("App::PropertyVector","Projection","Base",
                        "The projection vector of this object")
        obj.Projection = Vector(0,0,-1)
        obj.Proxy = self
        self.Type = "2DShapeView"

    def execute(self,obj):
        self.createGeometry(obj)

    def onChanged(self,obj,prop):
        if prop in ["Projection","Base"]:
            self.createGeometry(obj)

    def createGeometry(self,obj):
        import Drawing, DraftGeomUtils
        pl = obj.Placement
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if not DraftVecUtils.isNull(obj.Projection):
                    [visibleG0,visibleG1,hiddenG0,hiddenG1] = Drawing.project(obj.Base.Shape,obj.Projection)
                    if visibleG0:
                        obj.Shape = visibleG0
        if not DraftGeomUtils.isNull(pl):
            obj.Placement = pl

class _Array:
    "The Draft Array object"

    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object that must be duplicated")
        obj.addProperty("App::PropertyEnumeration","ArrayType","Base",
                        "The type of array to create")
        obj.addProperty("App::PropertyVector","Axis","Base",
                        "The axis direction")
        obj.addProperty("App::PropertyInteger","NumberX","Base",
                        "Number of copies in X direction")
        obj.addProperty("App::PropertyInteger","NumberY","Base",
                        "Number of copies in Y direction")
        obj.addProperty("App::PropertyInteger","NumberZ","Base",
                        "Number of copies in Z direction")
        obj.addProperty("App::PropertyInteger","NumberPolar","Base",
                        "Number of copies")
        obj.addProperty("App::PropertyVector","IntervalX","Base",
                        "Distance and orientation of intervals in X direction")
        obj.addProperty("App::PropertyVector","IntervalY","Base",
                        "Distance and orientation of intervals in Y direction")
        obj.addProperty("App::PropertyVector","IntervalZ","Base",
                        "Distance and orientation of intervals in Z direction")
        obj.addProperty("App::PropertyVector","Center","Base",
                        "Center point")
        obj.addProperty("App::PropertyAngle","Angle","Base",
                        "Angle to cover with copies")
        obj.Proxy = self
        self.Type = "Array"
        obj.ArrayType = ['ortho','polar']
        obj.NumberX = 1
        obj.NumberY = 1
        obj.NumberZ = 1
        obj.NumberPolar = 1
        obj.IntervalX = Vector(1,0,0)
        obj.IntervalY = Vector(0,1,0)
        obj.IntervalZ = Vector(0,0,1)
        obj.Angle = 360
        obj.Axis = Vector(0,0,1)

    def execute(self,obj):
        self.createGeometry(obj)

    def onChanged(self,obj,prop):
        if prop == "ArrayType":
            if obj.ViewObject:
                if obj.ArrayType == "ortho":
                    obj.ViewObject.setEditorMode('Axis',2)
                    obj.ViewObject.setEditorMode('NumberPolar',2)
                    obj.ViewObject.setEditorMode('Center',2)
                    obj.ViewObject.setEditorMode('Angle',2)
                    obj.ViewObject.setEditorMode('NumberX',0)
                    obj.ViewObject.setEditorMode('NumberY',0)
                    obj.ViewObject.setEditorMode('NumberZ',0)
                    obj.ViewObject.setEditorMode('IntervalX',0)
                    obj.ViewObject.setEditorMode('IntervalY',0)
                    obj.ViewObject.setEditorMode('IntervalZ',0)
                else:
                    obj.ViewObject.setEditorMode('Axis',0)
                    obj.ViewObject.setEditorMode('NumberPolar',0)
                    obj.ViewObject.setEditorMode('Center',0)
                    obj.ViewObject.setEditorMode('Angle',0)
                    obj.ViewObject.setEditorMode('NumberX',2)
                    obj.ViewObject.setEditorMode('NumberY',2)
                    obj.ViewObject.setEditorMode('NumberY',2)
                    obj.ViewObject.setEditorMode('IntervalX',2)
                    obj.ViewObject.setEditorMode('IntervalY',2)
                    obj.ViewObject.setEditorMode('IntervalZ',2)              
        if prop in ["ArrayType","NumberX","NumberY","NumberZ","NumberPolar",
                    "IntervalX","IntervalY","IntervalZ","Angle","Center","Axis"]:
            self.createGeometry(obj)
            
    def createGeometry(self,obj):
        import DraftGeomUtils
        if obj.Base:
            pl = obj.Placement
            if obj.ArrayType == "ortho":
                sh = self.rectArray(obj.Base.Shape,obj.IntervalX,obj.IntervalY,
                                    obj.IntervalZ,obj.NumberX,obj.NumberY,obj.NumberZ)
            else:
                sh = self.polarArray(obj.Base.Shape,obj.Center,obj.Angle,obj.NumberPolar,obj.Axis)
            obj.Shape = sh
            if not DraftGeomUtils.isNull(pl):
                obj.Placement = pl

    def rectArray(self,shape,xvector,yvector,zvector,xnum,ynum,znum):
        import Part
        base = [shape.copy()]
        for xcount in range(xnum):
            currentxvector=DraftVecUtils.scale(xvector,xcount)
            if not xcount==0:
                nshape = shape.copy()
                nshape.translate(currentxvector)
                base.append(nshape)
            for ycount in range(ynum):
                currentyvector=FreeCAD.Vector(currentxvector)
                currentyvector=currentyvector.add(DraftVecUtils.scale(yvector,ycount))
                if not ycount==0:
                    nshape = shape.copy()
                    nshape.translate(currentyvector)
                    base.append(nshape)
                for zcount in range(znum):
                    currentzvector=FreeCAD.Vector(currentyvector)
                    currentzvector=currentzvector.add(DraftVecUtils.scale(zvector,zcount))
                    if not zcount==0:
                        nshape = shape.copy()
                        nshape.translate(currentzvector)
                        base.append(nshape)
        return Part.makeCompound(base)

    def polarArray(self,shape,center,angle,num,axis):
        import Part
        fraction = angle/num
        base = [shape.copy()]
        for i in range(num):
            currangle = fraction + (i*fraction)
            nshape = shape.copy()
            nshape.rotate(DraftVecUtils.tup(center), DraftVecUtils.tup(axis), currangle)
            base.append(nshape)
        return Part.makeCompound(base)

class _Point:
    def __init__(self, obj,x,y,z):
        obj.addProperty("App::PropertyFloat","X","Point","Location").X = x
        obj.addProperty("App::PropertyFloat","Y","Point","Location").Y = y
        obj.addProperty("App::PropertyFloat","Z","Point","Location").Z = z
        mode = 2
        obj.setEditorMode('Placement',mode)
        obj.Proxy = self
        self.Type = "Point"

    def execute(self, fp):
        self.createGeometry(fp)

    def createGeometry(self,fp):
        import Part
        shape = Part.Vertex(Vector(fp.X,fp.Y,fp.Z))
        fp.Shape = shape

class _ViewProviderPoint:
    def __init__(self, obj):
        obj.Proxy = self

    def onChanged(self, vp, prop):
        mode = 2
        vp.setEditorMode('LineColor',mode)
        vp.setEditorMode('LineWidth',mode)
        vp.setEditorMode('BoundingBox',mode)
        vp.setEditorMode('ControlPoints',mode)
        vp.setEditorMode('Deviation',mode)
        vp.setEditorMode('DiffuseColor',mode)
        vp.setEditorMode('DisplayMode',mode)
        vp.setEditorMode('Lighting',mode)
        vp.setEditorMode('LineMaterial',mode)
        vp.setEditorMode('ShapeColor',mode)
        vp.setEditorMode('ShapeMaterial',mode)
        vp.setEditorMode('Transparency',mode)

    def getIcon(self):
        return ":/icons/Draft_Dot.svg"

class _Clone:
    "The Clone object"

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkList","Objects","Base",
                        "The objects included in this scale object")
        obj.addProperty("App::PropertyVector","Scale","Base",
                        "The scale vector of this object")
        obj.Scale = Vector(1,1,1)
        obj.Proxy = self
        self.Type = "Clone"

    def execute(self,obj):
        self.createGeometry(obj)

    def onChanged(self,obj,prop):
        if prop in ["Scale","Objects"]:
            self.createGeometry(obj)

    def createGeometry(self,obj):
        import Part, DraftGeomUtils
        pl = obj.Placement
        shapes = []
        for o in obj.Objects:
            if o.isDerivedFrom("Part::Feature"):
                sh = o.Shape.copy()
                m = FreeCAD.Matrix()
                if hasattr(obj,"Scale") and not sh.isNull():
                    m.scale(obj.Scale)
                    sh = sh.transformGeometry(m)
                if not sh.isNull():
                    shapes.append(sh)
        if shapes:
            if len(shapes) == 1:
                obj.Shape = shapes[0]
            else:
                obj.Shape = Part.makeCompound(shapes)   
        if not DraftGeomUtils.isNull(pl):
            obj.Placement = pl

class _ViewProviderDraftPart(_ViewProviderDraft):
    "a view provider that displays a Part icon instead of a Draft icon"
    
    def __init__(self,vobj):
        _ViewProviderDraft.__init__(self,vobj)

    def getIcon(self):
        return ":/icons/Tree_Part.svg"

    def claimChildren(self):
        return []

class _ViewProviderClone(_ViewProviderDraft):
    "a view provider that displays a Part icon instead of a Draft icon"
    
    def __init__(self,vobj):
        _ViewProviderDraft.__init__(self,vobj)

    def getIcon(self):
        return ":/icons/Draft_Clone.svg"

    def claimChildren(self):
        return []
    
if not hasattr(FreeCADGui,"Snapper"):
    import DraftSnap
