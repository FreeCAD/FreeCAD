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
import pathlib
import zipfile
import enum
from typing import Optional, Type
from PySide.QtCore import QT_TRANSLATE_NOOP
from Path.Base.Generator import toolchange
from .Shape.base import ToolBitShape
from .Shape.util import (
    get_builtin_shape_file_from_name,
    get_shape_from_name,
    get_shape_name_from_basename,
)
from .Shape import TOOL_BIT_SHAPE_NAMES
from lazy_loader.lazy_loader import LazyLoader

class ToolBitMaterial(enum.Enum):
    HSS = "HSS"
    Carbide = "Carbide"

Part = LazyLoader("Part", globals(), "Part")
GuiBit = LazyLoader("Path.Tool.Gui.Bit", globals(), "Path.Tool.Gui.Bit")

__title__ = "Tool bits."
__author__ = "sliptonic (Brad Collette), Samuel Abels"
__url__ = "https://www.freecad.org"
__doc__ = "Class to deal with and represent a tool bit."

PropertyGroupShape = "Shape"

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

def _findToolFile(name, containerFile, typ) -> Optional[str]:
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

def findToolShape(name, path=None) -> Optional[str]:
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
    def __init__(self,
                 obj,
                 tool_bit_shape: ToolBitShape,
                 path: Optional[pathlib.Path]=None):
        Path.Log.track(obj.Label, tool_bit_shape.label, path)
        self.obj = obj
        self._tool_bit_shape = tool_bit_shape

        self._create_properties(obj)

        if path:
            obj.File = str(path)
        obj.ShapeFile = str(self._tool_bit_shape.filepath or "")

        # Set the initial shape based on the provided instance
        obj.ShapeName = self._tool_bit_shape.name

        # Initialize properties and visual representation
        self._update_properties(obj)
        self._update_visual_representation(obj)

        self.onDocumentRestored(obj)

    def _create_properties(self, obj):
        # Create the properties in the Base group.
        if not hasattr(obj, "BitPropertyNames"):
            obj.addProperty(
                "App::PropertyStringList",
                "BitPropertyNames",
                "Base",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "List of all properties of the shape"
                ),
            )
            obj.BitPropertyNames = [] # Initialize empty
        obj.setEditorMode("BitPropertyNames", 2) # Read-only

        if not hasattr(obj, "ShapeName"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "ShapeName",
                "Base",
                QT_TRANSLATE_NOOP("App::Property", "Shape type for the tool bit"),
            )
            obj.ShapeName = TOOL_BIT_SHAPE_NAMES
        if not hasattr(obj, "ShapeFile"):
            obj.addProperty(
                "App::PropertyFile",
                "ShapeFile",
                "Base",
                QT_TRANSLATE_NOOP("App::Property", "The file defining the tool shape"),
            )
        if not hasattr(obj, "BitBody"):
            obj.addProperty(
                "App::PropertyLink",
                "BitBody",
                "Base",
                QT_TRANSLATE_NOOP("App::Property", "The parametrized body representing the tool bit"),
            )
        if not hasattr(obj, "File"):
            obj.addProperty(
                "App::PropertyFile",
                "File",
                "Base",
                QT_TRANSLATE_NOOP("App::Property", "The file of the tool"),
            )
        if not hasattr(obj, "BitPropertyNames"):
            obj.addProperty(
                "App::PropertyStringList",
                "BitPropertyNames",
                "Base",
                QT_TRANSLATE_NOOP("App::Property", "List of all properties inherited from the bit"),
            )

        # Create the properties in the Base group.
        if not hasattr(obj, "Attributes"):
            obj.addProperty(
                "App::PropertyStringList",
                "Attributes",
                "Base",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "List of all attributes of the tool"
                ),
            )
            obj.Attributes = [] # Initialize empty
        obj.setEditorMode("Attributes", 2) # Read-only

        if not hasattr(obj, "SpindleDirection"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "SpindleDirection",
                "Attributes",
                QT_TRANSLATE_NOOP("App::Property", "Direction of spindle rotation"),
            )
            obj.SpindleDirection = ["Forward", "Reverse", "None"]

        # Add new attributes: Material and Chipload
        if not hasattr(obj, "Material"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "Material",
                "Attributes",
                QT_TRANSLATE_NOOP("App::Property", "Material of the tool bit"),
            )
            obj.Material = [m.value for m in ToolBitMaterial]
            obj.Material = ToolBitMaterial.HSS.value

        if not hasattr(obj, "Chipload"):
            obj.addProperty(
                "App::PropertyFloat",
                "Chipload",
                "Attributes",
                QT_TRANSLATE_NOOP("App::Property", "Chip load for the tool bit"),
            )
            obj.Chipload = 0.0

    def dumps(self):
        return None

    def loads(self, state):
        for obj in FreeCAD.ActiveDocument.Objects:
            if hasattr(obj, "Proxy") and obj.Proxy == self:
                self.obj = obj
                break
        return None

    def _promote_bit_v1_to_v2(self, obj):
        """
        Promotes a legacy tool bit (v1) to the new format (v2).
        Legacy tools have a filename in the BitShape attribute.
        New tools use ShapeFile for the filename (empty for built-in)
        and ShapeName.
        """
        Path.Log.track(obj.Label)

        # Get shape name and file from legacy properties
        shape_file_legacy = obj.BitShape
        inferred_shape_name = get_shape_name_from_basename(shape_file_legacy)
        shape_name = obj.ShapeName if hasattr(obj, "ShapeName") else ""
        if not shape_name:
            if inferred_shape_name:
                Path.Log.warning(
                    f"legacy tool bit has no ShapeName: {obj.Label}. Inferring {inferred_shape_name}"
                )
            else:
                Path.Log.warning(
                    f"legacy tool bit has no ShapeName: {obj.Label}. Assuming endmill"
                )
                shape_name = 'endmill'
        obj.ShapeName = shape_name

        # Determine the new ShapeFile value
        # If the legacy BitShape is a built-in shape file, set ShapeFile to empty
        # Otherwise, keep the legacy BitShape value in ShapeFile
        if inferred_shape_name:
            obj.ShapeFile = ""
            Path.Log.debug(
                f"Legacy BitShape '{shape_file_legacy}' is built-in. "
                "Setting ShapeFile to empty."
            )
        else:
            obj.ShapeFile = shape_file_legacy
            Path.Log.debug(
                f"Legacy BitShape '{shape_file_legacy}' is not built-in. "
                "Setting ShapeFile to '{obj.ShapeFile}'."
            )

        # Update SpindleDirection:
        # Old tools may still have "CCW", "CW", "Off", "None".
        # New tools use "None", "Forward", "Reverse".
        Path.Log.debug(
            f"Promoting tool bit {obj.Label}: SpindleDirection before normalization: {obj.SpindleDirection}"
        )
        old_direction = obj.SpindleDirection
        normalized_direction = "None" # Default to None

        if isinstance(old_direction, str):
            lower_direction = old_direction.lower()
            if lower_direction in ("none", "off"):
                normalized_direction = "None"
            elif lower_direction in ("cw", "forward"):
                normalized_direction = "Forward"
            elif lower_direction in ("ccw", "reverse"):
                normalized_direction = "Reverse"

        obj.SpindleDirection = ["Forward", "Reverse", "None"]
        obj.SpindleDirection = normalized_direction
        if old_direction != normalized_direction:
            Path.Log.info(
                f"Promoted tool bit {obj.Label}: SpindleDirection from {old_direction} to {obj.SpindleDirection}"
            )

        # Remove the old BitShape property
        try:
            obj.removeProperty("BitShape")
            Path.Log.debug("Removed obsolete BitShape property.")
        except Exception as e:
            Path.Log.error(f"Failed removing obsolete BitShape property: {e}")

    def onDocumentRestored(self, obj):
        Path.Log.track(obj.Label)

        # Promote legacy tool bits to the new format
        self._create_properties(obj)
        if hasattr(obj, "BitShape"):
            self._promote_bit_v1_to_v2(obj)

        obj.setEditorMode('ShapeName', 1)
        obj.setEditorMode('ShapeFile', 1)
        obj.setEditorMode("BitBody", 2)
        obj.setEditorMode("File", 1)
        obj.setEditorMode("Shape", 2)

        # Get the shape instance based on the potentially updated
        # ShapeName and ShapeFile.
        self._tool_bit_shape, _ = get_shape_from_name(
            obj.ShapeName, pathlib.Path(obj.ShapeFile) if obj.ShapeFile else None
        )

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
                Path.Log.debug(
                    f"onDocumentRestored: Re-initializing properties from shape for {obj.Label}"
                )
                self._update_properties(obj)

        # Ensure the correct ViewProvider is attached during document restore
        if hasattr(obj, "ViewObject") and obj.ViewObject:
            if not isinstance(obj.ViewObject.Proxy, GuiBit.ViewProvider):
                Path.Log.debug(f"onDocumentRestored: Attaching ViewProvider for {obj.Label}")
                GuiBit.ViewProvider(obj.ViewObject, "ToolBit")

        # Ensure BitPropertyNames exists and is correct after restore.
        self._update_properties(obj)

        # Set editor modes for properties listed in BitPropertyNames.
        for prop in obj.BitPropertyNames:
            if obj.getGroupOfProperty(prop) != PropertyGroupShape:
                # Properties in the Shape group can only be modified if we have the
                # body.
                obj.setEditorMode(prop, 0 if obj.BitBody else 1)
            else:
                # All other custom properties can and should be edited directly in the
                # property editor widget.
                obj.setEditorMode(prop, 0)

    def onChanged(self, obj, prop):
        Path.Log.track(obj.Label, prop)
        if prop == "ShapeName" and "Restore" not in obj.State:
            self._handle_shape_change(obj)

    def onDelete(self, obj, arg2=None):
        Path.Log.track(obj.Label)
        self.unloadBitBody(obj)
        obj.Document.removeObject(obj.Name)

    def _updateShapeFile(self, obj, properties=None):
        if obj.BitBody is None:
            return

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
        if not obj.ShapeName:
            return

        # Find the path of the shape file.
        if obj.ShapeFile and os.path.exists(obj.ShapeFile):
            path = obj.ShapeFile
        elif not obj.ShapeFile:
            path = get_builtin_shape_file_from_name(obj.ShapeName)
        else:
            return
        if not path:
            return

        # Get the existing thumbnail.
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

    def _update_properties(self, obj):
        """
        Initializes or updates the tool bit's properties based on the current
        _tool_bit_shape. Adds/updates shape parameters, removes obsolete shape
        parameters, and updates BitPropertyNames to only list current shape
        parameters. Does not handle updating the visual representation.
        """
        Path.Log.track(obj.Label)

        # 1. Get current and new shape parameter names
        # Use try-except in case BitPropertyNames exists but is None or invalid
        try:
            current_shape_prop_names = set(obj.BitPropertyNames)
        except TypeError:
             Path.Log.warning("BitPropertyNames was invalid, resetting.")
             current_shape_prop_names = set()
        new_shape_params = self._tool_bit_shape.get_parameters()
        new_shape_param_names = set(new_shape_params.keys())

        # 2. Add/Update properties for the new shape
        for name, value in new_shape_params.items():
            prop_type = self._tool_bit_shape.get_parameter_property_type(name)
            if not prop_type:
                Path.Log.error(
                    f"No property type for parameter '{name}' in shape "
                    f"'{self._tool_bit_shape.name}'. Skipping."
                )
                continue

            docstring = self._tool_bit_shape.get_parameter_label(name)

            if not hasattr(obj, name):
                # Add new property
                obj.addProperty(prop_type, name, PropertyGroupShape, docstring)
                PathUtil.setProperty(obj, name, value) # Set to default value
                Path.Log.debug(f"Added new shape property: {name}")

            # Ensure editor mode is correct
            obj.setEditorMode(name, 0)

        # 3. Remove obsolete shape properties
        # These are properties currently listed AND in the Shape group,
        # but not required by the new shape.
        obsolete_prop_names = current_shape_prop_names - new_shape_param_names
        for name in obsolete_prop_names:
            if hasattr(obj, name):
                if obj.getGroupOfProperty(name) == PropertyGroupShape:
                    try:
                        obj.removeProperty(name)
                        Path.Log.debug(f"Removed obsolete shape property: {name}")
                    except Exception as e:
                        Path.Log.error(f"Failed removing obsolete property '{name}': {e}")
            else:
                 Path.Log.warning(
                     f"'{name}' in BitPropertyNames but not on object."
                 )

        # 4. Update BitPropertyNames to exactly match the new shape's parameters
        obj.BitPropertyNames = sorted(list(new_shape_param_names))

        # 5. Update non-shape properties.
        # Set editor mode for SpindleDirection based on can_rotate()
        if hasattr(obj, "SpindleDirection"):
            if not self._tool_bit_shape.can_rotate():
                obj.SpindleDirection = "None"
                obj.setEditorMode("SpindleDirection", 2) # Read-only
            else:
                obj.setEditorMode("SpindleDirection", 0) # Editable
        if hasattr(obj, "Material"):
            obj.setEditorMode("Material", 0) # Editable
        if hasattr(obj, "Chipload"):
            obj.setEditorMode("Chipload", 0) # Editable

    def _update_visual_representation(self, obj):
        Path.Log.track(obj.Label)
        # Remove existing BitBody if it exists
        if obj.BitBody:
            obj.Document.removeObject(obj.BitBody.Name)
            obj.BitBody = None

        try:
            # Use the shape's make_body method to create the visual representation
            body = self._tool_bit_shape.make_body(obj.Document)

            if not body:
                Path.Log.error(
                f"Failed to create visual representation for shape "
                f"'{self._tool_bit_shape.name}'"
                )
                return

            # Assign the created object to BitBody and copy its shape
            obj.BitBody = body
            obj.Shape = obj.BitBody.Shape # Copy the evaluated Solid shape

            # Hide the visual representation
            if obj.BitBody and hasattr(obj.BitBody, 'ViewObject') and obj.BitBody.ViewObject:
                obj.BitBody.ViewObject.Visibility = False

        except Exception as e:
            Path.Log.error(
                f"Failed to create visual representation using make_body for shape"
                f" '{self._tool_bit_shape.name}': {e}"
            )
            raise

    def _handle_shape_change(self, obj):
        Path.Log.track(obj, obj.label)

        new_shape_name = obj.ShapeName

        old_shape_params = self._tool_bit_shape.get_parameters() if self._tool_bit_shape else {}

        try:
            # Use the utility function to get the shape instance.
            new_shape_instance, _ = get_shape_from_name(
                new_shape_name, pathlib.Path(obj.File) if obj.File else None
            )
            obj.ShapeFile = str(new_shape_instance.filepath or "")

            # Attempt to preserve parameters from the old shape if they exist
            # in the new one. We iterate through the parameters of the new shape
            # instance and copy values from the old shape's parameters if they exist.
            for name in new_shape_instance.get_parameters():
                if name in old_shape_params:
                    new_shape_instance.set_parameter(name, old_shape_params[name])
        except Exception as e:
            Path.Log.error(
                f"Failed to instantiate or set up new shape '{new_shape_name}': {e}"
            )

            # Revert to the previous shape
            obj.ShapeName = self._tool_bit_shape.name
            return

        # Re-initialize properties and visual representation based on the new shape
        self._tool_bit_shape = new_shape_instance
        self._update_properties(obj)
        self._update_visual_representation(obj)

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

        attrs["attribute"] = {
            name: PathUtil.getPropertyValueString(obj, name)
            for name in self.toolAdditionalProperties(obj)
        }
        return attrs

    def get_spindle_direction(self, obj) -> toolchange.SpindleDirection:
        # To be safe, never allow non-rotatable shapes (such as probes) to rotate.
        if not self._tool_bit_shape.can_rotate():
            return toolchange.SpindleDirection.OFF

        # Otherwise use power from defined attribute.
        if hasattr(obj, "SpindleDirection") and obj.SpindleDirection is not None:
            if obj.SpindleDirection.lower() in ('cw', 'forward'):
                return toolchange.SpindleDirection.CW
            else:
                return toolchange.SpindleDirection.CCW

        # Default to keeping spindle off.
        return toolchange.SpindleDirection.OFF


