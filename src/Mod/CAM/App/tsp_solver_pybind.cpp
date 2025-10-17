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

std::vector<int> tspSolvePy(const std::vector<std::pair<double, double>>& points)
{
    std::vector<TSPPoint> pts;
    for (const auto& p : points) {
        pts.emplace_back(p.first, p.second);
    }
    return TSPSolver::solve(pts);
}

PYBIND11_MODULE(tsp_solver, m)
{
    m.doc() = "Simple TSP solver (2-Opt) for FreeCAD";
    m.def("solve",
          &tspSolvePy,
          py::arg("points"),
          "Solve TSP for a list of (x, y) points using 2-Opt, returns visit order");
}
