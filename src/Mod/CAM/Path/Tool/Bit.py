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
import Path
import Path.Base.Util as PathUtil
import Path.Base.PropertyBag as PathPropertyBag
import json
import os
import zipfile
from PySide.QtCore import QT_TRANSLATE_NOOP

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

__title__ = "Tool bits."
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Class to deal with and represent a tool bit."

PropertyGroupShape = "Shape"

_DebugFindTool = False


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def _findToolFile(name, containerFile, typ):
    Path.Log.track(name)
    if os.path.exists(name):  # absolute reference
        return name

    if containerFile:
        rootPath = os.path.dirname(os.path.dirname(containerFile))
        paths = [os.path.join(rootPath, typ)]
    else:
        paths = []
    paths.extend(Path.Preferences.searchPathsTool(typ))

    def _findFile(path, name):
        Path.Log.track(path, name)
        fullPath = os.path.join(path, name)
        if os.path.exists(fullPath):
            return (True, fullPath)
        for root, ds, fs in os.walk(path):
            for d in ds:
                found, fullPath = _findFile(d, name)
                if found:
                    return (True, fullPath)
        return (False, None)

    for p in paths:
        found, path = _findFile(p, name)
        if found:
            return path
    return None


def findToolShape(name, path=None):
    """findToolShape(name, path) ... search for name, if relative path look in path"""
    Path.Log.track(name, path)
    return _findToolFile(name, path, "Shape")


def findToolBit(name, path=None):
    """findToolBit(name, path) ... search for name, if relative path look in path"""
    Path.Log.track(name, path)
    if name.endswith(".fctb"):
        return _findToolFile(name, path, "Bit")
    return _findToolFile("{}.fctb".format(name), path, "Bit")


# Only used in ToolBit unit test module: TestPathToolBit.py
def findToolLibrary(name, path=None):
    """findToolLibrary(name, path) ... search for name, if relative path look in path"""
    Path.Log.track(name, path)
    if name.endswith(".fctl"):
        return _findToolFile(name, path, "Library")
    return _findToolFile("{}.fctl".format(name), path, "Library")


def _findRelativePath(path, typ):
    Path.Log.track(path, typ)
    relative = path
    for p in Path.Preferences.searchPathsTool(typ):
        if path.startswith(p):
            p = path[len(p) :]
            if os.path.sep == p[0]:
                p = p[1:]
            if len(p) < len(relative):
                relative = p
    return relative


# Unused due to bug fix related to relative paths
"""
def findRelativePathShape(path):
    return _findRelativePath(path, 'Shape')


def findRelativePathTool(path):
    return _findRelativePath(path, 'Bit')
"""


def findRelativePathLibrary(path):
    return _findRelativePath(path, "Library")


