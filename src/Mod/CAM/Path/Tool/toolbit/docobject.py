import FreeCAD
import Path


class DetachedDocumentObject:
    """
    A lightweight class mimicking the property API of a FreeCAD DocumentObject.

    This class is used by ToolBit instances when they are not associated
    with a real FreeCAD DocumentObject, allowing properties to be stored
    and accessed in a detached state.
    """

    def __init__(self, label="DetachedObject"):
        self.Label = label
        self.Name = label.replace(" ", "_")
        self.PropertiesList = []
        self._properties = {}
        self._property_groups = {}
        self._property_types = {}
        self._property_docs = {}
        self._editor_modes = {}
        self._property_enums = {}  # Stores list of choices for enum properties

    def addProperty(self, thetype, name, group, doc):
        """Mimics FreeCAD DocumentObject.addProperty."""
        if name not in self._properties:
            self.PropertiesList.append(name)
            self._properties[name] = None
            self._property_groups[name] = group
            self._property_types[name] = thetype
            self._property_docs[name] = doc

    def getPropertyByName(self, name):
        """Mimics FreeCAD DocumentObject.getPropertyByName."""
        return self._properties.get(name)

    def setPropertyByName(self, name, value):
        """Mimics FreeCAD DocumentObject.setPropertyByName."""
        if name not in self._properties:
            self.addProperty(None, name, None, None)
        self._properties[name] = value

    def __setattr__(self, name, value):
        """
        Intercept attribute assignment. This is done to behave like
        FreeCAD's DocumentObject, which may have any property assigned,
        pre-defined or not.
        Without this, code linters report an error when trying to set
        a property that is not defined in the class.

        Handles assignment of enumeration choices (lists/tuples) and
        converts string representations of Quantity types to Quantity objects.
        """
        if name in ("PropertiesList", "Label", "Name") or name.startswith("_"):
            super().__setattr__(name, value)
            return

        # Handle assignment of enumeration choices (list/tuple)
        prop_type = self._property_types.get(name)
        if prop_type == "App::PropertyEnumeration" and \
           isinstance(value, (list, tuple)):
            self._property_enums[name] = list(value)
            assert len(value) > 0, f"Enum property '{name}' must have at least one entry"
            self._properties.setdefault(name, value[0])
            return

        # Attempt to convert string values to Quantity if the property type is Quantity
        elif prop_type in [
            "App::PropertyQuantity",
            "App::PropertyLength",
            "App::PropertyArea",
            "App::PropertyVolume",
            "App::PropertyAngle",
        ]:
            value = FreeCAD.Units.Quantity(value)

        # Store the (potentially converted) value
        self._properties[name] = value
        Path.Log.debug(
            f"DetachedDocumentObject: Set property '{name}' to "
            f"value {value} (type: {type(value)})"
        )

    def __getattr__(self, name):
        """Intercept attribute access."""
        if name in self._properties:
            return self._properties[name]
        # Default behaviour: raise AttributeError
        raise AttributeError(
            f"'{type(self).__name__}' object has no attribute '{name}'"
        )

    def setEditorMode(self, name, mode):
        """Stores editor mode settings in detached state."""
        self._editor_modes[name] = mode

    def getGroupOfProperty(self, name):
        """Returns the stored group for a property in detached state."""
        return self._property_groups.get(name)

    def getTypeIdOfProperty(self, name):
        """Returns the stored type string for a property in detached state."""
        return self._property_types.get(name)

    def getEnumerationsOfProperty(self, name):
        """Returns the stored enumeration list for a property."""
        return self._property_enums.get(name, [])

    @property
    def ExpressionEngine(self):
        """Mimics the ExpressionEngine attribute of a real DocumentObject."""
        return [] # Return an empty list to satisfy iteration

    def copy_to(self, obj):
        """
        Copies properties from this detached object to a real DocumentObject.
        """
        for prop_name in self.PropertiesList:
            if not hasattr(self, prop_name):
                continue

            prop_value = self.getPropertyByName(prop_name)
            prop_type = self._property_types.get(prop_name)
            prop_group = self._property_groups.get(prop_name)
            prop_doc = self._property_docs.get(prop_name, "")
            prop_editor_mode = self._editor_modes.get(prop_name)

            # If the property doesn't exist in the target object, add it
            if not hasattr(obj, prop_name):
                # For enums, addProperty expects "App::PropertyEnumeration"
                # The list of choices is set afterwards.
                obj.addProperty(prop_type, prop_name, prop_group, prop_doc)

            # If it's an enumeration, set its list of choices first
            if prop_type == "App::PropertyEnumeration":
                enum_choices = self._property_enums.get(prop_name)
                assert enum_choices is not None
                setattr(obj, prop_name, enum_choices)

            # Set the property value and editor mode
            try:
                if prop_type == "App::PropertyEnumeration":
                    first_choice = self._property_enums[prop_name][0]
                    setattr(obj, prop_name, first_choice)
                else:
                    setattr(obj, prop_name, prop_value)

            except Exception as e:
                Path.Log.error(
                    f"Error setting property {prop_name} to {prop_value} "
                    f"(type: {type(prop_value)}, expected type: {prop_type}): {e}"
                )
                raise

            if prop_editor_mode is not None:
                obj.setEditorMode(prop_name, prop_editor_mode)
