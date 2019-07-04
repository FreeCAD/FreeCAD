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

import FreeCAD

def translate(ctx,txt):
    return txt

def QT_TRANSLATE_NOOP(ctx,txt):
    return txt

"This module contains everything related to Draft Layers"


def makeLayer(name=None,linecolor=None,drawstyle=None,shapecolor=None,transparency=None):

    '''makeLayer([name,linecolor,drawstyle,shapecolor,transparency]):
       creates a Layer object in the active document'''

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError(translate("draft","No active document. Aborting")+"\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Layer")
    Layer(obj)
    if name:
        obj.Label = name
    else:
        obj.Label = translate("draft","Layer")
    if FreeCAD.GuiUp:
        ViewProviderLayer(obj.ViewObject)
        if linecolor:
            obj.ViewObject.LineColor = linecolor
        if drawstyle:
            obj.ViewObject.DrawStyle = drawstyle
        if shapecolor:
            obj.ViewObject.ShapeColor = shapecolor
        if transparency:
            obj.ViewObject.Transparency = transparency
    getLayerContainer().addObject(obj)
    return obj


def getLayerContainer():

    '''getLayerContainer(): returns a group object to put layers in'''

    for obj in FreeCAD.ActiveDocument.Objects:
        if obj.Name == "LayerContainer":
            return obj
    obj = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroupPython","LayerContainer")
    obj.Label = translate("draft","Layers")
    LayerContainer(obj)
    if FreeCAD.GuiUp:
        ViewProviderLayerContainer(obj.ViewObject)
    return obj


class CommandLayer():

    "The Draft_Layer FreeCAD command"

    def GetResources(self):

        return {'Pixmap'  : 'Draft_VisGroup',
                'MenuText': QT_TRANSLATE_NOOP("Draft_Layer", "Layer"),
                'ToolTip' : QT_TRANSLATE_NOOP("Draft_Layer", "Adds a layer")}

    def Activated(self):

        import FreeCADGui
        FreeCAD.ActiveDocument.openTransaction("Create Layer")
        FreeCADGui.addModule("Draft")
        FreeCADGui.doCommand('Draft.makeLayer()')
        FreeCADGui.doCommand('FreeCAD.ActiveDocument.recompute()')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class Layer:

    "The Draft Layer object"

    def __init__(self,obj):

        self.Type = "Layer"
        obj.Proxy = self
        self.Object = obj
        self.setProperties(obj)

    def onDocumentRestored(self,obj):

        self.setProperties(obj)

    def setProperties(self,obj):

        if not "Group" in obj.PropertiesList:
            obj.addProperty("App::PropertyLinkList","Group","Layer",QT_TRANSLATE_NOOP("App::Property","The objects that are part of this layer"))

    def __getstate__(self):

        return self.Type

    def __setstate__(self,state):

        if state:
            self.Type = state

    def execute(self,obj):
        pass

    def addObject(self,obj,child):

        g = obj.Group
        if not child in g:
            g.append(child)
        obj.Group = g


