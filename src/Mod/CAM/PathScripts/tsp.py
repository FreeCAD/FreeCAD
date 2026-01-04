# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD
import Path
import math


translate = FreeCAD.Qt.translate


def tsp_solver_points(points, routeStartPoint=None, routeEndPoint=None):
    """
    https://forum.freecad.org/viewtopic.php?p=857145#p857145

    Takes a list of points and reorders them to create efficient route for the router
    It uses nearest neighbour algorithm, further improved with 2-opt and relocations

    If needed add extra keys to dictionary, like 'index' and 'd'
    Optional keys of dictionary can be skipped

    'routeStartPoint' and 'routeEndPoint' are optional and specify
    where ROUGHLY the route should start and end


    # get list of points
    points = []
    for i, pos in enumerate(subs):
        points.append(
            {
                "index": i,         # only for debug (optional)
                "x": pos.x,         # x position
                "y": pos.y,         # y position
                "d": holeDiameter,  # diameter of the hole (optional)
            }
        )

    # get reordered list of points
    points = tsp_solver_points(
        points,                     # list of dictionaries
        routeStartPoint=startPoint, # None or FreeCAD.Vector or [x, y]
        routeEndPoint=endPoint,     # None or FreeCAD.Vector or [x, y]
    )
    """

    # STEP 0: Checking input data
    if not isinstance(points, list) or not points or not isinstance(points[-1], dict):
        Path.Log.warning(translate("TSP", "Error input data"))
        return None
    if {"x", "y"} > points[-1].keys():
        Path.Log.warning(translate("TSP", "Not enough keys in dictionary 'points'"))
        return None

    # STEP 1: Adds the routeStartPoint (it will be deleted at the end)
    if routeStartPoint is None:
        points.insert(0, {"x": points[0]["x"], "y": points[0]["y"]})
    else:
        points.insert(0, {"x": routeStartPoint[0], "y": routeStartPoint[1]})

    # STEP 2: Applies nearest neighbour algorithm
    potentialNeighbours = points[1:]
    route = [points[0]]
    while potentialNeighbours:
        costCurrent = float("inf")
        for neighbour in potentialNeighbours:
            costNew = (route[-1]["x"] - neighbour["x"]) ** 2 + (
                route[-1]["y"] - neighbour["y"]
            ) ** 2
            if costNew > costCurrent + 0.1:
                continue
            elif costNew < costCurrent - 0.1:
                costCurrent = costNew
                nearestNeighbour = neighbour
            elif abs(route[0]["y"] - neighbour["y"]) < abs(route[0]["y"] - nearestNeighbour["y"]):
                costCurrent = costNew
                nearestNeighbour = neighbour
        route.append(nearestNeighbour)
        potentialNeighbours.remove(nearestNeighbour)

    # STEP 3: Adds the routeEndPoint (it will be deleted at the end)
    if routeEndPoint is not None:
        route.append({"x": routeEndPoint[0], "y": routeEndPoint[1]})

    # STEP 4: Additional improvement of the route
    limitReorderI = len(route) - 2
    if routeEndPoint is not None:
        limitReorderI -= 1
    limitReorderJ = len(route)
    limitRelocationI = len(route) - 1
    limitRelocationJ = len(route) - 1
    lastImprovementAtStep = 0

    while True:

        # STEP 4.1: Applies 2-opt
        if lastImprovementAtStep == 1:
            break
        improvementFound = True
        while improvementFound:
            improvementFound = False
            for i in range(0, limitReorderI):
                subRouteLengthCurrentPart = math.dist(
                    [route[i]["x"], route[i]["y"]], [route[i + 1]["x"], route[i + 1]["y"]]
                )
                for j in range(i + 3, limitReorderJ):
                    subRouteLengthCurrent = subRouteLengthCurrentPart
                    subRouteLengthCurrent += math.dist(
                        [route[j - 1]["x"], route[j - 1]["y"]], [route[j]["x"], route[j]["y"]]
                    )
                    subRouteLengthNew = math.dist(
                        [route[i + 1]["x"], route[i + 1]["y"]], [route[j]["x"], route[j]["y"]]
                    )
                    subRouteLengthNew += math.dist(
                        [route[i]["x"], route[i]["y"]], [route[j - 1]["x"], route[j - 1]["y"]]
                    )
                    subRouteLengthNew += 10e-6
                    if subRouteLengthNew < subRouteLengthCurrent:
                        route[i + 1 : j] = route[i + 1 : j][
                            ::-1
                        ]  # reverse the order of points between i-th and j-th point
                        subRouteLengthCurrentPart = math.dist(
                            [route[i]["x"], route[i]["y"]], [route[i + 1]["x"], route[i + 1]["y"]]
                        )
                        improvementFound = True
                        lastImprovementAtStep = 1
                if routeEndPoint is None:
                    subRouteLengthCurrent = math.dist(
                        [route[i]["x"], route[i]["y"]], [route[i + 1]["x"], route[i + 1]["y"]]
                    )
                    subRouteLengthNew = math.dist(
                        [route[i]["x"], route[i]["y"]], [route[-1]["x"], route[-1]["y"]]
                    )
                    subRouteLengthNew += 10e-6
                    if subRouteLengthNew < subRouteLengthCurrent:
                        route[i + 1 : limitReorderJ] = route[i + 1 : limitReorderJ][
                            ::-1
                        ]  # reverse the order of points after i-th to the last point
                        improvementFound = True
                        lastImprovementAtStep = 1

        # STEP 4.2: Applies relocation
        if lastImprovementAtStep == 2:
            break
        improvementFound = True
        while improvementFound:
            improvementFound = False
            for i in range(1, limitRelocationI):
                subRouteLengthCurrentPart = math.dist(
                    [route[i - 1]["x"], route[i - 1]["y"]], [route[i]["x"], route[i]["y"]]
                )
                subRouteLengthCurrentPart += math.dist(
                    [route[i]["x"], route[i]["y"]], [route[i + 1]["x"], route[i + 1]["y"]]
                )
                subRouteLengthNewPart = math.dist(
                    [route[i - 1]["x"], route[i - 1]["y"]], [route[i + 1]["x"], route[i + 1]["y"]]
                )
                subRouteLengthNewPart += 10e-6
                for j in range(0, i - 2):
                    subRouteLengthCurrent = subRouteLengthCurrentPart
                    subRouteLengthCurrent += math.dist(
                        [route[j]["x"], route[j]["y"]], [route[j + 1]["x"], route[j + 1]["y"]]
                    )
                    subRouteLengthNew = subRouteLengthNewPart
                    subRouteLengthNew += math.dist(
                        [route[j]["x"], route[j]["y"]], [route[i]["x"], route[i]["y"]]
                    )
                    subRouteLengthNew += math.dist(
                        [route[i]["x"], route[i]["y"]], [route[j + 1]["x"], route[j + 1]["y"]]
                    )
                    if subRouteLengthNew < subRouteLengthCurrent:
                        route.insert(
                            j + 1, route.pop(i)
                        )  # relocate the i-th point backward (after j-th point)
                        subRouteLengthCurrentPart = math.dist(
                            [route[i - 1]["x"], route[i - 1]["y"]], [route[i]["x"], route[i]["y"]]
                        )
                        subRouteLengthCurrentPart += math.dist(
                            [route[i]["x"], route[i]["y"]], [route[i + 1]["x"], route[i + 1]["y"]]
                        )
                        subRouteLengthNewPart = math.dist(
                            [route[i - 1]["x"], route[i - 1]["y"]],
                            [route[i + 1]["x"], route[i + 1]["y"]],
                        )
                        subRouteLengthNewPart += 10e-6
                        improvementFound = True
                        lastImprovementAtStep = 2
                for j in range(i + 1, limitRelocationJ):
                    subRouteLengthCurrent = subRouteLengthCurrentPart
                    subRouteLengthCurrent += math.dist(
                        [route[j]["x"], route[j]["y"]], [route[j + 1]["x"], route[j + 1]["y"]]
                    )
                    subRouteLengthNew = subRouteLengthNewPart
                    subRouteLengthNew += math.dist(
                        [route[j]["x"], route[j]["y"]], [route[i]["x"], route[i]["y"]]
                    )
                    subRouteLengthNew += math.dist(
                        [route[i]["x"], route[i]["y"]], [route[j + 1]["x"], route[j + 1]["y"]]
                    )
                    if subRouteLengthNew < subRouteLengthCurrent:
                        route.insert(
                            j, route.pop(i)
                        )  # relocate the i-th point forward (after j-th point)
                        subRouteLengthCurrentPart = math.dist(
                            [route[i - 1]["x"], route[i - 1]["y"]], [route[i]["x"], route[i]["y"]]
                        )
                        subRouteLengthCurrentPart += math.dist(
                            [route[i]["x"], route[i]["y"]], [route[i + 1]["x"], route[i + 1]["y"]]
                        )
                        subRouteLengthNewPart = math.dist(
                            [route[i - 1]["x"], route[i - 1]["y"]],
                            [route[i + 1]["x"], route[i + 1]["y"]],
                        )
                        subRouteLengthNewPart += 10e-6
                        improvementFound = True
                        lastImprovementAtStep = 2
            if routeEndPoint is None:
                subRouteLengthCurrentPart = math.dist(
                    [route[-2]["x"], route[-2]["y"]], [route[-1]["x"], route[-1]["y"]]
                )
                for j in range(0, len(route) - 2):
                    subRouteLengthCurrent = subRouteLengthCurrentPart
                    subRouteLengthCurrent += math.dist(
                        [route[j]["x"], route[j]["y"]], [route[j + 1]["x"], route[j + 1]["y"]]
                    )
                    subRouteLengthNew = math.dist(
                        [route[j]["x"], route[j]["y"]], [route[-1]["x"], route[-1]["y"]]
                    )
                    subRouteLengthNew += math.dist(
                        [route[-1]["x"], route[-1]["y"]], [route[j + 1]["x"], route[j + 1]["y"]]
                    )
                    subRouteLengthNew += 10e-6
                    if subRouteLengthNew < subRouteLengthCurrent:
                        route.insert(
                            j + 1, route.pop(-1)
                        )  # relocate the last point after j-th point
                        subRouteLengthCurrentPart = math.dist(
                            [route[-2]["x"], route[-2]["y"]], [route[-1]["x"], route[-1]["y"]]
                        )
                        improvementFound = True
                        lastImprovementAtStep = 2

        if lastImprovementAtStep == 0:
            break  # no additional improvementes could be made

    # STEP 5: Deletes temporary start and end points
    route.pop(0)
    if routeEndPoint is not None:
        route.pop()

    return route


