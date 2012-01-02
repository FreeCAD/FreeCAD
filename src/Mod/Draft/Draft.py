#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009, 2010                                              *  
#*   Yorik van Havre <yorik@uncreated.net>, Ken Cline <cline@frii.com>     *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
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
__author__ = "Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, Dmitry Chigrin"
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
    - draftlibs/fcvec.py: a vector math library, contains functions that are not
    implemented in the standard FreeCAD vector
    - draftlibs/fcgeo.py: a library of misc functions to manipulate shapes.
        
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

import FreeCAD, FreeCADGui, math, sys, os, WorkingPlane
from FreeCAD import Vector
from draftlibs import fcvec
from pivy import coin

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
    elif param in ["constructiongroupname","textfont","patternFile","template","maxSnapEdges"]:
        return "string"
    elif param in ["textheight","tolerance","gridSpacing"]:
        return "float"
    elif param in ["selectBaseObjects","alwaysSnap","grid","fillmode","saveonexit","maxSnap"]:
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
    if obj.isDerivedFrom("Part::Feature"):
        return "Part"
    if (obj.Type == "App::Annotation"):
        return "Annotation"
    if obj.isDerivedFrom("Mesh::Feature"):
        return "Mesh"
    if (obj.Type == "App::DocumentObjectGroup"):
        return "Group"
    return "Unknown"

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
    if obj.Type == "Part::Feature": return obj
    shape = obj.Shape
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
        if obj.Type == "App::DocumentObjectGroup":
            newlist.extend(getGroupContents(obj.Group))
        else:
            newlist.append(obj)
    return newlist

def formatObject(target,origin=None):
    '''
    formatObject(targetObject,[originObject]): This function applies
    to the given target object the current properties 
    set on the toolbar (line color and line width),
    or copies the properties of another object if given as origin.
    It also places the object in construction group if needed.
    '''
    obrep = target.ViewObject
    ui = FreeCADGui.draftToolBar
    doc = FreeCAD.ActiveDocument
    if ui.isConstructionMode():
        col = fcol = ui.getDefaultColor("constr")
        gname = getParam("constructiongroupname")
        if gname:
            grp = doc.getObject(gname)
            if not grp: grp = doc.addObject("App::DocumentObjectGroup",gname) 
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
        if ("LineWidth" in obrep.PropertiesList) and \
                ("LineWidth" in matchrep.PropertiesList):
            obrep.LineWidth = matchrep.LineWidth
        if ("PointColor" in obrep.PropertiesList) and \
                ("PointColor" in matchrep.PropertiesList):
            obrep.PointColor = matchrep.PointColor
        if ("LineColor" in obrep.PropertiesList) and \
                ("LineColor" in matchrep.PropertiesList):
            obrep.LineColor = matchrep.LineColor
        if ("ShapeColor" in obrep.PropertiesList) and \
                ("ShapeColor" in matchrep.PropertiesList):
            obrep.ShapeColor = matchrep.ShapeColor
        if matchrep.DisplayMode in obrep.listDisplayModes():
            obrep.DisplayMode = matchrep.DisplayMode

def getSelection():
    "getSelection(): returns the current FreeCAD selection"
    return FreeCADGui.Selection.getSelection()

def select(objs):
    "select(object): deselects everything and selects only the passed object or list"
    FreeCADGui.Selection.clearSelection()
    if not isinstance(objs,list):
        objs = [objs]
    for obj in objs:
        FreeCADGui.Selection.addSelection(obj)

