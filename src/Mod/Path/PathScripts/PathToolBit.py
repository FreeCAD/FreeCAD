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
import PathScripts.PathPropertyBag as PathPropertyBag
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

PropertyGroupShape = 'Shape'

_DebugFindTool = False

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
#PathLog.trackModule()


def translate(context, text, disambig=None):
    return PySide.QtCore.QCoreApplication.translate(context, text, disambig)

def _findTool(path, typ, dbg=_DebugFindTool):
    PathLog.track(path)
    if os.path.exists(path):  # absolute reference
        if dbg:
            PathLog.debug("Found {} at {}".format(typ, path))
        return path

    def searchFor(pname, fname):
        # PathLog.debug("pname: {} fname: {}".format(pname, fname))
        if dbg:
            PathLog.debug("Looking for {} in {}".format(pname, fname))
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

class ToolBit(object):

    def __init__(self, obj, shapeFile):
        PathLog.track(obj.Label, shapeFile)
        self.obj = obj
        obj.addProperty('App::PropertyFile', 'BitShape', 'Base', translate('PathToolBit', 'Shape for bit shape'))
        obj.addProperty('App::PropertyLink', 'BitBody', 'Base', translate('PathToolBit', 'The parametrized body representing the tool bit'))
        obj.addProperty('App::PropertyFile', 'File', 'Base', translate('PathToolBit', 'The file of the tool'))
        obj.addProperty('App::PropertyString', 'ShapeName', 'Base', translate('PathToolBit', 'The name of the shape file'))
        obj.addProperty('App::PropertyStringList', 'BitPropertyNames', 'Base', translate('PathToolBit', 'List of all properties inherited from the bit'))

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

    def onDocumentRestored(self, obj):
        # when files are shared it is essential to be able to change/set the shape file,
        # otherwise the file is hard to use
        # obj.setEditorMode('BitShape', 1)
        obj.setEditorMode('BitBody', 2)
        obj.setEditorMode('File', 1)
        obj.setEditorMode('Shape', 2)
        obj.setEditorMode('BitPropertyNames', 2)

        for prop in obj.BitPropertyNames:
            if obj.getGroupOfProperty(prop) == PropertyGroupShape:
                # properties in the Shape group can only be modified while the actual
                # shape is loaded, so we have to disable direct property editing
                obj.setEditorMode(prop, 1)
            else:
                # all other custom properties can and should be edited directly in the
                # property editor widget, not much value in re-implementing that
                obj.setEditorMode(prop, 0)

    def onChanged(self, obj, prop):
        PathLog.track(obj.Label, prop)
        if prop == 'BitShape' and 'Restore' not in obj.State:
            self._setupBitShape(obj)

    def onDelete(self, obj, arg2=None):
        PathLog.track(obj.Label)
        self.unloadBitBody(obj)
        obj.Document.removeObject(obj.Name)

    def _updateBitShape(self, obj, properties=None):
        if obj.BitBody is not None:
            for attributes in [o for o in obj.BitBody.Group if hasattr(o, 'Proxy') and hasattr(o.Proxy, 'getCustomProperties')]:
                for prop in attributes.Proxy.getCustomProperties():
                    setattr(attributes, prop, obj.getPropertyByName(prop))
            self._copyBitShape(obj)

    def _copyBitShape(self, obj):
        obj.Document.recompute()
        if obj.BitBody and obj.BitBody.Shape:
            obj.Shape = obj.BitBody.Shape
        else:
            obj.Shape = Part.Shape()

    def _loadBitBody(self, obj, path=None):
        PathLog.track(obj.Label, path)
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
            PathLog.debug("ToolBit {} using shape file: {}".format(obj.Label, p))
            doc = FreeCAD.openDocument(p, True)
            obj.ShapeName = doc.Name
            docOpened = True
        else:
            PathLog.debug("ToolBit {} already open: {}".format(obj.Label, doc))
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
        for prop in obj.BitPropertyNames:
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

        docName = doc.Name
        if docOpened:
            FreeCAD.setActiveDocument(activeDoc.Name)
            FreeCAD.closeDocument(doc.Name)

        if bitBody.ViewObject:
            bitBody.ViewObject.Visibility = False

        PathLog.debug("bitBody.{} ({}): {}".format(bitBody.Label, bitBody.Name, type(bitBody)))

        propNames = []
        for attributes in [o for o in bitBody.Group if PathPropertyBag.IsPropertyBag(o)]:
            PathLog.debug("Process properties from {}".format(attributes.Label))
            for prop in attributes.Proxy.getCustomProperties():
                # extract property parameters and values so it can be copied
                src = attributes.getPropertyByName(prop)
                typ = PathPropertyBag.getPropertyType(src)
                grp = attributes.getGroupOfProperty(prop)
                dsc = attributes.getDocumentationOfProperty(prop)

                obj.addProperty(typ, prop, grp, dsc)
                obj.setEditorMode(prop, 1)
                PathUtil.setProperty(obj, prop, src)
                propNames.append(prop)
        if not propNames:
            PathLog.error(translate('PathToolBit', 'Did not find a PropertyBag in {} - not a ToolBit shape?').format(docName))

        # has to happen last because it could trigger op.execute evaluations
        obj.BitPropertyNames = propNames
        obj.BitBody = bitBody
        self._copyBitShape(obj)

    def toolShapeProperties(self, obj):
        '''toolShapeProperties(obj) ... return all properties defining it's shape'''
        return sorted([prop for prop in obj.BitPropertyNames if obj.getGroupOfProperty(prop) == PropertyGroupShape])

    def toolAdditionalProperties(self, obj):
        '''toolShapeProperties(obj) ... return all properties unrelated to it's shape'''
        return sorted([prop for prop in obj.BitPropertyNames if obj.getGroupOfProperty(prop) != PropertyGroupShape])

    def toolGroupsAndProperties(self, obj, includeShape=True):
        '''toolGroupsAndProperties(obj) ... returns a dictionary of group names with a list of property names.'''
        category = {}
        for prop in obj.BitPropertyNames:
            group = obj.getGroupOfProperty(prop)
            if includeShape or group != PropertyGroupShape:
                properties  = category.get(group, [])
                properties.append(prop)
                category[group] = properties
        return category

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
            PathLog.error("Could not save tool {} to {} ({})".format(obj.Label, path, e))
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
        for name in obj.BitPropertyNames:
            params[name] = PathUtil.getPropertyValueString(obj, name)
        attrs['parameter'] = params
        params = {}
        attrs['attribute'] = params
        return attrs


def Declaration(path):
    PathLog.track(path)
    with open(path, 'r') as fp:
        return json.load(fp)


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
        return obj

    def CreateFrom(self, path, name='ToolBit'):
        PathLog.track(name, path)
        try:
            data = Declaration(path)
            bit = Factory.CreateFromAttrs(data, name)
            bit.File = path
            return bit
        except (OSError, IOError) as e:
            PathLog.error("%s not a valid tool file (%s)" % (path, e))
            raise

    def Create(self, name='ToolBit', shapeFile=None):
        PathLog.track(name, shapeFile)
        obj = FreeCAD.ActiveDocument.addObject('Part::FeaturePython', name)
        obj.Proxy = ToolBit(obj, shapeFile)
        return obj


Factory = ToolBitFactory()
