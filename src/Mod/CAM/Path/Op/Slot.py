# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2020 Russell Johnson (russ4262) <russ4262@gmail.com>    *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************


__title__ = "CAM Slot Operation"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "https://www.freecad.org"
__doc__ = "Class and implementation of Slot operation."
__contributors__ = ""

import FreeCAD
from PySide import QtCore
import Path
import Path.Op.Base as PathOp
import PathScripts.PathUtils as PathUtils
import math

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


class ObjectSlot(PathOp.ObjectOp):
    """Proxy object for Slot operation."""

    def opFeatures(self, obj):
        """opFeatures(obj) ... return all standard features"""
        return (
            PathOp.FeatureTool
            | PathOp.FeatureDepths
            | PathOp.FeatureHeights
            | PathOp.FeatureStepDown
            | PathOp.FeatureCoolant
            | PathOp.FeatureBaseVertexes
            | PathOp.FeatureBaseEdges
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
                newPropMsg = translate("CAM_Slot", "New property added to")
                newPropMsg += ' "{}": {}'.format(obj.Label, self.addNewProps) + ". "
                newPropMsg += translate("CAM_Slot", "Check default value(s).")
                FreeCAD.Console.PrintWarning(newPropMsg + "\n")

        self.propertiesReady = True

    def opPropertyDefinitions(self):
        """opPropertyDefinitions(obj) ... Store operation specific properties"""

        return [
            (
                "App::PropertyBool",
                "ShowTempObjects",
                "Debug",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Show the temporary toolpath construction objects when module is in DEBUG mode.",
                ),
            ),
            (
                "App::PropertyVectorDistance",
                "CustomPoint1",
                "Slot",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property", "Enter custom start point for slot toolpath."
                ),
            ),
            (
                "App::PropertyVectorDistance",
                "CustomPoint2",
                "Slot",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property", "Enter custom end point for slot toolpath."
                ),
            ),
            (
                "App::PropertyEnumeration",
                "CutPattern",
                "Slot",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the geometric clearing pattern to use for the operation.",
                ),
            ),
            (
                "App::PropertyDistance",
                "ExtendPathStart",
                "Slot",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Positive extends the beginning of the toolpath, negative shortens.",
                ),
            ),
            (
                "App::PropertyDistance",
                "ExtendPathEnd",
                "Slot",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Positive extends the end of the toolpath, negative shortens.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "LayerMode",
                "Slot",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Complete the operation in a single pass at depth, or multiple passes to final depth.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "PathOrientation",
                "Slot",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Choose the toolpath orientation with regard to the feature(s) selected.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "Reference1",
                "Slot",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Choose what point to use on the first selected feature.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "Reference2",
                "Slot",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Choose what point to use on the second selected feature.",
                ),
            ),
            (
                "App::PropertyDistance",
                "ExtendRadius",
                "Slot",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "For arcs/circular edges, offset the radius for the toolpath.",
                ),
            ),
            (
                "App::PropertyBool",
                "ReverseDirection",
                "Slot",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Enable to reverse the cut direction of the slot toolpath.",
                ),
            ),
            (
                "App::PropertyVectorDistance",
                "StartPoint",
                "Start Point",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The custom start point for the toolpath of this operation",
                ),
            ),
            (
                "App::PropertyBool",
                "UseStartPoint",
                "Start Point",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Make True, if specifying a Start Point"),
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
            "CutPattern": [
                (translate("CAM_Slot", "Line"), "Line"),
                (translate("CAM_Slot", "ZigZag"), "ZigZag"),
            ],
            "LayerMode": [
                (translate("CAM_Slot", "Single-pass"), "Single-pass"),
                (translate("CAM_Slot", "Multi-pass"), "Multi-pass"),
            ],
            "PathOrientation": [
                (translate("CAM_Slot", "Start to End"), "Start to End"),
                (translate("CAM_Slot", "Perpendicular"), "Perpendicular"),
            ],
            "Reference1": [
                (translate("CAM_Slot", "Center of Mass"), "Center of Mass"),
                (
                    translate("CAM_Slot", "Center of Bounding Box"),
                    "Center of BoundBox",
                ),
                (translate("CAM_Slot", "Lowest Point"), "Lowest Point"),
                (translate("CAM_Slot", "Highest Point"), "Highest Point"),
                (translate("CAM_Slot", "Long Edge"), "Long Edge"),
                (translate("CAM_Slot", "Short Edge"), "Short Edge"),
                (translate("CAM_Slot", "Vertex"), "Vertex"),
            ],
            "Reference2": [
                (translate("CAM_Slot", "Center of Mass"), "Center of Mass"),
                (
                    translate("CAM_Slot", "Center of Bounding Box"),
                    "Center of BoundBox",
                ),
                (translate("CAM_Slot", "Lowest Point"), "Lowest Point"),
                (translate("CAM_Slot", "Highest Point"), "Highest Point"),
                (translate("CAM_Slot", "Vertex"), "Vertex"),
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
            "CustomPoint1": FreeCAD.Vector(0.0, 0.0, 0.0),
            "ExtendPathStart": 0.0,
            "Reference1": "Center of Mass",
            "CustomPoint2": FreeCAD.Vector(0.0, 0.0, 0.0),
            "ExtendPathEnd": 0.0,
            "Reference2": "Center of Mass",
            "LayerMode": "Multi-pass",
            "CutPattern": "ZigZag",
            "PathOrientation": "Start to End",
            "ExtendRadius": 0.0,
            "ReverseDirection": False,
            # For debugging
            "ShowTempObjects": False,
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
                if subCnt == 1:
                    # Adjust available enumerations
                    ENUMS["Reference1"] = self._makeReference1Enumerations(subsList[0], True)
                elif subCnt == 2:
                    # Adjust available enumerations
                    ENUMS["Reference1"] = self._makeReference1Enumerations(subsList[0])
                    ENUMS["Reference2"] = self._makeReference2Enumerations(subsList[1])
        return ENUMS

    def updateEnumerations(self, obj):
        """updateEnumerations(obj) ...
        Method updates property enumerations based on active conditions
        in the operation.  Returns the updated enumerations dictionary.
        Existing property values must be stored, and then restored after
        the assignment of updated enumerations."""
        Path.Log.debug("updateEnumerations()")
        # Save existing values
        pre_Ref1 = obj.Reference1
        pre_Ref2 = obj.Reference2

        # Update enumerations
        ENUMS = self.getActiveEnumerations(obj)
        obj.Reference1 = ENUMS["Reference1"]
        obj.Reference2 = ENUMS["Reference2"]

        # Restore pre-existing values if available with active enumerations.
        # If not, set to first element in active enumeration list.
        if pre_Ref1 in ENUMS["Reference1"]:
            obj.Reference1 = pre_Ref1
        else:
            obj.Reference1 = ENUMS["Reference1"][0]
        if pre_Ref2 in ENUMS["Reference2"]:
            obj.Reference2 = pre_Ref2
        else:
            obj.Reference2 = ENUMS["Reference2"][0]

        return ENUMS

    def opSetEditorModes(self, obj):
        # Used to hide inputs in properties list
        A = B = 2
        C = 0
        if hasattr(obj, "Base"):
            if obj.Base:
                # (base, subsList) = obj.Base[0]
                subsList = obj.Base[0][1]
                subCnt = len(subsList)
                if subCnt == 1:
                    A = 0
                elif subCnt == 2:
                    A = B = 0
                    C = 2

        obj.setEditorMode("Reference1", A)
        obj.setEditorMode("Reference2", B)
        obj.setEditorMode("ExtendRadius", C)

    def onChanged(self, obj, prop):
        if hasattr(self, "propertiesReady"):
            if self.propertiesReady:
                if prop in ["Base"]:
                    self.updateEnumerations(obj)
                    self.opSetEditorModes(obj)

    def opOnDocumentRestored(self, obj):
        self.propertiesReady = False
        job = PathUtils.findParentJob(obj)

        self.initOpProperties(obj, warn=True)
        self.opApplyPropertyDefaults(obj, job, self.addNewProps)

        mode = 2 if Path.Log.getLevel(Path.Log.thisModule()) != 4 else 0
        obj.setEditorMode("ShowTempObjects", mode)

        # Repopulate enumerations in case of changes
        ENUMS = self.updateEnumerations(obj)
        for n in ENUMS:
            restore = False
            if hasattr(obj, n):
                val = obj.getPropertyByName(n)
                restore = True
            setattr(obj, n, ENUMS[n])  # set the enumerations list
            if restore:
                setattr(obj, n, val)  # restore the value

        self.opSetEditorModes(obj)

    def opApplyPropertyDefaults(self, obj, job, propList):
        # Set standard property defaults
        PROP_DFLTS = self.opPropertyDefaults(obj, job)
        for n in PROP_DFLTS:
            if n in propList:
                prop = getattr(obj, n)
                val = PROP_DFLTS[n]
                setVal = False
                if hasattr(prop, "Value"):
                    if isinstance(val, int) or isinstance(val, float):
                        setVal = True
                if setVal:
                    # propVal = getattr(prop, 'Value')
                    setattr(prop, "Value", val)
                else:
                    setattr(obj, n, val)

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj, job) ... initialize defaults"""
        job = PathUtils.findParentJob(obj)

        self.opApplyPropertyDefaults(obj, job, self.addNewProps)

        # need to overwrite the default depth calculations for facing
        d = None
        if job:
            if job.Stock:
                d = PathUtils.guessDepths(job.Stock.Shape, None)
                Path.Log.debug("job.Stock exists")
            else:
                Path.Log.debug("job.Stock NOT exist")
        else:
            Path.Log.debug("job NOT exist")

        if d is not None:
            obj.OpFinalDepth.Value = d.final_depth
            obj.OpStartDepth.Value = d.start_depth
        else:
            obj.OpFinalDepth.Value = -10
            obj.OpStartDepth.Value = 10

        Path.Log.debug("Default OpFinalDepth: {}".format(obj.OpFinalDepth.Value))
        Path.Log.debug("Default OpStartDepth: {}".format(obj.OpStartDepth.Value))

    def opApplyPropertyLimits(self, obj):
        """opApplyPropertyLimits(obj) ... Apply necessary limits to user input property values before performing main operation."""
        pass

    def opUpdateDepths(self, obj):
        if hasattr(obj, "Base") and obj.Base:
            base, sublist = obj.Base[0]
            fbb = base.Shape.getElement(sublist[0]).BoundBox
            zmin = fbb.ZMax
            for base, sublist in obj.Base:
                for sub in sublist:
                    try:
                        fbb = base.Shape.getElement(sub).BoundBox
                        zmin = min(zmin, fbb.ZMin)
                    except Part.OCCError as e:
                        Path.Log.error(e)
            obj.OpFinalDepth = zmin

    def opExecute(self, obj):
        """opExecute(obj) ... process surface operation"""
        Path.Log.track()

        # Init operation state
        self.base = None
        self.shape1 = None
        self.shape2 = None
        self.shapeType1 = None
        self.shapeType2 = None
        self.shapeLength1 = None
        self.shapeLength2 = None
        self.dYdX1 = None
        self.dYdX2 = None
        self.bottomEdges = None
        self.isArc = 0
        self.arcCenter = None
        self.arcMidPnt = None
        self.arcRadius = 0.0
        self.newRadius = 0.0
        self.featureDetails = ["", ""]
        self.commandlist = []
        self.stockZMin = self.job.Stock.Shape.BoundBox.ZMin

        # Debug settings
        self.isDebug = Path.Log.getLevel(Path.Log.thisModule()) == 4
        self.showDebugObjects = self.isDebug and obj.ShowTempObjects

        if self.showDebugObjects:
            self._clearDebugGroups()
            self.tmpGrp = FreeCAD.ActiveDocument.addObject(
                "App::DocumentObjectGroup", "tmpDebugGrp"
            )

        # GCode operation header
        tool = obj.ToolController.Tool
        toolType = getattr(tool, "ShapeType", None)
        if toolType is None:
            Path.Log.warning("Tool does not define ShapeType, using label as fallback.")
            toolType = tool.Label

        if obj.Comment:
            self.commandlist.append(Path.Command(f"N ({obj.Comment})", {}))
        self.commandlist.append(Path.Command(f"N ({obj.Label})", {}))
        self.commandlist.append(Path.Command(f"N (Tool type: {toolType})", {}))
        self.commandlist.append(
            Path.Command(f"N (Compensated Tool Path. Diameter: {tool.Diameter})", {})
        )
        self.commandlist.append(Path.Command("N ()", {}))

        self.commandlist.append(
            Path.Command("G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid})
        )
        if obj.UseStartPoint:
            self.commandlist.append(
                Path.Command(
                    "G0",
                    {
                        "X": obj.StartPoint.x,
                        "Y": obj.StartPoint.y,
                        "F": self.horizRapid,
                    },
                )
            )

        # Enforce limits and prep depth steps
        self.opApplyPropertyLimits(obj)
        self.depthParams = PathUtils.depth_params(
            obj.ClearanceHeight.Value,
            obj.SafeHeight.Value,
            obj.StartDepth.Value,
            obj.StepDown.Value,
            0.0,
            obj.FinalDepth.Value,
        )

        # Main path generation
        cmds = self._makeOperation(obj)
        if cmds:
            self.commandlist.extend(cmds)

        # Final move to clearance height
        self.commandlist.append(
            Path.Command("G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid})
        )

        # Hide debug visuals
        if self.showDebugObjects and FreeCAD.GuiUp:
            FreeCADGui.ActiveDocument.getObject(self.tmpGrp.Name).Visibility = False
            self.tmpGrp.purgeTouched()

        return True

    def _clearDebugGroups(self):
        doc = FreeCAD.ActiveDocument
        for name in ["tmpDebugGrp", "tmpDebugGrp001"]:
            grp = getattr(doc, name, None)
            if grp:
                for obj in grp.Group:
                    doc.removeObject(obj.Name)
                doc.removeObject(name)

    # Control methods for operation
    def _makeOperation(self, obj):
        """This method controls the overall slot creation process."""
        pnts = False
        featureCount = 0

        if not hasattr(obj, "Base"):
            msg = translate("CAM_Slot", "No Base Geometry object in the operation.")
            FreeCAD.Console.PrintUserWarning(msg + "\n")
            return False

        if not obj.Base:
            # Use custom inputs here
            p1 = obj.CustomPoint1
            p2 = obj.CustomPoint2
            if p1 == p2:
                msg = translate(
                    "CAM_Slot", "Custom points are identical. No slot path will be generated"
                )
                FreeCAD.Console.PrintUserWarning(msg + "\n")
                return False
            elif p1.z == p2.z:
                pnts = (p1, p2)
                featureCount = 2
            else:
                msg = translate(
                    "CAM_Slot", "Custom points not at same Z height. No slot path will be generated"
                )
                FreeCAD.Console.PrintUserWarning(msg + "\n")
                return False
        else:
            baseGeom = obj.Base[0]
            base, subsList = baseGeom
            self.base = base

            featureCount = len(subsList)
            if featureCount == 1:
                Path.Log.debug("Reference 1: {}".format(obj.Reference1))
                sub1 = subsList[0]
                shape_1 = getattr(base.Shape, sub1)
                self.shape1 = shape_1
                pnts = self._processSingle(obj, shape_1, sub1)
            else:
                Path.Log.debug("Reference 1: {}".format(obj.Reference1))
                Path.Log.debug("Reference 2: {}".format(obj.Reference2))
                sub1 = subsList[0]
                sub2 = subsList[1]
                shape_1 = getattr(base.Shape, sub1)
                shape_2 = getattr(base.Shape, sub2)
                self.shape1 = shape_1
                self.shape2 = shape_2
                pnts = self._processDouble(obj, shape_1, sub1, shape_2, sub2)

        if not pnts:
            return False

        if self.isArc:
            cmds = self._finishArc(obj, pnts, featureCount)
        else:
            cmds = self._finishLine(obj, pnts, featureCount)

        if cmds:
            return cmds

        return False

    def _finishArc(self, obj, pnts, featureCnt):
        """This method finishes an Arc Slot operation.
        It returns the gcode for the slot operation."""
        Path.Log.debug("arc center: {}".format(self.arcCenter))
        self._addDebugObject(Part.makeLine(self.arcCenter, self.arcMidPnt), "CentToMidPnt")

        # Path.Log.debug('Pre-offset points are:\np1 = {}\np2 = {}'.format(p1, p2))
        if obj.ExtendRadius.Value != 0:
            # verify offset does not force radius < 0
            newRadius = self.arcRadius + obj.ExtendRadius.Value
            Path.Log.debug("arc radius: {};  offset radius: {}".format(self.arcRadius, newRadius))
            if newRadius <= 0:
                msg = translate(
                    "CAM_Slot",
                    "Current Extend Radius value produces negative arc radius.",
                )
                FreeCAD.Console.PrintError(msg + "\n")
                return False
            else:
                (p1, p2) = pnts
                pnts = self._makeOffsetArc(p1, p2, self.arcCenter, newRadius)
                self.newRadius = newRadius
        else:
            Path.Log.debug("arc radius: {}".format(self.arcRadius))
            self.newRadius = self.arcRadius

        # Apply path extension for arcs
        # Path.Log.debug('Pre-extension points are:\np1 = {}\np2 = {}'.format(p1, p2))
        if self.isArc == 1:
            # Complete circle
            if obj.ExtendPathStart.Value != 0 or obj.ExtendPathEnd.Value != 0:
                msg = translate("CAM_Slot", "No path extensions available for full circles.")
                FreeCAD.Console.PrintWarning(msg + "\n")
        else:
            # Arc segment
            # Apply extensions to slot path
            (p1, p2) = pnts
            begExt = obj.ExtendPathStart.Value
            endExt = obj.ExtendPathEnd.Value
            # invert endExt, begExt args to apply extensions to correct ends
            # XY geom is positive CCW; Gcode positive CW
            pnts = self._extendArcSlot(p1, p2, self.arcCenter, endExt, begExt)

        if not pnts:
            return False

        (p1, p2) = pnts
        # Path.Log.error('Post-offset points are:\np1 = {}\np2 = {}'.format(p1, p2))
        if self.isDebug:
            Path.Log.debug("Path Points are:\np1 = {}\np2 = {}".format(p1, p2))
            if p1.sub(p2).Length != 0:
                self._addDebugObject(Part.makeLine(p1, p2), "Path")

        if featureCnt:
            obj.CustomPoint1 = p1
            obj.CustomPoint2 = p2

        if self._arcCollisionCheck(obj, p1, p2, self.arcCenter, self.newRadius):
            msg = obj.Label + " "
            msg += translate("CAM_Slot", "operation collides with model.")
            FreeCAD.Console.PrintError(msg + "\n")

        # Path.Log.warning('Unable to create G-code.  _makeArcGCode() is incomplete.')
        cmds = self._makeArcGCode(obj, p1, p2)
        return cmds

    def _makeArcGCode(self, obj, p1, p2):
        """This method is the last step in the overall arc slot creation process.
        It accepts the operation object and two end points for the path.
        It returns the gcode for the slot operation."""
        CMDS = list()
        PATHS = [(p2, p1, "G2"), (p1, p2, "G3")]
        if obj.ReverseDirection:
            path_index = 1
        else:
            path_index = 0

        def arcPass(POINTS, depth):
            cmds = list()
            (st_pt, end_pt, arcCmd) = POINTS
            # cmds.append(Path.Command('N (Tool type: {})'.format(toolType), {}))
            cmds.append(Path.Command("G0", {"X": st_pt.x, "Y": st_pt.y, "F": self.horizRapid}))
            cmds.append(Path.Command("G1", {"Z": depth, "F": self.vertFeed}))
            vtc = self.arcCenter.sub(st_pt)  # vector to center
            cmds.append(
                Path.Command(
                    arcCmd,
                    {
                        "X": end_pt.x,
                        "Y": end_pt.y,
                        "I": vtc.x,
                        "J": vtc.y,
                        "F": self.horizFeed,
                    },
                )
            )
            return cmds

        if obj.LayerMode == "Single-pass":
            CMDS.extend(arcPass(PATHS[path_index], obj.FinalDepth.Value))
        else:
            if obj.CutPattern == "Line":
                for depth in self.depthParams:
                    CMDS.extend(arcPass(PATHS[path_index], depth))
                    CMDS.append(
                        Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid})
                    )
            elif obj.CutPattern == "ZigZag":
                i = 0
                for depth in self.depthParams:
                    if i % 2.0 == 0:  # even
                        CMDS.extend(arcPass(PATHS[path_index], depth))
                    else:  # odd
                        CMDS.extend(arcPass(PATHS[not path_index], depth))
                    i += 1
        # Raise to SafeHeight when finished
        CMDS.append(Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid}))

        if self.isDebug:
            Path.Log.debug("G-code arc command is: {}".format(PATHS[path_index][2]))

        return CMDS

    def _finishLine(self, obj, pnts, featureCnt):
        """This method finishes a Line Slot operation.
        It returns the gcode for the line slot operation."""
        # Apply perpendicular rotation if requested
        perpZero = True
        if obj.PathOrientation == "Perpendicular":
            if featureCnt == 2:
                if self.shapeType1 == "Face" and self.shapeType2 == "Face":
                    if self.bottomEdges:
                        self.bottomEdges.sort(key=lambda edg: edg.Length, reverse=True)
                        BE = self.bottomEdges[0]
                        pnts = self._processSingleVertFace(obj, BE)
                        perpZero = False
                elif self.shapeType1 == "Edge" and self.shapeType2 == "Edge":
                    Path.Log.debug("_finishLine() Perp, featureCnt == 2")
            if perpZero:
                (p1, p2) = pnts
                initPerpDist = p1.sub(p2).Length
                pnts = self._makePerpendicular(p1, p2, initPerpDist)  # 10.0 offset below
        else:
            # Modify path points if user selected two parallel edges
            if featureCnt == 2 and self.shapeType1 == "Edge" and self.shapeType2 == "Edge":
                if self.featureDetails[0] == "arc" and self.featureDetails[1] == "arc":
                    perpZero = False
                elif self._isParallel(self.dYdX1, self.dYdX2):
                    Path.Log.debug("_finishLine() StE, featureCnt == 2 // edges")
                    (p1, p2) = pnts
                    edg1_len = self.shape1.Length
                    edg2_len = self.shape2.Length
                    set_length = max(edg1_len, edg2_len)
                    pnts = self._makePerpendicular(p1, p2, 10.0 + set_length)  # 10.0 offset below
                    if edg1_len != edg2_len:
                        msg = obj.Label + " "
                        msg += translate("CAM_Slot", "Verify slot path start and end points.")
                        FreeCAD.Console.PrintWarning(msg + "\n")
            else:
                perpZero = False

        # Reverse direction of path if requested
        if obj.ReverseDirection:
            (p2, p1) = pnts
        else:
            (p1, p2) = pnts

        # Apply extensions to slot path
        begExt = obj.ExtendPathStart.Value
        endExt = obj.ExtendPathEnd.Value
        if perpZero:
            # Offsets for 10.0 value above in _makePerpendicular()
            begExt -= 5.0
            endExt -= 5.0
        pnts = self._extendLineSlot(p1, p2, begExt, endExt)

        if not pnts:
            return False

        (p1, p2) = pnts
        if self.isDebug:
            Path.Log.debug("Path Points are:\np1 = {}\np2 = {}".format(p1, p2))
            if p1.sub(p2).Length != 0:
                self._addDebugObject(Part.makeLine(p1, p2), "Path")

        if featureCnt:
            obj.CustomPoint1 = p1
            obj.CustomPoint2 = p2

        if self._lineCollisionCheck(obj, p1, p2):
            msg = obj.Label + " "
            msg += translate("CAM_Slot", "operation collides with model.")
            FreeCAD.Console.PrintWarning(msg + "\n")

        cmds = self._makeLineGCode(obj, p1, p2)
        return cmds

    def _makeLineGCode(self, obj, p1, p2):
        """This method is the last in the overall line slot creation process.
        It accepts the operation object and two end points for the path.
        It returns the gcode for the slot operation."""
        CMDS = list()

        def linePass(p1, p2, depth):
            cmds = list()
            # cmds.append(Path.Command('N (Tool type: {})'.format(toolType), {}))
            cmds.append(Path.Command("G0", {"X": p1.x, "Y": p1.y, "F": self.horizRapid}))
            cmds.append(Path.Command("G1", {"Z": depth, "F": self.vertFeed}))
            cmds.append(Path.Command("G1", {"X": p2.x, "Y": p2.y, "F": self.horizFeed}))
            return cmds

        # CMDS.append(Path.Command('N (Tool type: {})'.format(toolType), {}))
        if obj.LayerMode == "Single-pass":
            CMDS.extend(linePass(p1, p2, obj.FinalDepth.Value))
            CMDS.append(Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid}))
        else:
            if obj.CutPattern == "Line":
                for dep in self.depthParams:
                    CMDS.extend(linePass(p1, p2, dep))
                    CMDS.append(
                        Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid})
                    )
            elif obj.CutPattern == "ZigZag":
                CMDS.append(Path.Command("G0", {"X": p1.x, "Y": p1.y, "F": self.horizRapid}))
                i = 0
                for dep in self.depthParams:
                    if i % 2.0 == 0:  # even
                        CMDS.append(Path.Command("G1", {"Z": dep, "F": self.vertFeed}))
                        CMDS.append(Path.Command("G1", {"X": p2.x, "Y": p2.y, "F": self.horizFeed}))
                    else:  # odd
                        CMDS.append(Path.Command("G1", {"Z": dep, "F": self.vertFeed}))
                        CMDS.append(Path.Command("G1", {"X": p1.x, "Y": p1.y, "F": self.horizFeed}))
                    i += 1
            CMDS.append(Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid}))

        return CMDS

    # Methods for processing single geometry
    def _processSingle(self, obj, shape_1, sub1):
        """This is the control method for slots based on a
        single Base Geometry feature."""
        done = False
        cat1 = sub1[:4]

        if cat1 == "Face":
            pnts = False
            norm = shape_1.normalAt(0.0, 0.0)
            Path.Log.debug("{}.normalAt(): {}".format(sub1, norm))

            if Path.Geom.isRoughly(shape_1.BoundBox.ZMax, shape_1.BoundBox.ZMin):
                # Horizontal face
                if norm.z == 1 or norm.z == -1:
                    pnts = self._processSingleHorizFace(obj, shape_1)
                elif norm.z == 0:
                    faceType = self._getVertFaceType(shape_1)
                    if faceType:
                        (geo, shp) = faceType
                        if geo == "Face":
                            pnts = self._processSingleComplexFace(obj, shp)
                        if geo == "Wire":
                            pnts = self._processSingleVertFace(obj, shp)
                        if geo == "Edge":
                            pnts = self._processSingleVertFace(obj, shp)
            else:
                if len(shape_1.Edges) == 4:
                    pnts = self._processSingleHorizFace(obj, shape_1)
                else:
                    pnts = self._processSingleComplexFace(obj, shape_1)

            if not pnts:
                msg = translate("CAM_Slot", "The selected face is inaccessible.")
                FreeCAD.Console.PrintError(msg + "\n")
                return False

            if pnts:
                (p1, p2) = pnts
                done = True

        elif cat1 == "Edge":
            Path.Log.debug("Single edge")
            pnts = self._processSingleEdge(obj, shape_1)
            if pnts:
                (p1, p2) = pnts
                done = True

        elif cat1 == "Vert":
            msg = translate(
                "CAM_Slot",
                "Only a vertex selected. Add another feature to the Base Geometry.",
            )
            FreeCAD.Console.PrintError(msg + "\n")

        if done:
            return (p1, p2)

        return False

    def _processSingleHorizFace(self, obj, shape):
        """Determine slot path endpoints from a single horizontally oriented face."""
        Path.Log.debug("_processSingleHorizFace()")
        line_types = ["Part::GeomLine"]

        def get_edge_angle_deg(edge):
            vect = self._dXdYdZ(edge)
            norm = self._normalizeVector(vect)
            rads = self._getVectorAngle(norm)
            deg = math.degrees(rads)
            if deg >= 180.0:
                deg -= 180.0
            return deg

        # Reject triangular faces
        if len(shape.Edges) < 4:
            msg = translate("CAM_Slot", "A single selected face must have four edges minimum.")
            FreeCAD.Console.PrintError(msg + "\n")
            return False

        # Create tuples as (edge index, edge length, edge angle)
        edge_info_list = []
        for edge_index in range(4):
            edge = shape.Edges[edge_index]
            edge_length = edge.Length
            edge_angle = get_edge_angle_deg(edge)
            edge_info_list.append((edge_index, edge_length, edge_angle))

        # Sort edges by angle ascending
        edge_info_list.sort(key=lambda tup: tup[2])

        # Identify parallel edge pairs and track flags
        parallel_pairs = []
        parallel_flags = [0] * len(shape.Edges)
        current_flag = 1
        last_edge_index = len(shape.Edges) - 1

        for i in range(len(shape.Edges)):
            if i >= last_edge_index:
                continue

            next_i = i + 1
            edge_a_info = edge_info_list[i]
            edge_b_info = edge_info_list[next_i]
            angle_a = edge_a_info[2]
            angle_b = edge_b_info[2]

            if abs(angle_a - angle_b) >= 1e-6:  # consider improving with normalized angle diff
                continue

            edge_a = shape.Edges[edge_a_info[0]]
            edge_b = shape.Edges[edge_b_info[0]]

            debug_type_id = None
            if edge_a.Curve.TypeId not in line_types:
                debug_type_id = edge_a.Curve.TypeId
            elif edge_b.Curve.TypeId not in line_types:
                debug_type_id = edge_b.Curve.TypeId

            if debug_type_id:
                Path.Log.debug(f"Erroneous Curve.TypeId: {debug_type_id}")
            else:
                parallel_pairs.append((edge_a, edge_b))
                parallel_flags[edge_a_info[0]] = current_flag
                parallel_flags[edge_b_info[0]] = current_flag
                current_flag += 1

        pair_count = len(parallel_pairs)
        if pair_count > 1:
            # Sort pairs by longest edge first
            parallel_pairs.sort(key=lambda pair: pair[0].Length, reverse=True)

        if self.isDebug:
            Path.Log.debug(f" - Parallel pair count: {pair_count}")
            for edge1, edge2 in parallel_pairs:
                Path.Log.debug(
                    f" - Pair lengths: {round(edge1.Length, 4)}, {round(edge2.Length, 4)}"
                )
            Path.Log.debug(f" - Parallel flags: {parallel_flags}")

        if pair_count == 0:
            msg = translate("CAM_Slot", "No parallel edges identified.")
            FreeCAD.Console.PrintError(msg + "\n")
            return False

        if pair_count == 1:
            if len(shape.Edges) == 4:
                # Find edges that are NOT in the identified parallel pair
                non_parallel_edges = [
                    shape.Edges[i] for i, flag in enumerate(parallel_flags) if flag == 0
                ]
                if len(non_parallel_edges) == 2:
                    selected_edges = (non_parallel_edges[0], non_parallel_edges[1])
                else:
                    selected_edges = parallel_pairs[0]
            else:
                selected_edges = parallel_pairs[0]
        else:
            if obj.Reference1 == "Long Edge":
                selected_edges = parallel_pairs[1]
            elif obj.Reference1 == "Short Edge":
                selected_edges = parallel_pairs[0]
            else:
                msg = "Reference1 " + translate("CAM_Slot", "value error.")
                FreeCAD.Console.PrintError(msg + "\n")
                return False

        (point1, point2) = self._getOppMidPoints(selected_edges)
        return (point1, point2)

    def _processSingleComplexFace(self, obj, shape):
        """Determine slot path endpoints from a single complex face."""
        Path.Log.debug("_processSingleComplexFace()")
        pnts = list()

        def zVal(p):
            return p.z

        for E in shape.Wires[0].Edges:
            p = self._findLowestEdgePoint(E)
            pnts.append(p)
        pnts.sort(key=zVal)
        return (pnts[0], pnts[1])

    def _processSingleVertFace(self, obj, shape):
        """Determine slot path endpoints from a single vertically oriented face
        with no single bottom edge."""
        Path.Log.debug("_processSingleVertFace()")
        eCnt = len(shape.Edges)
        V0 = shape.Edges[0].Vertexes[0]
        V1 = shape.Edges[eCnt - 1].Vertexes[1]
        v0 = FreeCAD.Vector(V0.X, V0.Y, V0.Z)
        v1 = FreeCAD.Vector(V1.X, V1.Y, V1.Z)

        dX = V1.X - V0.X
        dY = V1.Y - V0.Y
        dZ = V1.Z - V0.Z
        temp = FreeCAD.Vector(dX, dY, dZ)
        slope = self._normalizeVector(temp)
        perpVect = FreeCAD.Vector(-1 * slope.y, slope.x, slope.z)
        perpVect.multiply(self.tool.Diameter / 2.0)

        # Create offset endpoints for raw slot path
        a1 = v0.add(perpVect)
        a2 = v1.add(perpVect)
        b1 = v0.sub(perpVect)
        b2 = v1.sub(perpVect)
        (p1, p2) = self._getCutSidePoints(obj, v0, v1, a1, a2, b1, b2)

        msg = obj.Label + " "
        msg += translate("CAM_Slot", "Verify slot path start and end points.")
        FreeCAD.Console.PrintWarning(msg + "\n")

        return (p1, p2)

    def _processSingleEdge(self, obj, edge):
        """Determine slot path endpoints from a single horizontally oriented edge."""
        Path.Log.debug("_processSingleEdge()")
        tol = 1e-7
        lineTypes = {"Part::GeomLine"}
        curveTypes = {"Part::GeomCircle"}

        def oversizedTool(holeDiam):
            if self.tool.Diameter > holeDiam:
                msg = translate("CAM_Slot", "Current tool larger than arc diameter.")
                FreeCAD.Console.PrintError(msg + "\n")
                return True
            return False

        def isHorizontal(z1, z2, z3):
            return abs(z1 - z2) <= tol and abs(z1 - z3) <= tol

        def circumCircleFrom3Points(P1, P2, P3):
            v1 = P2 - P1
            v2 = P3 - P2
            v3 = P1 - P3
            L = v1.cross(v2).Length
            if round(L, 8) == 0.0:
                Path.Log.error("Three points are colinear. Arc is straight.")
                return False
            twoL2 = 2 * L * L
            a = -v2.dot(v2) * v1.dot(v3) / twoL2
            b = -v3.dot(v3) * v2.dot(v1) / twoL2
            c = -v1.dot(v1) * v3.dot(v2) / twoL2
            return P1 * a + P2 * b + P3 * c

        verts = edge.Vertexes
        V1 = verts[0]
        p1 = FreeCAD.Vector(V1.X, V1.Y, 0.0)
        p2 = p1 if len(verts) == 1 else FreeCAD.Vector(verts[1].X, verts[1].Y, 0.0)

        curveType = edge.Curve.TypeId
        if curveType in lineTypes:
            return (p1, p2)

        elif curveType in curveTypes:
            if len(verts) == 1:
                # Full circle
                Path.Log.debug("Arc with single vertex (circle).")
                if oversizedTool(edge.BoundBox.XLength):
                    return False
                self.isArc = 1
                tp1 = edge.valueAt(edge.getParameterByLength(edge.Length * 0.33))
                tp2 = edge.valueAt(edge.getParameterByLength(edge.Length * 0.66))
                if not isHorizontal(V1.Z, tp1.z, tp2.z):
                    return False

                center = edge.BoundBox.Center
                self.arcCenter = FreeCAD.Vector(center.x, center.y, 0.0)
                mid = edge.valueAt(edge.getParameterByLength(edge.Length / 2.0))
                self.arcMidPnt = FreeCAD.Vector(mid.x, mid.y, 0.0)
                self.arcRadius = edge.BoundBox.XLength / 2.0
            else:
                # Arc segment
                Path.Log.debug("Arc with multiple vertices.")
                V2 = verts[1]
                mid = edge.valueAt(edge.getParameterByLength(edge.Length / 2.0))
                if not isHorizontal(V1.Z, V2.Z, mid.z):
                    return False
                mid.z = 0.0
                center = circumCircleFrom3Points(p1, p2, FreeCAD.Vector(mid.x, mid.y, 0.0))
                if not center:
                    return False

                self.isArc = 2
                self.arcMidPnt = FreeCAD.Vector(mid.x, mid.y, 0.0)
                self.arcCenter = center
                self.arcRadius = (p1 - center).Length

                if oversizedTool(self.arcRadius * 2.0):
                    return False

            return (p1, p2)

        else:
            msg = translate(
                "CAM_Slot", "Failed, slot from edge only accepts lines, arcs and circles."
            )
            FreeCAD.Console.PrintError(msg + "\n")
            return False

    # Methods for processing double geometry
    def _processDouble(self, obj, shape_1, sub1, shape_2, sub2):
        """This is the control method for slots based on a
        two Base Geometry features."""
        Path.Log.debug("_processDouble()")

        p1 = None
        p2 = None
        dYdX1 = None
        dYdX2 = None
        self.bottomEdges = list()

        feature1 = self._processFeature(obj, shape_1, sub1, 1)
        if not feature1:
            msg = translate("CAM_Slot", "Failed to determine point 1 from")
            FreeCAD.Console.PrintError(msg + " {}.\n".format(sub1))
            return False
        (p1, dYdX1, shpType) = feature1
        self.shapeType1 = shpType
        if dYdX1:
            self.dYdX1 = dYdX1

        feature2 = self._processFeature(obj, shape_2, sub2, 2)
        if not feature2:
            msg = translate("CAM_Slot", "Failed to determine point 2 from")
            FreeCAD.Console.PrintError(msg + " {}.\n".format(sub2))
            return False
        (p2, dYdX2, shpType) = feature2
        self.shapeType2 = shpType
        if dYdX2:
            self.dYdX2 = dYdX2

        # Parallel check for twin face, and face-edge cases
        if dYdX1 and dYdX2:
            Path.Log.debug("dYdX1, dYdX2: {}, {}".format(dYdX1, dYdX2))
            if not self._isParallel(dYdX1, dYdX2):
                if self.shapeType1 != "Edge" or self.shapeType2 != "Edge":
                    msg = translate("CAM_Slot", "Selected geometry not parallel.")
                    FreeCAD.Console.PrintError(msg + "\n")
                    return False

        if p2:
            return (p1, p2)

        return False

    # Support methods
    def _dXdYdZ(self, E):
        v1 = E.Vertexes[0]
        v2 = E.Vertexes[1]
        dX = v2.X - v1.X
        dY = v2.Y - v1.Y
        dZ = v2.Z - v1.Z
        return FreeCAD.Vector(dX, dY, dZ)

    def _normalizeVector(self, v):
        """Return a normalized vector with components rounded to nearest axis-aligned value if close."""
        tol = 1e-10
        V = FreeCAD.Vector(v).normalize()

        def snap(val):
            if abs(val) < tol:
                return 0.0
            if abs(1.0 - abs(val)) < tol:
                return 1.0 if val > 0 else -1.0
            return val

        return FreeCAD.Vector(snap(V.x), snap(V.y), snap(V.z))

    def _getLowestPoint(self, shape):
        """Return the average XY of the vertices with the lowest Z value."""
        vertices = shape.Vertexes
        lowest_z = min(v.Z for v in vertices)
        lowest_vertices = [v for v in vertices if v.Z == lowest_z]

        avg_x = sum(v.X for v in lowest_vertices) / len(lowest_vertices)
        avg_y = sum(v.Y for v in lowest_vertices) / len(lowest_vertices)
        return FreeCAD.Vector(avg_x, avg_y, lowest_z)

    def _getHighestPoint(self, shape):
        """Return the average XY of the vertices with the highest Z value."""
        vertices = shape.Vertexes
        highest_z = max(v.Z for v in vertices)
        highest_vertices = [v for v in vertices if v.Z == highest_z]

        avg_x = sum(v.X for v in highest_vertices) / len(highest_vertices)
        avg_y = sum(v.Y for v in highest_vertices) / len(highest_vertices)
        return FreeCAD.Vector(avg_x, avg_y, highest_z)

    def _processFeature(self, obj, shape, sub, pNum):
        """Analyze a shape and return a tuple: (working point, slope, category)."""
        p = None
        dYdX = None

        Ref = getattr(obj, f"Reference{pNum}")

        if sub.startswith("Face"):
            cat = "Face"
            BE = self._getBottomEdge(shape)
            if BE:
                self.bottomEdges.append(BE)

            # Get slope from first vertex to center of mass
            V0 = shape.Vertexes[0]
            v1 = shape.CenterOfMass
            temp = FreeCAD.Vector(v1.x - V0.X, v1.y - V0.Y, 0.0)
            dYdX = self._normalizeVector(temp) if temp.Length != 0 else FreeCAD.Vector(0, 0, 0)

            # Face normal must be vertical
            norm = shape.normalAt(0.0, 0.0)
            if norm.z != 0:
                msg = translate("CAM_Slot", "The selected face is not oriented vertically:")
                FreeCAD.Console.PrintError(f"{msg} {sub}.\n")
                return False

            # Choose working point
            if Ref == "Center of Mass":
                com = shape.CenterOfMass
                p = FreeCAD.Vector(com.x, com.y, 0.0)
            elif Ref == "Center of BoundBox":
                bbox = shape.BoundBox.Center
                p = FreeCAD.Vector(bbox.x, bbox.y, 0.0)
            elif Ref == "Lowest Point":
                p = self._getLowestPoint(shape)
            elif Ref == "Highest Point":
                p = self._getHighestPoint(shape)

        elif sub.startswith("Edge"):
            cat = "Edge"
            featDetIdx = pNum - 1
            if shape.Curve.TypeId == "Part::GeomCircle":
                self.featureDetails[featDetIdx] = "arc"

            edge = shape.Edges[0] if hasattr(shape, "Edges") else shape
            v0 = edge.Vertexes[0]
            v1 = edge.Vertexes[1]
            temp = FreeCAD.Vector(v1.X - v0.X, v1.Y - v0.Y, 0.0)
            dYdX = self._normalizeVector(temp) if temp.Length != 0 else FreeCAD.Vector(0, 0, 0)

            if Ref == "Center of Mass":
                com = shape.CenterOfMass
                p = FreeCAD.Vector(com.x, com.y, 0.0)
            elif Ref == "Center of BoundBox":
                bbox = shape.BoundBox.Center
                p = FreeCAD.Vector(bbox.x, bbox.y, 0.0)
            elif Ref == "Lowest Point":
                p = self._findLowestPointOnEdge(shape)
            elif Ref == "Highest Point":
                p = self._findHighestPointOnEdge(shape)

        elif sub.startswith("Vert"):
            cat = "Vert"
            V = shape.Vertexes[0]
            p = FreeCAD.Vector(V.X, V.Y, 0.0)

        else:
            Path.Log.warning(f"Unrecognized subfeature type: {sub}")
            return False

        if p:
            return (p, dYdX, cat)

        return False

    def _extendArcSlot(self, p1, p2, cent, begExt, endExt):
        """Extend an arc defined by endpoints p1, p2 and center cent.
        begExt and endExt are extension lengths along the arc at each end.
        Returns new (p1, p2) as (n1, n2)."""
        if not begExt and not endExt:
            return (p1, p2)

        def makeChord(angle_rad):
            x = self.newRadius * math.cos(angle_rad)
            y = self.newRadius * math.sin(angle_rad)
            a = FreeCAD.Vector(self.newRadius, 0, 0)
            b = FreeCAD.Vector(x, y, 0)
            return Part.makeLine(a, b)

        origin = FreeCAD.Vector(0, 0, 0)
        z_axis = FreeCAD.Vector(0, 0, 1)

        n1, n2 = p1, p2

        if begExt:
            ext_rad = abs(begExt / self.newRadius)
            angle = self._getVectorAngle(p1.sub(self.arcCenter))
            angle += -2 * ext_rad if begExt > 0 else 0
            chord = makeChord(ext_rad)
            chord.rotate(origin, z_axis, math.degrees(angle))
            chord.translate(self.arcCenter)
            self._addDebugObject(chord, "ExtendStart")
            n1 = chord.Vertexes[1].Point

        if endExt:
            ext_rad = abs(endExt / self.newRadius)
            angle = self._getVectorAngle(p2.sub(self.arcCenter))
            angle += 0 if endExt > 0 else -2 * ext_rad
            chord = makeChord(ext_rad)
            chord.rotate(origin, z_axis, math.degrees(angle))
            chord.translate(self.arcCenter)
            self._addDebugObject(chord, "ExtendEnd")
            n2 = chord.Vertexes[1].Point

        return (n1, n2)

    def _makeOffsetArc(self, p1, p2, center, newRadius):
        """_makeOffsetArc(p1, p2, center, newRadius)...
        This function offsets an arc defined by endpoints, p1 and p2, and the center.
        New end points are returned at the radius passed by newRadius.
        The angle of the original arc is maintained."""
        n1 = p1.sub(center).normalize() * newRadius
        n2 = p2.sub(center).normalize() * newRadius
        return (n1.add(center), n2.add(center))

    def _extendLineSlot(self, p1, p2, begExt, endExt):
        """_extendLineSlot(p1, p2, begExt, endExt)...
        This function extends a line defined by endpoints, p1 and p2.
        The beginning is extended by begExt value and the end by endExt value."""
        if begExt:
            beg = p1.sub(p2)
            n1 = p1.add(beg.normalize() * begExt)
        else:
            n1 = p1
        if endExt:
            end = p2.sub(p1)
            n2 = p2.add(end.normalize() * endExt)
        else:
            n2 = p2
        return (n1, n2)

    def _getOppMidPoints(self, same):
        """_getOppMidPoints(same)...
        Find mid-points between ends of equal, oppossing edges passed in tuple (edge1, edge2)."""
        com1 = same[0].CenterOfMass
        com2 = same[1].CenterOfMass
        p1 = FreeCAD.Vector(com1.x, com1.y, 0.0)
        p2 = FreeCAD.Vector(com2.x, com2.y, 0.0)
        return (p1, p2)

    def _isParallel(self, dYdX1, dYdX2):
        """Determine if two orientation vectors are parallel."""
        return dYdX1.cross(dYdX2) == FreeCAD.Vector(0, 0, 0)

    def _makePerpendicular(self, p1, p2, length):
        """Using a line defined by p1 and p2, returns a perpendicular vector
        centered at the midpoint of the line, with given length."""

        midPnt = (p1.add(p2)).multiply(0.5)
        halfDist = length / 2.0

        if getattr(self, "dYdX1", None):
            half = FreeCAD.Vector(self.dYdX1.x, self.dYdX1.y, 0.0).multiply(halfDist)
            n1 = midPnt.add(half)
            n2 = midPnt.sub(half)
            return (n1, n2)

        elif getattr(self, "dYdX2", None):
            half = FreeCAD.Vector(self.dYdX2.x, self.dYdX2.y, 0.0).multiply(halfDist)
            n1 = midPnt.add(half)
            n2 = midPnt.sub(half)
            return (n1, n2)

        else:
            toEnd = p2.sub(p1)
            perp = FreeCAD.Vector(-toEnd.y, toEnd.x, 0.0)
            perp = perp.normalize()  # normalize() returns the vector normalized
            perp = perp.multiply(halfDist)
            n1 = midPnt.add(perp)
            n2 = midPnt.sub(perp)
            return (n1, n2)

    def _findLowestPointOnEdge(self, E):
        tol = 1e-7
        zMin = E.BoundBox.ZMin

        # Try each vertex
        for v in E.Vertexes:
            if abs(v.Z - zMin) < tol:
                return FreeCAD.Vector(v.X, v.Y, v.Z)

        # Try midpoint
        mid = E.valueAt(E.getParameterByLength(E.Length / 2.0))
        if abs(mid.z - zMin) < tol or E.BoundBox.ZLength < 1e-9:
            return mid

        # Fallback
        return self._findLowestEdgePoint(E)

    def _findLowestEdgePoint(self, E):
        zMin = E.BoundBox.ZMin
        L0, L1 = 0.0, E.Length
        tol = 1e-5
        max_iter = 2000
        cnt = 0

        while (L1 - L0) > tol and cnt < max_iter:
            p0 = E.valueAt(E.getParameterByLength(L0))
            p1 = E.valueAt(E.getParameterByLength(L1))

            diff0 = p0.z - zMin
            diff1 = p1.z - zMin

            adj = (L1 - L0) * 0.1
            if diff0 < diff1:
                L1 -= adj
            elif diff0 > diff1:
                L0 += adj
            else:
                # When equal, narrow from both ends
                L0 += adj
                L1 -= adj
            cnt += 1

        midLen = (L0 + L1) / 2.0
        return E.valueAt(E.getParameterByLength(midLen))

    def _findHighestPointOnEdge(self, E):
        tol = 1e-7
        zMax = E.BoundBox.ZMax

        # Check first vertex
        v = E.Vertexes[0]
        if abs(zMax - v.Z) < tol:
            return FreeCAD.Vector(v.X, v.Y, v.Z)

        # Check second vertex
        v = E.Vertexes[1]
        if abs(zMax - v.Z) < tol:
            return FreeCAD.Vector(v.X, v.Y, v.Z)

        # Check midpoint on edge
        midLen = E.Length / 2.0
        midPnt = E.valueAt(E.getParameterByLength(midLen))
        if abs(zMax - midPnt.z) < tol or E.BoundBox.ZLength < 1e-9:
            return midPnt

        return self._findHighestEdgePoint(E)

    def _findHighestEdgePoint(self, E):
        zMax = E.BoundBox.ZMax
        eLen = E.Length
        L0 = 0.0
        L1 = eLen
        cnt = 0
        while L1 - L0 > 1e-5 and cnt < 2000:
            adj = (L1 - L0) * 0.1
            p0 = E.valueAt(E.getParameterByLength(L0))
            p1 = E.valueAt(E.getParameterByLength(L1))

            diff0 = zMax - p0.z
            diff1 = zMax - p1.z

            # Closer to zMax means smaller diff (diff >= 0)
            if diff0 > diff1:
                # p1 is closer to zMax, so move L0 up to narrow range toward p1
                L0 += adj
            elif diff0 < diff1:
                # p0 is closer, move L1 down to narrow range toward p0
                L1 -= adj
            else:
                L0 += adj
                L1 -= adj

            cnt += 1

        midLen = (L0 + L1) / 2.0
        return E.valueAt(E.getParameterByLength(midLen))

    def _getVectorAngle(self, v):
        return math.atan2(v.y, v.x) % (2 * math.pi)

    def _getCutSidePoints(self, obj, v0, v1, a1, a2, b1, b2):
        ea1 = Part.makeLine(v0, a1)
        ea2 = Part.makeLine(a1, a2)
        ea3 = Part.makeLine(a2, v1)
        ea4 = Part.makeLine(v1, v0)
        boxA = Part.Face(Part.Wire([ea1, ea2, ea3, ea4]))
        cubeA = boxA.extrude(FreeCAD.Vector(0.0, 0.0, 1.0))
        cmnA = self.base.Shape.common(cubeA)
        eb1 = Part.makeLine(v0, b1)
        eb2 = Part.makeLine(b1, b2)
        eb3 = Part.makeLine(b2, v1)
        eb4 = Part.makeLine(v1, v0)
        boxB = Part.Face(Part.Wire([eb1, eb2, eb3, eb4]))
        cubeB = boxB.extrude(FreeCAD.Vector(0.0, 0.0, 1.0))
        cmnB = self.base.Shape.common(cubeB)
        if cmnA.Volume > cmnB.Volume:
            return (b1, b2)
        return (a1, a2)

    def _getBottomEdge(self, shape):
        EDGES = list()
        # Determine if selected face has a single bottom horizontal edge
        eCnt = len(shape.Edges)
        eZMin = shape.BoundBox.ZMin
        for ei in range(0, eCnt):
            E = shape.Edges[ei]
            if abs(E.BoundBox.ZMax - eZMin) < 0.00000001:
                EDGES.append(E)
        if len(EDGES) == 1:  # single bottom horiz. edge
            return EDGES[0]
        return False

    def _getVertFaceType(self, shape):
        bottom_edge = self._getBottomEdge(shape)
        if bottom_edge:
            return ("Edge", bottom_edge)

        # Extrude vertically to create a sliceable solid
        z_length = shape.BoundBox.ZLength
        extrude_vec = FreeCAD.Vector(0, 0, z_length * 2.2 + 10)
        extruded = shape.extrude(extrude_vec)

        # Slice halfway up the extrusion
        slice_z = shape.BoundBox.ZMin + extrude_vec.z / 2.0
        slices = extruded.slice(FreeCAD.Vector(0, 0, 1), slice_z)

        if not slices:
            return False

        if (wire := slices[0]).isClosed() and (face := Part.Face(wire)) > 0:
            # Align face Z with original shape
            z_offset = shape.BoundBox.ZMin - face.BoundBox.ZMin
            face.translate(FreeCAD.Vector(0, 0, z_offset))
            return ("Face", face)
        return ("Wire", wire)

    def _makeReference1Enumerations(self, sub, single=False):
        """Customize Reference1 enumerations based on feature type."""
        Path.Log.debug("_makeReference1Enumerations()")
        cat = sub[:4]
        if single:
            if cat == "Face":
                return ["Long Edge", "Short Edge"]
            elif cat == "Edge":
                return ["Long Edge"]
            elif cat == "Vert":
                return ["Vertex"]
        elif cat == "Vert":
            return ["Vertex"]

        return ["Center of Mass", "Center of BoundBox", "Lowest Point", "Highest Point"]

    def _makeReference2Enumerations(self, sub):
        """Customize Reference2 enumerations based on feature type."""
        Path.Log.debug("_makeReference2Enumerations()")
        cat = sub[:4]
        if cat == "Vert":
            return ["Vertex"]
        return ["Center of Mass", "Center of BoundBox", "Lowest Point", "Highest Point"]

    def _lineCollisionCheck(self, obj, p1, p2):
        """Model the swept volume of a linear tool move and check for collision with the model."""
        rad = getattr(self.tool.Diameter, "Value", self.tool.Diameter) / 2.0
        extVect = FreeCAD.Vector(0.0, 0.0, obj.StartDepth.Value - obj.FinalDepth.Value)

        def make_cylinder(point):
            circle = Part.makeCircle(rad, point)
            face = Part.Face(Part.Wire(circle.Edges))
            face.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - face.BoundBox.ZMin))
            return face.extrude(extVect)

        def make_rect_prism(p1, p2):
            toEnd = p2.sub(p1)
            if toEnd.Length == 0:
                return None
            perp = FreeCAD.Vector(-toEnd.y, toEnd.x, 0.0)
            if perp.Length == 0:
                return None
            perp.normalize()
            perp.multiply(rad)

            v1, v2 = p1.add(perp), p1.sub(perp)
            v3, v4 = p2.sub(perp), p2.add(perp)
            edges = Part.__sortEdges__(
                [
                    Part.makeLine(v1, v2),
                    Part.makeLine(v2, v3),
                    Part.makeLine(v3, v4),
                    Part.makeLine(v4, v1),
                ]
            )
            face = Part.Face(Part.Wire(edges))
            face.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - face.BoundBox.ZMin))
            return face.extrude(extVect)

        # Build swept volume
        startShp = make_cylinder(p1)
        endShp = make_cylinder(p2) if p1 != p2 else None
        boxShp = make_rect_prism(p1, p2)

        pathTravel = startShp
        if boxShp:
            pathTravel = pathTravel.fuse(boxShp)
        if endShp:
            pathTravel = pathTravel.fuse(endShp)

        self._addDebugObject(pathTravel, "PathTravel")

        try:
            cmn = self.base.Shape.common(pathTravel)
            return cmn.Volume > 1e-6
        except Exception:
            Path.Log.debug("Failed to complete path collision check.")
            return False

    def _arcCollisionCheck(self, obj, p1, p2, arcCenter, arcRadius):
        """Check for collision by modeling the swept volume of an arc toolpath."""

        def make_cylinder_at_point(point, radius, height, final_depth):
            circle = Part.makeCircle(radius, point)
            face = Part.Face(Part.Wire(circle.Edges))
            face.translate(FreeCAD.Vector(0, 0, final_depth - face.BoundBox.ZMin))
            return face.extrude(FreeCAD.Vector(0, 0, height))

        def make_arc_face(p1, p2, center, inner_radius, outer_radius):
            (pA, pB) = self._makeOffsetArc(p1, p2, center, inner_radius)
            arc_inside = Arcs.arcFrom2Pts(pA, pB, center)

            (pC, pD) = self._makeOffsetArc(p1, p2, center, outer_radius)
            arc_outside = Arcs.arcFrom2Pts(pC, pD, center)

            pa = FreeCAD.Vector(*arc_inside.Vertexes[0].Point[:2], 0.0)
            pb = FreeCAD.Vector(*arc_inside.Vertexes[1].Point[:2], 0.0)
            pc = FreeCAD.Vector(*arc_outside.Vertexes[1].Point[:2], 0.0)
            pd = FreeCAD.Vector(*arc_outside.Vertexes[0].Point[:2], 0.0)

            e1 = Part.makeLine(pb, pc)
            e2 = Part.makeLine(pd, pa)
            edges = Part.__sortEdges__([arc_inside, e1, arc_outside, e2])
            return Part.Face(Part.Wire(edges))

        # Radius and extrusion direction
        rad = getattr(self.tool.Diameter, "Value", self.tool.Diameter) / 2.0
        extVect = FreeCAD.Vector(0, 0, obj.StartDepth.Value - obj.FinalDepth.Value)

        if self.isArc == 1:
            # Full circle slot: make annular ring
            outer = Part.Face(Part.Wire(Part.makeCircle(arcRadius + rad, arcCenter).Edges))
            iRadius = arcRadius - rad
            path = (
                outer.cut(Part.Face(Part.Wire(Part.makeCircle(iRadius, arcCenter).Edges)))
                if iRadius > 0
                else outer
            )
            path.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - path.BoundBox.ZMin))
            pathTravel = path.extrude(extVect)

        else:
            # Arc slot with entry and exit cylinders
            startShp = make_cylinder_at_point(p1, rad, extVect.z, obj.FinalDepth.Value)
            endShp = make_cylinder_at_point(p2, rad, extVect.z, obj.FinalDepth.Value)

            # Validate inner arc
            inner_radius = arcRadius - rad
            if inner_radius <= 0:
                FreeCAD.Console.PrintError(
                    translate("CAM_Slot", "Current offset value produces negative radius.") + "\n"
                )
                return False

            # Validate outer arc
            outer_radius = arcRadius + rad
            if outer_radius <= 0:
                FreeCAD.Console.PrintError(
                    translate("CAM_Slot", "Current offset value produces negative radius.") + "\n"
                )
                return False

            rectFace = make_arc_face(p1, p2, arcCenter, inner_radius, outer_radius)
            rectFace.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - rectFace.BoundBox.ZMin))
            arcShp = rectFace.extrude(extVect)

            pathTravel = startShp.fuse(arcShp).fuse(endShp)

        self._addDebugObject(pathTravel, "PathTravel")

        try:
            cmn = self.base.Shape.common(pathTravel)
            return cmn.Volume > 1e-6
        except Exception:
            Path.Log.debug("Failed to complete path collision check.")
            return False

    def _addDebugObject(self, objShape, objName):
        if self.showDebugObjects:
            do = FreeCAD.ActiveDocument.addObject("Part::Feature", "tmp_" + objName)
            do.Shape = objShape
            do.purgeTouched()
            self.tmpGrp.addObject(do)


# Eclass


def SetupProperties():
    """SetupProperties() ... Return list of properties required for operation."""
    return [tup[1] for tup in ObjectSlot.opPropertyDefinitions(False)]


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Slot operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectSlot(obj, name, parentJob)
    return obj
