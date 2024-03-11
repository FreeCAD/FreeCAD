# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2018 Kresimir Tusek <kresimir.tusek@gmail.com>          *
# *   Copyright (c) 2019-2021 Schildkroet                                   *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************

import Path
import Path.Op.Base as PathOp
import PathScripts.PathUtils as PathUtils
import FreeCAD
import time
import json
import math
import area
from PySide.QtCore import QT_TRANSLATE_NOOP

if FreeCAD.GuiUp:
    from pivy import coin
    import FreeCADGui

__doc__ = "Class and implementation of the Adaptive CAM operation."

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")
# TechDraw = LazyLoader('TechDraw', globals(), 'TechDraw')
FeatureExtensions = LazyLoader(
    "Path.Op.FeatureExtension", globals(), "Path.Op.FeatureExtension"
)
DraftGeomUtils = LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


def convertTo2d(pathArray):
    output = []
    for path in pathArray:
        pth2 = []
        for edge in path:
            for pt in edge:
                pth2.append([pt[0], pt[1]])
        output.append(pth2)
    return output


sceneGraph = None
scenePathNodes = []  # for scene cleanup aftewards
topZ = 10


def sceneDrawPath(path, color=(0, 0, 1)):
    coPoint = coin.SoCoordinate3()

    pts = []
    for pt in path:
        pts.append([pt[0], pt[1], topZ])

    coPoint.point.setValues(0, len(pts), pts)
    ma = coin.SoBaseColor()
    ma.rgb = color
    li = coin.SoLineSet()
    li.numVertices.setValue(len(pts))
    pathNode = coin.SoSeparator()
    pathNode.addChild(coPoint)
    pathNode.addChild(ma)
    pathNode.addChild(li)
    sceneGraph.addChild(pathNode)
    scenePathNodes.append(pathNode)  # for scene cleanup afterwards


def sceneClean():
    for n in scenePathNodes:
        sceneGraph.removeChild(n)

    del scenePathNodes[:]


def discretize(edge, flipDirection=False):
    pts = edge.discretize(Deflection=0.002)
    if flipDirection:
        pts.reverse()

    return pts


def CalcHelixConePoint(height, cur_z, radius, angle):
    x = ((height - cur_z) / height) * radius * math.cos(math.radians(angle) * cur_z)
    y = ((height - cur_z) / height) * radius * math.sin(math.radians(angle) * cur_z)
    z = cur_z

    return {"X": x, "Y": y, "Z": z}


