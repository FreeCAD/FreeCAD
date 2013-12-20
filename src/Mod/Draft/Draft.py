# -*- coding: utf8 -*-

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
__url__ = "http://www.freecadweb.org"

'''
General description:

    The Draft module is a FreeCAD module for drawing/editing 2D entities.
    The aim is to give FreeCAD basic 2D-CAD capabilities (similar
    to Autocad and other similar software). This modules is made to be run
    inside FreeCAD and needs the PyQt4 and pivy modules available.

User manual:

    http://www.freecadweb.org/wiki/index.php?title=2d_Drafting_Module

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
import FreeCAD, math, sys, os, DraftVecUtils, Draft_rc
from FreeCAD import Vector

if FreeCAD.GuiUp:
    import FreeCADGui, WorkingPlane
    gui = True
else:
    print "FreeCAD Gui not present. Draft module will have some features disabled."
    gui = False
    
arrowtypes = ["Dot","Circle","Arrow"]

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
                 "maxSnapEdges","modalt","HatchPatternResolution","snapStyle",
                 "dimstyle"]:
        return "int"
    elif param in ["constructiongroupname","textfont","patternFile","template",
                   "snapModes","FontFile"]:
        return "string"
    elif param in ["textheight","tolerance","gridSpacing","arrowsize","extlines","dimspacing"]:
        return "float"
    elif param in ["selectBaseObjects","alwaysSnap","grid","fillmode","saveonexit","maxSnap",
                   "SvgLinesBlack","dxfStdSize","showSnapBar","hideSnapBar","alwaysShowGrid",
                   "renderPolylineWidth","showPlaneTracker","UsePartPrimitives","DiscretizeEllipses"]:
        return "bool"
    elif param in ["color","constructioncolor","snapcolor"]:
        return "unsigned"
    else:
        return None

def getParam(param,default=None):
    "getParam(parameterName): returns a Draft parameter value from the current config"
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    t = getParamType(param)
    #print "getting param ",param, " of type ",t, " default: ",str(default)
    if t == "int": 
        if default == None:
            default = 0
        return p.GetInt(param,default)
    elif t == "string": 
        if default == None:
            default = ""
        return p.GetString(param,default)
    elif t == "float": 
        if default == None:
            default = 0
        return p.GetFloat(param,default)
    elif t == "bool": 
        if default == None:
            default = False
        return p.GetBool(param,default)
    elif t == "unsigned":
        if default == None:
            default = 0
        return p.GetUnsigned(param,default)
    else: 
        return None

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
    return getParam("precision",6)

def tolerance():
    "tolerance(): returns the tolerance value from Draft user settings"
    return getParam("tolerance",0.05)

def epsilon():
    ''' epsilon(): returns a small number based on Draft.tolerance() for use in 
    floating point comparisons.  Use with caution. '''
    return (1.0/(10.0**tolerance()))
        
def getRealName(name):
    "getRealName(string): strips the trailing numbers from a string name"
    for i in range(1,len(name)):
        if not name[-i] in '1234567890':
            return name[:len(name)-(i-1)]
    return name

def getType(obj):
    "getType(object): returns the Draft type of the given object"
    import Part
    if not obj:
        return None
    if isinstance(obj,Part.Shape):
        return "Shape"
    if "Proxy" in obj.PropertiesList:
        if hasattr(obj.Proxy,"Type"):
            return obj.Proxy.Type
    if obj.isDerivedFrom("Sketcher::SketchObject"):
        return "Sketch"
    if (obj.TypeId == "Part::Line"):
        return "Part::Line"
    if obj.isDerivedFrom("Part::Feature"):
        return "Part"
    if (obj.TypeId == "App::Annotation"):
        return "Annotation"
    if obj.isDerivedFrom("Mesh::Feature"):
        return "Mesh"
    if obj.isDerivedFrom("Points::Feature"):
        return "Points"
    if (obj.TypeId == "App::DocumentObjectGroup"):
        return "Group"
    return "Unknown"

def getObjectsOfType(objectslist,typ):
    """getObjectsOfType(objectslist,typ): returns a list of objects of type "typ" found
    in the given object list"""
    objs = []
    for o in objectslist:
        if getType(o) == typ:
            objs.append(o)
    return objs

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
        if obj.TypeId == "App::DocumentObjectGroup":
            glist.append(obj.Name)
    return glist

def ungroup(obj):
    "removes the current object from any group it belongs to"
    for g in getGroupNames():
        grp = FreeCAD.ActiveDocument.getObject(g)
        if grp.hasObject(obj):
            grp.removeObject(obj)
      
def dimSymbol(symbol=None,invert=False):
    "returns the current dim symbol from the preferences as a pivy SoMarkerSet"
    if symbol == None:
        symbol = getParam("dimsymbol",0)
    from pivy import coin
    if symbol == 1: 
        marker = coin.SoMarkerSet()
        marker.markerIndex = coin.SoMarkerSet.CIRCLE_LINE_9_9
        return marker
    elif symbol == 2: 
        marker = coin.SoSeparator()
        t = coin.SoTransform()
        t.translation.setValue((0,-2,0))
        t.center.setValue((0,2,0)) 
        if invert:
            t.rotation.setValue(coin.SbVec3f((0,0,1)),-math.pi/2)
        else:
            t.rotation.setValue(coin.SbVec3f((0,0,1)),math.pi/2)
        c = coin.SoCone()
        c.height.setValue(4)
        marker.addChild(t)
        marker.addChild(c)
        return marker
    elif symbol == 3:
        print "Draft.dimsymbol: Not implemented"
    return coin.SoSphere()

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
        import DraftGeomUtils
        if DraftGeomUtils.geomType(shape.Edges[0]) == "Line":
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

def getGroupContents(objectslist,walls=False,addgroups=False):
    '''getGroupContents(objectlist,[walls,addgroups]): if any object of the given list
    is a group, its content is appened to the list, which is returned. If walls is True,
    walls are also scanned for included windows. If addgroups is true, the group itself
    is also included in the list.'''
    newlist = []
    if not isinstance(objectslist,list):
        objectslist = [objectslist]
    for obj in objectslist:
        if obj.isDerivedFrom("App::DocumentObjectGroup"):
            if obj.isDerivedFrom("Drawing::FeaturePage"):
                # skip if the group is a page
                newlist.append(obj)
            else:
                if addgroups:
                    newlist.append(obj)
                newlist.extend(getGroupContents(obj.Group,walls,addgroups))
        else:
            #print "adding ",obj.Name
            newlist.append(obj)
            if walls:
                if getType(obj) == "Wall":
                    for o in obj.OutList:
                        if (getType(o) == "Window") or isClone(o,"Window"):
                            newlist.append(o)
    # cleaning possible duplicates
    cleanlist = []
    for obj in newlist:
        if not obj in cleanlist:
            cleanlist.append(obj)
    return cleanlist

def removeHidden(objectslist):
    """removeHidden(objectslist): removes hidden objects from the list"""
    newlist = objectslist[:]
    for o in objectslist:
        if o.ViewObject:
            if not o.ViewObject.isVisible():
                newlist.remove(o)
    return newlist

def printShape(shape):
    """prints detailed information of a shape"""
    print "solids: ", len(shape.Solids)
    print "faces: ", len(shape.Faces)
    print "wires: ", len(shape.Wires)
    print "edges: ", len(shape.Edges)
    print "verts: ", len(shape.Vertexes)
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
            
def compareObjects(obj1,obj2):
    "Prints the differences between 2 objects"
    
    if obj1.TypeId != obj2.TypeId:
        print obj1.Name + " and " + obj2.Name + " are of different types"
    elif getType(obj1) != getType(obj2):
        print obj1.Name + " and " + obj2.Name + " are of different types"
    else:
        for p in obj1.PropertiesList:
            if p in obj2.PropertiesList:
                if p in ["Shape","Label"]:
                    pass
                elif p ==  "Placement":
                    delta = str((obj1.Placement.Base.sub(obj2.Placement.Base)).Length)
                    print "Objects have different placements. Distance between the 2: " + delta + " units"
                else:
                    if getattr(obj1,p) != getattr(obj2,p):
                        print "Property " + p + " has a different value"
            else:
                print "Property " + p + " doesn't exist in one of the objects"

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
            gname = getParam("constructiongroupname","Construction")
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
            if hasattr(matchrep,"DiffuseColor") and hasattr(obrep,"DiffuseColor"):
                obrep.DiffuseColor = matchrep.DiffuseColor

def getSelection():
    "getSelection(): returns the current FreeCAD selection"
    if gui:
        return FreeCADGui.Selection.getSelection()
    return None

def getSelectionEx():
    "getSelectionEx(): returns the current FreeCAD selection (with subobjects)"
    if gui:
        return FreeCADGui.Selection.getSelectionEx()
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

def loadSvgPatterns():
    "loads the default Draft SVG patterns and custom patters if available"
    import importSVG
    from PyQt4 import QtCore
    FreeCAD.svgpatterns = {}
    # getting default patterns
    patfiles = QtCore.QDir(":/patterns").entryList()
    for fn in patfiles:
        fn = ":/patterns/"+str(fn)
        f = QtCore.QFile(fn)
        f.open(QtCore.QIODevice.ReadOnly)
        p = importSVG.getContents(str(f.readAll()),'pattern',True)
        if p:
            for k in p:
                p[k] = [p[k],fn]
            FreeCAD.svgpatterns.update(p)
    # looking for user patterns
    altpat = getParam("patternFile","")
    if os.path.isdir(altpat):
        for f in os.listdir(altpat):
            if f[-4:].upper() == ".SVG":
                p = importSVG.getContents(altpat+os.sep+f,'pattern')
                if p:
                    for k in p:
                        p[k] = [p[k],fn]
                    FreeCAD.svgpatterns.update(p)
                    
def svgpatterns():
    """svgpatterns(): returns a dictionnary with installed SVG patterns"""
    if hasattr(FreeCAD,"svgpatterns"):
        return FreeCAD.svgpatterns
    else:
        loadSvgPatterns()
        if hasattr(FreeCAD,"svgpatterns"):
            return FreeCAD.svgpatterns
    return {}

def loadTexture(filename,size=None):
    """loadTexture(filename,[size]): returns a SoSFImage from a file. If size
    is defined (an int or a tuple), and provided the input image is a png file,
    it will be scaled to match the given size."""
    if gui:
        from pivy import coin
        from PyQt4 import QtGui,QtSvg
        try:
            p = QtGui.QImage(filename)
            # buggy - TODO: allow to use resolutions
            #if size and (".svg" in filename.lower()):
            #    # this is a pattern, not a texture
            #    if isinstance(size,int):
            #        size = (size,size)
            #    svgr = QtSvg.QSvgRenderer(filename)
            #    p = QtGui.QImage(size[0],size[1],QtGui.QImage.Format_ARGB32)
            #    pa = QtGui.QPainter()
            #    pa.begin(p)
            #    svgr.render(pa)
            #    pa.end()
            #else:   
            #    p = QtGui.QImage(filename)
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
            print "Draft: unable to load texture"
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
    import Part, DraftGeomUtils
    if placement: typecheck([(placement,FreeCAD.Placement)], "makeCircle")
    if startangle != endangle:
        n = "Arc"
    else:
        n = "Circle"
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",n)
    _Circle(obj)
    if isinstance(radius,Part.Edge):
        edge = radius
        if DraftGeomUtils.geomType(edge) == "Circle":
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
        l = []
        l.append((p1,"Vertex"+str(p2+1)))
        l.append((p1,"Vertex"+str(p3+1)))
        obj.LinkedGeometry = l
        obj.Support = p1
        p3 = p4
        if not p3:
            v1 = obj.Base.Shape.Vertexes[idx[0]].Point
            v2 = obj.Base.Shape.Vertexes[idx[1]].Point
            p3 = v2.sub(v1)
            p3.multiply(0.5)
            p3 = v1.add(p3)
    elif isinstance(p3,str):
        l = []
        l.append((p1,"Edge"+str(p2+1)))
        if p3 == "radius":
            l.append((p1,"Center"))
            obj.ViewObject.Override = "R $dim"
        elif p3 == "diameter":
            l.append((p1,"Diameter"))
            obj.ViewObject.Override = "D $dim"
        obj.LinkedGeometry = l
        obj.Support = p1
        p3 = p4
        if not p3:
            p3 = p1.Shape.Edges[p2].Curve.Center.add(Vector(1,0,0))
    obj.Dimline = p3
    if hasattr(FreeCAD,"DraftWorkingPlane"):
        normal = FreeCAD.DraftWorkingPlane.axis
    else:
        normal = App.Vector(0,0,1)
    if gui:
        # invert the normal if we are viewing it from the back
        vnorm = get3DView().getViewDirection()
        if vnorm.getAngle(normal) < math.pi/2:
            normal = normal.negative()
    obj.Normal = normal
    if gui:
        formatObject(obj)
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj

def makeAngularDimension(center,angles,p3,normal=None):
    '''makeAngularDimension(center,angle1,angle2,p3,[normal]): creates an angular Dimension
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
    if not normal:
        if hasattr(FreeCAD,"DraftWorkingPlane"):
            normal = FreeCAD.DraftWorkingPlane.axis
        else:
            normal = Vector(0,0,1)
    if gui:
        # invert the normal if we are viewing it from the back
        vnorm = get3DView().getViewDirection()
        if vnorm.getAngle(normal) < math.pi/2:
            normal = normal.negative()
    obj.Normal = normal
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
    if len(pointslist) == 0:
        print "Invalid input points: ",pointslist
    #print pointslist
    #print closed
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
        _ViewProviderWire(obj.ViewObject)
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
    for l in stringslist: 
        #textbuffer.append(l.decode("utf8").encode('latin1'))
        textbuffer.append(str(l))
    obj=FreeCAD.ActiveDocument.addObject("App::Annotation","Text")
    obj.LabelText=textbuffer
    obj.Position=point
    if not screen: obj.ViewObject.DisplayMode="World"
    h = getParam("textheight",0.20)
    if screen: h = h*10
    obj.ViewObject.FontSize = h
    obj.ViewObject.FontName = getParam("textfont","")
    obj.ViewObject.LineSpacing = 0.6
    formatObject(obj)
    select(obj)
    return obj