def makeCircle(radius, placement=None, face=True, startangle=None, endangle=None, support=None):
    '''makeCircle(radius,[placement,face,startangle,endangle]): Creates a circle
    object with given radius. If placement is given, it is
    used. If face is False, the circle is shown as a
    wireframe, otherwise as a face. If startangle AND endangle are given
    (in degrees), they are used and the object appears as an arc.'''
    if placement: typecheck([(placement,FreeCAD.Placement)], "makeCircle")
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Circle")
    _Circle(obj)
    _ViewProviderDraft(obj.ViewObject)
    obj.Radius = radius
    if not face: obj.ViewObject.DisplayMode = "Wireframe"
    if (startangle != None) and (endangle != None):
        if startangle == -0: startangle = 0
        obj.FirstAngle = startangle
        obj.LastAngle = endangle
    obj.Support = support
    if placement: obj.Placement = placement
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
    _ViewProviderRectangle(obj.ViewObject)
    obj.Length = length
    obj.Height = height
    obj.Support = support
    if not face: obj.ViewObject.DisplayMode = "Wireframe"
    if placement: obj.Placement = placement
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
    _ViewProviderDimension(obj.ViewObject)
    if isinstance(p1,Vector) and isinstance(p2,Vector):
        obj.Start = p1
        obj.End = p2
    elif isinstance(p2,int) and isinstance(p3,int):
        obj.Base = p1
        obj.LinkedVertices = [p2,p3]
        p3 = p4
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
        p3 = p2.sub(p1)
        p3.multiply(0.5)
        p3 = p1.add(p3)
    obj.Dimline = p3
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
    _ViewProviderAngularDimension(obj.ViewObject)
    obj.Center = center
    for a in range(len(angles)):
        if angles[a] > 2*math.pi:
            angles[a] = angles[a]-(2*math.pi)
    obj.FirstAngle = math.degrees(angles[1])
    obj.LastAngle = math.degrees(angles[0])
    obj.Dimline = p3
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
    from draftlibs import fcgeo
    if not isinstance(pointslist,list):
        nlist = []
        for v in pointslist.Vertexes:
            nlist.append(v.Point)
        if fcgeo.isReallyClosed(pointslist):
            nlist.append(pointslist.Vertexes[0].Point)
        pointslist = nlist
    if placement: typecheck([(placement,FreeCAD.Placement)], "makeWire")
    if len(pointslist) == 2: fname = "Line"
    else: fname = "Wire"
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",fname)
    _Wire(obj)
    _ViewProviderWire(obj.ViewObject)
    obj.Points = pointslist
    obj.Closed = closed
    obj.Support = support
    if not face: obj.ViewObject.DisplayMode = "Wireframe"
    if placement: obj.Placement = placement
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
    _ViewProviderDraft(obj.ViewObject)
    obj.FacesNumber = nfaces
    obj.Radius = radius
    if inscribed:
        obj.DrawMode = "inscribed"
    else:
        obj.DrawMode = "circumscribed"
    if not face: obj.ViewObject.DisplayMode = "Wireframe"
    obj.Support = support
    if placement: obj.Placement = placement
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
    _ViewProviderBSpline(obj.ViewObject)
    obj.Points = pointslist
    obj.Closed = closed
    obj.Support = support
    if not face: obj.ViewObject.DisplayMode = "Wireframe"
    if placement: obj.Placement = placement
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

def makeCopy(obj):
    '''makeCopy(object): returns an exact copy of an object'''
    newobj = FreeCAD.ActiveDocument.addObject(obj.Type,getRealName(obj.Name))
    if getType(obj) == "Rectangle":
        _Rectangle(newobj)
        _ViewProviderRectangle(newobj.ViewObject)
    elif getType(obj) == "Wire":
        _Wire(newobj)
        _ViewProviderWire(newobj.ViewObject)
    elif getType(obj) == "Circle":
        _Circle(newobj)
        _ViewProviderCircle(newobj.ViewObject)
    elif getType(obj) == "Polygon":
        _Polygon(newobj)
        _ViewProviderPolygon(newobj.ViewObject)
    elif getType(obj) == "BSpline":
        _BSpline(newobj)
        _ViewProviderBSpline(newobj.ViewObject)
    elif getType(obj) == "Block":
        _Block(newobj)
        _ViewProviderBlock(newobj.ViewObject)
    elif getType(obj) == "Structure":
        import Structure
        Structure._Structure(newobj)
        Structure._ViewProviderStructure(newobj.ViewObject)
    elif getType(obj) == "Wall":
        import Wall
        Wall._Wall(newobj)
        Wall._ViewProviderWall(newobj.ViewObject)
    elif obj.isDerivedFrom("Part::Feature"):
        newobj.Shape = obj.Shape
    else:
        print "Error: Object type cannot be copied"
        return None
    for p in obj.PropertiesList:
        if p in newobj.PropertiesList:
            setattr(newobj,p,obj.getPropertyByName(p))
    formatObject(newobj,obj)
    return newobj

def makeBlock(objectslist):
    '''makeBlock(objectslist): Creates a Draft Block from the given objects'''
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Block")
    _Block(obj)
    _ViewProviderBlock(obj.ViewObject)
    obj.Components = objectslist
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
    _ViewProviderDraft(obj.ViewObject)
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
    from draftlibs import fcgeo
    if fcgeo.isCoplanar(object1.Shape.fuse(object2.Shape).Faces):
        obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Fusion")
        _Wire(obj)
        _ViewProviderWire(obj.ViewObject)
    else:
        obj = FreeCAD.ActiveDocument.addObject("Part::Fuse","Fusion")
    obj.Base = object1
    obj.Tool = object2
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
                _DimensionViewProvider(newobj.ViewObject)
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
            currentxvector=fcvec.scale(xvector,xcount)
            if not xcount==0:
                move(objectslist,currentxvector,True)
            for ycount in range(ynum):
                currentxvector=FreeCAD.Base.Vector(currentxvector)
                currentyvector=currentxvector.add(fcvec.scale(yvector,ycount))
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
            shape.rotate(fcvec.tup(center), fcvec.tup(axis), angle)
            newobj.Shape = shape
        elif hasattr(obj,"Placement"):
            shape = Part.Shape()
            shape.Placement = obj.Placement
            shape.rotate(fcvec.tup(center), fcvec.tup(axis), angle)
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


