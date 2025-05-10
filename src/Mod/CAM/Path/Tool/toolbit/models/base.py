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
import uuid
import pathlib
from abc import ABC
from lazy_loader.lazy_loader import LazyLoader
from typing import List, Optional, Tuple, Type, Union, Mapping, Any
from PySide.QtCore import QT_TRANSLATE_NOOP
from Path.Base.Generator import toolchange
from ...assets import asset_manager
from ...shape import ToolBitShape, ToolBitShapeIcon
from ..docobject import DetachedDocumentObject
from ..util import to_json

Part = LazyLoader("Part", globals(), "Part")
GuiBit = LazyLoader("Path.Tool.Gui.Bit", globals(), "Path.Tool.Gui.Bit")


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
        tool_bit_shape: ToolBitShape,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track("ToolBit __init__ called", tool_bit_shape, path)
        self.id = str(uuid.uuid4())
        self.obj = DetachedDocumentObject()
        self.obj.Proxy = self
        self._tool_bit_shape = tool_bit_shape
        self._update_timer = None
        self._in_update = False

        self._create_base_properties()
        self.obj.File = str(path) if path else ""
        self.obj.ShapeFile = self._tool_bit_shape.get_id()   #TODO: use a Uri

        # Initialize properties
        self._update_tool_properties()

    @classmethod
    def create(
        cls,
        doc: FreeCAD.Document,
        tool_bit_shape: ToolBitShape,
        path: Optional[pathlib.Path] = None,
    ) -> "ToolBit":
        """
        Creates a ToolBit instance attached to a real FreeCAD DocumentObject.
        """
        try:
            obj = doc.addObject("Part::FeaturePython", tool_bit_shape.name)
        except Exception as e:
            Path.Log.error(f"Failed to create DocumentObject '{tool_bit_shape.name}': {e}")
            raise

        # Create the ToolBit instance and attach it to the DocumentObject
        tool_bit = cls(tool_bit_shape, path=path)
        tool_bit.attach(obj)

        return tool_bit

    def get_label(self) -> str:
        """Returns the label of the tool bit."""
        return self.obj.Label

    def set_label(self, label: str):
        """Sets the label of the tool bit."""
        self.obj.Label = label

    def get_shape_name(self) -> str:
        """Returns the shape name of the tool bit."""
        return self._tool_bit_shape.name

    def set_shape_name(self, name: str):
        """Sets the shape name of the tool bit."""
        self._tool_bit_shape.name = name

    def _create_base_properties(self):
        # Create the properties in the Base group.
        if not hasattr(self.obj, "ShapeFile"):
            self.obj.addProperty(
                "App::PropertyFile",
                "ShapeFile",
                "Base",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The file defining the tool shape (.fcstd)",
                ),
            )
        if not hasattr(self.obj, "BitBody"):
            self.obj.addProperty(
                "App::PropertyLink",
                "BitBody",
                "Base",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The parametrized body representing the tool bit",
                ),
            )
        if not hasattr(self.obj, "File"):
            self.obj.addProperty(
                "App::PropertyFile",
                "File",
                "Base",
                QT_TRANSLATE_NOOP("App::Property", "The toolbit file (.fctb)"),
            )

        # Create the ToolBit properties that are shared by all tool bits
        if not hasattr(self.obj, "SpindleDirection"):
            self.obj.addProperty(
                "App::PropertyEnumeration",
                "SpindleDirection",
                "Attributes",
                QT_TRANSLATE_NOOP("App::Property", "Direction of spindle rotation"),
            )
            self.obj.SpindleDirection = ["Forward", "Reverse", "None"]
            self.obj.SpindleDirection = "Forward"  # Default value
        if not hasattr(self.obj, "Material"):
            self.obj.addProperty(
                "App::PropertyEnumeration",
                "Material",
                "Attributes",
                QT_TRANSLATE_NOOP("App::Property", "Tool material"),
            )
            self.obj.Material = ["HSS", "Carbide"]
            self.obj.Material = "HSS"  # Default value

    def dumps(self):
        return None

    def loads(self, state):
        for obj in FreeCAD.ActiveDocument.Objects:
            if hasattr(obj, "Proxy") and obj.Proxy == self:
                self.obj = obj
                break
        return None

    def _ensure_shape_file(self):
        """
        Ensure obj.ShapeFile is set even for legacy tools
        """
        # Get file name. BitShape is legacy.
        name = None
        if hasattr(self.obj, "BitShape") and self.obj.BitShape:
            name = pathlib.Path(self.obj.BitShape).name
        elif hasattr(self.obj, "ShapeFile") and self.obj.ShapeFile:
            name = pathlib.Path(self.obj.ShapeFile).name
        elif hasattr(self.obj, "ShapeName") and self.obj.ShapeName:
            name = pathlib.Path(self.obj.ShapeName).name
        if name is None:
            raise ValueError("ToolBit is missing a shape ID")

        uri = ToolBitShape.resolve_name(name)
        if uri is None:
            raise ValueError(f"Failed to identify shape of ToolBit from name '{name}'")
        self.obj.ShapeFile = str(uri)

    def _promote_bit_v1_to_v2(self):
        """
        Promotes a legacy tool bit (v1) to the new format (v2).
        Legacy tools have a filename in the BitShape attribute.
        New tools use ShapeFile for the shape ID.
        """
        Path.Log.track(self.obj.Label)
        Path.Log.info(f"Promoting tool bit {self.obj.Label} ({self.obj.ShapeFile}) from v1 to v2")

        # Update SpindleDirection:
        # Old tools may still have "CCW", "CW", "Off", "None".
        # New tools use "None", "Forward", "Reverse".
        old_direction = self.obj.SpindleDirection
        normalized_direction = "None"  # Default to None

        if isinstance(old_direction, str):
            lower_direction = old_direction.lower()
            if lower_direction in ("none", "off"):
                normalized_direction = "None"
            elif lower_direction in ("cw", "forward"):
                normalized_direction = "Forward"
            elif lower_direction in ("ccw", "reverse"):
                normalized_direction = "Reverse"

        self.obj.SpindleDirection = ["Forward", "Reverse", "None"]
        self.obj.SpindleDirection = normalized_direction
        if old_direction != normalized_direction:
            Path.Log.info(
                f"Promoted tool bit {self.obj.Label}: SpindleDirection from {old_direction} to {self.obj.SpindleDirection}"
            )

        # Remove the old BitShape property
        try:
            self.obj.removeProperty("BitShape")
            Path.Log.debug("Removed obsolete BitShape property.")
        except Exception as e:
            Path.Log.error(f"Failed removing obsolete BitShape property: {e}")

        # Get the schema properties from the current shape
        shape_cls = ToolBitShape.get_subclass_by_name(self.obj.ShapeName)
        if not shape_cls:
            raise ValueError(f"Failed to find shape class named '{self.obj.ShapeName}'")
        shape_schema_props = shape_cls.schema().keys()

        # Move properties that are part of the shape schema to the "Shape" group
        for prop_name in self.obj.PropertiesList:
            if (
                self.obj.getGroupOfProperty(prop_name) == PropertyGroupShape
                or prop_name not in shape_schema_props
            ):
                continue
            try:
                Path.Log.debug(f"Moving property '{prop_name}' to group '{PropertyGroupShape}'")

                # Get property details before removing
                prop_type = self.obj.getTypeIdOfProperty(prop_name)
                prop_doc = self.obj.getDocumentationOfProperty(prop_name)
                prop_value = self.obj.getPropertyByName(prop_name)

                # Remove the property
                self.obj.removeProperty(prop_name)

                # Add the property back to the Shape group
                self.obj.addProperty(prop_type, prop_name, PropertyGroupShape, prop_doc)
                self._in_update = True  # Prevent onChanged from running
                PathUtil.setProperty(self.obj, prop_name, prop_value)
                Path.Log.info(f"Moved property '{prop_name}' to group '{PropertyGroupShape}'")
            except Exception as e:
                Path.Log.error(
                    f"Failed to move property '{prop_name}' to group '{PropertyGroupShape}': {e}"
                )
                raise
            finally:
                self._in_update = False

    def onDocumentRestored(self, obj):
        # Assign self.obj to the restored object
        self.obj = obj
        self.obj.Proxy = self
        Path.Log.track(self.obj.Label)

        # The way FreeCAD loads documents is to construct the object from C++,
        # and there is no guarantee that the constructor will have been (re-)executed.
        # So we must ensure to properly update/re-setup the properties.
        self._update_timer = None

        self._create_base_properties()
        self._ensure_shape_file()

        self.obj.setEditorMode("ShapeFile", 1)
        self.obj.setEditorMode("BitBody", 2)
        self.obj.setEditorMode("File", 1)
        self.obj.setEditorMode("Shape", 2)

        # Get the shape instance based on the potentially ShapeFile. For backward
        # compatibility, we try two approaches to find the shape and shape
        # class from the file.
        # First, translate the filename to an asset ID, then:
        #   1. try to find the shape file
        #   2. otherwise create a new empty instance
        uri = ToolBitShape.resolve_name(self.obj.ShapeFile)
        if uri is None:
            raise ValueError(
                f"Failed to identify URI of ToolBitShape from name '{self.obj.ShapeFile}'"
            )

        try:
            # Best case: we directly find the shape file in our assets.
            self._tool_bit_shape = asset_manager.get(uri, store="shapestore")
        except FileNotFoundError:
            # Otherwise, try to at least identify the type of the shape.
            shape_class = ToolBitShape.get_subclass_by_name(uri.asset_id)
            if not shape_class:
                raise ValueError(
                    "Failed to identify class of ToolBitShape from name "
                    f"'{self.obj.ShapeFile}' (asset id {uri.asset_id})"
                )
            self._tool_bit_shape = shape_class(uri.asset_id)

        # If BitBody exists and is in a different document after document restore,
        # it means a shallow copy occurred. We need to re-initialize the visual
        # representation and properties to ensure a deep copy of the BitBody
        # and its properties.
        # Only re-initialize properties from shape if not restoring from file
        if self.obj.BitBody and self.obj.BitBody.Document != self.obj.Document:
            Path.Log.debug(
                f"onDocumentRestored: Re-initializing BitBody for {self.obj.Label} after copy"
            )
            self._update_visual_representation()

        # Ensure the correct ViewProvider is attached during document restore,
        # because some legacy fcstd files may still have references to old view
        # providers.
        if hasattr(self.obj, "ViewObject") and self.obj.ViewObject:
            if hasattr(self.obj.ViewObject, "Proxy") and not isinstance(
                self.obj.ViewObject.Proxy, GuiBit.ViewProvider
            ):
                Path.Log.debug(f"onDocumentRestored: Attaching ViewProvider for {self.obj.Label}")
                GuiBit.ViewProvider(self.obj.ViewObject, "ToolBit")

        # Ensure property state is correct after restore.
        self._update_tool_properties()

        # Promote legacy tool bits to the new format. This requires properties
        # to be initialized before AND after the promotion.
        if hasattr(self.obj, "BitShape"):
            self._promote_bit_v1_to_v2()
            self._update_tool_properties()

    def attach(self, obj: FreeCAD.DocumentObject):
        """
        Attaches the ToolBit instance to a FreeCAD DocumentObject.

        Transfers properties from the internal DetachedDocumentObject to the
        new obj and updates the visual representation.
        """
        if not isinstance(self.obj, DetachedDocumentObject):
            Path.Log.warning(
                f"ToolBit {self.obj.Label} is already attached to a "
                "DocumentObject. Skipping attach."
            )
            return

        Path.Log.track(f"Attaching ToolBit to {obj.Label}")

        temp_obj = self.obj  # Store the detached object
        self.obj = obj  # Set the real DocumentObject
        self.obj.Proxy = self # Set the document object's proxy to this ToolBit instance

        self._create_base_properties()

        # Transfer property values from the detached object to the real object
        temp_obj.copy_to(self.obj)

        # Update the visual representation now that it's attached
        self._update_tool_properties()
        self._update_visual_representation()

    def onChanged(self, obj, prop):
        Path.Log.track(obj.Label, prop)
        # Avoid acting during document restore or internal updates
        if "Restore" in obj.State:
            return

        if hasattr(self, "_in_update") and self._in_update:
            Path.Log.debug(f"Skipping onChanged for {obj.Label} due to active update.")
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
            self._update_visual_representation()
        finally:
            self._in_update = False

    def onDelete(self, obj, arg2=None):
        Path.Log.track(obj.Label)
        self.unloadBitBody()
        obj.Document.removeObject(obj.Name)

    def _updateShapeFile(self, properties=None):
        if self.obj.BitBody is None:
            return

        for attributes in [
            o
            for o in self.obj.BitBody.Group
            if hasattr(o, "Proxy") and hasattr(o.Proxy, "getCustomProperties")
        ]:
            for prop in attributes.Proxy.getCustomProperties():
                # the property might not exist in our local object (new attribute in shape)
                # for such attributes we just keep the default
                if hasattr(self.obj, prop):
                    setattr(attributes, prop, self.obj.getPropertyByName(prop))
                else:
                    # if the template shape has a new attribute defined we should add that
                    # to the local object
                    self._setupProperty(prop, attributes)

        self._copyBitShape()

    def _copyBitShape(self):
        self.obj.Document.recompute()
        if self.obj.BitBody and self.obj.BitBody.Shape:
            self.obj.Shape = self.obj.BitBody.Shape
        else:
            self.obj.Shape = Part.Shape()

    def _removeBitBody(self):
        if self.obj.BitBody:
            self.obj.BitBody.removeObjectsFromDocument()
            self.obj.Document.removeObject(self.obj.BitBody.Name)
            self.obj.BitBody = None

    def unloadBitBody(self):
        self._removeBitBody()

    def _setupProperty(self, prop, orig):
        # extract property parameters and values so it can be copied
        val = orig.getPropertyByName(prop)
        typ = orig.getTypeIdOfProperty(prop)
        grp = orig.getGroupOfProperty(prop)
        dsc = orig.getDocumentationOfProperty(prop)

        self.obj.addProperty(typ, prop, grp, dsc)
        if "App::PropertyEnumeration" == typ:
            setattr(self.obj, prop, orig.getEnumerationsOfProperty(prop))
        self.obj.setEditorMode(prop, 1)
        PathUtil.setProperty(self.obj, prop, val)

    def _get_props(self, group: Optional[Union[str, Tuple[str, ...]]] = None) -> List[str]:
        """
        Returns a list of property names from the given group(s) for the object.
        Returns all groups if the group argument is None.
        """
        props_in_group = []
        # Use PropertiesList to get all property names
        for prop in self.obj.PropertiesList:
            prop_group = self.obj.getGroupOfProperty(prop)
            if group is None:
                props_in_group.append(prop)
            elif isinstance(group, str) and prop_group == group:
                props_in_group.append(prop)
            elif isinstance(group, tuple) and prop_group in group:
                props_in_group.append(prop)
        return props_in_group

    def toolGroupsAndProperties(self, includeShape=True):
        category = {}

        for prop in self._get_props():
            group = self.obj.getGroupOfProperty(prop)
            if includeShape or group != "Shape":
                properties = category.get(group, [])
                properties.append(prop)
                category[group] = properties
        return category

    def get_icon(self) -> Optional[ToolBitShapeIcon]:
        """
        Retrieves the thumbnail data for the tool bit shape, as
        taken from the explicit SVG or PNG, if the shape has one.
        """
        if self._tool_bit_shape:
            return self._tool_bit_shape.get_icon()
        return None

    def get_thumbnail(self) -> Optional[bytes]:
        """
        Retrieves the thumbnail data for the tool bit shape in PNG format,
        as embedded in the shape file.
        Fallback to the icon from get_icon() (converted to PNG)
        """
        if not self._tool_bit_shape:
            return None
        png_data = self._tool_bit_shape.get_thumbnail()
        if png_data:
            return png_data
        icon = self.get_icon()
        if icon:
            return icon.get_png()
        return None

    def saveToFile(self, path, setFile=True):
        Path.Log.track(path)
        try:
            with open(str(path), "w") as fp:
                json.dump(self.to_dict(), fp, indent="  ")
            if setFile:
                self.obj.File = str(path)
            return True
        except (OSError, IOError) as e:
            Path.Log.error("Could not save tool {} to {} ({})".format(self.obj.Label, path, e))
            raise

    def _remove_properties(self, group, prop_names):
        for name in prop_names:
            if hasattr(self.obj, name):
                if self.obj.getGroupOfProperty(name) == group:
                    try:
                        self.obj.removeProperty(name)
                        Path.Log.debug(f"Removed property: {group}.{name}")
                    except Exception as e:
                        Path.Log.error(f"Failed removing property '{group}.{name}': {e}")
            else:
                Path.Log.warning(f"'{group}.{name}' failed to remove property, not found")

    def _update_tool_properties(self):
        """
        Initializes or updates the tool bit's properties based on the current
        _tool_bit_shape. Adds/updates shape parameters, removes obsolete shape
        parameters, and updates the edit state of them.
        Does not handle updating the visual representation.
        """
        Path.Log.track(self.obj.Label)

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
            if not hasattr(self.obj, name):
                self.obj.addProperty(prop_type, name, "Shape", docstring)
                setattr(self.obj, name, value)
                Path.Log.debug(f"Added new shape property: {name}")

            # Ensure editor mode is correct
            self.obj.setEditorMode(name, 0)

        # 2. Remove obsolete shape properties
        # These are properties currently listed AND in the Shape group,
        # but not required by the new shape.
        current_shape_prop_names = set(self._get_props("Shape"))
        new_shape_param_names = self._tool_bit_shape.schema().keys()
        obsolete = current_shape_prop_names - new_shape_param_names
        self._remove_properties("Shape", obsolete)

    def _update_visual_representation(self):
        """
        Updates the visual representation of the tool bit based on the current
        _tool_bit_shape. Creates or updates the BitBody and copies its shape
        to the main object.
        """
        if isinstance(self.obj, DetachedDocumentObject):
            return
        Path.Log.track(self.obj.Label)

        # Remove existing BitBody if it exists
        self._removeBitBody()

        try:
            # Use the shape's make_body method to create the visual representation
            body = self._tool_bit_shape.make_body(self.obj.Document)

            if not body:
                Path.Log.error(
                    f"Failed to create visual representation for shape "
                    f"'{self._tool_bit_shape.name}'"
                )
                return

            # Assign the created object to BitBody and copy its shape
            self.obj.BitBody = body
            self.obj.Shape = self.obj.BitBody.Shape  # Copy the evaluated Solid shape

            # Hide the visual representation and remove from tree
            if hasattr(self.obj.BitBody, "ViewObject") and self.obj.BitBody.ViewObject:
                self.obj.BitBody.ViewObject.Visibility = False
                self.obj.BitBody.ViewObject.ShowInTree = False

        except Exception as e:
            Path.Log.error(
                f"Failed to create visual representation using make_body for shape"
                f" '{self._tool_bit_shape.name}': {e}"
            )
            raise

    def to_dict(self):
        """
        Returns a dictionary representation of the tool bit.
        """
        Path.Log.track(self.obj.Label)
        attrs = {}
        attrs["version"] = 2
        attrs["name"] = self.obj.Label

        if self._tool_bit_shape:
            attrs["shape"] = self._tool_bit_shape.get_id() + ".fcstd"
            attrs["parameter"] = {
                name: to_json(getattr(self.obj, name))
                for name in self._tool_bit_shape.get_parameters()
            }
        else:
            attrs["shape"] = ""
            attrs["parameter"] = {}

        attrs["parameter"].update(
            {
                name: to_json(getattr(self.obj, name))
                for name in self._get_props("Attributes")
            }
        )

        attrs["attribute"] = {}
        return attrs

    def get_spindle_direction(self) -> toolchange.SpindleDirection:
        # To be safe, never allow non-rotatable shapes (such as probes) to rotate.
        if not self.can_rotate():
            return toolchange.SpindleDirection.OFF

        # Otherwise use power from defined attribute.
        if hasattr(self.obj, "SpindleDirection") and self.obj.SpindleDirection is not None:
            if self.obj.SpindleDirection.lower() in ("cw", "forward"):
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
    def create_bit_from_dict(self, attrs: Mapping, filepath: Optional[pathlib.Path] = None) -> Any:
        """
        Given a dictionary as read from json.loads('file.fctb'), this method creates
        a new ToolBit attached to a DocumentObject and returns the DocumentObject.
        """
        Path.Log.track(attrs)
        shape_file = attrs.get("shape", "endmill.fcstd")
        params_dict = attrs.get("parameter", {})
        attributes = attrs.get("attribute", {})

        # Create the tool bit shape.
        try:
            uri = ToolBitShape.resolve_name(shape_file)
            tool_bit_shape = asset_manager.get(uri, store='shapestore')
            tool_bit_shape.set_parameters(**params_dict)
        except Exception as e:
            Path.Log.error(
                f"Failed to create shape from attributes for '{shape_file}': {e}."
                " Tool bit creation failed."
            )
            raise
        Path.Log.debug(
            f"Create shape from attributes for '{shape_file}' with ID {tool_bit_shape.get_id()}."
        )

        # Find the correct ToolBit subclass based on the shape name
        tool_bit_classes = {b.SHAPE_CLASS.name: b for b in ToolBit.__subclasses__()}
        tool_bit_class = tool_bit_classes[tool_bit_shape.name]

        # Create the ToolBit instance attached to a DocumentObject
        # Assuming FreeCAD.ActiveDocument is available in this context
        if FreeCAD.ActiveDocument is None:
             raise RuntimeError("Cannot create attached ToolBit: No active document.")

        tool_bit = tool_bit_class.create(
            FreeCAD.ActiveDocument,
            tool_bit_shape,
            path=filepath
        )

        # Set additional attributes on the ToolBit object using the proxy
        # TODO: This should probably be cleaned up
        for att in attributes:
            # Check if the property exists before setting it
            if hasattr(tool_bit.obj, att):
                PathUtil.setProperty(tool_bit.obj, att, attributes[att])
            else:
                Path.Log.warning(
                    f"Attribute '{att}' not found on tool bit '{tool_bit.obj.Label}'. Skipping."
                )

        return tool_bit.obj

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
            "parameter": {},
            "attribute": {},
        }

        return self.create_bit_from_dict(attrs, filepath=filepath)


Factory = ToolBitFactory()