def makeCopy(obj,force=None,reparent=False):
    '''makeCopy(object): returns an exact copy of an object'''
    if (getType(obj) == "Rectangle") or (force == "Rectangle"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        _Rectangle(newobj)
        if gui:
            _ViewProviderRectangle(newobj.ViewObject)
    elif (getType(obj) == "Dimension") or (force == "Dimension"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        _Dimension(newobj)
        if gui:
            _ViewProviderDimension(newobj.ViewObject)
    elif (getType(obj) == "Wire") or (force == "Wire"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        _Wire(newobj)
        if gui:
            _ViewProviderWire(newobj.ViewObject)
    elif (getType(obj) == "Circle") or (force == "Circle"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        _Circle(newobj)
        if gui:
            _ViewProviderDraft(newobj.ViewObject)
    elif (getType(obj) == "Polygon") or (force == "Polygon"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        _Polygon(newobj)
        if gui:
            _ViewProviderDraft(newobj.ViewObject)
    elif (getType(obj) == "BSpline") or (force == "BSpline"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        _BSpline(newobj)
        if gui:
            _ViewProviderWire(newobj.ViewObject)
    elif (getType(obj) == "Block") or (force == "BSpline"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        _Block(newobj)
        if gui:
            _ViewProviderDraftPart(newobj.ViewObject)
    elif (getType(obj) == "DrawingView") or (force == "DrawingView"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        _DrawingView(newobj)
    elif (getType(obj) == "Structure") or (force == "Structure"):
        import ArchStructure
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        ArchStructure._Structure(newobj)
        if gui:
            ArchStructure._ViewProviderStructure(newobj.ViewObject)
    elif (getType(obj) == "Wall") or (force == "Wall"):
        import ArchWall
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        ArchWall._Wall(newobj)
        if gui:
            ArchWall._ViewProviderWall(newobj.ViewObject)
    elif (getType(obj) == "Window") or (force == "Window"):
        import ArchWindow
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        ArchWindow._Window(newobj)
        if gui:
            Archwindow._ViewProviderWindow(newobj.ViewObject)
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
                try:
                    setattr(newobj,p,obj.getPropertyByName(p))
                except:
                    pass
    if reparent:
        parents = obj.InList
        if parents:
            for par in parents:
                if par.isDerivedFrom("App::DocumentObjectGroup"):
                    par.addObject(newobj)
                else:
                    for prop in par.PropertiesList:
                        if getattr(par,prop) == obj:
                            setattr(par,prop,newobj)
                            FreeCAD.ActiveDocument.recompute()
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

def makeArray(baseobject,arg1,arg2,arg3,arg4=None,name="Array"):
    '''makeArray(object,xvector,yvector,xnum,ynum,[name]) for rectangular array, or
    makeArray(object,center,totalangle,totalnum,[name]) for polar array: Creates an array
    of the given object
    with, in case of rectangular array, xnum of iterations in the x direction
    at xvector distance between iterations, and same for y direction with yvector
    and ynum. In case of polar array, center is a vector, totalangle is the angle
    to cover (in degrees) and totalnum is the number of objects, including the original.
    The result is a parametric Draft Array.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
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
 
def makePathArray(baseobject,pathobject,count,xlate=None,align=False,pathobjsubs=[]):
    '''makePathArray(docobj,path,count,xlate,align,pathobjsubs): distribute 
    count copies of a document baseobject along a pathobject or subobjects of a 
    pathobject. Optionally translates each copy by FreeCAD.Vector xlate direction 
    and distance to adjust for difference in shape centre vs shape reference point.
    Optionally aligns baseobject to tangent/normal/binormal of path.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","PathArray")
    _PathArray(obj)
    obj.Base = baseobject
    obj.PathObj = pathobject
    if pathobjsubs:
        sl = []
        for sub in pathobjsubs:
            sl.append((obj.PathObj,sub))
        obj.PathSubs = list(sl) 
    if count > 1:
        obj.Count = count
    if xlate:
        obj.Xlate = xlate
    obj.Align = align
    if gui:
        _ViewProviderDraftPart(obj.ViewObject)  
        baseobject.ViewObject.hide()
        select(obj)
    return obj

def makeEllipse(majradius,minradius,placement=None,face=True,support=None):
    '''makeEllipse(majradius,minradius,[placement],[face],[support]): makes
    an ellipse with the given major and minor radius, and optionally
    a placement.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Ellipse")
    _Ellipse(obj)
    if minradius > majradius:
        majradius,minradius = minradius,majradius
    obj.MajorRadius = majradius
    obj.MinorRadius = minradius
    obj.Support = support
    if placement: 
        obj.Placement = placement
    if gui:
        _ViewProviderDraft(obj.ViewObject)
        if not face: 
            obj.ViewObject.DisplayMode = "Wireframe"
        formatObject(obj)
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
    if copy and getParam("selectBaseObjects",False):
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
    to cover (in degrees) and totalnum is the number of objects, including the original.
    
    This function creates an array of independent objects. Use makeArray() to create a
    parametric array object.'''
    
    def rectArray(objectslist,xvector,yvector,xnum,ynum):
        typecheck([(xvector,Vector), (yvector,Vector), (xnum,int), (ynum,int)], "rectArray")
        if not isinstance(objectslist,list): objectslist = [objectslist]
        for xcount in range(xnum):
            currentxvector=Vector(xvector).multiply(xcount)
            if not xcount==0:
                move(objectslist,currentxvector,True)
            for ycount in range(ynum):
                currentxvector=FreeCAD.Base.Vector(currentxvector)
                currentyvector=currentxvector.add(Vector(yvector).multiply(ycount))
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
    if copy and getParam("selectBaseObjects",False):
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
            corr = (corr.sub(center)).negative()
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
            elif (obj.TypeId == "App::Annotation"):
                factor = delta.x * delta.y * delta.z * obj.ViewObject.FontSize
                obj.ViewObject.Fontsize = factor
            if copy: formatObject(newobj,obj)
            newobjlist.append(newobj)
        if copy and getParam("selectBaseObjects",False):
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
        corr = (corr.sub(center)).negative()
        p = obj.Placement
        p.move(corr)
        obj.Placement = p
        if not copy:
            for o in objectslist:
                o.ViewObject.hide()
        if gui:
            _ViewProviderClone(obj.ViewObject)
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
            d1 = Vector(delta).multiply(0.5)
            d2 = d1.negative()
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
            #print delta
            obj.Points = delta
            #print "done"
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
    if copy and getParam("selectBaseObjects",False):
        select(newobj)
    else:
        select(obj)
    return newobj

def draftify(objectslist,makeblock=False,delete=True):
    '''draftify(objectslist,[makeblock],[delete]): turns each object of the given list
    (objectslist can also be a single object) into a Draft parametric
    wire. If makeblock is True, multiple objects will be grouped in a block.
    If delete = False, old objects are not deleted'''
    import DraftGeomUtils, Part

    if not isinstance(objectslist,list):
        objectslist = [objectslist]
    newobjlist = []
    for obj in objectslist:
        if obj.isDerivedFrom('Part::Feature'):
            for w in obj.Shape.Wires:
                if DraftGeomUtils.hasCurves(w):
                    if (len(w.Edges) == 1) and (DraftGeomUtils.geomType(w.Edges[0]) == "Circle"):
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
            if delete:
                FreeCAD.ActiveDocument.removeObject(obj.Name)
    FreeCAD.ActiveDocument.recompute()
    if makeblock:
        return makeBlock(newobjlist)
    else:
        if len(newobjlist) == 1:
            return newobjlist[0]
        return newobjlist

def getSVG(obj,scale=1,linewidth=0.35,fontsize=12,fillstyle="shape color",direction=None,linestyle=None):
    '''getSVG(object,[scale], [linewidth],[fontsize],[fillstyle],[direction],[linestyle]):
    returns a string containing a SVG representation of the given object,
    with the given linewidth and fontsize (used if the given object contains
    any text). You can also supply an arbitrary projection vector. the
    scale parameter allows to scale linewidths down, so they are resolution-independant.'''
    import Part, DraftGeomUtils
    svg = ""
    linewidth = linewidth/scale
    fontsize = (fontsize/scale)/2
    pointratio = .75 # the number of times the dots are smaller than the arrow size
    plane = None
    if direction:
        if isinstance(direction,FreeCAD.Vector):
            if direction != Vector(0,0,0):
                plane = WorkingPlane.plane()
                plane.alignToPointAndAxis(Vector(0,0,0),direction.negative(),0)
        elif isinstance(direction,WorkingPlane.plane):
            plane = direction

    def getLineStyle():
        "returns a linestyle"
        if linestyle == "Dashed":
            return "0.09,0.05"
        elif linestyle == "Dashdot":
            return "0.09,0.05,0.02,0.05"
        elif linestyle == "Dotted":
            return "0.02,0.02"
        return "none"

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
        if pat in svgpatterns():
            return svgpatterns()[pat][0]
        return ''

    def getPath(edges=[],wires=[]):
        import DraftGeomUtils
        svg ='<path id="' + obj.Name + '" '
        svg += 'd="'
        if not wires:
            egroups = [edges]
        else:
            egroups = []
            for w in wires:
                egroups.append(w.Edges)
        for g in egroups:
            edges = DraftGeomUtils.sortEdges(g)
            v = getProj(edges[0].Vertexes[0].Point)
            svg += 'M '+ str(v.x) +' '+ str(v.y) + ' '
            for e in edges:
                if DraftGeomUtils.geomType(e) == "Circle":
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
                else:
                    v = getProj(e.Vertexes[-1].Point)
                    svg += 'L '+ str(v.x) +' '+ str(v.y) + ' '
            if fill != 'none': svg += 'Z '
        svg += '" '
        svg += 'stroke="' + stroke + '" '
        svg += 'stroke-width="' + str(linewidth) + ' px" '
        svg += 'style="stroke-width:'+ str(linewidth)
        svg += ';stroke-miterlimit:4'
        svg += ';stroke-dasharray:' + lstyle
        svg += ';fill:' + fill
        svg += ';fill-rule: evenodd "'
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
        
    def getArrow(arrowtype,point,arrowsize,color,linewidth,angle=0):
        svg = ""
        if obj.ViewObject.ArrowType == "Circle":
            svg += '<circle cx="'+str(point.x)+'" cy="'+str(point.y)
            svg += '" r="'+str(arrowsize)+'" '
            svg += 'fill="none" stroke="'+ getrgb(color) + '" '
            svg += 'style="stroke-width:'+ str(linewidth) + ';stroke-miterlimit:4;stroke-dasharray:none" '
            svg += 'freecad:skip="1"'
            svg += '/>\n'
        elif obj.ViewObject.ArrowType == "Dot":
            svg += '<circle cx="'+str(point.x)+'" cy="'+str(point.y)
            svg += '" r="'+str(arrowsize)+'" '
            svg += 'fill="'+ getrgb(color) +'" stroke="none" '
            svg += 'style="stroke-miterlimit:4;stroke-dasharray:none" '
            svg += 'freecad:skip="1"'
            svg += '/>\n'
        elif obj.ViewObject.ArrowType == "Arrow":
            svg += '<path transform="rotate('+str(math.degrees(angle))
            svg += ','+ str(point.x) + ',' + str(point.y) + ') '
            svg += 'translate(' + str(point.x) + ',' + str(point.y) + ') '
            svg += 'scale('+str(arrowsize)+','+str(arrowsize)+')" freecad:skip="1" '
            svg += 'fill="'+ getrgb(color) +'" stroke="none" '
            svg += 'style="stroke-miterlimit:4;stroke-dasharray:none" '
            svg += 'd="M 0 0 L 4 1 L 4 -1 Z"/>\n'
        else:
            print "getSVG: arrow type not implemented"
        return svg
        
    def getText(color,fontsize,fontname,angle,base,text):
        svg = '<text fill="'
        svg += getrgb(color) +'" font-size="'
        svg += str(fontsize) + '" '
        svg += 'style="text-anchor:middle;text-align:center;'
        svg += 'font-family:'+ fontname +'" '
        svg += 'transform="rotate('+str(math.degrees(angle))
        svg += ','+ str(base.x) + ',' + str(base.y) + ') '
        svg += 'translate(' + str(base.x) + ',' + str(base.y) + ') '
        #svg += 'scale('+str(tmod/2000)+',-'+str(tmod/2000)+') '
        svg += 'scale(1,-1) '
        svg += '" freecad:skip="1"'
        svg += '>\n'
        svg += text
        svg += '</text>\n'
        return svg

        
    if not obj:
        pass

    elif getType(obj) == "Dimension":
        if obj.ViewObject.Proxy:
            if hasattr(obj.ViewObject.Proxy,"p1"):
                prx = obj.ViewObject.Proxy
                ts = (len(prx.string)*obj.ViewObject.FontSize)/4
                rm = ((prx.p3.sub(prx.p2)).Length/2)-ts
                p2a = getProj(prx.p2.add(DraftVecUtils.scaleTo(prx.p3.sub(prx.p2),rm)))
                p2b = getProj(prx.p3.add(DraftVecUtils.scaleTo(prx.p2.sub(prx.p3),rm)))
                p1 = getProj(prx.p1)
                p2 = getProj(prx.p2)
                p3 = getProj(prx.p3)
                p4 = getProj(prx.p4)
                tbase = p2.add(p3.sub(p2).multiply(0.5))
                angle = -DraftVecUtils.angle(p3.sub(p2))
                    
                # drawing lines
                svg = '<path '
                if obj.ViewObject.DisplayMode == "2D":
                    tangle = angle
                    if tangle > math.pi/2:
                        tangle = tangle-math.pi
                    elif (tangle <= -math.pi/2) or (tangle > math.pi/2):
                        tangle = tangle+math.pi
                    tbase = tbase.add(DraftVecUtils.rotate(Vector(0,2/scale,0),tangle))
                    svg += 'd="M '+str(p1.x)+' '+str(p1.y)+' '
                    svg += 'L '+str(p2.x)+' '+str(p2.y)+' '
                    svg += 'L '+str(p3.x)+' '+str(p3.y)+' '
                    svg += 'L '+str(p4.x)+' '+str(p4.y)+'" '
                else:
                    tangle = 0
                    tbase = tbase.add(Vector(0,-2/scale,0))
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
                
                # drawing arrows
                if hasattr(obj.ViewObject,"ArrowType"):
                    arrowsize = obj.ViewObject.ArrowSize/pointratio
                    if hasattr(obj.ViewObject,"FlipArrows"):
                        if obj.ViewObject.FlipArrows:
                            angle = angle+math.pi
                    svg += getArrow(obj.ViewObject.ArrowType,p2,arrowsize,obj.ViewObject.LineColor,linewidth,angle)
                    svg += getArrow(obj.ViewObject.ArrowType,p3,arrowsize,obj.ViewObject.LineColor,linewidth,angle+math.pi)
                
                # drawing text
                svg += getText(obj.ViewObject.LineColor,fontsize,obj.ViewObject.FontName,tangle,tbase,prx.string)
                
    elif getType(obj) == "AngularDimension":
        if obj.ViewObject.Proxy:
            if hasattr(obj.ViewObject.Proxy,"circle"):
                prx = obj.ViewObject.Proxy
                
                # drawing arc
                fill= "none"
                stroke = getrgb(obj.ViewObject.LineColor)
                lstyle = getLineStyle()
                if obj.ViewObject.DisplayMode == "2D":
                    svg += getPath([prx.circle])
                else:
                    if hasattr(prx,"circle1"):
                        svg += getPath([prx.circle1])
                        svg += getPath([prx.circle2])
                    else:
                        svg += getPath([prx.circle])
                
                # drawing arrows
                if hasattr(obj.ViewObject,"ArrowType"):
                    p2 = getProj(prx.p2)
                    p3 = getProj(prx.p3)
                    arrowsize = obj.ViewObject.ArrowSize/pointratio
                    arrowlength = 4*obj.ViewObject.ArrowSize
                    u1 = getProj((prx.circle.valueAt(prx.circle.FirstParameter+arrowlength)).sub(prx.circle.valueAt(prx.circle.FirstParameter)))
                    u2 = getProj((prx.circle.valueAt(prx.circle.LastParameter-arrowlength)).sub(prx.circle.valueAt(prx.circle.LastParameter)))
                    angle1 = -DraftVecUtils.angle(u1)
                    angle2 = -DraftVecUtils.angle(u2)
                    if hasattr(obj.ViewObject,"FlipArrows"):
                        if obj.ViewObject.FlipArrows:
                            angle1 = angle1+math.pi
                            angle2 = angle2+math.pi
                    svg += getArrow(obj.ViewObject.ArrowType,p2,arrowsize,obj.ViewObject.LineColor,linewidth,angle1)
                    svg += getArrow(obj.ViewObject.ArrowType,p3,arrowsize,obj.ViewObject.LineColor,linewidth,angle2)
                    
                # drawing text
                if obj.ViewObject.DisplayMode == "2D":
                    t = prx.circle.tangentAt(prx.circle.FirstParameter+(prx.circle.LastParameter-prx.circle.FirstParameter)/2)
                    t = getProj(t)
                    tangle = DraftVecUtils.angle(t)
                    if (tangle <= -math.pi/2) or (tangle > math.pi/2):
                        tangle = tangle + math.pi
                    tbase = getProj(prx.circle.valueAt(prx.circle.FirstParameter+(prx.circle.LastParameter-prx.circle.FirstParameter)/2))
                    tbase = tbase.add(DraftVecUtils.rotate(Vector(0,2/scale,0),tangle))
                    print tbase
                else:
                    tangle = 0
                    tbase = getProj(prx.tbase)
                svg += getText(obj.ViewObject.LineColor,fontsize,obj.ViewObject.FontName,tangle,tbase,prx.string)

    elif getType(obj) == "Annotation":
        "returns an svg representation of a document annotation"
        p = getProj(obj.Position)
        svg = '<text id="' + obj.Name + '" fill="'
        svg += getrgb(obj.ViewObject.TextColor)
        svg += '" font-size="'+str(fontsize)
        svg += '" style="'
        if obj.ViewObject.Justification == "Left":
            svg += 'text-anchor:start;text-align:left;'
        elif obj.ViewObject.Justification == "Right":
            svg += 'text-anchor:end;text-align:right;'
        else:
            svg += 'text-anchor:middle;text-align:center;'
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
        for i in range(len(obj.LabelText)):
            if i == 0:
                svg += '<tspan>'
            else:
                svg += '<tspan x="0" dy="'+str(obj.ViewObject.LineSpacing/2)+'">'
            svg += obj.LabelText[i]+'</tspan>\n'
        svg += '</text>\n'
        #print svg

    elif getType(obj) == "Axis":
        "returns the SVG representation of an Arch Axis system"
        color = getrgb(obj.ViewObject.LineColor)
        lorig = getLineStyle()
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
            svg += 'font-family: sans;" '
            svg += 'transform="translate(' + str(center.x+rad/4) + ',' + str(center.y-rad/3) + ') '
            svg += 'scale(1,-1)"> '
            svg += '<tspan>' + obj.ViewObject.Proxy.getNumber(n) + '</tspan>\n'
            svg += '</text>\n'
            n += 1

    elif obj.isDerivedFrom('Part::Feature'):
        if obj.Shape.isNull(): 
            return ''
        if gui:
            color = getrgb(obj.ViewObject.LineColor)
        else:
            color = "#000000"
        # setting fill
        if obj.Shape.Faces:
            if gui:
                try:
                    m = obj.ViewObject.DisplayMode
                except:
                    m = None
                if (m != "Wireframe"):
                    if fillstyle == "shape color":
                        fill = getrgb(obj.ViewObject.ShapeColor,testbw=False)
                    else:
                        fill = 'url(#'+fillstyle+')'
                        svg += getPattern(fillstyle)
                else:
                    fill = "none"
            else:
                fill = "#888888"
        else:
            fill = 'none'
        lstyle = getLineStyle()
        if gui:
            try:
                try:
                    m = obj.ViewObject.DisplayMode
                except:
                    m = None
                if m == "Shaded":
                    stroke = "none"
                else:
                    stroke = getrgb(obj.ViewObject.LineColor)
            except:
                stroke = "#000000"
        else:
            stroke = "#000000"
        
        if len(obj.Shape.Vertexes) > 1:
            wiredEdges = []
            if obj.Shape.Faces:
                for f in obj.Shape.Faces:
                    svg += getPath(wires=f.Wires)
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
    
def getrgb(color,testbw=True):
    """getRGB(color,[testbw]): returns a rgb value #000000 from a freecad color
    if testwb = True (default), pure white will be converted into pure black"""
    r = str(hex(int(color[0]*255)))[2:].zfill(2)
    g = str(hex(int(color[1]*255)))[2:].zfill(2)
    b = str(hex(int(color[2]*255)))[2:].zfill(2)
    col = "#"+r+g+b
    if testbw:
        if col == "#ffffff":
            #print getParam('SvgLinesBlack')
            if getParam('SvgLinesBlack',True):
                col = "#000000"
    return col

def makeDrawingView(obj,page,lwmod=None,tmod=None):
    '''
    makeDrawingView(object,page,[lwmod,tmod]) - adds a View of the given object to the
    given page. lwmod modifies lineweights (in percent), tmod modifies text heights
    (in percent). The Hint scale, X and Y of the page are used.
    '''
    if getType(obj) == "SectionPlane":
        import ArchSectionPlane
        viewobj = FreeCAD.ActiveDocument.addObject("Drawing::FeatureViewPython","View")
        page.addObject(viewobj)
        ArchSectionPlane._ArchDrawingView(viewobj)
        viewobj.Source = obj
        viewobj.Label = "View of "+obj.Name
    else:
        viewobj = FreeCAD.ActiveDocument.addObject("Drawing::FeatureViewPython","View"+obj.Name)
        _DrawingView(viewobj)
        page.addObject(viewobj)
        viewobj.Scale = page.ViewObject.HintScale
        viewobj.X = page.ViewObject.HintOffsetX
        viewobj.Y = page.ViewObject.HintOffsetY
        viewobj.Source = obj
        if lwmod: viewobj.LineweightModifier = lwmod
        if tmod: viewobj.TextModifier = tmod
        if hasattr(obj.ViewObject,"Pattern"):
            if str(obj.ViewObject.Pattern) in svgpatterns().keys():
                viewobj.FillStyle = str(obj.ViewObject.Pattern)
        if hasattr(obj.ViewObject,"DrawStyle"):
            viewobj.LineStyle = obj.ViewObject.DrawStyle
    return viewobj

def makeShape2DView(baseobj,projectionVector=None,facenumbers=[]):
    '''
    makeShape2DView(object,[projectionVector,facenumbers]) - adds a 2D shape to the document, which is a
    2D projection of the given object. A specific projection vector can also be given. You can also
    specify a list of face numbers to be considered in individual faces mode.
    '''
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Shape2DView")
    _Shape2DView(obj)
    if gui:
        _ViewProviderDraftAlt(obj.ViewObject)
    obj.Base = baseobj
    if projectionVector:
        obj.Projection = projectionVector
    if facenumbers:
        obj.FaceNumbers = facenumbers
    select(obj)
    return obj

def makeSketch(objectslist,autoconstraints=False,addTo=None,delete=False,name="Sketch"):
    '''makeSketch(objectslist,[autoconstraints],[addTo],[delete],[name]): makes a Sketch
    objectslist with the given Draft objects. If autoconstraints is True,
    constraints will be automatically added to wire nodes, rectangles
    and circles. If addTo is an existing sketch, geometry will be added to it instead of
    creating a new one. If delete is True, the original object will be deleted'''
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
            print "makeSketch: BSplines not supported"
        elif tp == "Circle":
            g = (DraftGeomUtils.geom(obj.Shape.Edges[0],nobj.Placement))
            nobj.addGeometry(g)
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
            if obj.FilletRadius == 0:
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
            if not DraftGeomUtils.isPlanar(obj.Shape):
                print "Error: The given object is not planar and cannot be converted into a sketch."
                return None
            for e in obj.Shape.Edges:
                if DraftGeomUtils.geomType(e) == "BSplineCurve":
                    print "Error: One of the selected object contains BSplines, unable to convert"
                    return None
            if not addTo:
                nobj.Placement.Rotation = DraftGeomUtils.calculatePlacement(obj.Shape).Rotation
            edges = []
            for e in obj.Shape.Edges:
                g = (DraftGeomUtils.geom(e,nobj.Placement))
                if g:
                    nobj.addGeometry(g)
            ok = True
        formatObject(nobj,obj)
        if ok and delete:
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

def makeShapeString(String,FontFile,Size = 100,Tracking = 0):
    '''ShapeString(Text,FontFile,Height,Track): Turns a text string 
    into a Compound Shape'''
    
    # temporary code
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","ShapeString")
    _ShapeString(obj)
    obj.String = String
    obj.FontFile = FontFile
    obj.Size = Size
    obj.Tracking = Tracking
 
    if gui:
        _ViewProviderDraft(obj.ViewObject)
        formatObject(obj)
        obrep = obj.ViewObject
        if "PointSize" in obrep.PropertiesList: obrep.PointSize = 1             # hide the segment end points
        select(obj)
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
    cl.Objects = obj
    if delta:
        cl.Placement.move(delta)
    formatObject(cl,obj[0])
    return cl

def heal(objlist=None,delete=True,reparent=True):
    '''heal([objlist],[delete],[reparent]) - recreates Draft objects that are damaged,
    for example if created from an earlier version. If delete is True,
    the damaged objects are deleted (default). If ran without arguments, all the objects
    in the document will be healed if they are damaged. If reparent is True (default),
    new objects go at the very same place in the tree than their original.'''
    
    auto = False

    if not objlist:
        objlist = FreeCAD.ActiveDocument.Objects
        print "Automatic mode: Healing whole document..."
        auto = True
    else:
        print "Manual mode: Force-healing selected objects..."

    if not isinstance(objlist,list):
        objlist = [objlist]

    dellist = []
    got = False
    
    for obj in objlist:
        dtype = getType(obj)
        ftype = obj.TypeId
        if ftype in ["Part::FeaturePython","App::FeaturePython","Part::Part2DObjectPython","Drawing::FeatureViewPython"]:
            proxy = obj.Proxy
            if hasattr(obj,"ViewObject"):
                if hasattr(obj.ViewObject,"Proxy"):
                    proxy = obj.ViewObject.Proxy
            if (proxy == 1) or (dtype in ["Unknown","Part"]) or (not auto):
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
                    if "BSpline" in obj.Name:
                        print "Healing " + obj.Name + " of type BSpline"
                        nobj = makeCopy(obj,force="BSpline",reparent=reparent)
                    else:
                        print "Healing " + obj.Name + " of type Wire"
                        nobj = makeCopy(obj,force="Wire",reparent=reparent)
                elif ("Radius" in props) and ("FirstAngle" in props):
                    print "Healing " + obj.Name + " of type Circle"
                    nobj = makeCopy(obj,force="Circle",reparent=reparent)
                elif ("DrawMode" in props) and ("FacesNumber" in props):
                    print "Healing " + obj.Name + " of type Polygon"
                    nobj = makeCopy(obj,force="Polygon",reparent=reparent)
                elif ("FillStyle" in props) and ("FontSize" in props):
                    nobj = makeCopy(obj,force="DrawingView",reparent=reparent)
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
            
def makeFacebinder(selectionset,name="Facebinder"):
    """makeFacebinder(selectionset,[name]): creates a Facebinder object from a selection set.
    Only faces will be added."""
    if not isinstance(selectionset,list):
        selectionset = [selectionset]
    fb = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Facebinder(fb)
    if gui:
        _ViewProviderDraft(fb.ViewObject)
    faces = []
    fb.Proxy.addSubobjects(fb,selectionset)
    return fb
    
            
def upgrade(objects,delete=False,force=None):
    """upgrade(objects,delete=False,force=None): Upgrades the given object(s) (can be
    an object or a list of objects). If delete is True, old objects are deleted.
    The force attribute can be used to
    force a certain way of upgrading. It can be: makeCompound, closeGroupWires,
    makeSolid, closeWire, turnToParts, makeFusion, makeShell, makeFaces, draftify,
    joinFaces, makeSketchFace, makeWires
    Returns a dictionnary containing two lists, a list of new objects and a list 
    of objects to be deleted"""

    import Part, DraftGeomUtils
    from DraftTools import msg,translate
    
    if not isinstance(objects,list):
        objects = [objects]

    global deleteList, newList
    deleteList = []
    addList = []
    
    # definitions of actions to perform

    def makeCompound(objectslist):
        """returns a compound object made from the given objects"""
        newobj = makeBlock(objectslist)
        addList.append(newobj)
        return newobj

    def closeGroupWires(groupslist):
        """closes every open wire in the given groups"""
        result = False
        for grp in groupslist: 
            for obj in grp.Group:
                    newobj = closeWire(obj)
                    # add new objects to their respective groups
                    if newobj:
                        result = True
                        grp.addObject(newobj)
        return result

    def makeSolid(obj):
        """turns an object into a solid, if possible"""
        if obj.Shape.Solids:
            return None
        sol = None
        try:
            sol = Part.makeSolid(obj.Shape)
        except:
            return None
        else:
            if sol:
                if sol.isClosed():
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Solid")
                    newobj.Shape = sol
                    addList.append(newobj)
                    deleteList.append(obj)
            return newob
        
    def closeWire(obj):
        """closes a wire object, if possible"""
        if obj.Shape.Faces:
            return None
        if len(obj.Shape.Wires) != 1:
            return None
        if len(obj.Shape.Edges) == 1:
            return None
        if getType(obj) == "Wire":
            obj.Closed = True
            return True
        else:
            w = obj.Shape.Wires[0]
            if not w.isClosed():
                edges = w.Edges
                p0 = w.Vertexes[0].Point
                p1 = w.Vertexes[-1].Point
                if p0 == p1:
                    # sometimes an open wire can have its start and end points identical (OCC bug)
                    # in that case, although it is not closed, face works...
                    f = Part.Face(w)
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Face")
                    newobj.Shape = f
                else:
                    edges.append(Part.Line(p1,p0).toShape())
                    w = Part.Wire(DraftGeomUtils.sortEdges(edges))
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Wire")
                    newobj.Shape = w
                addList.append(newobj)
                deleteList.append(obj)
                return newobj
            else:
                return None

    def turnToParts(meshes):
        """turn given meshes to parts"""
        result = False
        import Arch
        for mesh in meshes:
            sh = Arch.getShapeFromMesh(mesh.Mesh)
            if sh:
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Shell")
                newobj.Shape = sh
                addList.append(newobj)
                deleteList.append(mesh)
                result = True
        return result
        
    def makeFusion(obj1,obj2):
        """makes a Draft or Part fusion between 2 given objects"""
        newobj = fuse(obj1,obj2)
        if newobj:
            addList.append(newobj)
            return newobj
        return None

    def makeShell(objectslist):
        """makes a shell with the given objects"""
        faces = []
        for obj in objectslist:
            faces.append(obj.Shape.Faces)
        sh = Part.makeShell(faces)
        if sh:
            if sh.Faces:
                newob = FreeCAD.ActiveDocument.addObject("Part::Feature","Shell")
                newob.Shape = sh
                addList.append(newobj)
                deleteList.extend(objectslist)
                return newobj
        return None
        
    def joinFaces(objectslist):
        """makes one big face from selected objects, if possible"""
        faces = []
        for obj in objectslist:
            faces.append(obj.Shape.Faces)
        u = faces.pop(0)
        for f in faces:
            u = u.fuse(f)
        if DraftGeomUtils.isCoplanar(faces):
            u = DraftGeomUtils.concatenate(u)
            if not DraftGeomUtils.hasCurves(u):
                # several coplanar and non-curved faces: they can becoem a Draft wire
                newobj = makeWire(u.Wires[0],closed=True,face=True)
            else:
                # if not possible, we do a non-parametric union
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Union")
                newobj.Shape = u
            addList.append(newobj)
            deleteList.extend(objectslist)
            return newobj
        return None
   
    def makeSketchFace(obj):
        """Makes a Draft face out of a sketch"""
        newobj = makeWire(obj.Shape,closed=True)
        if newobj:
            newobj.Base = obj
            obj.ViewObject.Visibility = False
            addList.append(newobj)
            return newobj
        return None
        
    def makeFaces(objectslist):
        """make a face from every closed wire in the list"""
        result = False
        for o in objectslist:
            for w in o.Shape.Wires:
                try:
                    f = Part.Face(w)
                except:
                    pass
                else:
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Face")
                    newobj.Shape = f
                    addList.append(newobj)
                    result = True
                    if not o in deleteList:
                        deleteList.append(o)
        return result

    def makeWires(objectslist):
        """joins edges in the given objects list into wires"""
        edges = []
        for o in objectslist:
            for e in o.Shape.Edges:
                edges.append(e)
        try:
            nedges = DraftGeomUtils.sortEdges(edges[:])
            # for e in nedges: print "debug: ",e.Curve,e.Vertexes[0].Point,e.Vertexes[-1].Point
            w = Part.Wire(nedges)
        except:
            return None
        else:    
            if len(w.Edges) == len(edges):
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Wire")
                newobj.Shape = w
                addList.append(newobj)
                deleteList.extend(objectslist)
                return True
        return None

    # analyzing what we have in our selection

    edges = []
    wires = []
    openwires = []
    faces = []
    groups = []
    parts = []
    curves = []
    facewires = []
    loneedges = []
    meshes = []
    for ob in objects:
        if ob.TypeId == "App::DocumentObjectGroup":
            groups.append(ob)
        elif ob.isDerivedFrom("Part::Feature"):
            parts.append(ob)
            faces.extend(ob.Shape.Faces)
            wires.extend(ob.Shape.Wires)
            edges.extend(ob.Shape.Edges)
            for f in ob.Shape.Faces:
                facewires.extend(f.Wires)
            wirededges = []
            for w in ob.Shape.Wires:
                if len(w.Edges) > 1:
                    for e in w.Edges:
                        wirededges.append(e.hashCode())
                if not w.isClosed():
                    openwires.append(w)
            for e in ob.Shape.Edges:
                if DraftGeomUtils.geomType(e) != "Line":
                    curves.append(e)
                if not e.hashCode() in wirededges:
                    loneedges.append(e)
        elif ob.isDerivedFrom("Mesh::Feature"):
            meshes.append(ob)
    objects = parts

    #print "objects:",objects," edges:",edges," wires:",wires," openwires:",openwires," faces:",faces
    #print "groups:",groups," curves:",curves," facewires:",facewires, "loneedges:", loneedges
    
    if force:
        if force in ["makeCompound","closeGroupWires","makeSolid","closeWire","turnToParts","makeFusion",
                     "makeShell","makeFaces","draftify","joinFaces","makeSketchFace","makeWires"]:
            result = eval(force)(objects)
        else:
            msg(translate("Upgrade: Unknow force method:")+" "+force)
            result = None

    else:
        
        # applying transformations automatically
        
        result = None

        # if we have a group: turn each closed wire inside into a face
        if groups:
            result = closeGroupWires(groups)
            if result: msg(translate("draft", "Found groups: closing each open object inside\n"))
                
        # if we have meshes, we try to turn them into shapes
        elif meshes:
            result = turnToParts(meshes)
            if result: msg(translate("draft", "Found mesh(es): turning into Part shapes\n"))
            
        # we have only faces here, no lone edges
        elif faces and (len(wires) + len(openwires) == len(facewires)):
            
            # we have one shell: we try to make a solid        
            if (len(objects) == 1) and (len(faces) > 3):
                result = makeSolid(objects[0])
                if result: msg(translate("draft", "Found 1 solidificable object: solidifying it\n"))
                
            # we have exactly 2 objects: we fuse them                    
            elif (len(objects) == 2) and (not curves):
                result = makeFusion(objects[0],objects[1])
                if result: msg(translate("draft", "Found 2 objects: fusing them\n"))
                
            # we have many separate faces: we try to make a shell        
            elif (len(objects) > 2) and (len(faces) > 1) and (not loneedges):
                result = makeShell(objects)
                if result: msg(translate("draft", "Found several objects: making a shell\n"))
                
            # we have faces: we try to join them if they are coplanar
            elif len(faces) > 1:
                result = joinFaces(objects)
                if result: msg(translate("draft", "Found several coplanar objects or faces: making one face\n"))
            
            # only one object: if not parametric, we "draftify" it
            elif len(objects) == 1 and (not objects[0].isDerivedFrom("Part::Part2DObjectPython")):
                result = draftify(objects[0])
                if result: msg(translate("draft", "Found 1 non-parametric objects: draftifying it\n"))
                
        # we have only closed wires, no faces
        elif wires and (not faces) and (not openwires):
        
            # we have a sketch: Extract a face
            if (len(objects) == 1) and objects[0].isDerivedFrom("Sketcher::SketchObject"):
                result = makeSketchFace(objects[0])
                if result: msg(translate("draft", "Found 1 closed sketch object: making a face from it\n"))

            # only closed wires
            else:
                result = makeFaces(objects)
                if result: msg(translate("draft", "Found closed wires: making faces\n"))

        # special case, we have only one open wire. We close it, unless it has only 1 edge!"
        elif (len(openwires) == 1) and (not faces) and (not loneedges):
            result = closeWire(objects[0])
            if result: msg(translate("draft", "Found 1 open wire: closing it\n"))
                        
        # only open wires and edges: we try to join their edges
        elif openwires and (not wires) and (not faces):
            result = makeWires(objects)
            if result: msg(translate("draft", "Found several open wires: joining them\n"))
            
        # only loneedges: we try to join them
        elif loneedges and (not facewires):
            result = makeWires(objects)
            if result: msg(translate("draft", "Found several edges: wiring them\n"))

        # all other cases, if more than 1 object, make a compound
        elif (len(objects) > 1):
            result = makeCompound(objects)
            if result: msg(translate("draft", "Found several non-treatable objects: making compound\n"))
            
        # no result has been obtained
        if not result:
            msg(translate("draft", "Unable to upgrade these objects.\n"))
            
    if delete:
        names = []
        for o in deleteList:
            names.append(o.Name)
        deleteList = []
        for n in names:
            FreeCAD.ActiveDocument.removeObject(n)

    return [addList,deleteList]
    
def downgrade(objects,delete=False,force=None):
    """downgrade(objects,delete=False,force=None): Downgrades the given object(s) (can be
    an object or a list of objects). If delete is True, old objects are deleted.
    The force attribute can be used to
    force a certain way of downgrading. It can be: explode, shapify, subtr,
    splitFaces, cut2, getWire, splitWires.
    Returns a dictionnary containing two lists, a list of new objects and a list 
    of objects to be deleted"""
    
    import Part, DraftGeomUtils
    from DraftTools import msg,translate
    
    if not isinstance(objects,list):
        objects = [objects]

    global deleteList, newList
    deleteList = []
    addList = []
        
    # actions definitions
    
    def explode(obj):
        """explodes a Draft block"""
        pl = obj.Placement
        newobj = []
        for o in obj.Components:
            o.ViewObject.Visibility = True
            o.Placement = o.Placement.multiply(pl)
        if newobj:
            deleteList(obj)
            return newobj
        return None
            
    def cut2(objects):
        """cuts first object from the last one"""
        newobj = cut(objects[0],objects[1])
        if newobj:
            addList.append(newobj)
            return newobj
        return None
        
    def splitFaces(objects):
        """split faces contained in objects into new objects"""
        result = False
        for o in objects:
            if o.Shape.Faces:
                for f in o.Shape.Faces:
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Face")
                    newobj.Shape = f
                    addList.append(newobj)
                result = True
                deleteList.append(o)
        return result
        
    def subtr(objects):
        """subtracts objects from the first one"""
        faces = []
        for o in objects:
            if o.Shape.Faces:
                faces.append(o.Shape.Faces)
                deleteList.append(o)
        u = faces.pop(0)
        for f in faces:
            u = u.cut(f)
        if not u.isNull():
            newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Subtraction")
            newobj.Shape = u
            addList.append(newobj)
            return newobj
        return None
        
    def getWire(obj):
        """gets the wire from a face object"""
        result = False
        for w in obj.Shape.Faces[0].Wires:
            newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Wire")
            newobj.Shape = w
            addList.append(newobj)
            result = True
        deleteList.append(obj)
        return result
        
    def splitWires(objects):
        """splits the wires contained in objects into edges"""
        result = False
        for o in objects:
            if o.Shape.Edges:
                for e in o.Shape.Edges:
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Edge")
                    newobj.Shape = e
                    addList.append(newobj)
                deleteList.append(o)
                result = True
        return result
        
    # analyzing objects
        
    faces = []
    edges = []
    onlyedges = True
    parts = []

    for o in objects:
        if o.isDerivedFrom("Part::Feature"):
            for f in o.Shape.Faces:
                faces.append(f)
            for e in o.Shape.Edges:
                edges.append(e)
            if o.Shape.ShapeType != "Edge":
                onlyedges = False
            parts.append(o)
    objects = parts

    if force:
        if force in ["explode","shapify","subtr","splitFaces","cut2","getWire","splitWires"]:
            result = eval(force)(objects)
        else:
            msg(translate("Upgrade: Unknow force method:")+" "+force)
            result = None

    else:

        # applying transformation automatically

        # we have a block, we explode it
        if (len(objects) == 1) and (getType(objects[0]) == "Block"):
            result = explode(objects[0])
            if result: msg(translate("draft", "Found 1 block: exploding it\n"))
            
        # special case, we have one parametric object: we "de-parametrize" it
        elif (len(objects) == 1) and (objects[0].isDerivedFrom("Part::Feature")) and ("Base" in objects[0].PropertiesList):
            result = shapify(objects[0])
            if result: msg(translate("draft", "Found 1 parametric object: breaking its dependencies\n"))

        # we have only 2 objects: cut 2nd from 1st
        elif len(objects) == 2:
            result = cut2(objects)
            if result: msg(translate("draft", "Found 2 objects: subtracting them\n"))

        elif (len(faces) > 1):

            # one object with several faces: split it
            if len(objects) == 1:
                result = splitFaces(objects)
                if result: msg(translate("draft", "Found several faces: splitting them\n"))

            # several objects: remove all the faces from the first one
            else:
                result = subtr(objects)
                if result: msg(translate("draft", "Found several objects: subtracting them from the first one\n"))

        # only one face: we extract its wires
        elif (len(faces) > 0):
            result = getWire(objects[0])
            if result: msg(translate("draft", "Found 1 face: extracting its wires\n"))

        # no faces: split wire into single edges
        elif not onlyedges:
            result = splitWires(objects)
            if result: msg(translate("draft", "Found only wires: extracting their edges\n"))

        # no result has been obtained
        if not result:
            msg(translate("draft", "No more downgrade possible\n"))
            
    if delete:
        names = []
        for o in deleteList:
            names.append(o.Name)
        deleteList = []
        for n in names:
            FreeCAD.ActiveDocument.removeObject(n)

    return [addList,deleteList]


#---------------------------------------------------------------------------
# Python Features definitions
#---------------------------------------------------------------------------

class _DraftObject:
    "The base class for Draft objects"
    def __init__(self,obj,tp="Unknown"):
        obj.Proxy = self
        self.Type = tp

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state   

    def execute(self,obj):
        pass

    def onChanged(self, fp, prop):
        pass 

class _ViewProviderDraft:
    "The base class for Draft Viewproviders"
        
    def __init__(self, vobj):
        from DraftTools import translate
        vobj.Proxy = self
        self.Object = vobj.Object
        vobj.addProperty("App::PropertyEnumeration","Pattern",
                        "Draft","Defines a hatch pattern")
        vobj.addProperty("App::PropertyFloat","PatternSize",
                        "Draft","Sets the size of the pattern")
        vobj.Pattern = [str(translate("draft","None"))]+svgpatterns().keys()
        vobj.PatternSize = 1

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None
        
    def attach(self,vobj):
        self.texture = None
        self.texcoords = None
        self.Object = vobj.Object
        return

    def updateData(self, obj, prop):
        return

    def getDisplayModes(self, vobj):
        modes=[]
        return modes

    def setDisplayMode(self, mode):
        return mode

    def onChanged(self, vobj, prop):
        # treatment of patterns and image textures
        if prop in ["TextureImage","Pattern"]:
            if hasattr(self.Object,"Shape"):
                if self.Object.Shape.Faces:
                    from pivy import coin
                    from PyQt4 import QtCore
                    path = None
                    if hasattr(vobj,"TextureImage"):
                        if vobj.TextureImage:
                            path = vobj.TextureImage
                    if not path:
                        if hasattr(vobj,"Pattern"):
                            if str(vobj.Pattern) in svgpatterns().keys():
                                path = svgpatterns()[vobj.Pattern][1]
                    if path:
                        r = vobj.RootNode.getChild(2).getChild(0).getChild(2)
                        i = QtCore.QFileInfo(path)
                        if self.texture:
                            r.removeChild(self.texture)
                            self.texture = None
                        if self.texcoords:
                            r.removeChild(self.texcoords)
                            self.texcoords = None
                        if i.exists():
                            size = None
                            if ":/patterns" in path:
                                size = getParam("HatchPatternResolution",128)
                                if not size:
                                    size = 128
                            im = loadTexture(path, size)
                            if im:
                                self.texture = coin.SoTexture2()
                                self.texture.image = im
                                r.insertChild(self.texture,1)
                                if size:
                                    s =1
                                    if hasattr(vobj,"PatternSize"):
                                        if vobj.PatternSize:
                                            s = vobj.PatternSize
                                    self.texcoords = coin.SoTextureCoordinatePlane()
                                    self.texcoords.directionS.setValue(s,0,0)
                                    self.texcoords.directionT.setValue(0,s,0)
                                    r.insertChild(self.texcoords,2)
        elif prop == "PatternSize":
            if hasattr(self,"texcoords"):
                if self.texcoords:
                    s = 1
                    if vobj.PatternSize:
                        s = vobj.PatternSize
                    self.texcoords.directionS.setValue(s,0,0)
                    self.texcoords.directionT.setValue(0,s,0)                
        return

    def execute(self,vobj):
        return

    def setEdit(self,vobj,mode):
        FreeCADGui.runCommand("Draft_Edit")
        return True

    def unsetEdit(self,vobj,mode):
        if FreeCAD.activeDraftCommand:
            FreeCAD.activeDraftCommand.finish()
        FreeCADGui.Control.closeDialog()
        return False
    
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

class _ViewProviderDraftAlt(_ViewProviderDraft):
    "a view provider that doesn't swallow its base object"
    
    def __init__(self,vobj):
        vobj.Proxy = self
        self.Object = vobj.Object

    def claimChildren(self):
        return []

class _ViewProviderDraftPart(_ViewProviderDraftAlt):
    "a view provider that displays a Part icon instead of a Draft icon"
    
    def __init__(self,vobj):
        vobj.Proxy = self
        self.Object = vobj.Object

    def getIcon(self):
        return ":/icons/Tree_Part.svg"


class _Dimension(_DraftObject):
    "The Draft Dimension object"
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"Dimension")
        obj.addProperty("App::PropertyVector","Start","Draft",
                        "Startpoint of dimension")
        obj.addProperty("App::PropertyVector","End","Draft",
                        "Endpoint of dimension")
        obj.addProperty("App::PropertyVector","Normal","Draft",
                        "the normal direction of this dimension")
        obj.addProperty("App::PropertyVector","Dimline","Draft",
                        "Point through which the dimension line passes")
        obj.addProperty("App::PropertyLink","Support","Draft",
                        "The object measured by this dimension")
        obj.addProperty("App::PropertyLinkSubList","LinkedGeometry","Draft",
                        "The geometry this dimension is linked to")
        obj.addProperty("App::PropertyLength","Distance","Draft","The measurement of this dimension")
        obj.Start = FreeCAD.Vector(0,0,0)
        obj.End = FreeCAD.Vector(1,0,0)
        obj.Dimline = FreeCAD.Vector(0,1,0)
        
    def onChanged(self,obj,prop):
        obj.setEditorMode('Distance',1)
        obj.setEditorMode('Normal',2)
        obj.setEditorMode('Support',2)
                            
    def execute(self, obj):
        if obj.LinkedGeometry:
            if "Edge" in obj.LinkedGeometry[0][1]:
                n = int(obj.LinkedGeometry[0][1][4:])-1
                if len(obj.LinkedGeometry) > 1:
                    c = obj.LinkedGeometry[0][0].Shape.Edges[n].Curve.Center
                    r = obj.LinkedGeometry[0][0].Shape.Edges[n].Curve.Radius
                    ray = DraftVecUtils.scaleTo(obj.Dimline.sub(c),r)
                    if "Center" in obj.LinkedGeometry[1][1]:
                        obj.Start = c
                        obj.End = c.add(ray)
                    elif "Diameter" in obj.LinkedGeometry[1][1]:
                        obj.Start = c.add(ray.negative())
                        obj.End = c.add(ray)
                else:
                    obj.Start = obj.LinkedGeometry[0][0].Shape.Edges[n].Vertexes[0].Point
                    obj.End = obj.LinkedGeometry[0][0].Shape.Edges[n].Vertexes[-1].Point
            elif "Vertex" in obj.LinkedGeometry[0][1]:
                n = int(obj.LinkedGeometry[0][1][6:])-1
                obj.Start = obj.LinkedGeometry[0][0].Shape.Vertexes[n].Point
                if len(obj.LinkedGeometry) > 1:
                    if "Vertex" in obj.LinkedGeometry[1][1]:
                        n = int(obj.LinkedGeometry[1][1][6:])-1
                        obj.End = obj.LinkedGeometry[1][0].Shape.Vertexes[n].Point
        if obj.ViewObject:
            obj.ViewObject.update()


class _ViewProviderDimension(_ViewProviderDraft):
    "A View Provider for the Draft Dimension object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyLength","FontSize","Draft","Font size")
        obj.addProperty("App::PropertyLength","ArrowSize","Draft","Arrow size")
        obj.addProperty("App::PropertyLength","TextSpacing","Draft","The spacing between the text and the dimension line")
        obj.addProperty("App::PropertyEnumeration","ArrowType","Draft","Arrow type")
        obj.addProperty("App::PropertyString","FontName","Draft","Font name")
        obj.addProperty("App::PropertyLength","LineWidth","Draft","Line width")
        obj.addProperty("App::PropertyColor","LineColor","Draft","Line color")
        obj.addProperty("App::PropertyLength","ExtLines","Draft","Length of the extension lines")
        obj.addProperty("App::PropertyBool","FlipArrows","Draft","Rotate the dimension arrows 180 degrees")
        obj.addProperty("App::PropertyVector","TextPosition","Draft","The position of the text. Leave (0,0,0) for automatic position")
        obj.addProperty("App::PropertyString","Override","Draft","Text override. Use $dim to insert the dimension length")
        obj.FontSize=getParam("textheight",0.20)
        obj.TextSpacing=getParam("dimspacing",0.05)
        obj.FontName=getParam("textfont","")
        obj.ArrowSize = getParam("arrowsize",0.1)
        obj.ArrowType = arrowtypes
        obj.ArrowType = arrowtypes[getParam("dimsymbol",0)]
        obj.ExtLines = getParam("extlines",0.3)
        _ViewProviderDraft.__init__(self,obj)

    def attach(self, vobj):
        "called on object creation"
        from pivy import coin
        self.Object = vobj.Object
        self.color = coin.SoBaseColor()
        self.font = coin.SoFont()
        self.font3d = coin.SoFont()
        self.text = coin.SoAsciiText()
        self.text3d = coin.SoText2()
        self.textpos = coin.SoTransform()
        self.text.justification = self.text3d.justification = coin.SoAsciiText.CENTER
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
        self.trans1 = coin.SoTransform()
        self.coord2 = coin.SoCoordinate3()
        self.trans2 = coin.SoTransform()
        self.marks = coin.SoSeparator()     
        self.drawstyle = coin.SoDrawStyle()    
        self.line = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        self.coords = coin.SoCoordinate3()
        self.node = coin.SoGroup()
        self.node.addChild(self.color)
        self.node.addChild(self.drawstyle)
        self.node.addChild(self.coords)
        self.node.addChild(self.line)
        self.node.addChild(self.marks)
        self.node.addChild(label)
        self.node3d = coin.SoGroup()
        self.node3d.addChild(self.color)
        self.node3d.addChild(self.drawstyle)
        self.node3d.addChild(self.coords)
        self.node3d.addChild(self.line)
        self.node3d.addChild(self.marks)
        self.node3d.addChild(label3d)
        vobj.addDisplayMode(self.node,"2D")
        vobj.addDisplayMode(self.node3d,"3D")
        self.updateData(vobj.Object,"Start")
        self.onChanged(vobj,"FontSize")
        self.onChanged(vobj,"FontName")
        self.onChanged(vobj,"ArrowType")
        self.onChanged(vobj,"LineColor")
            
    def updateData(self, obj, prop):
        "called when the base object is changed"
        if prop in ["Start","End","Dimline"]:
            
            if obj.Start == obj.End:
                return
                
            if not hasattr(self,"node"):
                return
                
            import Part, DraftGeomUtils
            from pivy import coin
            
            # calculate the 4 points
            self.p1 = obj.Start
            self.p4 = obj.End
            base = Part.Line(self.p1,self.p4).toShape()
            proj = DraftGeomUtils.findDistance(obj.Dimline,base)
            if proj:
                self.p2 = self.p1.add(proj.negative())
                self.p3 = self.p4.add(proj.negative())
                dmax = obj.ViewObject.ExtLines
                if dmax and (proj.Length > dmax):
                    self.p1 = self.p2.add(DraftVecUtils.scaleTo(proj,dmax))
                    self.p4 = self.p3.add(DraftVecUtils.scaleTo(proj,dmax))
            else:
                self.p2 = self.p1
                self.p3 = self.p4
                proj = (self.p3.sub(self.p2)).cross(Vector(0,0,1))

            # calculate the arrows positions
            self.trans1.translation.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.coord1.point.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.trans2.translation.setValue((self.p3.x,self.p3.y,self.p3.z))
            self.coord2.point.setValue((self.p3.x,self.p3.y,self.p3.z))
            
            # calculate the text position and orientation
            if DraftVecUtils.isNull(obj.Normal):
                if not proj: 
                    norm = Vector(0,0,1)
                else: 
                    norm = (self.p3.sub(self.p2).cross(proj)).negative()
            else:
                norm = obj.Normal
            if not DraftVecUtils.isNull(norm):
                norm.normalize()
            u = self.p3.sub(self.p2)
            u.normalize()
            v1 = norm.cross(u)
            rot1 = FreeCAD.Placement(DraftVecUtils.getPlaneRotation(u,v1,norm)).Rotation.Q
            if hasattr(obj.ViewObject,"FlipArrows"):
                if obj.ViewObject.FlipArrows:
                    u = u.negative()
            v2 = norm.cross(u)
            rot2 = FreeCAD.Placement(DraftVecUtils.getPlaneRotation(u,v2,norm)).Rotation.Q
            self.trans1.rotation.setValue((rot2[0],rot2[1],rot2[2],rot2[3]))
            self.trans2.rotation.setValue((rot2[0],rot2[1],rot2[2],rot2[3]))
            if hasattr(obj.ViewObject,"TextSpacing"):
                offset = DraftVecUtils.scaleTo(v1,obj.ViewObject.TextSpacing)
            else:
                offset = DraftVecUtils.scaleTo(v1,0.05)
            
            # setting text
            try:
                m = obj.ViewObject.DisplayMode
            except:
                m = ["2D","3D"][getParam("dimstyle",0)]
            if m== "3D":
                offset = offset.negative()
            tbase = (self.p2.add((self.p3.sub(self.p2).multiply(0.5)))).add(offset)
            if hasattr(obj.ViewObject,"TextPosition"):
                if not DraftVecUtils.isNull(obj.ViewObject.TextPosition):
                    tbase = obj.ViewObject.TextPosition
            self.textpos.translation.setValue([tbase.x,tbase.y,tbase.z])
            self.textpos.rotation = coin.SbRotation(rot1[0],rot1[1],rot1[2],rot1[3])
            
            # set text value
            l = self.p3.sub(self.p2).Length
            fstring = "%." + str(getParam("dimPrecision",2)) + "f"
            self.string = (fstring % l)
            if obj.ViewObject.Override:
                self.string = unicode(obj.ViewObject.Override).encode("latin1").replace("$dim",self.string)
            self.text.string = self.text3d.string = self.string

            # set the distance property
            if round(obj.Distance,precision()) != round(l,precision()):
                obj.Distance = l
                
            # set the lines
            if m == "3D":
                # calculate the spacing of the text
                textsize = (len(self.string)*obj.ViewObject.FontSize)/4
                spacing = ((self.p3.sub(self.p2)).Length/2) - textsize
                self.p2a = self.p2.add(DraftVecUtils.scaleTo(self.p3.sub(self.p2),spacing))
                self.p2b = self.p3.add(DraftVecUtils.scaleTo(self.p2.sub(self.p3),spacing))
                self.coords.point.setValues([[self.p1.x,self.p1.y,self.p1.z],
                                             [self.p2.x,self.p2.y,self.p2.z],
                                             [self.p2a.x,self.p2a.y,self.p2a.z],
                                             [self.p2b.x,self.p2b.y,self.p2b.z],
                                             [self.p3.x,self.p3.y,self.p3.z],
                                             [self.p4.x,self.p4.y,self.p4.z]])
                #self.line.numVertices.setValues([3,3])
                self.line.coordIndex.setValues(0,7,(0,1,2,-1,3,4,5))
            else:
                self.coords.point.setValues([[self.p1.x,self.p1.y,self.p1.z],
                                             [self.p2.x,self.p2.y,self.p2.z],
                                             [self.p3.x,self.p3.y,self.p3.z],
                                             [self.p4.x,self.p4.y,self.p4.z]])
                #self.line.numVertices.setValue(4)
                self.line.coordIndex.setValues(0,4,(0,1,2,3))

    def onChanged(self, vobj, prop):
        "called when a view property has changed"
        
        if prop in ["ExtLines","TextSpacing","DisplayMode","Override","FlipArrows"]:
            self.updateData(vobj.Object,"Start")
        elif prop == "FontSize":
            if hasattr(self,"font"):
                self.font.size = vobj.FontSize
            if hasattr(self,"font3d"):
                self.font3d.size = vobj.FontSize*100
        elif prop == "FontName":
            if hasattr(self,"font") and hasattr(self,"font3d"):
                self.font.name = self.font3d.name = str(vobj.FontName)
        elif prop == "LineColor":
            if hasattr(self,"color"):
                c = vobj.LineColor
                self.color.rgb.setValue(c[0],c[1],c[2])
        elif prop == "LineWidth":
            if hasattr(self,"drawstyle"):
                self.drawstyle.lineWidth = vobj.LineWidth
        elif prop in ["ArrowSize","ArrowType"]:
            if hasattr(self,"node") and hasattr(self,"p2"):
                from pivy import coin
                
                if not hasattr(vobj,"ArrowType"):
                    return
                    
                if self.p3.x < self.p2.x:
                    inv = False
                else:
                    inv = True
                
                # set scale
                symbol = arrowtypes.index(vobj.ArrowType)
                s = vobj.ArrowSize
                self.trans1.scaleFactor.setValue((s,s,s))
                self.trans2.scaleFactor.setValue((s,s,s))
                
                # remove existing nodes
                self.node.removeChild(self.marks)
                self.node3d.removeChild(self.marks)
                
                # set new nodes
                self.marks = coin.SoSeparator()
                self.marks.addChild(self.color)
                s1 = coin.SoSeparator()
                if symbol == "Circle":
                    s1.addChild(self.coord1)
                else:
                    s1.addChild(self.trans1)
                s1.addChild(dimSymbol(symbol,invert=not(inv)))
                self.marks.addChild(s1)
                s2 = coin.SoSeparator()
                if symbol == "Circle":
                    s2.addChild(self.coord2)
                else:
                    s2.addChild(self.trans2)
                s2.addChild(dimSymbol(symbol,invert=inv)) 
                self.marks.addChild(s2)      
                self.node.insertChild(self.marks,2)
                self.node3d.insertChild(self.marks,2)

    def getDisplayModes(self,vobj):
        return ["2D","3D"]

    def getDefaultDisplayMode(self):
        if hasattr(self,"defaultmode"):
            return self.defaultmode
        else:
            return ["2D","3D"][getParam("dimstyle",0)]

    def setDisplayMode(self,mode):
        return mode

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

    def __getstate__(self):
        return self.Object.ViewObject.DisplayMode
    
    def __setstate__(self,state):
        if state:
            self.defaultmode = state
            self.setDisplayMode(state)

class _AngularDimension(_DraftObject):
    "The Draft AngularDimension object"
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"AngularDimension")
        obj.addProperty("App::PropertyAngle","FirstAngle","Draft",
                        "Start angle of the dimension")
        obj.addProperty("App::PropertyAngle","LastAngle","Draft",
                        "End angle of the dimension")
        obj.addProperty("App::PropertyVector","Dimline","Draft",
                        "Point through which the dimension line passes")
        obj.addProperty("App::PropertyVector","Center","Draft",
                        "The center point of this dimension")
        obj.addProperty("App::PropertyVector","Normal","Draft",
                        "The normal direction of this dimension")
        obj.addProperty("App::PropertyLink","Support","Draft",
                        "The object measured by this dimension")
        obj.addProperty("App::PropertyLinkSubList","LinkedGeometry","Draft",
                        "The geometry this dimension is linked to")
        obj.addProperty("App::PropertyAngle","Angle","Draft","The measurement of this dimension")
        obj.FirstAngle = 0
        obj.LastAngle = 90
        obj.Dimline = FreeCAD.Vector(0,1,0)
        obj.Center = FreeCAD.Vector(0,0,0)
        
    def onChanged(self,obj,prop):
        obj.setEditorMode('Angle',1)
        obj.setEditorMode('Normal',2)
        obj.setEditorMode('Support',2)
        
    def execute(self, fp):
        if fp.ViewObject:
            fp.ViewObject.update()

class _ViewProviderAngularDimension(_ViewProviderDraft):
    "A View Provider for the Draft Angular Dimension object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyLength","FontSize","Draft","Font size")
        obj.addProperty("App::PropertyString","FontName","Draft","Font name")
        obj.addProperty("App::PropertyLength","ArrowSize","Draft","Arrow size")
        obj.addProperty("App::PropertyLength","TextSpacing","Draft","The spacing between the text and the dimension line")
        obj.addProperty("App::PropertyEnumeration","ArrowType","Draft","Arrow type")
        obj.addProperty("App::PropertyLength","LineWidth","Draft","Line width")
        obj.addProperty("App::PropertyColor","LineColor","Draft","Line color")
        obj.addProperty("App::PropertyBool","FlipArrows","Draft","Rotate the dimension arrows 180 degrees")
        obj.addProperty("App::PropertyVector","TextPosition","Draft","The position of the text. Leave (0,0,0) for automatic position")
        obj.addProperty("App::PropertyString","Override","Draft","Text override. Use 'dim' to insert the dimension length")
        obj.FontSize=getParam("textheight",0.20)
        obj.FontName=getParam("textfont","")
        obj.TextSpacing=getParam("dimspacing",0.05)
        obj.ArrowSize = getParam("arrowsize",0.1)
        obj.ArrowType = arrowtypes
        obj.ArrowType = arrowtypes[getParam("dimsymbol",0)]
        obj.Override = ''
        _ViewProviderDraft.__init__(self,obj)

    def attach(self, vobj):
        from pivy import coin
        self.Object = vobj.Object
        self.color = coin.SoBaseColor()
        self.color.rgb.setValue(vobj.LineColor[0],vobj.LineColor[1],vobj.LineColor[2])
        self.font = coin.SoFont()
        self.font3d = coin.SoFont()
        self.text = coin.SoAsciiText()
        self.text3d = coin.SoText2()
        self.text.justification = self.text3d.justification = coin.SoAsciiText.CENTER
        self.textpos = coin.SoTransform()
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
        self.trans1 = coin.SoTransform()
        self.coord2 = coin.SoCoordinate3()
        self.trans2 = coin.SoTransform()
        self.marks = coin.SoSeparator()
        self.drawstyle = coin.SoDrawStyle()       
        self.coords = coin.SoCoordinate3()
        self.arc = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        self.node = coin.SoGroup()
        self.node.addChild(self.color)
        self.node.addChild(self.drawstyle)
        self.node.addChild(self.coords)
        self.node.addChild(self.arc)
        self.node.addChild(self.marks)
        self.node.addChild(label)
        self.node3d = coin.SoGroup()
        self.node3d.addChild(self.color)
        self.node3d.addChild(self.drawstyle)
        self.node3d.addChild(self.coords)
        self.node3d.addChild(self.arc)
        self.node3d.addChild(self.marks)
        self.node3d.addChild(label3d)
        vobj.addDisplayMode(self.node,"2D")
        vobj.addDisplayMode(self.node3d,"3D")
        self.updateData(vobj.Object,None)
        self.onChanged(vobj,"FontSize")
        self.onChanged(vobj,"FontName")
        self.onChanged(vobj,"ArrowType")
        self.onChanged(vobj,"LineColor")

    def updateData(self, obj, prop):
        if hasattr(self,"arc"):
            from pivy import coin
            import Part, DraftGeomUtils
            text = None
            ivob = None
            arcsegs = 24
            
            # calculate the arc data
            if DraftVecUtils.isNull(obj.Normal):
                norm = Vector(0,0,1)
            else:
                norm = obj.Normal
            radius = (obj.Dimline.sub(obj.Center)).Length
            self.circle = Part.makeCircle(radius,obj.Center,norm,obj.FirstAngle,obj.LastAngle)
            self.p2 = self.circle.Vertexes[0].Point
            self.p3 = self.circle.Vertexes[-1].Point
            mp = DraftGeomUtils.findMidpoint(self.circle.Edges[0])
            ray = mp.sub(obj.Center)
            
            # set text value
            if obj.LastAngle > obj.FirstAngle:
                a = obj.LastAngle - obj.FirstAngle
            else:
                a = (360 - obj.FirstAngle) + obj.LastAngle
            fstring = "%." + str(getParam("dimPrecision",2)) + "f"
            self.string = (fstring % a)
            self.string += " d"
            if obj.ViewObject.Override:
                self.string = unicode(obj.ViewObject.Override).encode("latin1").replace("$dim",self.string)
            self.text.string = self.text3d.string = self.string
            
            # check display mode
            try:
                m = obj.ViewObject.DisplayMode
            except:
                m = ["2D","3D"][getParam("dimstyle",0)]
    
            # set the arc
            if m == "3D":
                # calculate the spacing of the text
                spacing = (len(self.string)*obj.ViewObject.FontSize)/8
                pts1 = []
                cut = None
                pts2 = []
                for i in range(arcsegs+1):
                    p = self.circle.valueAt(self.circle.FirstParameter+((self.circle.LastParameter-self.circle.FirstParameter)/arcsegs)*i)
                    if (p.sub(mp)).Length <= spacing:
                        if cut == None:
                            cut = i
                    else:
                        if cut == None:
                            pts1.append([p.x,p.y,p.z])
                        else:
                            pts2.append([p.x,p.y,p.z])
                self.coords.point.setValues(pts1+pts2)
                i1 = len(pts1)
                i2 = i1+len(pts2)
                self.arc.coordIndex.setValues(0,len(pts1)+len(pts2)+1,range(len(pts1))+[-1]+range(i1,i2))
                if (len(pts1) >= 3) and (len(pts2) >= 3):
                    self.circle1 = Part.Arc(Vector(pts1[0][0],pts1[0][1],pts1[0][2]),Vector(pts1[1][0],pts1[1][1],pts1[1][2]),Vector(pts1[-1][0],pts1[-1][1],pts1[-1][2])).toShape()
                    self.circle2 = Part.Arc(Vector(pts2[0][0],pts2[0][1],pts2[0][2]),Vector(pts2[1][0],pts2[1][1],pts2[1][2]),Vector(pts2[-1][0],pts2[-1][1],pts2[-1][2])).toShape()
            else:
                pts = []
                for i in range(arcsegs+1):
                    p = self.circle.valueAt(self.circle.FirstParameter+((self.circle.LastParameter-self.circle.FirstParameter)/arcsegs)*i)
                    pts.append([p.x,p.y,p.z])
                self.coords.point.setValues(pts)
                self.arc.coordIndex.setValues(0,arcsegs+1,range(arcsegs+1))
            
            # set the arrow coords and rotation
            self.trans1.translation.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.coord1.point.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.trans2.translation.setValue((self.p3.x,self.p3.y,self.p3.z))
            self.coord2.point.setValue((self.p3.x,self.p3.y,self.p3.z))
            # calculate small chords to make arrows look better
            arrowlength = 4*obj.ViewObject.ArrowSize
            u1 = (self.circle.valueAt(self.circle.FirstParameter+arrowlength)).sub(self.circle.valueAt(self.circle.FirstParameter)).normalize()
            u2 = (self.circle.valueAt(self.circle.LastParameter)).sub(self.circle.valueAt(self.circle.LastParameter-arrowlength)).normalize()
            if hasattr(obj.ViewObject,"FlipArrows"):
                if obj.ViewObject.FlipArrows:
                    u1 = u1.negative()
                    u2 = u2.negative()
            w2 = self.circle.Curve.Axis
            w1 = w2.negative()
            v1 = w1.cross(u1)
            v2 = w2.cross(u2)
            q1 = FreeCAD.Placement(DraftVecUtils.getPlaneRotation(u1,v1,w1)).Rotation.Q
            q2 = FreeCAD.Placement(DraftVecUtils.getPlaneRotation(u2,v2,w2)).Rotation.Q
            self.trans1.rotation.setValue((q1[0],q1[1],q1[2],q1[3]))
            self.trans2.rotation.setValue((q2[0],q2[1],q2[2],q2[3]))
            
            # setting text pos & rot
            self.tbase = mp
            if hasattr(obj.ViewObject,"TextPosition"):
                if not DraftVecUtils.isNull(obj.ViewObject.TextPosition):
                    self.tbase = obj.ViewObject.TextPosition

            u3 = ray.cross(norm).normalize()
            v3 = norm.cross(u3)
            r = FreeCAD.Placement(DraftVecUtils.getPlaneRotation(u3,v3,norm)).Rotation
            offset = r.multVec(Vector(0,1,0))
            
            if hasattr(obj.ViewObject,"TextSpacing"):
                offset = DraftVecUtils.scaleTo(offset,obj.ViewObject.TextSpacing)
            else:
                offset = DraftVecUtils.scaleTo(offset,0.05)
            if m == "3D":
                offset = offset.negative()
            self.tbase = self.tbase.add(offset)
            q = r.Q
            self.textpos.translation.setValue([self.tbase.x,self.tbase.y,self.tbase.z])
            self.textpos.rotation = coin.SbRotation(q[0],q[1],q[2],q[3])
                
            # set the angle property
            if round(obj.Angle,precision()) != round(a,precision()):
                obj.Angle = a

    def onChanged(self, vobj, prop):
        if prop == "FontSize":
            if hasattr(self,"font"):
                self.font.size = vobj.FontSize
            if hasattr(self,"font3d"):
                self.font3d.size = vobj.FontSize*100
        elif prop == "FontName":
            if hasattr(self,"font") and hasattr(self,"font3d"):
                self.font.name = self.font3d.name = str(vobj.FontName)
        elif prop == "LineColor":
            if hasattr(self,"color"):
                c = vobj.LineColor
                self.color.rgb.setValue(c[0],c[1],c[2])
        elif prop == "LineWidth":
            if hasattr(self,"drawstyle"):
                self.drawstyle.lineWidth = vobj.LineWidth
        elif prop in ["ArrowSize","ArrowType"]:
            if hasattr(self,"node") and hasattr(self,"p2"):
                from pivy import coin
                
                if not hasattr(vobj,"ArrowType"):
                    return
                
                # set scale
                symbol = arrowtypes.index(vobj.ArrowType)
                s = vobj.ArrowSize
                self.trans1.scaleFactor.setValue((s,s,s))
                self.trans2.scaleFactor.setValue((s,s,s))
                
                # remove existing nodes
                self.node.removeChild(self.marks)
                self.node3d.removeChild(self.marks)
                
                # set new nodes
                self.marks = coin.SoSeparator()
                self.marks.addChild(self.color)
                s1 = coin.SoSeparator()
                if symbol == "Circle":
                    s1.addChild(self.coord1)
                else:
                    s1.addChild(self.trans1)
                s1.addChild(dimSymbol(symbol,invert=False))
                self.marks.addChild(s1)
                s2 = coin.SoSeparator()
                if symbol == "Circle":
                    s2.addChild(self.coord2)
                else:
                    s2.addChild(self.trans2)
                s2.addChild(dimSymbol(symbol,invert=True)) 
                self.marks.addChild(s2)      
                self.node.insertChild(self.marks,2)
                self.node3d.insertChild(self.marks,2)
        else:
            self.updateData(vobj.Object, None)

    def getDisplayModes(self,obj):
        modes=[]
        modes.extend(["2D","3D"])
        return modes

    def getDefaultDisplayMode(self):
        if hasattr(self,"defaultmode"):
            return self.defaultmode
        else:
            return ["2D","3D"][getParam("dimstyle",0)]

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

    def __getstate__(self):
        return self.Object.ViewObject.DisplayMode

    def __setstate__(self,state):
        if state:
            self.defaultmode = state
            self.setDisplayMode(state)


class _Rectangle(_DraftObject):
    "The Rectangle object"
        
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"Rectangle")
        obj.addProperty("App::PropertyDistance","Length","Draft","Length of the rectangle")
        obj.addProperty("App::PropertyDistance","Height","Draft","Height of the rectange")
        obj.addProperty("App::PropertyDistance","FilletRadius","Draft","Radius to use to fillet the corners")
        obj.addProperty("App::PropertyDistance","ChamferSize","Draft","Size of the chamfer to give to the corners")
        obj.Length=1
        obj.Height=1

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        if prop in ["Length","Height"]:
            self.createGeometry(fp)
                        
    def createGeometry(self,fp):
        if (fp.Length != 0) and (fp.Height != 0):
            import Part, DraftGeomUtils
            plm = fp.Placement
            p1 = Vector(0,0,0)
            p2 = Vector(p1.x+fp.Length,p1.y,p1.z)
            p3 = Vector(p1.x+fp.Length,p1.y+fp.Height,p1.z)
            p4 = Vector(p1.x,p1.y+fp.Height,p1.z)
            shape = Part.makePolygon([p1,p2,p3,p4,p1])
            if "ChamferSize" in fp.PropertiesList:
                if fp.ChamferSize != 0:
                    w = DraftGeomUtils.filletWire(shape,fp.ChamferSize,chamfer=True)
                    if w:
                        shape = w  
            if "FilletRadius" in fp.PropertiesList:
                if fp.FilletRadius != 0:
                    w = DraftGeomUtils.filletWire(shape,fp.FilletRadius)
                    if w:
                        shape = w
            shape = Part.Face(shape)
            fp.Shape = shape
            fp.Placement = plm

class _ViewProviderRectangle(_ViewProviderDraft):
    def __init__(self,vobj):
        _ViewProviderDraft.__init__(self,vobj)
        vobj.addProperty("App::PropertyFile","TextureImage",
                        "Draft","Defines a texture image (overrides hatch patterns)")

class _Circle(_DraftObject):
    "The Circle object"
        
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"Circle")
        obj.addProperty("App::PropertyAngle","FirstAngle","Draft",
                        "Start angle of the arc")
        obj.addProperty("App::PropertyAngle","LastAngle","Draft",
                        "End angle of the arc (for a full circle, give it same value as First Angle)")
        obj.addProperty("App::PropertyDistance","Radius","Draft",
                        "Radius of the circle")

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
        
class _Ellipse(_DraftObject):
    "The Circle object"
        
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"Ellipse")
        obj.addProperty("App::PropertyDistance","MinorRadius","Draft",
                        "The minor radius of the ellipse")
        obj.addProperty("App::PropertyDistance","MajorRadius","Draft",
                        "The major radius of the ellipse")

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        if prop in ["MinorRadius","MajorRadius"]:
            self.createGeometry(fp)
                        
    def createGeometry(self,fp):
        import Part
        plm = fp.Placement
        if fp.MajorRadius < fp.MinorRadius:
            msg(translate("Error: Major radius is smaller than the minor radius"))
            return
        if fp.MajorRadius and fp.MinorRadius:
            shape = Part.Ellipse(Vector(0,0,0),fp.MajorRadius,fp.MinorRadius).toShape()
            shape = Part.Wire(shape)
            shape = Part.Face(shape)
            fp.Shape = shape
            fp.Placement = plm

class _Wire(_DraftObject):
    "The Wire object"
        
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"Wire")
        obj.addProperty("App::PropertyVectorList","Points","Draft",
                        "The vertices of the wire")
        obj.addProperty("App::PropertyBool","Closed","Draft",
                        "If the wire is closed or not")
        obj.addProperty("App::PropertyLink","Base","Draft",
                        "The base object is the wire is formed from 2 objects")
        obj.addProperty("App::PropertyLink","Tool","Draft",
                        "The tool object is the wire is formed from 2 objects")
        obj.addProperty("App::PropertyVector","Start","Draft",
                        "The start point of this line")
        obj.addProperty("App::PropertyVector","End","Draft",
                        "The end point of this line")
        obj.addProperty("App::PropertyDistance","FilletRadius","Draft","Radius to use to fillet the corners")
        obj.addProperty("App::PropertyDistance","ChamferSize","Draft","Size of the chamfer to give to the corners")
        obj.Closed = False

    def execute(self, fp):
        self.createGeometry(fp)
        
    def updateProps(self,fp):
        "sets the start and end properties"
        pl = FreeCAD.Placement(fp.Placement)
        if len(fp.Points) >= 2:
            displayfpstart = pl.multVec(fp.Points[0])
            displayfpend = pl.multVec(fp.Points[-1])
            if fp.Start != displayfpstart:
                fp.Start = displayfpstart
            if fp.End != displayfpend:
                fp.End = displayfpend
        if len(fp.Points) > 2:
            fp.setEditorMode('Start',2)
            fp.setEditorMode('End',2)

    def onChanged(self, fp, prop):
        if prop in ["Points","Closed","Base","Tool","FilletRadius"]:
            self.createGeometry(fp)
            if prop == "Points":
                self.updateProps(fp)
        elif prop == "Start":
            pts = fp.Points
            invpl = FreeCAD.Placement(fp.Placement).inverse()
            realfpstart = invpl.multVec(fp.Start)
            if pts:
                if pts[0] != realfpstart:
                    pts[0] = realfpstart
                    fp.Points = pts
        elif prop == "End":
            pts = fp.Points
            invpl = FreeCAD.Placement(fp.Placement).inverse()
            realfpend = invpl.multVec(fp.End)
            if len(pts) > 1:
                if pts[-1] != realfpend:
                    pts[-1] = realfpend
                    fp.Points = pts
        elif prop == "Placement":
            self.updateProps(fp)
                        
    def createGeometry(self,fp):
        import Part, DraftGeomUtils
        plm = fp.Placement
        if fp.Base and (not fp.Tool):
            if fp.Base.isDerivedFrom("Sketcher::SketchObject"):
                shape = fp.Base.Shape.copy()
                if fp.Base.Shape.isClosed():
                    shape = Part.Face(shape)
                fp.Shape = shape
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
                try:
                    shape = Part.Face(shape)
                except:
                    pass
            else:
                edges = []
                pts = fp.Points[1:]
                lp = fp.Points[0]
                for p in pts:
                    if not DraftVecUtils.equals(lp,p):
                        edges.append(Part.Line(lp,p).toShape())
                        lp = p
                try:
                    shape = Part.Wire(edges)
                except:
                    shape = None
                if "ChamferSize" in fp.PropertiesList:
                    if fp.ChamferSize != 0:
                        w = DraftGeomUtils.filletWire(shape,fp.ChamferSize,chamfer=True)
                        if w:
                            shape = w                        
                if "FilletRadius" in fp.PropertiesList:
                    if fp.FilletRadius != 0:
                        w = DraftGeomUtils.filletWire(shape,fp.FilletRadius)
                        if w:
                            shape = w
            if shape:
                fp.Shape = shape
        fp.Placement = plm