def scale(objectslist,delta,center=Vector(0,0,0),copy=False,legacy=False):
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
            corr = fcvec.neg(corr.sub(center))
            sh.translate(corr)
            if getType(obj) == "Rectangle":
                p = []
                for v in sh.Vertexes: p.append(v.Point)
                pl = obj.Placement.copy()
                pl.Base = p[0]
                diag = p[2].sub(p[0])
                bb = p[1].sub(p[0])
                bh = p[3].sub(p[0])
                nb = fcvec.project(diag,bb)
                nh = fcvec.project(diag,bh)
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
        _Scale(obj)
        _ViewProviderDraft(obj.ViewObject)
        obj.Objects = objectslist
        obj.Scale = delta
        obj.BasePoint = center
        for o in objectslist:
            o.ViewObject.hide()
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
    import Part
    from draftlibs import fcgeo

    def getRect(p,obj):
        "returns length,heigh,placement"
        pl = obj.Placement.copy()
        pl.Base = p[0]
        diag = p[2].sub(p[0])
        bb = p[1].sub(p[0])
        bh = p[3].sub(p[0])
        nb = fcvec.project(diag,bb)
        nh = fcvec.project(diag,bh)
        if obj.Length < 0: l = -nb.Length
        else: l = nb.Length
        if obj.Height < 0: h = -nh.Length
        else: h = nh.Length
        return l,h,pl

    def getRadius(obj,delta):
        "returns a new radius for a regular polygon"
        an = math.pi/obj.FacesNumber
        nr = fcvec.rotate(delta,-an)
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
    else:
        if sym:
            d1 = delta.multiply(0.5)
            d2 = fcvec.neg(d1)
            n1 = fcgeo.offsetWire(obj.Shape,d1)
            n2 = fcgeo.offsetWire(obj.Shape,d2)
        else:
            newwire = fcgeo.offsetWire(obj.Shape,delta)
            p = fcgeo.getVerts(newwire)
    if occ:
        newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
        newobj.Shape = fcgeo.offsetWire(obj.Shape,delta,occ=True)
        formatObject(newobj,obj)
    elif bind:
        if not fcgeo.isReallyClosed(obj.Shape):
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
        formatObject(newobj,obj)
    else:
        if sym: return None
        if getType(obj) == "Wire":
            if obj.Base or obj.Tool:
                FreeCAD.Console.PrintWarning("Warning: object history removed\n")
                obj.Base = None
                obj.Tool = None
            obj.Points = p
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
    from draftlibs import fcgeo
    if not isinstance(objectslist,list):
        objectslist = [objectslist]
    newobjlist = []
    for obj in objectslist:
        if obj.isDerivedFrom('Part::Feature'):
            for w in obj.Shape.Wires:
                if fcgeo.hasCurves(w):
                    nobj = FreeCAD.ActiveDocument.addObject("Part::Feature",name)
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

def getrgb(color):
    "getRGB(color): returns a rgb value #000000 from a freecad color"
    r = str(hex(int(color[0]*255)))[2:].zfill(2)
    g = str(hex(int(color[1]*255)))[2:].zfill(2)
    b = str(hex(int(color[2]*255)))[2:].zfill(2)
    return "#"+r+g+b

