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

/**
 * @brief Solve the Traveling Salesperson Problem using 2-opt algorithm
 *
 * This implementation handles optional start and end point constraints:
 * - If startPoint is provided, the path will begin at the point closest to startPoint
 * - If endPoint is provided, the path will end at the point closest to endPoint
 * - If both are provided, the path will respect both constraints while optimizing the middle path
 * - The algorithm ensures all points are visited exactly once
 */
std::vector<int> TSPSolver::solve(const std::vector<TSPPoint>& points,
                                  const TSPPoint* startPoint,
                                  const TSPPoint* endPoint)
{
    size_t n = points.size();
    if (n == 0) {
        return {};
    }

    // Start with a simple nearest neighbor path
    std::vector<bool> visited(n, false);
    std::vector<int> path;

    // If startPoint provided, find the closest point to it
    size_t current = 0;
    if (startPoint) {
        double minDist = std::numeric_limits<double>::max();
        for (size_t i = 0; i < n; ++i) {
            double d = dist(points[i], *startPoint);
            if (d < minDist) {
                minDist = d;
                current = i;
            }
        }
    }

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

    // Handle end point constraint if specified
    if (endPoint) {
        // If both start and end points are specified, we need to handle them differently
        if (startPoint) {
            // Find the closest points to start and end
            size_t startIdx = 0;
            size_t endIdx = 0;
            double minStartDist = std::numeric_limits<double>::max();
            double minEndDist = std::numeric_limits<double>::max();

            // Find the indices of the closest points to both start and end points
            for (size_t i = 0; i < n; ++i) {
                // Find closest to start
                double dStart = dist(points[i], *startPoint);
                if (dStart < minStartDist) {
                    minStartDist = dStart;
                    startIdx = i;
                }

                // Find closest to end
                double dEnd = dist(points[i], *endPoint);
                if (dEnd < minEndDist) {
                    minEndDist = dEnd;
                    endIdx = i;
                }
            }

            // If start and end are different points, create a new path
            if (startIdx != endIdx) {
                // Create a new path starting with the start point and ending with the end point
                // This ensures both constraints are met
                std::vector<bool> visited(n, false);
                std::vector<int> newPath;

                // Add start point
                newPath.push_back(static_cast<int>(startIdx));
                visited[startIdx] = true;

                // Add all other points except end point using nearest neighbor algorithm
                // This builds a path that starts at startIdx and visits all intermediate points
                size_t current = startIdx;
                for (size_t step = 1; step < n - 1; ++step) {
                    double minDist = std::numeric_limits<double>::max();
                    size_t next = n;  // Invalid index (n is out of bounds)

                    for (size_t i = 0; i < n; ++i) {
                        if (!visited[i] && i != endIdx) {
                            double d = dist(points[current], points[i]);
                            if (d < minDist) {
                                minDist = d;
                                next = i;
                            }
                        }
                    }

                    if (next == n) {
                        break;  // No more points to add
                    }

                    current = next;
                    newPath.push_back(static_cast<int>(current));
                    visited[current] = true;
                }

                // Add end point as the final stop in the path
                newPath.push_back(static_cast<int>(endIdx));

                // Apply 2-opt optimization while preserving the start and end points
                // The algorithm only swaps edges between interior points
                bool improved = true;
                while (improved) {
                    improved = false;
                    // Start from 1 and end before the last point to preserve start/end constraints
                    for (size_t i = 1; i < newPath.size() - 1; ++i) {
                        for (size_t k = i + 1; k < newPath.size() - 1; ++k) {
                            // Calculate improvement in distance if we swap these edges
                            double delta = dist(points[newPath[i - 1]], points[newPath[k]])
                                + dist(points[newPath[i]], points[newPath[k + 1]])
                                - dist(points[newPath[i - 1]], points[newPath[i]])
                                - dist(points[newPath[k]], points[newPath[k + 1]]);

                            // If the swap reduces the total distance, make the swap
                            if (delta < -Base::Precision::Confusion()) {
                                std::reverse(newPath.begin() + static_cast<long>(i),
                                             newPath.begin() + static_cast<long>(k) + 1);
                                improved = true;
                            }
                        }
                    }
                }

                path = newPath;
            }
            // If start and end are the same point, keep path as is
        }
        else {
            // Only end point specified (no start point constraint)
            // Find the point in the current path that's closest to the desired end point
            double minDist = std::numeric_limits<double>::max();
            size_t endIdx = 0;
            for (size_t i = 0; i < n; ++i) {
                double d = dist(points[path[i]], *endPoint);
                if (d < minDist) {
                    minDist = d;
                    endIdx = i;
                }
            }

            // Rotate the path so that endIdx is at the end
            // This preserves the relative order of points while ensuring the path ends
            // at the point closest to the specified end coordinates
            if (endIdx != n - 1) {
                std::vector<int> newPath;
                // Start with points after endIdx
                for (size_t i = endIdx + 1; i < n; ++i) {
                    newPath.push_back(path[i]);
                }
                // Then add points from beginning up to and including endIdx
                for (size_t i = 0; i <= endIdx; ++i) {
                    newPath.push_back(path[i]);
                }
                path = newPath;
            }
        }
    }

    return path;
}