class _ViewProviderWire(_ViewProviderDraft):
    "A View Provider for the Wire object"
    def __init__(self, obj):
        _ViewProviderDraft.__init__(self,obj)
        obj.addProperty("App::PropertyBool","EndArrow","Draft",
                        "Displays a dim symbol at the end of the wire")

    def attach(self, obj):
        from pivy import coin
        self.Object = obj.Object
        col = coin.SoBaseColor()
        col.rgb.setValue(obj.LineColor[0],
                         obj.LineColor[1],
                         obj.LineColor[2])
        self.coords = coin.SoCoordinate3()
        self.pt = coin.SoSeparator()
        self.pt.addChild(col)
        self.pt.addChild(self.coords)
        self.pt.addChild(dimSymbol())
        _ViewProviderDraft.attach(self,obj)
        
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
        _ViewProviderDraft.onChanged(self,vp,prop)
        return

    def claimChildren(self):
        if hasattr(self.Object,"Base"):
            return [self.Object.Base,self.Object.Tool]
        return []
        
class _Polygon(_DraftObject):
    "The Polygon object"
        
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"Polygon")
        obj.addProperty("App::PropertyInteger","FacesNumber","Draft","Number of faces")
        obj.addProperty("App::PropertyDistance","Radius","Draft","Radius of the control circle")
        obj.addProperty("App::PropertyEnumeration","DrawMode","Draft","How the polygon must be drawn from the control circle")
        obj.addProperty("App::PropertyDistance","FilletRadius","Draft","Radius to use to fillet the corners")
        obj.addProperty("App::PropertyDistance","ChamferSize","Draft","Size of the chamfer to give to the corners")
        obj.DrawMode = ['inscribed','circumscribed']
        obj.FacesNumber = 0
        obj.Radius = 1

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        if prop in ["FacesNumber","Radius","DrawMode"]:
            self.createGeometry(fp)
                        
    def createGeometry(self,fp):
        if (fp.FacesNumber >= 3) and (fp.Radius > 0):
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
            if "ChamferSize" in fp.PropertiesList:
                if fp.ChamferSize != 0:
                    w = DraftGeomUtils.filletWire(shape,fp.ChamferSize,chamfer=True)
                    if w:
                        shape = w  
            if "FilletRadius" in fp.PropertiesList:
                if fp.FilletRadius != 0:
                    w = DraftGeomUtils.filletWire(shape,fp.FilletRadius)
                    if w:
                        shape = w
            shape = Part.Face(shape)
            fp.Shape = shape
            fp.Placement = plm

