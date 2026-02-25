# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
# *                 2025 Samuel Abels <knipknap@gmail.com>                  *
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
import uuid
import pathlib
from abc import ABC
from itertools import chain
from lazy_loader.lazy_loader import LazyLoader
from typing import Any, List, Optional, Tuple, Type, Union, Mapping, cast
from PySide.QtCore import QT_TRANSLATE_NOOP
from Path.Base.Generator import toolchange
from ...docobject import DetachedDocumentObject
from ...assets.asset import Asset
from ...camassets import cam_assets
from ...shape import ToolBitShape, ToolBitShapeCustom, ToolBitShapeIcon
from ..util import to_json, format_value
from ..migration import ParameterAccessor, migrate_parameters


ToolBitView = LazyLoader("Path.Tool.toolbit.ui.view", globals(), "Path.Tool.toolbit.ui.view")


class ToolBitRecomputeObserver:
    """Document observer that triggers queued visual updates after recompute completes."""

    def __init__(self, toolbit_proxy):
        self.toolbit_proxy = toolbit_proxy

    def slotRecomputedDocument(self, doc):
        """Called when document recompute is finished."""
        # Check if the toolbit object is still valid
        try:
            obj_doc = self.toolbit_proxy.obj.Document
        except ReferenceError:
            # Object has been deleted or does not exist, nothing to do
            return

        # Only process updates for the correct document
        if doc != obj_doc:
            return

        # Process any queued visual updates
        if self.toolbit_proxy and hasattr(self.toolbit_proxy, "_process_queued_visual_update"):
            Path.Log.debug("Document recompute finished, processing queued visual update")
            self.toolbit_proxy._process_queued_visual_update()


PropertyGroupShape = "Shape"

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.ERROR, Path.Log.thisModule())


