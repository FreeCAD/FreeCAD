# SPDX-License-Identifier: LGPL-2.1-or-later

"""Generated component groups and part lifecycle helpers."""

import FreeCAD


QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

from .object_proxies import (
    ComponentGroupProxy,
    ViewProviderComponentGroup,
)

from .object_utils import (
    _add_property,
    _quantity_value,
)

def _make_component_group(stair, property_name, label, section):
    group = stair.Document.addObject("App::DocumentObjectGroupPython", f"{stair.Name}_{label}")
    group.Label = translate("BIM", label)
    ComponentGroupProxy(group, stair, section)
    if FreeCAD.GuiUp:
        ViewProviderComponentGroup(group.ViewObject)
    stair.addObject(group)
    setattr(stair, property_name, group)
    return group


def _set_generated_properties(obj, stair, role):
    _add_property(
        obj,
        "App::PropertyString",
        "GeneratedBy",
        "Stair Designer",
        "Name of the owning Stair Designer object",
        stair.Name,
        editor_mode=2,
    )
    _add_property(
        obj,
        "App::PropertyString",
        "StairDesignerRole",
        "Stair Designer",
        "Generated component role",
        role,
        editor_mode=2,
    )
    _add_property(
        obj,
        "App::PropertyInteger",
        "Index",
        "Stair Designer",
        "Sequential component index",
        0,
        editor_mode=1,
    )
    _add_property(
        obj,
        "App::PropertyInteger",
        "FlightIndex",
        "Stair Designer",
        "One-based index of the owning flight",
        1,
        editor_mode=1,
    )
    if "Material" in obj.PropertiesList:
        obj.removeProperty("Material")


def _set_tread_properties(tread):
    for name, description in (
        (
            "ExtraWidth",
            "Signed adjustment to the usable going of this tread",
        ),
        (
            "ExtraHeight",
            "Signed adjustment to the rise below this tread",
        ),
    ):
        value = 0.0
        if (
            name in tread.PropertiesList
            and tread.getTypeIdOfProperty(name) != "App::PropertyDistance"
        ):
            value = _quantity_value(getattr(tread, name))
            tread.setPropertyStatus(name, "-LockDynamic")
            tread.removeProperty(name)
        _add_property(
            tread,
            "App::PropertyDistance",
            name,
            "Step",
            description,
            value,
        )


def _tread_extra_widths(stair, tread_count):
    """Return stored per-tread adjustments in global stair order."""

    result = [0.0] * max(int(tread_count), 0)
    group = getattr(stair, "StepsGroup", None)
    if not group:
        return result
    for tread in _generated_parts(group, stair, "Tread"):
        index = int(getattr(tread, "Index", 0)) - 1
        if (
            0 <= index < len(result)
            and "ExtraWidth" in tread.PropertiesList
        ):
            result[index] = _quantity_value(tread.ExtraWidth)
    return result


def _tread_extra_heights(stair, tread_count):
    """Return stored per-riser adjustments in global stair order."""

    result = [0.0] * max(int(tread_count), 0)
    group = getattr(stair, "StepsGroup", None)
    if not group:
        return result
    for tread in _generated_parts(group, stair, "Tread"):
        index = int(getattr(tread, "Index", 0)) - 1
        if (
            0 <= index < len(result)
            and "ExtraHeight" in tread.PropertiesList
        ):
            result[index] = _quantity_value(tread.ExtraHeight)
    return result


def _generated_parts(group, stair, role):
    parts = [
        child
        for child in group.Group
        if getattr(child, "GeneratedBy", "") == stair.Name
        and getattr(child, "StairDesignerRole", "") == role
    ]
    for child in parts:
        if "Material" in child.PropertiesList:
            child.removeProperty("Material")
    return sorted(parts, key=lambda child: getattr(child, "Index", 0))


def _resize_generated_parts(group, stair, role, count):
    parts = _generated_parts(group, stair, role)
    while len(parts) < count:
        part = stair.Document.addObject("Part::Feature", f"{stair.Name}_{role}")
        _set_generated_properties(part, stair, role)
        group.addObject(part)
        parts.append(part)
    while len(parts) > count:
        part = parts.pop()
        group.removeObject(part)
        stair.Document.removeObject(part.Name)
    return parts