class _DrawingView(_DraftObject):
    "The Draft DrawingView object"
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"DrawingView")
        obj.addProperty("App::PropertyVector","Direction","Shape View","Projection direction")
        obj.addProperty("App::PropertyFloat","LineWidth","View Style","The width of the lines inside this object")
        obj.addProperty("App::PropertyFloat","FontSize","View Style","The size of the texts inside this object")
        obj.addProperty("App::PropertyLink","Source","Base","The linked object")
        obj.addProperty("App::PropertyEnumeration","FillStyle","View Style","Shape Fill Style")
        obj.addProperty("App::PropertyEnumeration","LineStyle","View Style","Line Style")
        obj.FillStyle = ['shape color'] + svgpatterns().keys()
        obj.LineStyle = ['Solid','Dashed','Dotted','Dashdot']
        obj.LineWidth = 0.35
        obj.FontSize = 12

    def execute(self, obj):
        if obj.Source:
            obj.ViewResult = self.updateSVG(obj)

    def onChanged(self, obj, prop):
        if prop in ["X","Y","Scale","LineWidth","FontSize","FillStyle","Direction","LineStyle"]:
            obj.ViewResult = self.updateSVG(obj)

    def updateSVG(self, obj):
        "encapsulates a svg fragment into a transformation node"
        result = ""
        if hasattr(obj,"Source"):
            if obj.Source:
                if hasattr(obj,"LineStyle"):
                    ls = obj.LineStyle
                else:
                    ls = None
                if obj.Source.isDerivedFrom("App::DocumentObjectGroup"):
                    svg = ""
                    shapes = []
                    others = []
                    for o in obj.Source.Group:
                        if o.ViewObject.isVisible():
                            svg += getSVG(o,obj.Scale,obj.LineWidth,obj.FontSize,obj.FillStyle,obj.Direction,ls)
                else:
                    svg = getSVG(obj.Source,obj.Scale,obj.LineWidth,obj.FontSize,obj.FillStyle,obj.Direction,ls)
                result += '<g id="' + obj.Name + '"'
                result += ' transform="'
                result += 'rotate('+str(obj.Rotation)+','+str(obj.X)+','+str(obj.Y)+') '
                result += 'translate('+str(obj.X)+','+str(obj.Y)+') '
                result += 'scale('+str(obj.Scale)+','+str(-obj.Scale)+')'
                result += '">'
                result += svg
                result += '</g>'
        return result

