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

std::vector<int> tspSolvePy(const std::vector<std::pair<double, double>>& points,
                            const py::object& startPoint = py::none(),
                            const py::object& endPoint = py::none())
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

PYBIND11_MODULE(tsp_solver, m)
{
    m.doc() = "Simple TSP solver (2-Opt) for FreeCAD";
    m.def("solve",
          &tspSolvePy,
          py::arg("points"),
          py::arg("startPoint") = py::none(),
          py::arg("endPoint") = py::none(),
          "Solve TSP for a list of (x, y) points using 2-Opt, returns visit order.\n"
          "Optional arguments:\n"
          "- startPoint: Optional [x, y] point where the path should start (closest point will be "
          "chosen)\n"
          "- endPoint: Optional [x, y] point where the path should end (closest point will be "
          "chosen)");
}
