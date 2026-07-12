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
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
ToolBit preset accessors.

Presets live on the ToolBit's FreeCAD doc object as an ``App::PropertyString``
named ``Presets`` containing a JSON-encoded list. The property is lazy-added:
existing tools open unchanged; the property only appears on first save.

**Scope.** ``set_presets`` writes to the doc-object property only. It does
*not* write back to the library asset. Two reasons:

1. Library tools are shared state. Vendor packs, multi-user network
   libraries, and read-only stores all break under silent in-job
   mutation. The Job's "Assign Stock Material" follows the same rule —
   it mutates the job's stock, not the source ``.FCMat``.
2. Users have two distinct mental modes: *curating the library* (explicit,
   library-editor flow) and *adjusting tools for this job* (local). Silently
   coupling them violates that distinction.

Consequences:

- Presets edited in a Job persist with the FCStd, not the library.
- A preset created in Job A is *not* visible to a freshly-opened Job B
  that pulls the same tool from the library.
- To promote a preset into the library, the user opens the tool from
  the library browser; its Presets tab edits the same Presets property,
  and the library write happens when the editor is accepted (the asset
  manager re-serializes the tool via ``to_dict``, which includes
  presets).

Preset record schema:

    {
        "name": str | None,                    # user-given label, optional
        "material_hint": {"uuid": str, "name": str} | None,
        "op_type_hint": str | None,            # one of types.OP_TYPES
        "surface_speed": float | None,         # m/min  (a.k.a. cutting speed Vc)
        "chipload": float | None,              # mm/tooth
        "vert_feed_ratio": float,              # default 0.33
        "notes": str | None,                   # free-text, optional
    }

Storage is engineering-only — surface_speed + chipload + the vertical-feed
ratio. Raw feed (mm/min) and raw spindle speed (rpm) are computed from
those plus the tool's geometry (diameter, flutes) at the moment they're
needed (display, application). They are not persisted: storing them
would be redundant when geometry is known and incoherent if the tool's
geometry later changes.
"""

import json
from typing import List, Optional

from PySide.QtCore import QT_TRANSLATE_NOOP

PRESETS_PROPERTY = "Presets"
PRESETS_GROUP = "Feeds & Speeds"


def get_presets(obj) -> List[dict]:
    raw = getattr(obj, PRESETS_PROPERTY, "") if obj is not None else ""
    if not raw:
        return []
    try:
        data = json.loads(raw)
    except (TypeError, ValueError):
        return []
    return data if isinstance(data, list) else []


def set_presets(obj, presets: List[dict]) -> None:
    """
    Lazy-add the ``Presets`` property if absent, then store the JSON-encoded
    list. Writes to the doc-object property only; never touches the library
    asset (see module docstring for rationale).
    """
    if presets is None:
        raise TypeError("presets must be a list, got None")
    if not hasattr(obj, PRESETS_PROPERTY):
        obj.addProperty(
            "App::PropertyString",
            PRESETS_PROPERTY,
            PRESETS_GROUP,
            QT_TRANSLATE_NOOP("App::Property", "JSON-encoded list of feeds & speeds presets"),
        )
    obj.Presets = json.dumps(presets, sort_keys=True)


def make_preset(
    name: Optional[str] = None,
    material_uuid=None,
    material_name=None,
    op_type=None,
    surface_speed=None,
    chipload=None,
    vert_feed_ratio=0.33,
    notes: Optional[str] = None,
) -> dict:
    """
    Build a well-formed preset record. ``material_hint`` is ``None`` if both
    UUID and name are ``None``. ``name`` is the user-given label and is
    optional. ``surface_speed`` is the cutting speed in m/min (also known
    as Vc; "SFM" in imperial). ``notes`` is free-text, optional.
    """
    if material_uuid is None and material_name is None:
        material_hint = None
    else:
        material_hint = {"uuid": material_uuid, "name": material_name}
    return {
        "name": name,
        "material_hint": material_hint,
        "op_type_hint": op_type,
        "surface_speed": surface_speed,
        "chipload": chipload,
        "vert_feed_ratio": vert_feed_ratio,
        "notes": notes,
    }


def derive_preset_label(preset: dict, fallback: str = "(unnamed)") -> str:
    """
    Compute a human-readable label for a preset. Prefers the user-given
    ``name`` field, falls back to a ``material/op`` summary, then to the
    fallback string.
    """
    n = preset.get("name")
    if n:
        return n
    hint = preset.get("material_hint")
    mat = (hint.get("name") or hint.get("uuid") or "any") if isinstance(hint, dict) else "any"
    op = preset.get("op_type_hint") or "any"
    if mat == "any" and op == "any":
        return fallback
    return f"{mat} / {op}"