def GenerateGCode(op, obj, adaptiveResults, helixDiameter):
    if len(adaptiveResults) == 0 or len(adaptiveResults[0]["AdaptivePaths"]) == 0:
        return

    # minLiftDistance = op.tool.Diameter
    helixRadius = 0
    for region in adaptiveResults:
        p1 = region["HelixCenterPoint"]
        p2 = region["StartPoint"]
        r = math.sqrt(
            (p1[0] - p2[0]) * (p1[0] - p2[0]) + (p1[1] - p2[1]) * (p1[1] - p2[1])
        )
        if r > helixRadius:
            helixRadius = r

    stepDown = obj.StepDown.Value
    passStartDepth = obj.StartDepth.Value

    if stepDown < 0.1:
        stepDown = 0.1

    length = 2 * math.pi * helixRadius

    if float(obj.HelixAngle) < 1:
        obj.HelixAngle = 1
    if float(obj.HelixAngle) > 89:
        obj.HelixAngle = 89

    if float(obj.HelixConeAngle) < 0:
        obj.HelixConeAngle = 0

    helixAngleRad = math.pi * float(obj.HelixAngle) / 180.0
    depthPerOneCircle = length * math.tan(helixAngleRad)
    # print("Helix circle depth: {}".format(depthPerOneCircle))

    stepUp = obj.LiftDistance.Value
    if stepUp < 0:
        stepUp = 0

    finish_step = obj.FinishDepth.Value if hasattr(obj, "FinishDepth") else 0.0
    if finish_step > stepDown:
        finish_step = stepDown

    depth_params = PathUtils.depth_params(
        clearance_height=obj.ClearanceHeight.Value,
        safe_height=obj.SafeHeight.Value,
        start_depth=obj.StartDepth.Value,
        step_down=stepDown,
        z_finish_step=finish_step,
        final_depth=obj.FinalDepth.Value,
        user_depths=None,
    )

    # ml: this is dangerous because it'll hide all unused variables hence forward
    #     however, I don't know what lx and ly signify so I'll leave them for now
    # lx = adaptiveResults[0]["HelixCenterPoint"][0]
    # ly = adaptiveResults[0]["HelixCenterPoint"][1]
    lz = passStartDepth
    step = 0

    for passEndDepth in depth_params.data:
        step = step + 1

        for region in adaptiveResults:
            startAngle = math.atan2(
                region["StartPoint"][1] - region["HelixCenterPoint"][1],
                region["StartPoint"][0] - region["HelixCenterPoint"][0],
            )

            # lx = region["HelixCenterPoint"][0]
            # ly = region["HelixCenterPoint"][1]

            passDepth = passStartDepth - passEndDepth

            p1 = region["HelixCenterPoint"]
            p2 = region["StartPoint"]
            helixRadius = math.sqrt(
                (p1[0] - p2[0]) * (p1[0] - p2[0]) + (p1[1] - p2[1]) * (p1[1] - p2[1])
            )

            # Helix ramp
            if helixRadius > 0.01:
                r = helixRadius - 0.01

                maxfi = passDepth / depthPerOneCircle * 2 * math.pi
                fi = 0
                offsetFi = -maxfi + startAngle - math.pi / 16

                helixStart = [
                    region["HelixCenterPoint"][0] + r * math.cos(offsetFi),
                    region["HelixCenterPoint"][1] + r * math.sin(offsetFi),
                ]

                op.commandlist.append(
                    Path.Command("(Helix to depth: %f)" % passEndDepth)
                )

                if obj.UseHelixArcs is False:
                    # rapid move to start point
                    op.commandlist.append(
                        Path.Command("G0", {"Z": obj.ClearanceHeight.Value})
                    )
                    op.commandlist.append(
                        Path.Command(
                            "G0",
                            {
                                "X": helixStart[0],
                                "Y": helixStart[1],
                                "Z": obj.ClearanceHeight.Value,
                            },
                        )
                    )

                    # rapid move to safe height
                    op.commandlist.append(
                        Path.Command(
                            "G0",
                            {
                                "X": helixStart[0],
                                "Y": helixStart[1],
                                "Z": obj.SafeHeight.Value,
                            },
                        )
                    )

                    # move to start depth
                    op.commandlist.append(
                        Path.Command(
                            "G1",
                            {
                                "X": helixStart[0],
                                "Y": helixStart[1],
                                "Z": passStartDepth,
                                "F": op.vertFeed,
                            },
                        )
                    )

                    if obj.HelixConeAngle == 0:
                        while fi < maxfi:
                            x = region["HelixCenterPoint"][0] + r * math.cos(
                                fi + offsetFi
                            )
                            y = region["HelixCenterPoint"][1] + r * math.sin(
                                fi + offsetFi
                            )
                            z = passStartDepth - fi / maxfi * (
                                passStartDepth - passEndDepth
                            )
                            op.commandlist.append(
                                Path.Command(
                                    "G1", {"X": x, "Y": y, "Z": z, "F": op.vertFeed}
                                )
                            )
                            # lx = x
                            # ly = y
                            fi = fi + math.pi / 16

                        # one more circle at target depth to make sure center is cleared
                        maxfi = maxfi + 2 * math.pi
                        while fi < maxfi:
                            x = region["HelixCenterPoint"][0] + r * math.cos(
                                fi + offsetFi
                            )
                            y = region["HelixCenterPoint"][1] + r * math.sin(
                                fi + offsetFi
                            )
                            z = passEndDepth
                            op.commandlist.append(
                                Path.Command(
                                    "G1", {"X": x, "Y": y, "Z": z, "F": op.horizFeed}
                                )
                            )
                            # lx = x
                            # ly = y
                            fi = fi + math.pi / 16

                    else:
                        # Cone
                        _HelixAngle = 360 - (float(obj.HelixAngle) * 4)

                        if obj.HelixConeAngle > 6:
                            obj.HelixConeAngle = 6

                        helixRadius *= 0.9

                        # Calculate everything
                        helix_height = passStartDepth - passEndDepth
                        r_extra = helix_height * math.tan(
                            math.radians(obj.HelixConeAngle)
                        )
                        HelixTopRadius = helixRadius + r_extra
                        helix_full_height = HelixTopRadius * (
                            math.cos(math.radians(obj.HelixConeAngle))
                            / math.sin(math.radians(obj.HelixConeAngle))
                        )

                        # Start height
                        z = passStartDepth
                        i = 0

                        # Default step down
                        z_step = 0.05

                        # Bigger angle, smaller step down
                        if _HelixAngle > 120:
                            z_step = 0.025
                        if _HelixAngle > 240:
                            z_step = 0.015

                        p = None
                        # Calculate conical helix
                        while z >= passEndDepth:
                            if z < passEndDepth:
                                z = passEndDepth

                            p = CalcHelixConePoint(
                                helix_full_height, i, HelixTopRadius, _HelixAngle
                            )
                            op.commandlist.append(
                                Path.Command(
                                    "G1",
                                    {
                                        "X": p["X"] + region["HelixCenterPoint"][0],
                                        "Y": p["Y"] + region["HelixCenterPoint"][1],
                                        "Z": z,
                                        "F": op.vertFeed,
                                    },
                                )
                            )
                            z = z - z_step
                            i = i + z_step

                        # Calculate some stuff for arcs at bottom
                        p["X"] = p["X"] + region["HelixCenterPoint"][0]
                        p["Y"] = p["Y"] + region["HelixCenterPoint"][1]
                        x_m = (
                            region["HelixCenterPoint"][0]
                            - p["X"]
                            + region["HelixCenterPoint"][0]
                        )
                        y_m = (
                            region["HelixCenterPoint"][1]
                            - p["Y"]
                            + region["HelixCenterPoint"][1]
                        )
                        i_off = (x_m - p["X"]) / 2
                        j_off = (y_m - p["Y"]) / 2

                        # One more circle at target depth to make sure center is cleared
                        op.commandlist.append(
                            Path.Command(
                                "G3",
                                {
                                    "X": x_m,
                                    "Y": y_m,
                                    "Z": passEndDepth,
                                    "I": i_off,
                                    "J": j_off,
                                    "F": op.horizFeed,
                                },
                            )
                        )
                        op.commandlist.append(
                            Path.Command(
                                "G3",
                                {
                                    "X": p["X"],
                                    "Y": p["Y"],
                                    "Z": passEndDepth,
                                    "I": -i_off,
                                    "J": -j_off,
                                    "F": op.horizFeed,
                                },
                            )
                        )

                else:
                    # Use arcs for helix - no conical shape support
                    helixStart = [
                        region["HelixCenterPoint"][0] + r,
                        region["HelixCenterPoint"][1],
                    ]

                    # rapid move to start point
                    op.commandlist.append(
                        Path.Command("G0", {"Z": obj.ClearanceHeight.Value})
                    )
                    op.commandlist.append(
                        Path.Command(
                            "G0",
                            {
                                "X": helixStart[0],
                                "Y": helixStart[1],
                                "Z": obj.ClearanceHeight.Value,
                            },
                        )
                    )

                    # rapid move to safe height
                    op.commandlist.append(
                        Path.Command(
                            "G0",
                            {
                                "X": helixStart[0],
                                "Y": helixStart[1],
                                "Z": obj.SafeHeight.Value,
                            },
                        )
                    )

                    # move to start depth
                    op.commandlist.append(
                        Path.Command(
                            "G1",
                            {
                                "X": helixStart[0],
                                "Y": helixStart[1],
                                "Z": passStartDepth,
                                "F": op.vertFeed,
                            },
                        )
                    )

                    x = region["HelixCenterPoint"][0] + r
                    y = region["HelixCenterPoint"][1]

                    curDep = passStartDepth
                    while curDep > (passEndDepth + depthPerOneCircle):
                        op.commandlist.append(
                            Path.Command(
                                "G2",
                                {
                                    "X": x - (2 * r),
                                    "Y": y,
                                    "Z": curDep - (depthPerOneCircle / 2),
                                    "I": -r,
                                    "F": op.vertFeed,
                                },
                            )
                        )
                        op.commandlist.append(
                            Path.Command(
                                "G2",
                                {
                                    "X": x,
                                    "Y": y,
                                    "Z": curDep - depthPerOneCircle,
                                    "I": r,
                                    "F": op.vertFeed,
                                },
                            )
                        )
                        curDep = curDep - depthPerOneCircle

                    lastStep = curDep - passEndDepth
                    if lastStep > (depthPerOneCircle / 2):
                        op.commandlist.append(
                            Path.Command(
                                "G2",
                                {
                                    "X": x - (2 * r),
                                    "Y": y,
                                    "Z": curDep - (lastStep / 2),
                                    "I": -r,
                                    "F": op.vertFeed,
                                },
                            )
                        )
                        op.commandlist.append(
                            Path.Command(
                                "G2",
                                {
                                    "X": x,
                                    "Y": y,
                                    "Z": passEndDepth,
                                    "I": r,
                                    "F": op.vertFeed,
                                },
                            )
                        )
                    else:
                        op.commandlist.append(
                            Path.Command(
                                "G2",
                                {
                                    "X": x - (2 * r),
                                    "Y": y,
                                    "Z": passEndDepth,
                                    "I": -r,
                                    "F": op.vertFeed,
                                },
                            )
                        )
                        op.commandlist.append(
                            Path.Command(
                                "G1",
                                {"X": x, "Y": y, "Z": passEndDepth, "F": op.vertFeed},
                            )
                        )

                    # one more circle at target depth to make sure center is cleared
                    op.commandlist.append(
                        Path.Command(
                            "G2",
                            {
                                "X": x - (2 * r),
                                "Y": y,
                                "Z": passEndDepth,
                                "I": -r,
                                "F": op.horizFeed,
                            },
                        )
                    )
                    op.commandlist.append(
                        Path.Command(
                            "G2",
                            {
                                "X": x,
                                "Y": y,
                                "Z": passEndDepth,
                                "I": r,
                                "F": op.horizFeed,
                            },
                        )
                    )
                    # lx = x
                    # ly = y

            else:  # no helix entry
                # rapid move to clearance height
                op.commandlist.append(
                    Path.Command("G0", {"Z": obj.ClearanceHeight.Value})
                )
                op.commandlist.append(
                    Path.Command(
                        "G0",
                        {
                            "X": region["StartPoint"][0],
                            "Y": region["StartPoint"][1],
                            "Z": obj.ClearanceHeight.Value,
                        },
                    )
                )
                # straight plunge to target depth
                op.commandlist.append(
                    Path.Command(
                        "G1",
                        {
                            "X": region["StartPoint"][0],
                            "Y": region["StartPoint"][1],
                            "Z": passEndDepth,
                            "F": op.vertFeed,
                        },
                    )
                )

            lz = passEndDepth
            z = obj.ClearanceHeight.Value
            op.commandlist.append(Path.Command("(Adaptive - depth: %f)" % passEndDepth))

            # add adaptive paths
            for pth in region["AdaptivePaths"]:
                motionType = pth[0]  # [0] contains motion type

                for pt in pth[1]:  # [1] contains list of points
                    x = pt[0]
                    y = pt[1]

                    # dist = math.sqrt((x-lx)*(x-lx) + (y-ly)*(y-ly))

                    if motionType == area.AdaptiveMotionType.Cutting:
                        z = passEndDepth
                        if z != lz:
                            op.commandlist.append(
                                Path.Command("G1", {"Z": z, "F": op.vertFeed})
                            )

                        op.commandlist.append(
                            Path.Command("G1", {"X": x, "Y": y, "F": op.horizFeed})
                        )

                    elif motionType == area.AdaptiveMotionType.LinkClear:
                        z = passEndDepth + stepUp
                        if z != lz:
                            op.commandlist.append(Path.Command("G0", {"Z": z}))

                        op.commandlist.append(Path.Command("G0", {"X": x, "Y": y}))

                    elif motionType == area.AdaptiveMotionType.LinkNotClear:
                        z = obj.ClearanceHeight.Value
                        if z != lz:
                            op.commandlist.append(Path.Command("G0", {"Z": z}))

                        op.commandlist.append(Path.Command("G0", {"X": x, "Y": y}))

                    # elif motionType == area.AdaptiveMotionType.LinkClearAtPrevPass:
                    #     if lx!=x or ly!=y:
                    #         op.commandlist.append(Path.Command("G0", { "X": lx, "Y":ly, "Z":passStartDepth+stepUp}))
                    #     op.commandlist.append(Path.Command("G0", { "X": x, "Y":y, "Z":passStartDepth+stepUp}))

                    # lx = x
                    # ly = y
                    lz = z

            # return to safe height in this Z pass
            z = obj.ClearanceHeight.Value
            if z != lz:
                op.commandlist.append(Path.Command("G0", {"Z": z}))

            lz = z

        passStartDepth = passEndDepth

        # return to safe height in this Z pass
        z = obj.ClearanceHeight.Value
        if z != lz:
            op.commandlist.append(Path.Command("G0", {"Z": z}))

        lz = z

    z = obj.ClearanceHeight.Value
    if z != lz:
        op.commandlist.append(Path.Command("G0", {"Z": z}))