class _BSpline(_DraftObject):
    "The BSpline object"
        
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"BSpline")
        obj.addProperty("App::PropertyVectorList","Points","Draft",
                        "The points of the b-spline")
        obj.addProperty("App::PropertyBool","Closed","Draft",
                        "If the b-spline is closed or not")
        obj.Closed = False

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

# for compatibility with older versions
_ViewProviderBSpline = _ViewProviderWire

class _Block(_DraftObject):
    "The Block object"
    
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"Block")
        obj.addProperty("App::PropertyLinkList","Components","Draft",
                        "The components of this block")

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

class _Shape2DView(_DraftObject):
    "The Shape2DView object"

    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Draft",
                        "The base object this 2D view must represent")
        obj.addProperty("App::PropertyVector","Projection","Draft",
                        "The projection vector of this object")
        obj.addProperty("App::PropertyEnumeration","ProjectionMode","Draft",
                        "The way the viewed object must be projected")
        obj.addProperty("App::PropertyIntegerList","FaceNumbers","Draft",
                        "The indices of the faces to be projected in Individual Faces mode")
        obj.addProperty("App::PropertyBool","HiddenLines","Draft",
                        "Show hidden lines")
        obj.Projection = Vector(0,0,1)
        obj.ProjectionMode = ["Solid","Individual Faces","Cutlines"]
        obj.HiddenLines = False
        _DraftObject.__init__(self,obj,"Shape2DView")

    def execute(self,obj):
        self.createGeometry(obj)

    def onChanged(self,obj,prop):
        if prop in ["Projection","Base","ProjectionMode","FaceNumbers"]:
            self.createGeometry(obj)

    def getProjected(self,obj,shape,direction):
        "returns projected edges from a shape and a direction"
        import Part,Drawing,DraftGeomUtils
        edges = []
        groups = Drawing.projectEx(shape,direction)
        for g in groups[0:5]:
            if g:
                edges.append(g)
        if hasattr(obj,"HiddenLines"):
            if obj.HiddenLines:
                for g in groups[5:]:
                    edges.append(g)
        #return Part.makeCompound(edges)
        return DraftGeomUtils.cleanProjection(Part.makeCompound(edges))

    def createGeometry(self,obj):
        import DraftGeomUtils
        pl = obj.Placement
        if obj.Base:
            if getType(obj.Base) == "SectionPlane":
                if obj.Base.Objects:
                    import Arch, Part, Drawing
                    objs = getGroupContents(obj.Base.Objects,walls=True)
                    objs = removeHidden(objs)
                    shapes = []
                    for o in objs:
                        if o.isDerivedFrom("Part::Feature"):
                            shapes.extend(o.Shape.Solids)
                    cutp,cutv,iv =Arch.getCutVolume(obj.Base.Shape,shapes)
                    cuts = []
                    if obj.ProjectionMode == "Solid":
                        for sh in shapes:
                            if sh.Volume < 0:
                                sh.reverse()
                            c = sh.cut(cutv)
                            cuts.extend(c.Solids)
                        comp = Part.makeCompound(cuts)
                        opl = FreeCAD.Placement(obj.Base.Placement)
                        proj = opl.Rotation.multVec(FreeCAD.Vector(0,0,1))
                        obj.Shape = self.getProjected(obj,comp,proj)
                    elif obj.ProjectionMode == "Cutlines":
                        for sh in shapes:
                            if sh.Volume < 0:
                                sh.reverse()
                            c = sh.section(cutp)
                            cuts.append(c)
                        comp = Part.makeCompound(cuts)
                        opl = FreeCAD.Placement(obj.Base.Placement)
                        comp.Placement = opl.inverse()
                        if comp:
                            obj.Shape = comp
                    
            elif obj.Base.isDerivedFrom("App::DocumentObjectGroup"):
                shapes = []
                objs = getGroupContents(obj.Base)
                for o in objs:
                    if o.isDerivedFrom("Part::Feature"):
                        if o.Shape:
                            if not o.Shape.isNull():
                                shapes.append(o.Shape)
                if shapes:
                    import Part
                    comp = Part.makeCompound(shapes)
                    obj.Shape = self.getProjected(obj,comp,obj.Projection)
                            
            elif obj.Base.isDerivedFrom("Part::Feature"):
                if not DraftVecUtils.isNull(obj.Projection):
                    if obj.ProjectionMode == "Solid":
                        obj.Shape = self.getProjected(obj,obj.Base.Shape,obj.Projection)
                    elif obj.ProjectionMode == "Individual Faces":
                        import Part
                        if obj.FaceNumbers:
                            faces = []
                            for i in obj.FaceNumbers:
                                if len(obj.Base.Shape.Faces) > i:
                                    faces.append(obj.Base.Shape.Faces[i])
                            views = []
                            for f in faces:
                                views.append(self.getProjected(obj,f,obj.Projection))
                            if views:
                                obj.Shape = Part.makeCompound(views)
        if not DraftGeomUtils.isNull(pl):
            obj.Placement = pl

