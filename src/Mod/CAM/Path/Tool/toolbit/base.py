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
from abc import ABC, abstractmethod
from typing import List, Optional, Tuple, Type, Union, Mapping, Any
from PySide.QtCore import QT_TRANSLATE_NOOP
from Path.Base.Generator import toolchange
from ..shape.base import ToolBitShape
from ..shape.registry import SHAPE_REGISTRY
from ..shape import TOOL_BIT_SHAPE_NAMES
from lazy_loader.lazy_loader import LazyLoader


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


class ToolBit(ABC):
    SHAPE_CLASS: Type[ToolBitShape]  # Abstract class attribute

    def __init__(
        self,
        obj,
        tool_bit_shape: Optional[ToolBitShape] = None,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(
            f"ToolBit __init__ called for {obj.Label}", tool_bit_shape, path
        )
        self.obj = obj
        self._tool_bit_shape = tool_bit_shape
        self._update_timer = None
        self._in_update = False

        self._create_base_properties(obj)
        if path:
            obj.File = str(path)
        assert self._tool_bit_shape.filepath is not None
        obj.ShapeFile = self._tool_bit_shape.filepath.name

        # Set the initial shape based on the provided instance
        obj.ShapeName = self._tool_bit_shape.name

        # Initialize properties and visual representation
        self._update_tool_properties(obj)
        self._update_visual_representation(obj)

        self.onDocumentRestored(obj)

    def _create_base_properties(self, obj):
        # Create the properties in the Base group.
        if not hasattr(obj, "ShapeName"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "ShapeName",
                "Base",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Shape type for the tool bit"
                ),
            )
            obj.ShapeName = TOOL_BIT_SHAPE_NAMES
        if not hasattr(obj, "ShapeFile"):
            obj.addProperty(
                "App::PropertyFile",
                "ShapeFile",
                "Base",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The file defining the tool shape (.fcstd)",
                ),
            )
        if not hasattr(obj, "BitBody"):
            obj.addProperty(
                "App::PropertyLink",
                "BitBody",
                "Base",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The parametrized body representing the tool bit",
                ),
            )
        if not hasattr(obj, "File"):
            obj.addProperty(
                "App::PropertyFile",
                "File",
                "Base",
                QT_TRANSLATE_NOOP("App::Property", "The toolbit file (.fctb)"),
            )

    @classmethod
    @abstractmethod
    def schema(
        cls,
    ) -> Mapping[
        str, Union[Tuple[str, str, Any], Tuple[str, str, Any, Tuple[str, ...]]]
    ]:
        """
        This schema defines any properties that the tool supports and
        that are not part of the shape file.

        Subclasses must implement this method to define their specific features.
        """
        return {
            "SpindleDirection": (
                FreeCAD.Qt.translate(
                    "ToolBit", "Direction of spindle rotation"
                ),
                "App::PropertyEnumeration",
                "Forward",  # Default value
                ("Forward", "Reverse", "None"),
            ),
            "Material": (
                FreeCAD.Qt.translate("ToolBit", "Tool material"),
                "App::PropertyEnumeration",
                "HSS",  # Default value
                ("HSS", "Carbide"),
            ),
        }

    def dumps(self):
        return None

    def loads(self, state):
        for obj in FreeCAD.ActiveDocument.Objects:
            if hasattr(obj, "Proxy") and obj.Proxy == self:
                self.obj = obj
                break
        return None

    def _ensure_shape_name(self, obj):
        """
        Ensure obj.ShapeName is set even for legacy tools
        """
        # Get file name. BitShape is legacy.
        filepath = None
        if hasattr(obj, "BitShape") and obj.BitShape:
            filepath = pathlib.Path(obj.BitShape)
        elif hasattr(obj, "ShapeFile") and obj.ShapeFile:
            filepath = pathlib.Path(obj.ShapeFile)
        elif hasattr(obj, "ShapeName") and obj.ShapeName:
            filepath = SHAPE_REGISTRY.get_shape_filename_from_alias(obj.ShapeName)
        if filepath is None:
            raise ValueError("ToolBit is missing a shape filename")
        obj.ShapeFile = filepath.name  # Remove the path

        # Find the shape name from the file or from the file name.
        inferred_shape_name = SHAPE_REGISTRY.get_shape_name_from_filename(
            obj.ShapeFile
        )
        shape_name = obj.ShapeName if hasattr(obj, "ShapeName") else ""
        if not shape_name:
            if inferred_shape_name:
                Path.Log.warning(
                    f"legacy tool bit has no ShapeName: {obj.Label}. Inferring {inferred_shape_name}"
                )
            else:
                Path.Log.warning(
                    f"legacy tool bit has no ShapeName: {obj.Label}. Assuming Endmill"
                )
                shape_name = "Endmill"
        obj.ShapeName = shape_name

    def _promote_bit_v1_to_v2(self, obj):
        """
        Promotes a legacy tool bit (v1) to the new format (v2).
        Legacy tools have a filename in the BitShape attribute.
        New tools use ShapeFile for the filename (empty for built-in)
        and ShapeName.
        """
        Path.Log.track(obj.Label)
        Path.Log.info(
            f"Promoting tool bit {obj.Label} ({obj.ShapeName}) from v1 to v2"
        )

        # Update SpindleDirection:
        # Old tools may still have "CCW", "CW", "Off", "None".
        # New tools use "None", "Forward", "Reverse".
        old_direction = obj.SpindleDirection
        normalized_direction = "None"  # Default to None

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

        # Get the schema properties from the current shape
        shape_cls = SHAPE_REGISTRY.get_shape_class_from_name(obj.ShapeName)
        if not shape_cls:
            raise ValueError(
                f"Failed to find shape class named '{obj.ShapeName}'"
            )
        shape_schema_props = shape_cls.schema().keys()

        # Move properties that are part of the shape schema to the "Shape" group
        for prop_name in obj.PropertiesList:
            if (
                obj.getGroupOfProperty(prop_name) == PropertyGroupShape
                or prop_name not in shape_schema_props
            ):
                continue
            try:
                Path.Log.debug(
                    f"Moving property '{prop_name}' to group '{PropertyGroupShape}'"
                )

                # Get property details before removing
                prop_type = obj.getTypeIdOfProperty(prop_name)
                prop_doc = obj.getDocumentationOfProperty(prop_name)
                prop_value = obj.getPropertyByName(prop_name)

                # Remove the property
                obj.removeProperty(prop_name)

                # Add the property back to the Shape group
                obj.addProperty(
                    prop_type, prop_name, PropertyGroupShape, prop_doc
                )
                self._in_update = True  # Prevent onChanged from running
                PathUtil.setProperty(obj, prop_name, prop_value)
                Path.Log.info(
                    f"Moved property '{prop_name}' to group '{PropertyGroupShape}'"
                )
            except Exception as e:
                Path.Log.error(
                    f"Failed to move property '{prop_name}' to group '{PropertyGroupShape}': {e}"
                )
                raise
            finally:
                self._in_update = False

    def onDocumentRestored(self, obj):
        Path.Log.track(obj.Label)

        # The way FreeCAD loads documents is to construct the object from C++,
        # and there is no guarantee that the constructor will have been (re-)executed.
        # So we must ensure to properly update/re-setup the properties.
        self._update_timer = None

        self._create_base_properties(obj)
        self._ensure_shape_name(obj)

        obj.setEditorMode("ShapeName", 1)
        obj.setEditorMode("ShapeFile", 1)
        obj.setEditorMode("BitBody", 2)
        obj.setEditorMode("File", 1)
        obj.setEditorMode("Shape", 2)

        # Get the shape instance based on the potentially updated
        # ShapeName and ShapeFile.
        self._tool_bit_shape = SHAPE_REGISTRY.get_shape_from_filename(
            obj.ShapeFile
        )

        # If BitBody exists and is in a different document after document restore,
        # it means a shallow copy occurred. We need to re-initialize the visual
        # representation and properties to ensure a deep copy of the BitBody
        # and its properties.
        # Only re-initialize properties from shape if not restoring from file
        if obj.BitBody and obj.BitBody.Document != obj.Document:
            Path.Log.debug(
                f"onDocumentRestored: Re-initializing BitBody for {obj.Label} after copy"
            )
            self._update_visual_representation(obj)

        # Ensure the correct ViewProvider is attached during document restore,
        # because some legacy fcstd files may still have references to old view
        # providers.
        if hasattr(obj, "ViewObject") and obj.ViewObject:
            if not isinstance(obj.ViewObject.Proxy, GuiBit.ViewProvider):
                Path.Log.debug(
                    f"onDocumentRestored: Attaching ViewProvider for {obj.Label}"
                )
                GuiBit.ViewProvider(obj.ViewObject, "ToolBit")

        # Ensure property state is correct after restore.
        self._update_tool_properties(obj)

        # Promote legacy tool bits to the new format. This requires properties
        # to be initialized before AND after the promotion.
        if hasattr(obj, "BitShape"):
            self._promote_bit_v1_to_v2(obj)
            self._update_tool_properties(obj)

    def onChanged(self, obj, prop):
        Path.Log.track(obj.Label, prop)
        # Avoid acting during document restore or internal updates
        if "Restore" in obj.State:
            return

        if hasattr(self, "_in_update") and self._in_update:
            Path.Log.debug(
                f"Skipping onChanged for {obj.Label} due to active update."
            )
            return

        # We only care about updates that affect the Shape
        if obj.getGroupOfProperty(prop) != PropertyGroupShape:
            return

        self._in_update = True
        try:
            new_value = obj.getPropertyByName(prop)
            Path.Log.debug(
                f"Shape parameter '{prop}' changed to {new_value}. "
                f"Updating visual representation."
            )
            self._tool_bit_shape.set_parameter(prop, new_value)
            self._update_visual_representation(obj)
        finally:
            self._in_update = False

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

    def _get_props(
        self, obj, group: Optional[Union[str, Tuple[str, ...]]] = None
    ) -> List[str]:
        """
        Returns a list of property names from the given group(s) for the object.
        Returns all groups if the group argument is None.
        """
        props_in_group = []
        # Use PropertiesList to get all property names
        for prop in obj.PropertiesList:
            prop_group = obj.getGroupOfProperty(prop)
            if group is None:
                props_in_group.append(prop)
            elif isinstance(group, str) and prop_group == group:
                props_in_group.append(prop)
            elif isinstance(group, tuple) and prop_group in group:
                props_in_group.append(prop)
        return props_in_group

    def toolGroupsAndProperties(self, obj, includeShape=True):
        category = {}

        for prop in self._get_props(obj):
            group = obj.getGroupOfProperty(prop)
            if includeShape or group != "Shape":
                properties = category.get(group, [])
                properties.append(prop)
                category[group] = properties
        return category

    def getBitThumbnail(self, obj):
        if not obj.ShapeFile:
            return

        # Find the path of the shape file.
        filepath = SHAPE_REGISTRY.shape_dir / obj.ShapeFile
        if not os.path.exists(filepath):
            return

        # Get the existing thumbnail.
        with open(filepath, "rb") as fd:
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
            with open(str(path), "w") as fp:
                json.dump(self.to_dict(obj), fp, indent="  ")
            if setFile:
                obj.File = str(path)
            return True
        except (OSError, IOError) as e:
            Path.Log.error(
                "Could not save tool {} to {} ({})".format(obj.Label, path, e)
            )
            raise

    def _remove_properties(self, obj, group, prop_names):
        for name in prop_names:
            if hasattr(obj, name):
                if obj.getGroupOfProperty(name) == group:
                    try:
                        obj.removeProperty(name)
                        Path.Log.debug(f"Removed property: {group}.{name}")
                    except Exception as e:
                        Path.Log.error(
                            f"Failed removing property '{group}.{name}': {e}"
                        )
            else:
                Path.Log.warning(
                    f"'{group}.{name}' failed to remove property, not found"
                )

    def _update_tool_properties(self, obj):
        """
        Initializes or updates the tool bit's properties based on the current
        _tool_bit_shape. Adds/updates shape parameters, removes obsolete shape
        parameters, and updates the edit state of them.
        Does not handle updating the visual representation.
        """
        Path.Log.track(obj.Label)

        # 1. Add/Update properties for the new shape
        for name, item in self._tool_bit_shape.schema().items():
            docstring = item[0]
            prop_type = item[1]
            value = self._tool_bit_shape.get_parameter(name)

            if not prop_type:
                Path.Log.error(
                    f"No property type for parameter '{name}' in shape "
                    f"'{self._tool_bit_shape.name}'. Skipping."
                )
                continue

            docstring = self._tool_bit_shape.get_parameter_label(name)

            # Add new property
            if not hasattr(obj, name):
                obj.addProperty(prop_type, name, "Shape", docstring)
                PathUtil.setProperty(obj, name, value)  # Set to default value
                Path.Log.debug(f"Added new shape property: {name}")

            # Ensure editor mode is correct
            obj.setEditorMode(name, 0)

        # 2. Remove obsolete shape properties
        # These are properties currently listed AND in the Shape group,
        # but not required by the new shape.
        current_shape_prop_names = set(self._get_props(obj, "Shape"))
        new_shape_param_names = self._tool_bit_shape.schema().keys()
        obsolete = current_shape_prop_names - new_shape_param_names
        self._remove_properties(obj, "Shape", obsolete)

        # 3. Add/Update properties for tool bit specific features
        for name, item in self.schema().items():
            docstring = item[0]
            prop_type = item[1]
            value = item[2]

            # Add new property
            if not hasattr(obj, name):
                obj.addProperty(prop_type, name, "Attributes", docstring)
                if prop_type == "App::PropertyEnumeration" and len(item) == 4:
                    setattr(obj, name, item[3])
                PathUtil.setProperty(obj, name, value)  # Set to default value
                Path.Log.debug(f"Added new feature property: {name}")

            # Ensure editor mode is correct
            obj.setEditorMode(name, 0)

            # Set editor mode for SpindleDirection based on can_rotate()
            if hasattr(obj, "SpindleDirection"):
                if not self.can_rotate():
                    obj.SpindleDirection = "None"
                    obj.setEditorMode("SpindleDirection", 2)  # Read-only

        # 4. Remove obsolete feature properties
        # These are properties currently listed AND in the Attributes group,
        # but not required by the current tool bit's schema.
        current_feature_names = set(self._get_props(obj, "Attributes"))
        new_feature_names = self.schema().keys()
        obsolete = current_feature_names - new_feature_names
        self._remove_properties(obj, "Attributes", obsolete)

    def _update_visual_representation(self, obj):
        """
        Updates the visual representation of the tool bit based on the current
        _tool_bit_shape. Creates or updates the BitBody and copies its shape
        to the main object.
        """
        Path.Log.track(obj.Label)

        # Remove existing BitBody if it exists
        self._removeBitBody(obj)

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
            obj.Shape = obj.BitBody.Shape  # Copy the evaluated Solid shape

            # Hide the visual representation and remove from tree
            if hasattr(obj.BitBody, "ViewObject") and obj.BitBody.ViewObject:
                obj.BitBody.ViewObject.Visibility = False
                obj.BitBody.ViewObject.ShowInTree = False

        except Exception as e:
            Path.Log.error(
                f"Failed to create visual representation using make_body for shape"
                f" '{self._tool_bit_shape.name}': {e}"
            )
            raise

    def to_dict(self, obj):
        Path.Log.track(obj.Label)
        attrs = {}
        attrs["version"] = 2
        attrs["name"] = obj.Label

        if self._tool_bit_shape:
            attrs["shape"] = str(self._tool_bit_shape.filepath)
            attrs["parameter"] = {
                name: PathUtil.getPropertyValueString(obj, name)
                for name in self._tool_bit_shape.get_parameters()
            }
        else:
            attrs["shape"] = ""
            attrs["parameter"] = {}

        attrs["parameter"].update(
            {
                name: PathUtil.getPropertyValueString(obj, name)
                for name in self._get_props(obj, "Attributes")
            }
        )

        attrs["attribute"] = {}
        return attrs

    def get_spindle_direction(self, obj) -> toolchange.SpindleDirection:
        # To be safe, never allow non-rotatable shapes (such as probes) to rotate.
        if not self.can_rotate():
            return toolchange.SpindleDirection.OFF

        # Otherwise use power from defined attribute.
        if (
            hasattr(obj, "SpindleDirection")
            and obj.SpindleDirection is not None
        ):
            if obj.SpindleDirection.lower() in ("cw", "forward"):
                return toolchange.SpindleDirection.CW
            else:
                return toolchange.SpindleDirection.CCW

        # Default to keeping spindle off.
        return toolchange.SpindleDirection.OFF

    def can_rotate(self) -> bool:
        """
        Whether the spindle is allowed to rotate for this kind of ToolBit.
        This mostly exists as a safe-hold for probes, which should never rotate.
        """
        return True


