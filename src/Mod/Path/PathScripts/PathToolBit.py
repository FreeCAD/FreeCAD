# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathSetupSheetOpPrototype as PathSetupSheetOpPrototype
import PathScripts.PathUtil as PathUtil
import PySide
import Sketcher
import json
import math
import os
import zipfile

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Part = LazyLoader('Part', globals(), 'Part')

__title__ = "Tool bits."
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Class to deal with and represent a tool bit."

# PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
# PathLog.trackModule()


def translate(context, text, disambig=None):
    return PySide.QtCore.QCoreApplication.translate(context, text, disambig)


ParameterTypeConstraint = {
        'Angle':        'App::PropertyAngle',
        'Distance':     'App::PropertyLength',
        'DistanceX':    'App::PropertyLength',
        'DistanceY':    'App::PropertyLength',
        'Radius':       'App::PropertyLength'}


def _findTool(path, typ, dbg=False):
    if os.path.exists(path):  # absolute reference
        if dbg:
            PathLog.debug("Found {} at {}".format(typ, path))
        return path

    def searchFor(pname, fname):
        # PathLog.debug("pname: {} fname: {}".format(pname, fname))
        if dbg:
            PathLog.debug("Looking for {}".format(pname))
        if fname:
            for p in PathPreferences.searchPathsTool(typ):
                PathLog.track(p)
                f = os.path.join(p, fname)
                if dbg:
                    PathLog.debug("  Checking {}".format(f))
                if os.path.exists(f):
                    if dbg:
                        PathLog.debug("  Found {} at {}".format(typ, f))
                    return f
        if pname and os.path.sep != pname:
            PathLog.track(pname)
            ppname, pfname = os.path.split(pname)
            ffname = os.path.join(pfname, fname) if fname else pfname
            return searchFor(ppname, ffname)
        return None

    return searchFor(path, '')


def findShape(path):
    '''
    findShape(path) ... search for path, full and partially
    in all known shape directories.
    '''
    return _findTool(path, 'Shape')


def findBit(path):
    PathLog.track(path)
    if path.endswith('.fctb'):
        return _findTool(path, 'Bit')
    return _findTool("{}.fctb".format(path), 'Bit')


def findLibrary(path, dbg=False):
    if path.endswith('.fctl'):
        return _findTool(path, 'Library', dbg)
    return _findTool("{}.fctl".format(path), 'Library', dbg)


def _findRelativePath(path, typ):
    relative = path
    for p in PathPreferences.searchPathsTool(typ):
        if path.startswith(p):
            p = path[len(p):]
            if os.path.sep == p[0]:
                p = p[1:]
            if len(p) < len(relative):
                relative = p
    return relative


def findRelativePathShape(path):
    return _findRelativePath(path, 'Shape')


def findRelativePathTool(path):
    return _findRelativePath(path, 'Bit')


def findRelativePathLibrary(path):
    return _findRelativePath(path, 'Library')


def updateConstraint(sketch, name, value):
    for i, constraint in enumerate(sketch.Constraints):
        if constraint.Name.split(';')[0] == name:
            constr = None
            if constraint.Type in ['DistanceX', 'DistanceY', 'Distance', 'Radius', 'Angle']:
                constr = Sketcher.Constraint(constraint.Type, constraint.First, constraint.FirstPos, constraint.Second, constraint.SecondPos, value)
            else:
                print(constraint.Name, constraint.Type)

            if constr is not None:
                if not PathGeom.isRoughly(constraint.Value, value.Value):
                    PathLog.track(name, constraint.Type,
                                  'update', i, "(%.2f -> %.2f)"
                                  % (constraint.Value, value.Value))
                    sketch.delConstraint(i)
                    sketch.recompute()
                    n = sketch.addConstraint(constr)
                    sketch.renameConstraint(n, constraint.Name)
                else:
                    PathLog.track(name, constraint.Type, 'unchanged')
            break


PropertyGroupBit       = 'Bit'
PropertyGroupAttribute = 'Attribute'