class _Array(_DraftObject):
    "The Draft Array object"

    def __init__(self,obj):
        _DraftObject.__init__(self,obj,"Array")
        obj.addProperty("App::PropertyLink","Base","Draft",
                        "The base object that must be duplicated")
        obj.addProperty("App::PropertyEnumeration","ArrayType","Draft",
                        "The type of array to create")
        obj.addProperty("App::PropertyVector","Axis","Draft",
                        "The axis direction")
        obj.addProperty("App::PropertyInteger","NumberX","Draft",
                        "Number of copies in X direction")
        obj.addProperty("App::PropertyInteger","NumberY","Draft",
                        "Number of copies in Y direction")
        obj.addProperty("App::PropertyInteger","NumberZ","Draft",
                        "Number of copies in Z direction")
        obj.addProperty("App::PropertyInteger","NumberPolar","Draft",
                        "Number of copies")
        obj.addProperty("App::PropertyVector","IntervalX","Draft",
                        "Distance and orientation of intervals in X direction")
        obj.addProperty("App::PropertyVector","IntervalY","Draft",
                        "Distance and orientation of intervals in Y direction")
        obj.addProperty("App::PropertyVector","IntervalZ","Draft",
                        "Distance and orientation of intervals in Z direction")
        obj.addProperty("App::PropertyVector","Center","Draft",
                        "Center point")
        obj.addProperty("App::PropertyAngle","Angle","Draft",
                        "Angle to cover with copies")
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
            currentxvector=Vector(xvector).multiply(xcount)
            if not xcount==0:
                nshape = shape.copy()
                nshape.translate(currentxvector)
                base.append(nshape)
            for ycount in range(ynum):
                currentyvector=FreeCAD.Vector(currentxvector)
                currentyvector=currentyvector.add(Vector(yvector).multiply(ycount))
                if not ycount==0:
                    nshape = shape.copy()
                    nshape.translate(currentyvector)
                    base.append(nshape)
                for zcount in range(znum):
                    currentzvector=FreeCAD.Vector(currentyvector)
                    currentzvector=currentzvector.add(Vector(zvector).multiply(zcount))
                    if not zcount==0:
                        nshape = shape.copy()
                        nshape.translate(currentzvector)
                        base.append(nshape)
        return Part.makeCompound(base)

    def polarArray(self,shape,center,angle,num,axis):
        #print "angle ",angle," num ",num
        import Part
        if angle == 360:
            fraction = angle/num
        else:
            if num == 0:
                return shape
            fraction = angle/(num-1)
        base = [shape.copy()]
        for i in range(num-1):
            currangle = fraction + (i*fraction)
            nshape = shape.copy()
            nshape.rotate(DraftVecUtils.tup(center), DraftVecUtils.tup(axis), currangle)
            base.append(nshape)
        return Part.makeCompound(base)

