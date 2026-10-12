# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.               #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses      #
#                                                                              #
################################################################################

"""Build a Machine from an MTConnect probe document.

An MTConnect agent describes its machine in the MTConnectDevices ("probe")
document, conventionally served at http://<host>:<port>/probe.  This module
parses the standard-namespace content of that document — Linear/Rotary axis
components, their travel and velocity Specifications, the spindle's
ROTARY_VELOCITY band, and coolant presence — and populates a Machine.

Only the standard MTConnect component tree is read.  Vendor extension
elements (foreign XML namespaces) are ignored, so any MTConnect 1.3+ agent
is a valid source; a vendor extension can only add fidelity, never break the
import.

The probe cannot know table-vs-head mounting or rotary chain order — that is
not part of the standard device model.  When rotary axes are found, the
chain is wired to the same defaults as the Machine factory configurations
(A/C and A/B as table rotaries, B/C as head rotaries) and the assumption is
recorded in the ImportReport and the machine's kinematics notes for the user
to review in the Machine Editor.

All lengths in a conforming probe are canonical millimetres; a source that
declares INCH units in its Specifications is imported as an imperial
configuration with values kept in inches.
"""

import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from typing import List, Optional, Tuple

import FreeCAD

from Machine.models.machine import (
    AxisRole,
    LinearAxis,
    Machine,
    RotaryAxis,
    Toolhead,
    ToolheadType,
)

translate = FreeCAD.Qt.translate

LINEAR_LETTERS = "XYZUVW"
ROTARY_LETTERS = "ABC"
SPINDLE_NAMES = {"S", "SPINDLE"}

# Canonical direction vectors used when the probe carries no Motion element.
AXIS_VECTORS = {
    "X": (1, 0, 0),
    "Y": (0, 1, 0),
    "Z": (0, 0, 1),
    "A": (1, 0, 0),
    "B": (0, 1, 0),
    "C": (0, 0, 1),
    "U": (1, 0, 0),
    "V": (0, 1, 0),
    "W": (0, 0, 1),
}

# Native length unit -> millimetres.
_LENGTH_TO_MM = {
    "MILLIMETER": 1.0,
    "INCH": 25.4,
    "CENTIMETER": 10.0,
    "METER": 1000.0,
}


class ProbeParseError(ValueError):
    """The document is not a usable MTConnectDevices probe."""


@dataclass
class ImportReport:
    """What an import took from the probe, assumed, or could not use."""

    device_name: str = ""
    imported: List[str] = field(default_factory=list)
    assumed: List[str] = field(default_factory=list)
    skipped: List[str] = field(default_factory=list)

    def summary(self) -> str:
        lines = []
        if self.imported:
            lines.append(translate("CAM_MachineImport", "Imported:"))
            lines.extend("  - " + item for item in self.imported)
        if self.assumed:
            lines.append(translate("CAM_MachineImport", "Assumed (please review):"))
            lines.extend("  - " + item for item in self.assumed)
        if self.skipped:
            lines.append(translate("CAM_MachineImport", "Not imported:"))
            lines.extend("  - " + item for item in self.skipped)
        return "\n".join(lines)


def _local(element) -> str:
    """Element name without its XML namespace."""
    return element.tag.rsplit("}", 1)[-1]


def _parse_root(xml_text: str):
    try:
        root = ET.fromstring(xml_text)
    except ET.ParseError as e:
        raise ProbeParseError(f"Not well-formed XML: {e}")
    if _local(root) != "MTConnectDevices":
        raise ProbeParseError(
            f"Root element is '{_local(root)}', expected 'MTConnectDevices'. "
            "Is this the /probe endpoint of an MTConnect agent?"
        )
    return root


def _device_elements(root):
    """All Device elements, excluding the agent's self-description."""
    devices = []
    for parent in root.iter():
        if _local(parent) != "Devices":
            continue
        for child in parent:
            if _local(child) == "Device":
                devices.append(child)
    return devices


def list_devices(xml_text: str) -> List[dict]:
    """Names and uuids of the machine devices described by a probe document."""
    root = _parse_root(xml_text)
    return [
        {"name": dev.get("name", ""), "uuid": dev.get("uuid", "")} for dev in _device_elements(root)
    ]