def getSVG(obj,modifier=100,textmodifier=100,linestyle="continuous",fillstyle="shape color",direction=None):
    '''getSVG(object,[modifier],[textmodifier],[linestyle],[fillstyle],[direction]):
    returns a string containing a SVG representation of the given object. the modifier attribute
    specifies a scale factor for linewidths in %, and textmodifier specifies
    a scale factor for texts, in % (both default = 100). You can also supply
    an arbitrary projection vector.'''
    import Part
    from draftlibs import fcgeo
    svg = ""
    tmod = ((textmodifier-100)/2)+100
    if tmod == 0: tmod = 0.01
    modifier = 200-modifier
    if modifier == 0: modifier = 0.01
    pmod = (200-textmodifier)/20
    if pmod == 0: pmod = 0.01
    plane = None
    if direction:
        if direction != Vector(0,0,0):
            plane = WorkingPlane.plane()
            plane.alignToPointAndAxis(Vector(0,0,0),fcvec.neg(direction),0)

    def getProj(vec):
        if not plane: return vec
        nx = fcvec.project(vec,plane.u)
        lx = nx.Length
        if abs(nx.getAngle(plane.u)) > 0.1: lx = -lx
        ny = fcvec.project(vec,plane.v)
        ly = ny.Length
        if abs(ny.getAngle(plane.v)) > 0.1: ly = -ly
        return Vector(lx,ly,0)

    def getPath(edges):
        svg ='<path id="' + name + '" '
        edges = fcgeo.sortEdges(edges)
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
        if fill != 'none': svg += 'Z'
        svg += '" '
        svg += 'stroke="' + stroke + '" '
        svg += 'stroke-width="' + str(width) + ' px" '
        svg += 'style="stroke-width:'+ str(width)
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
            angle = -fcvec.angle(vtext)
            svg += 'd="M '+str(p1.x)+' '+str(p1.y)+' '
            svg += 'L '+str(p2.x)+' '+str(p2.y)+' '
            svg += 'L '+str(p3.x)+' '+str(p3.y)+' '
            svg += 'L '+str(p4.x)+' '+str(p4.y)+'" '
        else:
            ts = (len(dimText)*obj.ViewObject.FontSize)/4
            rm = ((p3.sub(p2)).Length/2)-ts
            p2a = getProj(p2.add(fcvec.scaleTo(p3.sub(p2),rm)))
            p2b = getProj(p3.add(fcvec.scaleTo(p2.sub(p3),rm)))
            angle = 0
            svg += 'd="M '+str(p1.x)+' '+str(p1.y)+' '
            svg += 'L '+str(p2.x)+' '+str(p2.y)+' '
            svg += 'L '+str(p2a.x)+' '+str(p2a.y)+' '
            svg += 'M '+str(p2b.x)+' '+str(p2b.y)+' '
            svg += 'L '+str(p3.x)+' '+str(p3.y)+' '
            svg += 'L '+str(p4.x)+' '+str(p4.y)+'" '                        
        svg += 'fill="none" stroke="'
        svg += getrgb(obj.ViewObject.LineColor) + '" '
        svg += 'stroke-width="' + str(obj.ViewObject.LineWidth/modifier) + ' px" '
        svg += 'style="stroke-width:'+ str(obj.ViewObject.LineWidth/modifier)
        svg += ';stroke-miterlimit:4;stroke-dasharray:none" '
        svg += 'freecad:basepoint1="'+str(p1.x)+' '+str(p1.y)+'" '
        svg += 'freecad:basepoint2="'+str(p4.x)+' '+str(p4.y)+'" '
        svg += 'freecad:dimpoint="'+str(p2.x)+' '+str(p2.y)+'"'
        svg += '/>\n'
        svg += '<circle cx="'+str(p2.x)+'" cy="'+str(p2.y)
        svg += '" r="'+str(obj.ViewObject.FontSize/(pmod))+'" '
        svg += 'fill="'+ getrgb(obj.ViewObject.LineColor) +'" stroke="none" '
        svg += 'style="stroke-miterlimit:4;stroke-dasharray:none" '
        svg += 'freecad:skip="1"'
        svg += '/>\n'
        svg += '<circle cx="'+str(p3.x)+'" cy="'+str(p3.y)
        svg += '" r="'+str(obj.ViewObject.FontSize/(pmod))+'" '
        svg += 'fill="#000000" stroke="none" '
        svg += 'style="stroke-miterlimit:4;stroke-dasharray:none" '
        svg += 'freecad:skip="1"'
        svg += '/>\n'
        svg += '<text id="' + obj.Name + '" fill="'
        svg += getrgb(obj.ViewObject.LineColor) +'" font-size="'
        svg += str(obj.ViewObject.FontSize*(tmod/5))+'" '
        svg += 'style="text-anchor:middle;text-align:center;'
        svg += 'font-family:'+obj.ViewObject.FontName+'" '
        svg += 'transform="rotate('+str(math.degrees(angle))
        svg += ','+ str(tbase.x) + ',' + str(tbase.y) + ') '
        svg += 'translate(' + str(tbase.x) + ',' + str(tbase.y) + ') '
        svg += 'scale('+str(tmod/2000)+',-'+str(tmod/2000)+')" '
        svg += 'freecad:skip="1"'
        svg += '>\n'
        svg += dimText % p3.sub(p2).Length
        svg += '</text>\n</g>\n'

    elif getType(obj) == "Annotation":
        "returns an svg representation of a document annotation"
        p = getProj(obj.Position)
        svg = '<text id="' + obj.Name + '" fill="'
        svg += getrgb(obj.ViewObject.TextColor)
        svg += '" font-size="'
        svg += str(obj.ViewObject.FontSize*(tmod/5))+'" '
        svg += 'style="text-anchor:middle;text-align:center;'
        svg += 'font-family:'+obj.ViewObject.FontName+'" '
        svg += 'transform="'
        if obj.ViewObject.RotationAxis == 'Z':
            if obj.ViewObject.Rotation != 0:
                svg += 'rotate('+str(obj.ViewObject.Rotation)
                svg += ','+ str(p.x) + ',' + str(p.y) + ') '
        svg += 'translate(' + str(p.x) + ',' + str(p.y) + ') '
        svg +='scale('+str(tmod/2000)+','+str(-tmod/2000)+')">\n'
        for l in obj.LabelText:
            svg += '<tspan>'+l+'</tspan>\n'
        svg += '</text>\n'

    elif obj.isDerivedFrom('Part::Feature'):
        if obj.Shape.isNull(): return ''
        color = getrgb(obj.ViewObject.LineColor)
        # setting fill
        if obj.Shape.Faces and (obj.ViewObject.DisplayMode != "Wireframe"):
            if fillstyle == "shape color":
                fill = getrgb(obj.ViewObject.ShapeColor)
            else:
                fill = 'url(#'+hatch+')'
        else:
            fill = 'none'
        # setting linetype
        if linestyle == "dashed":
            lstyle = "0.09,0.05"
        elif linestyle == "dashdotted":
            lstyle = "0.09,0.05,0.02,0.05"
        elif linestyle == "dotted":
            lstyle = "0.02,0.02"
        else:
            lstyle = "none"
        name = obj.Name
        if obj.ViewObject.DisplayMode == "Shaded":
            stroke = "none"
        else:
            stroke = getrgb(obj.ViewObject.LineColor)
        width = obj.ViewObject.LineWidth/modifier
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
                    if (fcgeo.findEdge(e,wiredEdges) == None):
                        svg += getPath([e])
        else:
            cen = getProj(obj.Shape.Edges[0].Curve.Center)
            rad = obj.Shape.Edges[0].Curve.Radius
            svg = '<circle cx="' + str(cen.x)
            svg += '" cy="' + str(cen.y)
            svg += '" r="' + str(rad)+'" '
            svg += 'stroke="' + stroke + '" '
            svg += 'stroke-width="' + str(width) + ' px" '
            svg += 'style="stroke-width:'+ str(width)
            svg += ';stroke-miterlimit:4'
            svg += ';stroke-dasharray:' + lstyle
            svg += ';fill:' + fill + '"'
            svg += '/>\n'                
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
    import Part
    from draftlibs import fcgeo
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
                    if fcgeo.isAligned(nobj.Geometry[seg],"x"):
                        nobj.addConstraint(Constraint("Vertical",seg))
                    elif fcgeo.isAligned(nobj.Geometry[seg],"y"):
                        nobj.addConstraint(Constraint("Horizontal",seg))
                if closed:
                    nobj.addConstraint(Constraint("Coincident",last-1,EndPoint,segs[0],StartPoint))
            ok = True
        else:
            if fcgeo.hasOnlyWires(obj.Shape):
                for w in obj.Shape.Wires:
                    for edge in w.Edges:
                        nobj.addGeometry(fcgeo.geom(edge))
                    if autoconstraints:
                        last = nobj.GeometryCount
                        segs = range(last-len(w.Edges),last-1)
                        for seg in segs:
                            nobj.addConstraint(Constraint("Coincident",seg,EndPoint,seg+1,StartPoint))
                            if fcgeo.isAligned(nobj.Geometry[seg],"x"):
                                nobj.addConstraint(Constraint("Vertical",seg))
                            elif fcgeo.isAligned(nobj.Geometry[seg],"y"):
                                nobj.addConstraint(Constraint("Horizontal",seg))
                        if w.isClosed:
                            nobj.addConstraint(Constraint("Coincident",last-1,EndPoint,segs[0],StartPoint))
            else:
                for edge in obj.Shape.Edges:
                    nobj.addGeometry(fcgeo.geom(edge))
                    if autoconstraints:
                        last = nobj.GeometryCount - 1
                        if fcgeo.isAligned(nobj.Geometry[last],"x"):
                            nobj.addConstraint(Constraint("Vertical",last))
                        elif fcgeo.isAligned(nobj.Geometry[last],"y"):
                            nobj.addConstraint(Constraint("Horizontal",last))
            ok = True
        if ok:
            FreeCAD.ActiveDocument.removeObject(obj.Name)
    FreeCAD.ActiveDocument.recompute()
    return nobj

