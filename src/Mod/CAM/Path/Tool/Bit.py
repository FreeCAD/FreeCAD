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
import json
import os
import zipfile
from PySide.QtCore import QT_TRANSLATE_NOOP

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

from .Shape.base import ToolBitShape
from .Shape.endmill import ToolBitShapeEndMill
from .Shape import TOOL_BIT_SHAPE_NAMES, get_shape_class

__title__ = "Tool bits."
__author__ = "sliptonic (Brad Collette), Samuel Abels"
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


def findRelativePathLibrary(path):
    return _findRelativePath(path, "Library")


class ToolBit(object):
    def __init__(self, obj, tool_bit_shape: ToolBitShape, path=None):
        Path.Log.track(obj.Label, tool_bit_shape.name, path)
        self.obj = obj
        self._tool_bit_shape = tool_bit_shape

        obj.addProperty(
            "App::PropertyEnumeration",
            "BitShape",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "Shape type for the tool bit"),
        )
        obj.BitShape = TOOL_BIT_SHAPE_NAMES

        obj.addProperty(
            "App::PropertyLink",
            "BitBody",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "The parametrized body representing the tool bit"),
        )
        obj.addProperty(
            "App::PropertyFile",
            "File",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "The file of the tool"),
        )
        obj.addProperty(
            "App::PropertyStringList",
            "BitPropertyNames",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "List of all properties inherited from the bit"),
        )

        if path:
            obj.File = path

        # Set the initial shape based on the provided instance
        obj.BitShape = self._tool_bit_shape.name

        # Initialize properties and visual representation
        self._initialize_properties_from_shape(obj)
        self._update_visual_representation(obj)

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
        Path.Log.track(obj.Label)
        # when files are shared it is essential to be able to change/set the shape file,
        # otherwise the file is hard to use
        # obj.setEditorMode('BitShape', 1)
        obj.setEditorMode("BitBody", 2)
        obj.setEditorMode("File", 1)
        obj.setEditorMode("Shape", 2)

        # If BitBody exists and is in a different document after document restore,
        # it means a shallow copy occurred. We need to re-initialize the visual
        # representation and properties to ensure a deep copy of the BitBody
        # and its properties.
        # Only re-initialize properties from shape if not restoring from file
        if obj.BitBody and obj.BitBody.Document != obj.Document:
            Path.Log.debug(f"onDocumentRestored: Re-initializing BitBody for {obj.Label} after copy")
            self._update_visual_representation(obj)
            # Only re-initialize properties from shape if not restoring from file
            if 'Restore' not in obj.State:
                Path.Log.debug(f"onDocumentRestored: Re-initializing properties from shape for {obj.Label}")
                self._initialize_properties_from_shape(obj)

        if not hasattr(obj, "BitPropertyNames"):
            obj.addProperty(
                "App::PropertyStringList",
                "BitPropertyNames",
                "Base",
                QT_TRANSLATE_NOOP("App::Property", "List of all properties inherited from the bit"),
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
            self._handle_shape_change(obj, obj.BitShape)

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

    def _removeBitBody(self, obj):
        if obj.BitBody:
            obj.BitBody.removeObjectsFromDocument()
            obj.Document.removeObject(obj.BitBody.Name)
            obj.BitBody = None

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
                json.dump(self.to_dict(obj), fp, indent="  ")
            if setFile:
                obj.File = path
            return True
        except (OSError, IOError) as e:
            Path.Log.error("Could not save tool {} to {} ({})".format(obj.Label, path, e))
            raise


    def _initialize_properties_from_shape(self, obj):
        Path.Log.track(obj.Label)

        Path.Log.track(obj.Label)

        prop_names = []
        # Add/update properties based on the current shape's parameters
        for name, value in self._tool_bit_shape.get_parameters().items():
            prop_type = self._tool_bit_shape.get_parameter_property_type(name)

            if prop_type:
                # Use the parameter label for the property documentation
                docstring = self._tool_bit_shape.get_parameter_label(name)

                # Only add the property if it doesn't exist
                if not hasattr(obj, name):
                    obj.addProperty(prop_type, name, PropertyGroupShape, docstring)
                    PathUtil.setProperty(obj, name, value) # Set to default value
                    obj.setEditorMode(name, 0)  # Allow editing in property editor
                else:
                    # Property already exists, preserve its value.
                    # Ensure editor mode is correct for existing shape properties.
                    obj.setEditorMode(name, 0) # Allow editing

                prop_names.append(name) # Collect the property name
            else:
                Path.Log.error(
                    f"Could not determine FreeCAD property type for parameter '{name}'"
                    f" in shape '{self._tool_bit_shape.name}'. Skipping."
                )

        # Update the list of properties inherited from the bit
        # This list should now only contain the shape properties
        obj.BitPropertyNames = prop_names

    def _update_visual_representation(self, obj):
        Path.Log.track(obj.Label)
        # Remove existing BitBody if it exists
        if obj.BitBody:
            obj.Document.removeObject(obj.BitBody.Name)
            obj.BitBody = None

        # Load the shape file and copy the root object to create the BitBody
        activeDoc = FreeCAD.ActiveDocument
        try:
            # Use the shape's name to find the corresponding shape file
            shape_file_path = self._tool_bit_shape.filepath
            if shape_file_path is None:
                 Path.Log.error(
                    f"Could not find shape file for tool bit shape '{self._tool_bit_shape.name}'"
                 )
                 return

            # Open the shape file in a temporary document
            # Convert PosixPath to string for FreeCAD.openDocument
            doc = FreeCAD.openDocument(str(shape_file_path), True)
            if not doc or not doc.RootObjects:
                 Path.Log.error(
                    f"Could not open shape file '{shape_file_path}' or it is empty"
                 )
                 if doc:
                     FreeCAD.closeDocument(doc.Name)
                 return

            # Copy the root object from the shape file document to the current document
            bitBody = obj.Document.copyObject(doc.RootObjects[0], True)

            # Close the temporary shape file document
            FreeCAD.setActiveDocument(activeDoc.Name)
            FreeCAD.closeDocument(doc.Name)

            # Assign the copied object to BitBody
            obj.BitBody = bitBody

            # Hide the visual representation
            if obj.BitBody and hasattr(obj.BitBody, 'ViewObject') and obj.BitBody.ViewObject:
                obj.BitBody.ViewObject.Visibility = False

        except Exception as e:
            Path.Log.error(
                f"Failed to create visual representation by copying from shape file"
                f" for shape '{self._tool_bit_shape.name}': {e}"
            )

    def _handle_shape_change(self, obj, new_shape_name):
        Path.Log.track(obj.Label, new_shape_name)

        old_shape_params = self._tool_bit_shape.get_parameters() if self._tool_bit_shape else {}

        shape_class = get_shape_class(new_shape_name)
        if shape_class is None:
            Path.Log.error(f"Unknown tool bit shape: {new_shape_name}. Keeping current shape.")
            # Revert the enumeration property in the UI if possible
            # This might not be strictly necessary as the UI might handle it,
            # but it's safer to keep the internal state consistent.
            if self._tool_bit_shape:
                 obj.BitShape = self._tool_bit_shape.name
            return

        # Attempt to preserve parameters from the old shape if they exist in the new one
        new_shape_params = {}
        try:
            # Temporarily instantiate the new shape to get its default parameters
            temp_instance = shape_class()
            for name in temp_instance.get_parameters():
                if name in old_shape_params:
                    new_shape_params[name] = old_shape_params[name]
        except Exception as e:
             Path.Log.warning(
                f"Failed to get default parameters for new shape '{new_shape_name}': {e}."
                " Cannot preserve parameters."
             )
             new_shape_params = {} # Start with empty parameters if defaults fail

        try:
            self._tool_bit_shape = shape_class(**new_shape_params)
            # Re-initialize properties and visual representation based on the new shape
            self._initialize_properties_from_shape(obj)
            self._update_visual_representation(obj)
        except Exception as e:
            Path.Log.error(
                f"Failed to instantiate or set up new shape '{new_shape_name}': {e}"
            )
            # Optionally, revert to the previous shape or a default shape on error
            # For now, we'll just log the error and keep the potentially broken state

    def to_dict(self, obj):
        Path.Log.track(obj.Label)
        attrs = {}
        attrs["version"] = 2
        attrs["name"] = obj.Label

        if self._tool_bit_shape:
            attrs["shape"] = self._tool_bit_shape.name
            attrs["parameter"] = {
                name: PathUtil.getPropertyValueString(obj, name)
                for name in self._tool_bit_shape.get_parameters()
            }
        else:
            attrs["shape"] = ""
            attrs["parameter"] = {}

        # Include additional attributes (non-shape properties)
        attrs["attribute"] = {
            prop: PathUtil.getPropertyValueString(obj, prop)
            for prop in obj.BitPropertyNames
            if obj.getGroupOfProperty(prop) != PropertyGroupShape
        }

        return attrs

def Declaration(path):
    Path.Log.track(path)
    with open(path, "r") as fp:
        return json.load(fp)


class ToolBitFactory(object):
    def CreateFromAttrs(self, attrs, name="ToolBit", path=None):
        Path.Log.track(attrs, path)
        shape_val = attrs.get("shape")
        params_dict = attrs.get("parameter", {})
        attributes = attrs.get("attribute", {})

        shape_class = None
        shape_name = None

        # Find the shape class
        if isinstance(shape_val, str):
            shape_class = get_shape_class(shape_val)
        elif isinstance(shape_val, type) and issubclass(shape_val, ToolBitShape):
            shape_class = shape_val
        else:
            Path.Log.error(f"Invalid tool bit shape value in file: {shape_val}")
            shape_class = ToolBitShapeEndMill

        if shape_class is None:
            Path.Log.error(f"Unknown tool bit shape: {shape_name}")
            shape_class = ToolBitShapeEndMill


        # Instantiate the shape with parameters from the file
        try:
            shape = shape_class(**params_dict)
        except Exception as e:
            Path.Log.error(
                f"Failed to instantiate shape '{shape_class}' with parameters"
                f" {params_dict}: {e}. Falling back to default shape."
            )
            shape = ToolBitShapeEndMill()


        # Create the ToolBit object with the instantiated shape
        obj = FreeCAD.ActiveDocument.addObject(
            "Part::FeaturePython", name
        )
        obj.Label = attrs.get("name", name)
        obj.Proxy = ToolBit(obj, shape, path)

        # Set additional attributes on the ToolBit object using the proxy
        for att in attributes:
            if hasattr(obj.Proxy, att): # Check if property exists on the proxy
                 PathUtil.setProperty(obj, att, attributes[att])
            else:
                 Path.Log.warning(f"Attribute '{att}' not found on ToolBit proxy. Skipping.")

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

    def Create(self, name="ToolBit", shape_name=None, path=None):
        Path.Log.track(name, shape_name, path)
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", name)

        # Instantiate the specified shape or a default one
        tool_bit_shape = None
        if shape_name:
            shape_class = get_shape_class(shape_name)
            if shape_class:
                try:
                    tool_bit_shape = shape_class() # Instantiate with defaults
                except Exception as e:
                    Path.Log.error(
                        f"Failed to instantiate shape '{shape_name}': {e}."
                        " Falling back to default."
                    )
            else:
                Path.Log.error(f"Unknown tool bit shape: {shape_name}. Falling back to default.")

        if tool_bit_shape is None:
            tool_bit_shape = ToolBitShapeEndMill() # Default shape

        obj.Proxy = ToolBit(obj, tool_bit_shape, path)
        return obj


Factory = ToolBitFactory()