class ToolBit(object):
    def __init__(self, obj, shapeFile, path=None):
        Path.Log.track(obj.Label, shapeFile, path)
        self.obj = obj
        obj.addProperty(
            "App::PropertyFile",
            "BitShape",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "Shape for bit shape"),
        )
        obj.addProperty(
            "App::PropertyLink",
            "BitBody",
            "Base",
            QT_TRANSLATE_NOOP(
                "App::Property", "The parametrized body representing the tool bit"
            ),
        )
        obj.addProperty(
            "App::PropertyFile",
            "File",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "The file of the tool"),
        )
        obj.addProperty(
            "App::PropertyString",
            "ShapeName",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "The name of the shape file"),
        )
        obj.addProperty(
            "App::PropertyStringList",
            "BitPropertyNames",
            "Base",
            QT_TRANSLATE_NOOP(
                "App::Property", "List of all properties inherited from the bit"
            ),
        )

        if path:
            obj.File = path
        if shapeFile is None:
            obj.BitShape = "endmill.fcstd"
            self._setupBitShape(obj)
            self.unloadBitBody(obj)
        else:
            obj.BitShape = shapeFile
            self._setupBitShape(obj)
        self.onDocumentRestored(obj)

    def dumps(self):
        return None

    def loads(self, state):
        for obj in FreeCAD.ActiveDocument.Objects:
            if hasattr(obj, "Proxy") and obj.Proxy == self:
                self.obj = obj
                break
        return None

    def onDocumentRestored(self, obj):
        # when files are shared it is essential to be able to change/set the shape file,
        # otherwise the file is hard to use
        # obj.setEditorMode('BitShape', 1)
        obj.setEditorMode("BitBody", 2)
        obj.setEditorMode("File", 1)
        obj.setEditorMode("Shape", 2)
        if not hasattr(obj, "BitPropertyNames"):
            obj.addProperty(
                "App::PropertyStringList",
                "BitPropertyNames",
                "Base",
                QT_TRANSLATE_NOOP(
                    "App::Property", "List of all properties inherited from the bit"
                ),
            )
            propNames = []
            for prop in obj.PropertiesList:
                if obj.getGroupOfProperty(prop) == "Bit":
                    val = obj.getPropertyByName(prop)
                    typ = obj.getTypeIdOfProperty(prop)
                    dsc = obj.getDocumentationOfProperty(prop)

                    obj.removeProperty(prop)
                    obj.addProperty(typ, prop, PropertyGroupShape, dsc)

                    PathUtil.setProperty(obj, prop, val)
                    propNames.append(prop)
                elif obj.getGroupOfProperty(prop) == "Attribute":
                    propNames.append(prop)
            obj.BitPropertyNames = propNames
        obj.setEditorMode("BitPropertyNames", 2)

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
        Path.Log.track(obj.Label, prop)
        if prop == "BitShape" and "Restore" not in obj.State:
            self._setupBitShape(obj)

    def onDelete(self, obj, arg2=None):
        Path.Log.track(obj.Label)
        self.unloadBitBody(obj)
        obj.Document.removeObject(obj.Name)

    def _updateBitShape(self, obj, properties=None):
        if obj.BitBody is not None:
            for attributes in [
                o
                for o in obj.BitBody.Group
                if hasattr(o, "Proxy") and hasattr(o.Proxy, "getCustomProperties")
            ]:
                for prop in attributes.Proxy.getCustomProperties():
                    # the property might not exist in our local object (new attribute in shape)
                    # for such attributes we just keep the default
                    if hasattr(obj, prop):
                        setattr(attributes, prop, obj.getPropertyByName(prop))
                    else:
                        # if the template shape has a new attribute defined we should add that
                        # to the local object
                        self._setupProperty(obj, prop, attributes)
                        propNames = obj.BitPropertyNames
                        propNames.append(prop)
                        obj.BitPropertyNames = propNames
            self._copyBitShape(obj)

    def _copyBitShape(self, obj):
        obj.Document.recompute()
        if obj.BitBody and obj.BitBody.Shape:
            obj.Shape = obj.BitBody.Shape
        else:
            obj.Shape = Part.Shape()

    def _loadBitBody(self, obj, path=None):
        Path.Log.track(obj.Label, path)
        p = path if path else obj.BitShape
        docOpened = False
        doc = None
        for d in FreeCAD.listDocuments():
            if FreeCAD.getDocument(d).FileName == p:
                doc = FreeCAD.getDocument(d)
                break
        if doc is None:
            p = findToolShape(p, path if path else obj.File)
            if p is None:
                raise FileNotFoundError

            if not path and p != obj.BitShape:
                obj.BitShape = p
            Path.Log.debug("ToolBit {} using shape file: {}".format(obj.Label, p))
            doc = FreeCAD.openDocument(p, True)
            obj.ShapeName = doc.Name
            docOpened = True
        else:
            Path.Log.debug("ToolBit {} already open: {}".format(obj.Label, doc))
        return (doc, docOpened)

    def _removeBitBody(self, obj):
        if obj.BitBody:
            obj.BitBody.removeObjectsFromDocument()
            obj.Document.removeObject(obj.BitBody.Name)
            obj.BitBody = None

    def _deleteBitSetup(self, obj):
        Path.Log.track(obj.Label)
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

    def _setupProperty(self, obj, prop, orig):
        # extract property parameters and values so it can be copied
        val = orig.getPropertyByName(prop)
        typ = orig.getTypeIdOfProperty(prop)
        grp = orig.getGroupOfProperty(prop)
        dsc = orig.getDocumentationOfProperty(prop)

        obj.addProperty(typ, prop, grp, dsc)
        if "App::PropertyEnumeration" == typ:
            setattr(obj, prop, orig.getEnumerationsOfProperty(prop))

        obj.setEditorMode(prop, 1)
        PathUtil.setProperty(obj, prop, val)

    def _setupBitShape(self, obj, path=None):
        Path.Log.track(obj.Label)

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

        Path.Log.debug(
            "bitBody.{} ({}): {}".format(bitBody.Label, bitBody.Name, type(bitBody))
        )

        propNames = []
        for attributes in [
            o for o in bitBody.Group if PathPropertyBag.IsPropertyBag(o)
        ]:
            Path.Log.debug("Process properties from {}".format(attributes.Label))
            for prop in attributes.Proxy.getCustomProperties():
                self._setupProperty(obj, prop, attributes)
                propNames.append(prop)
        if not propNames:
            Path.Log.error(
                "Did not find a PropertyBag in {} - not a ToolBit shape?".format(
                    docName
                )
            )

        # has to happen last because it could trigger op.execute evaluations
        obj.BitPropertyNames = propNames
        obj.BitBody = bitBody
        self._copyBitShape(obj)

    def toolShapeProperties(self, obj):
        """toolShapeProperties(obj) ... return all properties defining it's shape"""
        return sorted(
            [
                prop
                for prop in obj.BitPropertyNames
                if obj.getGroupOfProperty(prop) == PropertyGroupShape
            ]
        )

    def toolAdditionalProperties(self, obj):
        """toolShapeProperties(obj) ... return all properties unrelated to it's shape"""
        return sorted(
            [
                prop
                for prop in obj.BitPropertyNames
                if obj.getGroupOfProperty(prop) != PropertyGroupShape
            ]
        )

    def toolGroupsAndProperties(self, obj, includeShape=True):
        """toolGroupsAndProperties(obj) ... returns a dictionary of group names with a list of property names."""
        category = {}
        for prop in obj.BitPropertyNames:
            group = obj.getGroupOfProperty(prop)
            if includeShape or group != PropertyGroupShape:
                properties = category.get(group, [])
                properties.append(prop)
                category[group] = properties
        return category

    def getBitThumbnail(self, obj):
        if obj.BitShape:
            path = findToolShape(obj.BitShape)
            if path:
                with open(path, "rb") as fd:
                    try:
                        zf = zipfile.ZipFile(fd)
                        pf = zf.open("thumbnails/Thumbnail.png", "r")
                        data = pf.read()
                        pf.close()
                        return data
                    except KeyError:
                        pass
        return None

    def saveToFile(self, obj, path, setFile=True):
        Path.Log.track(path)
        try:
            with open(path, "w") as fp:
                json.dump(self.templateAttrs(obj), fp, indent="  ")
            if setFile:
                obj.File = path
            return True
        except (OSError, IOError) as e:
            Path.Log.error(
                "Could not save tool {} to {} ({})".format(obj.Label, path, e)
            )
            raise

    def templateAttrs(self, obj):
        attrs = {}
        attrs["version"] = 2
        attrs["name"] = obj.Label
        if Path.Preferences.toolsStoreAbsolutePaths():
            attrs["shape"] = obj.BitShape
        else:
            # attrs['shape'] = findRelativePathShape(obj.BitShape)
            # Extract the name of the shape file
            __, filShp = os.path.split(
                obj.BitShape
            )  #  __ is an ignored placeholder acknowledged by LGTM
            attrs["shape"] = str(filShp)
        params = {}
        for name in obj.BitPropertyNames:
            params[name] = PathUtil.getPropertyValueString(obj, name)
        attrs["parameter"] = params
        params = {}
        attrs["attribute"] = params
        return attrs


def Declaration(path):
    Path.Log.track(path)
    with open(path, "r") as fp:
        return json.load(fp)


class ToolBitFactory(object):
    def CreateFromAttrs(self, attrs, name="ToolBit", path=None):
        Path.Log.track(attrs, path)
        obj = Factory.Create(name, attrs["shape"], path)
        obj.Label = attrs["name"]
        params = attrs["parameter"]
        for prop in params:
            PathUtil.setProperty(obj, prop, params[prop])
        obj.Proxy._updateBitShape(obj)
        obj.Proxy.unloadBitBody(obj)
        return obj

    def CreateFrom(self, path, name="ToolBit"):
        Path.Log.track(name, path)

        if not os.path.isfile(path):
            raise FileNotFoundError(f"{path} not found")
        try:
            data = Declaration(path)
            bit = Factory.CreateFromAttrs(data, name, path)
            return bit
        except (OSError, IOError) as e:
            Path.Log.error("%s not a valid tool file (%s)" % (path, e))
            raise

    def Create(self, name="ToolBit", shapeFile=None, path=None):
        Path.Log.track(name, shapeFile, path)
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", name)
        obj.Proxy = ToolBit(obj, shapeFile, path)
        return obj


Factory = ToolBitFactory()