class ViewProviderLayer:

    "A View Provider for the Layer object"

    def __init__(self,vobj):

        vobj.addProperty("App::PropertyBool","OverrideChildren","Layer",QT_TRANSLATE_NOOP("App::Property","If on, the child objects of this layer will match its visual aspects"))
        vobj.addProperty("App::PropertyColor","LineColor","Layer",QT_TRANSLATE_NOOP("App::Property","The line color of the children of this layer"))
        vobj.addProperty("App::PropertyColor","ShapeColor","Layer",QT_TRANSLATE_NOOP("App::Property","The shape color of the children of this layer"))
        vobj.addProperty("App::PropertyFloat","LineWidth","Layer",QT_TRANSLATE_NOOP("App::Property","The line width of the children of this layer"))
        vobj.addProperty("App::PropertyEnumeration","DrawStyle","Layer",QT_TRANSLATE_NOOP("App::Property","The draw style of the children of this layer"))
        vobj.addProperty("App::PropertyInteger","Transparency","Layer",QT_TRANSLATE_NOOP("App::Property","The transparency of the children of this layer"))
        vobj.DrawStyle = ["Solid","Dashed","Dotted","Dashdot"]

        vobj.OverrideChildren = True
        c = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").GetUnsigned("DefaultShapeLineColor",255)
        vobj.LineColor = (((c>>24)&0xFF)/255,((c>>16)&0xFF)/255,((c>>8)&0xFF)/255)
        w = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").GetInt("DefaultShapeLineWidth",2)
        vobj.LineWidth = w
        c = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View").GetUnsigned("DefaultShapeColor",4294967295)
        vobj.ShapeColor = (((c>>24)&0xFF)/255,((c>>16)&0xFF)/255,((c>>8)&0xFF)/255)
        vobj.DrawStyle = "Solid"

        vobj.Proxy = self

    def getIcon(self):

        if hasattr(self,"icondata"):
            return self.icondata
        import Draft_rc
        return ":/icons/Draft_VisGroup.svg"

    def attach(self,vobj):

        self.Object = vobj.Object
        from pivy import coin
        sep = coin.SoGroup()
        vobj.addDisplayMode(sep,"Default")
        return

    def claimChildren(self):

        if hasattr(self,"Object") and hasattr(self.Object,"Group"):
            return self.Object.Group

    def getDisplayModes(self, vobj):

        return ["Default"]

    def getDefaultDisplayMode(self):

        return "Default"

    def setDisplayMode(self, mode):

        return mode

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

    def updateData(self,obj,prop):

        if prop == "Group":
            self.onChanged(obj.ViewObject,"LineColor")

    def onChanged(self,vobj,prop):

        if hasattr(vobj,"OverrideChildren") and vobj.OverrideChildren:
            if hasattr(vobj,"Object")and hasattr(vobj.Object,"Group"):
                for o in vobj.Object.Group:
                    if o.ViewObject:
                        for p in ["LineColor","ShapeColor","LineWidth","DrawStyle","Transparency"]:
                            if hasattr(vobj,p) and hasattr(o.ViewObject,p):
                                setattr(o.ViewObject,p,getattr(vobj,p))

        if (prop == "Visibility") and hasattr(vobj,"Visibility"):
            if hasattr(vobj,"Object")and hasattr(vobj.Object,"Group"):
                for o in vobj.Object.Group:
                    if o.ViewObject and hasattr(o.ViewObject,"Visibility"):
                        o.ViewObject.Visibility = vobj.Visibility

        if (prop in ["LineColor","ShapeColor"]) and hasattr(vobj,"LineColor") and hasattr(vobj,"ShapeColor"):
            from PySide import QtCore,QtGui
            lc = vobj.LineColor
            sc = vobj.ShapeColor
            lc = QtGui.QColor(int(lc[0]*255),int(lc[1]*255),int(lc[2]*255))
            sc = QtGui.QColor(int(sc[0]*255),int(sc[1]*255),int(sc[2]*255))
            p1 = QtCore.QPointF(2,17)
            p2 = QtCore.QPointF(13,8)
            p3 = QtCore.QPointF(30,15)
            p4 = QtCore.QPointF(20,25)
            im = QtGui.QImage(32,32,QtGui.QImage.Format_ARGB32)
            im.fill(QtCore.Qt.transparent)
            pt = QtGui.QPainter(im)
            pt.setBrush(QtGui.QBrush(sc, QtCore.Qt.SolidPattern))
            pt.drawPolygon([p1,p2,p3,p4])
            pt.setPen(QtGui.QPen(lc, 2, QtCore.Qt.SolidLine, QtCore.Qt.FlatCap))
            pt.drawPolygon([p1,p2,p3,p4])
            pt.end()
            ba = QtCore.QByteArray()
            b = QtCore.QBuffer(ba)
            b.open(QtCore.QIODevice.WriteOnly)
            im.save(b,"XPM")
            self.icondata = ba.data().decode("latin1")

    def canDragObject(self,obj):

        return True

    def canDragObjects(self):

        return True

    def dragObject(self,vobj,otherobj):

        if hasattr(vobj.Object,"Group"):
            if otherobj in vobj.Object.Group:
                g = vobj.Object.Group
                g.remove(otherobj)
                vobj.Object.Group = g
                FreeCAD.ActiveDocument.recompute()

    def canDropObject(self,obj):

        if hasattr(obj,"Proxy") and isinstance(obj.Proxy,Layer): # for now, prevent stacking layers
            return False
        return True

    def canDropObjects(self):

        return True

    def dropObject(self,vobj,otherobj):

        if hasattr(vobj.Object,"Group"):
            if not otherobj in vobj.Object.Group:
                if not(hasattr(otherobj,"Proxy") and isinstance(otherobj.Proxy,Layer)): # for now, prevent stacking layers
                    g = vobj.Object.Group
                    g.append(otherobj)
                    vobj.Object.Group = g
                    # remove from all other layers (not automatic)
                    for parent in otherobj.InList:
                        if hasattr(parent,"Proxy") and isinstance(parent.Proxy,Layer):
                            if otherobj in parent.Group:
                                if parent != vobj.Object:
                                    g = parent.Group
                                    g.remove(otherobj)
                                    parent.Group = g
                    FreeCAD.ActiveDocument.recompute()

    def setupContextMenu(self,vobj,menu):

        from PySide import QtCore,QtGui
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/button_right.svg"),translate("draft","Activate this layer"),menu)
        action1.triggered.connect(self.activate)
        menu.addAction(action1)

    def activate(self):

        if hasattr(self,"Object"):
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelection(self.Object)
            FreeCADGui.runCommand("Draft_AutoGroup")


