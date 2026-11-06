# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>

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

import re
from dataclasses import dataclass, field, replace
from typing import Any, List, Optional, Tuple

import Path
import Path.Base.Util as PathUtil

debug = False
if debug:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def _get_effective_fixtures(processor):
    """Return the fixture list the postprocessor should use.

    If the configuration bundle contains a non-empty SELECTED_FIXTURES
    list, only those fixtures are returned (preserving job order).
    Otherwise all job fixtures are returned.
    """
    all_fixtures = processor._job.Fixtures
    selected = getattr(processor, "values", {}).get("SELECTED_FIXTURES", None)
    if selected:
        return [f for f in all_fixtures if f in selected]
    return all_fixtures


@dataclass
class Postable:
    """Uniform wrapper for every item that passes through the post-processing pipeline.

    Replaces the three ad-hoc mock classes (_FixtureSetupObject, _CommandObject,
    _RotationSetupObject) and raw document objects. All downstream code can rely on
    this single interface instead of duck-typing via hasattr.

    The wrapper copies essential data (path, label) to prevent postprocessing from
    accidentally marking the original FreeCAD document objects dirty.

    Attributes:
        item_type: One of "operation", "tool_controller", "fixture", "rotation", "command".
        label:     Human-readable name (mirrors the old .Label attribute).
        path:      Always a *copy* of the source path so mutations here never touch
                   the original FreeCAD document object.
        source:    Reference to the original object for traceability (read-only).
        data:      Extension dict. Standard keys:
                     "tool_controller" (Postable) - Present on operation items.
                     "tool_number" (int/float) - Present on tool_controller items.
    """

    item_type: str
    label: str
    path: "Path.Path"
    source: Optional[object]
    data: dict = field(default_factory=dict)

    # Backward-compat properties so legacy code continues to work without changes.
    @property
    def Path(self):
        return self.path

    @Path.setter
    def Path(self, value):
        self.path = value

    @property
    def Label(self):
        return self.label

    def __getattr__(self, name):
        """Forward unknown attribute lookups to source for backward-compat.

        Legacy post scripts access attributes like .Name, .InList, .Proxy, .TypeId,
        .ToolNumber, .CoolantMode, etc. directly on items. Forwarding to source keeps
        them working without modification.

        ToolController is intercepted: returns the Postable-wrapped TC stored in
        data["tool_controller"] rather than the live document object, preventing
        callers from accidentally marking the source operation dirty.
        """
        # Use object.__getattribute__ to avoid recursion if source itself isn't set yet.
        try:
            data = object.__getattribute__(self, "data")
        except AttributeError:
            data = {}

        # Return the Postable-wrapped TC instead of the live document object.
        if name == "ToolController":
            return data.get("tool_controller", None)

        # A path the post has already placed (WrapperPost._place_operations)
        # is in world coordinates: report no Placement, so a legacy script's
        # PathUtils.getPathWithPlacement() does not apply it a second time.
        if name == "Placement" and data.get("placed"):
            import FreeCAD

            return FreeCAD.Placement()

        try:
            source = object.__getattribute__(self, "source")
        except AttributeError:
            source = None

        if source is not None:
            return getattr(source, name)

        # Provide sensible fallbacks for items that have no source document object
        # (fixture, rotation, command).  These mirror what the old mock classes provided.
        try:
            label = object.__getattribute__(self, "label")
        except AttributeError:
            label = "Unknown"

        _fallbacks = {
            "Name": label,
            "InList": [],
        }
        if name in _fallbacks:
            return _fallbacks[name]

        raise AttributeError(f"'{type(self).__name__}' object has no attribute '{name}'")


def needsTcOp(oldTc: Any, newTc: Any) -> bool:
    return (
        oldTc is None
        or oldTc.ToolNumber != newTc.ToolNumber
        or oldTc.SpindleSpeed != newTc.SpindleSpeed
        or oldTc.SpindleDir != newTc.SpindleDir
    )


