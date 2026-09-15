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
Update a Job-embedded ToolBit by fully replacing it with its current
library version - presets, geometry, shape, everything - as if the tool
had been deleted from the document and picked fresh from the library.

Detection (this module) still checks presets and geometry separately, to
know whether a tool is stale at all and to describe what changed for the
review dialog. Applying an update is a single operation: point the
ToolController at a freshly-fetched library copy, then discard the old
object. A tool can't be half-updated (new geometry, stale presets, or
vice versa), and a fresh full fetch is definitionally correct on every
field, so there's no separate "what about a property this doesn't know
to look for" question the way a partial copy would have.

The relink itself (``tc.Tool = new_tool``) is the same primitive
``Path.Tool.Gui.Controller.ToolControllerEditor.updateToolController``
already uses when a user manually picks a different tool for a
ToolController - not a new mechanism, just applied to "the same tool's
current library version" instead of "a different tool".
"""

from dataclasses import dataclass, field
from typing import List, Optional

import Path
from .FeedsSpeeds.presets import get_presets
from .toolbit.models.base import PropertyGroupShape


def resolve_library_source(asset_manager, embedded_tool_id: str, depth: int = 0):
    """Find the library ToolBit asset that an embedded tool came from.

    Matches on ``ToolBitID`` only - no fallback, no guessing. A tool with
    no id, or an id that isn't found in the local library, is left alone;
    if the embedded tool is genuinely wrong, the user replaces it from the
    library directly rather than this trying to infer a match.

    ``asset_manager`` is passed explicitly (rather than importing the global
    ``cam_assets`` singleton) so this stays testable against an isolated
    in-memory store, matching the pattern already used by the free
    functions in ``camassets.py``. Production callers pass ``cam_assets``.

    ``depth`` defaults to 0 (shallow): Diameter/Flutes/Label/Presets all
    come from the tool's own JSON, not its shape geometry, so a presets-only
    caller has no need to resolve the shape asset dependency. Geometry
    callers must pass ``depth=1`` or higher - at depth=0,
    ``ToolBit.from_dict(..., shallow=True)`` builds the shape as a bare
    placeholder from the tool's own JSON only, never reading the shape
    file's own "Attributes" defaults, so a property that exists only as a
    shape-file default is silently absent rather than just stale. depth=1
    resolves the shape as a real dependency via ``ToolBitShape.from_bytes()``
    and populates those defaults for real.

    At depth=0 this only ever searches "local" - the toolbit itself only
    ever needs syncing from the user's own local library. But a resolved
    shape *dependency* is different: toolbitshape (.fcstd) assets are, by
    design, never copied into "local" the way .fctb tool files are - they
    stay in "builtin", so depth>=1 also searches "builtin", or every stock
    shape's dependency resolution would fail with "shape not found" for a
    perfectly normal tool.
    """
    if not embedded_tool_id:
        return None
    from .assets.uri import AssetUri

    stores = "local" if depth == 0 else ("local", "builtin")
    return asset_manager.get_or_none(
        AssetUri.build(asset_type="toolbit", asset_id=embedded_tool_id),
        store=stores,
        depth=depth,
    )


def geometry_properties(obj) -> List[str]:
    """Names of the dimensional/shape-defining properties on a ToolBit doc
    object - everything FreeCAD placed in the "Shape" property group."""
    return [p for p in obj.PropertiesList if obj.getGroupOfProperty(p) == PropertyGroupShape]


@dataclass
class GeometryChange:
    name: str
    old_value: object
    new_value: object


def diff_tool_geometry(embedded_obj, library_obj) -> List[GeometryChange]:
    """Compare "Shape"-group properties present on both objects, return the
    ones whose value differs. A property present on only one side (e.g. the
    shape's schema changed since the tool was embedded) is skipped, not
    guessed at - same "no fallback" principle as tool matching itself.

    ``library_obj`` comes from ``resolve_library_source``, which never
    attaches the asset to a document - some properties (e.g. ``Flutes``)
    only pick up their migration-default value once attached, and would
    otherwise read back as None. A None on the library side means "not
    materialized", never "the library wants this cleared", so it's treated
    as no change rather than a spurious diff.

    Refuses to diff at all if ``ShapeType`` differs between the two - that
    shouldn't normally happen (matching is by ToolBitID, same tool), but a
    property-by-property diff across two different shape classes would be
    meaningless: a name match (e.g. both happening to have "Diameter")
    would be coincidental, properties unique to one shape wouldn't be
    caught at all, and there'd be no way to actually reshape the tool from
    a property list alone. A shape-type change is squarely "replace the
    tool instead" territory, not a regrind.
    """
    embedded_shape_type = getattr(embedded_obj, "ShapeType", None)
    library_shape_type = getattr(library_obj, "ShapeType", None)
    if embedded_shape_type != library_shape_type:
        Path.Log.debug(
            f"diff_tool_geometry: ShapeType mismatch for '{getattr(embedded_obj, 'Label', '?')}' "
            f"({embedded_shape_type!r} vs {library_shape_type!r}) - skipping, not diffable."
        )
        return []

    common = set(geometry_properties(embedded_obj)) & set(geometry_properties(library_obj))
    changes = []
    for name in sorted(common):
        old_value = getattr(embedded_obj, name)
        new_value = getattr(library_obj, name)
        if new_value is not None and old_value != new_value:
            changes.append(
                GeometryChange(
                    name=name,
                    old_value=old_value,
                    new_value=new_value,
                )
            )
    return changes


def _presets_differ(embedded_obj, library_obj) -> bool:
    return get_presets(embedded_obj) != get_presets(library_obj)


@dataclass
class ToolStaleInfo:
    tc: object  # the owning ToolController - relinked in place on apply
    tool_obj: object  # the current embedded tool - removed from the doc on apply
    tool_number: Optional[int]
    label: str
    presets_differ: bool
    geometry_changes: List[GeometryChange] = field(default_factory=list)
    units: Optional[str] = None


def job_stale_tools(job, asset_manager) -> List[ToolStaleInfo]:
    """Dry-run only - never writes anything. For every ToolController's
    embedded tool with a resolvable library source, report whether it
    differs at all (presets and/or geometry). Tools with no library
    match, or with no differences, are simply absent from the result.
    """
    details = []
    tools_group = getattr(job, "Tools", None)
    tcs = getattr(tools_group, "Group", []) or []

    for tc in tcs:
        tool_obj = getattr(tc, "Tool", None)
        if tool_obj is None:
            continue
        tool_id = getattr(tool_obj, "ToolBitID", "") or ""
        try:
            # depth=1: enough to resolve the shape for a real geometry
            # diff (see resolve_library_source's docstring) - the full
            # depth=None fetch only happens on actual apply.
            library_asset = resolve_library_source(asset_manager, tool_id, depth=1)
        except Exception as e:
            Path.Log.debug(
                f"job_stale_tools: couldn't resolve library source for "
                f"'{getattr(tool_obj, 'Label', tool_id)}': {e}"
            )
            continue
        if library_asset is None:
            continue

        presets_differ = _presets_differ(tool_obj, library_asset.obj)
        geometry_changes = diff_tool_geometry(tool_obj, library_asset.obj)
        if not presets_differ and not geometry_changes:
            continue

        details.append(
            ToolStaleInfo(
                tc=tc,
                tool_obj=tool_obj,
                tool_number=getattr(tc, "ToolNumber", None),
                label=getattr(tool_obj, "Label", ""),
                presets_differ=presets_differ,
                geometry_changes=geometry_changes,
                units=getattr(library_asset.obj, "Units", None),
            )
        )

    return details


def replace_tool_from_library(tc, asset_manager) -> bool:
    """Fully replace a ToolController's embedded tool with a fresh copy
    from the library - presets, geometry, everything. Fetches at full
    depth (the same depth a manual "pick a tool from the library" fetch
    uses), so the replacement is exactly what picking this tool fresh
    would produce, not a partial/shallow copy.

    Removes the old tool object from the document - a ToolBit is only
    ever referenced by its owning ToolController, never shared or linked
    elsewhere, so once the relink happens the old object has no
    remaining references and would otherwise sit around as dead weight.

    Returns False (no-op) if the tool has no id or no resolvable library
    source - e.g. the library entry was deleted between the dry-run and
    the user pressing Update. Does not recompute - callers apply this to
    possibly many tools and should recompute once at the end.
    """
    old_tool_obj = getattr(tc, "Tool", None)
    if old_tool_obj is None:
        return False
    tool_id = getattr(old_tool_obj, "ToolBitID", "") or ""
    library_asset = resolve_library_source(asset_manager, tool_id, depth=None)
    if library_asset is None:
        return False

    doc = old_tool_obj.Document
    new_tool_obj = library_asset.attach_to_doc(doc)
    tc.Tool = new_tool_obj
    doc.removeObject(old_tool_obj.Name)
    return True