def Execute(op, obj):
    global sceneGraph
    global topZ

    if FreeCAD.GuiUp:
        sceneGraph = FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()

    Path.Log.info("*** Adaptive toolpath processing started...\n")

    # hide old toolpaths during recalculation
    obj.Path = Path.Path("(Calculating...)")

    if FreeCAD.GuiUp:
        # store old visibility state
        job = op.getJob(obj)
        oldObjVisibility = obj.ViewObject.Visibility
        oldJobVisibility = job.ViewObject.Visibility

        obj.ViewObject.Visibility = False
        job.ViewObject.Visibility = False

        FreeCADGui.updateGui()

    try:
        helixDiameter = obj.HelixDiameterLimit.Value
        topZ = op.stock.Shape.BoundBox.ZMax
        obj.Stopped = False
        obj.StopProcessing = False
        if obj.Tolerance < 0.001:
            obj.Tolerance = 0.001

        # Get list of working edges for adaptive algorithm
        pathArray = op.pathArray
        if not pathArray:
            Path.Log.error("No wire data returned.")
            return

        path2d = convertTo2d(pathArray)

        stockPaths = []
        if hasattr(op.stock, "StockType") and op.stock.StockType == "CreateCylinder":
            stockPaths.append([discretize(op.stock.Shape.Edges[0])])

        else:
            stockBB = op.stock.Shape.BoundBox
            v = []
            v.append(FreeCAD.Vector(stockBB.XMin, stockBB.YMin, 0))
            v.append(FreeCAD.Vector(stockBB.XMax, stockBB.YMin, 0))
            v.append(FreeCAD.Vector(stockBB.XMax, stockBB.YMax, 0))
            v.append(FreeCAD.Vector(stockBB.XMin, stockBB.YMax, 0))
            v.append(FreeCAD.Vector(stockBB.XMin, stockBB.YMin, 0))
            stockPaths.append([v])

        stockPath2d = convertTo2d(stockPaths)

        # opType = area.AdaptiveOperationType.ClearingInside  # Commented out per LGTM suggestion
        if obj.OperationType == "Clearing":
            if obj.Side == "Outside":
                opType = area.AdaptiveOperationType.ClearingOutside

            else:
                opType = area.AdaptiveOperationType.ClearingInside

        else:  # profiling
            if obj.Side == "Outside":
                opType = area.AdaptiveOperationType.ProfilingOutside

            else:
                opType = area.AdaptiveOperationType.ProfilingInside

        keepToolDownRatio = 3.0
        if hasattr(obj, "KeepToolDownRatio"):
            keepToolDownRatio = float(obj.KeepToolDownRatio)

        # put here all properties that influence calculation of adaptive base paths,

        inputStateObject = {
            "tool": float(op.tool.Diameter),
            "tolerance": float(obj.Tolerance),
            "geometry": path2d,
            "stockGeometry": stockPath2d,
            "stepover": float(obj.StepOver),
            "effectiveHelixDiameter": float(helixDiameter),
            "operationType": obj.OperationType,
            "side": obj.Side,
            "forceInsideOut": obj.ForceInsideOut,
            "finishingProfile": obj.FinishingProfile,
            "keepToolDownRatio": keepToolDownRatio,
            "stockToLeave": float(obj.StockToLeave),
        }

        inputStateChanged = False
        adaptiveResults = None

        if obj.AdaptiveOutputState is not None and obj.AdaptiveOutputState != "":
            adaptiveResults = obj.AdaptiveOutputState

        if json.dumps(obj.AdaptiveInputState) != json.dumps(inputStateObject):
            inputStateChanged = True
            adaptiveResults = None

        # progress callback fn, if return true it will stop processing
        def progressFn(tpaths):
            if FreeCAD.GuiUp:
                for (
                    path
                ) in (
                    tpaths
                ):  # path[0] contains the MotionType, #path[1] contains list of points
                    if path[0] == area.AdaptiveMotionType.Cutting:
                        sceneDrawPath(path[1], (0, 0, 1))

                    else:
                        sceneDrawPath(path[1], (1, 0, 1))

                FreeCADGui.updateGui()

            return obj.StopProcessing

        start = time.time()

        if inputStateChanged or adaptiveResults is None:
            a2d = area.Adaptive2d()
            a2d.stepOverFactor = 0.01 * obj.StepOver
            a2d.toolDiameter = float(op.tool.Diameter)
            a2d.helixRampDiameter = helixDiameter
            a2d.keepToolDownDistRatio = keepToolDownRatio
            a2d.stockToLeave = float(obj.StockToLeave)
            a2d.tolerance = float(obj.Tolerance)
            a2d.forceInsideOut = obj.ForceInsideOut
            a2d.finishingProfile = obj.FinishingProfile
            a2d.opType = opType

            # EXECUTE
            results = a2d.Execute(stockPath2d, path2d, progressFn)

            # need to convert results to python object to be JSON serializable
            adaptiveResults = []
            for result in results:
                adaptiveResults.append(
                    {
                        "HelixCenterPoint": result.HelixCenterPoint,
                        "StartPoint": result.StartPoint,
                        "AdaptivePaths": result.AdaptivePaths,
                        "ReturnMotionType": result.ReturnMotionType,
                    }
                )

        # GENERATE
        GenerateGCode(op, obj, adaptiveResults, helixDiameter)

        if not obj.StopProcessing:
            Path.Log.info("*** Done. Elapsed time: %f sec\n\n" % (time.time() - start))
            obj.AdaptiveOutputState = adaptiveResults
            obj.AdaptiveInputState = inputStateObject

        else:
            Path.Log.info(
                "*** Processing cancelled (after: %f sec).\n\n" % (time.time() - start)
            )

    finally:
        if FreeCAD.GuiUp:
            obj.ViewObject.Visibility = oldObjVisibility
            job.ViewObject.Visibility = oldJobVisibility
            sceneClean()