def makePoint(X=0, Y=0, Z=0,color=(0,1,0),name = "Point", point_size= 5):
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
    _ViewProviderPoint(obj.ViewObject)
    obj.X = X
    obj.Y = Y
    obj.Z = Z
    obj.ViewObject.PointColor = (float(color[0]), float(color[1]), float(color[2]))
    obj.ViewObject.PointSize = point_size
    obj.ViewObject.Visibility = True
    FreeCAD.ActiveDocument.recompute()
    return obj
			
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
        obj.addProperty("App::PropertyVector","Position","Base","The position of the text. Leave (0,0,0) for automatic position")
        obj.addProperty("App::PropertyString","Override","Base","Text override. Use 'dim' to insert the dimension length")
        obj.Proxy = self
        obj.FontSize=getParam("textheight")
        obj.FontName=getParam("textfont")
        obj.ExtLines=0.3
        obj.Override = ''

    def calcGeom(self,obj):
        import Part
        from draftlibs import fcgeo
        p1 = obj.Start
        p4 = obj.End
        base = Part.Line(p1,p4).toShape()
        proj = fcgeo.findDistance(obj.Dimline,base)
        if not proj:
            p2 = p1
            p3 = p4
        else:
            p2 = p1.add(fcvec.neg(proj))
            p3 = p4.add(fcvec.neg(proj))
            dmax = obj.ViewObject.ExtLines
            if dmax and (proj.Length > dmax):
                p1 = p2.add(fcvec.scaleTo(proj,dmax))
                p4 = p3.add(fcvec.scaleTo(proj,dmax))
        midpoint = p2.add(fcvec.scale(p3.sub(p2),0.5))
        if not proj:
            ed = fcgeo.vec(base)
            proj = ed.cross(Vector(0,0,1))
        if not proj: norm = Vector(0,0,1)
        else: norm = fcvec.neg(p3.sub(p2).cross(proj))
        norm.normalize()
        va = FreeCADGui.ActiveDocument.ActiveView.getViewDirection()
        if va.getAngle(norm) < math.pi/2:
            norm = fcvec.neg(norm)
        u = p3.sub(p2)
        u.normalize()
        c = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
        r = c.orientation.getValue()
        ru = Vector(r.multVec(coin.SbVec3f(1,0,0)).getValue())
        if ru.getAngle(u) > math.pi/2: u = fcvec.neg(u)
        v = norm.cross(u)
        offset = fcvec.scaleTo(v,obj.ViewObject.FontSize*.2)
        if obj.ViewObject:
            if hasattr(obj.ViewObject,"DisplayMode"):
                if obj.ViewObject.DisplayMode == "3D":
                    offset = fcvec.neg(offset)
        if obj.ViewObject.Position == Vector(0,0,0):
            tbase = midpoint.add(offset)
        else:
            tbase = obj.ViewObject.Position
        rot = FreeCAD.Placement(fcvec.getPlaneRotation(u,v,norm)).Rotation.Q
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
        tm = fcvec.getPlaneRotation(p3.sub(p2),norm)
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
        text = None
        if obj.Base and obj.LinkedVertices:
            if "Shape" in obj.Base.PropertiesList:
                if len(obj.LinkedVertices) == 3:
                    # arc linked dimension
                    e = obj.Base.Shape.Edges[obj.LinkedVertices[0]]
                    c = e.Curve.Center
                    bray = fcvec.scaleTo(obj.Dimline.sub(c),e.Curve.Radius)
                    if obj.LinkedVertices[1] == 1:
                        v1 = c
                    else:
                        v1 = c.add(fcvec.neg(bray))
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
        self.text.string = self.text3d.string = text
        self.textpos.rotation = coin.SbRotation(rot[0],rot[1],rot[2],rot[3])
        self.textpos.translation.setValue([tbase.x,tbase.y,tbase.z])
        if obj.ViewObject.DisplayMode == "2D":
            self.coords.point.setValues([[p1.x,p1.y,p1.z],
                                         [p2.x,p2.y,p2.z],
                                         [p3.x,p3.y,p3.z],
                                         [p4.x,p4.y,p4.z]])
            self.line.numVertices.setValues([4])
        else:
            ts = (len(text)*obj.ViewObject.FontSize)/4
            rm = ((p3.sub(p2)).Length/2)-ts
            p2a = p2.add(fcvec.scaleTo(p3.sub(p2),rm))
            p2b = p3.add(fcvec.scaleTo(p2.sub(p3),rm))
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
            self.updateData(vp.Object, None)

    def getDisplayModes(self,obj):
        modes=[]
        modes.extend(["2D","3D"])
        return modes

    def getDefaultDisplayMode(self):
        return "2D"

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
        return None
    
    def __setstate__(self,state):
        return None

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
        obj.addProperty("App::PropertyVector","Position","Base","The position of the text. Leave (0,0,0) for automatic position")
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
        import Part
        from draftlibs import fcgeo
        rad = (obj.Dimline.sub(obj.Center)).Length
        cir = Part.makeCircle(rad,obj.Center,Vector(0,0,1),obj.FirstAngle,obj.LastAngle)
        cp = fcgeo.findMidpoint(cir.Edges[0])
        rv = cp.sub(obj.Center)
        rv = fcvec.scaleTo(rv,rv.Length + obj.ViewObject.FontSize*.2)
        tbase = obj.Center.add(rv)
        trot = fcvec.angle(rv)-math.pi/2
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
        import Part
        plm = fp.Placement
        p1 = Vector(0,0,0)
        p2 = Vector(p1.x+fp.Length,p1.y,p1.z)
        p3 = Vector(p1.x+fp.Length,p1.y+fp.Height,p1.z)
        p4 = Vector(p1.x,p1.y+fp.Height,p1.z)
        shape = Part.makePolygon([p1,p2,p3,p4,p1])
        shape = Part.Face(shape)
        fp.Shape = shape
        fp.Placement = plm

