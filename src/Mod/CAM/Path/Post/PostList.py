# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

import re
from typing import Any, List, Tuple

import Path
import Path.Base.Util as PathUtil
import Path.Tool.Controller as PathToolController


class _FixtureSetupObject:
    Path = None
    Name = "Fixture"
    InList = []
    Label = "Fixture"


class _CommandObject:
    def __init__(self, command):
        self.Path = Path.Path([command])
        self.Name = "Command"
        self.InList = []
        self.Label = "Command"


def needsTcOp(oldTc: Any, newTc: Any) -> bool:
    return (
        oldTc is None
        or oldTc.ToolNumber != newTc.ToolNumber
        or oldTc.SpindleSpeed != newTc.SpindleSpeed
        or oldTc.SpindleDir != newTc.SpindleDir
    )


def create_fixture_setup(processor: Any, order: int, fixture: str) -> _FixtureSetupObject:
    fobj = _FixtureSetupObject()
    c1 = Path.Command(fixture)
    fobj.Path = Path.Path([c1])

    if order != 0:
        clearance_z = (
            processor._job.Stock.Shape.BoundBox.ZMax
            + processor._job.SetupSheet.ClearanceHeightOffset.Value
        )
        c2 = Path.Command(f"G0 Z{clearance_z}")
        fobj.Path.addCommands(c2)

    fobj.InList.append(processor._job)
    return fobj


def build_postlist_by_fixture(processor: Any, early_tool_prep: bool = False) -> list:
    Path.Log.debug("Ordering by Fixture")
    postlist = []
    wcslist = processor._job.Fixtures
    currTc = None

    for index, f in enumerate(wcslist):
        sublist = [create_fixture_setup(processor, index, f)]

        for obj in processor._operations:
            tc = PathUtil.toolControllerForOp(obj)
            if tc is not None and PathUtil.activeForOp(obj):
                if needsTcOp(currTc, tc):
                    sublist.append(tc)
                    Path.Log.debug(f"Appending TC: {tc.Name}")
                    currTc = tc
            sublist.append(obj)

        postlist.append((f, sublist))

    return postlist


def build_postlist_by_tool(processor: Any, early_tool_prep: bool = False) -> list:
    Path.Log.debug("Ordering by Tool")
    postlist = []
    wcslist = processor._job.Fixtures
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

        tc = PathUtil.toolControllerForOp(obj)

        if tc is None or not needsTcOp(currTc, tc):
            curlist.append(obj)
        else:
            commitToPostlist()

            sublist = [tc]
            curlist = [obj]
            currTc = tc

            if "%T" in processor._job.PostProcessorOutputFile:
                toolstring = f"{tc.ToolNumber}"
            else:
                toolstring = re.sub(r"[^\w\d-]", "_", tc.Label)

    commitToPostlist()

    return postlist


def build_postlist_by_operation(processor: Any, early_tool_prep: bool = False) -> list:
    Path.Log.debug("Ordering by Operation")
    postlist = []
    wcslist = processor._job.Fixtures
    currTc = None

    for obj in processor._operations:
        if not PathUtil.activeForOp(obj):
            continue

        sublist = []
        Path.Log.debug(f"obj: {obj.Name}")

        for index, f in enumerate(wcslist):
            sublist.append(create_fixture_setup(processor, index, f))
            tc = PathUtil.toolControllerForOp(obj)
            if tc is not None:
                if processor._job.SplitOutput or needsTcOp(currTc, tc):
                    sublist.append(tc)
                    currTc = tc
            sublist.append(obj)

        postlist.append((obj.Label, sublist))

    return postlist


def buildPostList(processor: Any, early_tool_prep: bool = False) -> List[Tuple[str, List]]:
    orderby = processor._job.OrderOutputBy
    Path.Log.debug(f"Ordering by {orderby}")

    if orderby == "Fixture":
        postlist = build_postlist_by_fixture(processor, early_tool_prep)
    elif orderby == "Tool":
        postlist = build_postlist_by_tool(processor, early_tool_prep)
    elif orderby == "Operation":
        postlist = build_postlist_by_operation(processor, early_tool_prep)
    else:
        raise ValueError(f"Unknown order: {orderby}")

    Path.Log.debug(f"Postlist: {postlist}")

    if processor._job.SplitOutput:
        final_postlist = postlist
    else:
        final_postlist = [("allitems", [item for sublist in postlist for item in sublist[1]])]

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
    # First, collect all tool controllers across all groups to find next tool
    all_tool_controllers = []
    for group_idx, (name, sublist) in enumerate(postlist):
        for item_idx, item in enumerate(sublist):
            if hasattr(item, "Proxy") and isinstance(item.Proxy, PathToolController.ToolController):
                all_tool_controllers.append((group_idx, item_idx, item))

    new_postlist = []
    for group_idx, (name, sublist) in enumerate(postlist):
        new_sublist = []
        i = 0
        while i < len(sublist):
            item = sublist[i]
            # Check if item is a tool controller
            if hasattr(item, "Proxy") and isinstance(item.Proxy, PathToolController.ToolController):
                # Tool controller has Path.Commands like: [Command (comment), Command M6 [T:n]]
                # Find the M6 command
                m6_cmd = None
                for cmd in item.Path.Commands:
                    if cmd.Name == "M6":
                        m6_cmd = cmd
                        break

                if m6_cmd and len(m6_cmd.Parameters) > 0:
                    # M6 command has parameters like {'T': 5}, access via key
                    tool_number = m6_cmd.Parameters.get("T", item.ToolNumber)

                    # Find this TC in the global list and check if there's a next one
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

                    # Keep the original M6 command with tool parameter (M6 T5 format)
                    # This is the valid FreeCAD format, postprocessor will reformat if needed
                    new_sublist.append(item)

                    # If there's a next tool controller, add Tn prep for it immediately after M6
                    if next_tc is not None:
                        next_tool_number = next_tc.ToolNumber
                        prep_next = Path.Command(f"T{next_tool_number}")
                        prep_next_object = _CommandObject(prep_next)
                        new_sublist.append(prep_next_object)
                else:
                    # No M6 command found or no tool parameter, keep as-is
                    new_sublist.append(item)
            else:
                new_sublist.append(item)
            i += 1
        new_postlist.append((name, new_sublist))
    return new_postlist
