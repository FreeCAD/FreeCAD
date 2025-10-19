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
/**
 * @brief Calculate Euclidean distance between two points
 *
 * Used for 2-opt and relocation steps where actual distance matters for path length optimization.
 *
 * @param a First point
 * @param b Second point
 * @return Actual distance: sqrt(dx² + dy²)
 */
double dist(const TSPPoint& a, const TSPPoint& b)
{
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

/**
 * @brief Calculate squared distance between two points (no sqrt)
 *
 * Used for nearest neighbor selection for performance (avoids expensive sqrt operation).
 * Since we only need to compare distances, squared distance preserves ordering:
 * if dist(A,B) < dist(A,C), then distSquared(A,B) < distSquared(A,C)
 *
 * @param a First point
 * @param b Second point
 * @return Squared distance: dx² + dy²
 */
double distSquared(const TSPPoint& a, const TSPPoint& b)
{
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

/**
 * @brief Core TSP solver implementation using nearest neighbor + iterative improvement
 *
 * Algorithm steps:
 * 1. Add temporary start/end points if specified
 * 2. Build initial route using nearest neighbor heuristic
 * 3. Optimize route with 2-opt and relocation moves
 * 4. Remove temporary points and map back to original indices
 *
 * @param points Input points to visit
 * @param startPoint Optional starting location constraint
 * @param endPoint Optional ending location constraint
 * @return Vector of indices representing optimized visit order
 */
std::vector<int> solve_impl(const std::vector<TSPPoint>& points,
                            const TSPPoint* startPoint,
                            const TSPPoint* endPoint)
{
    // ========================================================================
    // STEP 1: Prepare point set with temporary start/end markers
    // ========================================================================
    // We insert temporary points to enforce start/end constraints.
    // These will be removed after optimization and won't appear in final result.
    std::vector<TSPPoint> pts = points;
    int tempStartIdx = -1, tempEndIdx = -1;

    if (startPoint) {
        // Insert user-specified start point at beginning
        pts.insert(pts.begin(), TSPPoint(startPoint->x, startPoint->y));
        tempStartIdx = 0;
    }
    else if (!pts.empty()) {
        // No start specified: duplicate first point as anchor
        pts.insert(pts.begin(), TSPPoint(pts[0].x, pts[0].y));
        tempStartIdx = 0;
    }

    if (endPoint) {
        // Add user-specified end point at the end
        pts.push_back(TSPPoint(endPoint->x, endPoint->y));
        tempEndIdx = static_cast<int>(pts.size()) - 1;
    }

    // ========================================================================
    // STEP 2: Build initial route using Nearest Neighbor algorithm
    // ========================================================================
    // Greedy approach: always visit the closest unvisited point next.
    // This gives a decent initial solution quickly (O(n²) complexity).
    //
    // Tie-breaking rule:
    // - If distances are within ±0.1, prefer point with y-value closer to start
    // - This provides deterministic results when points are nearly equidistant
    std::vector<int> route;
    std::vector<bool> visited(pts.size(), false);
    route.push_back(0);  // Start from temp start point (index 0)
    visited[0] = true;

    for (size_t step = 1; step < pts.size(); ++step) {
        double minDist = std::numeric_limits<double>::max();
        int next = -1;
        double nextYDiff = std::numeric_limits<double>::max();

        // Find nearest unvisited neighbor
        for (size_t i = 0; i < pts.size(); ++i) {
            if (!visited[i]) {
                // Use squared distance for speed (no sqrt needed for comparison)
                double d = distSquared(pts[route.back()], pts[i]);
                double yDiff = std::abs(pts[route.front()].y - pts[i].y);

                // Tie-breaking logic:
                if (d > minDist + 0.1) {
                    continue;  // Clearly farther, skip
                }
                else if (d < minDist - 0.1) {
                    // Clearly closer, use it
                    minDist = d;
                    next = static_cast<int>(i);
                    nextYDiff = yDiff;
                }
                else if (yDiff < nextYDiff) {
                    // Tie: prefer point closer to start in Y-axis
                    minDist = d;
                    next = static_cast<int>(i);
                    nextYDiff = yDiff;
                }
            }
        }

        if (next == -1) {
            break;  // No more unvisited points
        }
        route.push_back(next);
        visited[next] = true;
    }

    // Ensure temporary end point is at the end of route
    if (tempEndIdx != -1 && route.back() != tempEndIdx) {
        auto it = std::find(route.begin(), route.end(), tempEndIdx);
        if (it != route.end()) {
            route.erase(it);
        }
        route.push_back(tempEndIdx);
    }

    // ========================================================================
    // STEP 3: Iterative improvement using 2-Opt and Relocation
    // ========================================================================
    // Repeatedly apply local optimizations until no improvement is possible.
    // This typically converges quickly (a few iterations) to a near-optimal solution.
    //
    // Two optimization techniques:
    // 1. 2-Opt: Reverse segments of the route to eliminate crossing paths
    // 2. Relocation: Move individual points to better positions in the route
    bool improvementFound = true;
    while (improvementFound) {
        improvementFound = false;

        // --- 2-Opt Optimization ---
        // Try reversing every possible segment of the route.
        // If reversing segment [i+1...j-1] reduces total distance, keep it.
        //
        // Example: Route A-B-C-D-E becomes A-D-C-B-E if reversing B-C-D is better
        bool reorderFound = true;
        while (reorderFound) {
            reorderFound = false;
            for (size_t i = 0; i + 3 < route.size(); ++i) {
                for (size_t j = i + 3; j < route.size(); ++j) {
                    // Current edges: i→(i+1) and (j-1)→j
                    double curLen = dist(pts[route[i]], pts[route[i + 1]])
                        + dist(pts[route[j - 1]], pts[route[j]]);

                    // New edges after reversal: (i+1)→j and i→(j-1)
                    // Add epsilon to prevent cycles from floating point errors
                    double newLen = dist(pts[route[i + 1]], pts[route[j]])
                        + dist(pts[route[i]], pts[route[j - 1]]) + 1e-5;

                    if (newLen < curLen) {
                        // Reverse the segment between i+1 and j (exclusive)
                        std::reverse(route.begin() + i + 1, route.begin() + j);
                        reorderFound = true;
                        improvementFound = true;
                    }
                }
            }
        }

        // --- Relocation Optimization ---
        // Try moving each point to a different position in the route.
        // If moving point i to position j improves the route, do it.
        bool relocateFound = true;
        while (relocateFound) {
            relocateFound = false;
            for (size_t i = 1; i + 1 < route.size(); ++i) {

                // Try moving point i backward (to positions before i)
                for (size_t j = 1; j + 2 < i; ++j) {
                    // Current cost: edges around point i and edge j→(j+1)
                    double curLen = dist(pts[route[i - 1]], pts[route[i]])
                        + dist(pts[route[i]], pts[route[i + 1]])
                        + dist(pts[route[j]], pts[route[j + 1]]);

                    // New cost: bypass i, insert i after j
                    double newLen = dist(pts[route[i - 1]], pts[route[i + 1]])
                        + dist(pts[route[j]], pts[route[i]])
                        + dist(pts[route[i]], pts[route[j + 1]]) + 1e-5;

                    if (newLen < curLen) {
                        // Move point i to position after j
                        int node = route[i];
                        route.erase(route.begin() + i);
                        route.insert(route.begin() + j + 1, node);
                        relocateFound = true;
                        improvementFound = true;
                    }
                }

                // Try moving point i forward (to positions after i)
                for (size_t j = i + 1; j + 1 < route.size(); ++j) {
                    double curLen = dist(pts[route[i - 1]], pts[route[i]])
                        + dist(pts[route[i]], pts[route[i + 1]])
                        + dist(pts[route[j]], pts[route[j + 1]]);

                    double newLen = dist(pts[route[i - 1]], pts[route[i + 1]])
                        + dist(pts[route[j]], pts[route[i]])
                        + dist(pts[route[i]], pts[route[j + 1]]) + 1e-5;

                    if (newLen < curLen) {
                        int node = route[i];
                        route.erase(route.begin() + i);
                        route.insert(route.begin() + j, node);
                        relocateFound = true;
                        improvementFound = true;
                    }
                }
            }
        }
    }

    // ========================================================================
    // STEP 4: Remove temporary start/end points
    // ========================================================================
    // The temporary markers served their purpose during optimization.
    // Now remove them so they don't appear in the final result.
    if (tempEndIdx != -1 && !route.empty() && route.back() == tempEndIdx) {
        route.pop_back();
    }
    if (tempStartIdx != -1 && !route.empty() && route.front() == tempStartIdx) {
        route.erase(route.begin());
    }

    // ========================================================================
    // STEP 5: Map route indices back to original point array
    // ========================================================================
    // Since we inserted a temp start point at index 0, all subsequent indices
    // are offset by 1. Adjust them back to match the original points array.
    std::vector<int> result;
    for (int idx : route) {
        // Adjust for temp start offset
        if (tempStartIdx != -1) {
            --idx;
        }
        // Only include valid indices from the original points array
        if (idx >= 0 && idx < static_cast<int>(points.size())) {
            result.push_back(idx);
        }
    }
    return result;
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
    return solve_impl(points, startPoint, endPoint);
}
