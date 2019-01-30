# -*- coding: utf-8 -*-

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

#from __future__ import division

__title__="FreeCAD Draft Workbench"
__author__ = "Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, Dmitry Chigrin, Daniel Falck"
__url__ = "http://www.freecadweb.org"

## \addtogroup DRAFT
#  \brief Create and manipulate basic 2D objects
#
#  This module offers a range of tools to create and manipulate basic 2D objects
#
#  The module allows to create 2D geometric objects such as line, rectangle, circle,
#  etc, modify these objects by moving, scaling or rotating them, and offers a couple of
#  other utilities to manipulate further these objects, such as decompose them (downgrade)
#  into smaller elements.
#
#  The functionality of the module is divided into GUI tools, usable from the
#  FreeCAD interface, and corresponding python functions, that can perform the same
#  operation programmatically.
#
#  @{

'''The Draft module offers a range of tools to create and manipulate basic 2D objects'''

import FreeCAD, math, sys, os, DraftVecUtils, WorkingPlane
from FreeCAD import Vector

if FreeCAD.GuiUp:
    import FreeCADGui, Draft_rc
    from PySide import QtCore
    from PySide.QtCore import QT_TRANSLATE_NOOP
    gui = True
    #from DraftGui import translate
else:
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    #print("FreeCAD Gui not present. Draft module will have some features disabled.")
    gui = False

def translate(ctx,txt):
    return txt

arrowtypes = ["Dot","Circle","Arrow","Tick","Tick-2"]

#---------------------------------------------------------------------------
# General functions
#---------------------------------------------------------------------------

def stringencodecoin(ustr):
    """stringencodecoin(str): Encodes a unicode object to be used as a string in coin"""
    try:
        from pivy import coin
        coin4 = coin.COIN_MAJOR_VERSION >= 4
    except (ImportError, AttributeError):
        coin4 = False
    if coin4:
        return ustr.encode('utf-8')
    else:
        return ustr.encode('latin1')

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
                 "dimstyle","gridSize"]:
        return "int"
    elif param in ["constructiongroupname","textfont","patternFile","template",
                   "snapModes","FontFile","ClonePrefix","labeltype"]:
        return "string"
    elif param in ["textheight","tolerance","gridSpacing","arrowsize","extlines","dimspacing",
                   "dimovershoot","extovershoot"]:
        return "float"
    elif param in ["selectBaseObjects","alwaysSnap","grid","fillmode","saveonexit","maxSnap",
                   "SvgLinesBlack","dxfStdSize","showSnapBar","hideSnapBar","alwaysShowGrid",
                   "renderPolylineWidth","showPlaneTracker","UsePartPrimitives","DiscretizeEllipses",
                   "showUnit"]:
        return "bool"
    elif param in ["color","constructioncolor","snapcolor"]:
        return "unsigned"
    else:
        return None

def getParam(param,default=None):
    "getParam(parameterName): returns a Draft parameter value from the current config"
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    t = getParamType(param)
    #print("getting param ",param, " of type ",t, " default: ",str(default))
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
    if (obj.TypeId == "Part::Offset2D"):
        return "Offset2D"
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
    if FreeCAD.GuiUp:
        import FreeCADGui
        v = FreeCADGui.ActiveDocument.ActiveView
        if str(type(v)) == "<type 'View3DInventorPy'>":
            return v
        v = FreeCADGui.ActiveDocument.mdiViewsOfType("Gui::View3DInventor")
        if v:
            return v[0]
    return None

def isClone(obj,objtype,recursive=False):
    """isClone(obj,objtype,[recursive]): returns True if the given object is
    a clone of an object of the given type. If recursive is True, also check if
    the clone is a clone of clone (of clone...)  of the given type."""
    if isinstance(objtype,list):
        return any([isClone(obj,t,recursive) for t in objtype])
    if getType(obj) == "Clone":
        if len(obj.Objects) == 1:
            if getType(obj.Objects[0]) == objtype:
                return True
            elif recursive and (getType(obj.Objects[0]) == "Clone"):
                return isClone(obj.Objects[0],objtype,recursive)
    elif hasattr(obj,"CloneOf"):
        if obj.CloneOf:
            return True
    return False

def getGroupNames():
    "returns a list of existing groups in the document"
    glist = []
    doc = FreeCAD.ActiveDocument
    for obj in doc.Objects:
        if obj.isDerivedFrom("App::DocumentObjectGroup") or (getType(obj) in ["Floor","Building","Site"]):
            glist.append(obj.Name)
    return glist

def ungroup(obj):
    "removes the current object from any group it belongs to"
    for g in getGroupNames():
        grp = FreeCAD.ActiveDocument.getObject(g)
        if obj in grp.Group:
            g = grp.Group
            g.remove(obj)
            grp.Group = g

def autogroup(obj):
    "adds a given object to the autogroup, if applicable"
    if FreeCAD.GuiUp:
        if hasattr(FreeCADGui,"draftToolBar"):
            if hasattr(FreeCADGui.draftToolBar,"autogroup") and (not FreeCADGui.draftToolBar.isConstructionMode()):
                if FreeCADGui.draftToolBar.autogroup != None:
                    g = FreeCAD.ActiveDocument.getObject(FreeCADGui.draftToolBar.autogroup)
                    if g:
                        found = False
                        for o in g.Group:
                            if o.Name == obj.Name:
                                found = True
                        if not found:
                            gr = g.Group
                            gr.append(obj)
                            g.Group = gr
                else:
                    # Arch active container
                    a = FreeCADGui.ActiveDocument.ActiveView.getActiveObject("Arch")
                    if a:
                        a.addObject(obj)

def dimSymbol(symbol=None,invert=False):
    "returns the current dim symbol from the preferences as a pivy SoMarkerSet"
    if symbol == None:
        symbol = getParam("dimsymbol",0)
    from pivy import coin
    if symbol == 0:
        return coin.SoSphere()
    elif symbol == 1:
        marker = coin.SoMarkerSet()
        marker.markerIndex = FreeCADGui.getMarkerIndex("circle", 9)
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
        marker = coin.SoSeparator()
        c = coin.SoCoordinate3()
        c.point.setValues([(-1,-2,0),(0,2,0),(1,2,0),(0,-2,0)])
        f = coin.SoFaceSet()
        marker.addChild(c)
        marker.addChild(f)
        return marker
    elif symbol == 4:
        return dimDash((-1.5,-1.5,0),(1.5,1.5,0))
    else:
        print("Draft.dimsymbol: Not implemented")
        return coin.SoSphere()

def dimDash(p1, p2):
    '''dimDash(p1, p2): returns pivy SoSeparator.
    Used for making Tick-2, DimOvershoot, ExtOvershoot dashes.
    '''
    from pivy import coin
    dash = coin.SoSeparator()
    v = coin.SoVertexProperty()
    v.vertex.set1Value(0, p1)
    v.vertex.set1Value(1, p2)
    l = coin.SoLineSet()
    l.vertexProperty = v
    dash.addChild(l)
    return dash

def shapify(obj):
    '''shapify(object): transforms a parametric shape object into
    non-parametric and returns the new object'''
    if not (obj.isDerivedFrom("Part::Feature")): return None
    if not "Shape" in obj.PropertiesList: return None
    shape = obj.Shape
    if len(shape.Faces) == 1:
        name = "Face"
    elif len(shape.Solids) == 1:
        name = "Solid"
    elif len(shape.Solids) > 1:
        name = "Compound"
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

def getGroupContents(objectslist,walls=False,addgroups=False,spaces=False,noarchchild=False):
    '''getGroupContents(objectlist,[walls,addgroups]): if any object of the given list
    is a group, its content is appened to the list, which is returned. If walls is True,
    walls and structures are also scanned for included windows or rebars. If addgroups
    is true, the group itself is also included in the list.'''
    def getWindows(obj):
        l = []
        if getType(obj) in ["Wall","Structure"]:
            for o in obj.OutList:
                l.extend(getWindows(o))
            for i in obj.InList:
                if (getType(i) in ["Window"]) or isClone(obj,"Window"):
                    if hasattr(i,"Hosts"):
                        if obj in i.Hosts:
                            l.append(i)
                elif (getType(i) in ["Rebar"]) or isClone(obj,"Rebar"):
                    if hasattr(i,"Host"):
                        if obj == i.Host:
                            l.append(i)
        elif (getType(obj) in ["Window","Rebar"]) or isClone(obj,["Window","Rebar"]):
            l.append(obj)
        return l

    newlist = []
    if not isinstance(objectslist,list):
        objectslist = [objectslist]
    for obj in objectslist:
        if obj:
            if obj.isDerivedFrom("App::DocumentObjectGroup") or ((getType(obj) in ["Building","BuildingPart","Space","Site"]) and hasattr(obj,"Group")):
                if getType(obj) == "Site":
                    if obj.Shape:
                        newlist.append(obj)
                if obj.isDerivedFrom("Drawing::FeaturePage"):
                    # skip if the group is a page
                    newlist.append(obj)
                else:
                    if addgroups or (spaces and (getType(obj) == "Space")):
                        newlist.append(obj)
                    if noarchchild and (getType(obj) in ["Building","BuildingPart"]):
                        pass
                    else:
                        newlist.extend(getGroupContents(obj.Group,walls,addgroups))
            else:
                #print("adding ",obj.Name)
                newlist.append(obj)
                if walls:
                    newlist.extend(getWindows(obj))

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
    print("solids: ", len(shape.Solids))
    print("faces: ", len(shape.Faces))
    print("wires: ", len(shape.Wires))
    print("edges: ", len(shape.Edges))
    print("verts: ", len(shape.Vertexes))
    if shape.Faces:
        for f in range(len(shape.Faces)):
            print("face ",f,":")
            for v in shape.Faces[f].Vertexes:
                print("    ",v.Point)
    elif shape.Wires:
        for w in range(len(shape.Wires)):
            print("wire ",w,":")
            for v in shape.Wires[w].Vertexes:
                print("    ",v.Point)
    else:
        for v in shape.Vertexes:
            print("    ",v.Point)

def compareObjects(obj1,obj2):
    "Prints the differences between 2 objects"

    if obj1.TypeId != obj2.TypeId:
        print(obj1.Name + " and " + obj2.Name + " are of different types")
    elif getType(obj1) != getType(obj2):
        print(obj1.Name + " and " + obj2.Name + " are of different types")
    else:
        for p in obj1.PropertiesList:
            if p in obj2.PropertiesList:
                if p in ["Shape","Label"]:
                    pass
                elif p ==  "Placement":
                    delta = str((obj1.Placement.Base.sub(obj2.Placement.Base)).Length)
                    print("Objects have different placements. Distance between the 2: " + delta + " units")
                else:
                    if getattr(obj1,p) != getattr(obj2,p):
                        print("Property " + p + " has a different value")
            else:
                print("Property " + p + " doesn't exist in one of the objects")

def formatObject(target,origin=None):
    '''
    formatObject(targetObject,[originObject]): This function applies
    to the given target object the current properties
    set on the toolbar (line color and line width),
    or copies the properties of another object if given as origin.
    It also places the object in construction group if needed.
    '''
    if not target:
        return
    obrep = target.ViewObject
    if not obrep:
        return
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
            if hasattr(obrep,"Transparency"):
                obrep.Transparency = 80
        else:
            col = ui.getDefaultColor("ui")
            fcol = ui.getDefaultColor("face")
        col = (float(col[0]),float(col[1]),float(col[2]),0.0)
        fcol = (float(fcol[0]),float(fcol[1]),float(fcol[2]),0.0)
        lw = ui.linewidth
        fs = ui.fontsize
        if not origin or not hasattr(origin,'ViewObject'):
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
                        if not obrep.getEditorMode(p):
                            if hasattr(getattr(matchrep,p),"Value"):
                                val = getattr(matchrep,p).Value
                            else:
                                val = getattr(matchrep,p)
                            setattr(obrep,p,val)
            if matchrep.DisplayMode in obrep.listDisplayModes():
                obrep.DisplayMode = matchrep.DisplayMode
            if hasattr(matchrep,"DiffuseColor") and hasattr(obrep,"DiffuseColor"):
                if matchrep.DiffuseColor:
                    FreeCAD.ActiveDocument.recompute()
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
                if obj:
                    FreeCADGui.Selection.addSelection(obj)

def loadSvgPatterns():
    "loads the default Draft SVG patterns and custom patters if available"
    import importSVG
    from PySide import QtCore
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
                        p[k] = [p[k],altpat+os.sep+f]
                    FreeCAD.svgpatterns.update(p)

def svgpatterns():
    """svgpatterns(): returns a dictionary with installed SVG patterns"""
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
        from PySide import QtGui,QtSvg
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
            buffersize = p.byteCount()
            numcomponents = int (float(buffersize) / ( size[0] * size[1] ))

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
            print("Draft: unable to load texture")
            return None
        else:
            return img
    return None

def getMovableChildren(objectslist,recursive=True):
    '''getMovableChildren(objectslist,[recursive]): extends the given list of objects
    with all child objects that have a "MoveWithHost" property set to True. If
    recursive is True, all descendents are considered, otherwise only direct children.'''
    added = []
    if not isinstance(objectslist,list):
        objectslist = [objectslist]
    for obj in objectslist:
        if not (getType(obj) in ["Clone","SectionPlane","Facebinder","BuildingPart"]):
            # objects that should never move their children
            children = obj.OutList
            if  hasattr(obj,"Proxy"):
                if obj.Proxy:
                    if hasattr(obj.Proxy,"getSiblings") and not(getType(obj) in ["Window"]):
                        #children.extend(obj.Proxy.getSiblings(obj))
                        pass
            for child in children:
                if hasattr(child,"MoveWithHost"):
                    if child.MoveWithHost:
                        if hasattr(obj,"CloneOf"):
                            if obj.CloneOf:
                                if obj.CloneOf.Name != child.Name:
                                    added.append(child)
                            else:
                                added.append(child)
                        else:
                            added.append(child)
            if recursive:
                added.extend(getMovableChildren(children))
    return added

def makeCircle(radius, placement=None, face=None, startangle=None, endangle=None, support=None):
    '''makeCircle(radius,[placement,face,startangle,endangle])
    or makeCircle(edge,[face]):
    Creates a circle object with given radius. If placement is given, it is
    used. If face is False, the circle is shown as a
    wireframe, otherwise as a face. If startangle AND endangle are given
    (in degrees), they are used and the object appears as an arc. If an edge
    is passed, its Curve must be a Part.Circle'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    import Part, DraftGeomUtils
    if placement: typecheck([(placement,FreeCAD.Placement)], "makeCircle")
    if startangle != endangle:
        n = "Arc"
    else:
        n = "Circle"
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",n)
    _Circle(obj)
    if face != None:
        obj.MakeFace = face
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
        formatObject(obj)
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj

def makeRectangle(length, height, placement=None, face=None, support=None):
    '''makeRectangle(length,width,[placement],[face]): Creates a Rectangle
    object with length in X direction and height in Y direction.
    If a placement is given, it is used. If face is False, the
    rectangle is shown as a wireframe, otherwise as a face.'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    if placement: typecheck([(placement,FreeCAD.Placement)], "makeRectangle")
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Rectangle")
    _Rectangle(obj)

    obj.Length = length
    obj.Height = height
    obj.Support = support
    if face != None:
        obj.MakeFace = face
    if placement: obj.Placement = placement
    if gui:
        _ViewProviderRectangle(obj.ViewObject)
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
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
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
            #l.append((p1,"Center"))
            obj.ViewObject.Override = "R $dim"
            obj.Diameter = False
        elif p3 == "diameter":
            #l.append((p1,"Diameter"))
            obj.ViewObject.Override = "Ø $dim"
            obj.Diameter = True
        obj.LinkedGeometry = l
        obj.Support = p1
        p3 = p4
        if not p3:
            p3 = p1.Shape.Edges[p2].Curve.Center.add(Vector(1,0,0))
    obj.Dimline = p3
    if hasattr(FreeCAD,"DraftWorkingPlane"):
        normal = FreeCAD.DraftWorkingPlane.axis
    else:
        normal = FreeCAD.Vector(0,0,1)
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
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
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

def makeWire(pointslist,closed=False,placement=None,face=None,support=None):
    '''makeWire(pointslist,[closed],[placement]): Creates a Wire object
    from the given list of vectors. If closed is True or first
    and last points are identical, the wire is closed. If face is
    true (and wire is closed), the wire will appear filled. Instead of
    a pointslist, you can also pass a Part Wire.'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    import DraftGeomUtils, Part
    if not isinstance(pointslist,list):
        e = pointslist.Wires[0].Edges
        pointslist = Part.Wire(Part.__sortEdges__(e))
        nlist = []
        for v in pointslist.Vertexes:
            nlist.append(v.Point)
        if DraftGeomUtils.isReallyClosed(pointslist):
            closed = True
        pointslist = nlist
    if len(pointslist) == 0:
        print("Invalid input points: ",pointslist)
    #print(pointslist)
    #print(closed)
    if placement:
        typecheck([(placement,FreeCAD.Placement)], "makeWire")
        ipl = placement.inverse()
        pointslist = [ipl.multVec(p) for p in pointslist]
    if len(pointslist) == 2: fname = "Line"
    else: fname = "Wire"
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",fname)
    _Wire(obj)
    obj.Points = pointslist
    obj.Closed = closed
    obj.Support = support
    if face != None:
        obj.MakeFace = face
    if placement: obj.Placement = placement
    if gui:
        _ViewProviderWire(obj.ViewObject)
        formatObject(obj)
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj

def makePolygon(nfaces,radius=1,inscribed=True,placement=None,face=None,support=None):
    '''makePolgon(nfaces,[radius],[inscribed],[placement],[face]): Creates a
    polygon object with the given number of faces and the radius.
    if inscribed is False, the polygon is circumscribed around a circle
    with the given radius, otherwise it is inscribed. If face is True,
    the resulting shape is displayed as a face, otherwise as a wireframe.
    '''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    if nfaces < 3: return None
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Polygon")
    _Polygon(obj)
    obj.FacesNumber = nfaces
    obj.Radius = radius
    if face != None:
        obj.MakeFace = face
    if inscribed:
        obj.DrawMode = "inscribed"
    else:
        obj.DrawMode = "circumscribed"
    obj.Support = support
    if placement: obj.Placement = placement
    if gui:
        _ViewProviderDraft(obj.ViewObject)
        formatObject(obj)
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj

def makeLine(p1,p2=None):
    '''makeLine(p1,p2): Creates a line between p1 and p2.
    makeLine(LineSegment): Creates a line from a Part.LineSegment
    makeLine(Shape): Creates a line from first vertex to last vertex of the given shape'''
    if not p2:
        if hasattr(p1,"StartPoint") and hasattr(p1,"EndPoint"):
            p2 = p1.EndPoint
            p1 = p1.StartPoint
        elif hasattr(p1,"Vertexes"):
            p2 = p1.Vertexes[-1].Point
            p1 = p1.Vertexes[0].Point
        else:
            FreeCAD.Console.PrintError("Unable to create a line from the given data\n")
            return
    obj = makeWire([p1,p2])
    return obj

def makeBSpline(pointslist,closed=False,placement=None,face=None,support=None):
    '''makeBSpline(pointslist,[closed],[placement]): Creates a B-Spline object
    from the given list of vectors. If closed is True or first
    and last points are identical, the wire is closed. If face is
    true (and wire is closed), the wire will appear filled. Instead of
    a pointslist, you can also pass a Part Wire.'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    from DraftTools import msg
    if not isinstance(pointslist,list):
        nlist = []
        for v in pointslist.Vertexes:
            nlist.append(v.Point)
        pointslist = nlist
    if len(pointslist) < 2:
        msg(translate("draft","Draft.makeBSpline: not enough points")+"\n", 'error')
        return
    if (pointslist[0] == pointslist[-1]):
        if len(pointslist) > 2:
            closed = True
            pointslist.pop()
            msg(translate("draft","Draft.makeBSpline: Equal endpoints forced Closed")+"\n", 'warning')
        else:                                                                            # len == 2 and first == last   GIGO
            msg(translate("draft","Draft.makeBSpline: Invalid pointslist")+"\n", 'error')
            return
    # should have sensible parms from here on
    if placement: typecheck([(placement,FreeCAD.Placement)], "makeBSpline")
    if len(pointslist) == 2: fname = "Line"
    else: fname = "BSpline"
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",fname)
    _BSpline(obj)
    obj.Closed = closed
    obj.Points = pointslist
    obj.Support = support
    if face != None:
        obj.MakeFace = face
    if placement: obj.Placement = placement
    if gui:
        _ViewProviderWire(obj.ViewObject)
        formatObject(obj)
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj

def makeBezCurve(pointslist,closed=False,placement=None,face=None,support=None,Degree=None):
    '''makeBezCurve(pointslist,[closed],[placement]): Creates a Bezier Curve object
    from the given list of vectors.   Instead of a pointslist, you can also pass a Part Wire.'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    if not isinstance(pointslist,list):
        nlist = []
        for v in pointslist.Vertexes:
            nlist.append(v.Point)
        pointslist = nlist
    if placement: typecheck([(placement,FreeCAD.Placement)], "makeBezCurve")
    if len(pointslist) == 2: fname = "Line"
    else: fname = "BezCurve"
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",fname)
    _BezCurve(obj)
    obj.Points = pointslist
    if Degree:
        obj.Degree = Degree
    else:
        import Part
        obj.Degree = min((len(pointslist)-(1 * (not closed))),\
            Part.BezierCurve().MaxDegree)
    obj.Closed = closed
    obj.Support = support
    if face != None:
        obj.MakeFace = face
    obj.Proxy.resetcontinuity(obj)
    if placement: obj.Placement = placement
    if gui:
        _ViewProviderWire(obj.ViewObject)
#        if not face: obj.ViewObject.DisplayMode = "Wireframe"
#        obj.ViewObject.DisplayMode = "Wireframe"
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
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    typecheck([(point,Vector)], "makeText")
    if not isinstance(stringslist,list): stringslist = [stringslist]
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Text")
    DraftText(obj)
    obj.Text = stringslist
    obj.Placement.Base = point
    if FreeCAD.GuiUp:
        ViewProviderDraftText(obj.ViewObject)
        if not screen:
            obj.ViewObject.DisplayMode = "3D text"
        h = getParam("textheight",0.20)
        if screen:
            h = h*10
        obj.ViewObject.FontSize = h
        obj.ViewObject.FontName = getParam("textfont","")
        obj.ViewObject.LineSpacing = 1
        formatObject(obj)
        select(obj)
    return obj

def makeCopy(obj,force=None,reparent=False):
    '''makeCopy(object): returns an exact copy of an object'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    if (getType(obj) == "Rectangle") or (force == "Rectangle"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        _Rectangle(newobj)
        if gui:
            _ViewProviderRectangle(newobj.ViewObject)
    elif (getType(obj) == "Point") or (force == "Point"):
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        _Point(newobj)
        if gui:
            _ViewProviderPoint(newobj.ViewObject)
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
            ArchWindow._ViewProviderWindow(newobj.ViewObject)
    elif (getType(obj) == "Panel") or (force == "Panel"):
        import ArchPanel
        newobj = FreeCAD.ActiveDocument.addObject(obj.TypeId,getRealName(obj.Name))
        ArchPanel._Panel(newobj)
        if gui:
            ArchPanel._ViewProviderPanel(newobj.ViewObject)
    elif (getType(obj) == "Sketch") or (force == "Sketch"):
        newobj = FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject",getRealName(obj.Name))
        for geo in obj.Geometry:
            newobj.addGeometry(geo)
        for con in obj.Constraints:
            newobj.addConstraint(con)
    elif obj.isDerivedFrom("Part::Feature"):
        newobj = FreeCAD.ActiveDocument.addObject("Part::Feature",getRealName(obj.Name))
        newobj.Shape = obj.Shape
    else:
        print("Error: Object type cannot be copied")
        return None
    for p in obj.PropertiesList:
        if not p in ["Proxy"]:
            if p in newobj.PropertiesList:
                if not newobj.getEditorMode(p):
                    try:
                        setattr(newobj,p,obj.getPropertyByName(p))
                    except AttributeError:
                        try:
                            setattr(newobj,p,obj.getPropertyByName(p).Value)
                        except AttributeError:
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
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Block")
    _Block(obj)
    obj.Components = objectslist
    if gui:
        _ViewProviderDraftPart(obj.ViewObject)
        for o in objectslist:
            o.ViewObject.Visibility = False
        select(obj)
    return obj

def makeArray(baseobject,arg1,arg2,arg3,arg4=None,arg5=None,arg6=None,name="Array"):
    '''makeArray(object,xvector,yvector,xnum,ynum,[name]) for rectangular array, or
    makeArray(object,xvector,yvector,zvector,xnum,ynum,znum,[name]) for rectangular array, or
    makeArray(object,center,totalangle,totalnum,[name]) for polar array: Creates an array
    of the given object
    with, in case of rectangular array, xnum of iterations in the x direction
    at xvector distance between iterations, same for y direction with yvector and ynum,
    same for z direction with zvector and znum. In case of polar array, center is a vector,
    totalangle is the angle to cover (in degrees) and totalnum is the number of objects,
    including the original. The result is a parametric Draft Array.'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Array(obj)
    obj.Base = baseobject
    if arg6:
        obj.ArrayType = "ortho"
        obj.IntervalX = arg1
        obj.IntervalY = arg2
        obj.IntervalZ = arg3
        obj.NumberX = arg4
        obj.NumberY = arg5
        obj.NumberZ = arg6
    elif arg4:
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
        _ViewProviderDraftArray(obj.ViewObject)
        baseobject.ViewObject.hide()
        formatObject(obj,obj.Base)
        if len(obj.Base.ViewObject.DiffuseColor) > 1:
            FreeCAD.ActiveDocument.recompute()
            obj.ViewObject.Proxy.resetColors(obj.ViewObject)
        select(obj)
    return obj

def makePathArray(baseobject,pathobject,count,xlate=None,align=False,pathobjsubs=[]):
    '''makePathArray(docobj,path,count,xlate,align,pathobjsubs): distribute
    count copies of a document baseobject along a pathobject or subobjects of a
    pathobject. Optionally translates each copy by FreeCAD.Vector xlate direction
    and distance to adjust for difference in shape centre vs shape reference point.
    Optionally aligns baseobject to tangent/normal/binormal of path.'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
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
        _ViewProviderDraftArray(obj.ViewObject)
        baseobject.ViewObject.hide()
        formatObject(obj,obj.Base)
        if len(obj.Base.ViewObject.DiffuseColor) > 1:
            FreeCAD.ActiveDocument.recompute()
            obj.ViewObject.Proxy.resetColors(obj.ViewObject)
        select(obj)
    return obj