def _own_children(component, localname):
    """Descendants matched by localname without crossing into sub-components.

    A Linear/Rotary component's Configuration and DataItems belong to that
    component; anything under a nested Components element belongs to a child
    component and must not be attributed to this one.
    """
    found = []

    def walk(element):
        for child in element:
            name = _local(child)
            if name == "Components":
                continue
            if name == localname:
                found.append(child)
            walk(child)

    walk(component)
    return found


def _spec_bounds(spec) -> Tuple[Optional[float], Optional[float]]:
    minimum = maximum = None
    for child in spec.iter():
        try:
            if _local(child) == "Minimum":
                minimum = float(child.text)
            elif _local(child) == "Maximum":
                maximum = float(child.text)
        except (TypeError, ValueError):
            continue
    return minimum, maximum


def _axis_letter(name: str) -> Optional[str]:
    """Normalize a component name ('X', 'x', 'X1') to an axis letter."""
    name = (name or "").strip().upper()
    if len(name) == 1 and name in LINEAR_LETTERS + ROTARY_LETTERS:
        return name
    if len(name) > 1 and name[0] in LINEAR_LETTERS + ROTARY_LETTERS and name[1:].isdigit():
        return name[0]
    return None


def _motion_vector(component) -> Optional[FreeCAD.Vector]:
    for motion in _own_children(component, "Motion"):
        for child in motion:
            if _local(child) == "Axis" and child.text:
                try:
                    x, y, z = (float(v) for v in child.text.split())
                    return FreeCAD.Vector(x, y, z)
                except ValueError:
                    return None
    return None


@dataclass
class _ParsedComponent:
    element: object
    name: str
    kind: str  # "Linear" or "Rotary"
    letter: Optional[str]
    specs: dict  # spec type -> (units, min, max)
    dataitem_types: set
    vector: Optional[FreeCAD.Vector]


def _parse_component(component) -> _ParsedComponent:
    specs = {}
    for spec in _own_children(component, "Specification"):
        spec_type = (spec.get("type") or "").upper()
        if spec_type:
            specs[spec_type] = (spec.get("units") or "", *_spec_bounds(spec))
    dataitem_types = set()
    for item in _own_children(component, "DataItem"):
        item_type = (item.get("type") or "").upper()
        # Vendor extension types are prefixed ('x:FLOOD'); keep the local part.
        dataitem_types.add(item_type.rsplit(":", 1)[-1])
    name = component.get("name", "")
    return _ParsedComponent(
        element=component,
        name=name,
        kind=_local(component),
        letter=_axis_letter(name),
        specs=specs,
        dataitem_types=dataitem_types,
        vector=_motion_vector(component),
    )


def _axis_components(device):
    """Linear/Rotary components in the device, with parent tracking."""
    components = []

    def walk(element):
        for child in element:
            if _local(child) in ("Linear", "Rotary") and _local(element) == "Components":
                components.append(_parse_component(child))
            walk(child)

    walk(device)
    return components


def _is_spindle(comp: _ParsedComponent) -> bool:
    if comp.name.strip().upper() in SPINDLE_NAMES:
        return True
    if "ROTARY_VELOCITY" in comp.dataitem_types or "SPINDLE_SPEED" in comp.dataitem_types:
        return True
    return "ROTARY_VELOCITY" in comp.specs


def _length_factor(units: str, imperial: bool) -> float:
    """Convert a spec value in `units` to configuration units (mm or inch)."""
    to_mm = _LENGTH_TO_MM.get(units, 1.0)
    return to_mm / 25.4 if imperial else to_mm


def _velocity_to_config_units(value: float, units: str, imperial: bool) -> float:
    """Convert a velocity spec to mm/min or in/min."""
    units = units or ""
    length_unit = units.split("/", 1)[0]
    value *= _length_factor(length_unit, imperial)
    if units.endswith("/SECOND"):
        value *= 60.0
    return value


def _canonical_vector(letter: str) -> FreeCAD.Vector:
    return FreeCAD.Vector(*AXIS_VECTORS.get(letter, (0, 0, 1)))


