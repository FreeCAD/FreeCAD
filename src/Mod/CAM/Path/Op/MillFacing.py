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


class ObjectSlot(PathOp.ObjectOp):
    """Proxy object for Mill Facing operation."""

    def opFeatures(self, obj):
        """opFeatures(obj) ... return all standard features"""
        return (
            PathOp.FeatureTool
            | PathOp.FeatureDepths
            | PathOp.FeatureHeights
            | PathOp.FeatureStepDown
            | PathOp.FeatureCoolant
            | PathOp.FeatureBaseFaces
        )

    def initOperation(self, obj):
        """initOperation(obj) ... Initialize the operation by
        managing property creation and property editor status."""
        self.propertiesReady = False

        self.initOpProperties(obj)  # Initialize operation-specific properties

        # For debugging
        if Path.Log.getLevel(Path.Log.thisModule()) != 4:
            obj.setEditorMode("ShowTempObjects", 2)  # hide

        if not hasattr(obj, "DoNotSetDefaultValues"):
            self.opSetEditorModes(obj)

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
            enumDict = ObjectSlot.propertyEnumerations(dataType="raw")
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

    def getActiveEnumerations(self, obj):
        """getActiveEnumerations(obj) ...
        Method returns dictionary of property enumerations based on
        active conditions in the operation."""
        ENUMS = dict()
        for prop, data in ObjectSlot.propertyEnumerations():
            ENUMS[prop] = data
        if hasattr(obj, "Base"):
            if obj.Base:
                # (base, subsList) = obj.Base[0]
                subsList = obj.Base[0][1]
                subCnt = len(subsList)
        return ENUMS

    def updateEnumerations(self, obj):
        """updateEnumerations(obj) ...
        Method updates property enumerations based on active conditions
        in the operation.  Returns the updated enumerations dictionary.
        Existing property values must be stored, and then restored after
        the assignment of updated enumerations."""
        Path.Log.debug("updateEnumerations()")


    def opExecute(self, obj):
        """opExecute(obj) ... process Mill Facing operation"""
        Path.Log.track()

        # Extract the polygon from the base object
        # If there's no base object, we're using the stock.  Get the stock object top face

        # use facing_common.get_angled_polygon to get the angled polygon with the angle property

        # Call the correct path generator based on the clearing pattern and get the result path.
        # directional
        # bidirectional
        # zigzag
        # spiral
        
        # Determine the step-downs
        # For each step-down:
        #   - copy the returned path.
        #   - Revise the Z words on the feed moves
        #   - Add the feed rates
        #   - Check if this is the last step-down
        #   - If not, 
        #       - Extract the final position from prior.
        #       - Call the linking generator
        #       - Add the linking path to the command list
        #   - If yes, 
        #       - Rapid to clearance height
        # 


# Eclass




def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Mill Facing operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectSlot(obj, name, parentJob)
    return obj
