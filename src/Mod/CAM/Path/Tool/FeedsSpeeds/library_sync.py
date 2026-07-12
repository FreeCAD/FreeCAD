# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
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
Sync ``Presets`` from a library ToolBit asset into a Job's embedded copy.

See ``Roadmap/Epics/FeedsAndSpeeds.md`` ("Library -> Job preset sync") for
the design this implements. In short: a tool embedded in an old Job has no way to pick up
presets added/updated on the library version later, short of the user
manually re-entering them. This module provides an explicit, on-demand pull
of the ``Presets`` list only - it never touches ``ToolController`` feed,
speed, or spindle properties.
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional

from .presets import get_presets, set_presets

__title__ = "Feeds & Speeds Library Sync"
__author__ = "Connor (Billy Huddleston <billy@ivdc.com>)"
__url__ = "https://www.freecad.org"
__doc__ = "Sync ToolBit presets from the library into a Job's tools."


def preset_key(preset: dict) -> tuple:
    """Stable identity for a preset: name plus the material/op-type
    combination it targets - not name alone. Two presets that share a name
    but target different materials or op types are different presets, not
    a collision (e.g. "Default" for hardwood/any vs "Default" for
    softwood/any). Used both for merge matching here and for the duplicate
    check in the Presets tab editor.
    """
    hint = preset.get("material_hint") or {}
    material_key = hint.get("uuid") or hint.get("name")
    return (preset.get("name"), material_key, preset.get("op_type_hint"))


def merge_presets(embedded: List[dict], library: List[dict]) -> List[dict]:
    """Merge a library's presets into an embedded tool's presets.

    Job-local presets not present in the library are preserved. On a key
    collision the library version wins - the common case is "I fixed up
    my numbers in the library, pull the fix into old jobs." Order follows
    first-seen key: existing embedded order first, then any newly
    introduced library presets appended.
    """
    by_key: Dict[tuple, dict] = {}
    for preset in embedded:
        by_key[preset_key(preset)] = preset
    for preset in library:
        by_key[preset_key(preset)] = preset
    return list(by_key.values())


def sync_tool_presets(embedded_obj, library_obj, apply: bool = True) -> bool:
    """Merge ``library_obj``'s presets into ``embedded_obj`` in place.

    Returns True if the merge would change (``apply=False``) or changed
    (``apply=True``) the embedded tool's Presets property. With
    ``apply=False`` nothing is written - used for the cheap passive
    "presets are stale" check on document load.
    """
    current = get_presets(embedded_obj)
    merged = merge_presets(current, get_presets(library_obj))
    if merged == current:
        return False
    if apply:
        set_presets(embedded_obj, merged)
    return True


def resolve_library_source(asset_manager, embedded_tool_id: str):
    """Find the library ToolBit asset that an embedded tool came from.

    Matches on ``ToolBitID`` only - no fallback, no guessing. A tool with
    no id, or an id that isn't found in the local library, is left alone
    (reported as orphaned by ``sync_job_tools``); if the embedded tool is
    genuinely wrong, the user replaces it from the library directly rather
    than this module trying to infer a match.

    ``asset_manager`` is passed explicitly (rather than importing the global
    ``cam_assets`` singleton) so this stays testable against an isolated
    in-memory store, matching the pattern already used by the free
    functions in ``camassets.py``. Production callers pass ``cam_assets``.

    Uses ``depth=0`` (shallow): Diameter/Flutes/Label/Presets all come
    from the tool's own JSON, not its shape geometry, so there is no need
    to resolve the shape asset dependency here.
    """
    if not embedded_tool_id:
        return None
    from ..assets.uri import AssetUri

    return asset_manager.get_or_none(
        AssetUri.build(asset_type="toolbit", asset_id=embedded_tool_id),
        store="local",
        depth=0,
    )


@dataclass
class ToolSyncDetail:
    label: str
    status: str  # "updated" | "unchanged" | "orphaned"
    tool_number: Optional[int] = None


@dataclass
class SyncReport:
    updated: List[ToolSyncDetail] = field(default_factory=list)
    unchanged: List[ToolSyncDetail] = field(default_factory=list)
    orphaned: List[ToolSyncDetail] = field(default_factory=list)

    @property
    def details(self) -> List[ToolSyncDetail]:
        return self.updated + self.unchanged + self.orphaned


def sync_job_tools(job, asset_manager, apply: bool = True) -> SyncReport:
    """Sync Presets for every ToolController's embedded tool in a Job.

    Only touches objects whose merge actually changes something. Does not
    save the document - callers are responsible for persisting (or
    prompting the user to persist) after this returns. Production callers
    pass ``Path.Tool.cam_assets`` as ``asset_manager``.

    With ``apply=False``, nothing is written to the document at all - the
    returned report still reflects what *would* happen. Used for the
    passive "presets are stale" check on document load, which must never
    mutate the document as a side effect of merely opening it.
    """
    report = SyncReport()
    tools_group = getattr(job, "Tools", None)
    tcs = getattr(tools_group, "Group", []) or []

    for tc in tcs:
        tool_obj = getattr(tc, "Tool", None)
        if tool_obj is None:
            continue
        label = getattr(tool_obj, "Label", "")
        tool_number = getattr(tc, "ToolNumber", None)
        tool_id = getattr(tool_obj, "ToolBitID", "") or ""

        library_asset = resolve_library_source(asset_manager, tool_id)
        if library_asset is None:
            report.orphaned.append(
                ToolSyncDetail(label=label, status="orphaned", tool_number=tool_number)
            )
            continue

        changed = sync_tool_presets(tool_obj, library_asset.obj, apply=apply)
        detail = ToolSyncDetail(
            label=label,
            status="updated" if changed else "unchanged",
            tool_number=tool_number,
        )
        (report.updated if changed else report.unchanged).append(detail)

    return report
