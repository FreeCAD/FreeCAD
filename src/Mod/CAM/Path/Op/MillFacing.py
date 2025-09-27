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
import Path.Base.Generator.facing_common as facing_common
import Path.Base.Generator.linking  as linking
import PathScripts.PathUtils as PathUtils
import Path.Base.FeedRate as FeedRate

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")
Arcs = LazyLoader("draftgeoutils.arcs", globals(), "draftgeoutils.arcs")
if FreeCAD.GuiUp:
    FreeCADGui = LazyLoader("FreeCADGui", globals(), "FreeCADGui")


translate = FreeCAD.Qt.translate


if True:
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
            enumDict = ObjectMillFacing.propertyEnumerations(dataType="raw")
            for k, tupList in enumDict.items():
                if k in self.addNewProps:
                    setattr(obj, k, [t[1] for t in tupList])

            if warn:
                newPropMsg = translate("CAM_MIllFacing", "New property added to")
                newPropMsg += ' "{}": {}'.format(obj.Label, self.addNewProps) + ". "
                newPropMsg += translate("CAM_MillFacing", "Check default value(s).")
                FreeCAD.Console.PrintWarning(newPropMsg + "\n")

        self.propertiesReady = True

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
                "App::PropertyDistance",
                "StepOver",
                "Facing",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the stepover for the operation.",
                ),
            ),
            (
                "App::PropertyDistance",
                "MaterialAllowance",
                "Facing",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the stock to leave for the operation.",
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
        }

        return defaults



    def opExecute(self, obj):
        """opExecute(obj) ... process Mill Facing operation"""
        Path.Log.track()

        # Get tool information
        tool = obj.ToolController.Tool
        tool_diameter = tool.Diameter.Value
        
        # Determine the step-downs
        finish_step = 0.0  # No finish step for facing
        depthparams = PathUtils.depth_params(
            clearance_height=obj.ClearanceHeight.Value,
            safe_height=obj.SafeHeight.Value,
            start_depth=obj.StartDepth.Value,
            step_down=obj.StepDown.Value,
            z_finish_step=finish_step,
            final_depth=obj.FinalDepth.Value + obj.MaterialAllowance.Value,
            user_depths=None,
        )

        # Always use the stock object top face for facing operations
        job = PathUtils.findParentJob(obj)
        if job and job.Stock:
            stock_faces = job.Stock.Shape.Faces
            # Find the topmost face
            top_face = max(stock_faces, key=lambda f: f.BoundBox.ZMax)
            wire = top_face.OuterWire
        else:
            raise ValueError("No stock found for facing operation")

        # Use facing_common.get_angled_polygon to get the angled polygon with the angle property
        if obj.Angle.Value != 0:
            wire = facing_common.get_angled_polygon(wire, obj.Angle.Value)

        # Determine milling direction
        milling_direction = "climb" if obj.CutMode == "Climb" else "conventional"
        
        # Convert stepover percentage
        stepover_percent = obj.StepOver.Value

        # Generate the base toolpath for one depth level based on clearing pattern
        if obj.ClearingPattern == "Spiral":
            # Spiral has different signature - no pass_extension or retract_height
            base_commands = spiral_facing.spiral(
                polygon=wire,
                tool_diameter=tool_diameter,
                stepover_percent=stepover_percent,
                axis_preference="long",
                milling_direction=milling_direction
            )
        elif obj.ClearingPattern == "ZigZag":
            base_commands = zigzag_facing.zigzag(
                polygon=wire,
                tool_diameter=tool_diameter,
                stepover_percent=stepover_percent,
                axis_preference="long",
                pass_extension=None,
                retract_height=None,
                milling_direction=milling_direction
            )
        elif obj.ClearingPattern == "Bidirectional":
            base_commands = bidirectional_facing.bidirectional(
                polygon=wire,
                tool_diameter=tool_diameter,
                stepover_percent=stepover_percent,
                axis_preference="long",
                pass_extension=None,
                retract_height=None,
                milling_direction=milling_direction
            )
        elif obj.ClearingPattern == "Directional":
            base_commands = directional_facing.directional(
                polygon=wire,
                tool_diameter=tool_diameter,
                stepover_percent=stepover_percent,
                axis_preference="long",
                pass_extension=None,
                retract_height=None,
                milling_direction=milling_direction
            )
        else:
            raise ValueError(f"Unknown clearing pattern: {obj.ClearingPattern}")

        # Initialize the result command list
        commands = []
        
        # Add initial rapid to clearance height
        commands.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

        # Process each step-down
        for i, depth in enumerate(depthparams.depths):
            is_last_pass = (i == len(depthparams.depths) - 1)
            
            # Copy the base commands and adjust Z coordinates
            depth_commands = []
            for cmd in base_commands:
                new_params = dict(cmd.Parameters)
                if "Z" in new_params:
                    new_params["Z"] = depth
                
                depth_commands.append(Path.Command(cmd.Name, new_params))
            
            commands.extend(depth_commands)
            
            # Handle linking between passes
            if not is_last_pass:
                # Extract final position from current pass
                final_cmd = depth_commands[-1]
                final_pos = FreeCAD.Vector(
                    final_cmd.Parameters.get("X", 0),
                    final_cmd.Parameters.get("Y", 0),
                    depth
                )
                
                # Get the next depth level
                next_depth = depthparams.depths[i + 1]
                
                # Generate the next pass commands but replace the first G0 with linking moves
                next_depth_commands = []
                first_g0_cmd = None
                
                for j, cmd in enumerate(base_commands):
                    new_params = dict(cmd.Parameters)
                    if "Z" in new_params:
                        new_params["Z"] = next_depth
                    
                    
                    # Replace the first G0 command with linking moves
                    if j == 0 and cmd.Name == "G0":
                        first_g0_cmd = cmd
                        target_pos = FreeCAD.Vector(
                            new_params.get("X", 0),
                            new_params.get("Y", 0),
                            next_depth
                        )
                        
                        # Generate linking moves to replace the G0
                        link_commands = linking.get_linking_moves(
                            start_position=final_pos,
                            target_position=target_pos,
                            local_clearance=obj.SafeHeight.Value,
                            global_clearance=obj.ClearanceHeight.Value,
                            tool_shape=obj.ToolController.Tool.Shape
                        )
                        next_depth_commands.extend(link_commands)
                    else:
                        next_depth_commands.append(Path.Command(cmd.Name, new_params))
                
                commands.extend(next_depth_commands)
            else:
                # Final pass - rapid to clearance height
                commands.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

        # Create the path and apply feedrates using the helper
        path = Path.Path(commands)
        FeedRate.setFeedRate(path.Commands, obj.ToolController)
        obj.Path = path


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
    return setup