def _get_working_edges(op, obj):
    """_get_working_edges(op, obj)...
    Compile all working edges from the Base Geometry selection (obj.Base)
    for the current operation.
    Additional modifications to selected region(face), such as extensions,
    should be placed within this function.
    """
    all_regions = list()
    edge_list = list()
    avoidFeatures = list()
    rawEdges = list()

    # Get extensions and identify faces to avoid
    extensions = FeatureExtensions.getExtensions(obj)
    for e in extensions:
        if e.avoid:
            avoidFeatures.append(e.feature)

    # Get faces selected by user
    for base, subs in obj.Base:
        for sub in subs:
            if sub.startswith("Face"):
                if sub not in avoidFeatures:
                    if obj.UseOutline:
                        face = base.Shape.getElement(sub)
                        # get outline with wire_A method used in PocketShape, but it does not play nicely later
                        # wire_A = TechDraw.findShapeOutline(face, 1, FreeCAD.Vector(0.0, 0.0, 1.0))
                        wire_B = face.OuterWire
                        shape = Part.Face(wire_B)
                    else:
                        shape = base.Shape.getElement(sub)
                    all_regions.append(shape)
            elif sub.startswith("Edge"):
                # Save edges for later processing
                rawEdges.append(base.Shape.getElement(sub))
    # Efor

    # Process selected edges
    if rawEdges:
        edgeWires = DraftGeomUtils.findWires(rawEdges)
        if edgeWires:
            for w in edgeWires:
                for e in w.Edges:
                    edge_list.append([discretize(e)])

    # Apply regular Extensions
    op.exts = []
    for ext in extensions:
        if not ext.avoid:
            wire = ext.getWire()
            if wire:
                for f in ext.getExtensionFaces(wire):
                    op.exts.append(f)
                    all_regions.append(f)

    # Second face-combining method attempted
    horizontal = Path.Geom.combineHorizontalFaces(all_regions)
    if horizontal:
        obj.removalshape = Part.makeCompound(horizontal)
        for f in horizontal:
            for w in f.Wires:
                for e in w.Edges:
                    edge_list.append([discretize(e)])

    return edge_list