class ToolBit(object):

    def __init__(self, obj, shapeFile):
        PathLog.track(obj.Label, shapeFile)
        self.obj = obj
        obj.addProperty('App::PropertyFile', 'BitShape', 'Base',
                        translate('PathToolBit', 'Shape for bit shape'))
        obj.addProperty('App::PropertyLink', 'BitBody', 'Base',
                        translate('PathToolBit',
                        'The parametrized body representing the tool bit'))
        obj.addProperty('App::PropertyFile', 'File', 'Base',
                        translate('PathToolBit', 'The file of the tool'))
        obj.addProperty('App::PropertyString', 'ShapeName', 'Base',
                        translate('PathToolBit', 'The name of the shape file'))
        if shapeFile is None:
            obj.BitShape = 'endmill.fcstd'
            self._setupBitShape(obj)
            self.unloadBitBody(obj)
        else:
            obj.BitShape = shapeFile
            self._setupBitShape(obj)
        self.onDocumentRestored(obj)

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        for obj in FreeCAD.ActiveDocument.Objects:
            if hasattr(obj, 'Proxy') and obj.Proxy == self:
                self.obj = obj
                break
        return None

    def propertyNamesBit(self, obj):
        return [prop for prop in obj.PropertiesList if obj.getGroupOfProperty(prop) == PropertyGroupBit]

    def propertyNamesAttribute(self, obj):
        return [prop for prop in obj.PropertiesList if obj.getGroupOfProperty(prop) == PropertyGroupAttribute]

    def onDocumentRestored(self, obj):
        # when files are shared it is essential to be able to change/set the shape file,
        # otherwise the file is hard to use
        # obj.setEditorMode('BitShape', 1)
        obj.setEditorMode('BitBody', 2)
        obj.setEditorMode('File', 1)
        obj.setEditorMode('Shape', 2)

        for prop in self.propertyNamesBit(obj):
            obj.setEditorMode(prop, 1)
        # I currently don't see why these need to be read-only
        # for prop in self.propertyNamesAttribute(obj):
        #    obj.setEditorMode(prop, 1)

    def onChanged(self, obj, prop):
        PathLog.track(obj.Label, prop)
        if prop == 'BitShape' and 'Restore' not in obj.State:
            self._setupBitShape(obj)
        # elif obj.getGroupOfProperty(prop) == PropertyGroupBit:
        #    self._updateBitShape(obj, [prop])

    def onDelete(self, obj, arg2=None):
        PathLog.track(obj.Label)
        self.unloadBitBody(obj)
        obj.Document.removeObject(obj.Name)

    def _updateBitShape(self, obj, properties=None):
        if obj.BitBody is not None:
            if not properties:
                properties = self.propertyNamesBit(obj)
            for prop in properties:
                for sketch in [o for o in obj.BitBody.Group if o.TypeId == 'Sketcher::SketchObject']:
                    PathLog.track(obj.Label, sketch.Label, prop)
                    updateConstraint(sketch, prop, obj.getPropertyByName(prop))
            self._copyBitShape(obj)

    def _copyBitShape(self, obj):
        obj.Document.recompute()
        if obj.BitBody and obj.BitBody.Shape:
            obj.Shape = obj.BitBody.Shape
        else:
            obj.Shape = Part.Shape()

    def _loadBitBody(self, obj, path=None):
        p = path if path else obj.BitShape
        docOpened = False
        doc = None
        for d in FreeCAD.listDocuments():
            if FreeCAD.getDocument(d).FileName == p:
                doc = FreeCAD.getDocument(d)
                break
        if doc is None:
            p = findShape(p)
            if not path and p != obj.BitShape:
                obj.BitShape = p
            doc = FreeCAD.openDocument(p, True)
            obj.ShapeName = doc.Name
            docOpened = True
        return (doc, docOpened)

    def _removeBitBody(self, obj):
        if obj.BitBody:
            obj.BitBody.removeObjectsFromDocument()
            obj.Document.removeObject(obj.BitBody.Name)
            obj.BitBody = None

    def _deleteBitSetup(self, obj):
        PathLog.track(obj.Label)
        self._removeBitBody(obj)
        self._copyBitShape(obj)
        for prop in self.propertyNamesBit(obj):
            obj.removeProperty(prop)

    def loadBitBody(self, obj, force=False):
        if force or not obj.BitBody:
            activeDoc = FreeCAD.ActiveDocument
            if force:
                self._removeBitBody(obj)
            (doc, opened) = self._loadBitBody(obj)
            obj.BitBody = obj.Document.copyObject(doc.RootObjects[0], True)
            if opened:
                FreeCAD.setActiveDocument(activeDoc.Name)
                FreeCAD.closeDocument(doc.Name)
            self._updateBitShape(obj)

    def unloadBitBody(self, obj):
        self._removeBitBody(obj)

    def _setupBitShape(self, obj, path=None):
        PathLog.track(obj.Label)

        activeDoc = FreeCAD.ActiveDocument
        (doc, docOpened) = self._loadBitBody(obj, path)

        obj.Label = doc.RootObjects[0].Label
        self._deleteBitSetup(obj)
        bitBody = obj.Document.copyObject(doc.RootObjects[0], True)
        if docOpened:
            FreeCAD.setActiveDocument(activeDoc.Name)
            FreeCAD.closeDocument(doc.Name)

        if bitBody.ViewObject:
            bitBody.ViewObject.Visibility = False

        for sketch in [o for o in bitBody.Group if o.TypeId == 'Sketcher::SketchObject']:
            for constraint in [c for c in sketch.Constraints if c.Name != '']:
                typ = ParameterTypeConstraint.get(constraint.Type)
                PathLog.track(constraint, typ)
                if typ is not None:
                    parts = [p.strip() for p in constraint.Name.split(';')]
                    prop = parts[0]
                    desc = ''
                    if len(parts) > 1:
                        desc = parts[1]
                    obj.addProperty(typ, prop, PropertyGroupBit, desc)
                    obj.setEditorMode(prop, 1)
                    value = constraint.Value
                    if constraint.Type == 'Angle':
                        value = value * 180 / math.pi
                    PathUtil.setProperty(obj, prop, value)
        # has to happen last because it could trigger op.execute evaluations
        obj.BitBody = bitBody
        self._copyBitShape(obj)

    def getBitThumbnail(self, obj):
        if obj.BitShape:
            path = findShape(obj.BitShape)
            if path:
                with open(path, 'rb') as fd:
                    try:
                        zf = zipfile.ZipFile(fd)
                        pf = zf.open('thumbnails/Thumbnail.png', 'r')
                        data = pf.read()
                        pf.close()
                        return data
                    except KeyError:
                        pass
        return None

    def saveToFile(self, obj, path, setFile=True):
        PathLog.track(path)
        try:
            with open(path, 'w') as fp:
                json.dump(self.templateAttrs(obj), fp, indent='  ')
            if setFile:
                obj.File = path
            return True
        except (OSError, IOError) as e:
            PathLog.error("Could not save tool {} to {} ({})".format(
                          obj.Label, path, e))
            raise

    def templateAttrs(self, obj):
        attrs = {}
        attrs['version'] = 2  # Path.Tool is version 1
        attrs['name'] = obj.Label
        if PathPreferences.toolsStoreAbsolutePaths():
            attrs['shape'] = obj.BitShape
        else:
            attrs['shape'] = findRelativePathShape(obj.BitShape)
        params = {}
        for name in self.propertyNamesBit(obj):
            params[name] = PathUtil.getPropertyValueString(obj, name)
        attrs['parameter'] = params
        params = {}
        for name in self.propertyNamesAttribute(obj):
            if name == "UserAttributes":
                for key, value in obj.UserAttributes.items():
                    params[key] = value
            else:
                params[name] = PathUtil.getPropertyValueString(obj, name)
        attrs['attribute'] = params
        return attrs


