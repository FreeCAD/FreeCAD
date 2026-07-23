# SPDX-License-Identifier: LGPL-2.1-or-later

"""Flight, component-group, and view-provider proxies."""

from importlib import import_module

import FreeCAD


QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

from .object_utils import (
    _add_property,
    _is_circular_flight,
    _is_landing_flight,
    get_flights,
    sync_circular_radii,
    sync_flight_side_lengths,
)

class FlightProxy:
    """Stores the dimensions and turn direction of one stair flight."""

    Type = "Flight"

    def __init__(self, obj, stair=None):
        self._updating = True
        obj.Proxy = self
        self.set_properties(obj)
        if stair:
            obj.StairName = stair.Name
        self._updating = False

    def set_properties(self, obj):
        _add_property(
            obj,
            "App::PropertyString",
            "StairName",
            "Flight",
            "The internal name of the owning stair",
            editor_mode=2,
        )
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "FlightType",
            "Flight",
            "The flight geometry type",
        )
        flight_type = "Straight" if added else str(obj.FlightType)
        flight_types = [
            "Straight",
            "Circular",
            "Straight landing",
            "Circular landing",
        ]
        obj.FlightType = flight_types
        obj.FlightType = (
            flight_type if flight_type in flight_types else "Straight"
        )
        for side in ("Left", "Right"):
            property_name = f"{side}StringerType"
            added = _add_property(
                obj,
                "App::PropertyEnumeration",
                property_name,
                "Stringers",
                "The stringer type on this side of this flight",
            )
            current_type = (
                "None" if added else str(getattr(obj, property_name))
            )
            if current_type == "Cut stringer":
                current_type = "Notched stringer"
            stringer_types = [
                "None",
                "Housed stringer",
                "Notched stringer",
            ]
            setattr(obj, property_name, stringer_types)
            setattr(
                obj,
                property_name,
                current_type
                if current_type in stringer_types
                else "None",
            )
            _add_property(
                obj,
                "App::PropertyBool",
                f"{side}HandrailEnabled",
                "Handrails",
                f"Create a handrail on the {side.lower()} side of this flight",
                False,
            )
        _add_property(
            obj, "App::PropertyLength", "LeftLength", "Flight", "Left-side length", 3500.0
        )
        _add_property(
            obj, "App::PropertyLength", "RightLength", "Flight", "Right-side length", 3500.0
        )
        _add_property(
            obj, "App::PropertyLength", "Width", "Flight", "Flight width", 1000.0
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "InnerRadius",
            "Flight",
            "Inner radius of a circular flight",
            500.0,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "OuterRadius",
            "Flight",
            "Outer radius of a circular flight",
            1500.0,
        )
        _add_property(
            obj,
            "App::PropertyAngle",
            "Angle",
            "Flight",
            "Turn angle or circular-flight sweep angle",
            0.0,
        )
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "Rotation",
            "Flight",
            "Direction of the turn into this flight",
        )
        if added:
            obj.Rotation = ["Left", "Right"]
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "TurnType",
            "Flight",
            "How the turn into this flight is resolved",
        )
        if added:
            obj.TurnType = ["Herse balancing", "Landing"]
        _add_property(
            obj,
            "App::PropertyPercent",
            "WindingLocal",
            "Winding",
            "Winding adjustment for steps nearest the inner corner",
            50,
        )
        _add_property(
            obj,
            "App::PropertyPercent",
            "WindingDistant",
            "Winding",
            "Winding adjustment for steps farther from the inner corner",
            50,
        )
        _add_property(
            obj,
            "App::PropertyAngle",
            "StartAngle",
            "Flight",
            "Angle of the stair footprint's start edge",
            0.0,
        )
        _add_property(
            obj,
            "App::PropertyAngle",
            "EndAngle",
            "Flight",
            "Angle of the stair footprint's end edge",
            0.0,
        )
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "EntryDirection",
            "Flight",
            "Direction from which the user enters the stair",
        )
        if added:
            obj.EntryDirection = ["Straight", "From left", "From right"]
        added = _add_property(
            obj,
            "App::PropertyEnumeration",
            "ExitDirection",
            "Flight",
            "Direction in which the user leaves the stair",
        )
        if added:
            obj.ExitDirection = ["Straight", "To left", "To right"]
        _add_property(
            obj,
            "App::PropertyInteger",
            "NumberOfTreads",
            "Flight check",
            "The number of manufactured treads assigned to this flight",
            editor_mode=1,
        )
        _add_property(
            obj,
            "App::PropertyLength",
            "TreadWidth",
            "Flight check",
            "The computed going in this flight",
            editor_mode=1,
        )
        self._update_dimension_visibility(obj)

    @staticmethod
    def _update_dimension_visibility(obj):
        circular = _is_circular_flight(obj)
        for name in ("LeftLength", "RightLength"):
            obj.setEditorMode(name, 2 if circular else 0)
        for name in ("InnerRadius", "OuterRadius"):
            obj.setEditorMode(name, 0 if circular else 2)

    def onChanged(self, obj, prop):
        if (
            getattr(self, "_updating", False)
            or FreeCAD.isRestoring()
            or obj.Document.Transacting
        ):
            return
        geometry_properties = {
            "FlightType",
            "LeftLength",
            "RightLength",
            "Width",
            "InnerRadius",
            "OuterRadius",
            "Angle",
            "Rotation",
            "TurnType",
            "WindingLocal",
            "WindingDistant",
            "StartAngle",
            "EndAngle",
            "EntryDirection",
            "ExitDirection",
        }
        stringer_properties = {
            "LeftStringerType",
            "RightStringerType",
        }
        handrail_properties = {
            "LeftHandrailEnabled",
            "RightHandrailEnabled",
        }
        if (
            prop in geometry_properties
            or prop in stringer_properties
            or prop in handrail_properties
        ):
            stair = obj.Document.getObject(obj.StairName)
            if stair and getattr(stair, "Proxy", None):
                flights = get_flights(stair)
                if prop in geometry_properties and prop == "FlightType":
                    self._update_dimension_visibility(obj)
                    if _is_circular_flight(obj) or _is_landing_flight(obj):
                        self._updating = True
                        try:
                            obj.Angle = 90.0
                            if _is_circular_flight(obj):
                                obj.StartAngle = 0.0
                                obj.EndAngle = 0.0
                                obj.EntryDirection = "Straight"
                                obj.ExitDirection = "Straight"
                        finally:
                            self._updating = False
                if prop in geometry_properties and prop in {
                    "InnerRadius",
                    "OuterRadius",
                }:
                    sync_circular_radii(obj, prop)
                elif (
                    prop in geometry_properties
                    and _is_circular_flight(obj)
                    and prop in {"FlightType", "Width"}
                ):
                    sync_circular_radii(obj)
                elif prop in geometry_properties and prop in {
                    "LeftLength",
                    "RightLength",
                }:
                    sync_flight_side_lengths(stair, obj, prop)
                elif prop in geometry_properties and prop in {
                    "StartAngle",
                    "EndAngle",
                }:
                    sync_flight_side_lengths(stair, obj, "LeftLength")
                elif prop in geometry_properties and prop in {
                    "FlightType",
                    "Width",
                    "Angle",
                    "Rotation",
                }:
                    sync_flight_side_lengths(stair, obj)
                if prop in geometry_properties and prop in {
                    "FlightType",
                    "Width",
                    "Angle",
                    "Rotation",
                } and obj in flights:
                    index = flights.index(obj)
                    if index:
                        sync_flight_side_lengths(stair, flights[index - 1])
                    if index + 1 < len(flights):
                        sync_flight_side_lengths(stair, flights[index + 1])
                stair.Proxy.rebuild(stair, allow_structure_changes=True)

    def onDocumentRestored(self, obj):
        self._updating = True
        self.set_properties(obj)
        self._updating = False

    def dumps(self):
        return None

    def loads(self, state):
        self._updating = False