class PathAdaptive(PathOp.ObjectOp):
    def opFeatures(self, obj):
        """opFeatures(obj) ... returns the OR'ed list of features used and supported by the operation.
        The default implementation returns "FeatureTool | FeatureDepths | FeatureHeights | FeatureStartPoint"
        Should be overwritten by subclasses."""
        return (
            PathOp.FeatureTool
            | PathOp.FeatureBaseEdges
            | PathOp.FeatureDepths
            | PathOp.FeatureFinishDepth
            | PathOp.FeatureStepDown
            | PathOp.FeatureHeights
            | PathOp.FeatureBaseGeometry
            | PathOp.FeatureCoolant
            | PathOp.FeatureLocations
        )

    @classmethod
    def propertyEnumerations(self, dataType="data"):
        """helixOpPropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        # Enumeration lists for App::PropertyEnumeration properties
        enums = {
            "Side": [
                (translate("CAM_Adaptive", "Outside"), "Outside"),
                (translate("CAM_Adaptive", "Inside"), "Inside"),
            ],  # this is the direction that the profile runs
            "OperationType": [
                (translate("CAM_Adaptive", "Clearing"), "Clearing"),
                (translate("CAM_Adaptive", "Profiling"), "Profiling"),
            ],  # side of profile that cutter is on in relation to direction of profile
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

    def initOperation(self, obj):
        """initOperation(obj) ... implement to create additional properties.
        Should be overwritten by subclasses."""
        obj.addProperty(
            "App::PropertyEnumeration",
            "Side",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Side of selected faces that tool should cut",
            ),
        )
        # obj.Side = [
        #     "Outside",
        #     "Inside",
        # ]  # side of profile that cutter is on in relation to direction of profile

        obj.addProperty(
            "App::PropertyEnumeration",
            "OperationType",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Type of adaptive operation",
            ),
        )
        # obj.OperationType = [
        #     "Clearing",
        #     "Profiling",
        # ]  # side of profile that cutter is on in relation to direction of profile

        obj.addProperty(
            "App::PropertyFloat",
            "Tolerance",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Influences accuracy and performance",
            ),
        )
        obj.addProperty(
            "App::PropertyPercent",
            "StepOver",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Percent of cutter diameter to step over on each pass",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "LiftDistance",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Lift distance for rapid moves",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "KeepToolDownRatio",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Max length of keep tool down path compared to direct distance between points",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "StockToLeave",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "How much stock to leave (i.e. for finishing operation)",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "ForceInsideOut",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Force plunging into material inside and clearing towards the edges",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "FinishingProfile",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "To take a finishing profile path at the end",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "Stopped",
            "Adaptive",
            QT_TRANSLATE_NOOP("App::Property", "Stop processing"),
        )
        obj.setEditorMode("Stopped", 2)  # hide this property

        obj.addProperty(
            "App::PropertyBool",
            "StopProcessing",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Stop processing",
            ),
        )
        obj.setEditorMode("StopProcessing", 2)  # hide this property

        obj.addProperty(
            "App::PropertyBool",
            "UseHelixArcs",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Use Arcs (G2) for helix ramp",
            ),
        )

        obj.addProperty(
            "App::PropertyPythonObject",
            "AdaptiveInputState",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Internal input state",
            ),
        )
        obj.addProperty(
            "App::PropertyPythonObject",
            "AdaptiveOutputState",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Internal output state",
            ),
        )
        obj.setEditorMode("AdaptiveInputState", 2)  # hide this property
        obj.setEditorMode("AdaptiveOutputState", 2)  # hide this property
        obj.addProperty(
            "App::PropertyAngle",
            "HelixAngle",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Helix ramp entry angle (degrees)",
            ),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "HelixConeAngle",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Helix cone angle (degrees)",
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "HelixDiameterLimit",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Limit helix entry diameter, if limit larger than tool diameter or 0, tool diameter is used",
            ),
        )

        obj.addProperty(
            "App::PropertyBool",
            "UseOutline",
            "Adaptive",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Uses the outline of the base geometry.",
            ),
        )

        obj.addProperty(
            "Part::PropertyPartShape",
            "removalshape",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", ""),
        )

        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

        obj.setEditorMode("removalshape", 2)  # hide

        FeatureExtensions.initialize_properties(obj)

    def opSetDefaultValues(self, obj, job):
        obj.Side = "Inside"
        obj.OperationType = "Clearing"
        obj.Tolerance = 0.1
        obj.StepOver = 20
        obj.LiftDistance = 0
        # obj.ProcessHoles = True
        obj.ForceInsideOut = False
        obj.FinishingProfile = True
        obj.Stopped = False
        obj.StopProcessing = False
        obj.HelixAngle = 5
        obj.HelixConeAngle = 0
        obj.HelixDiameterLimit = 0.0
        obj.AdaptiveInputState = ""
        obj.AdaptiveOutputState = ""
        obj.StockToLeave = 0
        obj.KeepToolDownRatio = 3.0
        obj.UseHelixArcs = False
        obj.UseOutline = False
        FeatureExtensions.set_default_property_values(obj, job)

    def opExecute(self, obj):
        """opExecute(obj) ... called whenever the receiver needs to be recalculated.
        See documentation of execute() for a list of base functionality provided.
        Should be overwritten by subclasses."""

        self.pathArray = _get_working_edges(self, obj)
        Execute(self, obj)

    def opOnDocumentRestored(self, obj):
        if not hasattr(obj, "HelixConeAngle"):
            obj.addProperty(
                "App::PropertyAngle",
                "HelixConeAngle",
                "Adaptive",
                "Helix cone angle (degrees)",
            )

        if not hasattr(obj, "UseOutline"):
            obj.addProperty(
                "App::PropertyBool",
                "UseOutline",
                "Adaptive",
                "Uses the outline of the base geometry.",
            )

        if not hasattr(obj, "removalshape"):
            obj.addProperty("Part::PropertyPartShape", "removalshape", "Path", "")
        obj.setEditorMode("removalshape", 2)  # hide

        FeatureExtensions.initialize_properties(obj)


# Eclass


def SetupProperties():
    setup = [
        "Side",
        "OperationType",
        "Tolerance",
        "StepOver",
        "LiftDistance",
        "KeepToolDownRatio",
        "StockToLeave",
        "ForceInsideOut",
        "FinishingProfile",
        "Stopped",
        "StopProcessing",
        "UseHelixArcs",
        "AdaptiveInputState",
        "AdaptiveOutputState",
        "HelixAngle",
        "HelixConeAngle",
        "HelixDiameterLimit",
        "UseOutline",
    ]
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Adaptive operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = PathAdaptive(obj, name, parentJob)
    return obj
