# -*- coding: utf-8 -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 sliptonic sliptonic@freecad.org                    *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************


__title__ = "CAM Mill Facing Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Class and implementation of Mill Facing operation."
__contributors__ = ""

import FreeCAD
from PySide import QtCore
import Path
import Path.Op.Base as PathOp

import Path.Base.Generator.spiral_facing as spiral_facing
import Path.Base.Generator.zigzag_facing as zigzag_facing
import Path.Base.Generator.directional_facing as directional_facing
import Path.Base.Generator.bidirectional_facing as bidirectional_facing
import Path.Base.Generator.linking as linking
import PathScripts.PathUtils as PathUtils
import Path.Base.FeedRate as FeedRate

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")
Arcs = LazyLoader("draftgeoutils.arcs", globals(), "draftgeoutils.arcs")
if FreeCAD.GuiUp:
    FreeCADGui = LazyLoader("FreeCADGui", globals(), "FreeCADGui")


translate = FreeCAD.Qt.translate


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectMillFacing(PathOp.ObjectOp):
    """Proxy object for Mill Facing operation."""

    def opFeatures(self, obj):
        """opFeatures(obj) ... return all standard features"""
        return (
            PathOp.FeatureTool
            | PathOp.FeatureDepths
            | PathOp.FeatureHeights
            | PathOp.FeatureStepDown
            | PathOp.FeatureCoolant
        )

    def initOperation(self, obj):
        """initOperation(obj) ... Initialize the operation by
        managing property creation and property editor status."""
        self.propertiesReady = False

        self.initOpProperties(obj)  # Initialize operation-specific properties

    def initOpProperties(self, obj, warn=False):
        """initOpProperties(obj) ... create operation specific properties"""
        Path.Log.track()
        self.addNewProps = list()

        for prtyp, nm, grp, tt in self.opPropertyDefinitions():
            if not hasattr(obj, nm):
                obj.addProperty(prtyp, nm, grp, tt)
                self.addNewProps.append(nm)

        # Set enumeration lists for enumeration properties
        if len(self.addNewProps) > 0:
            ENUMS = self.propertyEnumerations()
            for n in ENUMS:
                if n[0] in self.addNewProps:
                    setattr(obj, n[0], n[1])
            if warn:
                newPropMsg = translate("CAM_MIllFacing", "New property added to")
                newPropMsg += ' "{}": {}'.format(obj.Label, self.addNewProps) + ". "
                newPropMsg += translate("CAM_MillFacing", "Check default value(s).")
                FreeCAD.Console.PrintWarning(newPropMsg + "\n")

        self.propertiesReady = True

    def onChanged(self, obj, prop):
        """onChanged(obj, prop) ... Called when a property changes"""
        if prop == "StepOver" and hasattr(obj, "StepOver"):
            # Validate StepOver is between 0 and 100 percent
            if obj.StepOver < 0:
                obj.StepOver = 0
            elif obj.StepOver > 100:
                obj.StepOver = 100

    def opPropertyDefinitions(self):
        """opPropertyDefinitions(obj) ... Store operation specific properties"""

        return [
            (
                "App::PropertyEnumeration",
                "CutMode",
                "Facing",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the cut mode for the operation.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "ClearingPattern",
                "Facing",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the clearing pattern for the operation.",
                ),
            ),
            (
                "App::PropertyAngle",
                "Angle",
                "Facing",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the angle for the operation.",
                ),
            ),
            (
                "App::PropertyPercent",
                "StepOver",
                "Facing",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the stepover percentage of tool diameter.",
                ),
            ),
            (
                "App::PropertyDistance",
                "AxialStockToLeave",
                "Facing",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the stock to leave for the operation.",
                ),
            ),
            (
                "App::PropertyDistance",
                "PassExtension",
                "Facing",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Distance to extend cuts beyond polygon boundary for tool disengagement.",
                ),
            ),
            (
                "App::PropertyDistance",
                "StockExtension",
                "Facing",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Extends the boundary in both direction.",
                ),
            ),
            (
                "App::PropertyBool",
                "Reverse",
                "Facing",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Reverse the cutting direction for the selected pattern.",
                ),
            ),
        ]

    @classmethod
    def propertyEnumerations(self, dataType="data"):
        """propertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """
        Path.Log.track()

        enums = {
            "CutMode": [
                (translate("CAM_MillFacing", "Climb"), "Climb"),
                (translate("CAM_MillFacing", "Conventional"), "Conventional"),
            ],
            "ClearingPattern": [
                (translate("CAM_MillFacing", "ZigZag"), "ZigZag"),
                (translate("CAM_MillFacing", "Bidirectional"), "Bidirectional"),
                (translate("CAM_MillFacing", "Directional"), "Directional"),
                (translate("CAM_MillFacing", "Spiral"), "Spiral"),
            ],
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def opPropertyDefaults(self, obj, job):
        """opPropertyDefaults(obj, job) ... returns a dictionary of default values
        for the operation's properties."""
        defaults = {
            "CutMode": "Climb",
            "ClearingPattern": "ZigZag",
            "Angle": 0,
            "StepOver": 25,
            "AxialStockToLeave": 0.0,
        }

        return defaults

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj, job) ... set default values for operation-specific properties"""
        Path.Log.track()

        # Set default values directly like other operations do
        obj.CutMode = "Climb"
        obj.ClearingPattern = "ZigZag"
        obj.Angle = 0.0
        obj.StepOver = 25  # 25% as percentage
        obj.AxialStockToLeave = 0.0
        obj.PassExtension = (
            3.0  # Default to 3mm, will be adjusted based on tool diameter in opExecute
        )
        obj.Reverse = False

    def opExecute(self, obj):
        """opExecute(obj) ... process Mill Facing operation"""
        Path.Log.track()
        Path.Log.debug("MillFacing.opExecute() starting")

        # Get tool information
        tool = obj.ToolController.Tool
        Path.Log.debug(f"Tool: {tool.Label if tool else 'None'}")
        tool_diameter = tool.Diameter.Value
        Path.Log.debug(f"Tool diameter: {tool_diameter}")

        # Determine the step-downs
        finish_step = 0.0  # No finish step for facing
        Path.Log.debug(
            f"Depth parameters: clearance={obj.ClearanceHeight.Value}, safe={obj.SafeHeight.Value}, start={obj.StartDepth.Value}, step={obj.StepDown.Value}, final={obj.FinalDepth.Value + obj.AxialStockToLeave.Value}"
        )
        depthparams = PathUtils.depth_params(
            clearance_height=obj.ClearanceHeight.Value,
            safe_height=obj.SafeHeight.Value,
            start_depth=obj.StartDepth.Value,
            step_down=obj.StepDown.Value,
            z_finish_step=finish_step,
            final_depth=obj.FinalDepth.Value + obj.AxialStockToLeave.Value,
            user_depths=None,
        )
        Path.Log.debug(f"Depth params object: {depthparams}")

        # Always use the stock object top face for facing operations
        job = PathUtils.findParentJob(obj)
        Path.Log.debug(f"Job: {job.Label if job else 'None'}")
        if job and job.Stock:
            Path.Log.debug(f"Stock: {job.Stock.Label}")
            stock_faces = job.Stock.Shape.Faces
            Path.Log.debug(f"Number of stock faces: {len(stock_faces)}")

            # Find faces with normal pointing toward Z+ (upward)
            z_up_faces = []
            for face in stock_faces:
                # Get face normal at center
                u_mid = (face.ParameterRange[0] + face.ParameterRange[1]) / 2
                v_mid = (face.ParameterRange[2] + face.ParameterRange[3]) / 2
                normal = face.normalAt(u_mid, v_mid)
                Path.Log.debug(f"Face normal: {normal}, Z component: {normal.z}")

                # Check if normal points upward (Z+ direction) with some tolerance
                if normal.z > 0.9:  # Allow for slight deviation from perfect vertical
                    z_up_faces.append(face)
                    Path.Log.debug(f"Found upward-facing face at Z={face.BoundBox.ZMax}")

            if not z_up_faces:
                Path.Log.error("No upward-facing faces found in stock")
                raise ValueError("No upward-facing faces found in stock")

            # From the upward-facing faces, select the highest one
            top_face = max(z_up_faces, key=lambda f: f.BoundBox.ZMax)
            Path.Log.debug(f"Selected top face ZMax: {top_face.BoundBox.ZMax}")
            boundary_wire = top_face.OuterWire
            Path.Log.debug(f"Wire vertices: {len(boundary_wire.Vertexes)}")
        else:
            Path.Log.error("No stock found for facing operation")
            raise ValueError("No stock found for facing operation")

        boundary_wire = boundary_wire.makeOffset2D(
            obj.StockExtension.Value, 2
        )  # offset with intersection joins

        # Determine milling direction
        milling_direction = "climb" if obj.CutMode == "Climb" else "conventional"

        # Get operation parameters
        stepover_percent = obj.StepOver
        pass_extension = (
            obj.PassExtension.Value if hasattr(obj, "PassExtension") else tool_diameter * 0.5
        )
        retract_height = obj.SafeHeight.Value

        # Generate the base toolpath for one depth level based on clearing pattern
        try:
            if obj.ClearingPattern == "Spiral":
                # Spiral has different signature - no pass_extension or retract_height
                Path.Log.debug("Generating spiral toolpath")
                base_commands = spiral_facing.spiral(
                    polygon=boundary_wire,
                    tool_diameter=tool_diameter,
                    stepover_percent=stepover_percent,
                    milling_direction=milling_direction,
                    reverse=bool(getattr(obj, "Reverse", False)),
                    angle_degrees=getattr(obj.Angle, "Value", obj.Angle),
                )
            elif obj.ClearingPattern == "ZigZag":
                Path.Log.debug("Generating zigzag toolpath")
                base_commands = zigzag_facing.zigzag(
                    polygon=boundary_wire,
                    tool_diameter=tool_diameter,
                    stepover_percent=stepover_percent,
                    pass_extension=pass_extension,
                    retract_height=retract_height,
                    milling_direction=milling_direction,
                    reverse=bool(getattr(obj, "Reverse", False)),
                    angle_degrees=getattr(obj.Angle, "Value", obj.Angle),
                )
            elif obj.ClearingPattern == "Bidirectional":
                Path.Log.debug("Generating bidirectional toolpath")
                base_commands = bidirectional_facing.bidirectional(
                    polygon=boundary_wire,
                    tool_diameter=tool_diameter,
                    stepover_percent=stepover_percent,
                    pass_extension=pass_extension,
                    milling_direction=milling_direction,
                    reverse=bool(getattr(obj, "Reverse", False)),
                    angle_degrees=getattr(obj.Angle, "Value", obj.Angle),
                )
            elif obj.ClearingPattern == "Directional":
                Path.Log.debug("Generating directional toolpath")
                base_commands = directional_facing.directional(
                    polygon=boundary_wire,
                    tool_diameter=tool_diameter,
                    stepover_percent=stepover_percent,
                    pass_extension=pass_extension,
                    retract_height=retract_height,
                    milling_direction=milling_direction,
                    reverse=bool(getattr(obj, "Reverse", False)),
                    angle_degrees=getattr(obj.Angle, "Value", obj.Angle),
                )
            else:
                Path.Log.error(f"Unknown clearing pattern: {obj.ClearingPattern}")
                raise ValueError(f"Unknown clearing pattern: {obj.ClearingPattern}")

            Path.Log.debug(f"Generated {len(base_commands)} base commands")
            Path.Log.debug(base_commands)

        except Exception as e:
            Path.Log.error(f"Error generating toolpath: {e}")
            raise

        # clear commandlist
        self.commandlist = []

        # Be safe. Add first G0 to clearance height
        targetZ = obj.ClearanceHeight.Value
        self.commandlist.append(Path.Command("G0", {"Z": targetZ}))

        # Process each step-down using iterator protocol and add to commandlist
        depth_count = 0
        try:
            while True:
                depth = depthparams.next()
                depth_count += 1
                Path.Log.debug(f"Processing depth {depth_count}: {depth}")

                if depth_count == 1:
                    # First stepdown preamble:
                    # 1) Rapid to ClearanceHeight (already done at line 401)
                    # 2) Rapid to XY start position at ClearanceHeight
                    # 3) Rapid down to SafeHeight
                    # 4) Rapid down to cutting depth

                    # Find the first XY target from the base commands
                    first_xy = None
                    first_move_idx = None
                    for i, bc in enumerate(base_commands):
                        if "X" in bc.Parameters and "Y" in bc.Parameters:
                            first_xy = (bc.Parameters["X"], bc.Parameters["Y"])
                            first_move_idx = i
                            break

                    if first_xy is not None:
                        # 1) G0 to XY position at current height (ClearanceHeight from line 401)
                        pre1 = {"X": first_xy[0], "Y": first_xy[1]}
                        if not self.commandlist or any(
                            abs(pre1[k] - self.commandlist[-1].Parameters.get(k, pre1[k] + 1))
                            > 1e-9
                            for k in ("X", "Y")
                        ):
                            self.commandlist.append(Path.Command("G0", pre1))

                        # 2) G0 down to SafeHeight
                        pre2 = {"Z": obj.SafeHeight.Value}
                        if (
                            abs(pre2["Z"] - self.commandlist[-1].Parameters.get("Z", pre2["Z"] + 1))
                            > 1e-9
                        ):
                            self.commandlist.append(Path.Command("G0", pre2))

                        # 3) G0 down to cutting depth
                        pre3 = {"Z": depth}
                        if (
                            abs(pre3["Z"] - self.commandlist[-1].Parameters.get("Z", pre3["Z"] + 1))
                            > 1e-9
                        ):
                            self.commandlist.append(Path.Command("G0", pre3))

                    # Now append the base commands, skipping the generator's initial positioning move
                    for i, cmd in enumerate(base_commands):
                        # Skip the first move if it only positions at the start point
                        if i == first_move_idx:
                            # If this first move has only XY(Z) to the start point, skip it because we preambled it
                            pass
                        else:
                            new_params = dict(cmd.Parameters)
                            # Handle Z coordinate based on command type
                            if "Z" in new_params:
                                if cmd.Name == "G0":
                                    # For rapids, distinguish between true retracts and plunges
                                    # True retracts are at/near retract_height and should be preserved
                                    # Plunges are at/near polygon ZMin and should be clamped to depth
                                    if (
                                        abs(new_params["Z"] - retract_height) < 1.0
                                    ):  # Within 1mm of retract height
                                        # Keep as-is (true retract)
                                        pass
                                    else:
                                        # Not a retract - clamp to depth (includes plunges)
                                        new_params["Z"] = depth
                                else:
                                    # For G1 cutting moves, always use depth
                                    new_params["Z"] = depth
                            else:
                                # Missing Z coordinate - set based on command type
                                if cmd.Name == "G1":
                                    # Cutting moves always at depth
                                    new_params["Z"] = depth
                                else:
                                    # Rapids without Z - carry forward last Z
                                    if self.commandlist:
                                        new_params["Z"] = self.commandlist[-1].Parameters.get(
                                            "Z", depth
                                        )

                            # Fill in missing X,Y coordinates from last position
                            if self.commandlist:
                                last = self.commandlist[-1].Parameters
                                if "X" not in cmd.Parameters:
                                    new_params.setdefault("X", last.get("X"))
                                if "Y" not in cmd.Parameters:
                                    new_params.setdefault("Y", last.get("Y"))
                            # Skip zero-length moves (but allow Z-only moves for plunges/retracts)
                            if self.commandlist:
                                last_params = self.commandlist[-1].Parameters
                                # Only skip if X and Y are identical (allow Z-only moves)
                                if all(
                                    abs(new_params[k] - last_params.get(k, new_params[k] + 1))
                                    <= 1e-9
                                    for k in ("X", "Y")
                                ):
                                    # But if Z is different, keep it (it's a plunge or retract)
                                    z_changed = (
                                        abs(
                                            new_params.get("Z", 0)
                                            - last_params.get("Z", new_params.get("Z", 0) + 1)
                                        )
                                        > 1e-9
                                    )
                                    if not z_changed:
                                        continue
                            self.commandlist.append(Path.Command(cmd.Name, new_params))
                    Path.Log.debug(
                        f"First stepdown: Added {len(base_commands)} commands for depth {depth}"
                    )
                else:
                    # Subsequent stepdowns - handle linking
                    # Make a copy of base_commands and update Z depths
                    copy_commands = []
                    for cmd in base_commands:
                        new_params = dict(cmd.Parameters)
                        # Handle Z coordinate based on command type (same logic as first stepdown)
                        if "Z" in new_params:
                            if cmd.Name == "G0":
                                # For rapids, distinguish between true retracts and plunges
                                if (
                                    abs(new_params["Z"] - retract_height) < 1.0
                                ):  # Within 1mm of retract height
                                    # Keep as-is (true retract)
                                    pass
                                else:
                                    # Not a retract - clamp to depth (includes plunges)
                                    new_params["Z"] = depth
                            else:
                                # For G1 cutting moves, always use depth
                                new_params["Z"] = depth
                        else:
                            # Missing Z coordinate - set based on command type
                            if cmd.Name == "G1":
                                # Cutting moves always at depth
                                new_params["Z"] = depth
                            # For G0 without Z, we'll let it get filled in later from context
                        copy_commands.append(Path.Command(cmd.Name, new_params))

                    # Get the last position from self.commandlist
                    last_cmd = self.commandlist[-1]
                    last_position = FreeCAD.Vector(
                        last_cmd.Parameters.get("X", 0),
                        last_cmd.Parameters.get("Y", 0),
                        last_cmd.Parameters.get("Z", depth),
                    )

                    # Identify the initial retract+position+plunge bundle (G0s) before the next cut
                    bundle_start = None
                    bundle_end = None
                    target_xy = None
                    for i, cmd in enumerate(copy_commands):
                        if cmd.Name == "G0":
                            bundle_start = i
                            # collect consecutive G0s
                            j = i
                            while j < len(copy_commands) and copy_commands[j].Name == "G0":
                                # capture XY target if present
                                if (
                                    "X" in copy_commands[j].Parameters
                                    and "Y" in copy_commands[j].Parameters
                                ):
                                    target_xy = (
                                        copy_commands[j].Parameters.get("X"),
                                        copy_commands[j].Parameters.get("Y"),
                                    )
                                j += 1
                            bundle_end = j  # exclusive
                            break

                    if bundle_start is not None and target_xy is not None:
                        # Build target position at cutting depth
                        first_position = FreeCAD.Vector(target_xy[0], target_xy[1], depth)

                        # Generate collision-aware linking moves up to safe/clearance and back down
                        link_commands = linking.get_linking_moves(
                            start_position=last_position,
                            target_position=first_position,
                            local_clearance=obj.SafeHeight.Value,
                            global_clearance=obj.ClearanceHeight.Value,
                            tool_shape=obj.ToolController.Tool.Shape,
                        )
                        # Append linking moves, ensuring full XYZ continuity
                        current = last_position
                        for lc in link_commands:
                            params = dict(lc.Parameters)
                            X = params.get("X", current.x)
                            Y = params.get("Y", current.y)
                            Z = params.get("Z", current.z)
                            # Skip zero-length
                            if not (
                                abs(X - current.x) <= 1e-9
                                and abs(Y - current.y) <= 1e-9
                                and abs(Z - current.z) <= 1e-9
                            ):
                                self.commandlist.append(
                                    Path.Command(lc.Name, {"X": X, "Y": Y, "Z": Z})
                                )
                            current = FreeCAD.Vector(X, Y, Z)

                        # Remove the entire initial G0 bundle (up, XY, down) from the copy
                        del copy_commands[bundle_start:bundle_end]

                    # Append the copy commands, filling missing coords
                    for cc in copy_commands:
                        cp = dict(cc.Parameters)
                        if self.commandlist:
                            last = self.commandlist[-1].Parameters
                            # Only fill in coordinates that are truly missing from the original command
                            if "X" not in cc.Parameters:
                                cp.setdefault("X", last.get("X"))
                            if "Y" not in cc.Parameters:
                                cp.setdefault("Y", last.get("Y"))
                            # Don't carry forward Z - it should already be set correctly in copy_commands
                            if "Z" not in cc.Parameters:
                                # Only set Z if it wasn't in the original command
                                if cc.Name == "G1":
                                    cp["Z"] = depth  # Cutting moves at depth
                                else:
                                    cp.setdefault("Z", last.get("Z"))
                        # Skip zero-length
                        if self.commandlist:
                            last = self.commandlist[-1].Parameters
                            if all(
                                abs(cp[k] - last.get(k, cp[k] + 1)) <= 1e-9 for k in ("X", "Y", "Z")
                            ):
                                continue
                        self.commandlist.append(Path.Command(cc.Name, cp))
                    Path.Log.debug(
                        f"Stepdown {depth_count}: Added linking + {len(copy_commands)} commands for depth {depth}"
                    )

        except StopIteration:
            Path.Log.debug(f"All depths processed. Total depth levels: {depth_count}")

        # Add final G0 to clearance height
        targetZ = obj.ClearanceHeight.Value
        if self.commandlist:
            last = self.commandlist[-1].Parameters
            lastZ = last.get("Z")
            if lastZ is None or abs(targetZ - lastZ) > 1e-9:
                # Prefer Z-only to avoid non-numeric XY issues
                self.commandlist.append(Path.Command("G0", {"Z": targetZ}))

        # # Sanitize commands: ensure full XYZ continuity and remove zero-length/invalid/absurd moves
        # sanitized = []
        # curX = curY = curZ = None
        # # Compute XY bounds from original wire
        # try:
        #     bb = boundary_wire.BoundBox
        #     import math

        #     diag = math.hypot(bb.XLength, bb.YLength)
        #     xy_limit = max(1.0, diag * 10.0)
        # except Exception:
        #     xy_limit = 1e6
        # for idx, cmd in enumerate(self.commandlist):
        #     params = dict(cmd.Parameters)
        #     # Carry forward
        #     if curX is not None:
        #         params.setdefault("X", curX)
        #         params.setdefault("Y", curY)
        #         params.setdefault("Z", curZ)
        #     # Extract
        #     X = params.get("X")
        #     Y = params.get("Y")
        #     Z = params.get("Z")
        #     # Skip NaN/inf
        #     try:
        #         _ = float(X) + float(Y) + float(Z)
        #     except Exception:
        #         Path.Log.warning(
        #             f"Dropping cmd {idx} non-finite coords: {cmd.Name} {cmd.Parameters}"
        #         )
        #         continue
        #     # Debug: large finite XY - log but keep for analysis (do not drop)
        #     if abs(X) > xy_limit or abs(Y) > xy_limit:
        #         Path.Log.warning(f"Large XY detected (limit {xy_limit}): {cmd.Name} {params}")
        #     # Skip zero-length
        #     if (
        #         curX is not None
        #         and abs(X - curX) <= 1e-12
        #         and abs(Y - curY) <= 1e-12
        #         and abs(Z - curZ) <= 1e-12
        #     ):
        #         continue

        #     # Preserve I, J, K parameters for arc commands (G2/G3)
        #     if cmd.Name in ["G2", "G3"]:
        #         arc_params = {"X": X, "Y": Y, "Z": Z}
        #         if "I" in params:
        #             arc_params["I"] = params["I"]
        #         if "J" in params:
        #             arc_params["J"] = params["J"]
        #         if "K" in params:
        #             arc_params["K"] = params["K"]
        #         if "R" in params:
        #             arc_params["R"] = params["R"]
        #         sanitized.append(Path.Command(cmd.Name, arc_params))
        #     else:
        #         sanitized.append(Path.Command(cmd.Name, {"X": X, "Y": Y, "Z": Z}))
        #     curX, curY, curZ = X, Y, Z

        # self.commandlist = sanitized

        # Apply feedrates to the entire commandlist, with debug on failure
        try:
            FeedRate.setFeedRate(self.commandlist, obj.ToolController)
        except Exception as e:
            # Dump last 12 commands for diagnostics
            n = len(self.commandlist)
            start = max(0, n - 12)
            Path.Log.error("FeedRate failure. Dumping last commands:")
            for i in range(start, n):
                c = self.commandlist[i]
                Path.Log.error(f"  #{i}: {c.Name} {c.Parameters}")
            raise

        Path.Log.debug(f"Total commands in commandlist: {len(self.commandlist)}")
        Path.Log.debug("MillFacing.opExecute() completed successfully")
        Path.Log.debug(self.commandlist)


# Eclass


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Mill Facing operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectMillFacing(obj, name, parentJob)
    return obj


def SetupProperties():
    """SetupProperties() ... Return list of properties required for the operation."""
    setup = []
    setup.append("CutMode")
    setup.append("ClearingPattern")
    setup.append("Angle")
    setup.append("StepOver")
    setup.append("AxialStockToLeave")
    setup.append("PassExtension")
    setup.append("Reverse")
    return setup
