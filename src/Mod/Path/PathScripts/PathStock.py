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
import Part
import PathScripts.PathIconViewProvider as PathIconViewProvider
import PathScripts.PathLog as PathLog
import math

from PySide import QtCore


if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class StockType:
    NoStock        = 'None'
    FromBase       = 'FromBase'
    CreateBox      = 'CreateBox'
    CreateCylinder = 'CreateCylinder'
    Unknown        = 'Unknown'

    @classmethod
    def FromStock(cls, stock):
        '''FromStock(stock) ... Answer a string representing the type of stock.'''
        if not stock:
            return cls.NoStock
        if hasattr(stock, 'StockType'):
            return stock.StockType

        # fallback in case somebody messed with internals
        if hasattr(stock, 'ExtXneg') and hasattr(stock, 'ExtZpos'):
            return cls.FromBase
        if hasattr(stock, 'Length') and hasattr(stock, 'Width'):
            return cls.CreateBox
        if hasattr(stock, 'Radius') and hasattr(stock, 'Height'):
            return cls.CreateCylinder
        return cls.Unknown

def shapeBoundBox(obj):
    if hasattr(obj, 'Shape'):
        return obj.Shape.BoundBox
    if obj and 'App::Part' == obj.TypeId:
        bounds = [shapeBoundBox(o) for o in obj.Group]
        if bounds:
            bb = bounds[0]
            for b in bounds[1:]:
                bb = bb.united(b)
            return bb
    if obj:
        PathLog.error(translate('PathStock', "Invalid base object %s - no shape found") % obj.Name)
    return None

class StockFromBase:

    def __init__(self, obj, base, placement):
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

        dPos = placement.Base - base.Placement.Base
        dRot = placement.Rotation.multiply(base.Placement.Rotation.inverted())
        dPlacement = FreeCAD.Placement(dPos, dRot)
        PathLog.debug("%s - %s: %s" % (placement, base.Placement, dPlacement))
        obj.Placement = dPlacement
        obj.Proxy = self

    def __getstate__(self):
        return None
    def __setstate__(self, state):
        return None

    def execute(self, obj):
        bb = shapeBoundBox(obj.Base)

        # Sometimes, when the Base changes it's temporarily not assigned when
        # Stock.execute is triggered - it'll be set correctly the next time around.
        if bb:
            origin = FreeCAD.Vector(bb.XMin, bb.YMin, bb.ZMin)
            self.origin = origin - FreeCAD.Vector(obj.ExtXneg.Value, obj.ExtYneg.Value, obj.ExtZneg.Value)

            self.length = bb.XLength + obj.ExtXneg.Value + obj.ExtXpos.Value
            self.width  = bb.YLength + obj.ExtYneg.Value + obj.ExtYpos.Value
            self.height = bb.ZLength + obj.ExtZneg.Value + obj.ExtZpos.Value

            shape = Part.makeBox(self.length, self.width, self.height, self.origin)
            shape.Placement = obj.Placement
            obj.Shape = shape

    def onChanged(self, obj, prop):
        if prop in ['ExtXneg', 'ExtXpos', 'ExtYneg', 'ExtYpos', 'ExtZneg', 'ExtZpos'] and not 'Restore' in obj.State:
            self.execute(obj)


class StockCreateBox:
    MinExtent = 0.001

    def __init__(self, obj):
        obj.addProperty('App::PropertyLength', 'Length', 'Stock', QtCore.QT_TRANSLATE_NOOP("PathStock", "Length of this stock box"))
        obj.addProperty('App::PropertyLength', 'Width', 'Stock', QtCore.QT_TRANSLATE_NOOP("PathStock", "Width of this stock box"))
        obj.addProperty('App::PropertyLength', 'Height', 'Stock', QtCore.QT_TRANSLATE_NOOP("PathStock", "Height of this stock box"))

        obj.Length = 10
        obj.Width  = 10
        obj.Height = 10

        obj.Proxy = self

    def __getstate__(self):
        return None
    def __setstate__(self, state):
        return None

    def execute(self, obj):
        if obj.Length < self.MinExtent:
            obj.Length = self.MinExtent
        if obj.Width < self.MinExtent:
            obj.Width = self.MinExtent
        if obj.Height < self.MinExtent:
            obj.Height = self.MinExtent

        shape = Part.makeBox(obj.Length, obj.Width, obj.Height)
        shape.Placement = obj.Placement
        obj.Shape = shape

    def onChanged(self, obj, prop):
        if prop in ['Length', 'Width', 'Height'] and not 'Restore' in obj.State:
            self.execute(obj)