class _PathArray(_DraftObject):
    "The Draft Path Array object"

    def __init__(self,obj):
        _DraftObject.__init__(self,obj,"PathArray")
        obj.addProperty("App::PropertyLink","Base","Draft",
                        "The base object that must be duplicated")
        obj.addProperty("App::PropertyLink","PathObj","Draft",
                        "The path object along which to distribute objects")
        obj.addProperty("App::PropertyLinkSubList","PathSubs","Draft",
                        "Selected subobjects (edges) of PathObj")                        
        obj.addProperty("App::PropertyInteger","Count","Draft",
                        "Number of copies")
        obj.addProperty("App::PropertyVector","Xlate","Draft",
                        "Optional translation vector")
        obj.addProperty("App::PropertyBool","Align","Draft","Orientation of Base along path")
        obj.Count = 2
        obj.PathSubs = []
        obj.Xlate = FreeCAD.Vector(0,0,0)
        obj.Align = False

    def execute(self,obj):
        self.createGeometry(obj)
 
    def onChanged(self,obj,prop):
        if prop in ["Count","Xlate","Align"]:
            self.createGeometry(obj)
            
    def createGeometry(self,obj):
        import FreeCAD
        import Part
        import DraftGeomUtils
        if obj.Base and obj.PathObj:
            pl = obj.Placement
            if obj.PathSubs:
                w = self.getWireFromSubs(obj)
            elif (hasattr(obj.PathObj.Shape,'Wires') and obj.PathObj.Shape.Wires):
                w = obj.PathObj.Shape.Wires[0]
            elif obj.PathObj.Shape.Edges:
                w = Part.Wire(obj.PathObj.Shape.Edges)
            else:
                FreeCAD.Console.PrintLog ("_PathArray.createGeometry: path " + obj.PathObj.Name + " has no edges\n")
                return
            obj.Shape = self.pathArray(obj.Base.Shape,w,obj.Count,obj.Xlate,obj.Align) 
            if not DraftGeomUtils.isNull(pl):
                obj.Placement = pl
                
    def getWireFromSubs(self,obj):
        '''Make a wire from PathObj subelements'''
        import Part
        sl = []
        for sub in obj.PathSubs:
            e = sub[0].Shape.getElement(sub[1])
            sl.append(e)
        return Part.Wire(sl)
                                   
    def getParameterFromV0(self, edge, offset):
        '''return parameter at distance offset from edge.Vertexes[0]'''
        '''sb method in Part.TopoShapeEdge???'''
        lpt = edge.valueAt(edge.getParameterByLength(0))
        vpt = edge.Vertexes[0].Point
        if not DraftVecUtils.equals(vpt,lpt):
            # this edge is flipped
            length = edge.Length - offset
        else:
            # this edge is right way around
            length = offset
        return(edge.getParameterByLength(length))
        
    def orientShape(self,shape,edge,offset,RefPt,xlate,align):
        '''Orient shape to edge tangent at offset.'''
        import Part
        import DraftGeomUtils
        import math
        z = FreeCAD.Vector(0,0,1)                                    # unit +Z  Probably defined elsewhere?
        y = FreeCAD.Vector(0,1,0)                                    # unit +Y
        x = FreeCAD.Vector(1,0,0)                                    # unit +X
        nullv = FreeCAD.Vector(0,0,0)
        ns = shape.copy()
        ns.translate(RefPt+xlate)
        if not align:
            return ns
            
        # get local coord system - tangent, normal, binormal, if possible
        t = edge.tangentAt(self.getParameterFromV0(edge,offset))
        t.normalize()
        try:
            n = edge.normalAt(self.getParameterFromV0(edge,offset))
            n.normalize()
            b = (t.cross(n)) 
            b.normalize()
        except:                                                      # no normal defined here
            n = nullv
            b = nullv 
            FreeCAD.Console.PrintLog ("Draft PathArray.orientShape - Shape not oriented (no normal).\n")
        lnodes = z.cross(b)
        if lnodes != nullv:
            lnodes.normalize()                                       # Can't normalize null vector.                                       
                                                                     # pathological cases:
        if n == nullv:                                               # 1) edge has inf. normals (ie line segment)
            psi = math.degrees(DraftVecUtils.angle(x,t,z))           #    align shape to tangent
            theta = 0.0
            phi = 0.0
        elif b == z:                                                 # 2) binormal is same as z
            psi = math.degrees(DraftVecUtils.angle(x,t,z))           #    align shape to tangent 
            theta = 0.0
            phi = 0.0
            FreeCAD.Console.PrintLog ("Draft PathArray.orientShape - Aligned to tangent only (no line of nodes).\n")
        else:                                                        # regular case                                
            psi = math.degrees(DraftVecUtils.angle(lnodes,t,z))
            theta = abs(math.degrees(DraftVecUtils.angle(z,b,x)))    # 0<=theta<=pi
            phi = math.degrees(DraftVecUtils.angle(x,lnodes,z))
        ns.rotate(RefPt,z,psi)
        ns.rotate(RefPt,x,theta)
        ns.rotate(RefPt,z,phi)
        return ns
                
    def pathArray(self,shape,pathwire,count,xlate,align):
        '''Distribute shapes along a path.'''
        import Part
        import DraftGeomUtils
        closedpath = DraftGeomUtils.isReallyClosed(pathwire)
        path = DraftGeomUtils.sortEdges(pathwire.Edges) 
        ends = []
        cdist = 0
        for e in path:                                                 # find cumulative edge end distance
            cdist += e.Length
            ends.append(cdist)
        base = []
        pt = path[0].Vertexes[0].Point                                 # place the start shape
        ns = self.orientShape(shape,path[0],0,pt,xlate,align)
        base.append(ns)
        if not(closedpath):                                            # closed path doesn't need shape on last vertex
            pt = path[-1].Vertexes[-1].Point                           # place the end shape
            ns = self.orientShape(shape,path[-1],path[-1].Length,pt,xlate,align)
            base.append(ns)
        if count < 3:
            return(Part.makeCompound(base))                            

        # place the middle shapes 
        if closedpath:                        
            stop = count                          
        else:
            stop = count - 1                        
        step = cdist/stop   
        remain = 0
        travel = step
        for i in range(1,stop):                            
            # which edge in path should contain this shape?
            iend = len(ends) - 1                                       # avoids problems with float math travel > ends[-1]
            for j in range(0,len(ends)):  
                if travel <= ends[j]:                      
                    iend = j
                    break
            # place shape at proper spot on proper edge
            remains = ends[iend] - travel
            offset = path[iend].Length - remains           
            pt = path[iend].valueAt(self.getParameterFromV0(path[iend],offset))
            ns = self.orientShape(shape,path[iend],offset,pt,xlate,align)
            base.append(ns)
            travel += step
        return(Part.makeCompound(base))      