def Declaration(path):
    Path.Log.track(path)
    with open(path, "r") as fp:
        return json.load(fp)


class ToolBitFactory(object):
    def CreateFromAttrs(self, attrs, name="ToolBit", path=None, document=None) -> Type[ToolBit]:
        document = document if document else FreeCAD.ActiveDocument
        if not document:
            raise Exception('no open document found')

        Path.Log.track(attrs, path)
        shape_val = os.path.splitext(attrs.get("shape"))[0]
        params_dict = attrs.get("parameter", {})
        attributes = attrs.get("attribute", {})

        try:
            # Use the utility function to get the shape instance.
            # The file path is now stored within the instance itself.
            tool_bit_shape, _ = get_shape_from_name(
                shape_val, pathlib.Path(path) if path else None, params_dict
            )
        except Exception as e:
            Path.Log.error(
                f"Failed to create shape from attributes for '{shape_val}': {e}."
                " Tool bit creation failed."
            )
            raise
        Path.Log.debug(
            f"Create shape from attributes for '{shape_val}' from {tool_bit_shape.filepath}."
        )

        # Create the ToolBit object with the instantiated shape
        obj = document.addObject("Part::FeaturePython", name)
        obj.Proxy = ToolBit(obj, tool_bit_shape, path)
        obj.Label = attrs.get("name", name)

        # Set additional attributes on the ToolBit object using the proxy
        for att in attributes:
            # Check if the property exists before setting it
            if hasattr(obj, att):
                PathUtil.setProperty(obj, att, attributes[att])
            else:
                Path.Log.warning(
                    f"Attribute '{att}' not found on tool bit '{obj.Label}'. Skipping."
                )

        return obj

    def CreateFrom(self, path, name="ToolBit", document=None):
        Path.Log.track(name, path)

        if not os.path.isfile(path):
            raise FileNotFoundError(f"{path} not found")
        try:
            data = Declaration(path)
            bit = Factory.CreateFromAttrs(data, name, document)
            return bit
        except (OSError, IOError) as e:
            Path.Log.error("%s not a valid tool file (%s)" % (path, e))
            raise

    def Create(self, shape_name: str, name="ToolBit", path=None, document=None):
        Path.Log.track(name, shape_name, path)

        # Construct the attributes dictionary for CreateFromAttrs
        attrs = {
            "shape": shape_name,
            "parameter": {},  # Parameters will be loaded from the shape file
            "attribute": {},  # Attributes will be loaded from the shape file
        }

        # Use CreateFromAttrs to create the tool bit
        return self.CreateFromAttrs(attrs, name, path, document)

Factory = ToolBitFactory()