class StockCreateCylinder:
    MinExtent = 0.001

    def __init__(self, obj):
        obj.addProperty('App::PropertyLength', 'Radius', 'Stock', QtCore.QT_TRANSLATE_NOOP("PathStock", "Radius of this stock cylinder"))
        obj.addProperty('App::PropertyLength', 'Height', 'Stock', QtCore.QT_TRANSLATE_NOOP("PathStock", "Height of this stock cylinder"))

        obj.Radius = 2
        obj.Height = 10

        obj.Proxy = self

    def __getstate__(self):
        return None
    def __setstate__(self, state):
        return None

    def execute(self, obj):
        if obj.Radius < self.MinExtent:
            obj.Radius = self.MinExtent
        if obj.Height < self.MinExtent:
            obj.Height = self.MinExtent

        shape = Part.makeCylinder(obj.Radius, obj.Height)
        shape.Placement = obj.Placement
        obj.Shape = shape

    def onChanged(self, obj, prop):
        if prop in ['Radius', 'Height'] and not 'Restore' in obj.State:
            self.execute(obj)

def SetupStockObject(obj, stockType):
    if FreeCAD.GuiUp and obj.ViewObject:
        obj.addProperty('App::PropertyString', 'StockType', 'Stock', QtCore.QT_TRANSLATE_NOOP("PathStock", "Internal representation of stock type"))
        obj.StockType = stockType
        obj.setEditorMode('StockType', 2) # hide

        PathIconViewProvider.ViewProvider(obj.ViewObject, 'Stock')
        obj.ViewObject.Transparency = 90
        obj.ViewObject.DisplayMode = 'Wireframe'

def CreateFromBase(job, neg=None, pos=None, placement=None):
    obj = FreeCAD.ActiveDocument.addObject('Part::FeaturePython', 'Stock')
    # don't want to use the resrouce clone - we want the real object so 
    # Base and Stock can be placed independently
    proxy = StockFromBase(obj, job.Proxy.baseObject(job), job.Base.Placement)
    if neg:
        obj.ExtXneg = neg.x
        obj.ExtYneg = neg.y
        obj.ExtZneg = neg.z
    if pos:
        obj.ExtXpos = pos.x
        obj.ExtYpos = pos.y
        obj.ExtZpos = pos.z
    if placement:
        obj.Placement = placement
    SetupStockObject(obj, StockType.FromBase)
    proxy.execute(obj)
    obj.purgeTouched()
    return obj

def CreateBox(job, extent=None, placement=None):
    base = job.Base if job and hasattr(job, 'Base') else None
    obj = FreeCAD.ActiveDocument.addObject('Part::FeaturePython', 'Stock')
    proxy = StockCreateBox(obj)
    if extent:
        obj.Length = extent.x
        obj.Width  = extent.y
        obj.Height = extent.z
    elif base:
        bb = shapeBoundBox(base)
        obj.Length = max(bb.XLength, 1)
        obj.Width  = max(bb.YLength, 1)
        obj.Height = max(bb.ZLength, 1)
    if placement:
        obj.Placement = placement
    elif base:
        bb = shapeBoundBox(base)
        origin = FreeCAD.Vector(bb.XMin, bb.YMin, bb.ZMin)
        obj.Placement = FreeCAD.Placement(origin, FreeCAD.Vector(), 0)
    SetupStockObject(obj, StockType.CreateBox)
    return obj

def CreateCylinder(job, radius=None, height=None, placement=None):
    base = job.Base if job and hasattr(job, 'Base') else None
    obj = FreeCAD.ActiveDocument.addObject('Part::FeaturePython', 'Stock')
    proxy = StockCreateCylinder(obj)
    if radius:
        obj.Radius = radius
    if height:
        obj.Height = height
    elif base:
        bb = shapeBoundBox(base)
        obj.Radius = math.sqrt(bb.XLength ** 2 + bb.YLength ** 2) / 2.0
        obj.Height = max(bb.ZLength, 1)
    if placement:
        obj.Placement = placement
    elif base:
        bb = shapeBoundBox(base)
        origin = FreeCAD.Vector((bb.XMin + bb.XMax)/2, (bb.YMin + bb.YMax)/2, bb.ZMin)
        obj.Placement = FreeCAD.Placement(origin, FreeCAD.Vector(), 0)
    SetupStockObject(obj, StockType.CreateCylinder)
    return obj