def _wrap_tc(tc: Any) -> Postable:
    """Wrap a FreeCAD ToolController document object in a Postable.

    Always generates the toolchange path from TC attributes to ensure commands are
    present even if the document hasn't been recomputed (tc.Path may be empty).
    """
    from Path.Base.Generator import toolchange

    if tc.Path and tc.Path.Commands:
        path = Path.Path(tc.Path.Commands)
    else:
        try:
            spindle_dir = (
                tc.Tool.Proxy.get_spindle_direction()
                if tc.Tool
                else toolchange.SpindleDirection.OFF
            )
            commands = toolchange.generate(
                toolnumber=tc.ToolNumber,
                toollabel=tc.Label,
                spindlespeed=tc.SpindleSpeed if tc.SpindleSpeed else 0,
                spindledirection=spindle_dir,
            )
            path = Path.Path(commands)
        except Exception:
            path = Path.Path([Path.Command("M6", {"T": int(tc.ToolNumber)})])

    return Postable(
        item_type="tool_controller",
        label=tc.Label,
        path=path,
        source=tc,
        data={"tool_number": tc.ToolNumber},
    )


def _wrap_op(op: Any) -> Postable:
    """Wrap a FreeCAD operation document object in a Postable.

    Creates a safe wrapper that copies essential operation data (path, ToolController)
    so downstream postprocessing code can read from the copy rather than the live
    document object. This prevents postprocessing from accidentally marking operations
    dirty.

    Data keys populated:
        "tool_controller" (Postable) - Present when the operation has a ToolController.
    """
    data = {}  # WHATIF: = op.postable_annotations()
    raw_tc = PathUtil.toolControllerForOp(op)
    if raw_tc is not None:
        data["tool_controller"] = _wrap_tc(raw_tc)

    # For a don't-interpret-text-blob: data={'str': blob}
    return Postable(
        item_type="operation",
        label=op.Label,
        path=Path.Path(op.Path.Commands) if op.Path else Path.Path([]),
        source=op,
        data=data,
    )


def create_fixture_setup(processor: Any, order: int, fixture: str) -> Postable:
    c1 = Path.Command(fixture)
    commands = [c1]

    if order != 0:
        clearance_z = (
            processor._job.Stock.Shape.BoundBox.ZMax
            + processor._job.SetupSheet.ClearanceHeightOffset.Value
        )
        commands.append(Path.Command(f"G0 Z{clearance_z}"))

    postable = Postable(
        item_type="fixture",
        label="Fixture",
        path=Path.Path(commands),
        source=None,
    )
    return postable


def build_postlist_by_fixture(processor: Any) -> list:
    """Build postlist ordered by fixture (work coordinate system).

    Wraps operations early to prevent accessing live document objects, which can
    mark operations dirty during postprocessing.

    Args:
        processor: The postprocessor object with job and operations

    Returns:
        List of tuples: [(fixture_name, [postable_items])]
    """
    Path.Log.debug("Ordering by Fixture")
    postlist = []
    wcslist = _get_effective_fixtures(processor)
    currTc = None

    for index, f in enumerate(wcslist):
        sublist = [create_fixture_setup(processor, index, f)]

        for obj in processor._operations:
            if not PathUtil.activeForOp(obj):
                continue

            # Wrap early: all further access uses the Postable copy, not the raw document object.
            wrapped_op = _wrap_op(obj)

            tc_postable = wrapped_op.ToolController
            if tc_postable is not None:
                if needsTcOp(currTc, tc_postable):
                    sublist.append(_wrap_tc(tc_postable.source))
                    Path.Log.debug(f"Appending TC: {tc_postable.source.Name}")
                    currTc = tc_postable
            sublist.append(wrapped_op)

        postlist.append((f, sublist))

    return postlist


def build_postlist_by_tool(processor: Any) -> list:
    """Build postlist ordered by tool.

    Groups operations by tool controller to minimize tool changes. Wraps operations
    early to prevent accessing live document objects.

    Args:
        processor: The postprocessor object with job and operations

    Returns:
        List of tuples: [(tool_name, [postable_items])]
    """
    Path.Log.debug("Ordering by Tool")
    postlist = []
    wcslist = _get_effective_fixtures(processor)
    toolstring = "None"
    currTc = None

    fixturelist = []
    for index, f in enumerate(wcslist):
        fixturelist.append(create_fixture_setup(processor, index, f))

    curlist = []
    sublist = []

    def commitToPostlist():
        if len(curlist) > 0:
            for fixture in fixturelist:
                sublist.append(fixture)
                sublist.extend(curlist)
            postlist.append((toolstring, sublist))

    Path.Log.track(processor._job.PostProcessorOutputFile)
    for _, obj in enumerate(processor._operations):
        Path.Log.track(obj.Label)

        if not PathUtil.activeForOp(obj):
            Path.Log.track()
            continue

        # Wrap early: all further access uses the Postable copy, not the raw document object.
        wrapped_op = _wrap_op(obj)
        tc_postable = wrapped_op.ToolController

        if tc_postable is None or not needsTcOp(currTc, tc_postable):
            curlist.append(wrapped_op)
        else:
            commitToPostlist()

            sublist = [_wrap_tc(tc_postable.source)]
            curlist = [wrapped_op]
            currTc = tc_postable

            if "%T" in processor._job.PostProcessorOutputFile:
                toolstring = f"{tc_postable.data['tool_number']}"
            else:
                toolstring = re.sub(r"[^\w\d-]", "_", tc_postable.label)

    commitToPostlist()

    return postlist