class ComponentGroupProxy:
    """Marks a generated component group and links it to its stair."""

    Type = "StairDesignerComponentGroup"

    def __init__(self, obj, stair=None, section="stairs"):
        obj.Proxy = self
        self.Section = section
        _add_property(
            obj,
            "App::PropertyString",
            "StairName",
            "Stair Designer",
            "The internal name of the owning stair",
            stair.Name if stair else "",
            editor_mode=2,
        )
        _add_property(
            obj,
            "App::PropertyString",
            "PanelSection",
            "Stair Designer",
            "The task panel opened for this group",
            section,
            editor_mode=2,
        )

    def onDocumentRestored(self, obj):
        self.Section = getattr(obj, "PanelSection", "stairs")

    def dumps(self):
        return self.Section

    def loads(self, state):
        self.Section = state or "stairs"


class ViewProviderStair:
    """View provider for the Stair Designer root object."""

    def __init__(self, vobj):
        if not vobj.hasExtension(
            "Gui::ViewProviderGeoFeatureGroupExtensionPython"
        ):
            vobj.addExtension(
                "Gui::ViewProviderGeoFeatureGroupExtensionPython"
            )
        vobj.Proxy = self
        self.Object = vobj.Object

    def attach(self, vobj):
        self.Object = vobj.Object

    def getIcon(self):
        import_module("Arch_rc")

        return ":/icons/Arch_Stairs_Tree.svg"

    def getTransactionText(self):
        return QT_TRANSLATE_NOOP("Command", "Edit Stair Designer")

    def doubleClicked(self, vobj):
        if FreeCAD.GuiUp:
            import FreeCADGui

            FreeCADGui.ActiveDocument.setEdit(vobj.Object.Name, 0)
        return True

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None
        import FreeCADGui

        task_panel = import_module("stairdesigner.taskpanel").StairDesignerTaskPanel

        if not FreeCADGui.Control.activeDialog():
            FreeCADGui.Control.showDialog(task_panel(vobj.Object))
        return True

    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None
        import FreeCADGui

        FreeCADGui.Control.closeDialog()
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None


class ViewProviderComponentGroup:
    """Opens the Stair Designer on the component group's tab."""

    def __init__(self, vobj):
        vobj.Proxy = self
        self.Object = vobj.Object

    def attach(self, vobj):
        self.Object = vobj.Object

    def getIcon(self):
        import_module("Arch_rc")

        return {
            "handrails": ":/icons/Arch_Handrail_Tree.svg",
            "stringers": ":/icons/Arch_Stringer_Tree.svg",
        }.get(
            getattr(self.Object, "PanelSection", "stairs"),
            ":/icons/Arch_Stairs_Tree.svg",
        )

    def getTransactionText(self):
        return QT_TRANSLATE_NOOP("Command", "Edit Stair Designer component")

    def doubleClicked(self, vobj):
        if FreeCAD.GuiUp:
            import FreeCADGui

            FreeCADGui.ActiveDocument.setEdit(vobj.Object.Name, 0)
        return True

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None
        import FreeCADGui

        task_panel = import_module("stairdesigner.taskpanel").StairDesignerTaskPanel

        stair = vobj.Object.Document.getObject(vobj.Object.StairName)
        if stair and not FreeCADGui.Control.activeDialog():
            section = getattr(vobj.Object, "PanelSection", "stairs")
            FreeCADGui.Control.showDialog(
                task_panel(
                    stair,
                    edit_object=vobj.Object,
                    active_section=section,
                )
            )
        return True

    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None
        import FreeCADGui

        FreeCADGui.Control.closeDialog()
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None