def _wire_rotary_chain(machine: Machine, report: ImportReport):
    """Apply the factory-default chain wiring for the discovered rotary set.

    The probe cannot state table-vs-head mounting, so these follow the same
    conventions as the Machine factory configurations and are flagged for
    review.
    """
    letters = set(machine.rotary_axes)
    wiring = {
        frozenset("AC"): ("C", "A", AxisRole.TABLE_ROTARY, "A/C table (trunnion)"),
        frozenset("AB"): ("A", "B", AxisRole.TABLE_ROTARY, "A/B table"),
        frozenset("BC"): ("B", "C", AxisRole.HEAD_ROTARY, "B/C head"),
    }
    if frozenset(letters) in wiring:
        first, second, role, label = wiring[frozenset(letters)]
        machine.rotary_axes[first].role = role
        machine.rotary_axes[first].sequence = 0
        machine.rotary_axes[first].parent = None
        machine.rotary_axes[second].role = role
        machine.rotary_axes[second].sequence = 1
        machine.rotary_axes[second].parent = first
        # Match the factory configs' alignment argument order exactly.
        if frozenset(letters) == frozenset("AC"):
            machine.set_alignment_axes("C", "A")
        elif frozenset(letters) == frozenset("BC"):
            machine.set_alignment_axes("C", "B")
        else:
            machine.set_alignment_axes("A", "B")
        report.assumed.append(
            translate(
                "CAM_MachineImport",
                "Rotary axes wired as a {label} configuration; the probe does "
                "not describe rotary mounting. Review roles and chain order.",
            ).format(label=label)
        )
    elif len(letters) == 1:
        letter = next(iter(letters))
        machine.rotary_axes[letter].role = AxisRole.TABLE_ROTARY
        machine.rotary_axes[letter].sequence = 0
        machine.set_alignment_axes(letter, None)
        report.assumed.append(
            translate(
                "CAM_MachineImport",
                "Rotary axis {letter} assumed to be a table rotary. Review its role.",
            ).format(letter=letter)
        )
    elif letters:
        report.skipped.append(
            translate(
                "CAM_MachineImport",
                "Rotary axes {letters} have no default chain wiring; "
                "assign roles and parents in the editor.",
            ).format(letters=", ".join(sorted(letters)))
        )