def tsp_solver_tunnels(tunnels, allowFlipping=False, routeStartPoint=None, routeEndPoint=None):
    """
    https://forum.freecad.org/viewtopic.php?p=857145#p857145

    Takes a list of tunnels and reorders them to create efficient route for the router
    It uses nearest neighbour algorithm, further improved with relocation
    and (if 'allowFlipping'=True) 2-opt + flipping

    If needed add extra keys to dictionary, like 'index' and 'd'
    Optional keys of dictionary can be skipped

    'allowFlipping' defines whether entry/exit point of each tunnel can be flipped

    'routeStartPoint' and 'routeEndPoint' are optional
    specify where ROUGHLY the route should start and end

    # get list of tunnels
    tunnels = []
    for i, wire in wires:
        tunnels.append(
            {
                "index": i,                                     # *index of the wire
                "startX": wire.OrderedEdges[0].firstVertex().X, # point.x tunnel start
                "startY": wire.OrderedEdges[0].firstVertex().Y, # point.y tunnel start
                "endX": (
                    wire.OrderedVertexes[0].X
                    if wire.isClosed()
                    else wire.OrderedVertexes[-1].X
                ),                                              # point.x tunnel end
                "endY": (
                    wire.OrderedVertexes[0].Y
                    if wire.isClosed()
                    else wire.OrderedVertexes[-1].Y
                ),                                              # point.y tunnel end
                "flipped": False,                               # *init value should be False
                                                                # Result will be True,
                                                                # if flipped wire get better sorting result
            }
        )

    # *Keys 'index' and 'fipped' will be added to dictionary automatically


    # get reordered tunnels
    orderedTunnels = tsp_solver_tunnels(
        tunnels,                         # list of dictionaries
        allowFlipping=allowFlipping,     # False or True
        routeStartPoint=obj.StartPoint,  # None or FreeCAD.Vector or [x, y]
        routeEndPoint=endPoint,          # None or FreeCAD.Vector or [x, y]
    )

    orderedWires = []
    for tunnel in orderedTunnels:
        if tunnel["flipped"]:
            orderedWires.append(Path.Geom.flipWire(wires[wire["index"]]))
        else:
            orderedWires.append(wires[wire["index"]])
    """

    # STEP 0.1: Checking and prepare input data
    if not isinstance(tunnels, list) or not tunnels or not isinstance(tunnels[-1], dict):
        Path.Log.warning(translate("TSP", "Error input data"))
        return None
    if {"startX", "startY", "endX", "endY"} > tunnels[-1].keys():
        Path.Log.warning(translate("TSP", "Not enough keys in dictionary 'tunnels'"))
        return None

    # STEP 0.2: Add keys 'index' and 'fipped'
    for i in range(len(tunnels)):
        tunnels[i].update({"index": i, "flipped": False})

    # STEP 1: Adds the routeStartPoint (it will be deleted at the end)
    if routeStartPoint is None:
        tunnels.insert(0, {"endX": 0, "endY": 0})
    else:
        tunnels.insert(0, {"endX": routeStartPoint[0], "endY": routeStartPoint[1]})

    # STEP 2: Applies nearest neighbour algorithm
    potentialNeighbours = tunnels[1:]
    route = [tunnels[0]]
    while potentialNeighbours:
        costCurrent = float("inf")
        for neighbour in potentialNeighbours:
            costNew = (route[-1]["endX"] - neighbour["startX"]) ** 2 + (
                route[-1]["endY"] - neighbour["startY"]
            ) ** 2
            if costNew < costCurrent:
                costCurrent = costNew
                toBeFlipped = False
                nearestNeighbour = neighbour
        if allowFlipping:
            for neighbour in potentialNeighbours:
                costNew = (route[-1]["endX"] - neighbour["endX"]) ** 2 + (
                    route[-1]["endY"] - neighbour["endY"]
                ) ** 2
                if costNew < costCurrent:
                    costCurrent = costNew
                    toBeFlipped = True
                    nearestNeighbour = neighbour
        potentialNeighbours.remove(nearestNeighbour)
        if toBeFlipped:
            nearestNeighbour["flipped"] = not nearestNeighbour["flipped"]
            nearestNeighbour["startX"], nearestNeighbour["endX"] = (
                nearestNeighbour["endX"],
                nearestNeighbour["startX"],
            )
            nearestNeighbour["startY"], nearestNeighbour["endY"] = (
                nearestNeighbour["endY"],
                nearestNeighbour["startY"],
            )
        route.append(nearestNeighbour)

    # STEP 3: Adds the routeEndPoint (it will be deleted at the end)
    if routeEndPoint is not None:
        route.append({"startX": routeEndPoint[0], "startY": routeEndPoint[1]})

    # STEP 4: Additional improvement of the route
    limitReorderI = len(route) - 2
    if routeEndPoint is not None:
        limitReorderI -= 1
    limitReorderJ = len(route)
    limitFlipI = len(route) - 1
    limitRelocationI = len(route) - 1
    limitRelocationJ = len(route) - 1
    lastImprovementAtStep = 0

    while True:

        if allowFlipping:
            # STEP 4.1: Applies 2-opt
            if lastImprovementAtStep == 1:
                break
            improvementFound = True
            while improvementFound:
                improvementFound = False
                for i in range(0, limitReorderI):
                    subRouteLengthCurrentPart = math.dist(
                        [route[i]["endX"], route[i]["endY"]],
                        [route[i + 1]["startX"], route[i + 1]["startY"]],
                    )
                    for j in range(i + 3, limitReorderJ):
                        subRouteLengthCurrent = subRouteLengthCurrentPart
                        subRouteLengthCurrent += math.dist(
                            [route[j - 1]["endX"], route[j - 1]["endY"]],
                            [route[j]["startX"], route[j]["startY"]],
                        )
                        subRouteLengthNew = math.dist(
                            [route[i + 1]["startX"], route[i + 1]["startY"]],
                            [route[j]["startX"], route[j]["startY"]],
                        )
                        subRouteLengthNew += math.dist(
                            [route[i]["endX"], route[i]["endY"]],
                            [route[j - 1]["endX"], route[j - 1]["endY"]],
                        )
                        subRouteLengthNew += 10e-6
                        if subRouteLengthNew < subRouteLengthCurrent:
                            for k in range(
                                i + 1, j
                            ):  # flips direction of each tunnel between i-th and j-th tunnel
                                route[k]["flipped"] = not route[k]["flipped"]
                                route[k]["startX"], route[k]["endX"] = (
                                    route[k]["endX"],
                                    route[k]["startX"],
                                )
                                route[k]["startY"], route[k]["endY"] = (
                                    route[k]["endY"],
                                    route[k]["startY"],
                                )
                            route[i + 1 : j] = route[i + 1 : j][
                                ::-1
                            ]  # reverse the order of tunnels between i-th and j-th tunnel
                            subRouteLengthCurrentPart = math.dist(
                                [route[i]["endX"], route[i]["endY"]],
                                [route[i + 1]["startX"], route[i + 1]["startY"]],
                            )
                            improvementFound = True
                            lastImprovementAtStep = 1
                    if routeEndPoint is None:
                        subRouteLengthCurrent = math.dist(
                            [route[i]["endX"], route[i]["endY"]],
                            [route[i + 1]["startX"], route[i + 1]["startY"]],
                        )
                        subRouteLengthNew = math.dist(
                            [route[i]["endX"], route[i]["endY"]],
                            [route[-1]["endX"], route[-1]["endY"]],
                        )
                        subRouteLengthNew += 10e-6
                        if subRouteLengthNew < subRouteLengthCurrent:
                            for k in range(
                                i + 1, limitReorderJ
                            ):  # flips direction of each tunnel after i-th to the last tunnel
                                route[k]["flipped"] = not route[k]["flipped"]
                                route[k]["startX"], route[k]["endX"] = (
                                    route[k]["endX"],
                                    route[k]["startX"],
                                )
                                route[k]["startY"], route[k]["endY"] = (
                                    route[k]["endY"],
                                    route[k]["startY"],
                                )
                            route[i + 1 : limitReorderJ] = route[i + 1 : limitReorderJ][
                                ::-1
                            ]  # reverse the order of tunnels after i-th to the last tunnel
                            improvementFound = True
                            lastImprovementAtStep = 1

            # STEP 4.2: Applies flipping
            if lastImprovementAtStep == 2:
                break
            improvementFound = True
            while improvementFound:
                improvementFound = False
                for i in range(1, limitFlipI):
                    subRouteLengthCurrent = math.dist(
                        [route[i - 1]["endX"], route[i - 1]["endY"]],
                        [route[i]["startX"], route[i]["startY"]],
                    )
                    subRouteLengthCurrent += math.dist(
                        [route[i]["endX"], route[i]["endY"]],
                        [route[i + 1]["startX"], route[i + 1]["startY"]],
                    )
                    subRouteLengthNew = math.dist(
                        [route[i - 1]["endX"], route[i - 1]["endY"]],
                        [route[i]["endX"], route[i]["endY"]],
                    )
                    subRouteLengthNew += math.dist(
                        [route[i]["startX"], route[i]["startY"]],
                        [route[i + 1]["startX"], route[i + 1]["startY"]],
                    )
                    subRouteLengthNew += 10e-6
                    if subRouteLengthNew < subRouteLengthCurrent:
                        route[i]["flipped"] = not route[i][
                            "flipped"
                        ]  # flips direction of i-th tunnel
                        route[i]["startX"], route[i]["endX"] = route[i]["endX"], route[i]["startX"]
                        route[i]["startY"], route[i]["endY"] = route[i]["endY"], route[i]["startY"]
                        improvementFound = True
                        lastImprovementAtStep = 2
                if routeEndPoint is None:
                    subRouteLengthCurrent = math.dist(
                        [route[-2]["endX"], route[-2]["endY"]],
                        [route[-1]["startX"], route[-1]["startY"]],
                    )
                    subRouteLengthNew = math.dist(
                        [route[-2]["endX"], route[-2]["endY"]],
                        [route[-1]["endX"], route[-1]["endY"]],
                    )
                    subRouteLengthNew += 10e-6
                    if subRouteLengthNew < subRouteLengthCurrent:
                        route[-1]["flipped"] = not route[-1][
                            "flipped"
                        ]  # flips direction of the last tunnel
                        route[-1]["startX"], route[-1]["endX"] = (
                            route[-1]["endX"],
                            route[-1]["startX"],
                        )
                        route[-1]["startY"], route[-1]["endY"] = (
                            route[-1]["endY"],
                            route[-1]["startY"],
                        )
                        improvementFound = True
                        lastImprovementAtStep = 2

        # STEP 4.3: Applies relocation
        if lastImprovementAtStep == 3:
            break
        improvementFound = True
        while improvementFound:
            improvementFound = False
            for i in range(1, limitRelocationI):
                subRouteLengthCurrentPart = math.dist(
                    [route[i - 1]["endX"], route[i - 1]["endY"]],
                    [route[i]["startX"], route[i]["startY"]],
                )
                subRouteLengthCurrentPart += math.dist(
                    [route[i]["endX"], route[i]["endY"]],
                    [route[i + 1]["startX"], route[i + 1]["startY"]],
                )
                subRouteLengthNewPart = math.dist(
                    [route[i - 1]["endX"], route[i - 1]["endY"]],
                    [route[i + 1]["startX"], route[i + 1]["startY"]],
                )
                subRouteLengthNewPart += 10e-6
                for j in range(0, i - 2):
                    subRouteLengthCurrent = subRouteLengthCurrentPart
                    subRouteLengthCurrent += math.dist(
                        [route[j]["endX"], route[j]["endY"]],
                        [route[j + 1]["startX"], route[j + 1]["startY"]],
                    )
                    subRouteLengthNew = subRouteLengthNewPart
                    subRouteLengthNew += math.dist(
                        [route[j]["endX"], route[j]["endY"]],
                        [route[i]["startX"], route[i]["startY"]],
                    )
                    subRouteLengthNew += math.dist(
                        [route[i]["endX"], route[i]["endY"]],
                        [route[j + 1]["startX"], route[j + 1]["startY"]],
                    )
                    if subRouteLengthNew < subRouteLengthCurrent:
                        route.insert(
                            j + 1, route.pop(i)
                        )  # relocate the i-th tunnel backward (after j-th tunnel)
                        subRouteLengthCurrentPart = math.dist(
                            [route[i - 1]["endX"], route[i - 1]["endY"]],
                            [route[i]["startX"], route[i]["startY"]],
                        )
                        subRouteLengthCurrentPart += math.dist(
                            [route[i]["endX"], route[i]["endY"]],
                            [route[i + 1]["startX"], route[i + 1]["startY"]],
                        )
                        subRouteLengthNewPart = math.dist(
                            [route[i - 1]["endX"], route[i - 1]["endY"]],
                            [route[i + 1]["startX"], route[i + 1]["startY"]],
                        )
                        subRouteLengthNewPart += 10e-6
                        improvementFound = True
                        lastImprovementAtStep = 3
                for j in range(i + 1, limitRelocationJ):
                    subRouteLengthCurrent = subRouteLengthCurrentPart
                    subRouteLengthCurrent += math.dist(
                        [route[j]["endX"], route[j]["endY"]],
                        [route[j + 1]["startX"], route[j + 1]["startY"]],
                    )
                    subRouteLengthNew = subRouteLengthNewPart
                    subRouteLengthNew += math.dist(
                        [route[j]["endX"], route[j]["endY"]],
                        [route[i]["startX"], route[i]["startY"]],
                    )
                    subRouteLengthNew += math.dist(
                        [route[i]["endX"], route[i]["endY"]],
                        [route[j + 1]["startX"], route[j + 1]["startY"]],
                    )
                    if subRouteLengthNew < subRouteLengthCurrent:
                        route.insert(
                            j, route.pop(i)
                        )  # relocate the i-th tunnel forward (after j-th tunnel)
                        subRouteLengthCurrentPart = math.dist(
                            [route[i - 1]["endX"], route[i - 1]["endY"]],
                            [route[i]["startX"], route[i]["startY"]],
                        )
                        subRouteLengthCurrentPart += math.dist(
                            [route[i]["endX"], route[i]["endY"]],
                            [route[i + 1]["startX"], route[i + 1]["startY"]],
                        )
                        subRouteLengthNewPart = math.dist(
                            [route[i - 1]["endX"], route[i - 1]["endY"]],
                            [route[i + 1]["startX"], route[i + 1]["startY"]],
                        )
                        subRouteLengthNewPart += 10e-6
                        improvementFound = True
                        lastImprovementAtStep = 3
            if routeEndPoint is None:
                subRouteLengthCurrentPart = math.dist(
                    [route[-2]["endX"], route[-2]["endY"]],
                    [route[-1]["startX"], route[-1]["startY"]],
                )
                for j in range(0, len(route) - 2):
                    subRouteLengthCurrent = subRouteLengthCurrentPart
                    subRouteLengthCurrent += math.dist(
                        [route[j]["endX"], route[j]["endY"]],
                        [route[j + 1]["startX"], route[j + 1]["startY"]],
                    )
                    subRouteLengthNew = math.dist(
                        [route[j]["endX"], route[j]["endY"]],
                        [route[-1]["startX"], route[-1]["startY"]],
                    )
                    subRouteLengthNew += math.dist(
                        [route[-1]["endX"], route[-1]["endY"]],
                        [route[j + 1]["startX"], route[j + 1]["startY"]],
                    )
                    subRouteLengthNew += 10e-6
                    if subRouteLengthNew < subRouteLengthCurrent:
                        route.insert(
                            j + 1, route.pop(-1)
                        )  # relocate the last tunnel after j-th tunnel
                        subRouteLengthCurrentPart = math.dist(
                            [route[-2]["endX"], route[-2]["endY"]],
                            [route[-1]["startX"], route[-1]["startY"]],
                        )
                        improvementFound = True
                        lastImprovementAtStep = 3

        if lastImprovementAtStep == 0:
            break  # no additional improvementes could be made

    # STEP 5: Deletes temporary start and end points
    route.pop(0)
    if routeEndPoint is not None:
        route.pop()

    return route