def TemplateAttributes(stock, includeExtent=True, includePlacement=True):
    attrs = {}
    if stock:
        attrs['version'] = 1
        stockType = StockType.FromStock(stock)
        attrs['create'] = stockType

        if includeExtent:
            if stockType == StockType.FromBase:
                attrs['xneg'] = ("%s" % stock.ExtXneg)
                attrs['xpos'] = ("%s" % stock.ExtXpos)
                attrs['yneg'] = ("%s" % stock.ExtYneg)
                attrs['ypos'] = ("%s" % stock.ExtYpos)
                attrs['zneg'] = ("%s" % stock.ExtZneg)
                attrs['zpos'] = ("%s" % stock.ExtZpos)
            if stockType == StockType.CreateBox:
                attrs['length'] = ("%s" % stock.Length)
                attrs['width']  = ("%s" % stock.Width)
                attrs['height'] = ("%s" % stock.Height)
            if stockType == StockType.CreateCylinder:
                attrs['radius'] = ("%s" % stock.Radius)
                attrs['height'] = ("%s" % stock.Height)

        if includePlacement:
            pos = stock.Placement.Base
            attrs['posX'] = pos.x
            attrs['posY'] = pos.y
            attrs['posZ'] = pos.z
            rot = stock.Placement.Rotation
            attrs['rotX'] = rot.Q[0]
            attrs['rotY'] = rot.Q[1]
            attrs['rotZ'] = rot.Q[2]
            attrs['rotW'] = rot.Q[3]

    return attrs

def CreateFromTemplate(job, template):
    if template.get('version') and 1 == int(template['version']):
        stockType = template.get('create')
        if stockType:
            placement = None
            posX = template.get('posX')
            posY = template.get('posY')
            posZ = template.get('posZ')
            rotX = template.get('rotX')
            rotY = template.get('rotY')
            rotZ = template.get('rotZ')
            rotW = template.get('rotW')
            if posX is not None and posY is not None and posZ is not None and rotX is not None and rotY is not None and rotZ is not None and rotW is not None:
                pos = FreeCAD.Vector(float(posX), float(posY), float(posZ)) 
                rot = FreeCAD.Rotation(float(rotX), float(rotY), float(rotZ), float(rotW))
                placement = FreeCAD.Placement(pos, rot)
            elif posX is not None or posY is not None or posZ is not None or rotX is not None or rotY is not None or rotZ is not None or rotW is not None:
                PathLog.warning(translate('PathStock', 'Corrupted or incomplete placement information in template - ignoring'))

            if stockType == StockType.FromBase:
                xneg = template.get('xneg')
                xpos = template.get('xpos')
                yneg = template.get('yneg')
                ypos = template.get('ypos')
                zneg = template.get('zneg')
                zpos = template.get('zpos')
                neg = None
                pos = None
                if xneg is not None and xpos is not None and yneg is not None and ypos is not None and zneg is not None and zpos is not None:
                    neg = FreeCAD.Vector(FreeCAD.Units.Quantity(xneg).Value, FreeCAD.Units.Quantity(yneg).Value, FreeCAD.Units.Quantity(zneg).Value)
                    pos = FreeCAD.Vector(FreeCAD.Units.Quantity(xpos).Value, FreeCAD.Units.Quantity(ypos).Value, FreeCAD.Units.Quantity(zpos).Value)
                elif xneg is not None or xpos is not None or yneg is not None or ypos is not None or zneg is not None or zpos is not None:
                    PathLog.error(translate('PathStock', 'Corrupted or incomplete specification for creating stock from base - ignoring extent'))
                return CreateFromBase(job, neg, pos, placement)

            if stockType == StockType.CreateBox:
                length = template.get('length')
                width  = template.get('width')
                height = template.get('height')
                extent = None
                if length is not None and width is not None and height is not None:
                    extent = FreeCAD.Vector(FreeCAD.Units.Quantity(length).Value, FreeCAD.Units.Quantity(width).Value, FreeCAD.Units.Quantity(height).Value)
                elif length is not None or width is not None or height is not None:
                    PathLog.error(translate('PathStock', 'Corrupted or incomplete size for creating a stock box - ignoring size'))
                return CreateBox(job, extent, placement)

            if stockType == StockType.CreateCylinder:
                radius = template.get('radius')
                height = template.get('height')
                if radius is not None and height is not None:
                    pass
                elif radius is not None or height is not None:
                    radius = None
                    height = None
                    PathLog.error(translate('PathStock', 'Corrupted or incomplete size for creating a stock cylinder - ignoring size'))
                return CreateCylinder(job, radius, height, placement)

            PathLog.error(translate('PathStock', 'Unsupported stock type named {}').format(stockType))
        else:
            PathLog.error(translate('PathStock', 'Unsupported PathStock template version {}').format(template.get('version')))
        return None


FreeCAD.Console.PrintLog("Loading PathStock... done\n")
