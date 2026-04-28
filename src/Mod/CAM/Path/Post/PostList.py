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
from dataclasses import dataclass, field
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
    data = {}
    raw_tc = PathUtil.toolControllerForOp(op)
    if raw_tc is not None:
        data["tool_controller"] = _wrap_tc(raw_tc)

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
                    sublist.append(tc_postable)
                    Path.Log.debug(f"Appending TC: {tc_postable.label}")
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

            sublist = [tc_postable]
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
                    sublist.append(tc_postable)
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

    # Apply early_tool_prep if configured on the machine
    early_tool_prep = False
    if (
        hasattr(processor, "_machine")
        and processor._machine
        and hasattr(processor._machine, "processing")
    ):
        early_tool_prep = getattr(processor._machine.processing, "early_tool_prep", False)

    if early_tool_prep:
        return apply_early_tool_prep(final_postlist)
    return final_postlist


def apply_early_tool_prep(postlist: List[Tuple[str, List]]) -> List[Tuple[str, List]]:
    """
    Apply early tool preparation optimization to the postlist.

    This function modifies tool change commands to enable early tool preparation:
    - Always outputs tool changes as "Tn M6" (tool number followed by change command)
    - Additionally emits standalone "Tn" prep commands immediately after the previous M6
      to allow the machine to prepare the next tool while the current tool is working

    Example output:
        T4 M6      <- change to tool 4
        T5         <- prep tool 5 early (while T4 is working)
        <gcode>    <- operations with T4
        T5 M6      <- change to tool 5 (already prepped)
        T7         <- prep tool 7 early (while T5 is working)
        <gcode>    <- operations with T5
        T7 M6      <- change to tool 7 (already prepped)
    """
    # Collect all tool controllers across all groups to find the next tool
    all_tool_controllers = []
    for group_idx, (name, sublist) in enumerate(postlist):
        for item_idx, item in enumerate(sublist):
            if item.item_type == "tool_controller":
                all_tool_controllers.append((group_idx, item_idx, item))

    new_postlist = []
    for group_idx, (name, sublist) in enumerate(postlist):
        new_sublist = []
        i = 0
        while i < len(sublist):
            item = sublist[i]
            if item.item_type == "tool_controller":
                m6_cmd = None
                for cmd in item.path.Commands:
                    if cmd.Name == "M6":
                        m6_cmd = cmd
                        break

                if m6_cmd and len(m6_cmd.Parameters) > 0:

                    tc_position = next(
                        (
                            idx
                            for idx, (g_idx, i_idx, tc) in enumerate(all_tool_controllers)
                            if g_idx == group_idx and i_idx == i
                        ),
                        None,
                    )

                    next_tc = (
                        all_tool_controllers[tc_position + 1][2]
                        if tc_position is not None and tc_position + 1 < len(all_tool_controllers)
                        else None
                    )

                    new_sublist.append(item)

                    if next_tc is not None:
                        next_tool_number = next_tc.data["tool_number"]
                        prep_cmd = Path.Command(f"T{next_tool_number}")
                        new_sublist.append(
                            Postable(
                                item_type="command",
                                label="Command",
                                path=Path.Path([prep_cmd]),
                                source=None,
                            )
                        )
                else:
                    new_sublist.append(item)
            else:
                new_sublist.append(item)
            i += 1
        new_postlist.append((name, new_sublist))
    return new_postlist