def makePointArray(base, ptlst):
    '''makePointArray(base,pointlist):'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","PointArray")
    _PointArray(obj, base, ptlst)
    obj.Base = base
    obj.PointList = ptlst
    if gui:
        _ViewProviderDraftArray(obj.ViewObject)
        base.ViewObject.hide()
        formatObject(obj,obj.Base)
        if len(obj.Base.ViewObject.DiffuseColor) > 1:
            FreeCAD.ActiveDocument.recompute()
            obj.ViewObject.Proxy.resetColors(obj.ViewObject)
        select(obj)
    return obj

def makeEllipse(majradius,minradius,placement=None,face=True,support=None):
    '''makeEllipse(majradius,minradius,[placement],[face],[support]): makes
    an ellipse with the given major and minor radius, and optionally
    a placement.'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
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
        #if not face:
        #    obj.ViewObject.DisplayMode = "Wireframe"
        formatObject(obj)
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj

def makeVisGroup(group=None,name="VisGroup"):
    '''makeVisGroup([group]): creates a VisGroup object in the given group, or in the
    active document if no group is given'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroupPython",name)
    _VisGroup(obj)
    if FreeCAD.GuiUp:
        _ViewProviderVisGroup(obj.ViewObject)
        formatObject(obj)
    if group:
        group.addObject(obj)
    return obj

def extrude(obj,vector,solid=False):
    '''makeExtrusion(object,vector): extrudes the given object
    in the direction given by the vector. The original object
    gets hidden.'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    newobj = FreeCAD.ActiveDocument.addObject("Part::Extrusion","Extrusion")
    newobj.Base = obj
    newobj.Dir = vector
    newobj.Solid = solid
    if gui:
        obj.ViewObject.Visibility = False
        formatObject(newobj,obj)
        select(newobj)
    FreeCAD.ActiveDocument.recompute()
    return newobj

def joinWires(wires, joinAttempts = 0):
    '''joinWires(objects): merges a set of wires where possible, if any of those
    wires have a coincident start and end point'''
    if joinAttempts > len(wires):
        return FreeCAD.ActiveDocument.recompute()
    joinAttempts += 1
    for wire1Index, wire1 in enumerate(wires):
        for wire2Index, wire2 in enumerate(wires):
            if wire2Index <= wire1Index:
                continue
            if joinTwoWires(wire1, wire2):
                wires.pop(wire2Index)
                break
    joinWires(wires, joinAttempts)

def joinTwoWires(wire1, wire2):
    '''joinTwoWires(object, object): joins two wires if they share a common
    point as a start or an end'''
    wire1AbsPoints = [wire1.Placement.multVec(point) for point in wire1.Points]
    wire2AbsPoints = [wire2.Placement.multVec(point) for point in wire2.Points]
    if wire1AbsPoints[0] == wire2AbsPoints[0]:
        wire1AbsPoints = list(reversed(wire1AbsPoints))
    elif wire1AbsPoints[0] == wire2AbsPoints[-1]:
        wire1AbsPoints = list(reversed(wire1AbsPoints))
        wire2AbsPoints = list(reversed(wire2AbsPoints))
    elif wire1AbsPoints[-1] == wire2AbsPoints[-1]:
        wire2AbsPoints = list(reversed(wire2AbsPoints))
    elif wire1AbsPoints[-1] == wire2AbsPoints[0]:
        pass
    else:
        return False
    wire2AbsPoints.pop(0)
    wire1.Points = [wire1.Placement.inverse().multVec(point) for point in wire1AbsPoints] + [wire1.Placement.inverse().multVec(point) for point in wire2AbsPoints]
    FreeCAD.ActiveDocument.removeObject(wire2.Name)
    return True

def split(wire, newPoint, edgeIndex):
    wire1Points = []
    wire2Points = []
    for index, point in enumerate(wire.Points):
        if index == edgeIndex:
            wire1Points.append(wire.Placement.inverse().multVec(newPoint))
            wire2Points.append(newPoint)
            wire2Points.append(wire.Placement.multVec(point))
        elif index < edgeIndex:
            wire1Points.append(point)
        elif index > edgeIndex:
            wire2Points.append(wire.Placement.multVec(point))
    wire.Points = wire1Points
    makeWire(wire2Points, placement=wire.Placement)
    FreeCAD.ActiveDocument.recompute()

def fuse(object1,object2):
    '''fuse(oject1,object2): returns an object made from
    the union of the 2 given objects. If the objects are
    coplanar, a special Draft Wire is used, otherwise we use
    a standard Part fuse.'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
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
        select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj

def cut(object1,object2):
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    '''cut(oject1,object2): returns a cut object made from
    the difference of the 2 given objects.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::Cut","Cut")
    obj.Base = object1
    obj.Tool = object2
    object1.ViewObject.Visibility = False
    object2.ViewObject.Visibility = False
    formatObject(obj,object1)
    select(obj)
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
    objectslist.extend(getMovableChildren(objectslist))
    newobjlist = []
    newgroups = {}
    for obj in objectslist:
        if hasattr(obj,"Placement"):
           if obj.getEditorMode("Placement") == ["ReadOnly"]:
               if not copy:
                   FreeCAD.Console.PrintError(obj.Name+" cannot be moved because its placement is readonly.")
                   continue
        if getType(obj) == "Point":
            v = Vector(obj.X,obj.Y,obj.Z)
            v = v.add(vector)
            if copy:
                newobj = makeCopy(obj)
            else:
                newobj = obj
            newobj.X = v.x
            newobj.Y = v.y
            newobj.Z = v.z
        elif (obj.isDerivedFrom("Part::Feature")):
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
                if gui:
                    formatObject(newobj,obj)
            else:
                newobj = obj
            newobj.Position = obj.Position.add(vector)
        elif getType(obj) == "DraftText":
            if copy:
                newobj = FreeCAD.ActiveDocument.addObject("App::FeaturePython",getRealName(obj.Name))
                DraftText(newobj)
                if gui:
                    ViewProviderDraftText(newobj.ViewObject)
                    formatObject(newobj,obj)
                newobj.Text = obj.Text
                newobj.Placement = obj.Placement
                if gui:
                    formatObject(newobj,obj)
            else:
                newobj = obj
            newobj.Placement.Base = obj.Placement.Base.add(vector)
        elif getType(obj) == "Dimension":
            if copy:
                newobj = FreeCAD.ActiveDocument.addObject("App::FeaturePython",getRealName(obj.Name))
                _Dimension(newobj)
                if gui:
                    _ViewProviderDimension(newobj.ViewObject)
                    formatObject(newobj,obj)
            else:
                newobj = obj
            newobj.Start = obj.Start.add(vector)
            newobj.End = obj.End.add(vector)
            newobj.Dimline = obj.Dimline.add(vector)
        else:
            if copy and obj.isDerivedFrom("Mesh::Feature"):
                print("Mesh copy not supported at the moment") # TODO
            newobj = obj
            if "Placement" in obj.PropertiesList:
                pla = obj.Placement
                pla.move(vector)
        newobjlist.append(newobj)
        if copy:
            for p in obj.InList:
                if p.isDerivedFrom("App::DocumentObjectGroup") and (p in objectslist):
                    g = newgroups.setdefault(p.Name,FreeCAD.ActiveDocument.addObject(p.TypeId,p.Name))
                    g.addObject(newobj)
                    break
    if copy and getParam("selectBaseObjects",False):
        select(objectslist)
    else:
        select(newobjlist)
    if len(newobjlist) == 1: return newobjlist[0]
    return newobjlist

