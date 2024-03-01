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

        for (prtyp, nm, grp, tt) in self.opPropertyDefinitions():
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
                    "For arcs/circlular edges, offset the radius for the toolpath.",
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
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property", "Make True, if specifying a Start Point"
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
                    ENUMS["Reference1"] = self._makeReference1Enumerations(
                        subsList[0], True
                    )
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
        self.stockZMin = None
        self.isArc = 0
        self.arcCenter = None
        self.arcMidPnt = None
        self.arcRadius = 0.0
        self.newRadius = 0.0
        self.featureDetails = ["", ""]
        self.isDebug = False if Path.Log.getLevel(Path.Log.thisModule()) != 4 else True
        self.showDebugObjects = False
        self.stockZMin = self.job.Stock.Shape.BoundBox.ZMin
        CMDS = list()

        try:
            dotIdx = __name__.index(".") + 1
        except Exception:
            dotIdx = 0
        self.module = __name__[dotIdx:]

        # Setup debugging group for temp objects, when in DEBUG mode
        if self.isDebug:
            self.showDebugObjects = obj.ShowTempObjects
        if self.showDebugObjects:
            FCAD = FreeCAD.ActiveDocument
            for grpNm in ["tmpDebugGrp", "tmpDebugGrp001"]:
                if hasattr(FCAD, grpNm):
                    for go in FCAD.getObject(grpNm).Group:
                        FCAD.removeObject(go.Name)
                    FCAD.removeObject(grpNm)
            self.tmpGrp = FCAD.addObject("App::DocumentObjectGroup", "tmpDebugGrp")

        # Begin GCode for operation with basic information
        # ... and move cutter to clearance height and startpoint
        tool = obj.ToolController.Tool
        toolType = tool.ToolType if hasattr(tool, "ToolType") else tool.ShapeName
        output = ""
        if obj.Comment != "":
            self.commandlist.append(Path.Command("N ({})".format(obj.Comment), {}))
        self.commandlist.append(Path.Command("N ({})".format(obj.Label), {}))
        self.commandlist.append(Path.Command("N (Tool type: {})".format(toolType), {}))
        self.commandlist.append(
            Path.Command(
                "N (Compensated Tool Path. Diameter: {})".format(tool.Diameter), {}
            )
        )
        self.commandlist.append(Path.Command("N ({})".format(output), {}))
        self.commandlist.append(
            Path.Command("G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid})
        )
        if obj.UseStartPoint is True:
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

        # Impose property limits
        self.opApplyPropertyLimits(obj)

        # Calculate default depthparams for operation
        self.depthParams = PathUtils.depth_params(
            obj.ClearanceHeight.Value,
            obj.SafeHeight.Value,
            obj.StartDepth.Value,
            obj.StepDown.Value,
            0.0,
            obj.FinalDepth.Value,
        )

        # ######  MAIN COMMANDS FOR OPERATION ######

        cmds = self._makeOperation(obj)
        if cmds:
            CMDS.extend(cmds)

        # Save gcode produced
        CMDS.append(
            Path.Command("G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid})
        )
        self.commandlist.extend(CMDS)

        # ######  CLOSING COMMANDS FOR OPERATION ######

        # Hide the temporary objects
        if self.showDebugObjects:
            if FreeCAD.GuiUp:
                FreeCADGui.ActiveDocument.getObject(self.tmpGrp.Name).Visibility = False
            self.tmpGrp.purgeTouched()

        return True

    # Control methods for operation
    def _makeOperation(self, obj):
        """This method controls the overall slot creation process."""
        pnts = False
        featureCount = 0

        if not hasattr(obj, "Base"):
            msg = translate("CAM_Slot", "No Base Geometry object in the operation.")
            FreeCAD.Console.PrintError(msg + "\n")
            return False

        if not obj.Base:
            # Use custom inputs here
            p1 = obj.CustomPoint1
            p2 = obj.CustomPoint2
            if p1 == p2:
                msg = translate("CAM_Slot", "Custom points are identical.")
                FreeCAD.Console.PrintError(msg + "\n")
                return False
            elif p1.z == p2.z:
                pnts = (p1, p2)
                featureCount = 2
            else:
                msg = translate("CAM_Slot", "Custom points not at same Z height.")
                FreeCAD.Console.PrintError(msg + "\n")
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
        self._addDebugObject(
            Part.makeLine(self.arcCenter, self.arcMidPnt), "CentToMidPnt"
        )

        # Path.Log.debug('Pre-offset points are:\np1 = {}\np2 = {}'.format(p1, p2))
        if obj.ExtendRadius.Value != 0:
            # verify offset does not force radius < 0
            newRadius = self.arcRadius + obj.ExtendRadius.Value
            Path.Log.debug(
                "arc radius: {};  offset radius: {}".format(self.arcRadius, newRadius)
            )
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
                msg = translate(
                    "CAM_Slot", "No path extensions available for full circles."
                )
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
            cmds.append(
                Path.Command("G0", {"X": st_pt.x, "Y": st_pt.y, "F": self.horizRapid})
            )
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
                        Path.Command(
                            "G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid}
                        )
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
        CMDS.append(
            Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid})
        )

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
                pnts = self._makePerpendicular(
                    p1, p2, initPerpDist
                )  # 10.0 offset below
        else:
            # Modify path points if user selected two parallel edges
            if (
                featureCnt == 2
                and self.shapeType1 == "Edge"
                and self.shapeType2 == "Edge"
            ):
                if self.featureDetails[0] == "arc" and self.featureDetails[1] == "arc":
                    perpZero = False
                elif self._isParallel(self.dYdX1, self.dYdX2):
                    Path.Log.debug("_finishLine() StE, featureCnt == 2 // edges")
                    (p1, p2) = pnts
                    edg1_len = self.shape1.Length
                    edg2_len = self.shape2.Length
                    set_length = max(edg1_len, edg2_len)
                    pnts = self._makePerpendicular(
                        p1, p2, 10.0 + set_length
                    )  # 10.0 offset below
                    if edg1_len != edg2_len:
                        msg = obj.Label + " "
                        msg += translate(
                            "CAM_Slot", "Verify slot path start and end points."
                        )
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
            cmds.append(
                Path.Command("G0", {"X": p1.x, "Y": p1.y, "F": self.horizRapid})
            )
            cmds.append(Path.Command("G1", {"Z": depth, "F": self.vertFeed}))
            cmds.append(Path.Command("G1", {"X": p2.x, "Y": p2.y, "F": self.horizFeed}))
            return cmds

        # CMDS.append(Path.Command('N (Tool type: {})'.format(toolType), {}))
        if obj.LayerMode == "Single-pass":
            CMDS.extend(linePass(p1, p2, obj.FinalDepth.Value))
            CMDS.append(
                Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid})
            )
        else:
            if obj.CutPattern == "Line":
                for dep in self.depthParams:
                    CMDS.extend(linePass(p1, p2, dep))
                    CMDS.append(
                        Path.Command(
                            "G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid}
                        )
                    )
            elif obj.CutPattern == "ZigZag":
                CMDS.append(
                    Path.Command("G0", {"X": p1.x, "Y": p1.y, "F": self.horizRapid})
                )
                i = 0
                for dep in self.depthParams:
                    if i % 2.0 == 0:  # even
                        CMDS.append(Path.Command("G1", {"Z": dep, "F": self.vertFeed}))
                        CMDS.append(
                            Path.Command(
                                "G1", {"X": p2.x, "Y": p2.y, "F": self.horizFeed}
                            )
                        )
                    else:  # odd
                        CMDS.append(Path.Command("G1", {"Z": dep, "F": self.vertFeed}))
                        CMDS.append(
                            Path.Command(
                                "G1", {"X": p1.x, "Y": p1.y, "F": self.horizFeed}
                            )
                        )
                    i += 1
            CMDS.append(
                Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid})
            )

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
        lineTypes = ["Part::GeomLine"]

        def getRadians(self, E):
            vect = self._dXdYdZ(E)
            norm = self._normalizeVector(vect)
            rads = self._getVectorAngle(norm)
            deg = math.degrees(rads)
            if deg >= 180.0:
                deg -= 180.0
            return deg

        # Reject triangular faces
        if len(shape.Edges) < 4:
            msg = translate(
                "CAM_Slot", "A single selected face must have four edges minimum."
            )
            FreeCAD.Console.PrintError(msg + "\n")
            return False

        # Create tuples as (edge index, length, angle)
        eTups = list()
        for i in range(0, 4):
            eTups.append((i, shape.Edges[i].Length, getRadians(self, shape.Edges[i])))

        # Sort tuples by edge angle
        eTups.sort(key=lambda tup: tup[2])

        # Identify parallel edges
        parallel_edge_pairs = list()
        parallel_edge_flags = list()
        flag = 1
        eCnt = len(shape.Edges)
        lstE = eCnt - 1
        for i in range(0, eCnt):  # populate empty parallel edge flag list
            parallel_edge_flags.append(0)
        for i in range(0, eCnt):  # Cycle through edges to identify parallel pairs
            if i < lstE:
                ni = i + 1
                A = eTups[i]
                B = eTups[ni]
                if abs(A[2] - B[2]) < 0.00000001:  # test slopes(yaw angles)
                    debug = False
                    eA = shape.Edges[A[0]]
                    eB = shape.Edges[B[0]]
                    if eA.Curve.TypeId not in lineTypes:
                        debug = eA.Curve.TypeId
                    if not debug:
                        if eB.Curve.TypeId not in lineTypes:
                            debug = eB.Curve.TypeId
                        else:
                            parallel_edge_pairs.append((eA, eB))
                            # set parallel flags for this pair of edges
                            parallel_edge_flags[A[0]] = flag
                            parallel_edge_flags[B[0]] = flag
                            flag += 1
                    if debug:
                        msg = "Erroneous Curve.TypeId: {}".format(debug)
                        Path.Log.debug(msg)

        pairCnt = len(parallel_edge_pairs)
        if pairCnt > 1:
            parallel_edge_pairs.sort(key=lambda tup: tup[0].Length, reverse=True)

        if self.isDebug:
            Path.Log.debug(" -pairCnt: {}".format(pairCnt))
            for (a, b) in parallel_edge_pairs:
                Path.Log.debug(
                    " -pair: {}, {}".format(round(a.Length, 4), round(b.Length, 4))
                )
            Path.Log.debug(" -parallel_edge_flags: {}".format(parallel_edge_flags))

        if pairCnt == 0:
            msg = translate("CAM_Slot", "No parallel edges identified.")
            FreeCAD.Console.PrintError(msg + "\n")
            return False
        elif pairCnt == 1:
            # One pair of parallel edges identified
            if eCnt == 4:
                flag_set = list()
                for i in range(0, 4):
                    e = parallel_edge_flags[i]
                    if e == 0:
                        flag_set.append(shape.Edges[i])
                if len(flag_set) == 2:
                    same = (flag_set[0], flag_set[1])
                else:
                    same = parallel_edge_pairs[0]
            else:
                same = parallel_edge_pairs[0]
        else:
            if obj.Reference1 == "Long Edge":
                same = parallel_edge_pairs[1]
            elif obj.Reference1 == "Short Edge":
                same = parallel_edge_pairs[0]
            else:
                msg = "Reference1 "
                msg += translate("CAM_Slot", "value error.")
                FreeCAD.Console.PrintError(msg + "\n")
                return False

        (p1, p2) = self._getOppMidPoints(same)
        return (p1, p2)

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
        tolrnc = 0.0000001
        lineTypes = ["Part::GeomLine"]
        curveTypes = ["Part::GeomCircle"]

        def oversizedTool(holeDiam):
            # Test if tool larger than opening
            if self.tool.Diameter > holeDiam:
                msg = translate("CAM_Slot", "Current tool larger than arc diameter.")
                FreeCAD.Console.PrintError(msg + "\n")
                return True
            return False

        def isHorizontal(z1, z2, z3):
            # Check that all Z values are equal (isRoughly same)
            if abs(z1 - z2) > tolrnc or abs(z1 - z3) > tolrnc:
                # abs(z2 - z3) > tolrnc):  3rd test redundant.
                return False
            return True

        def circumCircleFrom3Points(P1, P2, P3):
            # Source code for this function copied from (with modifications):
            # https://wiki.freecad.org/Macro_Draft_Circle_3_Points_3D
            vP2P1 = P2 - P1
            vP3P2 = P3 - P2
            vP1P3 = P1 - P3

            L = vP2P1.cross(vP3P2).Length
            # Circle radius (not used)
            # r = vP1P2.Length * vP2P3.Length * vP3P1.Length / 2 / l
            if round(L, 8) == 0.0:
                Path.Log.error("The three points are colinear, arc is a straight.")
                return False

            # Sphere center.
            twolsqr = 2 * L * L
            a = -vP3P2.dot(vP3P2) * vP2P1.dot(vP1P3) / twolsqr
            b = -vP1P3.dot(vP1P3) * vP3P2.dot(vP2P1) / twolsqr
            c = -vP2P1.dot(vP2P1) * vP1P3.dot(vP3P2) / twolsqr
            return P1 * a + P2 * b + P3 * c

        V1 = edge.Vertexes[0]
        p1 = FreeCAD.Vector(V1.X, V1.Y, 0.0)
        if len(edge.Vertexes) == 1:  # circle has one virtex
            p2 = FreeCAD.Vector(p1)
        else:
            V2 = edge.Vertexes[1]
            p2 = FreeCAD.Vector(V2.X, V2.Y, 0.0)

        # Process edge based on curve type
        if edge.Curve.TypeId in lineTypes:
            return (p1, p2)

        elif edge.Curve.TypeId in curveTypes:
            if len(edge.Vertexes) == 1:
                # Circle edge
                Path.Log.debug("Arc with single vertex.")
                if oversizedTool(edge.BoundBox.XLength):
                    return False

                self.isArc = 1
                tp1 = edge.valueAt(edge.getParameterByLength(edge.Length * 0.33))
                tp2 = edge.valueAt(edge.getParameterByLength(edge.Length * 0.66))
                if not isHorizontal(V1.Z, tp1.z, tp2.z):
                    return False

                center = edge.BoundBox.Center
                self.arcCenter = FreeCAD.Vector(center.x, center.y, 0.0)
                midPnt = edge.valueAt(edge.getParameterByLength(edge.Length / 2.0))
                self.arcMidPnt = FreeCAD.Vector(midPnt.x, midPnt.y, 0.0)
                self.arcRadius = edge.BoundBox.XLength / 2.0
            else:
                # Arc edge
                Path.Log.debug("Arc with multiple vertices.")
                self.isArc = 2
                midPnt = edge.valueAt(edge.getParameterByLength(edge.Length / 2.0))
                if not isHorizontal(V1.Z, V2.Z, midPnt.z):
                    return False

                midPnt.z = 0.0
                circleCenter = circumCircleFrom3Points(p1, p2, midPnt)
                if not circleCenter:
                    return False
                self.arcMidPnt = midPnt
                self.arcCenter = circleCenter
                self.arcRadius = p1.sub(circleCenter).Length

                if oversizedTool(self.arcRadius * 2.0):
                    return False

            return (p1, p2)
        else:
            msg = translate(
                "CAM_Slot",
                "Failed, slot from edge only accepts lines, arcs and circles.",
            )
            FreeCAD.Console.PrintError(msg + "\n")

            return False  # not line , not circle

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
        """_normalizeVector(v)...
        Returns a copy of the vector received with values rounded to 10 decimal places."""
        posTol = 0.0000000001  # arbitrary, use job Geometry Tolerance ???
        negTol = -1 * posTol
        V = FreeCAD.Vector(v.x, v.y, v.z)
        V.normalize()
        x = V.x
        y = V.y
        z = V.z

        if V.x != 0 and abs(V.x) < posTol:
            x = 0.0
        if V.x != 1 and 1.0 - V.x < posTol:
            x = 1.0
        if V.x != -1 and -1.0 - V.x > negTol:
            x = -1.0

        if V.y != 0 and abs(V.y) < posTol:
            y = 0.0
        if V.y != 1 and 1.0 - V.y < posTol:
            y = 1.0
        if V.y != -1 and -1.0 - V.y > negTol:
            y = -1.0

        if V.z != 0 and abs(V.z) < posTol:
            z = 0.0
        if V.z != 1 and 1.0 - V.z < posTol:
            z = 1.0
        if V.z != -1 and -1.0 - V.z > negTol:
            z = -1.0

        return FreeCAD.Vector(x, y, z)

    def _getLowestPoint(self, shape_1):
        """_getLowestPoint(shape)... Returns lowest vertex of shape as vector."""
        # find lowest vertex
        vMin = shape_1.Vertexes[0]
        zmin = vMin.Z
        same = [vMin]
        for V in shape_1.Vertexes:
            if V.Z < zmin:
                zmin = V.Z
                # vMin = V
            elif V.Z == zmin:
                same.append(V)
        if len(same) > 1:
            X = [E.X for E in same]
            Y = [E.Y for E in same]
            avgX = sum(X) / len(X)
            avgY = sum(Y) / len(Y)
            return FreeCAD.Vector(avgX, avgY, zmin)
        else:
            return FreeCAD.Vector(V.X, V.Y, V.Z)

    def _getHighestPoint(self, shape_1):
        """_getHighestPoint(shape)... Returns highest vertex of shape as vector."""
        # find highest vertex
        vMax = shape_1.Vertexes[0]
        zmax = vMax.Z
        same = [vMax]
        for V in shape_1.Vertexes:
            if V.Z > zmax:
                zmax = V.Z
                # vMax = V
            elif V.Z == zmax:
                same.append(V)
        if len(same) > 1:
            X = [E.X for E in same]
            Y = [E.Y for E in same]
            avgX = sum(X) / len(X)
            avgY = sum(Y) / len(Y)
            return FreeCAD.Vector(avgX, avgY, zmax)
        else:
            return FreeCAD.Vector(V.X, V.Y, V.Z)

    def _processFeature(self, obj, shape, sub, pNum):
        """_processFeature(obj, shape, sub, pNum)...
        This function analyzes a shape and returns a three item tuple containing:
            working point,
            shape orientation/slope,
            shape category as face, edge, or vert."""
        p = None
        dYdX = None
        cat = sub[:4]
        Path.Log.debug("sub-feature is {}".format(cat))
        Ref = getattr(obj, "Reference" + str(pNum))
        if cat == "Face":
            BE = self._getBottomEdge(shape)
            if BE:
                self.bottomEdges.append(BE)
            # calculate slope of face
            V0 = shape.Vertexes[0]
            v1 = shape.CenterOfMass
            temp = FreeCAD.Vector(v1.x - V0.X, v1.y - V0.Y, 0.0)
            dYdX = self._normalizeVector(temp)

            # Determine normal vector for face
            norm = shape.normalAt(0.0, 0.0)
            # FreeCAD.Console.PrintMessage('{} normal {}.\n'.format(sub, norm))
            if norm.z != 0:
                msg = translate(
                    "CAM_Slot", "The selected face is not oriented vertically:"
                )
                FreeCAD.Console.PrintError(msg + " {}.\n".format(sub))
                return False

            if Ref == "Center of Mass":
                comS = shape.CenterOfMass
                p = FreeCAD.Vector(comS.x, comS.y, 0.0)
            elif Ref == "Center of BoundBox":
                comS = shape.BoundBox.Center
                p = FreeCAD.Vector(comS.x, comS.y, 0.0)
            elif Ref == "Lowest Point":
                p = self._getLowestPoint(shape)
            elif Ref == "Highest Point":
                p = self._getHighestPoint(shape)

        elif cat == "Edge":
            featDetIdx = pNum - 1
            if shape.Curve.TypeId == "Part::GeomCircle":
                self.featureDetails[featDetIdx] = "arc"
            # calculate slope between end vertexes
            v0 = shape.Edges[0].Vertexes[0]
            v1 = shape.Edges[0].Vertexes[1]
            temp = FreeCAD.Vector(v1.X - v0.X, v1.Y - v0.Y, 0.0)
            dYdX = self._normalizeVector(temp)

            if Ref == "Center of Mass":
                comS = shape.CenterOfMass
                p = FreeCAD.Vector(comS.x, comS.y, 0.0)
            elif Ref == "Center of BoundBox":
                comS = shape.BoundBox.Center
                p = FreeCAD.Vector(comS.x, comS.y, 0.0)
            elif Ref == "Lowest Point":
                p = self._findLowestPointOnEdge(shape)
            elif Ref == "Highest Point":
                p = self._findHighestPointOnEdge(shape)

        elif cat == "Vert":
            V = shape.Vertexes[0]
            p = FreeCAD.Vector(V.X, V.Y, 0.0)

        if p:
            return (p, dYdX, cat)

        return False

    def _extendArcSlot(self, p1, p2, cent, begExt, endExt):
        """_extendArcSlot(p1, p2, cent, begExt, endExt)...
                This function extends an arc defined by two end points, p1 and p2, and the center.
        The arc is extended along the circumference with begExt and endExt values.
        The function returns the new end points as tuple (n1, n2) to replace p1 and p2."""
        cancel = True
        if not begExt and not endExt:
            return (p1, p2)

        n1 = p1
        n2 = p2

        # Create a chord of the right length, on XY plane, starting on x axis
        def makeChord(rads):
            x = self.newRadius * math.cos(rads)
            y = self.newRadius * math.sin(rads)
            a = FreeCAD.Vector(self.newRadius, 0.0, 0.0)
            b = FreeCAD.Vector(x, y, 0.0)
            return Part.makeLine(a, b)

        # Convert extension to radians; make a generic chord ( line ) on XY plane from the x axis
        # rotate and shift into place so it has same vertices as the required arc extension
        # adjust rotation angle to provide +ve or -ve extension as needed
        origin = FreeCAD.Vector(0.0, 0.0, 0.0)
        if begExt:
            ExtRadians = abs(begExt / self.newRadius)
            chord = makeChord(ExtRadians)

            beginRadians = self._getVectorAngle(p1.sub(self.arcCenter))
            if begExt < 0:
                beginRadians += 0  # negative Ext shortens slot so chord endpoint is slot start point
            else:
                beginRadians -= (
                    2 * ExtRadians
                )  # positive Ext lengthens slot so decrease start point angle

            # Path.Log.debug('begExt angles are: {},  {}'.format(beginRadians, math.degrees(beginRadians)))

            chord.rotate(origin, FreeCAD.Vector(0, 0, 1), math.degrees(beginRadians))
            chord.translate(self.arcCenter)
            self._addDebugObject(chord, "ExtendStart")

            v1 = chord.Vertexes[1]
            n1 = FreeCAD.Vector(v1.X, v1.Y, 0.0)

        if endExt:
            ExtRadians = abs(endExt / self.newRadius)
            chord = makeChord(ExtRadians)

            endRadians = self._getVectorAngle(p2.sub(self.arcCenter))
            if endExt > 0:
                endRadians += 0  # positive Ext lengthens slot so chord endpoint is good
            else:
                endRadians -= (
                    2 * ExtRadians
                )  # negative Ext shortens slot so decrease end point angle

            # Path.Log.debug('endExt angles are: {},  {}'.format(endRadians, math.degrees(endRadians)))

            chord.rotate(origin, FreeCAD.Vector(0, 0, 1), math.degrees(endRadians))
            chord.translate(self.arcCenter)
            self._addDebugObject(chord, "ExtendEnd")

            v1 = chord.Vertexes[1]
            n2 = FreeCAD.Vector(v1.X, v1.Y, 0.0)

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
        # if dYdX1.add(dYdX2).Length == 0:
        #    return True
        # if ((dYdX1.x + dYdX2.x) / 2.0 == dYdX1.x and
        #    (dYdX1.y + dYdX2.y) / 2.0 == dYdX1.y):
        #    return True
        # return False
        return dYdX1.cross(dYdX2) == FreeCAD.Vector(0, 0, 0)

    def _makePerpendicular(self, p1, p2, length):
        """_makePerpendicular(p1, p2, length)...
        Using a line defined by p1 and p2, returns a perpendicular vector centered
        at the midpoint of the line, with length value."""
        line = Part.makeLine(p1, p2)
        midPnt = line.CenterOfMass

        halfDist = length / 2.0
        if self.dYdX1:
            half = FreeCAD.Vector(self.dYdX1.x, self.dYdX1.y, 0.0).multiply(halfDist)
            n1 = midPnt.add(half)
            n2 = midPnt.sub(half)
            return (n1, n2)
        elif self.dYdX2:
            half = FreeCAD.Vector(self.dYdX2.x, self.dYdX2.y, 0.0).multiply(halfDist)
            n1 = midPnt.add(half)
            n2 = midPnt.sub(half)
            return (n1, n2)
        else:
            toEnd = p2.sub(p1)
            perp = FreeCAD.Vector(-1 * toEnd.y, toEnd.x, 0.0)
            perp.normalize()
            perp.multiply(halfDist)
            n1 = midPnt.add(perp)
            n2 = midPnt.sub(perp)
            return (n1, n2)

    def _findLowestPointOnEdge(self, E):
        tol = 0.0000001
        zMin = E.BoundBox.ZMin
        # Test first vertex
        v = E.Vertexes[0]
        if abs(v.Z - zMin) < tol:
            return FreeCAD.Vector(v.X, v.Y, v.Z)
        # Test second vertex
        v = E.Vertexes[1]
        if abs(v.Z - zMin) < tol:
            return FreeCAD.Vector(v.X, v.Y, v.Z)
        # Test middle point of edge
        eMidLen = E.Length / 2.0
        eMidPnt = E.valueAt(E.getParameterByLength(eMidLen))
        if abs(eMidPnt.z - zMin) < tol:
            return eMidPnt
        if E.BoundBox.ZLength < 0.000000001:  # roughly horizontal edge
            return eMidPnt
        return self._findLowestEdgePoint(E)

    def _findLowestEdgePoint(self, E):
        zMin = E.BoundBox.ZMin
        eLen = E.Length
        L0 = 0.0
        L1 = eLen
        p0 = None
        p1 = None
        cnt = 0
        while L1 - L0 > 0.00001 and cnt < 2000:
            adj = (L1 - L0) * 0.1
            # Get points at L0 and L1 along edge
            p0 = E.valueAt(E.getParameterByLength(L0))
            p1 = E.valueAt(E.getParameterByLength(L1))
            # Adjust points based on proximity to target depth
            diff0 = p0.z - zMin
            diff1 = p1.z - zMin
            if diff0 < diff1:
                L1 -= adj
            elif diff0 > diff1:
                L0 += adj
            else:
                L0 += adj
                L1 -= adj
            cnt += 1
        midLen = (L0 + L1) / 2.0
        return E.valueAt(E.getParameterByLength(midLen))

    def _findHighestPointOnEdge(self, E):
        tol = 0.0000001
        zMax = E.BoundBox.ZMax
        # Test first vertex
        v = E.Vertexes[0]
        if abs(zMax - v.Z) < tol:
            return FreeCAD.Vector(v.X, v.Y, v.Z)
        # Test second vertex
        v = E.Vertexes[1]
        if abs(zMax - v.Z) < tol:
            return FreeCAD.Vector(v.X, v.Y, v.Z)
        # Test middle point of edge
        eMidLen = E.Length / 2.0
        eMidPnt = E.valueAt(E.getParameterByLength(eMidLen))
        if abs(zMax - eMidPnt.z) < tol:
            return eMidPnt
        if E.BoundBox.ZLength < 0.000000001:  # roughly horizontal edge
            return eMidPnt
        return self._findHighestEdgePoint(E)

    def _findHighestEdgePoint(self, E):
        zMax = E.BoundBox.ZMax
        eLen = E.Length
        L0 = 0
        L1 = eLen
        p0 = None
        p1 = None
        cnt = 0
        while L1 - L0 > 0.00001 and cnt < 2000:
            adj = (L1 - L0) * 0.1
            # Get points at L0 and L1 along edge
            p0 = E.valueAt(E.getParameterByLength(L0))
            p1 = E.valueAt(E.getParameterByLength(L1))
            # Adjust points based on proximity to target depth
            diff0 = zMax - p0.z
            diff1 = zMax - p1.z
            if diff0 < diff1:
                L1 -= adj
            elif diff0 > diff1:
                L0 += adj
            else:
                L0 += adj
                L1 -= adj
            cnt += 1
        midLen = (L0 + L1) / 2.0
        return E.valueAt(E.getParameterByLength(midLen))

    def _getVectorAngle(self, v):
        # Assumes Z value of vector is zero
        halfPi = math.pi / 2

        if v.y == 1 and v.x == 0:
            return halfPi
        if v.y == -1 and v.x == 0:
            return math.pi + halfPi
        if v.y == 0 and v.x == 1:
            return 0.0
        if v.y == 0 and v.x == -1:
            return math.pi

        x = abs(v.x)
        y = abs(v.y)
        rads = math.atan(y / x)
        if v.x > 0:
            if v.y > 0:
                return rads
            else:
                return (2 * math.pi) - rads
        if v.x < 0:
            if v.y > 0:
                return math.pi - rads
            else:
                return math.pi + rads

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
        wires = list()

        bottomEdge = self._getBottomEdge(shape)
        if bottomEdge:
            return ("Edge", bottomEdge)

        # Extract cross-section of face
        extFwd = (shape.BoundBox.ZLength * 2.2) + 10
        extShp = shape.extrude(FreeCAD.Vector(0.0, 0.0, extFwd))
        sliceZ = shape.BoundBox.ZMin + (extFwd / 2.0)
        slcs = extShp.slice(FreeCAD.Vector(0, 0, 1), sliceZ)
        for i in slcs:
            wires.append(i)
        if len(wires) > 0:
            if wires[0].isClosed():
                face = Part.Face(wires[0])
                if face.Area > 0:
                    face.translate(
                        FreeCAD.Vector(
                            0.0, 0.0, shape.BoundBox.ZMin - face.BoundBox.ZMin
                        )
                    )
                    return ("Face", face)
            return ("Wire", wires[0])
        return False

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
        """Make simple circle with diameter of tool, at start point.
        Extrude it latterally along path.
        Extrude it vertically.
        Check for collision with model."""
        # Make path travel of tool as 3D solid.
        rad = self.tool.Diameter / 2.0

        def getPerp(p1, p2, dist):
            toEnd = p2.sub(p1)
            perp = FreeCAD.Vector(-1 * toEnd.y, toEnd.x, 0.0)
            if perp.x == 0 and perp.y == 0:
                return perp
            perp.normalize()
            perp.multiply(dist)
            return perp

        # Make first cylinder
        ce1 = Part.Wire(Part.makeCircle(rad, p1).Edges)
        C1 = Part.Face(ce1)
        zTrans = obj.FinalDepth.Value - C1.BoundBox.ZMin
        C1.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
        extFwd = obj.StartDepth.Value - obj.FinalDepth.Value
        extVect = FreeCAD.Vector(0.0, 0.0, extFwd)
        startShp = C1.extrude(extVect)

        if p2.sub(p1).Length > 0:
            # Make second cylinder
            ce2 = Part.Wire(Part.makeCircle(rad, p2).Edges)
            C2 = Part.Face(ce2)
            zTrans = obj.FinalDepth.Value - C2.BoundBox.ZMin
            C2.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
            endShp = C2.extrude(extVect)

            # Make extruded rectangle to connect cylinders
            perp = getPerp(p1, p2, rad)
            v1 = p1.add(perp)
            v2 = p1.sub(perp)
            v3 = p2.sub(perp)
            v4 = p2.add(perp)
            e1 = Part.makeLine(v1, v2)
            e2 = Part.makeLine(v2, v3)
            e3 = Part.makeLine(v3, v4)
            e4 = Part.makeLine(v4, v1)
            edges = Part.__sortEdges__([e1, e2, e3, e4])
            rectFace = Part.Face(Part.Wire(edges))
            zTrans = obj.FinalDepth.Value - rectFace.BoundBox.ZMin
            rectFace.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
            boxShp = rectFace.extrude(extVect)

            # Fuse two cylinders and box together
            part1 = startShp.fuse(boxShp)
            pathTravel = part1.fuse(endShp)
        else:
            pathTravel = startShp

        self._addDebugObject(pathTravel, "PathTravel")

        # Check for collision with model
        try:
            cmn = self.base.Shape.common(pathTravel)
            if cmn.Volume > 0.000001:
                return True
        except Exception:
            Path.Log.debug("Failed to complete path collision check.")

        return False

    def _arcCollisionCheck(self, obj, p1, p2, arcCenter, arcRadius):
        """Make simple circle with diameter of tool, at start and end points.
        Make arch face between circles. Fuse and extrude it vertically.
        Check for collision with model."""
        # Make path travel of tool as 3D solid.
        if hasattr(self.tool.Diameter, "Value"):
            rad = self.tool.Diameter.Value / 2.0
        else:
            rad = self.tool.Diameter / 2.0
        extFwd = obj.StartDepth.Value - obj.FinalDepth.Value
        extVect = FreeCAD.Vector(0.0, 0.0, extFwd)

        if self.isArc == 1:
            # full circular slot
            # make outer circle
            oCircle = Part.makeCircle(arcRadius + rad, arcCenter)
            oWire = Part.Wire(oCircle.Edges[0])
            outer = Part.Face(oWire)
            # make inner circle
            iRadius = arcRadius - rad
            if iRadius > 0:
                iCircle = Part.makeCircle(iRadius, arcCenter)
                iWire = Part.Wire(iCircle.Edges[0])
                inner = Part.Face(iWire)
                # Cut outer with inner
                path = outer.cut(inner)
            else:
                path = outer
            zTrans = obj.FinalDepth.Value - path.BoundBox.ZMin
            path.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
            pathTravel = path.extrude(extVect)
        else:
            # arc slot
            # Make first cylinder
            ce1 = Part.Wire(Part.makeCircle(rad, p1).Edges)
            C1 = Part.Face(ce1)
            zTrans = obj.FinalDepth.Value - C1.BoundBox.ZMin
            C1.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
            startShp = C1.extrude(extVect)
            # self._addDebugObject(startShp, 'StartCyl')

            # Make second cylinder
            ce2 = Part.Wire(Part.makeCircle(rad, p2).Edges)
            C2 = Part.Face(ce2)
            zTrans = obj.FinalDepth.Value - C2.BoundBox.ZMin
            C2.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
            endShp = C2.extrude(extVect)
            # self._addDebugObject(endShp, 'EndCyl')

            # Make wire with inside and outside arcs, and lines on ends.
            # Convert wire to face, then extrude

            # verify offset does not force radius < 0
            newRadius = arcRadius - rad
            # Path.Log.debug('arcRadius, newRadius: {}, {}'.format(arcRadius, newRadius))
            if newRadius <= 0:
                msg = translate(
                    "CAM_Slot", "Current offset value produces negative radius."
                )
                FreeCAD.Console.PrintError(msg + "\n")
                return False
            else:
                (pA, pB) = self._makeOffsetArc(p1, p2, arcCenter, newRadius)
                arc_inside = Arcs.arcFrom2Pts(pA, pB, arcCenter)

            # Arc 2 - outside
            # verify offset does not force radius < 0
            newRadius = arcRadius + rad
            # Path.Log.debug('arcRadius, newRadius: {}, {}'.format(arcRadius, newRadius))
            if newRadius <= 0:
                msg = translate(
                    "CAM_Slot", "Current offset value produces negative radius."
                )
                FreeCAD.Console.PrintError(msg + "\n")
                return False
            else:
                (pC, pD) = self._makeOffsetArc(p1, p2, arcCenter, newRadius)
                arc_outside = Arcs.arcFrom2Pts(pC, pD, arcCenter)

            # Make end lines to connect arcs
            vA = arc_inside.Vertexes[0]
            vB = arc_inside.Vertexes[1]
            vC = arc_outside.Vertexes[1]
            vD = arc_outside.Vertexes[0]
            pa = FreeCAD.Vector(vA.X, vA.Y, 0.0)
            pb = FreeCAD.Vector(vB.X, vB.Y, 0.0)
            pc = FreeCAD.Vector(vC.X, vC.Y, 0.0)
            pd = FreeCAD.Vector(vD.X, vD.Y, 0.0)

            # Make closed arch face and extrude
            e1 = Part.makeLine(pb, pc)
            e2 = Part.makeLine(pd, pa)
            edges = Part.__sortEdges__([arc_inside, e1, arc_outside, e2])
            rectFace = Part.Face(Part.Wire(edges))
            zTrans = obj.FinalDepth.Value - rectFace.BoundBox.ZMin
            rectFace.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
            boxShp = rectFace.extrude(extVect)
            # self._addDebugObject(boxShp, 'ArcBox')

            # Fuse two cylinders and box together
            part1 = startShp.fuse(boxShp)
            pathTravel = part1.fuse(endShp)

        self._addDebugObject(pathTravel, "PathTravel")

        # Check for collision with model
        try:
            cmn = self.base.Shape.common(pathTravel)
            if cmn.Volume > 0.000001:
                # print("volume=", cmn.Volume)
                return True
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