class _Point(_DraftObject):
    "The Draft Point object"
    def __init__(self, obj,x,y,z):
        _DraftObject.__init__(self,obj,"Point")
        obj.addProperty("App::PropertyFloat","X","Draft","Location").X = x
        obj.addProperty("App::PropertyFloat","Y","Draft","Location").Y = y
        obj.addProperty("App::PropertyFloat","Z","Draft","Location").Z = z
        mode = 2
        obj.setEditorMode('Placement',mode)

    def execute(self, fp):
        import Part
        shape = Part.Vertex(Vector(fp.X,fp.Y,fp.Z))
        fp.Shape = shape

class _ViewProviderPoint(_ViewProviderDraft):
    "A viewprovider for the Draft Point object"
    def __init__(self, obj):
        _ViewProviderDraft.__init__(self,obj)

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

class _Clone(_DraftObject):
    "The Clone object"

    def __init__(self,obj):
        _DraftObject.__init__(self,obj,"Clone")
        obj.addProperty("App::PropertyLinkList","Objects","Draft",
                        "The objects included in this scale object")
        obj.addProperty("App::PropertyVector","Scale","Draft",
                        "The scale vector of this object")
        obj.Scale = Vector(1,1,1)

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

class _ViewProviderClone(_ViewProviderDraftAlt):
    "a view provider that displays a Part icon instead of a Draft icon"
    
    def __init__(self,vobj):
        _ViewProviderDraftAlt.__init__(self,vobj)

    def getIcon(self):
        return ":/icons/Draft_Clone.svg"
        
class _ShapeString(_DraftObject):
    "The ShapeString object"
        
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"ShapeString")
        obj.addProperty("App::PropertyString","String","Draft","Text string")
        obj.addProperty("App::PropertyFile","FontFile","Draft","Font file name")
        obj.addProperty("App::PropertyFloat","Size","Draft","Height of text")
        obj.addProperty("App::PropertyInteger","Tracking","Draft",
                        "Inter-character spacing")
                        
    def execute(self, fp):                                    
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        pass
                        
    def createGeometry(self,fp):
        import Part
#        import OpenSCAD2Dgeom
        import os
        if fp.String and fp.FontFile:
            if fp.Placement:
                plm = fp.Placement
            # TODO: os.path.splitunc() for Win/Samba net files?  
            head, tail = os.path.splitdrive(fp.FontFile)          # os.path.splitdrive() for Win
            head, tail = os.path.split(tail)
            head = head + '/'                                     # os.split drops last '/' from head
            CharList = Part.makeWireString(fp.String,
                                           head,
                                           tail,
                                           fp.Size,
                                           fp.Tracking)
            SSChars = []
            for char in CharList:
                CharFaces = []
                for CWire in char:
                    f = Part.Face(CWire)
                    if f:
                        CharFaces.append(f)
                # whitespace (ex: ' ') has no faces. This breaks OpenSCAD2Dgeom...
                if CharFaces:
#                    s = OpenSCAD2Dgeom.Overlappingfaces(CharFaces).makeshape()
                    #s = self.makeGlyph(CharFaces)
                    s = self.makeFaces(char)
                    SSChars.append(s)
            shape = Part.Compound(SSChars)
            fp.Shape = shape 
            if plm:                     
                fp.Placement = plm

    def makeFaces(self, wireChar):
        import Part
        compFaces=[]
        wirelist=sorted(wireChar,key=(lambda shape: shape.BoundBox.DiagonalLength),reverse=True)
        fixedwire = []
        for w in wirelist:
            compEdges = Part.Compound(w.Edges)
            compEdges = compEdges.connectEdgesToWires()
            fixedwire.append(compEdges.Wires[0])
        wirelist = fixedwire

        sep_wirelist = []
        while len(wirelist) > 0:
            wire2Face = [wirelist[0]]
            face = Part.Face(wirelist[0])
            for w in wirelist[1:]:
                p = w.Vertexes[0].Point
                u,v = face.Surface.parameter(p)
                if face.isPartOfDomain(u,v):
                    f = Part.Face(w)
                    if face.Orientation == f.Orientation:
                        if f.Surface.Axis * face.Surface.Axis < 0:
                            w.reverse()
                    else:
                        if f.Surface.Axis * face.Surface.Axis > 0:
                            w.reverse()
                    wire2Face.append(w)
                else:
                    sep_wirelist.append(w)
            wirelist = sep_wirelist
            sep_wirelist = []
            face = Part.Face(wire2Face)
            face.validate()
            if face.Surface.Axis.z < 0.0:
                face.reverse()
            compFaces.append(face)
        ret = Part.Compound(compFaces)
        return ret

    def makeGlyph(self, facelist):
        ''' turn list of simple contour faces into a compound shape representing a glyph '''
        ''' remove cuts, fuse overlapping contours, retain islands '''
        import Part
        if len(facelist) == 1:
            return(facelist[0])
    
        sortedfaces = sorted(facelist,key=(lambda shape: shape.Area),reverse=True)

        biggest = sortedfaces[0]
        result = biggest
        islands =[]
        for face in sortedfaces[1:]:
            bcfA = biggest.common(face).Area
            fA = face.Area
            difA = abs(bcfA - fA)
            eps = epsilon()
#            if biggest.common(face).Area == face.Area:
            if difA <= eps:                              # close enough to zero
                # biggest completely overlaps current face ==> cut
                result = result.cut(face)
#            elif biggest.common(face).Area == 0:
            elif bcfA <= eps:                        
                # island
                islands.append(face)
            else:
                # partial overlap - (font designer error?)
                result = result.fuse(face)  
        #glyphfaces = [result]
        wl = result.Wires
        for w in wl:
            w.fixWire()
        glyphfaces = [Part.Face(wl)]
        glyphfaces.extend(islands)     
        ret = Part.Compound(glyphfaces)           # should we fuse these instead of making compound?
        return ret


class _Facebinder(_DraftObject):
    "The Draft Facebinder object"
    def __init__(self,obj):
        _DraftObject.__init__(self,obj,"Facebinder")
        obj.addProperty("App::PropertyLinkSubList","Faces","Draft","Linked faces")

    def execute(self,obj):
        pl = obj.Placement
        if not obj.Faces:
            return
        faces = []
        for f in obj.Faces:
            if "Face" in f[1]:
                try:
                    fnum = int(f[1][4:])-1
                    faces.append(f[0].Shape.Faces[fnum])
                except:
                    print "Draft: wrong face index"
                    return
        if not faces:
            return
        import Part
        sh = faces.pop()
        try:
            for f in faces:
                sh = sh.fuse(f)
            sh = sh.removeSplitter()
        except:
            print "Draft: error building facebinder"
            return
        obj.Shape = sh
        obj.Placement = pl
        
    def addSubobjects(self,obj,facelinks):
        "adds facelinks to this facebinder"
        objs = obj.Faces
        for o in facelinks:
            if isinstance(o,tuple) or isinstance(o,list):
                if o[0].Name != obj.Name:
                    objs.append(tuple(o))
            else:
                for el in o.SubElementNames:
                    if "Face" in el:
                        if o.Object.Name != obj.Name:
                            objs.append((o.Object,el))
        obj.Faces = objs
        self.execute(obj)


#----End of Python Features Definitions----#

if gui:    
    if not hasattr(FreeCADGui,"Snapper"):
        import DraftSnap
