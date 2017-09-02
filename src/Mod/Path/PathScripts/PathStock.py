# -*- coding: utf-8 -*-
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
'''used to create material stock around a machined part- for visualization '''

import FreeCAD
import FreeCADGui
import Part
import PathIconViewProvider

from PySide import QtCore

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class StockFromBase:

    def __init__(self, obj, base):
        "Make stock"
        obj.addProperty("App::PropertyLink", "Base", "Base", QtCore.QT_TRANSLATE_NOOP("PathStock", "The base object this stock is derived from"))
        obj.addProperty("App::PropertyLength", "ExtXneg", "Stock", QtCore.QT_TRANSLATE_NOOP("PathStock", "Extra allowance from part bound box in negative X direction"))
        obj.addProperty("App::PropertyLength", "ExtXpos", "Stock", QtCore.QT_TRANSLATE_NOOP("PathStock", "Extra allowance from part bound box in positive X direction"))
        obj.addProperty("App::PropertyLength", "ExtYneg", "Stock", QtCore.QT_TRANSLATE_NOOP("PathStock", "Extra allowance from part bound box in negative Y direction"))
        obj.addProperty("App::PropertyLength", "ExtYpos", "Stock", QtCore.QT_TRANSLATE_NOOP("PathStock", "Extra allowance from part bound box in positive Y direction"))
        obj.addProperty("App::PropertyLength", "ExtZneg", "Stock", QtCore.QT_TRANSLATE_NOOP("PathStock", "Extra allowance from part bound box in negative Z direction"))
        obj.addProperty("App::PropertyLength", "ExtZpos", "Stock", QtCore.QT_TRANSLATE_NOOP("PathStock", "Extra allowance from part bound box in positive Z direction"))

        obj.Base = base
        obj.ExtXneg= 1.0
        obj.ExtXpos= 1.0
        obj.ExtYneg= 1.0
        obj.ExtYpos= 1.0
        obj.ExtZneg= 1.0
        obj.ExtZpos= 1.0

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def execute(self, obj):
        bb = obj.Base.Shape.BoundBox

        origin = FreeCAD.Vector(bb.XMin, bb.YMin, bb.ZMin)
        self.origin = origin - FreeCAD.Vector(obj.ExtXneg.Value, obj.ExtYneg.Value, obj.ExtZneg.Value)

        self.length = bb.XLength + obj.ExtXneg.Value + obj.ExtXpos.Value
        self.width  = bb.YLength + obj.ExtYneg.Value + obj.ExtYpos.Value
        self.height = bb.ZLength + obj.ExtZneg.Value + obj.ExtZpos.Value

        obj.Shape = Part.makeBox(self.length, self.width, self.height, self.origin)

def SetupStockObject(obj, addVPProxy):
    if FreeCAD.GuiUp and obj.ViewObject:
        if addVPProxy:
            PathIconViewProvider.ViewProvider(obj.ViewObject, 'Stock')
        obj.ViewObject.Transparency = 90
        obj.ViewObject.DisplayMode = 'Wireframe'

def CreateFromBase(job):
    obj = FreeCAD.ActiveDocument.addObject('Part::FeaturePython', 'Stock')
    proxy = StockFromBase(obj, job.Base)
    SetupStockObject(obj, True)
    proxy.execute(obj)
    obj.purgeTouched()
    return obj

def CreateBox(job, extent=None, at=None):
    obj = FreeCAD.ActiveDocument.addObject('Part::Box', 'Stock')
    if extent:
        obj.Length = extent.x
        obj.Width  = extent.y
        obj.Height = extent.z
    elif job.Base:
        bb = job.Base.Shape.BoundBox
        obj.Length = bb.XLength
        obj.Width  = bb.YLength
        obj.Height = bb.ZLength
    if at:
        obj.Placement = FreeCAD.Placement(at, FreeCAD.Vector(), 0)
    else:
        bb = job.Base.Shape.BoundBox
        origin = FreeCAD.Vector(bb.XMin, bb.YMin, bb.ZMin)
        obj.Placement = FreeCAD.Placement(origin, FreeCAD.Vector(), 0)
    SetupStockObject(obj, False)
    return obj

def CreateCylinder(job, radius=None, height=None, at=None):
    obj = FreeCAD.ActiveDocument.addObject('Part::Cylinder', 'Stock')
    if radius:
        obj.Radius = radius
    if height:
        obj.Height = height
    elif job.Base:
        bb = job.Base.Shape.BoundBox
        obj.Radius = max(bb.XLength, bb.YLength) * 0.7072 # 1/sqrt(2)
        obj.Height = bb.ZLength
    if at:
        obj.Placement = FreeCAD.Placement(at, FreeCAD.Vector(), 0)
    else:
        bb = job.Base.Shape.BoundBox
        origin = FreeCAD.Vector((bb.XMin + bb.XMax)/2, (bb.YMin + bb.YMax)/2, bb.ZMin)
        obj.Placement = FreeCAD.Placement(origin, FreeCAD.Vector(), 0)
    SetupStockObject(obj, False)
    return obj


class _ViewProviderStock:

    def __init__(self, obj):  # mandatory
        #        obj.addProperty("App::PropertyFloat","SomePropertyName","PropertyGroup","Description of this property")
        obj.Proxy = self

    def __getstate__(self):  # mandatory
        return None

    def __setstate__(self, state):  # mandatory
        return None

    def getIcon(self):  # optional
        return ":/icons/Path-Stock.svg"

    def attach(self, vobj):  # optional
        self.Object = vobj.Object


class CommandPathStock:

    def GetResources(self):
        return {'Pixmap': 'Path-Stock',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathStock", "Stock"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathStock", "Creates a 3D object to represent raw stock to mill the part out of")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Stock", "Creates a 3D object to represent raw stock to mill the part out of"))
        FreeCADGui.addModule("PathScripts.PathStock")
        snippet = '''
import FreeCADGui
if len(FreeCADGui.Selection.getSelection())>0:
    sel=FreeCADGui.Selection.getSelection()
    o = sel[0]
    if "Shape" in o.PropertiesList:
        obj =FreeCAD.ActiveDocument.addObject('Part::FeaturePython',sel[0].Name+('_Stock'))
        PathScripts.PathStock.Stock(obj)
        PathScripts.PathStock._ViewProviderStock(obj.ViewObject)
        PathScripts.PathUtils.addToJob(obj)
        baseobj = sel[0]
        obj.Base = baseobj
        FreeCADGui.ActiveDocument.getObject(sel[0].Name+("_Stock")).ShapeColor = (0.3333,0.6667,1.0000)
        FreeCADGui.ActiveDocument.getObject(sel[0].Name+("_Stock")).Transparency = 75
        FreeCAD.ActiveDocument.recompute()
    else:
        FreeCAD.Console.PrintMessage("Select a Solid object and try again.\\n")
else:
    FreeCAD.Console.PrintMessage("Select the object you want to show stock for and try again.\\n")
        '''
        FreeCADGui.doCommand(snippet)

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Stock', CommandPathStock())

FreeCAD.Console.PrintLog("Loading PathStock... done\n")
