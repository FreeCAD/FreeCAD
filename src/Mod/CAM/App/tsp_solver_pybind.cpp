// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Billy Huddleston <billy@ivdc.com>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License (LGPL)    *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "tsp_solver.h"

namespace py = pybind11;

std::vector<int> tspSolvePy(
    const std::vector<std::pair<double, double>>& points,
    const py::object& startPoint = py::none(),
    const py::object& endPoint = py::none()
)
{
    std::vector<TSPPoint> pts;
    for (const auto& p : points) {
        pts.emplace_back(p.first, p.second);
    }

    // Handle optional start point
    TSPPoint* pStartPoint = nullptr;
    TSPPoint startPointObj(0, 0);
    if (!startPoint.is_none()) {
        try {
            // Use py::cast to convert to standard C++ types
            auto sp = startPoint.cast<std::vector<double>>();
            if (sp.size() >= 2) {
                startPointObj.x = sp[0];
                startPointObj.y = sp[1];
                pStartPoint = &startPointObj;
            }
        }
        catch (py::cast_error&) {
            // If casting fails, try accessing elements directly
            try {
                if (py::len(startPoint) >= 2) {
                    startPointObj.x = py::cast<double>(startPoint.attr("__getitem__")(0));
                    startPointObj.y = py::cast<double>(startPoint.attr("__getitem__")(1));
                    pStartPoint = &startPointObj;
                }
            }
            catch (py::error_already_set&) {
                // Ignore if we can't access the elements
            }
        }
    }

    // Handle optional end point
    TSPPoint* pEndPoint = nullptr;
    TSPPoint endPointObj(0, 0);
    if (!endPoint.is_none()) {
        try {
            // Use py::cast to convert to standard C++ types
            auto ep = endPoint.cast<std::vector<double>>();
            if (ep.size() >= 2) {
                endPointObj.x = ep[0];
                endPointObj.y = ep[1];
                pEndPoint = &endPointObj;
            }
        }
        catch (py::cast_error&) {
            // If casting fails, try accessing elements directly
            try {
                if (py::len(endPoint) >= 2) {
                    endPointObj.x = py::cast<double>(endPoint.attr("__getitem__")(0));
                    endPointObj.y = py::cast<double>(endPoint.attr("__getitem__")(1));
                    pEndPoint = &endPointObj;
                }
            }
            catch (py::error_already_set&) {
                // Ignore if we can't access the elements
            }
        }
    }

    return TSPSolver::solve(pts, pStartPoint, pEndPoint);
}

// Python wrapper for solveTunnels function
std::vector<py::dict> tspSolveTunnelsPy(
    const std::vector<py::dict>& tunnels,
    bool allowFlipping = false,
    const py::object& routeStartPoint = py::none(),
    const py::object& routeEndPoint = py::none()
)
{
    std::vector<TSPTunnel> cppTunnels;

    // Convert Python dictionaries to C++ TSPTunnel objects
    for (const auto& tunnel : tunnels) {
        double startX = py::cast<double>(tunnel["startX"]);
        double startY = py::cast<double>(tunnel["startY"]);
        double endX = py::cast<double>(tunnel["endX"]);
        double endY = py::cast<double>(tunnel["endY"]);
        bool isOpen = tunnel.contains("isOpen") ? py::cast<bool>(tunnel["isOpen"]) : true;

        cppTunnels.emplace_back(startX, startY, endX, endY, isOpen);
    }

    // Handle optional start point
    TSPPoint* pStartPoint = nullptr;
    TSPPoint startPointObj(0, 0);
    if (!routeStartPoint.is_none()) {
        try {
            auto sp = routeStartPoint.cast<std::vector<double>>();
            if (sp.size() >= 2) {
                startPointObj.x = sp[0];
                startPointObj.y = sp[1];
                pStartPoint = &startPointObj;
            }
        }
        catch (py::cast_error&) {
            try {
                if (py::len(routeStartPoint) >= 2) {
                    startPointObj.x = py::cast<double>(routeStartPoint.attr("__getitem__")(0));
                    startPointObj.y = py::cast<double>(routeStartPoint.attr("__getitem__")(1));
                    pStartPoint = &startPointObj;
                }
            }
            catch (py::error_already_set&) {
                // Ignore if we can't access the elements
            }
        }
    }

    // Handle optional end point
    TSPPoint* pEndPoint = nullptr;
    TSPPoint endPointObj(0, 0);
    if (!routeEndPoint.is_none()) {
        try {
            auto ep = routeEndPoint.cast<std::vector<double>>();
            if (ep.size() >= 2) {
                endPointObj.x = ep[0];
                endPointObj.y = ep[1];
                pEndPoint = &endPointObj;
            }
        }
        catch (py::cast_error&) {
            try {
                if (py::len(routeEndPoint) >= 2) {
                    endPointObj.x = py::cast<double>(routeEndPoint.attr("__getitem__")(0));
                    endPointObj.y = py::cast<double>(routeEndPoint.attr("__getitem__")(1));
                    pEndPoint = &endPointObj;
                }
            }
            catch (py::error_already_set&) {
                // Ignore if we can't access the elements
            }
        }
    }

    // Solve the tunnel TSP
    auto result = TSPSolver::solveTunnels(cppTunnels, allowFlipping, pStartPoint, pEndPoint);

    // Convert result back to Python dictionaries
    std::vector<py::dict> pyResult;
    for (const auto& tunnel : result) {
        py::dict tunnelDict;
        tunnelDict["startX"] = tunnel.startX;
        tunnelDict["startY"] = tunnel.startY;
        tunnelDict["endX"] = tunnel.endX;
        tunnelDict["endY"] = tunnel.endY;
        tunnelDict["isOpen"] = tunnel.isOpen;
        tunnelDict["flipped"] = tunnel.flipped;
        tunnelDict["index"] = tunnel.index;
        pyResult.push_back(tunnelDict);
    }

    return pyResult;
}

PYBIND11_MODULE(tsp_solver, m)
{
    m.doc() = "Simple TSP solver (2-Opt) for FreeCAD";

    m.def(
        "solve",
        &tspSolvePy,
        py::arg("points"),
        py::arg("startPoint") = py::none(),
        py::arg("endPoint") = py::none(),
        "Solve TSP for a list of (x, y) points using 2-Opt, returns visit order.\n"
        "Optional arguments:\n"
        "- startPoint: Optional [x, y] point where the path should start (closest point will be "
        "chosen)\n"
        "- endPoint: Optional [x, y] point where the path should end (closest point will be "
        "chosen)"
    );

    m.def(
        "solveTunnels",
        &tspSolveTunnelsPy,
        py::arg("tunnels"),
        py::arg("allowFlipping") = false,
        py::arg("routeStartPoint") = py::none(),
        py::arg("routeEndPoint") = py::none(),
        "Solve TSP for tunnels (path segments with entry/exit points).\n"
        "Arguments:\n"
        "- tunnels: List of dictionaries with keys: startX, startY, endX, endY, isOpen (optional)\n"
        "- allowFlipping: Whether tunnels can be reversed (entry becomes exit)\n"
        "- routeStartPoint: Optional [x, y] point where route should start\n"
        "- routeEndPoint: Optional [x, y] point where route should end\n"
        "Returns: List of tunnel dictionaries in optimized order with flipped status"
    );
}
