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

#include "tsp_solver.h"
#include <vector>
#include <limits>
#include <cmath>
#include <algorithm>
#include <Base/Precision.h>

namespace
{
// Euclidean distance between two points
double dist(const TSPPoint& a, const TSPPoint& b)
{
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

// Calculate total path length
double pathLength(const std::vector<TSPPoint>& points, const std::vector<int>& path)
{
    double total = 0.0;
    size_t n = path.size();
    for (size_t i = 0; i < n - 1; ++i) {
        total += dist(points[path[i]], points[path[i + 1]]);
    }
    // Optionally, close the loop: total += dist(points[path[n-1]], points[path[0]]);
    return total;
}

// 2-Opt swap
void twoOptSwap(std::vector<int>& path, size_t i, size_t k)
{
    std::reverse(path.begin() + static_cast<long>(i), path.begin() + static_cast<long>(k) + 1);
}
}  // namespace

std::vector<int> TSPSolver::solve(const std::vector<TSPPoint>& points)
{
    size_t n = points.size();
    if (n == 0) {
        return {};
    }
    // Start with a simple nearest neighbor path
    std::vector<bool> visited(n, false);
    std::vector<int> path;
    size_t current = 0;
    path.push_back(static_cast<int>(current));
    visited[current] = true;
    for (size_t step = 1; step < n; ++step) {
        double min_dist = std::numeric_limits<double>::max();
        size_t next = n;  // Use n as an invalid index
        for (size_t i = 0; i < n; ++i) {
            if (!visited[i]) {
                double d = dist(points[current], points[i]);
                if (d < min_dist) {
                    min_dist = d;
                    next = i;
                }
            }
        }
        current = next;
        path.push_back(static_cast<int>(current));
        visited[current] = true;
    }

    // 2-Opt optimization
    bool improved = true;
    while (improved) {
        improved = false;
        for (size_t i = 1; i < n - 1; ++i) {
            for (size_t k = i + 1; k < n; ++k) {
                double delta = dist(points[path[i - 1]], points[path[k]])
                    + dist(points[path[i]], points[path[(k + 1) % n]])
                    - dist(points[path[i - 1]], points[path[i]])
                    - dist(points[path[k]], points[path[(k + 1) % n]]);
                if (delta < -Base::Precision::Confusion()) {
                    twoOptSwap(path, i, k);
                    improved = true;
                }
            }
        }
    }
    return path;
}