def machine_from_probe(
    xml_text: str, device_name: Optional[str] = None, source: Optional[str] = None
) -> Tuple[Machine, ImportReport]:
    """Build a Machine from an MTConnect probe document.

    Args:
        xml_text: the MTConnectDevices XML.
        device_name: which Device to import when the agent hosts several;
            optional when the document describes exactly one machine.
        source: where the document came from (URL or file), recorded in the
            machine description.

    Returns:
        (machine, report). The machine is in-memory only; saving is the
        caller's (normally the Machine Editor's) responsibility.
    """
    root = _parse_root(xml_text)
    devices = _device_elements(root)
    if not devices:
        raise ProbeParseError("The probe document describes no machine devices.")
    if device_name is not None:
        matches = [d for d in devices if d.get("name") == device_name]
        if not matches:
            raise ProbeParseError(f"No device named '{device_name}' in the probe document.")
        device = matches[0]
    elif len(devices) == 1:
        device = devices[0]
    else:
        names = ", ".join(d.get("name", "?") for d in devices)
        raise ProbeParseError(
            f"The probe document describes several devices ({names}); pass device_name."
        )

    report = ImportReport(device_name=device.get("name", ""))
    machine = Machine(name=device.get("name") or "Imported Machine")

    manufacturer = ""
    for child in device:
        if _local(child) == "Description":
            manufacturer = child.get("manufacturer", "")
            break
    machine.manufacturer = manufacturer

    provenance = translate("CAM_MachineImport", "Imported from MTConnect probe")
    if source:
        provenance += f" ({source})"
    uuid = device.get("uuid", "")
    if uuid:
        provenance += f", device uuid {uuid}"
    machine.description = provenance

    components = _axis_components(device)
    linear = [c for c in components if c.kind == "Linear"]
    rotary = [c for c in components if c.kind == "Rotary" and not _is_spindle(c)]
    spindles = [c for c in components if c.kind == "Rotary" and _is_spindle(c)]

    # A probe that declares INCH travel specifications is imported as an
    # imperial configuration; conforming agents emit canonical millimetres.
    imperial = any(c.specs.get("POSITION", ("",))[0] == "INCH" for c in linear)
    machine.configuration_units = "imperial" if imperial else "metric"

    for comp in linear:
        if comp.letter is None:
            report.skipped.append(
                translate(
                    "CAM_MachineImport", "Linear component '{name}' is not a recognized axis."
                ).format(name=comp.name)
            )
            continue
        axis = LinearAxis(comp.letter, comp.vector or _canonical_vector(comp.letter))
        units, minimum, maximum = comp.specs.get("POSITION", ("", None, None))
        factor = _length_factor(units, imperial)
        detail = ""
        if minimum is not None:
            axis.min_limit = minimum * factor
        if maximum is not None:
            axis.max_limit = maximum * factor
        if minimum is not None or maximum is not None:
            detail = f" ({axis.min_limit:g} .. {axis.max_limit:g} {machine.unit_format})"
        vunits, _, vmax = comp.specs.get("VELOCITY", ("", None, None))
        if vmax is not None:
            axis.max_velocity = _velocity_to_config_units(vmax, vunits, imperial)
        if comp.letter == "Z":
            axis.role = AxisRole.HEAD_LINEAR
        machine.linear_axes[comp.letter] = axis
        report.imported.append(
            translate("CAM_MachineImport", "Linear axis {letter}{detail}").format(
                letter=comp.letter, detail=detail
            )
        )

    for comp in rotary:
        if comp.letter is None:
            report.skipped.append(
                translate(
                    "CAM_MachineImport",
                    "Rotary component '{name}' is neither an A/B/C axis nor a spindle.",
                ).format(name=comp.name)
            )
            continue
        axis = RotaryAxis(comp.letter, comp.vector or _canonical_vector(comp.letter))
        _, minimum, maximum = comp.specs.get("ANGLE", ("", None, None))
        detail = ""
        if minimum is not None:
            axis.min_limit = minimum
        if maximum is not None:
            axis.max_limit = maximum
        if minimum is not None or maximum is not None:
            detail = f" ({axis.min_limit:g} .. {axis.max_limit:g} deg)"
        machine.rotary_axes[comp.letter] = axis
        report.imported.append(
            translate("CAM_MachineImport", "Rotary axis {letter}{detail}").format(
                letter=comp.letter, detail=detail
            )
        )

    _wire_rotary_chain(machine, report)

    # Coolant presence: a Coolant component anywhere in the device.
    coolant_components = [el for el in device.iter() if _local(el) == "Coolant"]
    has_coolant = bool(coolant_components)
    has_mist = any(
        "MIST" in item_type
        for comp in coolant_components
        for item_type in _parse_component(comp).dataitem_types
    )

    toolhead = Toolhead(name="Spindle", toolhead_type=ToolheadType.ROTARY)
    toolhead.coolant_flood = has_coolant
    toolhead.coolant_mist = has_mist
    if spindles:
        comp = spindles[0]
        _, rpm_min, rpm_max = comp.specs.get("ROTARY_VELOCITY", ("", None, None))
        detail = ""
        if rpm_min is not None:
            toolhead.min_rpm = rpm_min
        if rpm_max is not None:
            toolhead.max_rpm = rpm_max
        if rpm_min is not None or rpm_max is not None:
            detail = f" ({toolhead.min_rpm:g} .. {toolhead.max_rpm:g} rpm)"
        report.imported.append(
            translate("CAM_MachineImport", "Spindle{detail}").format(detail=detail)
        )
        if len(spindles) > 1:
            report.skipped.append(
                translate(
                    "CAM_MachineImport", "Additional spindles found; only the first was imported."
                )
            )
    else:
        report.assumed.append(
            translate(
                "CAM_MachineImport",
                "No spindle described by the probe; a default rotary toolhead was added.",
            )
        )
    machine.toolheads.append(toolhead)

    if not any("VELOCITY" in comp.specs for comp in linear):
        report.assumed.append(
            translate(
                "CAM_MachineImport",
                "Axis velocities are not published by this machine; defaults were used.",
            )
        )

    notes = [provenance]
    if report.assumed:
        notes.append(translate("CAM_MachineImport", "Import assumptions:"))
        notes.extend("- " + item for item in report.assumed)
    machine.kinematics.notes = "\n".join(notes)

    for problem in machine.validate_kinematic_chain():
        report.skipped.append(problem)

    return machine, report