def Declaration(path):
    PathLog.track(path)
    with open(path, 'r') as fp:
        return json.load(fp)


class AttributePrototype(PathSetupSheetOpPrototype.OpPrototype):

    def __init__(self):
        PathSetupSheetOpPrototype.OpPrototype.__init__(self, 'ToolBitAttribute')
        self.addProperty('App::PropertyEnumeration', 'Material',
                PropertyGroupAttribute,
                translate('PathToolBit', 'Tool bit material'))
        self.Material = ['Carbide', 'CastAlloy', 'Ceramics', 'Diamond',
                'HighCarbonToolSteel', 'HighSpeedSteel', 'Sialon']
        self.addProperty('App::PropertyDistance', 'LengthOffset',
                PropertyGroupAttribute, translate('PathToolBit',
                'Length offset in Z direction'))
        self.addProperty('App::PropertyInteger', 'Flutes',
                PropertyGroupAttribute, translate('PathToolBit',
                'The number of flutes'))
        self.addProperty('App::PropertyDistance', 'ChipLoad',
                PropertyGroupAttribute, translate('PathToolBit',
                'Chipload as per manufacturer'))
        self.addProperty('App::PropertyMap', 'UserAttributes',
                PropertyGroupAttribute, translate('PathToolBit',
                'User Defined Values'))
        self.addProperty('App::PropertyBool', 'SpindlePower',
                PropertyGroupAttribute, translate('PathToolBit',
                'Whether Spindle Power should be allowed'))


class ToolBitFactory(object):

    def CreateFromAttrs(self, attrs, name='ToolBit'):
        PathLog.debug(attrs)
        obj = Factory.Create(name, attrs['shape'])
        obj.Label = attrs['name']
        params = attrs['parameter']
        for prop in params:
            PathUtil.setProperty(obj, prop, params[prop])
        obj.Proxy._updateBitShape(obj)
        obj.Proxy.unloadBitBody(obj)
        params = attrs['attribute']
        proto = AttributePrototype()
        uservals = {}
        for pname in params:
            try:
                prop = proto.getProperty(pname)
                prop.setupProperty(obj, pname, PropertyGroupAttribute, prop.valueFromString(params[pname]))
            except Exception:
                prop = proto.getProperty("UserAttributes")
                uservals.update({pname: params[pname]})

        if len(uservals.items()) > 0:
            prop.setupProperty(obj, "UserAttributes",
                               PropertyGroupAttribute, uservals)

        return obj

    def CreateFrom(self, path, name='ToolBit'):
        try:
            data = Declaration(path)
            bit = Factory.CreateFromAttrs(data, name)
            bit.File = path
            return bit
        except (OSError, IOError) as e:
            PathLog.error("%s not a valid tool file (%s)" % (path, e))
            raise

    def Create(self, name='ToolBit', shapeFile=None):
        obj = FreeCAD.ActiveDocument.addObject('Part::FeaturePython', name)
        obj.Proxy = ToolBit(obj, shapeFile)
        return obj


Factory = ToolBitFactory()