def Declaration(path):
    Path.Log.track(path)
    with open(path, "r") as fp:
        return json.load(fp)


class ToolBitFactory(object):
    def create_bit_from_dict(
        self, attrs: Mapping, filepath: Optional[pathlib.Path] = None
    ) -> Any:
        """
        Given a dictionary as read from json.loads('file.fctb'), this method creates
        a new ToolBit and returns a FeaturePython object for it.
        """
        Path.Log.track(attrs)
        shape_file = attrs.get("shape", "endmill.fcstd")
        params_dict = attrs.get("parameter", {})
        attributes = attrs.get("attribute", {})

        # Create the tool bit shape.
        try:
            tool_bit_shape = SHAPE_REGISTRY.get_shape_from_filename(
                shape_file, params_dict
            )
        except Exception as e:
            Path.Log.error(
                f"Failed to create shape from attributes for '{shape_file}': {e}."
                " Tool bit creation failed."
            )
            raise
        Path.Log.debug(
            f"Create shape from attributes for '{shape_file}' from {tool_bit_shape.filepath}."
        )

        # Find the correct ToolBit subclass based on the shape name
        tool_bit_classes = {
            b.SHAPE_CLASS.name: b for b in ToolBit.__subclasses__()
        }
        tool_bit_class = tool_bit_classes[tool_bit_shape.name]

        # Create the ToolBit proxy
        Path.Log.track(
            f"ToolBitFactory.create_bit_from_dict: Creating {tool_bit_class.__name__}"
        )
        name = attrs.get("name", tool_bit_shape.name)
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", name)
        obj.Proxy = tool_bit_class(obj, tool_bit_shape, filepath)
        obj.Label = name

        # Set additional attributes on the ToolBit object using the proxy
        # TODO: This should probably be cleaned up
        for att in attributes:
            # Check if the property exists before setting it
            if hasattr(obj, att):
                PathUtil.setProperty(obj, att, attributes[att])
            else:
                Path.Log.warning(
                    f"Attribute '{att}' not found on tool bit '{obj.Label}'. Skipping."
                )

        return obj

    def create_bit_from_file(self, path):
        Path.Log.track(path)

        if not os.path.isfile(path):
            raise FileNotFoundError(f"{path} not found")
        try:
            data = Declaration(path)
        except (OSError, IOError) as e:
            Path.Log.error("%s not a valid tool file (%s)" % (path, e))
            raise

        return Factory.create_bit_from_dict(data, filepath=path)

    def create_bit(
        self,
        filepath: Optional[pathlib.Path] = None,
        shapefile: str = "endmill.fcstd",
    ):
        """
        path is a path to the tool file
        shape_path is a path to the file that defines a shape (built-in or not)
        """
        Path.Log.track(filepath, shapefile)

        # Construct the attributes dictionary for create_bit_from_dict
        attrs = {
            "shape": shapefile,
            "parameter": {},  # Parameters will be loaded from the shape file
            "attribute": {},  # Attributes will be loaded from the shape file
        }

        return self.create_bit_from_dict(attrs, filepath=filepath)


Factory = ToolBitFactory()