def build_postlist_by_operation(processor: Any) -> list:
    """Build postlist ordered by operation.

    Creates separate sections for each operation with all fixtures. Wraps operations
    early to prevent accessing live document objects.

    Args:
        processor: The postprocessor object with job and operations

    Returns:
        List of tuples: [(operation_name, [postable_items])]
    """
    Path.Log.debug("Ordering by Operation")
    postlist = []
    wcslist = _get_effective_fixtures(processor)
    currTc = None

    for obj in processor._operations:
        if not PathUtil.activeForOp(obj):
            continue

        # Wrap early: all further access uses the Postable copy, not the raw document object.
        wrapped_op = _wrap_op(obj)
        Path.Log.debug(f"obj: {wrapped_op.label}")

        sublist = []

        for index, f in enumerate(wcslist):
            sublist.append(create_fixture_setup(processor, index, f))

            tc_postable = wrapped_op.ToolController
            if tc_postable is not None:
                if processor._job.SplitOutput or needsTcOp(currTc, tc_postable):
                    sublist.append(_wrap_tc(tc_postable.source))
                    currTc = tc_postable
            sublist.append(wrapped_op)

        postlist.append((wrapped_op.label, sublist))

    return postlist


def buildPostList(processor: Any) -> List[Tuple[str, List]]:
    """Build ordered list of postables for postprocessing.

    Determines ordering strategy based on job.OrderOutputBy and delegates to
    appropriate builder function. All operations are wrapped early to prevent
    accessing live document objects.

    Args:
        processor: The postprocessor object with job and operations

    Returns:
        List of tuples: [(section_name, [postable_items])]
    """
    orderby = processor._job.OrderOutputBy
    Path.Log.debug(f"Ordering by {orderby}")

    if orderby == "Fixture":
        postlist = build_postlist_by_fixture(processor)
    elif orderby == "Tool":
        postlist = build_postlist_by_tool(processor)
    elif orderby == "Operation":
        postlist = build_postlist_by_operation(processor)
    else:
        raise ValueError(f"Unknown order: {orderby}")

    Path.Log.debug(f"Postlist: {postlist}")

    if processor._job.SplitOutput:
        final_postlist = postlist
    else:
        final_postlist = [("allitems", [item for sublist in postlist for item in sublist[1]])]

    # Apply tool change formatting / early tool prep / tool prep assertion
    processing = getattr(processor._machine, "processing", None)

    tool_change_format = getattr(getattr(processing, "tool_change_format", None), "value", "m6_t")
    early_tool_prep = bool(getattr(processing, "early_tool_prep", False))
    assert_tool_prep = bool(getattr(processing, "assert_tool_prep", False))

    return apply_tool_change_format(
        final_postlist, tool_change_format, early_tool_prep, assert_tool_prep
    )


def _t_command_postable(tool_number) -> Postable:
    """Standalone 'T<n>' command item."""
    return Postable(
        item_type="command",
        label="Command",
        path=Path.Path([Path.Command(f"T{int(tool_number)}")]),
        source=None,
    )


def _ends_with_t(item: Postable, tool_number) -> bool:
    """True if the item's last command is already 'T<tool_number>'."""
    cmds = item.path.Commands if item.path else []
    return bool(cmds) and cmds[-1].Name == f"T{int(tool_number)}"