def array(objectslist,arg1,arg2,arg3,arg4=None,arg5=None,arg6=None):
    '''array(objectslist,xvector,yvector,xnum,ynum) for rectangular array,
    array(objectslist,xvector,yvector,zvector,xnum,ynum,znum) for rectangular array,
    or array(objectslist,center,totalangle,totalnum) for polar array: Creates an array
    of the objects contained in list (that can be an object or a list of objects)
    with, in case of rectangular array, xnum of iterations in the x direction
    at xvector distance between iterations, and same for y and z directions with yvector
    and ynum and zvector and znum. In case of polar array, center is a vector, totalangle
    is the angle to cover (in degrees) and totalnum is the number of objects, including
    the original.

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
    def rectArray2(objectslist,xvector,yvector,zvector,xnum,ynum,znum):
        typecheck([(xvector,Vector), (yvector,Vector), (zvector,Vector),(xnum,int), (ynum,int),(znum,int)], "rectArray2")
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
                for zcount in range(znum):
                    currentzvector=currentyvector.add(Vector(zvector).multiply(zcount))
                    if not zcount==0:
                        move(objectslist,currentzvector,True)
    def polarArray(objectslist,center,angle,num):
        typecheck([(center,Vector), (num,int)], "polarArray")
        if not isinstance(objectslist,list): objectslist = [objectslist]
        fraction = float(angle)/num
        for i in range(num):
            currangle = fraction + (i*fraction)
            rotate(objectslist,currangle,center,copy=True)
    if arg6:
        rectArray2(objectslist,arg1,arg2,arg3,arg4,arg5,arg6)
    elif arg4:
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
    objectslist.extend(getMovableChildren(objectslist))
    newobjlist = []
    newgroups = {}
    for obj in objectslist:
        if hasattr(obj,"Placement"):
           if obj.getEditorMode("Placement") == ["ReadOnly"]:
               if not copy:
                   FreeCAD.Console.PrintError(obj.Name+" cannot be rotated because its placement is readonly.")
                   continue
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
        elif getType(obj) == "Point":
            v = Vector(obj.X,obj.Y,obj.Z)
            rv = v.sub(center)
            rv = DraftVecUtils.rotate(rv,math.radians(angle),axis)
            v = center.add(rv)
            newobj.X = v.x
            newobj.Y = v.y
            newobj.Z = v.z
        elif hasattr(obj,"Placement"):
            shape = Part.Shape()
            shape.Placement = obj.Placement
            shape.rotate(DraftVecUtils.tup(center), DraftVecUtils.tup(axis), angle)
            newobj.Placement = shape.Placement
        if copy:
            formatObject(newobj,obj)
        newobjlist.append(newobj)
        if copy:
            for p in obj.InList:
                if p.isDerivedFrom("App::DocumentObjectGroup") and (p in objectslist):
                    g = newgroups.setdefault(p.Name,FreeCAD.ActiveDocument.addObject(p.TypeId,p.Name))
                    g.addObject(newobj)
                    break
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
            if obj.isDerivedFrom("Part::Feature"):
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
                #print(p)
                newobj.Points = p
            elif getType(obj) == "BSpline":
                p = []
                for p1 in obj.Points:
                    p2 = p1.sub(center)
                    p2.scale(delta.x,delta.y,delta.z)
                    p.append(p2)
                newobj.Points = p
            elif (obj.isDerivedFrom("Part::Feature")):
                newobj.Shape = sh
            elif (obj.TypeId == "App::Annotation"):
                factor = delta.y * obj.ViewObject.FontSize
                newobj.ViewObject.FontSize = factor
                d = obj.Position.sub(center)
                newobj.Position = center.add(Vector(d.x*delta.x,d.y*delta.y,d.z*delta.z))
            if copy:
                formatObject(newobj,obj)
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
    '''offset(object,delta,[copymode],[bind]): offsets the given wire by
    applying the given delta Vector to its first vertex. If copymode is
    True, another object is created, otherwise the same object gets
    offsetted. If bind is True, and provided the wire is open, the original
    and the offsetted wires will be bound by their endpoints, forming a face
    if sym is True, bind must be true too, and the offset is made on both
    sides, the total width being the given delta length. If offsetting a
    BSpline, the delta must not be a Vector but a list of Vectors, one for
    each node of the spline.'''
    import Part, DraftGeomUtils
    newwire = None
    delete = None

    if getType(obj) in ["Sketch","Part"]:
        copy = True
        print("the offset tool is currently unable to offset a non-Draft object directly - Creating a copy")

    def getRect(p,obj):
        "returns length,height,placement"
        pl = obj.Placement.copy()
        pl.Base = p[0]
        diag = p[2].sub(p[0])
        bb = p[1].sub(p[0])
        bh = p[3].sub(p[0])
        nb = DraftVecUtils.project(diag,bb)
        nh = DraftVecUtils.project(diag,bh)
        if obj.Length.Value < 0: l = -nb.Length
        else: l = nb.Length
        if obj.Height.Value < 0: h = -nh.Length
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

    newwire = None
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
            if isinstance(delta,float) and (len(obj.Shape.Edges) == 1):
                # circle
                c = obj.Shape.Edges[0].Curve
                nc = Part.Circle(c.Center,c.Axis,delta)
                if len(obj.Shape.Vertexes) > 1:
                    nc = Part.ArcOfCircle(nc,obj.Shape.Edges[0].FirstParameter,obj.Shape.Edges[0].LastParameter)
                newwire = Part.Wire(nc.toShape())
                p = []
            else:
                newwire = DraftGeomUtils.offsetWire(obj.Shape,delta)
                if DraftGeomUtils.hasCurves(newwire) and copy:
                    p = []
                else:
                    p = DraftGeomUtils.getVerts(newwire)
    if occ:
        newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
        newobj.Shape = DraftGeomUtils.offsetWire(obj.Shape,delta,occ=True)
        formatObject(newobj,obj)
        if not copy:
            delete = obj.Name
    elif bind:
        if not DraftGeomUtils.isReallyClosed(obj.Shape):
            if sym:
                s1 = n1
                s2 = n2
            else:
                s1 = obj.Shape
                s2 = newwire
            if s1 and s2:
                w1 = s1.Edges
                w2 = s2.Edges
                w3 = Part.LineSegment(s1.Vertexes[0].Point,s2.Vertexes[0].Point).toShape()
                w4 = Part.LineSegment(s1.Vertexes[-1].Point,s2.Vertexes[-1].Point).toShape()
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
                newobj.Shape = Part.Face(Part.Wire(w1+[w3]+w2+[w4]))
            else:
                print("Draft.offset: Unable to bind wires")
        else:
            newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
            newobj.Shape = Part.Face(obj.Shape.Wires[0])
        if not copy:
            delete = obj.Name
    elif copy:
        newobj = None
        if sym: return None
        if getType(obj) == "Wire":
            if p:
                newobj = makeWire(p)
                newobj.Closed = obj.Closed
            elif newwire:
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
                newobj.Shape = newwire
            else:
                print("Draft.offset: Unable to duplicate this object")
        elif getType(obj) == "Rectangle":
            if p:
                length,height,plac = getRect(p,obj)
                newobj = makeRectangle(length,height,plac)
            elif newwire:
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
                newobj.Shape = newwire
            else:
                print("Draft.offset: Unable to duplicate this object")
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
        elif getType(obj) == "BSpline":
            newobj = makeBSpline(delta)
            newobj.Closed = obj.Closed
        else:
            # try to offset anyway
            try:
                if p:
                    newobj = makeWire(p)
                    newobj.Closed = obj.Shape.isClosed()
            except Part.OCCError:
                pass
            if not(newobj) and newwire:
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Offset")
                newobj.Shape = newwire
            else:
                print("Draft.offset: Unable to create an offset")
        if newobj:
            formatObject(newobj,obj)
    else:
        newobj = None
        if sym: return None
        if getType(obj) == "Wire":
            if obj.Base or obj.Tool:
                FreeCAD.Console.PrintWarning("Warning: object history removed\n")
                obj.Base = None
                obj.Tool = None
            obj.Points = p
        elif getType(obj) == "BSpline":
            #print(delta)
            obj.Points = delta
            #print("done")
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
            print("unsupported object") # TODO
        newobj = obj
    if copy and getParam("selectBaseObjects",False):
        select(newobj)
    else:
        select(obj)
    if delete:
        FreeCAD.ActiveDocument.removeObject(delete)
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
            for cluster in Part.getSortedClusters(obj.Shape.Edges):
                w = Part.Wire(cluster)
                if DraftGeomUtils.hasCurves(w):
                    if (len(w.Edges) == 1) and (DraftGeomUtils.geomType(w.Edges[0]) == "Circle"):
                        nobj = makeCircle(w.Edges[0])
                    else:
                        nobj = FreeCAD.ActiveDocument.addObject("Part::Feature",obj.Name)
                        nobj.Shape = w
                else:
                    nobj = makeWire(w)
                newobjlist.append(nobj)
                formatObject(nobj,obj)
                # sketches are always in wireframe mode. In Draft we don't like that!
                nobj.ViewObject.DisplayMode = "Flat Lines"
            if delete:
                FreeCAD.ActiveDocument.removeObject(obj.Name)
    FreeCAD.ActiveDocument.recompute()
    if makeblock:
        return makeBlock(newobjlist)
    else:
        if len(newobjlist) == 1:
            return newobjlist[0]
        return newobjlist

def getDXF(obj,direction=None):
    '''getDXF(object,[direction]): returns a DXF entity from the given
    object. If direction is given, the object is projected in 2D.'''
    plane = None
    result = ""
    if obj.isDerivedFrom("Drawing::View") or obj.isDerivedFrom("TechDraw::DrawView"):
        if obj.Source.isDerivedFrom("App::DocumentObjectGroup"):
            for o in obj.Source.Group:
                result += getDXF(o,obj.Direction)
        else:
            result += getDXF(obj.Source,obj.Direction)
        return result
    if direction:
        if isinstance(direction,FreeCAD.Vector):
            if direction != Vector(0,0,0):
                plane = WorkingPlane.plane()
                plane.alignToPointAndAxis(Vector(0,0,0),direction)

    def getProj(vec):
        if not plane: return vec
        nx = DraftVecUtils.project(vec,plane.u)
        ny = DraftVecUtils.project(vec,plane.v)
        return Vector(nx.Length,ny.Length,0)

    if getType(obj) == "Dimension":
        p1 = getProj(obj.Start)
        p2 = getProj(obj.End)
        p3 = getProj(obj.Dimline)
        result += "0\nDIMENSION\n8\n0\n62\n0\n3\nStandard\n70\n1\n"
        result += "10\n"+str(p3.x)+"\n20\n"+str(p3.y)+"\n30\n"+str(p3.z)+"\n"
        result += "13\n"+str(p1.x)+"\n23\n"+str(p1.y)+"\n33\n"+str(p1.z)+"\n"
        result += "14\n"+str(p2.x)+"\n24\n"+str(p2.y)+"\n34\n"+str(p2.z)+"\n"

    elif getType(obj) == "Annotation":
        p = getProj(obj.Position)
        count = 0
        for t in obj.LabeLtext:
            result += "0\nTEXT\n8\n0\n62\n0\n"
            result += "10\n"+str(p.x)+"\n20\n"+str(p.y+count)+"\n30\n"+str(p.z)+"\n"
            result += "40\n1\n"
            result += "1\n"+str(t)+"\n"
            result += "7\nSTANDARD\n"
            count += 1

    elif obj.isDerivedFrom("Part::Feature"):
        # TODO do this the Draft way, for ex. using polylines and rectangles
        import Drawing
        if not direction:
            direction = FreeCAD.Vector(0,0,-1)
        if DraftVecUtils.isNull(direction):
            direction = FreeCAD.Vector(0,0,-1)
        try:
            d = Drawing.projectToDXF(obj.Shape,direction)
        except:
            print("Draft.getDXF: Unable to project ",obj.Label," to ",direction)
        else:
            result += d

    else:
        print("Draft.getDXF: Unsupported object: ",obj.Label)

    return result


def getrgb(color,testbw=True):
    """getRGB(color,[testbw]): returns a rgb value #000000 from a freecad color
    if testwb = True (default), pure white will be converted into pure black"""
    r = str(hex(int(color[0]*255)))[2:].zfill(2)
    g = str(hex(int(color[1]*255)))[2:].zfill(2)
    b = str(hex(int(color[2]*255)))[2:].zfill(2)
    col = "#"+r+g+b
    if testbw:
        if col == "#ffffff":
            #print(getParam('SvgLinesBlack'))
            if getParam('SvgLinesBlack',True):
                col = "#000000"
    return col


import getSVG as svg


getSVG = svg.getSVG


def makeDrawingView(obj,page,lwmod=None,tmod=None,otherProjection=None):
    '''
    makeDrawingView(object,page,[lwmod,tmod]) - adds a View of the given object to the
    given page. lwmod modifies lineweights (in percent), tmod modifies text heights
    (in percent). The Hint scale, X and Y of the page are used.
    '''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    if getType(obj) == "SectionPlane":
        import ArchSectionPlane
        viewobj = FreeCAD.ActiveDocument.addObject("Drawing::FeatureViewPython","View")
        page.addObject(viewobj)
        ArchSectionPlane._ArchDrawingView(viewobj)
        viewobj.Source = obj
        viewobj.Label = "View of "+obj.Name
    elif getType(obj) == "Panel":
        import ArchPanel
        viewobj = ArchPanel.makePanelView(obj,page)
    else:
        viewobj = FreeCAD.ActiveDocument.addObject("Drawing::FeatureViewPython","View"+obj.Name)
        _DrawingView(viewobj)
        page.addObject(viewobj)
        if (otherProjection):
            if hasattr(otherProjection,"Scale"):
                viewobj.Scale = otherProjection.Scale
            if hasattr(otherProjection,"X"):
                viewobj.X = otherProjection.X
            if hasattr(otherProjection,"Y"):
                viewobj.Y = otherProjection.Y
            if hasattr(otherProjection,"Rotation"):
                viewobj.Rotation = otherProjection.Rotation
            if hasattr(otherProjection,"Direction"):
                viewobj.Direction = otherProjection.Direction
        else:
            if hasattr(page.ViewObject,"HintScale"):
                viewobj.Scale = page.ViewObject.HintScale
            if hasattr(page.ViewObject,"HintOffsetX"):
                viewobj.X = page.ViewObject.HintOffsetX
            if hasattr(page.ViewObject,"HintOffsetY"):
                viewobj.Y = page.ViewObject.HintOffsetY
        viewobj.Source = obj
        if lwmod: viewobj.LineweightModifier = lwmod
        if tmod: viewobj.TextModifier = tmod
        if hasattr(obj.ViewObject,"Pattern"):
            if str(obj.ViewObject.Pattern) in list(svgpatterns().keys()):
                viewobj.FillStyle = str(obj.ViewObject.Pattern)
        if hasattr(obj.ViewObject,"DrawStyle"):
            viewobj.LineStyle = obj.ViewObject.DrawStyle
        if hasattr(obj.ViewObject,"LineColor"):
            viewobj.LineColor = obj.ViewObject.LineColor
        elif hasattr(obj.ViewObject,"TextColor"):
            viewobj.LineColor = obj.ViewObject.TextColor
    return viewobj

def makeShape2DView(baseobj,projectionVector=None,facenumbers=[]):
    '''
    makeShape2DView(object,[projectionVector,facenumbers]) - adds a 2D shape to the document, which is a
    2D projection of the given object. A specific projection vector can also be given. You can also
    specify a list of face numbers to be considered in individual faces mode.
    '''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
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
    FreeCAD.ActiveDocument.recompute()
    return obj

def makeSketch(objectslist,autoconstraints=False,addTo=None,
        delete=False,name="Sketch",radiusPrecision=-1):
    '''makeSketch(objectslist,[autoconstraints],[addTo],[delete],[name],[radiusPrecision]):

    Makes a Sketch objectslist with the given Draft objects.

    * objectlist: can be single or list of objects of Draft type objects,
        Part::Feature, Part.Shape, or mix of them.

    * autoconstraints(False): if True, constraints will be automatically added to
        wire nodes, rectangles and circles.

    * addTo(None) : if set to an existing sketch, geometry will be added to it
        instead of creating a new one.

    * delete(False): if True, the original object will be deleted.
        If set to a string 'all' the object and all its linked object will be
        deleted

    * name('Sketch'): the name for the new sketch object

    * radiusPrecision(-1): If <0, disable radius constraint. If =0, add indiviaul
        radius constraint. If >0, the radius will be rounded according to this
        precision, and 'Equal' constraint will be added to curve with equal
        radius within precision.'''

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return

    import Part, DraftGeomUtils
    from Sketcher import Constraint
    import Sketcher
    import math

    StartPoint = 1
    EndPoint = 2
    MiddlePoint = 3
    deletable = None

    if not isinstance(objectslist,(list,tuple)):
        objectslist = [objectslist]
    for obj in objectslist:
        if isinstance(obj,Part.Shape):
            shape = obj
        elif not obj.isDerivedFrom("Part::Feature"):
            FreeCAD.Console.PrintError(translate("draft","not shape found"))
            return None
        else:
            shape = obj.Shape
        if not DraftGeomUtils.isPlanar(shape):
            FreeCAD.Console.PrintError(translate("draft","All Shapes must be co-planar"))
            return None
    if addTo:
        nobj = addTo
    else:
        nobj = FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject", name)
        deletable = nobj
        nobj.ViewObject.Autoconstraints = False

    # Collect constraints and add in one go to improve performance
    constraints = []
    radiuses = {}

    def addRadiusConstraint(edge):
        try:
            if radiusPrecision<0:
                return
            if radiusPrecision==0:
                constraints.append(Constraint('Radius',
                        nobj.GeometryCount-1, edge.Curve.Radius))
                return
            r = round(edge.Curve.Radius,radiusPrecision)
            constraints.append(Constraint('Equal',
                    radiuses[r],nobj.GeometryCount-1))
        except KeyError:
            radiuses[r] = nobj.GeometryCount-1
            constraints.append(Constraint('Radius',nobj.GeometryCount-1, r))
        except AttributeError:
            pass

    def convertBezier(edge):
        if DraftGeomUtils.geomType(edge) == "BezierCurve":
            return(edge.Curve.toBSpline(edge.FirstParameter,edge.LastParameter).toShape())
        else:
            return(edge)

    rotation = None
    for obj in objectslist:
        ok = False
        tp = getType(obj)
        if tp in ["Circle","Ellipse"]:
            if rotation is None:
                rotation = obj.Placement.Rotation
            edge = obj.Shape.Edges[0]
            if len(edge.Vertexes) == 1:
                newEdge = DraftGeomUtils.orientEdge(edge)
                nobj.addGeometry(newEdge)
            else:
                # make new ArcOfCircle
                circle = DraftGeomUtils.orientEdge(edge)
                angle  = edge.Placement.Rotation.Angle
                axis   = edge.Placement.Rotation.Axis
                circle.Center = DraftVecUtils.rotate(edge.Curve.Center, -angle, axis)
                first  = math.radians(obj.FirstAngle)
                last   = math.radians(obj.LastAngle)
                arc    = Part.ArcOfCircle(circle, first, last)
                nobj.addGeometry(arc)
            addRadiusConstraint(edge)
            ok = True
        elif tp == "Rectangle":
            if rotation is None:
                rotation = obj.Placement.Rotation
            if obj.FilletRadius.Value == 0:
                for edge in obj.Shape.Edges:
                    nobj.addGeometry(DraftGeomUtils.orientEdge(edge))
                if autoconstraints:
                    last = nobj.GeometryCount - 1
                    segs = [last-3,last-2,last-1,last]
                    if obj.Placement.Rotation.Q == (0,0,0,1):
                        constraints.append(Constraint("Coincident",last-3,EndPoint,last-2,StartPoint))
                        constraints.append(Constraint("Coincident",last-2,EndPoint,last-1,StartPoint))
                        constraints.append(Constraint("Coincident",last-1,EndPoint,last,StartPoint))
                        constraints.append(Constraint("Coincident",last,EndPoint,last-3,StartPoint))
                    constraints.append(Constraint("Horizontal",last-3))
                    constraints.append(Constraint("Vertical",last-2))
                    constraints.append(Constraint("Horizontal",last-1))
                    constraints.append(Constraint("Vertical",last))
                ok = True
        elif tp in ["Wire","Polygon"]:
            if obj.FilletRadius.Value == 0:
                closed = False
                if tp == "Polygon":
                    closed = True
                elif hasattr(obj,"Closed"):
                    closed = obj.Closed

                if len(obj.Shape.Vertexes) < 3:
                    e = obj.Shape.Edges[0]
                    nobj.addGeometry(Part.LineSegment(e.Curve,e.FirstParameter,e.LastParameter))
                else:
                    # Use the first three points to make a working plane. We've already
                    # checked to make sure everything is coplanar
                    plane = Part.Plane(*[i.Point for i in obj.Shape.Vertexes[:3]])
                    normal = plane.Axis
                    if rotation is None:
                        axis = FreeCAD.Vector(0,0,1).cross(normal)
                        angle = DraftVecUtils.angle(normal, FreeCAD.Vector(0,0,1)) * FreeCAD.Units.Radian
                        rotation = FreeCAD.Rotation(axis, angle)
                    for edge in obj.Shape.Edges:
                        # edge.rotate(FreeCAD.Vector(0,0,0), rotAxis, rotAngle)
                        edge = DraftGeomUtils.orientEdge(edge, normal)
                        nobj.addGeometry(edge)
                    if autoconstraints:
                        last = nobj.GeometryCount
                        segs = list(range(last-len(obj.Shape.Edges),last-1))
                        for seg in segs:
                            constraints.append(Constraint("Coincident",seg,EndPoint,seg+1,StartPoint))
                            if DraftGeomUtils.isAligned(nobj.Geometry[seg],"x"):
                                constraints.append(Constraint("Vertical",seg))
                            elif DraftGeomUtils.isAligned(nobj.Geometry[seg],"y"):
                                constraints.append(Constraint("Horizontal",seg))
                        if closed:
                            constraints.append(Constraint("Coincident",last-1,EndPoint,segs[0],StartPoint))
                ok = True
        elif tp == "BSpline":
            nobj.addGeometry(obj.Shape.Edges[0].Curve)
            nobj.exposeInternalGeometry(nobj.GeometryCount-1)
            ok = True
        elif tp == "BezCurve":
            bez = obj.Shape.Edges[0].Curve
            bsp = bez.toBSpline(bez.FirstParameter,bez.LastParameter)
            nobj.addGeometry(bsp)
            nobj.exposeInternalGeometry(nobj.GeometryCount-1)
            ok = True
        elif tp == 'Shape' or obj.isDerivedFrom("Part::Feature"):
            shape = obj if tp == 'Shape' else obj.Shape

            if not DraftGeomUtils.isPlanar(shape):
                FreeCAD.Console.PrintError(translate("draft","The given object is not planar and cannot be converted into a sketch."))
                return None
            if rotation is None:
                #rotation = obj.Placement.Rotation
                norm = DraftGeomUtils.getNormal(shape)
                if norm:
                    rotation = FreeCAD.Rotation(FreeCAD.Vector(0,0,1),norm)
                else:
                    FreeCAD.Console.PrintWarning(translate("draft","Unable to guess the normal direction of this object"))
                    rotation = FreeCAD.Rotation()
                    norm = obj.Placement.Rotation.Axis
            if not shape.Wires:
                for e in shape.Edges:
                    # unconnected edges
                    newedge = convertBezier(e)
                    nobj.addGeometry(DraftGeomUtils.orientEdge(newedge,norm,make_arc=True))
                    addRadiusConstraint(newedge)

            # if not addTo:
                # nobj.Placement.Rotation = DraftGeomUtils.calculatePlacement(shape).Rotation

            if autoconstraints:
                for wire in shape.Wires:
                    last_count = nobj.GeometryCount
                    edges = wire.OrderedEdges
                    for edge in edges:
                        newedge = convertBezier(edge)
                        nobj.addGeometry(DraftGeomUtils.orientEdge(
                                            newedge,norm,make_arc=True))
                        addRadiusConstraint(newedge)
                    for i,g in enumerate(nobj.Geometry[last_count:]):
                        if edges[i].Closed:
                            continue
                        seg = last_count+i

                        if DraftGeomUtils.isAligned(g,"x"):
                            constraints.append(Constraint("Vertical",seg))
                        elif DraftGeomUtils.isAligned(g,"y"):
                            constraints.append(Constraint("Horizontal",seg))

                        if seg == nobj.GeometryCount-1:
                            if not wire.isClosed():
                                break
                            g2 = nobj.Geometry[last_count]
                            seg2 = last_count
                        else:
                            seg2 = seg+1
                            g2 = nobj.Geometry[seg2]

                        end1 = g.value(g.LastParameter)
                        start2 = g2.value(g2.FirstParameter)
                        if DraftVecUtils.equals(end1,start2) :
                            constraints.append(Constraint(
                                "Coincident",seg,EndPoint,seg2,StartPoint))
                            continue
                        end2 = g2.value(g2.LastParameter)
                        start1 = g.value(g.FirstParameter)
                        if DraftVecUtils.equals(end2,start1):
                            constraints.append(Constraint(
                                "Coincident",seg,StartPoint,seg2,EndPoint))
                        elif DraftVecUtils.equals(start1,start2):
                            constraints.append(Constraint(
                                "Coincident",seg,StartPoint,seg2,StartPoint))
                        elif DraftVecUtils.equals(end1,end2):
                            constraints.append(Constraint(
                                "Coincident",seg,EndPoint,seg2,EndPoint))
            else:
                for wire in shape.Wires:
                    for edge in wire.OrderedEdges:
                        newedge = convertBezier(edge)
                        nobj.addGeometry(DraftGeomUtils.orientEdge(
                                                newedge,norm,make_arc=True))
            ok = True
        formatObject(nobj,obj)
        if ok and delete and obj.isDerivedFrom("Part::Feature"):
            doc = obj.Document
            def delObj(obj):
                if obj.InList:
                    FreeCAD.Console.PrintWarning(translate("draft",
                        "Cannot delete object {} with dependency".format(obj.Label))+"\n")
                else:
                    doc.removeObject(obj.Name)
            try:
                if delete == 'all':
                    objs = [obj]
                    while objs:
                        obj = objs[0]
                        objs = objs[1:] + obj.OutList
                        delObj(obj)
                else:
                    delObj(obj)
            except Exception as ex:
                FreeCAD.Console.PrintWarning(translate("draft",
                    "Failed to delete object {}: {}".format(obj.Label,ex))+"\n")
    if rotation:
        nobj.Placement.Rotation = rotation
    else:
        print("-----error!!! rotation is still None...")
    nobj.addConstraint(constraints)
    FreeCAD.ActiveDocument.recompute()
    return nobj

def makePoint(X=0, Y=0, Z=0,color=None,name = "Point", point_size= 5):
    ''' makePoint(x,y,z ,[color(r,g,b),point_size]) or
        makePoint(Vector,color(r,g,b),point_size]) -
        creates a Point in the current document.
        example usage:
        p1 = makePoint()
        p1.ViewObject.Visibility= False # make it invisible
        p1.ViewObject.Visibility= True  # make it visible
        p1 = makePoint(-1,0,0) #make a point at -1,0,0
        p1 = makePoint(1,0,0,(1,0,0)) # color = red
        p1.X = 1 #move it in x
        p1.ViewObject.PointColor =(0.0,0.0,1.0) #change the color-make sure values are floats
    '''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj=FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    if isinstance(X,FreeCAD.Vector):
        Z = X.z
        Y = X.y
        X = X.x
    _Point(obj,X,Y,Z)
    obj.X = X
    obj.Y = Y
    obj.Z = Z
    if gui:
        _ViewProviderPoint(obj.ViewObject)
        if hasattr(FreeCADGui,"draftToolBar") and (not color):
            color = FreeCADGui.draftToolBar.getDefaultColor('ui')
        obj.ViewObject.PointColor = (float(color[0]), float(color[1]), float(color[2]))
        obj.ViewObject.PointSize = point_size
        obj.ViewObject.Visibility = True
    select(obj)
    FreeCAD.ActiveDocument.recompute()
    return obj

def makeShapeString(String,FontFile,Size = 100,Tracking = 0):
    '''ShapeString(Text,FontFile,Height,Track): Turns a text string
    into a Compound Shape'''
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
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

def clone(obj,delta=None,forcedraft=False):

    '''clone(obj,[delta,forcedraft]): makes a clone of the given object(s). The clone is an exact,
    linked copy of the given object. If the original object changes, the final object
    changes too. Optionally, you can give a delta Vector to move the clone from the
    original position. If forcedraft is True, the resulting object is a Draft clone
    even if the input object is an Arch object.'''

    prefix = getParam("ClonePrefix","")
    if prefix:
        prefix = prefix.strip()+" "
    if not isinstance(obj,list):
        obj = [obj]
    if (len(obj) == 1) and obj[0].isDerivedFrom("Part::Part2DObject"):
        cl = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Clone2D")
        cl.Label = prefix + obj[0].Label + " (2D)"
    elif (len(obj) == 1) and (hasattr(obj[0],"CloneOf") or (getType(obj[0]) == "BuildingPart")) and (not forcedraft):
        # arch objects can be clones
        import Arch
        if getType(obj[0]) == "BuildingPart":
            cl = Arch.makeComponent()
        else:
            cl = getattr(Arch,"make"+obj[0].Proxy.Type)()
        base = getCloneBase(obj[0])
        cl.Label = prefix + base.Label
        cl.CloneOf = base
        if hasattr(cl,"Material") and hasattr(obj[0],"Material"):
            cl.Material = obj[0].Material
        if getType(obj[0]) != "BuildingPart":
            cl.Placement = obj[0].Placement
        try:
            cl.Role = base.Role
            cl.Description = base.Description
            cl.Tag = base.Tag
        except:
            pass
        if gui:
            cl.ViewObject.DiffuseColor = base.ViewObject.DiffuseColor
            if getType(obj[0]) in ["Window","BuildingPart"]:
                from DraftGui import todo
                todo.delay(Arch.recolorize,cl)
        select(cl)
        return cl
    else:
        cl = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Clone")
        cl.addExtension("Part::AttachExtensionPython", None)
        cl.Label = prefix + obj[0].Label
    _Clone(cl)
    if gui:
        _ViewProviderClone(cl.ViewObject)
    cl.Objects = obj
    if delta:
        cl.Placement.move(delta)
    elif len(obj) == 1:
        cl.Placement = obj[0].Placement
    formatObject(cl,obj[0])
    if gui and (len(obj) > 1):
        cl.ViewObject.Proxy.resetColors(cl.ViewObject)
    select(cl)
    return cl

def getCloneBase(obj,strict=False):
    '''getCloneBase(obj,[strict]): returns the object cloned by this object, if
    any, or this object if it is no clone. If strict is True, if this object is
    not a clone, this function returns False'''
    if hasattr(obj,"CloneOf"):
        if obj.CloneOf:
            return getCloneBase(obj.CloneOf)
    if getType(obj) == "Clone":
        return obj.Objects[0]
    if strict:
        return False
    return obj


def mirror(objlist,p1,p2):
    '''mirror(objlist,p1,p2,[clone]): creates a mirrored version of the given object(s)
    along an axis that passes through the two vectors p1 and p2.'''

    if not objlist:
        FreeCAD.Console.PrintError(translate("draft","No object given")+"\n")
        return
    if p1 == p2:
        FreeCAD.Console.PrintError(translate("draft","The two points are coincident")+"\n")
        return
    if not isinstance(objlist,list):
        objlist = [objlist]

    result = []

    for obj in objlist:
        mir = FreeCAD.ActiveDocument.addObject("Part::Mirroring","mirror")
        mir.Label = "Mirror of "+obj.Label
        mir.Source = obj
        if gui:
            norm = FreeCADGui.ActiveDocument.ActiveView.getViewDirection().negative()
        else:
            norm = FreeCAD.Vector(0,0,1)
        pnorm = p2.sub(p1).cross(norm).normalize()
        mir.Base = p1
        mir.Normal = pnorm
        formatObject(mir,obj)
        result.append(mir)

    if len(result) == 1:
        result = result[0]
        select(result)
    return result


def heal(objlist=None,delete=True,reparent=True):
    '''heal([objlist],[delete],[reparent]) - recreates Draft objects that are damaged,
    for example if created from an earlier version. If delete is True,
    the damaged objects are deleted (default). If ran without arguments, all the objects
    in the document will be healed if they are damaged. If reparent is True (default),
    new objects go at the very same place in the tree than their original.'''

    auto = False

    if not objlist:
        objlist = FreeCAD.ActiveDocument.Objects
        print("Automatic mode: Healing whole document...")
        auto = True
    else:
        print("Manual mode: Force-healing selected objects...")

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
                    print("Healing " + obj.Name + " of type Dimension")
                    nobj = makeCopy(obj,force="Dimension",reparent=reparent)
                elif ("Height" in props) and ("Length" in props):
                    print("Healing " + obj.Name + " of type Rectangle")
                    nobj = makeCopy(obj,force="Rectangle",reparent=reparent)
                elif ("Points" in props) and ("Closed" in props):
                    if "BSpline" in obj.Name:
                        print("Healing " + obj.Name + " of type BSpline")
                        nobj = makeCopy(obj,force="BSpline",reparent=reparent)
                    else:
                        print("Healing " + obj.Name + " of type Wire")
                        nobj = makeCopy(obj,force="Wire",reparent=reparent)
                elif ("Radius" in props) and ("FirstAngle" in props):
                    print("Healing " + obj.Name + " of type Circle")
                    nobj = makeCopy(obj,force="Circle",reparent=reparent)
                elif ("DrawMode" in props) and ("FacesNumber" in props):
                    print("Healing " + obj.Name + " of type Polygon")
                    nobj = makeCopy(obj,force="Polygon",reparent=reparent)
                elif ("FillStyle" in props) and ("FontSize" in props):
                    nobj = makeCopy(obj,force="DrawingView",reparent=reparent)
                else:
                    dellist.pop()
                    print("Object " + obj.Name + " is not healable")

    if not got:
        print("No object seems to need healing")
    else:
        print("Healed ",len(dellist)," objects")

    if dellist and delete:
        for n in dellist:
            FreeCAD.ActiveDocument.removeObject(n)

def makeFacebinder(selectionset,name="Facebinder"):
    """makeFacebinder(selectionset,[name]): creates a Facebinder object from a selection set.
    Only faces will be added."""
    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    if not isinstance(selectionset,list):
        selectionset = [selectionset]
    fb = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Facebinder(fb)
    if gui:
        _ViewProviderFacebinder(fb.ViewObject)
    faces = []
    fb.Proxy.addSubobjects(fb,selectionset)
    select(fb)
    return fb


def upgrade(objects,delete=False,force=None):
    """upgrade(objects,delete=False,force=None): Upgrades the given object(s) (can be
    an object or a list of objects). If delete is True, old objects are deleted.
    The force attribute can be used to
    force a certain way of upgrading. It can be: makeCompound, closeGroupWires,
    makeSolid, closeWire, turnToParts, makeFusion, makeShell, makeFaces, draftify,
    joinFaces, makeSketchFace, makeWires
    Returns a dictionary containing two lists, a list of new objects and a list
    of objects to be deleted"""

    import Part, DraftGeomUtils
    from DraftTools import msg

    if not isinstance(objects,list):
        objects = [objects]

    global deleteList, newList
    deleteList = []
    addList = []

    # definitions of actions to perform

    def turnToLine(obj):
        """turns an edge into a Draft line"""
        p1 = obj.Shape.Vertexes[0].Point
        p2 = obj.Shape.Vertexes[-1].Point
        newobj = makeLine(p1,p2)
        addList.append(newobj)
        deleteList.append(obj)
        return newobj

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
        except Part.OCCError:
            return None
        else:
            if sol:
                if sol.isClosed():
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Solid")
                    newobj.Shape = sol
                    addList.append(newobj)
                    deleteList.append(obj)
            return newobj

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
                    edges.append(Part.LineSegment(p1,p0).toShape())
                    w = Part.Wire(Part.__sortEdges__(edges))
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
        params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        preserveFaceColor = params.GetBool("preserveFaceColor") # True
        preserveFaceNames = params.GetBool("preserveFaceNames") # True
        faces = []
        facecolors = [[], []] if (preserveFaceColor) else None
        for obj in objectslist:
            faces.extend(obj.Shape.Faces)
            if (preserveFaceColor):
                """ at this point, obj.Shape.Faces are not in same order as the
                original faces we might have gotten as a result of downgrade, nor do they
                have the same hashCode(); but they still keep reference to their original
                colors - capture that in facecolors.
                Also, cannot w/ .ShapeColor here, need a whole array matching the colors
                of the array of faces per object, only DiffuseColor has that """
                facecolors[0].extend(obj.ViewObject.DiffuseColor)
                facecolors[1] = faces
        sh = Part.makeShell(faces)
        if sh:
            if sh.Faces:
                newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Shell")
                newobj.Shape = sh
                if (preserveFaceNames):
                    import re
                    firstName = objectslist[0].Label
                    nameNoTrailNumbers = re.sub("\d+$", "", firstName)
                    newobj.Label = "{} {}".format(newobj.Label, nameNoTrailNumbers)
                if (preserveFaceColor):
                    """ At this point, sh.Faces are completely new, with different hashCodes
                    and different ordering from obj.Shape.Faces; since we cannot compare
                    via hashCode(), we have to iterate and use a different criteria to find
                    the original matching color """
                    colarray = []
                    for ind, face in enumerate(newobj.Shape.Faces):
                        for fcind, fcface in enumerate(facecolors[1]):
                            if ((face.Area == fcface.Area) and (face.CenterOfMass == fcface.CenterOfMass)):
                                colarray.append(facecolors[0][fcind])
                                break
                    newobj.ViewObject.DiffuseColor = colarray;
                addList.append(newobj)
                deleteList.extend(objectslist)
                return newobj
        return None

    def joinFaces(objectslist):
        """makes one big face from selected objects, if possible"""
        faces = []
        for obj in objectslist:
            faces.extend(obj.Shape.Faces)
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
                except Part.OCCError:
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
            nedges = Part.__sortEdges__(edges[:])
            # for e in nedges: print("debug: ",e.Curve,e.Vertexes[0].Point,e.Vertexes[-1].Point)
            w = Part.Wire(nedges)
        except Part.OCCError:
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

    #print("objects:",objects," edges:",edges," wires:",wires," openwires:",openwires," faces:",faces)
    #print("groups:",groups," curves:",curves," facewires:",facewires, "loneedges:", loneedges)

    if force:
        if force in ["makeCompound","closeGroupWires","makeSolid","closeWire","turnToParts","makeFusion",
                     "makeShell","makeFaces","draftify","joinFaces","makeSketchFace","makeWires","turnToLine"]:
            result = eval(force)(objects)
        else:
            msg(translate("Upgrade: Unknown force method:")+" "+force)
            result = None

    else:

        # applying transformations automatically

        result = None

        # if we have a group: turn each closed wire inside into a face
        if groups:
            result = closeGroupWires(groups)
            if result: msg(translate("draft", "Found groups: closing each open object inside")+"\n")

        # if we have meshes, we try to turn them into shapes
        elif meshes:
            result = turnToParts(meshes)
            if result: msg(translate("draft", "Found mesh(es): turning into Part shapes")+"\n")

        # we have only faces here, no lone edges
        elif faces and (len(wires) + len(openwires) == len(facewires)):

            # we have one shell: we try to make a solid
            if (len(objects) == 1) and (len(faces) > 3):
                result = makeSolid(objects[0])
                if result: msg(translate("draft", "Found 1 solidificable object: solidifying it")+"\n")

            # we have exactly 2 objects: we fuse them
            elif (len(objects) == 2) and (not curves):
                result = makeFusion(objects[0],objects[1])
                if result: msg(translate("draft", "Found 2 objects: fusing them")+"\n")

            # we have many separate faces: we try to make a shell
            elif (len(objects) > 2) and (len(faces) > 1) and (not loneedges):
                result = makeShell(objects)
                if result: msg(translate("draft", "Found several objects: creating a shell")+"\n")

            # we have faces: we try to join them if they are coplanar
            elif len(faces) > 1:
                result = joinFaces(objects)
                if result: msg(translate("draft", "Found several coplanar objects or faces: creating one face")+"\n")

            # only one object: if not parametric, we "draftify" it
            elif len(objects) == 1 and (not objects[0].isDerivedFrom("Part::Part2DObjectPython")):
                result = draftify(objects[0])
                if result: msg(translate("draft", "Found 1 non-parametric objects: draftifying it")+"\n")

        # we have only one object that contains one edge
        elif (not faces) and (len(objects) == 1) and (len(edges) == 1):
            # we have a closed sketch: Extract a face
            if objects[0].isDerivedFrom("Sketcher::SketchObject") and (len(edges[0].Vertexes) == 1):
                result = makeSketchFace(objects[0])
                if result: msg(translate("draft", "Found 1 closed sketch object: creating a face from it")+"\n")
            else:
                # turn to Draft line
                e = objects[0].Shape.Edges[0]
                if isinstance(e.Curve,(Part.LineSegment,Part.Line)):
                    result = turnToLine(objects[0])
                    if result: msg(translate("draft", "Found 1 linear object: converting to line")+"\n")

        # we have only closed wires, no faces
        elif wires and (not faces) and (not openwires):

            # we have a sketch: Extract a face
            if (len(objects) == 1) and objects[0].isDerivedFrom("Sketcher::SketchObject"):
                result = makeSketchFace(objects[0])
                if result: msg(translate("draft", "Found 1 closed sketch object: creating a face from it")+"\n")

            # only closed wires
            else:
                result = makeFaces(objects)
                if result: msg(translate("draft", "Found closed wires: creating faces")+"\n")

        # special case, we have only one open wire. We close it, unless it has only 1 edge!"
        elif (len(openwires) == 1) and (not faces) and (not loneedges):
            result = closeWire(objects[0])
            if result: msg(translate("draft", "Found 1 open wire: closing it")+"\n")

        # only open wires and edges: we try to join their edges
        elif openwires and (not wires) and (not faces):
            result = makeWires(objects)
            if result: msg(translate("draft", "Found several open wires: joining them")+"\n")

        # only loneedges: we try to join them
        elif loneedges and (not facewires):
            result = makeWires(objects)
            if result: msg(translate("draft", "Found several edges: wiring them")+"\n")

        # all other cases, if more than 1 object, make a compound
        elif (len(objects) > 1):
            result = makeCompound(objects)
            if result: msg(translate("draft", "Found several non-treatable objects: creating compound")+"\n")

        # no result has been obtained
        if not result:
            msg(translate("draft", "Unable to upgrade these objects.")+"\n")

    if delete:
        names = []
        for o in deleteList:
            names.append(o.Name)
        deleteList = []
        for n in names:
            FreeCAD.ActiveDocument.removeObject(n)
    select(addList)
    return [addList,deleteList]

def downgrade(objects,delete=False,force=None):
    """downgrade(objects,delete=False,force=None): Downgrades the given object(s) (can be
    an object or a list of objects). If delete is True, old objects are deleted.
    The force attribute can be used to
    force a certain way of downgrading. It can be: explode, shapify, subtr,
    splitFaces, cut2, getWire, splitWires, splitCompounds.
    Returns a dictionary containing two lists, a list of new objects and a list
    of objects to be deleted"""

    import Part, DraftGeomUtils
    from DraftTools import msg

    if not isinstance(objects,list):
        objects = [objects]

    global deleteList, addList
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

    def splitCompounds(objects):
        """split solids contained in compound objects into new objects"""
        result = False
        for o in objects:
            if o.Shape.Solids:
                for s in o.Shape.Solids:
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Solid")
                    newobj.Shape = s
                    addList.append(newobj)
                result = True
                deleteList.append(o)
        return result

    def splitFaces(objects):
        """split faces contained in objects into new objects"""
        result = False
        params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        preserveFaceColor = params.GetBool("preserveFaceColor") # True
        preserveFaceNames = params.GetBool("preserveFaceNames") # True
        for o in objects:
            voDColors = o.ViewObject.DiffuseColor if (preserveFaceColor and hasattr(o,'ViewObject')) else None
            oLabel = o.Label if hasattr(o,'Label') else ""
            if o.Shape.Faces:
                for ind, f in enumerate(o.Shape.Faces):
                    newobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Face")
                    newobj.Shape = f
                    if preserveFaceNames:
                        newobj.Label = "{} {}".format(oLabel, newobj.Label)
                    if preserveFaceColor:
                        """ At this point, some single-color objects might have
                        just a single entry in voDColors for all their faces; handle that"""
                        tcolor = voDColors[ind] if ind<len(voDColors) else voDColors[0]
                        newobj.ViewObject.DiffuseColor[0] = tcolor # does is not applied visually on its own; left just in case
                        newobj.ViewObject.ShapeColor = tcolor # this gets applied, works by itself too
                    addList.append(newobj)
                result = True
                deleteList.append(o)
        return result

    def subtr(objects):
        """subtracts objects from the first one"""
        faces = []
        for o in objects:
            if o.Shape.Faces:
                faces.extend(o.Shape.Faces)
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
    solids = []
    result = None

    for o in objects:
        if o.isDerivedFrom("Part::Feature"):
            for s in o.Shape.Solids:
                solids.append(s)
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
            msg(translate("Upgrade: Unknown force method:")+" "+force)
            result = None

    else:

        # applying transformation automatically

        # we have a block, we explode it
        if (len(objects) == 1) and (getType(objects[0]) == "Block"):
            result = explode(objects[0])
            if result: msg(translate("draft", "Found 1 block: exploding it")+"\n")

        # we have one multi-solids compound object: extract its solids
        elif (len(objects) == 1) and (getType(objects[0]) == "Part") and (len(solids) > 1):
            result = splitCompounds(objects)
            #print(result)
            if result: msg(translate("draft", "Found 1 multi-solids compound: exploding it")+"\n")

        # special case, we have one parametric object: we "de-parametrize" it
        elif (len(objects) == 1) and (objects[0].isDerivedFrom("Part::Feature")) and ("Base" in objects[0].PropertiesList):
            result = shapify(objects[0])
            if result:
                msg(translate("draft", "Found 1 parametric object: breaking its dependencies")+"\n")
                addList.append(result)
                #deleteList.append(objects[0])

        # we have only 2 objects: cut 2nd from 1st
        elif len(objects) == 2:
            result = cut2(objects)
            if result: msg(translate("draft", "Found 2 objects: subtracting them")+"\n")

        elif (len(faces) > 1):

            # one object with several faces: split it
            if len(objects) == 1:
                result = splitFaces(objects)
                if result: msg(translate("draft", "Found several faces: splitting them")+"\n")

            # several objects: remove all the faces from the first one
            else:
                result = subtr(objects)
                if result: msg(translate("draft", "Found several objects: subtracting them from the first one")+"\n")

        # only one face: we extract its wires
        elif (len(faces) > 0):
            result = getWire(objects[0])
            if result: msg(translate("draft", "Found 1 face: extracting its wires")+"\n")

        # no faces: split wire into single edges
        elif not onlyedges:
            result = splitWires(objects)
            if result: msg(translate("draft", "Found only wires: extracting their edges")+"\n")

        # no result has been obtained
        if not result:
            msg(translate("draft", "No more downgrade possible")+"\n")

    if delete:
        names = []
        for o in deleteList:
            names.append(o.Name)
        deleteList = []
        for n in names:
            FreeCAD.ActiveDocument.removeObject(n)
    select(addList)
    return [addList,deleteList]


def makeWorkingPlaneProxy(placement):
    "creates a Working Plane proxy object in the current document"
    if FreeCAD.ActiveDocument:
        obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","WPProxy")
        WorkingPlaneProxy(obj)
        if FreeCAD.GuiUp:
            ViewProviderWorkingPlaneProxy(obj.ViewObject)
            obj.ViewObject.Proxy.writeCamera()
            obj.ViewObject.Proxy.writeState()
        obj.Placement = placement
        return obj


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

    def onChanged(self, obj, prop):
        pass

class _ViewProviderDraft:
    "The base class for Draft Viewproviders"

    def __init__(self, vobj):
        vobj.Proxy = self
        self.Object = vobj.Object
        vobj.addProperty("App::PropertyEnumeration","Pattern","Draft",QT_TRANSLATE_NOOP("App::Property","Defines a hatch pattern"))
        vobj.addProperty("App::PropertyFloat","PatternSize","Draft",QT_TRANSLATE_NOOP("App::Property","Sets the size of the pattern"))
        vobj.Pattern = ["None"]+list(svgpatterns().keys())
        vobj.PatternSize = 1

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def attach(self,vobj):
        self.texture = None
        self.texcoords = None
        self.Object = vobj.Object
        self.onChanged(vobj,"Pattern")
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
        if prop in ["TextureImage","Pattern","DiffuseColor"]:
            if hasattr(self.Object,"Shape"):
                if self.Object.Shape.Faces:
                    from pivy import coin
                    from PySide import QtCore
                    path = None
                    if hasattr(vobj,"TextureImage"):
                        if vobj.TextureImage:
                            path = vobj.TextureImage
                    if not path:
                        if hasattr(vobj,"Pattern"):
                            if str(vobj.Pattern) in list(svgpatterns().keys()):
                                path = svgpatterns()[vobj.Pattern][1]
                            else:
                                path = "None"
                    if path and vobj.RootNode:
                        if vobj.RootNode.getChildren().getLength() > 2:
                            if vobj.RootNode.getChild(2).getChildren().getLength() > 0:
                                if vobj.RootNode.getChild(2).getChild(0).getChildren().getLength() > 2:
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
                                        if ".SVG" in path.upper():
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
                    vS = FreeCAD.Vector(self.texcoords.directionS.getValue().getValue())
                    vT = FreeCAD.Vector(self.texcoords.directionT.getValue().getValue())
                    vS.Length = s
                    vT.Length = s
                    self.texcoords.directionS.setValue(vS.x,vS.y,vS.z)
                    self.texcoords.directionT.setValue(vT.x,vT.y,vT.z)
        return

    def execute(self,vobj):
        return

    def setEdit(self,vobj,mode=0):
        if mode == 0:
            FreeCADGui.runCommand("Draft_Edit")
            return True
        return False

    def unsetEdit(self,vobj,mode=0):
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
        if hasattr(self.Object,"Group"):
            objs.extend(self.Object.Group)
        return objs

class _ViewProviderDraftAlt(_ViewProviderDraft):
    "a view provider that doesn't swallow its base object"

    def __init__(self,vobj):
        _ViewProviderDraft.__init__(self,vobj)

    def claimChildren(self):
        return []

class _ViewProviderDraftPart(_ViewProviderDraftAlt):
    "a view provider that displays a Part icon instead of a Draft icon"

    def __init__(self,vobj):
        _ViewProviderDraftAlt.__init__(self,vobj)

    def getIcon(self):
        return ":/icons/Tree_Part.svg"


class _Dimension(_DraftObject):
    "The Draft Dimension object"
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"Dimension")
        obj.addProperty("App::PropertyVectorDistance","Start","Draft",QT_TRANSLATE_NOOP("App::Property","Startpoint of dimension"))
        obj.addProperty("App::PropertyVectorDistance","End","Draft",QT_TRANSLATE_NOOP("App::Property","Endpoint of dimension"))
        obj.addProperty("App::PropertyVector","Normal","Draft",QT_TRANSLATE_NOOP("App::Property","The normal direction of this dimension"))
        obj.addProperty("App::PropertyVector","Direction","Draft",QT_TRANSLATE_NOOP("App::Property","The normal direction of this dimension"))
        obj.addProperty("App::PropertyVectorDistance","Dimline","Draft",QT_TRANSLATE_NOOP("App::Property","Point through which the dimension line passes"))
        obj.addProperty("App::PropertyLink","Support","Draft",QT_TRANSLATE_NOOP("App::Property","The object measured by this dimension"))
        obj.addProperty("App::PropertyLinkSubList","LinkedGeometry","Draft",QT_TRANSLATE_NOOP("App::Property","The geometry this dimension is linked to"))
        obj.addProperty("App::PropertyLength","Distance","Draft",QT_TRANSLATE_NOOP("App::Property","The measurement of this dimension"))
        obj.addProperty("App::PropertyBool","Diameter","Draft",QT_TRANSLATE_NOOP("App::Property","For arc/circle measurements, false = radius, true = diameter"))
        obj.Start = FreeCAD.Vector(0,0,0)
        obj.End = FreeCAD.Vector(1,0,0)
        obj.Dimline = FreeCAD.Vector(0,1,0)
        obj.Normal = FreeCAD.Vector(0,0,1)

    def onChanged(self,obj,prop):
        if hasattr(obj,"Distance"):
            obj.setEditorMode('Distance',1)
        if hasattr(obj,"Normal"):
            obj.setEditorMode('Normal',2)
        if hasattr(obj,"Support"):
            obj.setEditorMode('Support',2)

    def execute(self, obj):
        import DraftGeomUtils
        # set start point and end point according to the linked geometry
        if obj.LinkedGeometry:
            if len(obj.LinkedGeometry) == 1:
                lobj = obj.LinkedGeometry[0][0]
                lsub = obj.LinkedGeometry[0][1]
                if len(lsub) == 1:
                    if "Edge" in lsub[0]:
                        n = int(lsub[0][4:])-1
                        edge = lobj.Shape.Edges[n]
                        if DraftGeomUtils.geomType(edge) == "Line":
                            obj.Start = edge.Vertexes[0].Point
                            obj.End = edge.Vertexes[-1].Point
                        elif DraftGeomUtils.geomType(edge) == "Circle":
                            c = edge.Curve.Center
                            r = edge.Curve.Radius
                            a = edge.Curve.Axis
                            ray = obj.Dimline.sub(c).projectToPlane(Vector(0,0,0),a)
                            if (ray.Length == 0):
                                ray = a.cross(Vector(1,0,0))
                                if (ray.Length == 0):
                                    ray = a.cross(Vector(0,1,0))
                            ray = DraftVecUtils.scaleTo(ray,r)
                            if hasattr(obj,"Diameter"):
                                if obj.Diameter:
                                    obj.Start = c.add(ray.negative())
                                    obj.End = c.add(ray)
                                else:
                                    obj.Start = c
                                    obj.End = c.add(ray)
                elif len(lsub) == 2:
                    if ("Vertex" in lsub[0]) and ("Vertex" in lsub[1]):
                        n1 = int(lsub[0][6:])-1
                        n2 = int(lsub[1][6:])-1
                        obj.Start = lobj.Shape.Vertexes[n1].Point
                        obj.End = lobj.Shape.Vertexes[n2].Point
            elif len(obj.LinkedGeometry) == 2:
                lobj1 = obj.LinkedGeometry[0][0]
                lobj2 = obj.LinkedGeometry[1][0]
                lsub1 = obj.LinkedGeometry[0][1]
                lsub2 = obj.LinkedGeometry[1][1]
                if (len(lsub1) == 1) and (len(lsub2) == 1):
                    if ("Vertex" in lsub1[0]) and ("Vertex" in lsub2[1]):
                        n1 = int(lsub1[0][6:])-1
                        n2 = int(lsub2[0][6:])-1
                        obj.Start = lobj1.Shape.Vertexes[n1].Point
                        obj.End = lobj2.Shape.Vertexes[n2].Point
        # set the distance property
        total_len = (obj.Start.sub(obj.End)).Length
        if round(obj.Distance.Value, precision()) != round(total_len, precision()):
            obj.Distance = total_len
        if gui:
            if obj.ViewObject:
                obj.ViewObject.update()


class _ViewProviderDimension(_ViewProviderDraft):
    "A View Provider for the Draft Dimension object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyLength","FontSize","Draft",QT_TRANSLATE_NOOP("App::Property","Font size"))
        obj.addProperty("App::PropertyInteger","Decimals","Draft",QT_TRANSLATE_NOOP("App::Property","The number of decimals to show"))
        obj.addProperty("App::PropertyLength","ArrowSize","Draft",QT_TRANSLATE_NOOP("App::Property","Arrow size"))
        obj.addProperty("App::PropertyLength","TextSpacing","Draft",QT_TRANSLATE_NOOP("App::Property","The spacing between the text and the dimension line"))
        obj.addProperty("App::PropertyEnumeration","ArrowType","Draft",QT_TRANSLATE_NOOP("App::Property","Arrow type"))
        obj.addProperty("App::PropertyFont","FontName","Draft",QT_TRANSLATE_NOOP("App::Property","Font name"))
        obj.addProperty("App::PropertyFloat","LineWidth","Draft",QT_TRANSLATE_NOOP("App::Property","Line width"))
        obj.addProperty("App::PropertyColor","LineColor","Draft",QT_TRANSLATE_NOOP("App::Property","Line color"))
        obj.addProperty("App::PropertyDistance","ExtLines","Draft",QT_TRANSLATE_NOOP("App::Property","Length of the extension lines"))
        obj.addProperty("App::PropertyDistance","DimOvershoot","Draft",QT_TRANSLATE_NOOP("App::Property","The distance the dimension line is extended past the extension lines"))
        obj.addProperty("App::PropertyDistance","ExtOvershoot","Draft",QT_TRANSLATE_NOOP("App::Property","Length of the extension line above the dimension line"))
        obj.addProperty("App::PropertyBool","FlipArrows","Draft",QT_TRANSLATE_NOOP("App::Property","Rotate the dimension arrows 180 degrees"))
        obj.addProperty("App::PropertyBool","FlipText","Draft",QT_TRANSLATE_NOOP("App::Property","Rotate the dimension text 180 degrees"))
        obj.addProperty("App::PropertyBool","ShowUnit","Draft",QT_TRANSLATE_NOOP("App::Property","Show the unit suffix"))
        obj.addProperty("App::PropertyVectorDistance","TextPosition","Draft",QT_TRANSLATE_NOOP("App::Property","The position of the text. Leave (0,0,0) for automatic position"))
        obj.addProperty("App::PropertyString","Override","Draft",QT_TRANSLATE_NOOP("App::Property","Text override. Use $dim to insert the dimension length"))
        obj.addProperty("App::PropertyString","UnitOverride","Draft",QT_TRANSLATE_NOOP("App::Property","A unit to express the measurement. Leave blank for system default"))
        obj.FontSize = getParam("textheight",0.20)
        obj.TextSpacing = getParam("dimspacing",0.05)
        obj.FontName = getParam("textfont","")
        obj.ArrowSize = getParam("arrowsize",0.1)
        obj.ArrowType = arrowtypes
        obj.ArrowType = arrowtypes[getParam("dimsymbol",0)]
        obj.ExtLines = getParam("extlines",0.3)
        obj.DimOvershoot = getParam("dimovershoot",0)
        obj.ExtOvershoot = getParam("extovershoot",0)
        obj.Decimals = getParam("dimPrecision",2)
        obj.ShowUnit = getParam("showUnit",True)
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
        self.text.string = "d" # some versions of coin crash if string is not set
        self.text3d.string = "d"
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
        self.transDimOvershoot1 = coin.SoTransform()
        self.transDimOvershoot2 = coin.SoTransform()
        self.transExtOvershoot1 = coin.SoTransform()
        self.transExtOvershoot2 = coin.SoTransform()
        self.marks = coin.SoSeparator()
        self.marksDimOvershoot = coin.SoSeparator()
        self.marksExtOvershoot = coin.SoSeparator()
        self.drawstyle = coin.SoDrawStyle()
        self.line = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        self.coords = coin.SoCoordinate3()
        self.node = coin.SoGroup()
        self.node.addChild(self.color)
        self.node.addChild(self.drawstyle)
        self.node.addChild(self.coords)
        self.node.addChild(self.line)
        self.node.addChild(self.marks)
        self.node.addChild(self.marksDimOvershoot)
        self.node.addChild(self.marksExtOvershoot)
        self.node.addChild(label)
        self.node3d = coin.SoGroup()
        self.node3d.addChild(self.color)
        self.node3d.addChild(self.drawstyle)
        self.node3d.addChild(self.coords)
        self.node3d.addChild(self.line)
        self.node3d.addChild(self.marks)
        self.node3d.addChild(self.marksDimOvershoot)
        self.node3d.addChild(self.marksExtOvershoot)
        self.node3d.addChild(label3d)
        vobj.addDisplayMode(self.node,"2D")
        vobj.addDisplayMode(self.node3d,"3D")
        self.updateData(vobj.Object,"Start")
        self.onChanged(vobj,"FontSize")
        self.onChanged(vobj,"FontName")
        self.onChanged(vobj,"ArrowType")
        self.onChanged(vobj,"LineColor")
        self.onChanged(vobj,"DimOvershoot")
        self.onChanged(vobj,"ExtOvershoot")

    def updateData(self, obj, prop):
        "called when the base object is changed"
        import DraftGui
        if prop in ["Start","End","Dimline","Direction"]:

            if obj.Start == obj.End:
                return

            if not hasattr(self,"node"):
                return

            import Part, DraftGeomUtils
            from pivy import coin

            # calculate the 4 points
            self.p1 = obj.Start
            self.p4 = obj.End
            base = None
            if hasattr(obj,"Direction"):
                if not DraftVecUtils.isNull(obj.Direction):
                    v2 = self.p1.sub(obj.Dimline)
                    v3 = self.p4.sub(obj.Dimline)
                    v2 = DraftVecUtils.project(v2,obj.Direction)
                    v3 = DraftVecUtils.project(v3,obj.Direction)
                    self.p2 = obj.Dimline.add(v2)
                    self.p3 = obj.Dimline.add(v3)
                    if DraftVecUtils.equals(self.p2,self.p3):
                        base = None
                        proj = None
                    else:
                        base = Part.LineSegment(self.p2,self.p3).toShape()
                        proj = DraftGeomUtils.findDistance(self.p1,base)
                        if proj:
                            proj = proj.negative()
            if not base:
                if DraftVecUtils.equals(self.p1,self.p4):
                    base = None
                    proj = None
                else:
                    base = Part.LineSegment(self.p1,self.p4).toShape()
                    proj = DraftGeomUtils.findDistance(obj.Dimline,base)
                if proj:
                    self.p2 = self.p1.add(proj.negative())
                    self.p3 = self.p4.add(proj.negative())
                else:
                    self.p2 = self.p1
                    self.p3 = self.p4
            if proj:
                if hasattr(obj.ViewObject,"ExtLines"):
                    dmax = obj.ViewObject.ExtLines.Value
                    if dmax and (proj.Length > dmax):
                        if (dmax > 0):
                            self.p1 = self.p2.add(DraftVecUtils.scaleTo(proj,dmax))
                            self.p4 = self.p3.add(DraftVecUtils.scaleTo(proj,dmax))
                        else:
                            rest = proj.Length + dmax
                            self.p1 = self.p2.add(DraftVecUtils.scaleTo(proj,rest))
                            self.p4 = self.p3.add(DraftVecUtils.scaleTo(proj,rest))
            else:
                proj = (self.p3.sub(self.p2)).cross(Vector(0,0,1))

            # calculate the arrows positions
            self.trans1.translation.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.coord1.point.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.trans2.translation.setValue((self.p3.x,self.p3.y,self.p3.z))
            self.coord2.point.setValue((self.p3.x,self.p3.y,self.p3.z))

            # calculate dimension and extension lines overshoots positions
            self.transDimOvershoot1.translation.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.transDimOvershoot2.translation.setValue((self.p3.x,self.p3.y,self.p3.z))
            self.transExtOvershoot1.translation.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.transExtOvershoot2.translation.setValue((self.p3.x,self.p3.y,self.p3.z))

            # calculate the text position and orientation
            if hasattr(obj,"Normal"):
                if DraftVecUtils.isNull(obj.Normal):
                    if proj:
                        norm = (self.p3.sub(self.p2).cross(proj)).negative()
                    else:
                        norm = Vector(0,0,1)
                else:
                    norm = FreeCAD.Vector(obj.Normal)
            else:
                if proj:
                    norm = (self.p3.sub(self.p2).cross(proj)).negative()
                else:
                    norm = Vector(0,0,1)
            if not DraftVecUtils.isNull(norm):
                norm.normalize()
            u = self.p3.sub(self.p2)
            u.normalize()
            v1 = norm.cross(u)
            rot1 = FreeCAD.Placement(DraftVecUtils.getPlaneRotation(u,v1,norm)).Rotation.Q
            self.transDimOvershoot1.rotation.setValue((rot1[0],rot1[1],rot1[2],rot1[3]))
            self.transDimOvershoot2.rotation.setValue((rot1[0],rot1[1],rot1[2],rot1[3]))
            if hasattr(obj.ViewObject,"FlipArrows"):
                if obj.ViewObject.FlipArrows:
                    u = u.negative()
            v2 = norm.cross(u)
            rot2 = FreeCAD.Placement(DraftVecUtils.getPlaneRotation(u,v2,norm)).Rotation.Q
            self.trans1.rotation.setValue((rot2[0],rot2[1],rot2[2],rot2[3]))
            self.trans2.rotation.setValue((rot2[0],rot2[1],rot2[2],rot2[3]))
            if self.p1 != self.p2:
                u3 = self.p1.sub(self.p2)
                u3.normalize()
                v3 = norm.cross(u3)
                rot3 = FreeCAD.Placement(DraftVecUtils.getPlaneRotation(u3,v3,norm)).Rotation.Q
                self.transExtOvershoot1.rotation.setValue((rot3[0],rot3[1],rot3[2],rot3[3]))
                self.transExtOvershoot2.rotation.setValue((rot3[0],rot3[1],rot3[2],rot3[3]))
            if hasattr(obj.ViewObject,"TextSpacing"):
                offset = DraftVecUtils.scaleTo(v1,obj.ViewObject.TextSpacing.Value)
            else:
                offset = DraftVecUtils.scaleTo(v1,0.05)
            rott = rot1
            if hasattr(obj.ViewObject,"FlipText"):
                if obj.ViewObject.FlipText:
                    rott = FreeCAD.Rotation(*rott).multiply(FreeCAD.Rotation(norm,180)).Q
                    offset = offset.negative()
            # setting text
            try:
                m = obj.ViewObject.DisplayMode
            except: # swallow all exceptions here since it always fails on first run (Displaymode enum no set yet)
                m = ["2D","3D"][getParam("dimstyle",0)]
            if m == "3D":
                offset = offset.negative()
            self.tbase = (self.p2.add((self.p3.sub(self.p2).multiply(0.5)))).add(offset)
            if hasattr(obj.ViewObject,"TextPosition"):
                if not DraftVecUtils.isNull(obj.ViewObject.TextPosition):
                    self.tbase = obj.ViewObject.TextPosition
            self.textpos.translation.setValue([self.tbase.x,self.tbase.y,self.tbase.z])
            self.textpos.rotation = coin.SbRotation(rott[0],rott[1],rott[2],rott[3])
            su = True
            if hasattr(obj.ViewObject,"ShowUnit"):
                su = obj.ViewObject.ShowUnit
            # set text value
            l = self.p3.sub(self.p2).Length
            unit = None
            if hasattr(obj.ViewObject,"UnitOverride"):
                unit = obj.ViewObject.UnitOverride
            # special representation if "Building US" scheme
            if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt("UserSchema",0) == 5:
                s = FreeCAD.Units.Quantity(l,FreeCAD.Units.Length).UserString
                self.string = s.replace("' ","'- ")
                self.string = s.replace("+"," ")
            elif hasattr(obj.ViewObject,"Decimals"):
                self.string = DraftGui.displayExternal(l,obj.ViewObject.Decimals,'Length',su,unit)
            else:
                self.string = DraftGui.displayExternal(l,None,'Length',su,unit)
            if hasattr(obj.ViewObject,"Override"):
                if obj.ViewObject.Override:
                    self.string = obj.ViewObject.Override.replace("$dim",\
                            self.string)
            self.text.string = self.text3d.string = stringencodecoin(self.string)

            # set the lines
            if m == "3D":
                # calculate the spacing of the text
                textsize = (len(self.string)*obj.ViewObject.FontSize.Value)/4.0
                spacing = ((self.p3.sub(self.p2)).Length/2.0) - textsize
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

        if (prop == "FontSize") and hasattr(vobj,"FontSize"):
            if hasattr(self,"font"):
                self.font.size = vobj.FontSize.Value
            if hasattr(self,"font3d"):
                self.font3d.size = vobj.FontSize.Value*100
            vobj.Object.touch()
        elif (prop == "FontName") and hasattr(vobj,"FontName"):
            if hasattr(self,"font") and hasattr(self,"font3d"):
                self.font.name = self.font3d.name = str(vobj.FontName)
                vobj.Object.touch()
        elif (prop == "LineColor") and hasattr(vobj,"LineColor"):
            if hasattr(self,"color"):
                c = vobj.LineColor
                self.color.rgb.setValue(c[0],c[1],c[2])
        elif (prop == "LineWidth") and hasattr(vobj,"LineWidth"):
            if hasattr(self,"drawstyle"):
                self.drawstyle.lineWidth = vobj.LineWidth
        elif (prop in ["ArrowSize","ArrowType"]) and hasattr(vobj,"ArrowSize"):
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
                s = vobj.ArrowSize.Value
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
                vobj.Object.touch()
        elif (prop == "DimOvershoot") and hasattr(vobj,"DimOvershoot"):
            from pivy import coin

            # set scale
            s = vobj.DimOvershoot.Value
            self.transDimOvershoot1.scaleFactor.setValue((s,s,s))
            self.transDimOvershoot2.scaleFactor.setValue((s,s,s))

            # remove existing nodes
            self.node.removeChild(self.marksDimOvershoot)
            self.node3d.removeChild(self.marksDimOvershoot)

            # set new nodes
            self.marksDimOvershoot = coin.SoSeparator()
            if vobj.DimOvershoot.Value:
                self.marksDimOvershoot.addChild(self.color)
                s1 = coin.SoSeparator()
                s1.addChild(self.transDimOvershoot1)
                s1.addChild(dimDash((-1,0,0),(0,0,0)))
                self.marksDimOvershoot.addChild(s1)
                s2 = coin.SoSeparator()
                s2.addChild(self.transDimOvershoot2)
                s2.addChild(dimDash((0,0,0),(1,0,0)))
                self.marksDimOvershoot.addChild(s2)
            self.node.insertChild(self.marksDimOvershoot,2)
            self.node3d.insertChild(self.marksDimOvershoot,2)
            vobj.Object.touch()
        elif (prop == "ExtOvershoot") and hasattr(vobj,"ExtOvershoot"):
            from pivy import coin

            # set scale
            s = vobj.ExtOvershoot.Value
            self.transExtOvershoot1.scaleFactor.setValue((s,s,s))
            self.transExtOvershoot2.scaleFactor.setValue((s,s,s))

            # remove existing nodes
            self.node.removeChild(self.marksExtOvershoot)
            self.node3d.removeChild(self.marksExtOvershoot)

            # set new nodes
            self.marksExtOvershoot = coin.SoSeparator()
            if vobj.ExtOvershoot.Value:
                self.marksExtOvershoot.addChild(self.color)
                s1 = coin.SoSeparator()
                s1.addChild(self.transExtOvershoot1)
                s1.addChild(dimDash((0,0,0),(-1,0,0)))
                self.marksExtOvershoot.addChild(s1)
                s2 = coin.SoSeparator()
                s2.addChild(self.transExtOvershoot2)
                s2.addChild(dimDash((0,0,0),(-1,0,0)))
                self.marksExtOvershoot.addChild(s2)
            self.node.insertChild(self.marksExtOvershoot,2)
            self.node3d.insertChild(self.marksExtOvershoot,2)
            vobj.Object.touch()
        else:
            self.updateData(vobj.Object,"Start")

    def doubleClicked(self,vobj):
        self.setEdit(vobj)

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
            "   c None",
            ".  c #000000",
            "+  c #FFFF00",
            "@  c #FFFFFF",
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
        obj.addProperty("App::PropertyAngle","FirstAngle","Draft",QT_TRANSLATE_NOOP("App::Property","Start angle of the dimension"))
        obj.addProperty("App::PropertyAngle","LastAngle","Draft",QT_TRANSLATE_NOOP("App::Property","End angle of the dimension"))
        obj.addProperty("App::PropertyVectorDistance","Dimline","Draft",QT_TRANSLATE_NOOP("App::Property","Point through which the dimension line passes"))
        obj.addProperty("App::PropertyVectorDistance","Center","Draft",QT_TRANSLATE_NOOP("App::Property","The center point of this dimension"))
        obj.addProperty("App::PropertyVector","Normal","Draft",QT_TRANSLATE_NOOP("App::Property","The normal direction of this dimension"))
        obj.addProperty("App::PropertyLink","Support","Draft",QT_TRANSLATE_NOOP("App::Property","The object measured by this dimension"))
        obj.addProperty("App::PropertyLinkSubList","LinkedGeometry","Draft",QT_TRANSLATE_NOOP("App::Property","The geometry this dimension is linked to"))
        obj.addProperty("App::PropertyAngle","Angle","Draft",QT_TRANSLATE_NOOP("App::Property","The measurement of this dimension"))
        obj.FirstAngle = 0
        obj.LastAngle = 90
        obj.Dimline = FreeCAD.Vector(0,1,0)
        obj.Center = FreeCAD.Vector(0,0,0)
        obj.Normal = FreeCAD.Vector(0,0,1)

    def onChanged(self,obj,prop):
        if hasattr(obj,"Angle"):
            obj.setEditorMode('Angle',1)
        if hasattr(obj,"Normal"):
            obj.setEditorMode('Normal',2)
        if hasattr(obj,"Support"):
            obj.setEditorMode('Support',2)

    def execute(self, fp):
        if fp.ViewObject:
            fp.ViewObject.update()

class _ViewProviderAngularDimension(_ViewProviderDraft):
    "A View Provider for the Draft Angular Dimension object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyLength","FontSize","Draft",QT_TRANSLATE_NOOP("App::Property","Font size"))
        obj.addProperty("App::PropertyInteger","Decimals","Draft",QT_TRANSLATE_NOOP("App::Property","The number of decimals to show"))
        obj.addProperty("App::PropertyFont","FontName","Draft",QT_TRANSLATE_NOOP("App::Property","Font name"))
        obj.addProperty("App::PropertyLength","ArrowSize","Draft",QT_TRANSLATE_NOOP("App::Property","Arrow size"))
        obj.addProperty("App::PropertyLength","TextSpacing","Draft",QT_TRANSLATE_NOOP("App::Property","The spacing between the text and the dimension line"))
        obj.addProperty("App::PropertyEnumeration","ArrowType","Draft",QT_TRANSLATE_NOOP("App::Property","Arrow type"))
        obj.addProperty("App::PropertyFloat","LineWidth","Draft",QT_TRANSLATE_NOOP("App::Property","Line width"))
        obj.addProperty("App::PropertyColor","LineColor","Draft",QT_TRANSLATE_NOOP("App::Property","Line color"))
        obj.addProperty("App::PropertyBool","FlipArrows","Draft",QT_TRANSLATE_NOOP("App::Property","Rotate the dimension arrows 180 degrees"))
        obj.addProperty("App::PropertyBool","ShowUnit","Draft",QT_TRANSLATE_NOOP("App::Property","Show the unit suffix"))
        obj.addProperty("App::PropertyVectorDistance","TextPosition","Draft",QT_TRANSLATE_NOOP("App::Property","The position of the text. Leave (0,0,0) for automatic position"))
        obj.addProperty("App::PropertyString","Override","Draft",QT_TRANSLATE_NOOP("App::Property","Text override. Use 'dim' to insert the dimension length"))
        obj.FontSize = getParam("textheight",0.20)
        obj.FontName = getParam("textfont","")
        obj.TextSpacing = getParam("dimspacing",0.05)
        obj.ArrowSize = getParam("arrowsize",0.1)
        obj.ArrowType = arrowtypes
        obj.ArrowType = arrowtypes[getParam("dimsymbol",0)]
        obj.Override = ''
        obj.Decimals = getParam("dimPrecision",2)
        obj.ShowUnit = getParam("showUnit",True)
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
        self.text.string = "d" # some versions of coin crash if string is not set
        self.text3d.string = "d"
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
            import DraftGui
            text = None
            ivob = None
            arcsegs = 24

            # calculate the arc data
            if DraftVecUtils.isNull(obj.Normal):
                norm = Vector(0,0,1)
            else:
                norm = obj.Normal
            radius = (obj.Dimline.sub(obj.Center)).Length
            self.circle = Part.makeCircle(radius,obj.Center,norm,obj.FirstAngle.Value,obj.LastAngle.Value)
            self.p2 = self.circle.Vertexes[0].Point
            self.p3 = self.circle.Vertexes[-1].Point
            mp = DraftGeomUtils.findMidpoint(self.circle.Edges[0])
            ray = mp.sub(obj.Center)

            # set text value
            if obj.LastAngle.Value > obj.FirstAngle.Value:
                a = obj.LastAngle.Value - obj.FirstAngle.Value
            else:
                a = (360 - obj.FirstAngle.Value) + obj.LastAngle.Value
            su = True
            if hasattr(obj.ViewObject,"ShowUnit"):
                su = obj.ViewObject.ShowUnit
            if hasattr(obj.ViewObject,"Decimals"):
                self.string = DraftGui.displayExternal(a,obj.ViewObject.Decimals,'Angle',su)
            else:
                self.string = DraftGui.displayExternal(a,None,'Angle',su)
            if obj.ViewObject.Override:
                self.string = obj.ViewObject.Override.replace("$dim",\
                    self.string)
            self.text.string = self.text3d.string = stringencodecoin(self.string)

            # check display mode
            try:
                m = obj.ViewObject.DisplayMode
            except: # swallow all exceptions here since it always fails on first run (Displaymode enum no set yet)
                m = ["2D","3D"][getParam("dimstyle",0)]

            # set the arc
            if m == "3D":
                # calculate the spacing of the text
                spacing = (len(self.string)*obj.ViewObject.FontSize.Value)/8.0
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
                self.arc.coordIndex.setValues(0,len(pts1)+len(pts2)+1,list(range(len(pts1)))+[-1]+list(range(i1,i2)))
                if (len(pts1) >= 3) and (len(pts2) >= 3):
                    self.circle1 = Part.Arc(Vector(pts1[0][0],pts1[0][1],pts1[0][2]),Vector(pts1[1][0],pts1[1][1],pts1[1][2]),Vector(pts1[-1][0],pts1[-1][1],pts1[-1][2])).toShape()
                    self.circle2 = Part.Arc(Vector(pts2[0][0],pts2[0][1],pts2[0][2]),Vector(pts2[1][0],pts2[1][1],pts2[1][2]),Vector(pts2[-1][0],pts2[-1][1],pts2[-1][2])).toShape()
            else:
                pts = []
                for i in range(arcsegs+1):
                    p = self.circle.valueAt(self.circle.FirstParameter+((self.circle.LastParameter-self.circle.FirstParameter)/arcsegs)*i)
                    pts.append([p.x,p.y,p.z])
                self.coords.point.setValues(pts)
                self.arc.coordIndex.setValues(0,arcsegs+1,list(range(arcsegs+1)))

            # set the arrow coords and rotation
            self.trans1.translation.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.coord1.point.setValue((self.p2.x,self.p2.y,self.p2.z))
            self.trans2.translation.setValue((self.p3.x,self.p3.y,self.p3.z))
            self.coord2.point.setValue((self.p3.x,self.p3.y,self.p3.z))
            # calculate small chords to make arrows look better
            arrowlength = 4*obj.ViewObject.ArrowSize.Value
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
                offset = DraftVecUtils.scaleTo(offset,obj.ViewObject.TextSpacing.Value)
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
                self.font.size = vobj.FontSize.Value
            if hasattr(self,"font3d"):
                self.font3d.size = vobj.FontSize.Value*100
            vobj.Object.touch()
        elif prop == "FontName":
            if hasattr(self,"font") and hasattr(self,"font3d"):
                self.font.name = self.font3d.name = str(vobj.FontName)
                vobj.Object.touch()
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
                s = vobj.ArrowSize.Value
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
                vobj.Object.touch()
        else:
            self.updateData(vobj.Object, None)

    def doubleClicked(self,vobj):
        self.setEdit(vobj)

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
                        "   c None",
                        ".  c #000000",
                        "+  c #FFFF00",
                        "@  c #FFFFFF",
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
        obj.addProperty("App::PropertyDistance","Length","Draft",QT_TRANSLATE_NOOP("App::Property","Length of the rectangle"))
        obj.addProperty("App::PropertyDistance","Height","Draft",QT_TRANSLATE_NOOP("App::Property","Height of the rectangle"))
        obj.addProperty("App::PropertyLength","FilletRadius","Draft",QT_TRANSLATE_NOOP("App::Property","Radius to use to fillet the corners"))
        obj.addProperty("App::PropertyLength","ChamferSize","Draft",QT_TRANSLATE_NOOP("App::Property","Size of the chamfer to give to the corners"))
        obj.addProperty("App::PropertyBool","MakeFace","Draft",QT_TRANSLATE_NOOP("App::Property","Create a face"))
        obj.addProperty("App::PropertyInteger","Rows","Draft",QT_TRANSLATE_NOOP("App::Property","Horizontal subdivisions of this rectangle"))
        obj.addProperty("App::PropertyInteger","Columns","Draft",QT_TRANSLATE_NOOP("App::Property","Vertical subdivisions of this rectangle"))
        obj.MakeFace = getParam("fillmode",True)
        obj.Length=1
        obj.Height=1
        obj.Rows=1
        obj.Columns=1

    def execute(self, obj):
        if (obj.Length.Value != 0) and (obj.Height.Value != 0):
            import Part, DraftGeomUtils
            plm = obj.Placement
            shape = None
            if hasattr(obj,"Rows") and hasattr(obj,"Columns"):
                if obj.Rows > 1:
                    rows = obj.Rows
                else:
                    rows = 1
                if obj.Columns > 1:
                    columns = obj.Columns
                else:
                    columns = 1
                if (rows > 1) or (columns > 1):
                    shapes = []
                    l = obj.Length.Value/columns
                    h = obj.Height.Value/rows
                    for i in range(columns):
                        for j in range(rows):
                            p1 = Vector(i*l,j*h,0)
                            p2 = Vector(p1.x+l,p1.y,p1.z)
                            p3 = Vector(p1.x+l,p1.y+h,p1.z)
                            p4 = Vector(p1.x,p1.y+h,p1.z)
                            p = Part.makePolygon([p1,p2,p3,p4,p1])
                            if "ChamferSize" in obj.PropertiesList:
                                if obj.ChamferSize.Value != 0:
                                    w = DraftGeomUtils.filletWire(p,obj.ChamferSize.Value,chamfer=True)
                                    if w:
                                        p = w
                            if "FilletRadius" in obj.PropertiesList:
                                if obj.FilletRadius.Value != 0:
                                    w = DraftGeomUtils.filletWire(p,obj.FilletRadius.Value)
                                    if w:
                                        p = w
                            if hasattr(obj,"MakeFace"):
                                if obj.MakeFace:
                                    p = Part.Face(p)
                            shapes.append(p)
                    if shapes:
                        shape = Part.makeCompound(shapes)
            if not shape:
                p1 = Vector(0,0,0)
                p2 = Vector(p1.x+obj.Length.Value,p1.y,p1.z)
                p3 = Vector(p1.x+obj.Length.Value,p1.y+obj.Height.Value,p1.z)
                p4 = Vector(p1.x,p1.y+obj.Height.Value,p1.z)
                shape = Part.makePolygon([p1,p2,p3,p4,p1])
                if "ChamferSize" in obj.PropertiesList:
                    if obj.ChamferSize.Value != 0:
                        w = DraftGeomUtils.filletWire(shape,obj.ChamferSize.Value,chamfer=True)
                        if w:
                            shape = w
                if "FilletRadius" in obj.PropertiesList:
                    if obj.FilletRadius.Value != 0:
                        w = DraftGeomUtils.filletWire(shape,obj.FilletRadius.Value)
                        if w:
                            shape = w
                if hasattr(obj,"MakeFace"):
                    if obj.MakeFace:
                        shape = Part.Face(shape)
                else:
                    shape = Part.Face(shape)
            obj.Shape = shape
            obj.Placement = plm
        obj.positionBySupport()

class _ViewProviderRectangle(_ViewProviderDraft):
    def __init__(self,vobj):
        _ViewProviderDraft.__init__(self,vobj)
        vobj.addProperty("App::PropertyFile","TextureImage","Draft",QT_TRANSLATE_NOOP("App::Property","Defines a texture image (overrides hatch patterns)"))

class _Circle(_DraftObject):
    "The Circle object"

    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"Circle")
        obj.addProperty("App::PropertyAngle","FirstAngle","Draft",QT_TRANSLATE_NOOP("App::Property","Start angle of the arc"))
        obj.addProperty("App::PropertyAngle","LastAngle","Draft",QT_TRANSLATE_NOOP("App::Property","End angle of the arc (for a full circle, give it same value as First Angle)"))
        obj.addProperty("App::PropertyLength","Radius","Draft",QT_TRANSLATE_NOOP("App::Property","Radius of the circle"))
        obj.addProperty("App::PropertyBool","MakeFace","Draft",QT_TRANSLATE_NOOP("App::Property","Create a face"))
        obj.MakeFace = getParam("fillmode",True)

    def execute(self, obj):
        import Part
        plm = obj.Placement
        shape = Part.makeCircle(obj.Radius.Value,Vector(0,0,0),Vector(0,0,1),obj.FirstAngle.Value,obj.LastAngle.Value)
        if obj.FirstAngle.Value == obj.LastAngle.Value:
            shape = Part.Wire(shape)
            if hasattr(obj,"MakeFace"):
                if obj.MakeFace:
                    shape = Part.Face(shape)
            else:
                shape = Part.Face(shape)
        obj.Shape = shape
        obj.Placement = plm
        obj.positionBySupport()

class _Ellipse(_DraftObject):
    "The Circle object"

    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"Ellipse")
        obj.addProperty("App::PropertyAngle","FirstAngle","Draft",QT_TRANSLATE_NOOP("App::Property","Start angle of the arc"))
        obj.addProperty("App::PropertyAngle","LastAngle","Draft",QT_TRANSLATE_NOOP("App::Property","End angle of the arc (for a full circle, give it same value as First Angle)"))
        obj.addProperty("App::PropertyLength","MinorRadius","Draft",QT_TRANSLATE_NOOP("App::Property","The minor radius of the ellipse"))
        obj.addProperty("App::PropertyLength","MajorRadius","Draft",QT_TRANSLATE_NOOP("App::Property","The major radius of the ellipse"))
        obj.addProperty("App::PropertyBool","MakeFace","Draft",QT_TRANSLATE_NOOP("App::Property","Create a face"))
        obj.MakeFace = getParam("fillmode",True)

    def execute(self, obj):
        import Part
        plm = obj.Placement
        if obj.MajorRadius.Value < obj.MinorRadius.Value:
            msg(translate("Error: Major radius is smaller than the minor radius"))
            return
        if obj.MajorRadius.Value and obj.MinorRadius.Value:
            ell = Part.Ellipse(Vector(0,0,0),obj.MajorRadius.Value,obj.MinorRadius.Value)
            shape = ell.toShape()
            if hasattr(obj,"FirstAngle"):
                if obj.FirstAngle.Value != obj.LastAngle.Value:
                    a1 = obj.FirstAngle.getValueAs(FreeCAD.Units.Radian)
                    a2 = obj.LastAngle.getValueAs(FreeCAD.Units.Radian)
                    shape = Part.ArcOfEllipse(ell,a1,a2).toShape()
            shape = Part.Wire(shape)
            if shape.isClosed():
                if hasattr(obj,"MakeFace"):
                    if obj.MakeFace:
                        shape = Part.Face(shape)
                else:
                    shape = Part.Face(shape)
            obj.Shape = shape
            obj.Placement = plm
        obj.positionBySupport()

class _Wire(_DraftObject):
    "The Wire object"

    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"Wire")
        obj.addProperty("App::PropertyVectorList","Points","Draft",QT_TRANSLATE_NOOP("App::Property","The vertices of the wire"))
        obj.addProperty("App::PropertyBool","Closed","Draft",QT_TRANSLATE_NOOP("App::Property","If the wire is closed or not"))
        obj.addProperty("App::PropertyLink","Base","Draft",QT_TRANSLATE_NOOP("App::Property","The base object is the wire, it's formed from 2 objects"))
        obj.addProperty("App::PropertyLink","Tool","Draft",QT_TRANSLATE_NOOP("App::Property","The tool object is the wire, it's formed from 2 objects"))
        obj.addProperty("App::PropertyVectorDistance","Start","Draft",QT_TRANSLATE_NOOP("App::Property","The start point of this line"))
        obj.addProperty("App::PropertyVectorDistance","End","Draft",QT_TRANSLATE_NOOP("App::Property","The end point of this line"))
        obj.addProperty("App::PropertyLength","Length","Draft",QT_TRANSLATE_NOOP("App::Property","The length of this line"))
        obj.addProperty("App::PropertyLength","FilletRadius","Draft",QT_TRANSLATE_NOOP("App::Property","Radius to use to fillet the corners"))
        obj.addProperty("App::PropertyLength","ChamferSize","Draft",QT_TRANSLATE_NOOP("App::Property","Size of the chamfer to give to the corners"))
        obj.addProperty("App::PropertyBool","MakeFace","Draft",QT_TRANSLATE_NOOP("App::Property","Create a face if this object is closed"))
        obj.addProperty("App::PropertyInteger","Subdivisions","Draft",QT_TRANSLATE_NOOP("App::Property","The number of subdivisions of each edge"))
        obj.MakeFace = getParam("fillmode",True)
        obj.Closed = False

    def execute(self, obj):
        import Part, DraftGeomUtils
        plm = obj.Placement
        if obj.Base and (not obj.Tool):
            if obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                shape = obj.Base.Shape.copy()
                if obj.Base.Shape.isClosed():
                    if hasattr(obj,"MakeFace"):
                        if obj.MakeFace:
                            shape = Part.Face(shape)
                    else:
                        shape = Part.Face(shape)
                obj.Shape = shape
        elif obj.Base and obj.Tool:
            if obj.Base.isDerivedFrom("Part::Feature") and obj.Tool.isDerivedFrom("Part::Feature"):
                if (not obj.Base.Shape.isNull()) and (not obj.Tool.Shape.isNull()):
                    sh1 = obj.Base.Shape.copy()
                    sh2 = obj.Tool.Shape.copy()
                    shape = sh1.fuse(sh2)
                    if DraftGeomUtils.isCoplanar(shape.Faces):
                        shape = DraftGeomUtils.concatenate(shape)
                        obj.Shape = shape
                        p = []
                        for v in shape.Vertexes: p.append(v.Point)
                        if obj.Points != p: obj.Points = p
        elif obj.Points:
            if obj.Points[0] == obj.Points[-1]:
                if not obj.Closed: obj.Closed = True
                obj.Points.pop()
            if obj.Closed and (len(obj.Points) > 2):
                pts = obj.Points
                if hasattr(obj,"Subdivisions"):
                    if obj.Subdivisions > 0:
                        npts = []
                        for i in range(len(pts)):
                            p1 = pts[i]
                            npts.append(pts[i])
                            if i == len(pts)-1:
                                p2 = pts[0]
                            else:
                                p2 = pts[i+1]
                            v = p2.sub(p1)
                            v = DraftVecUtils.scaleTo(v,v.Length/(obj.Subdivisions+1))
                            for j in range(obj.Subdivisions):
                                npts.append(p1.add(FreeCAD.Vector(v).multiply(j+1)))
                        pts = npts
                shape = Part.makePolygon(pts+[pts[0]])
                if "ChamferSize" in obj.PropertiesList:
                    if obj.ChamferSize.Value != 0:
                        w = DraftGeomUtils.filletWire(shape,obj.ChamferSize.Value,chamfer=True)
                        if w:
                            shape = w
                if "FilletRadius" in obj.PropertiesList:
                    if obj.FilletRadius.Value != 0:
                        w = DraftGeomUtils.filletWire(shape,obj.FilletRadius.Value)
                        if w:
                            shape = w
                try:
                    if hasattr(obj,"MakeFace"):
                        if obj.MakeFace:
                            shape = Part.Face(shape)
                    else:
                        shape = Part.Face(shape)
                except Part.OCCError:
                    pass
            else:
                edges = []
                pts = obj.Points[1:]
                lp = obj.Points[0]
                for p in pts:
                    if not DraftVecUtils.equals(lp,p):
                        if hasattr(obj,"Subdivisions"):
                            if obj.Subdivisions > 0:
                                npts = []
                                v = p.sub(lp)
                                v = DraftVecUtils.scaleTo(v,v.Length/(obj.Subdivisions+1))
                                edges.append(Part.LineSegment(lp,lp.add(v)).toShape())
                                lv = lp.add(v)
                                for j in range(obj.Subdivisions):
                                    edges.append(Part.LineSegment(lv,lv.add(v)).toShape())
                                    lv = lv.add(v)
                            else:
                                edges.append(Part.LineSegment(lp,p).toShape())
                        else:
                            edges.append(Part.LineSegment(lp,p).toShape())
                        lp = p
                try:
                    shape = Part.Wire(edges)
                except Part.OCCError:
                    print("Error wiring edges")
                    shape = None
                if "ChamferSize" in obj.PropertiesList:
                    if obj.ChamferSize.Value != 0:
                        w = DraftGeomUtils.filletWire(shape,obj.ChamferSize.Value,chamfer=True)
                        if w:
                            shape = w
                if "FilletRadius" in obj.PropertiesList:
                    if obj.FilletRadius.Value != 0:
                        w = DraftGeomUtils.filletWire(shape,obj.FilletRadius.Value)
                        if w:
                            shape = w
            if shape:
                obj.Shape = shape
                if hasattr(obj,"Length"):
                    obj.Length = shape.Length
        obj.Placement = plm
        obj.positionBySupport()
        self.onChanged(obj,"Placement")

    def onChanged(self, obj, prop):
        if prop == "Start":
            pts = obj.Points
            invpl = FreeCAD.Placement(obj.Placement).inverse()
            realfpstart = invpl.multVec(obj.Start)
            if pts:
                if pts[0] != realfpstart:
                    pts[0] = realfpstart
                    obj.Points = pts
        elif prop == "End":
            pts = obj.Points
            invpl = FreeCAD.Placement(obj.Placement).inverse()
            realfpend = invpl.multVec(obj.End)
            if len(pts) > 1:
                if pts[-1] != realfpend:
                    pts[-1] = realfpend
                    obj.Points = pts
        elif prop == "Length":
            if obj.Shape and not obj.Shape.isNull():
                if obj.Length.Value != obj.Shape.Length:
                    if len(obj.Points) == 2:
                        v = obj.Points[-1].sub(obj.Points[0])
                        v = DraftVecUtils.scaleTo(v,obj.Length.Value)
                        obj.Points = [obj.Points[0],obj.Points[0].add(v)]

        elif prop == "Placement":
            pl = FreeCAD.Placement(obj.Placement)
            if len(obj.Points) >= 2:
                displayfpstart = pl.multVec(obj.Points[0])
                displayfpend = pl.multVec(obj.Points[-1])
                if obj.Start != displayfpstart:
                    obj.Start = displayfpstart
                if obj.End != displayfpend:
                    obj.End = displayfpend


class _ViewProviderWire(_ViewProviderDraft):
    "A View Provider for the Wire object"
    def __init__(self, obj):
        _ViewProviderDraft.__init__(self,obj)
        obj.addProperty("App::PropertyBool","EndArrow","Draft",QT_TRANSLATE_NOOP("App::Property","Displays a dim symbol at the end of the wire"))
        obj.addProperty("App::PropertyLength","ArrowSize","Draft",QT_TRANSLATE_NOOP("App::Property","Arrow size"))
        obj.addProperty("App::PropertyEnumeration","ArrowType","Draft",QT_TRANSLATE_NOOP("App::Property","Arrow type"))
        obj.ArrowSize = getParam("arrowsize",0.1)
        obj.ArrowType = arrowtypes
        obj.ArrowType = arrowtypes[getParam("dimsymbol",0)]

    def attach(self, obj):
        from pivy import coin
        self.Object = obj.Object
        col = coin.SoBaseColor()
        col.rgb.setValue(obj.LineColor[0],obj.LineColor[1],obj.LineColor[2])
        self.coords = coin.SoTransform()
        self.pt = coin.SoSeparator()
        self.pt.addChild(col)
        self.pt.addChild(self.coords)
        self.symbol = dimSymbol()
        self.pt.addChild(self.symbol)
        _ViewProviderDraft.attach(self,obj)
        self.onChanged(obj,"EndArrow")

    def updateData(self, obj, prop):
        from pivy import coin
        if prop == "Points":
            if obj.Points:
                p = obj.Points[-1]
                if hasattr(self,"coords"):
                    self.coords.translation.setValue((p.x,p.y,p.z))
                    if len(obj.Points) >= 2:
                        v1 = obj.Points[-2].sub(obj.Points[-1])
                        if not DraftVecUtils.isNull(v1):
                            v1.normalize()
                            _rot = coin.SbRotation()
                            _rot.setValue(coin.SbVec3f(1, 0, 0), coin.SbVec3f(v1[0], v1[1], v1[2]))
                            self.coords.rotation.setValue(_rot)
        return

    def onChanged(self, vobj, prop):
        from pivy import coin
        if prop in ["EndArrow","ArrowSize","ArrowType","Visibility"]:
            rn = vobj.RootNode
            if hasattr(self,"pt") and hasattr(vobj,"EndArrow"):
                if vobj.EndArrow and vobj.Visibility:
                    self.pt.removeChild(self.symbol)
                    s = arrowtypes.index(vobj.ArrowType)
                    self.symbol = dimSymbol(s)
                    self.pt.addChild(self.symbol)
                    self.updateData(vobj.Object,"Points")
                    if hasattr(vobj,"ArrowSize"):
                        s = vobj.ArrowSize
                    else:
                        s = getParam("arrowsize",0.1)
                    self.coords.scaleFactor.setValue((s,s,s))
                    rn.addChild(self.pt)
                else:
                    if self.symbol:
                        if self.pt.findChild(self.symbol) != -1:
                            self.pt.removeChild(self.symbol)
                        if rn.findChild(self.pt) != -1:
                            rn.removeChild(self.pt)
        if prop in ["LineColor"]:
            if hasattr(self, "pt"):
                self.pt[0].rgb.setValue(vobj.LineColor[0],vobj.LineColor[1],vobj.LineColor[2])
        _ViewProviderDraft.onChanged(self,vobj,prop)
        return

    def claimChildren(self):
        if hasattr(self.Object,"Base"):
            return [self.Object.Base,self.Object.Tool]
        return []

    def setupContextMenu(self,vobj,menu):
        from PySide import QtCore,QtGui
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_Edit.svg"),"Flatten this wire",menu)
        QtCore.QObject.connect(action1,QtCore.SIGNAL("triggered()"),self.flatten)
        menu.addAction(action1)

    def flatten(self):
        if hasattr(self,"Object"):
            if len(self.Object.Shape.Wires) == 1:
                import DraftGeomUtils
                fw = DraftGeomUtils.flattenWire(self.Object.Shape.Wires[0])
                points = [v.Point for v in fw.Vertexes]
                if len(points) == len(self.Object.Points):
                    if points != self.Object.Points:
                        FreeCAD.ActiveDocument.openTransaction("Flatten wire")
                        FreeCADGui.doCommand("FreeCAD.ActiveDocument."+self.Object.Name+".Points="+str(points).replace("Vector","FreeCAD.Vector").replace(" ",""))
                        FreeCAD.ActiveDocument.commitTransaction()
                        FreeCAD.ActiveDocument.recompute()
                    else:
                        from DraftTools import translate
                        FreeCAD.Console.PrintMessage(translate("Draft","This Wire is already flat")+"\n")

class _Polygon(_DraftObject):
    "The Polygon object"

    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"Polygon")
        obj.addProperty("App::PropertyInteger","FacesNumber","Draft",QT_TRANSLATE_NOOP("App::Property","Number of faces"))
        obj.addProperty("App::PropertyLength","Radius","Draft",QT_TRANSLATE_NOOP("App::Property","Radius of the control circle"))
        obj.addProperty("App::PropertyEnumeration","DrawMode","Draft",QT_TRANSLATE_NOOP("App::Property","How the polygon must be drawn from the control circle"))
        obj.addProperty("App::PropertyLength","FilletRadius","Draft",QT_TRANSLATE_NOOP("App::Property","Radius to use to fillet the corners"))
        obj.addProperty("App::PropertyLength","ChamferSize","Draft",QT_TRANSLATE_NOOP("App::Property","Size of the chamfer to give to the corners"))
        obj.addProperty("App::PropertyBool","MakeFace","Draft",QT_TRANSLATE_NOOP("App::Property","Create a face"))
        obj.MakeFace = getParam("fillmode",True)
        obj.DrawMode = ['inscribed','circumscribed']
        obj.FacesNumber = 0
        obj.Radius = 1

    def execute(self, obj):
        if (obj.FacesNumber >= 3) and (obj.Radius.Value > 0):
            import Part, DraftGeomUtils
            plm = obj.Placement
            angle = (math.pi*2)/obj.FacesNumber
            if obj.DrawMode == 'inscribed':
                delta = obj.Radius.Value
            else:
                delta = obj.Radius.Value/math.cos(angle/2.0)
            pts = [Vector(delta,0,0)]
            for i in range(obj.FacesNumber-1):
                ang = (i+1)*angle
                pts.append(Vector(delta*math.cos(ang),delta*math.sin(ang),0))
            pts.append(pts[0])
            shape = Part.makePolygon(pts)
            if "ChamferSize" in obj.PropertiesList:
                if obj.ChamferSize.Value != 0:
                    w = DraftGeomUtils.filletWire(shape,obj.ChamferSize.Value,chamfer=True)
                    if w:
                        shape = w
            if "FilletRadius" in obj.PropertiesList:
                if obj.FilletRadius.Value != 0:
                    w = DraftGeomUtils.filletWire(shape,obj.FilletRadius.Value)
                    if w:
                        shape = w
            if hasattr(obj,"MakeFace"):
                if obj.MakeFace:
                    shape = Part.Face(shape)
            else:
                shape = Part.Face(shape)
            obj.Shape = shape
            obj.Placement = plm
        obj.positionBySupport()


class _DrawingView(_DraftObject):
    "The Draft DrawingView object"
    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"DrawingView")
        obj.addProperty("App::PropertyVector","Direction","Shape View",QT_TRANSLATE_NOOP("App::Property","Projection direction"))
        obj.addProperty("App::PropertyFloat","LineWidth","View Style",QT_TRANSLATE_NOOP("App::Property","The width of the lines inside this object"))
        obj.addProperty("App::PropertyLength","FontSize","View Style",QT_TRANSLATE_NOOP("App::Property","The size of the texts inside this object"))
        obj.addProperty("App::PropertyLength","LineSpacing","View Style",QT_TRANSLATE_NOOP("App::Property","The spacing between lines of text"))
        obj.addProperty("App::PropertyColor","LineColor","View Style",QT_TRANSLATE_NOOP("App::Property","The color of the projected objects"))
        obj.addProperty("App::PropertyLink","Source","Base",QT_TRANSLATE_NOOP("App::Property","The linked object"))
        obj.addProperty("App::PropertyEnumeration","FillStyle","View Style",QT_TRANSLATE_NOOP("App::Property","Shape Fill Style"))
        obj.addProperty("App::PropertyEnumeration","LineStyle","View Style",QT_TRANSLATE_NOOP("App::Property","Line Style"))
        obj.addProperty("App::PropertyBool","AlwaysOn","View Style",QT_TRANSLATE_NOOP("App::Property","If checked, source objects are displayed regardless of being visible in the 3D model"))
        obj.FillStyle = ['shape color'] + list(svgpatterns().keys())
        obj.LineStyle = ['Solid','Dashed','Dotted','Dashdot']
        obj.LineWidth = 0.35
        obj.FontSize = 12

    def execute(self, obj):
        result = ""
        if hasattr(obj,"Source"):
            if obj.Source:
                if hasattr(obj,"LineStyle"):
                    ls = obj.LineStyle
                else:
                    ls = None
                if hasattr(obj,"LineColor"):
                    lc = obj.LineColor
                else:
                    lc = None
                if hasattr(obj,"LineSpacing"):
                    lp = obj.LineSpacing
                else:
                    lp = None
                if obj.Source.isDerivedFrom("App::DocumentObjectGroup"):
                    svg = ""
                    shapes = []
                    others = []
                    objs = getGroupContents([obj.Source])
                    for o in objs:
                        v = o.ViewObject.isVisible()
                        if hasattr(obj,"AlwaysOn"):
                            if obj.AlwaysOn:
                                v = True
                        if v:
                            svg += getSVG(o,obj.Scale,obj.LineWidth,obj.FontSize.Value,obj.FillStyle,obj.Direction,ls,lc,lp)
                else:
                    svg = getSVG(obj.Source,obj.Scale,obj.LineWidth,obj.FontSize.Value,obj.FillStyle,obj.Direction,ls,lc,lp)
                result += '<g id="' + obj.Name + '"'
                result += ' transform="'
                result += 'rotate('+str(obj.Rotation)+','+str(obj.X)+','+str(obj.Y)+') '
                result += 'translate('+str(obj.X)+','+str(obj.Y)+') '
                result += 'scale('+str(obj.Scale)+','+str(-obj.Scale)+')'
                result += '">'
                result += svg
                result += '</g>'
        obj.ViewResult = result

    def getDXF(self,obj):
        "returns a DXF fragment"
        return getDXF(obj)

class _BSpline(_DraftObject):
    "The BSpline object"

    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"BSpline")
        obj.addProperty("App::PropertyVectorList","Points","Draft", QT_TRANSLATE_NOOP("App::Property","The points of the B-spline"))
        obj.addProperty("App::PropertyBool","Closed","Draft",QT_TRANSLATE_NOOP("App::Property","If the B-spline is closed or not"))
        obj.addProperty("App::PropertyBool","MakeFace","Draft",QT_TRANSLATE_NOOP("App::Property","Create a face if this spline is closed"))
        obj.MakeFace = getParam("fillmode",True)
        obj.Closed = False
        obj.Points = []
        self.assureProperties(obj)

    def assureProperties(self, obj): # for Compatibility with older versions
        if not hasattr(obj, "Parameterization"):
            obj.addProperty("App::PropertyFloat","Parameterization","Draft",QT_TRANSLATE_NOOP("App::Property","Parameterization factor"))
            obj.Parameterization = 1.0
            self.knotSeq = []

    def parameterization (self, pts, a, closed):
        # Computes a knot Sequence for a set of points
        # fac (0-1) : parameterization factor
        # fac=0 -> Uniform / fac=0.5 -> Centripetal / fac=1.0 -> Chord-Length
        if closed: # we need to add the first point as the end point
            pts.append(pts[0])
        params = [0]
        for i in range(1,len(pts)):
            p = pts[i].sub(pts[i-1])
            pl = pow(p.Length,a)
            params.append(params[-1] + pl)
        return params

    def onChanged(self, fp, prop):
        if prop == "Parameterization":
            if fp.Parameterization < 0.:
                fp.Parameterization = 0.
            if fp.Parameterization > 1.0:
                fp.Parameterization = 1.0

    def execute(self, obj):
        import Part
        from DraftTools import msg
        self.assureProperties(obj)
        if obj.Points:
            self.knotSeq = self.parameterization(obj.Points, obj.Parameterization, obj.Closed)
            plm = obj.Placement
            if obj.Closed and (len(obj.Points) > 2):
                if obj.Points[0] == obj.Points[-1]:  # should not occur, but OCC will crash
                    msg(translate('draft',  "_BSpline.createGeometry: Closed with same first/last Point. Geometry not updated.")+"\n", "error")
                    return
                spline = Part.BSplineCurve()
                spline.interpolate(obj.Points, PeriodicFlag = True, Parameters = self.knotSeq)
                # DNC: bug fix: convert to face if closed
                shape = Part.Wire(spline.toShape())
                # Creating a face from a closed spline cannot be expected to always work
                # Usually, if the spline is not flat the call of Part.Face() fails
                try:
                    if hasattr(obj,"MakeFace"):
                        if obj.MakeFace:
                            shape = Part.Face(shape)
                    else:
                        shape = Part.Face(shape)
                except Part.OCCError:
                    pass
                obj.Shape = shape
            else:
                spline = Part.BSplineCurve()
                spline.interpolate(obj.Points, PeriodicFlag = False, Parameters = self.knotSeq)
                obj.Shape = spline.toShape()
            obj.Placement = plm
        obj.positionBySupport()

# for compatibility with older versions
_ViewProviderBSpline = _ViewProviderWire

class _BezCurve(_DraftObject):
    "The BezCurve object"

    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"BezCurve")
        obj.addProperty("App::PropertyVectorList","Points","Draft",QT_TRANSLATE_NOOP("App::Property","The points of the Bezier curve"))
        obj.addProperty("App::PropertyInteger","Degree","Draft",QT_TRANSLATE_NOOP("App::Property","The degree of the Bezier function"))
        obj.addProperty("App::PropertyIntegerList","Continuity","Draft",QT_TRANSLATE_NOOP("App::Property","Continuity"))
        obj.addProperty("App::PropertyBool","Closed","Draft",QT_TRANSLATE_NOOP("App::Property","If the Bezier curve should be closed or not"))
        obj.addProperty("App::PropertyBool","MakeFace","Draft",QT_TRANSLATE_NOOP("App::Property","Create a face if this curve is closed"))
        obj.MakeFace = getParam("fillmode",True)
        obj.Closed = False
        obj.Degree = 3
        obj.Continuity = []
        #obj.setEditorMode("Degree",2)#hide
        obj.setEditorMode("Continuity",1)#ro

    def execute(self, fp):
        self.createGeometry(fp)
        fp.positionBySupport()

    def _segpoleslst(self,fp):
        """split the points into segments"""
        if not fp.Closed and len(fp.Points) >= 2: #allow lower degree segment
            poles=fp.Points[1:]
        elif fp.Closed and len(fp.Points) >= fp.Degree: #drawable
            #poles=fp.Points[1:(fp.Degree*(len(fp.Points)//fp.Degree))]+fp.Points[0:1]
            poles=fp.Points[1:]+fp.Points[0:1]
        else:
            poles=[]
        return [poles[x:x+fp.Degree] for x in \
            range(0, len(poles), (fp.Degree or 1))]

    def resetcontinuity(self,fp):
        fp.Continuity = [0]*(len(self._segpoleslst(fp))-1+1*fp.Closed)
        #nump= len(fp.Points)-1+fp.Closed*1
        #numsegments = (nump // fp.Degree) + 1 * (nump % fp.Degree > 0) -1
        #fp.Continuity = [0]*numsegments

    def onChanged(self, fp, prop):
        if prop == 'Closed': # if remove the last entry when curve gets opened
            oldlen = len(fp.Continuity)
            newlen = (len(self._segpoleslst(fp))-1+1*fp.Closed)
            if oldlen > newlen:
                fp.Continuity = fp.Continuity[:newlen]
            if oldlen < newlen:
                fp.Continuity = fp.Continuity + [0]*(newlen-oldlen)
        if hasattr(fp,'Closed') and fp.Closed and prop in  ['Points','Degree','Closed'] and\
                len(fp.Points) % fp.Degree: # the curve editing tools can't handle extra points
            fp.Points=fp.Points[:(fp.Degree*(len(fp.Points)//fp.Degree))] #for closed curves
        if prop in ["Degree"] and fp.Degree >= 1: #reset Continuity
            self.resetcontinuity(fp)
        if prop in ["Points","Degree","Continuity","Closed"]:
            self.createGeometry(fp)

    def createGeometry(self,fp):
        import Part
        plm = fp.Placement
        if fp.Points:
            startpoint=fp.Points[0]
            edges = []
            for segpoles in self._segpoleslst(fp):
#                if len(segpoles) == fp.Degree # would skip additional poles
                 c = Part.BezierCurve() #last segment may have lower degree
                 c.increase(len(segpoles))
                 c.setPoles([startpoint]+segpoles)
                 edges.append(Part.Edge(c))
                 startpoint = segpoles[-1]
            w = Part.Wire(edges)
            if fp.Closed and w.isClosed():
                try:
                    if hasattr(fp,"MakeFace"):
                        if fp.MakeFace:
                            w = Part.Face(w)
                    else:
                        w = Part.Face(w)
                except Part.OCCError:
                    pass
            fp.Shape = w
        fp.Placement = plm

    @classmethod
    def symmetricpoles(cls,knot, p1, p2):
        """make two poles symmetric respective to the knot"""
        p1h=FreeCAD.Vector(p1)
        p2h=FreeCAD.Vector(p2)
        p1h.multiply(0.5)
        p2h.multiply(0.5)
        return ( knot+p1h-p2h , knot+p2h-p1h)

    @classmethod
    def tangentpoles(cls,knot, p1, p2,allowsameside=False):
        """make two poles have the same tangent at knot"""
        p12n=p2.sub(p1)
        p12n.normalize()
        p1k=knot-p1
        p2k=knot-p2
        p1k_= FreeCAD.Vector(p12n)
        kon12=(p1k*p12n)
        if allowsameside or not (kon12 < 0 or p2k*p12n > 0):# instead of moving
            p1k_.multiply(kon12)
            pk_k=knot-p1-p1k_
            return (p1+pk_k,p2+pk_k)
        else:
            return cls.symmetricpoles(knot, p1, p2)

    @staticmethod
    def modifysymmetricpole(knot,p1):
        """calculate the coordinates of the opposite pole
        of a symmetric knot"""
        return knot+knot-p1

    @staticmethod
    def modifytangentpole(knot,p1,oldp2):
        """calculate the coordinates of the opposite pole
        of a tangent knot"""
        pn=knot-p1
        pn.normalize()
        pn.multiply((knot-oldp2).Length)
        return pn+knot

# for compatibility with older versions ???????
_ViewProviderBezCurve = _ViewProviderWire

class _Block(_DraftObject):
    "The Block object"

    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"Block")
        obj.addProperty("App::PropertyLinkList","Components","Draft",QT_TRANSLATE_NOOP("App::Property","The components of this block"))

    def execute(self, obj):
        import Part
        plm = obj.Placement
        shps = []
        for c in obj.Components:
            shps.append(c.Shape)
        if shps:
            shape = Part.makeCompound(shps)
            obj.Shape = shape
        obj.Placement = plm
        obj.positionBySupport()

class _Shape2DView(_DraftObject):
    "The Shape2DView object"

    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Draft",QT_TRANSLATE_NOOP("App::Property","The base object this 2D view must represent"))
        obj.addProperty("App::PropertyVector","Projection","Draft",QT_TRANSLATE_NOOP("App::Property","The projection vector of this object"))
        obj.addProperty("App::PropertyEnumeration","ProjectionMode","Draft",QT_TRANSLATE_NOOP("App::Property","The way the viewed object must be projected"))
        obj.addProperty("App::PropertyIntegerList","FaceNumbers","Draft",QT_TRANSLATE_NOOP("App::Property","The indices of the faces to be projected in Individual Faces mode"))
        obj.addProperty("App::PropertyBool","HiddenLines","Draft",QT_TRANSLATE_NOOP("App::Property","Show hidden lines"))
        obj.addProperty("App::PropertyBool","Tessellation","Draft",QT_TRANSLATE_NOOP("App::Property","Tessellate Ellipses and B-splines into line segments"))
        obj.addProperty("App::PropertyBool","InPlace","Draft",QT_TRANSLATE_NOOP("App::Property","For Cutlines and Cutfaces modes, this leaves the faces at the cut location"))
        obj.addProperty("App::PropertyFloat","SegmentLength","Draft",QT_TRANSLATE_NOOP("App::Property","Length of line segments if tessellating Ellipses or B-splines into line segments"))
        obj.addProperty("App::PropertyBool","VisibleOnly","Draft",QT_TRANSLATE_NOOP("App::Property","If this is True, this object will be recomputed only if it is visible"))
        obj.Projection = Vector(0,0,1)
        obj.ProjectionMode = ["Solid","Individual Faces","Cutlines","Cutfaces"]
        obj.HiddenLines = False
        obj.Tessellation = False
        obj.VisibleOnly = False
        obj.InPlace = True
        obj.SegmentLength = .05
        _DraftObject.__init__(self,obj,"Shape2DView")

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
        if hasattr(obj,"Tessellation") and obj.Tessellation:
            return DraftGeomUtils.cleanProjection(Part.makeCompound(edges),obj.Tessellation,obj.SegmentLength)
        else:
            return Part.makeCompound(edges)
            #return DraftGeomUtils.cleanProjection(Part.makeCompound(edges))

    def execute(self,obj):
        if hasattr(obj,"VisibleOnly"):
            if obj.VisibleOnly:
                if obj.ViewObject:
                    if obj.ViewObject.Visibility == False:
                        return False
        import DraftGeomUtils
        obj.positionBySupport()
        pl = obj.Placement
        if obj.Base:
            if getType(obj.Base) == "SectionPlane":
                if obj.Base.Objects:
                    onlysolids = True
                    if hasattr(obj.Base,"OnlySolids"):
                        onlysolids = obj.Base.OnlySolids
                    import Arch, Part, Drawing
                    objs = getGroupContents(obj.Base.Objects,walls=True)
                    objs = removeHidden(objs)
                    shapes = []
                    for o in objs:
                        if o.isDerivedFrom("Part::Feature"):
                            if onlysolids:
                                shapes.extend(o.Shape.Solids)
                            else:
                                shapes.append(o.Shape.copy())
                    cutp,cutv,iv =Arch.getCutVolume(obj.Base.Shape,shapes)
                    cuts = []
                    opl = FreeCAD.Placement(obj.Base.Placement)
                    proj = opl.Rotation.multVec(FreeCAD.Vector(0,0,1))
                    if obj.ProjectionMode == "Solid":
                        for sh in shapes:
                            if cutv:
                                if sh.Volume < 0:
                                    sh.reverse()
                                #if cutv.BoundBox.intersect(sh.BoundBox):
                                #    c = sh.cut(cutv)
                                #else:
                                #    c = sh.copy()
                                c = sh.cut(cutv)
                                if onlysolids:
                                    cuts.extend(c.Solids)
                                else:
                                    cuts.append(c)
                            else:
                                if onlysolids:
                                    cuts.extend(sh.Solids)
                                else:
                                    cuts.append(sh.copy())
                        comp = Part.makeCompound(cuts)
                        obj.Shape = self.getProjected(obj,comp,proj)
                    elif obj.ProjectionMode in ["Cutlines","Cutfaces"]:
                        for sh in shapes:
                            if sh.Volume < 0:
                                sh.reverse()
                            c = sh.section(cutp)
                            faces = []
                            if (obj.ProjectionMode == "Cutfaces") and (sh.ShapeType == "Solid"):
                                if hasattr(obj,"InPlace"):
                                    if not obj.InPlace:
                                        c = self.getProjected(obj,c,proj)
                                wires = DraftGeomUtils.findWires(c.Edges)
                                for w in wires:
                                    if w.isClosed():
                                        faces.append(Part.Face(w))
                            if faces:
                                cuts.extend(faces)
                            else:
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
                    else:
                        FreeCAD.Console.PrintWarning(obj.ProjectionMode+" mode not implemented\n")
        if not DraftGeomUtils.isNull(pl):
            obj.Placement = pl

class _Array(_DraftObject):
    "The Draft Array object"

    def __init__(self,obj):
        _DraftObject.__init__(self,obj,"Array")
        obj.addProperty("App::PropertyLink","Base","Draft",QT_TRANSLATE_NOOP("App::Property","The base object that must be duplicated"))
        obj.addProperty("App::PropertyEnumeration","ArrayType","Draft",QT_TRANSLATE_NOOP("App::Property","The type of array to create"))
        obj.addProperty("App::PropertyVector","Axis","Draft",QT_TRANSLATE_NOOP("App::Property","The axis direction"))
        obj.addProperty("App::PropertyInteger","NumberX","Draft",QT_TRANSLATE_NOOP("App::Property","Number of copies in X direction"))
        obj.addProperty("App::PropertyInteger","NumberY","Draft",QT_TRANSLATE_NOOP("App::Property","Number of copies in Y direction"))
        obj.addProperty("App::PropertyInteger","NumberZ","Draft",QT_TRANSLATE_NOOP("App::Property","Number of copies in Z direction"))
        obj.addProperty("App::PropertyInteger","NumberPolar","Draft",QT_TRANSLATE_NOOP("App::Property","Number of copies"))
        obj.addProperty("App::PropertyVectorDistance","IntervalX","Draft",QT_TRANSLATE_NOOP("App::Property","Distance and orientation of intervals in X direction"))
        obj.addProperty("App::PropertyVectorDistance","IntervalY","Draft",QT_TRANSLATE_NOOP("App::Property","Distance and orientation of intervals in Y direction"))
        obj.addProperty("App::PropertyVectorDistance","IntervalZ","Draft",QT_TRANSLATE_NOOP("App::Property","Distance and orientation of intervals in Z direction"))
        obj.addProperty("App::PropertyVectorDistance","IntervalAxis","Draft",QT_TRANSLATE_NOOP("App::Property","Distance and orientation of intervals in Axis direction"))
        obj.addProperty("App::PropertyVectorDistance","Center","Draft",QT_TRANSLATE_NOOP("App::Property","Center point"))
        obj.addProperty("App::PropertyAngle","Angle","Draft",QT_TRANSLATE_NOOP("App::Property","Angle to cover with copies"))
        obj.addProperty("App::PropertyBool","Fuse","Draft",QT_TRANSLATE_NOOP("App::Property","Specifies if copies must be fused (slower)"))
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
        obj.Fuse = False

    def execute(self,obj):
        import DraftGeomUtils
        if hasattr(obj,"Fuse"):
            fuse = obj.Fuse
        else:
            fuse = False
        if obj.Base:
            pl = obj.Placement
            if obj.ArrayType == "ortho":
                sh = self.rectArray(obj.Base.Shape,obj.IntervalX,obj.IntervalY,
                                    obj.IntervalZ,obj.NumberX,obj.NumberY,obj.NumberZ,fuse)
            else:
                av = obj.IntervalAxis if hasattr(obj,"IntervalAxis") else None
                sh = self.polarArray(obj.Base.Shape,obj.Center,obj.Angle.Value,obj.NumberPolar,obj.Axis,av,fuse)
            obj.Shape = sh
            if not DraftGeomUtils.isNull(pl):
                obj.Placement = pl

    def rectArray(self,shape,xvector,yvector,zvector,xnum,ynum,znum,fuse=False):
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
        if fuse and len(base) > 1:
            return base[0].multiFuse(base[1:]).removeSplitter()
        else:
            return Part.makeCompound(base)

    def polarArray(self,shape,center,angle,num,axis,axisvector,fuse=False):
        #print("angle ",angle," num ",num)
        import Part
        if angle == 360:
            fraction = float(angle)/num
        else:
            if num == 0:
                return shape
            fraction = float(angle)/(num-1)
        base = [shape.copy()]
        for i in range(num-1):
            currangle = fraction + (i*fraction)
            nshape = shape.copy()
            nshape.rotate(DraftVecUtils.tup(center), DraftVecUtils.tup(axis), currangle)
            if axisvector:
                if not DraftVecUtils.isNull(axisvector):
                    nshape.translate(FreeCAD.Vector(axisvector).multiply(i+1))
            base.append(nshape)
        if fuse and len(base) > 1:
            return base[0].multiFuse(base[1:]).removeSplitter()
        else:
            return Part.makeCompound(base)


class _PathArray(_DraftObject):
    "The Draft Path Array object"

    def __init__(self,obj):
        _DraftObject.__init__(self,obj,"PathArray")
        obj.addProperty("App::PropertyLinkGlobal","Base","Draft",QT_TRANSLATE_NOOP("App::Property","The base object that must be duplicated"))
        obj.addProperty("App::PropertyLinkGlobal","PathObj","Draft",QT_TRANSLATE_NOOP("App::Property","The path object along which to distribute objects"))
        obj.addProperty("App::PropertyLinkSubListGlobal","PathSubs",QT_TRANSLATE_NOOP("App::Property","Selected subobjects (edges) of PathObj"))
        obj.addProperty("App::PropertyInteger","Count","Draft",QT_TRANSLATE_NOOP("App::Property","Number of copies"))
        obj.addProperty("App::PropertyVectorDistance","Xlate","Draft",QT_TRANSLATE_NOOP("App::Property","Optional translation vector"))
        obj.addProperty("App::PropertyBool","Align","Draft",QT_TRANSLATE_NOOP("App::Property","Orientation of Base along path"))
        obj.Count = 2
        obj.PathSubs = []
        obj.Xlate = FreeCAD.Vector(0,0,0)
        obj.Align = False

    def execute(self,obj):
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
            edgeNames = sub[1]
            for n in edgeNames:
                e = sub[0].Shape.getElement(n)
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

    def orientShape(self,shape,edge,offset,RefPt,xlate,align,normal=None):
        '''Orient shape to tangent at parm offset along edge.'''
        # http://en.wikipedia.org/wiki/Euler_angles
        import Part
        import DraftGeomUtils
        import math
        z = FreeCAD.Vector(0,0,1)                                    # unit +Z  Probably defined elsewhere?
        y = FreeCAD.Vector(0,1,0)                                    # unit +Y
        x = FreeCAD.Vector(1,0,0)                                    # unit +X
        nullv = FreeCAD.Vector(0,0,0)
        nullPlace =FreeCAD.Placement()
        ns = shape.copy()
        ns.Placement.Base = nullPlace.Base                           # reset Placement point so translate goes to right place.
        ns.Placement.Rotation = shape.Placement.Rotation             # preserve global orientation
        ns.translate(RefPt+xlate)
        if not align:
            return ns

        # get local coord system - tangent, normal, binormal, if possible
        t = edge.tangentAt(self.getParameterFromV0(edge,offset))
        t.normalize()
        try:
            if normal:
                n = normal
            else:
                n = edge.normalAt(self.getParameterFromV0(edge,offset))
                n.normalize()
            b = (t.cross(n))
            b.normalize()
        except FreeCAD.Base.FreeCADError:                                                      # no normal defined here
            n = nullv
            b = nullv
            FreeCAD.Console.PrintLog ("Draft PathArray.orientShape - Cannot calculate Path normal.\n")
        lnodes = z.cross(b)
        try:
            lnodes.normalize()                                       # Can't normalize null vector.
        except:
            pass                                                     # pathological cases:
        if n == nullv:                                               # 1) can't determine normal, don't align.
            psi = 0.0
            theta = 0.0
            phi = 0.0
            FreeCAD.Console.PrintWarning("Draft PathArray.orientShape - Path normal is Null. Cannot align.\n")
        elif abs(b.dot(z)) == 1.0:                                    # 2) binormal is || z
            psi = math.degrees(DraftVecUtils.angle(x,t,z))            #    align shape to tangent only
            theta = 0.0
            phi = 0.0
            FreeCAD.Console.PrintWarning("Draft PathArray.orientShape - Gimbal lock. Infinite lnodes. Change Path or Base.\n")
        else:                                                        # regular case
            psi = math.degrees(DraftVecUtils.angle(x,lnodes,z))
            theta = math.degrees(DraftVecUtils.angle(z,b,lnodes))
            phi = math.degrees(DraftVecUtils.angle(lnodes,t,b))
        if psi != 0.0:
            ns.rotate(RefPt,z,psi)
        if theta != 0.0:
            ns.rotate(RefPt,lnodes,theta)
        if phi != 0.0:
            ns.rotate(RefPt,b,phi)
        return ns

    def pathArray(self,shape,pathwire,count,xlate,align):
        '''Distribute shapes along a path.'''
        import Part
        import DraftGeomUtils
        closedpath = DraftGeomUtils.isReallyClosed(pathwire)
        normal = DraftGeomUtils.getNormal(pathwire)
        path = Part.__sortEdges__(pathwire.Edges)
        ends = []
        cdist = 0
        for e in path:                                                 # find cumulative edge end distance
            cdist += e.Length
            ends.append(cdist)
        base = []
        pt = path[0].Vertexes[0].Point                                 # place the start shape
        ns = self.orientShape(shape,path[0],0,pt,xlate,align,normal)
        base.append(ns)
        if not(closedpath):                                            # closed path doesn't need shape on last vertex
            pt = path[-1].Vertexes[-1].Point                           # place the end shape
            ns = self.orientShape(shape,path[-1],path[-1].Length,pt,xlate,align,normal)
            base.append(ns)
        if count < 3:
            return(Part.makeCompound(base))

        # place the middle shapes
        if closedpath:
            stop = count
        else:
            stop = count - 1
        step = float(cdist)/stop
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
            ns = self.orientShape(shape,path[iend],offset,pt,xlate,align,normal)
            base.append(ns)
            travel += step
        return(Part.makeCompound(base))

class _PointArray(_DraftObject):
    "The Draft Point Array object"
    def __init__(self, obj, bobj, ptlst):
        _DraftObject.__init__(self,obj,"PointArray")
        obj.addProperty("App::PropertyLink","Base","Draft",QT_TRANSLATE_NOOP("App::Property","Base")).Base = bobj
        obj.addProperty("App::PropertyLink","PointList","Draft",QT_TRANSLATE_NOOP("App::Property","PointList")).PointList = ptlst
        obj.addProperty("App::PropertyInteger","Count","Draft",QT_TRANSLATE_NOOP("App::Property","Count")).Count = 0
        obj.setEditorMode("Count", 1)

    def execute(self, obj):
        import Part
        from FreeCAD import Base, Vector
        pls = []
        opl = obj.PointList
        while getType(opl) == 'Clone':
            opl = opl.Objects[0]
        if hasattr(opl, 'Geometry'):
            place = opl.Placement
            for pts in opl.Geometry:
                if hasattr(pts, 'X') and hasattr(pts, 'Y') and hasattr(pts, 'Z'):
                    pn = pts.copy()
                    pn.translate(place.Base)
                    pn.rotate(place)
                    pls.append(pn)
        elif hasattr(opl, 'Links'):
            pls = opl.Links
        elif hasattr(opl, 'Components'):
            pls = opl.Components

        base = []
        i = 0
        if hasattr(obj.Base, 'Shape'):
            for pts in pls:
                #print pts # inspect the objects
                if hasattr(pts, 'X') and hasattr(pts, 'Y') and hasattr(pts, 'Z'):
        	        nshape = obj.Base.Shape.copy()
        	        if hasattr(pts, 'Placement'):
        	            place = pts.Placement
        	            nshape.translate(place.Base)
        	            nshape.rotate(place.Base, place.Rotation.Axis, place.Rotation.Angle * 180 /  math.pi )
        	        nshape.translate(Base.Vector(pts.X,pts.Y,pts.Z))
        	        i += 1
        	        base.append(nshape)
        obj.Count = i
        if i > 0:
            obj.Shape = Part.makeCompound(base)
        else:
            FreeCAD.Console.PrintError(translate("draft","No point found\n"))
            obj.Shape = obj.Base.Shape.copy()

class _Point(_DraftObject):
    "The Draft Point object"
    def __init__(self, obj,x=0,y=0,z=0):
        _DraftObject.__init__(self,obj,"Point")
        obj.addProperty("App::PropertyDistance","X","Draft",QT_TRANSLATE_NOOP("App::Property","X Location")).X = x
        obj.addProperty("App::PropertyDistance","Y","Draft",QT_TRANSLATE_NOOP("App::Property","Y Location")).Y = y
        obj.addProperty("App::PropertyDistance","Z","Draft",QT_TRANSLATE_NOOP("App::Property","Z Location")).Z = z
        mode = 2
        obj.setEditorMode('Placement',mode)

    def execute(self, obj):
        import Part
        shape = Part.Vertex(Vector(obj.X.Value,obj.Y.Value,obj.Z.Value))
        obj.Shape = shape

class _ViewProviderPoint(_ViewProviderDraft):
    "A viewprovider for the Draft Point object"
    def __init__(self, obj):
        _ViewProviderDraft.__init__(self,obj)

    def onChanged(self, vobj, prop):
        mode = 2
        vobj.setEditorMode('LineColor',mode)
        vobj.setEditorMode('LineWidth',mode)
        vobj.setEditorMode('BoundingBox',mode)
        vobj.setEditorMode('Deviation',mode)
        vobj.setEditorMode('DiffuseColor',mode)
        vobj.setEditorMode('DisplayMode',mode)
        vobj.setEditorMode('Lighting',mode)
        vobj.setEditorMode('LineMaterial',mode)
        vobj.setEditorMode('ShapeColor',mode)
        vobj.setEditorMode('ShapeMaterial',mode)
        vobj.setEditorMode('Transparency',mode)

    def getIcon(self):
        return ":/icons/Draft_Dot.svg"

class _Clone(_DraftObject):
    "The Clone object"

    def __init__(self,obj):
        _DraftObject.__init__(self,obj,"Clone")
        obj.addProperty("App::PropertyLinkListGlobal","Objects","Draft",QT_TRANSLATE_NOOP("App::Property","The objects included in this clone"))
        obj.addProperty("App::PropertyVector","Scale","Draft",QT_TRANSLATE_NOOP("App::Property","The scale factor of this clone"))
        obj.addProperty("App::PropertyBool","Fuse","Draft",QT_TRANSLATE_NOOP("App::Property","If this clones several objects, this specifies if the result is a fusion or a compound"))
        obj.Scale = Vector(1,1,1)

    def join(self,obj,shapes):
        if len(shapes) < 2:
            return shapes[0]
        import Part
        if hasattr(obj,"Fuse"):
            if obj.Fuse:
                try:
                    sh = shapes[0].multiFuse(shapes[1:])
                    sh = sh.removeSplitter()
                except:
                    pass
                else:
                    return sh
        return Part.makeCompound(shapes)

    def execute(self,obj):
        import Part, DraftGeomUtils
        pl = obj.Placement
        shapes = []
        if obj.isDerivedFrom("Part::Part2DObject"):
            # if our clone is 2D, make sure all its linked geometry is 2D too
            for o in obj.Objects:
                if not o.isDerivedFrom("Part::Part2DObject"):
                    FreeCAD.Console.PrintWarning("Warning 2D Clone "+obj.Name+" contains 3D geometry")
                    return
        objs = getGroupContents(obj.Objects)
        for o in objs:
            sh = None
            if o.isDerivedFrom("Part::Feature"):
                if not o.Shape.isNull():
                    sh = o.Shape.copy()
            elif o.hasExtension("App::GeoFeatureGroupExtension"):
                shps = []
                for so in o.Group:
                    if so.isDerivedFrom("Part::Feature"):
                        if not so.Shape.isNull():
                            shps.append(so.Shape)
                if shps:
                    sh = self.join(obj,shps)
            if sh:
                m = FreeCAD.Matrix()
                if hasattr(obj,"Scale") and not sh.isNull():
                    sx,sy,sz = obj.Scale
                    if not DraftVecUtils.equals(obj.Scale,Vector(1,1,1)):
                        op = sh.Placement
                        sh.Placement = FreeCAD.Placement()
                        m.scale(obj.Scale)
                        if sx == sy == sz:
                            sh.transformShape(m)
                        else:
                            sh = sh.transformGeometry(m)
                        sh.Placement = op
                if not sh.isNull():
                    shapes.append(sh)
        if shapes:
            if len(shapes) == 1:
                obj.Shape = shapes[0]
                obj.Placement = shapes[0].Placement
            else:
                obj.Shape = self.join(obj,shapes)
        obj.Placement = pl
        if hasattr(obj,"positionBySupport"):
            obj.positionBySupport()

    def getSubVolume(self,obj,placement=None):
        # this allows clones of arch windows to return a subvolume too
        if obj.Objects:
            if hasattr(obj.Objects[0],"Proxy"):
                if hasattr(obj.Objects[0].Proxy,"getSubVolume"):
                    if not placement:
                        # clones must displace the original subvolume too
                        placement = obj.Placement
                    return obj.Objects[0].Proxy.getSubVolume(obj.Objects[0],placement)
        return None

class _ViewProviderClone:
    "a view provider that displays a Clone icon instead of a Draft icon"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/Draft_Clone.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def getDisplayModes(self, vobj):
        modes=[]
        return modes

    def setDisplayMode(self, mode):
        return mode

    def resetColors(self, vobj):
        colors = []
        for o in getGroupContents(vobj.Object.Objects):
            if o.isDerivedFrom("Part::Feature"):
                if len(o.ViewObject.DiffuseColor) > 1:
                    colors.extend(o.ViewObject.DiffuseColor)
                else:
                    c = o.ViewObject.ShapeColor
                    c = (c[0],c[1],c[2],o.ViewObject.Transparency/100.0)
                    for f in o.Shape.Faces:
                        colors.append(c)
            elif o.hasExtension("App::GeoFeatureGroupExtension"):
                for so in vobj.Object.Group:
                    if so.isDerivedFrom("Part::Feature"):
                        if len(so.ViewObject.DiffuseColor) > 1:
                            colors.extend(so.ViewObject.DiffuseColor)
                        else:
                            c = so.ViewObject.ShapeColor
                            c = (c[0],c[1],c[2],so.ViewObject.Transparency/100.0)
                            for f in so.Shape.Faces:
                                colors.append(c)
        if colors:
            vobj.DiffuseColor = colors

class _ViewProviderDraftArray(_ViewProviderDraft):
    "a view provider that displays a Array icon instead of a Draft icon"

    def __init__(self,vobj):
        _ViewProviderDraft.__init__(self,vobj)

    def getIcon(self):
        if hasattr(self.Object,"ArrayType"):
            return ":/icons/Draft_Array.svg"
        elif hasattr(self.Object,"PointList"):
            return ":/icons/Draft_PointArray.svg"
        return ":/icons/Draft_PathArray.svg"

    def resetColors(self, vobj):
        colors = []
        if vobj.Object.Base:
            if vobj.Object.Base.isDerivedFrom("Part::Feature"):
                if len(vobj.Object.Base.ViewObject.DiffuseColor) > 1:
                    colors = vobj.Object.Base.ViewObject.DiffuseColor
                else:
                    c = vobj.Object.Base.ViewObject.ShapeColor
                    c = (c[0],c[1],c[2],vobj.Object.Base.ViewObject.Transparency/100.0)
                    for f in vobj.Object.Base.Shape.Faces:
                        colors.append(c)
        if colors:
            n = 1
            if hasattr(vobj.Object,"ArrayType"):
                if vobj.Object.ArrayType == "ortho":
                    n = vobj.Object.NumberX * vobj.Object.NumberY * vobj.Object.NumberZ
                else:
                    n = vobj.Object.NumberPolar
            elif hasattr(vobj.Object,"Count"):
                n = vobj.Object.Count
            colors = colors * n
            vobj.DiffuseColor = colors

class _ShapeString(_DraftObject):
    "The ShapeString object"

    def __init__(self, obj):
        _DraftObject.__init__(self,obj,"ShapeString")
        obj.addProperty("App::PropertyString","String","Draft",QT_TRANSLATE_NOOP("App::Property","Text string"))
        obj.addProperty("App::PropertyFile","FontFile","Draft",QT_TRANSLATE_NOOP("App::Property","Font file name"))
        obj.addProperty("App::PropertyLength","Size","Draft",QT_TRANSLATE_NOOP("App::Property","Height of text"))
        obj.addProperty("App::PropertyLength","Tracking","Draft",QT_TRANSLATE_NOOP("App::Property","Inter-character spacing"))

    def execute(self, obj):
        import Part
        # import OpenSCAD2Dgeom
        import os
        from DraftTools import msg
        if obj.String and obj.FontFile:
            if obj.Placement:
                plm = obj.Placement
            ff8 = obj.FontFile.encode('utf8')                  # 1947 accents in filepath
                                                               # TODO: change for Py3?? bytes?
                                                               # Part.makeWireString uses FontFile as char* string
            if sys.version_info.major < 3:
                CharList = Part.makeWireString(obj.String,ff8,obj.Size,obj.Tracking)
            else:
                CharList = Part.makeWireString(obj.String,obj.FontFile,obj.Size,obj.Tracking)
            if len(CharList) == 0:
                msg(translate("draft","ShapeString: string has no wires")+"\n", 'warning')
                return
            SSChars = []

            # test a simple letter to know if we have a sticky font or not
            sticky = False
            if sys.version_info.major < 3:
                testWire = Part.makeWireString("L",ff8,obj.Size,obj.Tracking)[0][0]
            else:
                testWire = Part.makeWireString("L",obj.FontFile,obj.Size,obj.Tracking)[0][0]
            if testWire.isClosed:
                try:
                    testFace = Part.Face(testWire)
                except Part.OCCError:
                    sticky = True
                else:
                    if not testFace.isValid():
                        sticky = True
            else:
                sticky = True

            for char in CharList:
                if sticky:
                    for CWire in char:
                        SSChars.append(CWire)
                else:
                    CharFaces = []
                    for CWire in char:
                        f = Part.Face(CWire)
                        if f:
                            CharFaces.append(f)
                    # whitespace (ex: ' ') has no faces. This breaks OpenSCAD2Dgeom...
                    if CharFaces:
                        # s = OpenSCAD2Dgeom.Overlappingfaces(CharFaces).makeshape()
                        # s = self.makeGlyph(CharFaces)
                        s = self.makeFaces(char)
                        SSChars.append(s)
            shape = Part.Compound(SSChars)
            obj.Shape = shape
            if plm:
                obj.Placement = plm
        obj.positionBySupport()

    def makeFaces(self, wireChar):
        import Part
        compFaces=[]
        allEdges = []
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
        obj.addProperty("App::PropertyLinkSubList","Faces","Draft",QT_TRANSLATE_NOOP("App::Property","Linked faces"))
        obj.addProperty("App::PropertyBool","RemoveSplitter","Draft",QT_TRANSLATE_NOOP("App::Property","Specifies if splitter lines must be removed"))
        obj.addProperty("App::PropertyDistance","Extrusion","Draft",QT_TRANSLATE_NOOP("App::Property","An optional extrusion value to be applied to all faces"))
        obj.addProperty("App::PropertyBool","Sew","Draft",QT_TRANSLATE_NOOP("App::Property","This specifies if the shapes sew"))


    def execute(self,obj):
        import Part
        pl = obj.Placement
        if not obj.Faces:
            return
        faces = []
        for sel in obj.Faces:
            for f in sel[1]:
                if "Face" in f:
                    try:
                        fnum = int(f[4:])-1
                        faces.append(sel[0].Shape.Faces[fnum])
                    except(IndexError,Part.OCCError):
                        print("Draft: wrong face index")
                        return
        if not faces:
            return
        try:
            if len(faces) > 1:
                sh = None
                if hasattr(obj,"Extrusion"):
                    if obj.Extrusion.Value:
                        for f in faces:
                            f = f.extrude(f.normalAt(0,0).multiply(obj.Extrusion.Value))
                            if sh:
                                sh = sh.fuse(f)
                            else:
                                sh = f
                if not sh:
                    sh = faces.pop()
                    sh = sh.multiFuse(faces)
                if hasattr(obj,"Sew"):
                    if obj.Sew:
                        sh = sh.copy()
                        sh.sewShape()
                if hasattr(obj,"RemoveSplitter"):
                    if obj.RemoveSplitter:
                        sh = sh.removeSplitter()
                else:
                    sh = sh.removeSplitter()
            else:
                sh = faces[0]
                if hasattr(obj,"Extrusion"):
                    if obj.Extrusion.Value:
                        sh = sh.extrude(sh.normalAt(0,0).multiply(obj.Extrusion.Value))
                sh.transformShape(sh.Matrix, True)
        except Part.OCCError:
            print("Draft: error building facebinder")
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


class _ViewProviderFacebinder(_ViewProviderDraft):
    def __init__(self,vobj):
        _ViewProviderDraft.__init__(self,vobj)

    def getIcon(self):
        return ":/icons/Draft_Facebinder_Provider.svg"

    def setEdit(self,vobj,mode):
        import DraftGui
        taskd = DraftGui.FacebinderTaskPanel()
        taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        return False


class _VisGroup:
    "The VisGroup object"
    def __init__(self,obj):
        self.Type = "VisGroup"
        obj.Proxy = self
        self.Object = obj

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state

    def execute(self,obj):
        pass

class _ViewProviderVisGroup:
    "A View Provider for the VisGroup object"
    def __init__(self,vobj):
        vobj.addProperty("App::PropertyColor","LineColor","Base","")
        vobj.addProperty("App::PropertyColor","ShapeColor","Base","")
        vobj.addProperty("App::PropertyFloat","LineWidth","Base","")
        vobj.addProperty("App::PropertyEnumeration","DrawStyle","Base","")
        vobj.addProperty("App::PropertyInteger","Transparency","Base","")
        vobj.DrawStyle = ["Solid","Dashed","Dotted","Dashdot"]
        vobj.LineWidth = 1
        vobj.LineColor = (0.13,0.15,0.37)
        vobj.DrawStyle = "Solid"
        vobj.Proxy = self

    def getIcon(self):
        import Arch_rc
        return ":/icons/Draft_VisGroup.svg"

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def claimChildren(self):
        return self.Object.Group

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def updateData(self,obj,prop):
        if prop == "Group":
            if obj.ViewObject:
                obj.ViewObject.Proxy.onChanged(obj.ViewObject,"LineColor")

    def onChanged(self,vobj,prop):
        if hasattr(vobj,"Object"):
            if vobj.Object:
                if hasattr(vobj.Object,"Group"):
                    if vobj.Object.Group:
                        for o in vobj.Object.Group:
                            if o.ViewObject:
                                for p in ["LineColor","ShapeColor","LineWidth","DrawStyle","Transparency"]:
                                    if hasattr(vobj,p):
                                        if hasattr(o.ViewObject,p):
                                            setattr(o.ViewObject,p,getattr(vobj,p))
                                        elif hasattr(o,p):
                                            # for Drawing views
                                            setattr(o,p,getattr(vobj,p))
                                        elif (p == "DrawStyle") and hasattr(o,"LineStyle"):
                                            # Special case in Drawing views
                                            setattr(o,"LineStyle",getattr(vobj,p))
                                if vobj.Object.InList:
                                    # touch the page if something was changed
                                    if vobj.Object.InList[0].isDerivedFrom("Drawing::FeaturePage"):
                                        vobj.Object.InList[0].touch()


class WorkingPlaneProxy:

    "The Draft working plane proxy object"

    def __init__(self,obj):
        obj.Proxy = self
        obj.addProperty("App::PropertyPlacement","Placement","Base",QT_TRANSLATE_NOOP("App::Property","The placement of this object"))
        obj.addProperty("Part::PropertyPartShape","Shape","Base","")
        self.Type = "WorkingPlaneProxy"

    def execute(self,obj):
        import Part
        l = 1
        if obj.ViewObject:
            if hasattr(obj.ViewObject,"DisplaySize"):
                l = obj.ViewObject.DisplaySize.Value
        p = Part.makePlane(l,l,Vector(l/2,-l/2,0),Vector(0,0,-1))
        # make sure the normal direction is pointing outwards, you never know what OCC will decide...
        if p.normalAt(0,0).getAngle(obj.Placement.Rotation.multVec(FreeCAD.Vector(0,0,1))) > 1:
            p.reverse()
        p.Placement = obj.Placement
        obj.Shape = p

    def onChanged(self,obj,prop):
        pass

    def getNormal(self,obj):
        return obj.Shape.Faces[0].normalAt(0,0)

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state


class ViewProviderWorkingPlaneProxy:

    "A View Provider for working plane proxies"

    def __init__(self,vobj):
        # ViewData: 0,1,2: position; 3,4,5,6: rotation; 7: near dist; 8: far dist, 9:aspect ratio;
        # 10: focal dist; 11: height (ortho) or height angle (persp); 12: ortho (0) or persp (1)
        vobj.addProperty("App::PropertyLength","DisplaySize","Arch",QT_TRANSLATE_NOOP("App::Property","The display length of this section plane"))
        vobj.addProperty("App::PropertyLength","ArrowSize","Arch",QT_TRANSLATE_NOOP("App::Property","The size of the arrows of this section plane"))
        vobj.addProperty("App::PropertyPercent","Transparency","Base","")
        vobj.addProperty("App::PropertyFloat","LineWidth","Base","")
        vobj.addProperty("App::PropertyColor","LineColor","Base","")
        vobj.addProperty("App::PropertyFloatList","ViewData","Base","")
        vobj.addProperty("App::PropertyBool","RestoreView","Base","")
        vobj.addProperty("App::PropertyMap","VisibilityMap","Base","")
        vobj.addProperty("App::PropertyBool","RestoreState","Base","")
        vobj.DisplaySize = 100
        vobj.ArrowSize = 5
        vobj.Transparency = 70
        vobj.LineWidth = 1
        c = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetUnsigned("ColorHelpers",674321151)
        vobj.LineColor = (float((c>>24)&0xFF)/255.0,float((c>>16)&0xFF)/255.0,float((c>>8)&0xFF)/255.0,0.0)
        vobj.Proxy = self

    def getIcon(self):
        import Draft_rc
        return ":/icons/Draft_SelectPlane.svg"

    def claimChildren(self):
        return []

    def doubleClicked(self,vobj):
        FreeCADGui.runCommand("Draft_SelectPlane")
        return True

    def setupContextMenu(self,vobj,menu):
        from PySide import QtCore,QtGui
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_SelectPlane.svg"),"Write camera position",menu)
        QtCore.QObject.connect(action1,QtCore.SIGNAL("triggered()"),self.writeCamera)
        menu.addAction(action1)
        action2 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_SelectPlane.svg"),"Write objects state",menu)
        QtCore.QObject.connect(action2,QtCore.SIGNAL("triggered()"),self.writeState)
        menu.addAction(action2)

    def writeCamera(self):
        if hasattr(self,"Object"):
            from pivy import coin
            n = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
            FreeCAD.Console.PrintMessage(QT_TRANSLATE_NOOP("Draft","Writing camera position")+"\n")
            cdata = list(n.position.getValue().getValue())
            cdata.extend(list(n.orientation.getValue().getValue()))
            cdata.append(n.nearDistance.getValue())
            cdata.append(n.farDistance.getValue())
            cdata.append(n.aspectRatio.getValue())
            cdata.append(n.focalDistance.getValue())
            if isinstance(n,coin.SoOrthographicCamera):
                cdata.append(n.height.getValue())
                cdata.append(0.0) # orthograhic camera
            elif isinstance(n,coin.SoPerspectiveCamera):
                cdata.append(n.heightAngle.getValue())
                cdata.append(1.0) # perspective camera
            self.Object.ViewObject.ViewData = cdata

    def writeState(self):
        if hasattr(self,"Object"):
            FreeCAD.Console.PrintMessage(QT_TRANSLATE_NOOP("Draft","Writing objects shown/hidden state")+"\n")
            vis = {}
            for o in FreeCAD.ActiveDocument.Objects:
                if o.ViewObject:
                    vis[o.Name] = str(o.ViewObject.Visibility)
            self.Object.ViewObject.VisibilityMap = vis

    def attach(self,vobj):
        from pivy import coin
        self.clip = None
        self.mat1 = coin.SoMaterial()
        self.mat2 = coin.SoMaterial()
        self.fcoords = coin.SoCoordinate3()
        fs = coin.SoIndexedFaceSet()
        fs.coordIndex.setValues(0,7,[0,1,2,-1,0,2,3])
        self.drawstyle = coin.SoDrawStyle()
        self.drawstyle.style = coin.SoDrawStyle.LINES
        self.lcoords = coin.SoCoordinate3()
        ls = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        ls.coordIndex.setValues(0,28,[0,1,-1,2,3,4,5,-1,6,7,-1,8,9,10,11,-1,12,13,-1,14,15,16,17,-1,18,19,20,21])
        sep = coin.SoSeparator()
        psep = coin.SoSeparator()
        fsep = coin.SoSeparator()
        fsep.addChild(self.mat2)
        fsep.addChild(self.fcoords)
        fsep.addChild(fs)
        psep.addChild(self.mat1)
        psep.addChild(self.drawstyle)
        psep.addChild(self.lcoords)
        psep.addChild(ls)
        sep.addChild(fsep)
        sep.addChild(psep)
        vobj.addDisplayMode(sep,"Default")
        self.onChanged(vobj,"DisplaySize")
        self.onChanged(vobj,"LineColor")
        self.onChanged(vobj,"Transparency")
        self.Object = vobj.Object

    def getDisplayModes(self,vobj):
        return ["Default"]

    def getDefaultDisplayMode(self):
        return "Default"

    def setDisplayMode(self,mode):
        return mode

    def updateData(self,obj,prop):
        if prop in ["Placement"]:
            self.onChanged(obj.ViewObject,"DisplaySize")
        return

    def onChanged(self,vobj,prop):
        if prop == "LineColor":
            l = vobj.LineColor
            self.mat1.diffuseColor.setValue([l[0],l[1],l[2]])
            self.mat2.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "Transparency":
            if hasattr(vobj,"Transparency"):
                self.mat2.transparency.setValue(vobj.Transparency/100.0)
        elif prop in ["DisplaySize","ArrowSize"]:
            if hasattr(vobj,"DisplaySize"):
                l = vobj.DisplaySize.Value/2
            else:
                l = 1
            verts = []
            fverts = []
            l1 = 0.1
            if hasattr(vobj,"ArrowSize"):
                l1 = vobj.ArrowSize.Value if vobj.ArrowSize.Value > 0 else 0.1
            l2 = l1/3
            pl = FreeCAD.Placement(vobj.Object.Placement)
            fverts.append(pl.multVec(Vector(-l,-l,0)))
            fverts.append(pl.multVec(Vector(l,-l,0)))
            fverts.append(pl.multVec(Vector(l,l,0)))
            fverts.append(pl.multVec(Vector(-l,l,0)))

            verts.append(pl.multVec(Vector(0,0,0)))
            verts.append(pl.multVec(Vector(l-l1,0,0)))
            verts.append(pl.multVec(Vector(l-l1,l2,0)))
            verts.append(pl.multVec(Vector(l,0,0)))
            verts.append(pl.multVec(Vector(l-l1,-l2,0)))
            verts.append(pl.multVec(Vector(l-l1,l2,0)))

            verts.append(pl.multVec(Vector(0,0,0)))
            verts.append(pl.multVec(Vector(0,l-l1,0)))
            verts.append(pl.multVec(Vector(-l2,l-l1,0)))
            verts.append(pl.multVec(Vector(0,l,0)))
            verts.append(pl.multVec(Vector(l2,l-l1,0)))
            verts.append(pl.multVec(Vector(-l2,l-l1,0)))

            verts.append(pl.multVec(Vector(0,0,0)))
            verts.append(pl.multVec(Vector(0,0,l-l1)))
            verts.append(pl.multVec(Vector(-l2,0,l-l1)))
            verts.append(pl.multVec(Vector(0,0,l)))
            verts.append(pl.multVec(Vector(l2,0,l-l1)))
            verts.append(pl.multVec(Vector(-l2,0,l-l1)))
            verts.append(pl.multVec(Vector(0,-l2,l-l1)))
            verts.append(pl.multVec(Vector(0,0,l)))
            verts.append(pl.multVec(Vector(0,l2,l-l1)))
            verts.append(pl.multVec(Vector(0,-l2,l-l1)))

            self.lcoords.point.setValues(verts)
            self.fcoords.point.setValues(fverts)
        elif prop == "LineWidth":
            self.drawstyle.lineWidth = vobj.LineWidth
        return

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


def makeLabel(targetpoint=None,target=None,direction=None,distance=None,labeltype=None,placement=None):
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","dLabel")
    DraftLabel(obj)
    if FreeCAD.GuiUp:
        ViewProviderDraftLabel(obj.ViewObject)
    if targetpoint:
        obj.TargetPoint = targetpoint
    if target:
        obj.Target = target
    if direction:
        obj.StraightDirection = direction
    if distance:
        obj.StraightDistance = distance
    if labeltype:
        obj.LabelType = labeltype
    if placement:
        obj.Placement = placement
    FreeCAD.ActiveDocument.recompute()
    return obj


class DraftLabel:

    "The Draft Label object"

    def __init__(self,obj):
        obj.Proxy = self
        obj.addProperty("App::PropertyPlacement","Placement","Base",QT_TRANSLATE_NOOP("App::Property","The placement of this object"))
        obj.addProperty("App::PropertyDistance","StraightDistance","Base",QT_TRANSLATE_NOOP("App::Property","The length of the straight segment"))
        obj.addProperty("App::PropertyVector","TargetPoint","Base",QT_TRANSLATE_NOOP("App::Property","The point indicated by this label"))
        obj.addProperty("App::PropertyVectorList","Points","Base",QT_TRANSLATE_NOOP("App::Property","The points defining the label polyline"))
        obj.addProperty("App::PropertyEnumeration","StraightDirection","Base",QT_TRANSLATE_NOOP("App::Property","The direction of the straight segment"))
        obj.addProperty("App::PropertyEnumeration","LabelType","Base",QT_TRANSLATE_NOOP("App::Property","The type of information shown by this label"))
        obj.addProperty("App::PropertyLinkSub","Target","Base",QT_TRANSLATE_NOOP("App::Property","The target object of this label"))
        obj.addProperty("App::PropertyStringList","CustomText","Base",QT_TRANSLATE_NOOP("App::Property","The text to display when type is set to custom"))
        obj.addProperty("App::PropertyStringList","Text","Base",QT_TRANSLATE_NOOP("App::Property","The text displayed by this label"))
        self.Type = "Label"
        obj.StraightDirection = ["Horizontal","Vertical","Custom"]
        obj.LabelType = ["Custom","Name","Label","Position","Length","Area","Volume","Tag","Material"]
        obj.setEditorMode("Text",1)
        obj.StraightDistance = 1
        obj.TargetPoint = Vector(2,-1,0)
        obj.CustomText = "Label"

    def execute(self,obj):
        if obj.StraightDirection != "Custom":
            p1 = obj.Placement.Base
            if obj.StraightDirection == "Horizontal":
                p2 = Vector(obj.StraightDistance.Value,0,0)
            else:
                p2 = Vector(0,obj.StraightDistance.Value,0)
            p2 = obj.Placement.multVec(p2)
            # p3 = obj.Placement.multVec(obj.TargetPoint)
            p3 = obj.TargetPoint
            obj.Points = [p1,p2,p3]
        if obj.LabelType == "Custom":
            if obj.CustomText:
                obj.Text = obj.CustomText
        elif obj.Target:
            if obj.LabelType == "Name":
                obj.Text = [obj.Target[0].Name]
            elif obj.LabelType == "Label":
                obj.Text = [obj.Target[0].Label]
            elif obj.LabelType == "Tag":
                if hasattr(obj.Target[0],"Tag"):
                    obj.Text = [obj.Target[0].Tag]
            elif obj.LabelType == "Material":
                if hasattr(obj.Target[0],"Material"):
                    if hasattr(obj.Target[0].Material,"Label"):
                        obj.Text = [obj.Target[0].Material.Label]
            elif obj.LabelType == "Position":
                p = obj.Target[0].Placement.Base
                if obj.Target[1]:
                    if "Vertex" in obj.Target[1][0]:
                        p = obj.Target[0].Shape.Vertexes[int(obj.Target[1][0][6:])-1].Point
                obj.Text = [FreeCAD.Units.Quantity(x,FreeCAD.Units.Length).UserString for x in tuple(p)]
            elif obj.LabelType == "Length":
                if obj.Target[0].isDerivedFrom("Part::Feature"):
                    if hasattr(obj.Target[0].Shape,"Length"):
                        obj.Text = [FreeCAD.Units.Quantity(obj.Target[0].Shape.Length,FreeCAD.Units.Length).UserString]
                    if "Edge" in obj.Target[1][0]:
                        obj.Text = [FreeCAD.Units.Quantity(obj.Target[0].Shape.Edges[int(obj.Target[1][0][4:])-1].Length,FreeCAD.Units.Length).UserString]
            elif obj.LabelType == "Area":
                if obj.Target[0].isDerivedFrom("Part::Feature"):
                    if hasattr(obj.Target[0].Shape,"Area"):
                        obj.Text = [FreeCAD.Units.Quantity(obj.Target[0].Shape.Area,FreeCAD.Units.Area).UserString]
                    if "Face" in obj.Target[1][0]:
                        obj.Text = [FreeCAD.Units.Quantity(obj.Target[0].Shape.Faces[int(obj.Target[1][0][4:])-1].Area,FreeCAD.Units.Area).UserString]
            elif obj.LabelType == "Volume":
                if obj.Target[0].isDerivedFrom("Part::Feature"):
                    if hasattr(obj.Target[0].Shape,"Volume"):
                        obj.Text = [FreeCAD.Units.Quantity(obj.Target[0].Shape.Volume,FreeCAD.Units.Volume).UserString]

    def onChanged(self,obj,prop):
        pass

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state


class ViewProviderDraftLabel:

    "A View Provider for the Draft Label"

    def __init__(self,vobj):
        vobj.addProperty("App::PropertyLength","TextSize","Base",QT_TRANSLATE_NOOP("App::Property","The size of the text"))
        vobj.addProperty("App::PropertyFont","TextFont","Base",QT_TRANSLATE_NOOP("App::Property","The font of the text"))
        vobj.addProperty("App::PropertyLength","ArrowSize","Base",QT_TRANSLATE_NOOP("App::Property","The size of the arrow"))
        vobj.addProperty("App::PropertyEnumeration","TextAlignment","Base",QT_TRANSLATE_NOOP("App::Property","The vertical alignment of the text"))
        vobj.addProperty("App::PropertyEnumeration","ArrowType","Base",QT_TRANSLATE_NOOP("App::Property","The type of arrow of this label"))
        vobj.addProperty("App::PropertyEnumeration","Frame","Base",QT_TRANSLATE_NOOP("App::Property","The type of frame around the text of this object"))
        vobj.addProperty("App::PropertyBool","Line","Base",QT_TRANSLATE_NOOP("App::Property","Display a leader line or not"))
        vobj.addProperty("App::PropertyFloat","LineWidth","Base",QT_TRANSLATE_NOOP("App::Property","Line width"))
        vobj.addProperty("App::PropertyColor","LineColor","Base",QT_TRANSLATE_NOOP("App::Property","Line color"))
        vobj.addProperty("App::PropertyColor","TextColor","Base",QT_TRANSLATE_NOOP("App::Property","Text color"))
        vobj.addProperty("App::PropertyInteger","MaxChars","Base",QT_TRANSLATE_NOOP("App::Property","The maximum number of characters on each line of the text box"))
        vobj.Proxy = self
        self.Object = vobj.Object
        vobj.TextAlignment = ["Top","Middle","Bottom"]
        vobj.TextAlignment = "Middle"
        vobj.LineWidth = getParam("linewidth",1)
        vobj.TextFont = getParam("textfont")
        vobj.TextSize = getParam("textheight",1)
        vobj.ArrowSize = getParam("arrowsize",1)
        vobj.ArrowType = arrowtypes
        vobj.ArrowType = arrowtypes[getParam("dimsymbol")]
        vobj.Frame = ["None","Rectangle"]
        vobj.Line = True

    def getIcon(self):
        import Draft_rc
        return ":/icons/Draft_Label.svg"

    def claimChildren(self):
        return []

    def attach(self,vobj):
        from pivy import coin
        self.arrow = coin.SoSeparator()
        self.arrowpos = coin.SoTransform()
        self.arrow.addChild(self.arrowpos)
        self.matline = coin.SoMaterial()
        self.drawstyle = coin.SoDrawStyle()
        self.drawstyle.style = coin.SoDrawStyle.LINES
        self.lcoords = coin.SoCoordinate3()
        self.line = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        self.mattext = coin.SoMaterial()
        textdrawstyle = coin.SoDrawStyle()
        textdrawstyle.style = coin.SoDrawStyle.FILLED
        self.textpos = coin.SoTransform()
        self.font = coin.SoFont()
        self.text2d = coin.SoText2()
        self.text3d = coin.SoAsciiText()
        self.text2d.string = self.text3d.string = "Label" # need to init with something, otherwise, crash!
        self.text2d.justification = coin.SoText2.RIGHT
        self.text3d.justification = coin.SoAsciiText.RIGHT
        self.fcoords = coin.SoCoordinate3()
        self.frame = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        self.lineswitch = coin.SoSwitch()
        switchnode = coin.SoSeparator()
        switchnode.addChild(self.line)
        switchnode.addChild(self.arrow)
        self.lineswitch.addChild(switchnode)
        self.lineswitch.whichChild = 0
        self.node2d = coin.SoGroup()
        self.node2d.addChild(self.matline)
        self.node2d.addChild(self.arrow)
        self.node2d.addChild(self.drawstyle)
        self.node2d.addChild(self.lcoords)
        self.node2d.addChild(self.lineswitch)
        self.node2d.addChild(self.mattext)
        self.node2d.addChild(textdrawstyle)
        self.node2d.addChild(self.textpos)
        self.node2d.addChild(self.font)
        self.node2d.addChild(self.text2d)
        self.node2d.addChild(self.fcoords)
        self.node2d.addChild(self.frame)
        self.node3d = coin.SoGroup()
        self.node3d.addChild(self.matline)
        self.node3d.addChild(self.arrow)
        self.node3d.addChild(self.drawstyle)
        self.node3d.addChild(self.lcoords)
        self.node3d.addChild(self.lineswitch)
        self.node3d.addChild(self.mattext)
        self.node3d.addChild(textdrawstyle)
        self.node3d.addChild(self.textpos)
        self.node3d.addChild(self.font)
        self.node3d.addChild(self.text3d)
        self.node3d.addChild(self.fcoords)
        self.node3d.addChild(self.frame)
        vobj.addDisplayMode(self.node2d,"2D text")
        vobj.addDisplayMode(self.node3d,"3D text")
        self.onChanged(vobj,"LineColor")
        self.onChanged(vobj,"TextColor")
        self.onChanged(vobj,"ArrowSize")

    def getDisplayModes(self,vobj):
        return ["2D text","3D text"]

    def getDefaultDisplayMode(self):
        return "3D text"

    def setDisplayMode(self,mode):
        return mode

    def updateData(self,obj,prop):
        if prop == "Points":
            from pivy import coin
            if len(obj.Points) >= 2:
                self.line.coordIndex.deleteValues(0)
                self.lcoords.point.setValues(obj.Points)
                self.line.coordIndex.setValues(0,len(obj.Points),range(len(obj.Points)))
                self.onChanged(obj.ViewObject,"TextSize")
                self.onChanged(obj.ViewObject,"ArrowType")
            if obj.StraightDistance > 0:
                self.text2d.justification = coin.SoText2.RIGHT
                self.text3d.justification = coin.SoAsciiText.RIGHT
            else:
                self.text2d.justification = coin.SoText2.LEFT
                self.text3d.justification = coin.SoAsciiText.LEFT
        elif prop == "Text":
            if obj.Text:
                self.text2d.string.setValues([l.encode("utf8") for l in obj.Text if l])
                self.text3d.string.setValues([l.encode("utf8") for l in obj.Text if l])
                self.onChanged(obj.ViewObject,"TextAlignment")

    def getTextSize(self,vobj):
        from pivy import coin
        if vobj.DisplayMode == "3D text":
            text = self.text3d
        else:
            text = self.text2d
        v = FreeCADGui.ActiveDocument.ActiveView.getViewer().getSoRenderManager().getViewportRegion()
        b = coin.SoGetBoundingBoxAction(v)
        text.getBoundingBox(b)
        return b.getBoundingBox().getSize().getValue()

    def onChanged(self,vobj,prop):
        if prop == "LineColor":
            if hasattr(vobj,"LineColor"):
                l = vobj.LineColor
                self.matline.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "TextColor":
            if hasattr(vobj,"TextColor"):
                l = vobj.TextColor
                self.mattext.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "LineWidth":
            if hasattr(vobj,"LineWidth"):
                self.drawstyle.lineWidth = vobj.LineWidth
        elif (prop == "TextFont"):
            if hasattr(vobj,"TextFont"):
                self.font.name = vobj.TextFont.encode("utf8")
        elif prop in ["TextSize","TextAlignment"]:
            if hasattr(vobj,"TextSize") and hasattr(vobj,"TextAlignment"):
                self.font.size = vobj.TextSize.Value
                v = Vector(1,0,0)
                if vobj.Object.StraightDistance > 0:
                    v = v.negative()
                v.multiply(vobj.TextSize/10)
                tsize = self.getTextSize(vobj)
                if len(vobj.Object.Text) > 1:
                    v = v.add(Vector(0,(tsize[1]-1)*2,0))
                if vobj.TextAlignment == "Top":
                    v = v.add(Vector(0,-tsize[1]*2,0))
                elif vobj.TextAlignment == "Middle":
                    v = v.add(Vector(0,-tsize[1],0))
                v = vobj.Object.Placement.Rotation.multVec(v)
                pos = vobj.Object.Placement.Base.add(v)
                self.textpos.translation.setValue(pos)
                self.textpos.rotation.setValue(vobj.Object.Placement.Rotation.Q)
        elif prop == "Line":
            if hasattr(vobj,"Line"):
                if vobj.Line:
                    self.lineswitch.whichChild = 0
                else:
                    self.lineswitch.whichChild = -1
        elif prop == "ArrowType":
            if hasattr(vobj,"ArrowType"):
                if len(vobj.Object.Points) > 1:
                    if hasattr(self,"symbol"):
                        if self.arrow.findChild(self.symbol) != -1:
                                self.arrow.removeChild(self.symbol)
                    s = arrowtypes.index(vobj.ArrowType)
                    self.symbol = dimSymbol(s)
                    self.arrow.addChild(self.symbol)
                    self.arrowpos.translation.setValue(vobj.Object.Points[-1])
                    v1 = vobj.Object.Points[-2].sub(vobj.Object.Points[-1])
                    if not DraftVecUtils.isNull(v1):
                        v1.normalize()
                        import DraftGeomUtils
                        v2 = Vector(0,0,1)
                        if round(v2.getAngle(v1),4) in [0,round(math.pi,4)]:
                            v2 = Vector(0,1,0)
                        v3 = v1.cross(v2).negative()
                        q = FreeCAD.Placement(DraftVecUtils.getPlaneRotation(v1,v3,v2)).Rotation.Q
                        self.arrowpos.rotation.setValue((q[0],q[1],q[2],q[3]))
        elif prop == "ArrowSize":
            if hasattr(vobj,"ArrowSize"):
                s = vobj.ArrowSize.Value
                if s:
                    self.arrowpos.scaleFactor.setValue((s,s,s))
        elif prop == "Frame":
            if hasattr(vobj,"Frame"):
                self.frame.coordIndex.deleteValues(0)
                if vobj.Frame == "Rectangle":
                    tsize = self.getTextSize(vobj)
                    pts = []
                    base = vobj.Object.Placement.Base.sub(Vector(self.textpos.translation.getValue().getValue()))
                    pts.append(base.add(Vector(0,tsize[1]*3,0)))
                    pts.append(pts[-1].add(Vector(-tsize[0]*6,0,0)))
                    pts.append(pts[-1].add(Vector(0,-tsize[1]*6,0)))
                    pts.append(pts[-1].add(Vector(tsize[0]*6,0,0)))
                    pts.append(pts[0])
                    self.fcoords.point.setValues(pts)
                    self.frame.coordIndex.setValues(0,len(pts),range(len(pts)))

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class DraftText:

    "The Draft Text object"

    def __init__(self,obj):
        obj.Proxy = self
        obj.addProperty("App::PropertyPlacement","Placement","Base",QT_TRANSLATE_NOOP("App::Property","The placement of this object"))
        obj.addProperty("App::PropertyStringList","Text","Base",QT_TRANSLATE_NOOP("App::Property","The text displayed by this object"))
        self.Type = "DraftText"

    def execute(self,obj):
        pass


class ViewProviderDraftText:

    "A View Provider for the Draft Label"

    def __init__(self,vobj):
        vobj.addProperty("App::PropertyLength","FontSize","Base",QT_TRANSLATE_NOOP("App::Property","The size of the text"))
        vobj.addProperty("App::PropertyFont","FontName","Base",QT_TRANSLATE_NOOP("App::Property","The font of the text"))
        vobj.addProperty("App::PropertyEnumeration","Justification","Base",QT_TRANSLATE_NOOP("App::Property","The vertical alignment of the text"))
        vobj.addProperty("App::PropertyColor","TextColor","Base",QT_TRANSLATE_NOOP("App::Property","Text color"))
        vobj.addProperty("App::PropertyFloat","LineSpacing","Base",QT_TRANSLATE_NOOP("App::Property","Line spacing (relative to font size)"))
        vobj.Proxy = self
        self.Object = vobj.Object
        vobj.Justification = ["Left","Center","Right"]
        vobj.FontName = getParam("textfont","sans")
        vobj.FontSize = getParam("textheight",1)

    def getIcon(self):
        import Draft_rc
        return ":/icons/Draft_Text.svg"

    def claimChildren(self):
        return []

    def attach(self,vobj):
        from pivy import coin
        self.mattext = coin.SoMaterial()
        textdrawstyle = coin.SoDrawStyle()
        textdrawstyle.style = coin.SoDrawStyle.FILLED
        self.trans = coin.SoTransform()
        self.font = coin.SoFont()
        self.text2d = coin.SoText2()
        self.text3d = coin.SoAsciiText()
        self.text2d.string = self.text3d.string = "Label" # need to init with something, otherwise, crash!
        self.text2d.justification = coin.SoText2.LEFT
        self.text3d.justification = coin.SoAsciiText.LEFT
        self.node2d = coin.SoGroup()
        self.node2d.addChild(self.trans)
        self.node2d.addChild(self.mattext)
        self.node2d.addChild(textdrawstyle)
        self.node2d.addChild(self.font)
        self.node2d.addChild(self.text2d)
        self.node3d = coin.SoGroup()
        self.node3d.addChild(self.trans)
        self.node3d.addChild(self.mattext)
        self.node3d.addChild(textdrawstyle)
        self.node3d.addChild(self.font)
        self.node3d.addChild(self.text3d)
        vobj.addDisplayMode(self.node2d,"2D text")
        vobj.addDisplayMode(self.node3d,"3D text")
        self.onChanged(vobj,"TextColor")
        self.onChanged(vobj,"FontSize")
        self.onChanged(vobj,"FontName")
        self.onChanged(vobj,"Justification")
        self.onChanged(vobj,"LineSpacing")

    def getDisplayModes(self,vobj):
        return ["2D text","3D text"]

    def getDefaultDisplayMode(self):
        return "3D text"

    def setDisplayMode(self,mode):
        return mode

    def updateData(self,obj,prop):
        if prop == "Text":
            if obj.Text:
                if sys.version_info.major >= 3:
                    self.text2d.string.setValues([l for l in obj.Text if l])
                    self.text3d.string.setValues([l for l in obj.Text if l])
                else:
                    self.text2d.string.setValues([l.encode("utf8") for l in obj.Text if l])
                    self.text3d.string.setValues([l.encode("utf8") for l in obj.Text if l])
        elif prop == "Placement":
            self.trans.translation.setValue(obj.Placement.Base)
            self.trans.rotation.setValue(obj.Placement.Rotation.Q)

    def onChanged(self,vobj,prop):
        if prop == "TextColor":
            if "TextColor" in vobj.PropertiesList:
                l = vobj.TextColor
                self.mattext.diffuseColor.setValue([l[0],l[1],l[2]])
        elif (prop == "FontName"):
            if "FontName" in vobj.PropertiesList:
                self.font.name = vobj.FontName.encode("utf8")
        elif prop  == "FontSize":
            if "FontSize" in vobj.PropertiesList:
                self.font.size = vobj.FontSize.Value
        elif prop == "Justification":
            if getattr(vobj.PropertiesList, "Justification", None) is not None:
                from pivy import coin
                try:
                    if vobj.Justification == "Left":
                        self.text2d.justification = coin.SoText2.LEFT
                        self.text3d.justification = coin.SoAsciiText.LEFT
                    elif vobj.Justification == "Right":
                        self.text2d.justification = coin.SoText2.RIGHT
                        self.text3d.justification = coin.SoAsciiText.RIGHT
                    else:
                        self.text2d.justification = coin.SoText2.CENTER
                        self.text3d.justification = coin.SoAsciiText.CENTER
                except AssertionError:
                    pass # Race condition - Justification enum has not been set yet
        elif prop == "LineSpacing":
            if "LineSpacing" in vobj.PropertiesList:
                self.text2d.spacing = vobj.LineSpacing
                self.text3d.spacing = vobj.LineSpacing

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


def convertDraftTexts(textslist=[]):
    "converts the given Draft texts (or all that are found in the active document) to the new object"
    if not isinstance(textslist,list):
        textslist = [textslist]
    if not textslist:
        for o in FreeCAD.ActiveDocument.Objects:
            if o.TypeId == "App::Annotation":
                textslist.append(o)
    todelete = []
    for o in textslist:
        l = o.Label
        o.Label = l+".old"
        obj = makeText(o.LabelText,point=o.Position)
        obj.Label = l
        todelete.append(o.Name)
        for p in o.InList:
            if p.isDerivedFrom("App::DocumentObjectGroup"):
                if o in p.Group:
                    g = p.Group
                    g.append(obj)
                    p.Group = g
    for n in todelete:
        FreeCAD.ActiveDocument.removeObject(n)

## @}