class LayerContainer:

    "The Layer Container"

    def __init__(self,obj):

        self.Type = "LayerContainer"
        obj.Proxy = self

    def execute(self,obj):

        g = obj.Group
        g.sort(key=lambda o: o.Label)
        obj.Group = g

    def __getstate__(self):

        if hasattr(self,"Type"):
            return self.Type

    def __setstate__(self,state):

        if state:
            self.Type = state


class ViewProviderLayerContainer:

    "A View Provider for the Layer Container"

    def __init__(self,vobj):

        vobj.Proxy = self

    def getIcon(self):

        import Draft_rc
        return ":/icons/Draft_VisGroup.svg"

    def attach(self,vobj):

        self.Object = vobj.Object

    def setupContextMenu(self,vobj,menu):

        import Draft_rc
        from PySide import QtCore,QtGui
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_VisGroup.svg"),"Merge duplicates",menu)
        action1.triggered.connect(self.mergeByName)
        menu.addAction(action1)

    def mergeByName(self):

        if hasattr(self,"Object") and hasattr(self.Object,"Group"):
            layers = [o for o in self.Object.Group if (hasattr(o,"Proxy") and isinstance(o.Proxy,Layer))]
            todelete = []
            for layer in layerss:
                if layer.Label[-1].isdigit() and layer.Label[-2].isdigit() and layer.Label[-3].isdigit():
                    orig = None
                    for ol in layer:
                        if ol.Label == layer.Label[:-3].strip():
                            orig = ol
                            break
                    if orig:
                        for par in layer.InList:
                            for prop in par.PropertiesList:
                                if getattr(par,prop) == layer:
                                    FreeCAD.Console.PrintMessage("Changed property '"+prop+"' of object "+par.Label+" from "+layer.Label+" to "+orig.Label+"\n")
                                    setattr(par,prop,orig)
                        todelete.append(layer)
            for tod in todelete:
                if not tod.InList:
                    FreeCAD.Console.PrintMessage("Merging duplicate layer "+tod.Label+"\n")
                    FreeCAD.ActiveDocument.removeObject(tod.Name)
                elif (len(tod.InList) == 1) and (tod.InList[0].isDerivedFrom("App::DocumentObjectGroup")):
                    FreeCAD.Console.PrintMessage("Merging duplicate layer "+tod.Label+"\n")
                    FreeCAD.ActiveDocument.removeObject(tod.Name)
                else:
                    FreeCAD.Console.PrintMessage("Unable to delete layer "+tod.Label+": InList not empty\n")

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

if FreeCAD.GuiUp:
    import FreeCADGui
    FreeCADGui.addCommand('Draft_Layer',CommandLayer())