def apply_tool_change_format(
    postlist: List[Tuple[str, List]],
    tool_change_format: str = "m6_t",
    early_tool_prep: bool = False,
    assert_tool_prep: bool = False,
) -> List[Tuple[str, List]]:
    """
    Rewrite tool change items according to the machine's tool change settings.

    tool_change_format (ToolChangeFormat value), applied when the G-code is written:
        "m6_t"            M6 T4          (tool change to T4)
        "t_m6"            T4 M6          (tool change to T4)
        "m6_t_early_prep" M6 T6          (M6 changes to the prepped tool, T6 preps the NEXT tool)
        "m6_only"         M6             (no T parameter. T parameter may be written by early tool prep)
        "no_tool_change"                 (No T or M parameters written)

    The M6 command itself always keeps T<current tool>; the layout is stored in its
    annotations ("tool_change_format", "next_tool").

    early_tool_prep:
        For formats other than "m6_t_early_prep", a standalone "T<next>" is emitted after
        each tool change so the machine can prep the next tool while the current one cuts.
        For "m6_t_early_prep" the next tool prep is part of the M6 line, and the first tool
        of each output group is prepped with a standalone "T<n>" before its M6.

    assert_tool_prep (only active with early_tool_prep):
        A "T<n>" is written immediately before every M6
        Useful when starting partway through the file.

    Redundant consecutive T commands for the same tool are never written. Example with
    early prep + assert, format "m6_only":
        T4          <- prep / assert
        M6
        T6          <- prep next tool
        (gcode)
        T6          <- assert
        M6
    """
    use_early = early_tool_prep
    use_assert = early_tool_prep and assert_tool_prep
    early_tool_change_fmt = tool_change_format == "m6_t_early_prep"
    no_tool_change = tool_change_format == "no_tool_change"

    # Flatten tool controllers across all groups so "next tool" can cross group boundaries.
    all_tcs = [
        (g, i)
        for g, (_, sub) in enumerate(postlist)
        for i, item in enumerate(sub)
        if item.item_type == "tool_controller"
    ]
    next_lookup = {}
    for pos, key in enumerate(all_tcs):
        if pos + 1 < len(all_tcs):
            ng, ni = all_tcs[pos + 1]
            next_lookup[key] = postlist[ng][1][ni].data.get("tool_number")

    def add_tool(new_sublist: list, tool_number) -> None:
        if tool_number is None:
            return
        if new_sublist and _ends_with_t(new_sublist[-1], tool_number):
            return  # redundant, same T directly before
        new_sublist.append(_t_command_postable(tool_number))

    new_postlist = []

    for g, (name, sublist) in enumerate(postlist):
        new_sublist: list = []
        first_in_group = True
        for i, item in enumerate(sublist):
            if item.item_type != "tool_controller":
                new_sublist.append(item)
                continue

            cmds = list(item.path.Commands)
            m6_idx = next((k for k, c in enumerate(cmds) if c.Name == "M6"), None)
            tool_number = item.data.get("tool_number")
            if m6_idx is None or tool_number is None:
                new_sublist.append(item)
                continue

            next_tool = next_lookup.get((g, i))

            # Formats whose M6 line does not name the tool being changed to need the first
            # tool of each output group prepped explicitly.
            needs_initial_prep = first_in_group and (
                early_tool_change_fmt or (use_early and tool_change_format == "m6_only")
            )
            if (
                (needs_initial_prep or use_assert)
                and not no_tool_change
                and tool_change_format != "t_m6"
            ):
                # format already leads with this T
                add_tool(new_sublist, tool_number)

            # The M6 keeps T<current tool> so everything downstream that reads
            # Parameters["T"] (G43 TLO, tool tracking) keeps working. The requested layout
            # is an annotations that is applied when the G-code text is written in
            # Processor._convert_tool_change.
            annotations = {"tool_change_format": tool_change_format}
            if next_tool is not None:
                annotations["next_tool"] = str(int(next_tool))
            new_m6 = [Path.Command("M6", {"T": int(tool_number)}, annotations)]

            cmds[m6_idx : m6_idx + 1] = new_m6
            new_sublist.append(replace(item, path=Path.Path(cmds)))

            # Prep for next tool
            if (
                use_early
                and not early_tool_change_fmt
                and not no_tool_change
                and next_tool is not None
            ):
                new_sublist.append(_t_command_postable(next_tool))

            first_in_group = False
        new_postlist.append((name, new_sublist))
    return new_postlist