class ToolBit(Asset, ABC):
    asset_type: str = "toolbit"
    SHAPE_CLASS: Type[ToolBitShape]  # Abstract class attribute

    def __init__(self, tool_bit_shape: ToolBitShape, id: Optional[str] = None):
        Path.Log.track("ToolBit __init__ called")
        self.id = id if id is not None else str(uuid.uuid4())
        self.obj = DetachedDocumentObject()
        self.obj.Proxy = self
        self._tool_bit_shape: ToolBitShape = tool_bit_shape
        self._in_update = False

        self._create_base_properties()
        self.obj.ToolBitID = self.get_id()
        self.obj.ShapeID = tool_bit_shape.get_id()
        self.obj.ShapeType = tool_bit_shape.name
        self.obj.Label = tool_bit_shape.label or f"New {tool_bit_shape.name}"

        # Initialize properties
        self._update_tool_properties()

    def __eq__(self, other):
        """Compare ToolBit objects based on their unique ID."""
        if not isinstance(other, ToolBit):
            return False
        return self.id == other.id

    @staticmethod
    def _find_subclass_for_shape(shape: ToolBitShape) -> Type["ToolBit"]:
        """
        Finds the appropriate ToolBit subclass for a given ToolBitShape instance.
        """
        for subclass in ToolBit.__subclasses__():
            if isinstance(shape, subclass.SHAPE_CLASS):
                return subclass
        raise ValueError(f"No ToolBit subclass found for shape {type(shape).__name__}")

    @classmethod
    def from_dict(cls, attrs: Mapping, shallow: bool = False) -> "ToolBit":
        """
        Creates and populates a ToolBit instance from a dictionary.
        """
        # Find the shape ID.
        shape_id = pathlib.Path(
            str(attrs.get("shape", ""))
        ).stem  # backward compatibility. used to be a filename
        if not shape_id:
            raise ValueError("ToolBit dictionary is missing 'shape' key")

        # Try to find the shape type. Default to Unknown if necessary.
        if "shape" in attrs and "shape-type" not in attrs:
            attrs["shape-type"] = attrs["shape"]
        shape_type = attrs.get("shape-type")
        shape_class = ToolBitShape.get_shape_class_from_id(shape_id, shape_type)
        if not shape_class:
            Path.Log.debug(
                f"Failed to find usable shape for ID '{shape_id}'"
                f" (shape type {shape_type}). Falling back to 'Unknown'"
            )
            shape_class = ToolBitShapeCustom

        # Create a ToolBitShape instance.
        if not shallow:  # Shallow means: skip loading of child assets
            shape_asset_uri = ToolBitShape.resolve_name(shape_id)
            try:
                tool_bit_shape = cast(ToolBitShape, cam_assets.get(shape_asset_uri))
            except FileNotFoundError:
                Path.Log.debug(f"ToolBit.from_dict: Shape asset {shape_asset_uri} not found.")
                # Rely on the fallback below
            else:
                return cls.from_shape(tool_bit_shape, attrs, id=attrs.get("id"))

        # Ending up here means we either could not load the shape asset,
        # or we are in shallow mode and do not want to load it.
        # Create a shape instance from scratch as a "placeholder".
        params = attrs.get("parameter", {})
        tool_bit_shape = shape_class(shape_id, **params)
        Path.Log.debug(
            f"ToolBit.from_dict: created shape instance {tool_bit_shape.name}"
            f" from {shape_id}. Uri: {tool_bit_shape.get_uri()}"
        )

        # Now that we have a shape, create the toolbit instance.
        return cls.from_shape(tool_bit_shape, attrs, id=attrs.get("id"))

    @classmethod
    def from_shape(
        cls,
        tool_bit_shape: ToolBitShape,
        attrs: Mapping,
        id: Optional[str] = None,
    ) -> "ToolBit":
        selected_toolbit_subclass = cls._find_subclass_for_shape(tool_bit_shape)
        toolbit = selected_toolbit_subclass(tool_bit_shape, id=id)
        toolbit.label = attrs.get("name") or tool_bit_shape.label

        # Get params and attributes.
        params = attrs.get("parameter", {})
        attr = attrs.get("attribute", {})

        # Filter parameters if method exists
        if (
            hasattr(tool_bit_shape.__class__, "filter_parameters")
            and callable(getattr(tool_bit_shape.__class__, "filter_parameters"))
            and isinstance(params, dict)
        ):
            params = tool_bit_shape.__class__.filter_parameters(params)

        # Update parameters.
        for param_name, param_value in params.items():
            tool_bit_shape.set_parameter(param_name, param_value)
            if hasattr(toolbit.obj, param_name):
                PathUtil.setProperty(toolbit.obj, param_name, param_value)

        # Update attributes; the separation between parameters and attributes
        # is currently not well defined, so for now we add them to the
        # ToolBitShape and the DocumentObject.
        # Discussion: https://github.com/FreeCAD/FreeCAD/issues/21722
        for attr_name, attr_value in attr.items():
            tool_bit_shape.set_parameter(attr_name, attr_value)
            if hasattr(toolbit.obj, attr_name):
                PathUtil.setProperty(toolbit.obj, attr_name, attr_value)
            else:
                Path.Log.debug(
                    f"ToolBit {id} Attribute '{attr_name}' not found on"
                    f" {selected_toolbit_subclass.__name__} ({tool_bit_shape})"
                    f" '{toolbit.obj.Label}'. Skipping."
                )

        toolbit._update_tool_properties()
        return toolbit

    @classmethod
    def from_shape_id(cls, shape_id: str, label: Optional[str] = None) -> "ToolBit":
        """
        Creates and populates a ToolBit instance from a shape ID.
        """
        attrs = {"shape": shape_id, "name": label}
        return cls.from_dict(attrs)

    @classmethod
    def from_file(cls, path: Union[str, pathlib.Path]) -> "ToolBit":
        """
        Creates and populates a ToolBit instance from a .fctb file.
        """
        path = pathlib.Path(path)
        with path.open("r") as fp:
            attrs_map = json.load(fp)
        return cls.from_dict(attrs_map)

    @property
    def label(self) -> str:
        return self.obj.Label

    @label.setter
    def label(self, label: str):
        self.obj.Label = label

    def get_shape_name(self) -> str:
        """Returns the shape name of the tool bit."""
        return self._tool_bit_shape.name

    def set_shape_name(self, name: str):
        """Sets the shape name of the tool bit."""
        self._tool_bit_shape.name = name

    @property
    def summary(self) -> str:
        """
        To be overridden by subclasses to provide a better summary
        including parameter values. Used as "subtitle" for the tool
        in the UI.

        Example: "3.2 mm endmill, 4-flute, 8 mm cutting edge"
        """
        return self.get_shape_name()

    def _create_base_properties(self):
        # Create the properties in the Base group.
        if not hasattr(self.obj, "ShapeID"):
            self.obj.addProperty(
                "App::PropertyString",
                "ShapeID",
                "Base",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The unique ID of the tool shape (.fcstd)",
                ),
            )
        if not hasattr(self.obj, "ShapeType"):
            self.obj.addProperty(
                "App::PropertyEnumeration",
                "ShapeType",
                "Base",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The tool shape type",
                ),
            )
            names = [c.name for c in ToolBitShape.__subclasses__()]
            self.obj.ShapeType = names
            self.obj.ShapeType = ToolBitShapeCustom.name
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
        if not hasattr(self.obj, "ToolBitID"):
            self.obj.addProperty(
                "App::PropertyString",
                "ToolBitID",
                "Base",
                QT_TRANSLATE_NOOP("App::Property", "The unique ID of the toolbit"),
            )

        # 0 = read/write, 1 = read only, 2 = hide
        self.obj.setEditorMode("ShapeID", 1)
        self.obj.setEditorMode("ShapeType", 1)
        self.obj.setEditorMode("ToolBitID", 1)
        self.obj.setEditorMode("BitBody", 2)
        self.obj.setEditorMode("Shape", 2)

        # Create the ToolBit properties that are shared by all tool bits

        if not hasattr(self.obj, "Units"):
            self.obj.addProperty(
                "App::PropertyEnumeration",
                "Units",
                "Attributes",
                QT_TRANSLATE_NOOP("App::Property", "Measurement units for the tool bit"),
            )
            self.obj.Units = ["Metric", "Imperial"]
            self.obj.Units = "Metric"  # Default value
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

    def get_id(self) -> str:
        """Returns the unique ID of the tool bit."""
        return self.id

    def set_id(self, id: str = None):
        self.id = id if id is not None else str(uuid.uuid4())

    def _promote_toolbit(self):
        """
        Updates the toolbit properties for backward compatibility.
        Ensure obj.ShapeID and obj.ToolBitID are set, handling legacy cases.
        Also promotes embedded toolbits to correct shape type if needed.
        """
        Path.Log.track(f"Promoting tool bit {self.obj.Label}")

        # Ensure ShapeID is set (handling legacy BitShape/ShapeName)
        name = None
        if hasattr(self.obj, "ShapeID") and self.obj.ShapeID:
            name = self.obj.ShapeID
        elif hasattr(self.obj, "ShapeFile") and self.obj.ShapeFile:
            name = pathlib.Path(self.obj.ShapeFile).stem
        elif hasattr(self.obj, "BitShape") and self.obj.BitShape:
            name = pathlib.Path(self.obj.BitShape).stem
        elif hasattr(self.obj, "ShapeName") and self.obj.ShapeName:
            name = pathlib.Path(self.obj.ShapeName).stem
        if name is None:
            raise ValueError("ToolBit is missing a shape ID")

        uri = ToolBitShape.resolve_name(name)
        if uri is None:
            raise ValueError(f"Failed to identify ID of ToolBit from '{name}'")
        self.obj.ShapeID = uri.asset_id

        # Ensure ShapeType is set
        thetype = None
        if hasattr(self.obj, "ShapeType") and self.obj.ShapeType:
            thetype = self.obj.ShapeType
        elif hasattr(self.obj, "ShapeFile") and self.obj.ShapeFile:
            thetype = pathlib.Path(self.obj.ShapeFile).stem
        elif hasattr(self.obj, "BitShape") and self.obj.BitShape:
            thetype = pathlib.Path(self.obj.BitShape).stem
        elif hasattr(self.obj, "ShapeName") and self.obj.ShapeName:
            thetype = pathlib.Path(self.obj.ShapeName).stem
        if thetype is None:
            raise ValueError("ToolBit is missing a shape type")

        shape_class = ToolBitShape.get_subclass_by_name(thetype)
        if shape_class is None:
            raise ValueError(f"Failed to identify shape of ToolBit from '{thetype}'")
        self.obj.ShapeType = shape_class.name

        # Promote embedded toolbits to correct shape type if still Custom
        if self.obj.ShapeType == "Custom":
            shape_id = getattr(self.obj, "ShapeID", None)
            if shape_id:
                shape_class = ToolBitShape.get_subclass_by_name(shape_id)
                if shape_class and shape_class.name != "Custom":
                    self.obj.ShapeType = shape_class.name
                    self._tool_bit_shape = shape_class(shape_id)
                    Path.Log.info(
                        f"Promoted embedded toolbit '{self.obj.Label}' to shape '{shape_class.name}' via ShapeID"
                    )
        # Ensure ToolBitID is set
        if hasattr(self.obj, "File"):
            self.id = pathlib.Path(self.obj.File).stem
        self.obj.ToolBitID = self.id
        Path.Log.debug(f"Set ToolBitID to {self.obj.ToolBitID}")

        # Update SpindleDirection:
        # Old tools may still have "CCW", "CW", "Off", "None".
        # New tools use "None", "Forward", "Reverse".
        normalized_direction = old_direction = self.obj.SpindleDirection

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

        # Drop legacy properties.
        legacy = "ShapeFile", "File", "BitShape", "ShapeName"
        for name in legacy:
            if hasattr(self.obj, name):
                value = getattr(self.obj, name)
                self.obj.removeProperty(name)
                Path.Log.debug(f"Removed obsolete property '{name}' ('{value}').")

        # Get the schema properties from the current shape
        shape_cls = ToolBitShape.get_subclass_by_name(self.obj.ShapeType)
        if not shape_cls:
            raise ValueError(f"Failed to find shape class named '{self.obj.ShapeType}'")
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
        Path.Log.track(obj.Label)

        # Assign self.obj to the restored object
        self.obj = obj
        self.obj.Proxy = self
        if not hasattr(self, "id"):
            self.id = str(uuid.uuid4())
            Path.Log.debug(
                f"Assigned new id {self.id} for ToolBit {obj.Label} during document restore"
            )

        # Our constructor previously created the base properties in the
        # DetachedDocumentObject, which was now replaced.
        # So here we need to ensure to set them up in the new (real) DocumentObject
        # as well.
        self._create_base_properties()
        self._promote_toolbit()

        # Get the shape instance based on the ShapeType. We try two approaches
        # to find the shape and shape class:
        #   1. If the asset with the given type exists, use that.
        #   2. Otherwise create a new empty instance
        shape_uri = ToolBitShape.resolve_name(self.obj.ShapeType)
        try:
            # Best case: we directly find the shape file in our assets.
            self._tool_bit_shape = cast(ToolBitShape, cam_assets.get(shape_uri))
        except FileNotFoundError:
            # Otherwise, try to at least identify the type of the shape.
            shape_class = ToolBitShape.get_subclass_by_name(shape_uri.asset_id)
            if not shape_class:
                raise ValueError(
                    "Failed to identify class of ToolBitShape from name "
                    f"'{self.obj.ShapeType}' (asset id {shape_uri.asset_id})"
                )
            self._tool_bit_shape = shape_class(shape_uri.asset_id)

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
                self.obj.ViewObject.Proxy, ToolBitView.ViewProvider
            ):
                Path.Log.debug(f"onDocumentRestored: Attaching ViewProvider for {self.obj.Label}")
                ToolBitView.ViewProvider(self.obj.ViewObject, "ToolBit")

        # Migrate legacy parameters using unified accessor
        migrate_parameters(ParameterAccessor(obj))

        # Filter parameters if method exists (removes FlatRadius from obj)
        filter_func = getattr(self._tool_bit_shape.__class__, "filter_parameters", None)
        if callable(filter_func):
            # Only filter if FlatRadius is present
            if "FlatRadius" in self.obj.PropertiesList:
                try:
                    self.obj.removeProperty("FlatRadius")
                    Path.Log.info(f"Filtered out FlatRadius for {self.obj.Label}")
                except Exception as e:
                    Path.Log.error(f"Failed to remove FlatRadius for {self.obj.Label}: {e}")

        # Copy properties from the restored object to the ToolBitShape.
        for name, item in self._tool_bit_shape.schema().items():
            if name in self.obj.PropertiesList:
                value = self.obj.getPropertyByName(name)
                self._tool_bit_shape.set_parameter(name, value)

        # Ensure property state is correct after restore.
        self._update_tool_properties()

    def attach_to_doc(
        self, doc: FreeCAD.Document, label: Optional[str] = None
    ) -> FreeCAD.DocumentObject:
        """
        Creates a new FreeCAD DocumentObject in the given document and attaches
        this ToolBit instance to it.
        """
        label = label or self.label or self._tool_bit_shape.label
        tool_doc_obj = doc.addObject("Part::FeaturePython", label)
        self.attach_to_obj(tool_doc_obj, label=label)
        return tool_doc_obj

    def attach_to_obj(self, tool_doc_obj: FreeCAD.DocumentObject, label: Optional[str] = None):
        """
        Attaches the ToolBit instance to an existing FreeCAD DocumentObject.

        Transfers properties from the internal DetachedDocumentObject to the
        tool_doc_obj and updates the visual representation.
        """
        if not isinstance(self.obj, DetachedDocumentObject):
            Path.Log.warning(
                f"ToolBit {self.obj.Label} is already attached to a "
                "DocumentObject. Skipping attach_to_obj."
            )
            return

        Path.Log.track(f"Attaching ToolBit to {tool_doc_obj.Label}")

        temp_obj = self.obj
        self.obj = tool_doc_obj
        self.obj.Proxy = self
        if FreeCAD.GuiUp:
            ToolBitView.ViewProvider(self.obj.ViewObject, "ToolBit")

        self._create_base_properties()

        # Transfer property values from the detached object to the real object
        self._suppress_visual_update = True
        temp_obj.copy_to(self.obj)

        # Ensure label is set
        self.obj.Label = label or self.label or self._tool_bit_shape.label

        # Update the visual representation now that it's attached
        self._update_tool_properties()
        self._suppress_visual_update = False
        self._update_visual_representation()

    def onChanged(self, obj, prop):
        Path.Log.track(obj.Label, prop)
        # Avoid acting during document restore or internal updates
        if "Restore" in obj.State:
            return

        if getattr(self, "_suppress_visual_update", False):
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
                f"Queuing visual representation update."
            )
            self._tool_bit_shape.set_parameter(prop, new_value)
            self._queue_visual_update()
        finally:
            self._in_update = False

    def onDelete(self, obj, arg2=None):
        Path.Log.track(obj.Label)
        # Clean up any pending observer
        if hasattr(self, "_recompute_observer"):
            FreeCAD.removeDocumentObserver(self._recompute_observer)
            del self._recompute_observer
        self._removeBitBody()
        obj.Document.removeObject(obj.Name)

    def _removeBitBody(self):
        if self.obj.BitBody:
            self.obj.BitBody.removeObjectsFromDocument()
            self.obj.Document.removeObject(self.obj.BitBody.Name)
            self.obj.BitBody = None

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

    def get_property(self, name: str):
        return self.obj.getPropertyByName(name)

    def get_property_str(
        self, name: str, default: str | None = None, precision: int | None = None
    ) -> str | None:
        value = self.get_property(name)
        return format_value(value, precision=precision, units=self.obj.Units) if value else default

    def set_property(self, name: str, value: Any):
        return self.obj.setPropertyByName(name, value)

    def get_property_label_from_name(self, name: str):
        return self.obj.getPropertyByName

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

            if not prop_type:
                Path.Log.error(
                    f"No property type for parameter '{name}' in shape "
                    f"'{self._tool_bit_shape.name}'. Skipping."
                )
                continue

            # Add new property
            if not hasattr(self.obj, name):
                self.obj.addProperty(prop_type, name, "Shape", docstring)
                Path.Log.debug(f"Added new shape property: {name}")

            # Ensure editor mode is correct
            self.obj.setEditorMode(name, 0)

            try:
                value = self._tool_bit_shape.get_parameter(name)
            except KeyError:
                continue  # Retain existing property value.

            # Conditional to avoid unnecessary migration warning when called
            # from onDocumentRestored.
            if value is not None and getattr(self.obj, name) != value:
                PathUtil.setProperty(self.obj, name, value)

        # 2. Add additional properties that are part of the shape,
        # but not part of the schema.
        schema_prop_names = set(self._tool_bit_shape.schema().keys())
        for name, value in self._tool_bit_shape.get_parameters().items():
            if name in schema_prop_names:
                continue
            prop_type = self._tool_bit_shape.get_parameter_type(name)
            docstring = QT_TRANSLATE_NOOP("App::Property", f"Custom property from shape: {name}")

            # Skip existing properties if they have a different type
            if hasattr(self.obj, name) and self.obj.getTypeIdOfProperty(name) != prop_type:
                Path.Log.debug(
                    f"Skipping existing property '{name}' due to type mismatch."
                    f" has type {self.obj.getTypeIdOfProperty(name)}, expected {prop_type}"
                )
                continue

            # Add the property if it does not exist
            if not hasattr(self.obj, name):
                self.obj.addProperty(prop_type, name, PropertyGroupShape, docstring)
                Path.Log.debug(f"Added custom shape property: {name} ({prop_type})")

            # Set the property value
            if value is not None and getattr(self.obj, name) != value:
                PathUtil.setProperty(self.obj, name, value)
            self.obj.setEditorMode(name, 0)

        # 3. Ensure Units property exists and is set
        if not hasattr(self.obj, "Units"):
            Path.Log.debug("Adding Units property")
            self.obj.addProperty(
                "App::PropertyEnumeration",
                "Units",
                "Attributes",
                QT_TRANSLATE_NOOP("App::Property", "Measurement units for the tool bit"),
            )
            self.obj.Units = ["Metric", "Imperial"]
            self.obj.Units = "Metric"  # Default value

        units_value = self._tool_bit_shape.get_parameters().get("Units")
        if units_value in ("Metric", "Imperial") and self.obj.Units != units_value:
            PathUtil.setProperty(self.obj, "Units", units_value)

        # 4. Ensure SpindleDirection property exists and is set
        # Maybe this could be done with a global schema or added to each
        # shape schema?
        if not hasattr(self.obj, "SpindleDirection"):
            self.obj.addProperty(
                "App::PropertyEnumeration",
                "SpindleDirection",
                "Attributes",
                QT_TRANSLATE_NOOP("App::Property", "Direction of spindle rotation"),
            )
            self.obj.SpindleDirection = ["Forward", "Reverse", "None"]
            self.obj.SpindleDirection = "Forward"  # Default value

        spindle_value = self._tool_bit_shape.get_parameters().get("SpindleDirection")
        if (
            spindle_value in ("Forward", "Reverse", "None")
            and self.obj.SpindleDirection != spindle_value
        ):
            # self.obj.SpindleDirection = spindle_value
            PathUtil.setProperty(self.obj, "SpindleDirection", spindle_value)

        # 5. Ensure Material property exists and is set
        if not hasattr(self.obj, "Material"):
            self.obj.addProperty(
                "App::PropertyEnumeration",
                "Material",
                "Attributes",
                QT_TRANSLATE_NOOP("App::Property", "Tool material"),
            )
            self.obj.Material = ["HSS", "Carbide"]
            self.obj.Material = "HSS"  # Default value

        material_value = self._tool_bit_shape.get_parameters().get("Material")
        if material_value in ("HSS", "Carbide") and self.obj.Material != material_value:
            PathUtil.setProperty(self.obj, "Material", material_value)

    def _queue_visual_update(self):
        """Queue a visual update to be processed after document recompute is complete."""
        if not hasattr(self, "_visual_update_queued"):
            self._visual_update_queued = False

        if not self._visual_update_queued:
            self._visual_update_queued = True
            Path.Log.debug(f"Queuing visual update for {self.obj.Label}")

            # Set up a document observer to process the update after recompute
            self._setup_recompute_observer()

    def _setup_recompute_observer(self):
        """Set up a document observer to process queued visual updates after recompute."""
        if not hasattr(self, "_recompute_observer"):
            Path.Log.debug(f"Setting up recompute observer for {self.obj.Label}")
            self._recompute_observer = ToolBitRecomputeObserver(self)
            FreeCAD.addDocumentObserver(self._recompute_observer)

    def _process_queued_visual_update(self):
        """Process the queued visual update."""
        if hasattr(self, "_visual_update_queued") and self._visual_update_queued:
            self._visual_update_queued = False
            Path.Log.debug(f"Processing queued visual update for {self.obj.Label}")
            self._update_visual_representation()

            # Clean up the observer
            if hasattr(self, "_recompute_observer"):
                FreeCAD.removeDocumentObserver(self._recompute_observer)
                del self._recompute_observer

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

        # clear the touched state since visual updates shouldn't require recompute
        self.obj.purgeTouched()

    def to_dict(self):
        """
        Returns a dictionary representation of the tool bit.

        Returns:
            A dictionary with tool bit properties, JSON-serializable.
        """
        Path.Log.track(self.obj.Label)
        attrs = {}
        attrs["version"] = 2
        attrs["id"] = self.id
        attrs["name"] = self.obj.Label
        attrs["shape"] = self._tool_bit_shape.get_id() + ".fcstd"
        attrs["shape-type"] = self._tool_bit_shape.name
        attrs["parameter"] = {}
        attrs["attribute"] = {}

        # Store all shape parameter names and attribute names
        param_names = self._tool_bit_shape.get_parameters()
        attr_props = self._get_props("Attributes")
        property_names = list(chain(param_names, attr_props))
        for name in property_names:
            value = getattr(self.obj, name, None)
            if value is None or isinstance(value, FreeCAD.DocumentObject):
                Path.Log.warning(
                    f"Excluding property '{name}' from serialization "
                    f"(type {type(value).__name__ if value is not None else 'None'}, value {value})"
                )
            try:
                serialized_value = to_json(value)
                attrs["parameter"][name] = serialized_value
            except (TypeError, ValueError) as e:
                Path.Log.warning(
                    f"Excluding property '{name}' from serialization "
                    f"(type {type(value).__name__}, value {value}): {e}"
                )

        Path.Log.debug(f"to_dict output for {self.obj.Label}: {attrs}")
        return attrs

    def __getstate__(self):
        """
        Prepare the ToolBit for pickling by excluding non-picklable attributes.

        Returns:
            A dictionary with picklable and JSON-serializable state.
        """
        Path.Log.track("ToolBit.__getstate__")
        state = {
            "id": getattr(self, "id", str(uuid.uuid4())),  # Fallback to new UUID
            "_in_update": getattr(self, "_in_update", False),  # Fallback to False
            "_obj_data": self.to_dict(),
        }

        if not getattr(self, "_tool_bit_shape", None):
            return state

        # Store minimal shape data to reconstruct _tool_bit_shape
        state["_shape_data"] = {
            "id": self._tool_bit_shape.get_id(),
            "name": self._tool_bit_shape.name,
            "parameters": {
                name: to_json(getattr(self.obj, name, None))
                for name in self._tool_bit_shape.get_parameters()
                if not isinstance(getattr(self.obj, name, None), FreeCAD.DocumentObject)
            },
        }

        return state

    def get_spindle_direction(self) -> toolchange.SpindleDirection:
        """
        Returns the spindle direction for this toolbit.
        The direction is determined by the ToolBit's properties and safety rules:
        - Returns SpindleDirection.OFF if the tool cannot rotate (e.g., a probe).
        - Returns SpindleDirection.CW for clockwise or 'forward' spindle direction.
        - Returns SpindleDirection.CCW for counterclockwise or any other value.
        - Defaults to SpindleDirection.OFF if not specified.
        """
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