class _ViewProviderRectangle(_ViewProviderDraft):
    "A View Provider for the Rectangle object"
    def __init__(self, obj):
        _ViewProviderDraft.__init__(self,obj)
        obj.addProperty("App::PropertyFile","TextureImage",
                        "Base","Uses an image as a texture map")

    def attach(self,obj):
        self.texture = None

    def onChanged(self, vp, prop):
        if prop == "TextureImage":
            r = vp.RootNode
            if os.path.exists(vp.TextureImage):
                self.texture = coin.SoTexture2()
                self.texture.filename = str(vp.TextureImage)
                r.insertChild(self.texture,1)
            else:
                if self.texture:
                    r.removeChild(self.texture)
                    self.texture = None
        return
        
class _Circle:
    "The Circle object"
        
    def __init__(self, obj):
        obj.addProperty("App::PropertyAngle","FirstAngle","Arc",
                        "Start angle of the arc")
        obj.addProperty("App::PropertyAngle","LastAngle","Arc",
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
        obj.Proxy = self
        obj.Closed = False
        self.Type = "Wire"

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        if prop in ["Points","Closed","Base","Tool"]:
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
        import Part
        from draftlibs import fcgeo
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
            if ('Shape' in fp.Base.PropertiesList) and ('Shape' in fp.Tool.PropertiesList):
                sh1 = fp.Base.Shape.copy()
                sh2 = fp.Tool.Shape.copy()
                shape = sh1.fuse(sh2)
                if fcgeo.isCoplanar(shape.Faces):
                    shape = fcgeo.concatenate(shape)
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
                shape = Part.Face(shape)
            else:
                edges = []
                pts = fp.Points[1:]
                lp = fp.Points[0]
                for p in pts:
                    edges.append(Part.Line(lp,p).toShape())
                    lp = p
                shape = Part.Wire(edges)
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
        import Part
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
        shape = Part.Face(shape)
        fp.Shape = shape
        fp.Placement = plm

class _DrawingView:
    def __init__(self, obj):
        obj.addProperty("App::PropertyVector","Direction","Shape view","Projection direction")
        obj.addProperty("App::PropertyFloat","LinewidthModifier","Drawing view","Modifies the linewidth of the lines inside this object")
        obj.addProperty("App::PropertyFloat","TextModifier","Drawing view","Modifies the size of the texts inside this object")
        obj.addProperty("App::PropertyLink","Source","Base","The linked object")
        obj.addProperty("App::PropertyEnumeration","LineStyle","Drawing view","Line Style")
        obj.addProperty("App::PropertyEnumeration","FillStyle","Drawing view","Shape Fill Style")
        obj.LineStyle = ['continuous','dashed','dashdotted','dotted']
        fills = ['shape color']
        for f in FreeCAD.svgpatterns.keys():
            fills.append(f)
        obj.FillStyle = fills

        obj.Proxy = self
        obj.LinewidthModifier = 100
        obj.TextModifier = 100
        self.Type = "DrawingView"

    def execute(self, obj):
        if obj.Source:
            obj.ViewResult = self.updateSVG(obj)

    def onChanged(self, obj, prop):
        if prop in ["X","Y","Scale","LinewidthModifier","TextModifier","LineStyle","FillStyle","Direction"]:
            obj.ViewResult = self.updateSVG(obj)

    def updateSVG(self, obj):
        "encapsulates a svg fragment into a transformation node"
        svg = getSVG(obj.Source,obj.LinewidthModifier,obj.TextModifier,obj.LineStyle,obj.FillStyle,obj.Direction)
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

class _ViewProviderBlock(_ViewProviderDraft):
    "A View Provider for the Block object"
    def __init__(self,obj):
        _ViewProviderDraft.__init__(self,obj)

    def claimChildren(self):
        return self.Object.Components

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
        import Drawing
        from draftlibs import fcgeo
        pl = obj.Placement
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                [visibleG0,visibleG1,hiddenG0,hiddenG1] = Drawing.project(obj.Base.Shape,obj.Projection)
                if visibleG0:
                    obj.Shape = visibleG0
        if not fcgeo.isNull(pl):
            obj.Placement = pl

class _Array:
    "The Draft Array object"

    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object that must be duplicated")
        obj.addProperty("App::PropertyEnumeration","ArrayType","Base",
                        "The type of array to create")
        obj.addProperty("App::PropertyInteger","NumberX","Base",
                        "Number of copies in X direction (ortho arrays)")
        obj.addProperty("App::PropertyInteger","NumberY","Base",
                        "Number of copies in Y direction (ortho arrays)")
        obj.addProperty("App::PropertyInteger","NumberPolar","Base",
                        "Number of copies (polar arrays)")
        obj.addProperty("App::PropertyVector","IntervalX","Base",
                        "Distance and orientation of intervals in X direction (ortho arrays)")
        obj.addProperty("App::PropertyVector","IntervalY","Base",
                        "Distance and orientation of intervals in Y direction (ortho arrays)")
        obj.addProperty("App::PropertyVector","Center","Base",
                        "Center point (polar arrays)")
        obj.addProperty("App::PropertyAngle","Angle","Base",
                        "Angle to cover with copies (polar arrays)")
        obj.Proxy = self
        self.Type = "Array"
        obj.ArrayType = ['ortho','polar']
        obj.NumberX = 1
        obj.NumberY = 1
        obj.NumberPolar = 1
        obj.IntervalX = Vector(1,0,0)
        obj.IntervalY = Vector(0,1,0)
        obj.Angle = 360

    def execute(self,obj):
        self.createGeometry(obj)

    def onChanged(self,obj,prop):
        if prop in ["ArrayType","NumberX","NumberY","NumberPolar","IntervalX","IntervalY","Angle","Center"]:
            self.createGeometry(obj)

    def createGeometry(self,obj):
        from draftlibs import fcgeo
        if obj.Base:
            pl = obj.Placement
            if obj.ArrayType == "ortho":
                sh = self.rectArray(obj.Base.Shape,obj.IntervalX,obj.IntervalY,obj.NumberX,obj.NumberY)
            else:
                sh = self.polarArray(obj.Base.Shape,obj.Center,obj.Angle,obj.NumberPolar)
            obj.Shape = sh
            if not fcgeo.isNull(pl):
                obj.Placement = pl

    def rectArray(self,shape,xvector,yvector,xnum,ynum):
        import Part
        base = [shape.copy()]
        for xcount in range(xnum):
            currentxvector=fcvec.scale(xvector,xcount)
            if not xcount==0:
                nshape = shape.copy()
                nshape.translate(currentxvector)
                base.append(nshape)
            for ycount in range(ynum):
                currentxvector=FreeCAD.Vector(currentxvector)
                currentyvector=currentxvector.add(fcvec.scale(yvector,ycount))
                if not ycount==0:
                    nshape = shape.copy()
                    nshape.translate(currentyvector)
                    base.append(nshape)
        return Part.makeCompound(base)

    def polarArray(self,shape,center,angle,num):
        import Part
        fraction = angle/num
        base = [shape.copy()]
        for i in range(num):
            currangle = fraction + (i*fraction)
            nshape = shape.copy()
            nshape.rotate(fcvec.tup(center), (0,0,1), currangle)
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

class _Scale:
    "The Scale object"

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkList","Objects","Base",
                        "The objects included in this scale object")
        obj.addProperty("App::PropertyVector","Scale","Base",
                        "The scale vector of this object")
        obj.addProperty("App::PropertyVector","BasePoint","Base",
                        "The base point of this scale object")
        obj.Scale = Vector(1,1,1)
        obj.Proxy = self
        self.Type = "Scale"

    def execute(self,obj):
        self.createGeometry(obj)

    def onChanged(self,obj,prop):
        if prop in ["Scale","BasePoint","Objects"]:
            self.createGeometry(obj)

    def createGeometry(self,obj):
        import Part
        from draftlibs import fcgeo
        pl = obj.Placement
        shapes = []
        for o in obj.Objects:
            if hasattr(o,"Shape"):
                sh = o.Shape.copy()
                m = FreeCAD.Matrix()
                m.scale(obj.Scale)
                sh = sh.transformGeometry(m)
                corr = Vector(obj.BasePoint)
                corr.scale(obj.Scale.x,obj.Scale.y,obj.Scale.z)
                corr = fcvec.neg(corr.sub(obj.BasePoint))
                sh.translate(corr)
                shapes.append(sh)
        if shapes:
            obj.Shape = Part.makeCompound(shapes)   
        if not fcgeo.isNull(pl):
            obj.Placement = pl


