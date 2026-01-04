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
std::vector<int> solve_impl(
    const std::vector<TSPPoint>& points,
    const TSPPoint* startPoint,
    const TSPPoint* endPoint
)
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
    //
    // For open routes (no endPoint), additional optimizations are applied that
    // allow reversing/relocating segments to the end of the route.
    size_t limitReorderI = route.size() - 2;
    if (tempEndIdx != -1) {
        limitReorderI -= 1;
    }
    size_t limitReorderJ = route.size();
    size_t limitRelocationI = route.size() - 1;
    size_t limitRelocationJ = route.size() - 1;
    int lastImprovementAtStep = 0;

    while (true) {

        // --- 2-Opt Optimization ---
        // Try reversing every possible segment of the route.
        // If reversing segment [i+1...j-1] reduces total distance, keep it.
        //
        // Example: Route A-B-C-D-E becomes A-D-C-B-E if reversing B-C-D is better
        if (lastImprovementAtStep == 1) {
            break;
        }
        bool reorderFound = true;
        while (reorderFound) {
            reorderFound = false;
            for (size_t i = 0; i < limitReorderI; ++i) {
                double subRouteLengthCurrentPart = dist(pts[route[i]], pts[route[i + 1]]);

                for (size_t j = i + 3; j < limitReorderJ; ++j) {
                    // Current edges: i→(i+1) and (j-1)→j
                    double curLen = subRouteLengthCurrentPart
                        + dist(pts[route[j - 1]], pts[route[j]]);

                    // New edges after reversal: (i+1)→j and i→(j-1)
                    // Add epsilon to prevent cycles from floating point errors
                    double newLen = dist(pts[route[i + 1]], pts[route[j]])
                        + dist(pts[route[i]], pts[route[j - 1]]) + Base::Precision::Confusion();

                    if (newLen < curLen) {
                        // Reverse the segment between i+1 and j (exclusive)
                        std::reverse(route.begin() + i + 1, route.begin() + j);
                        subRouteLengthCurrentPart = dist(pts[route[i]], pts[route[i + 1]]);
                        reorderFound = true;
                        lastImprovementAtStep = 1;
                    }
                }

                // Open route optimization: can reverse from i to end if no endpoint constraint
                if (tempEndIdx == -1) {
                    double curLen = dist(pts[route[i]], pts[route[i + 1]]);
                    double newLen = dist(pts[route[i]], pts[route[limitReorderJ - 1]])
                        + Base::Precision::Confusion();

                    if (newLen < curLen) {
                        // Reverse the order of points after i-th to the last point
                        std::reverse(route.begin() + i + 1, route.begin() + limitReorderJ);
                        reorderFound = true;
                        lastImprovementAtStep = 1;
                    }
                }
            }
        }

        // --- Relocation Optimization ---
        // Try moving each point to a different position in the route.
        // If moving point i to position j improves the route, do it.
        if (lastImprovementAtStep == 2) {
            break;
        }
        bool relocateFound = true;
        while (relocateFound) {
            relocateFound = false;
            for (size_t i = 1; i < limitRelocationI; ++i) {
                double subRouteLengthCurrentPart = dist(pts[route[i - 1]], pts[route[i]])
                    + dist(pts[route[i]], pts[route[i + 1]]);
                double subRouteLengthNewPart = dist(pts[route[i - 1]], pts[route[i + 1]])
                    + Base::Precision::Confusion();

                // Try moving point i backward (to positions before i)
                for (size_t j = 0; j + 2 < i; ++j) {
                    // Current cost: edges around point i and edge j→(j+1)
                    double curLen = subRouteLengthCurrentPart
                        + dist(pts[route[j]], pts[route[j + 1]]);

                    // New cost: bypass i, insert i after j
                    double newLen = subRouteLengthNewPart + dist(pts[route[j]], pts[route[i]])
                        + dist(pts[route[i]], pts[route[j + 1]]);

                    if (newLen < curLen) {
                        // Move point i to position after j
                        int node = route[i];
                        route.erase(route.begin() + i);
                        route.insert(route.begin() + j + 1, node);
                        subRouteLengthCurrentPart = dist(pts[route[i - 1]], pts[route[i]])
                            + dist(pts[route[i]], pts[route[i + 1]]);
                        subRouteLengthNewPart = dist(pts[route[i - 1]], pts[route[i + 1]])
                            + Base::Precision::Confusion();
                        relocateFound = true;
                        lastImprovementAtStep = 2;
                    }
                }

                // Try moving point i forward (to positions after i)
                for (size_t j = i + 1; j < limitRelocationJ; ++j) {
                    double curLen = subRouteLengthCurrentPart
                        + dist(pts[route[j]], pts[route[j + 1]]);

                    double newLen = subRouteLengthNewPart + dist(pts[route[j]], pts[route[i]])
                        + dist(pts[route[i]], pts[route[j + 1]]);

                    if (newLen < curLen) {
                        int node = route[i];
                        route.erase(route.begin() + i);
                        route.insert(route.begin() + j, node);
                        subRouteLengthCurrentPart = dist(pts[route[i - 1]], pts[route[i]])
                            + dist(pts[route[i]], pts[route[i + 1]]);
                        subRouteLengthNewPart = dist(pts[route[i - 1]], pts[route[i + 1]])
                            + Base::Precision::Confusion();
                        relocateFound = true;
                        lastImprovementAtStep = 2;
                    }
                }
            }

            // Open route optimization: can relocate the last point anywhere
            if (tempEndIdx == -1) {
                double subRouteLengthCurrentPart
                    = dist(pts[route[route.size() - 2]], pts[route[route.size() - 1]]);

                for (size_t j = 0; j + 2 < route.size(); ++j) {
                    double curLen = subRouteLengthCurrentPart
                        + dist(pts[route[j]], pts[route[j + 1]]);

                    double newLen = dist(pts[route[j]], pts[route[route.size() - 1]])
                        + dist(pts[route[route.size() - 1]], pts[route[j + 1]])
                        + Base::Precision::Confusion();

                    if (newLen < curLen) {
                        // Relocate the last point after j-th point
                        int node = route[route.size() - 1];
                        route.erase(route.begin() + route.size() - 1);
                        route.insert(route.begin() + j + 1, node);
                        subRouteLengthCurrentPart
                            = dist(pts[route[route.size() - 2]], pts[route[route.size() - 1]]);
                        relocateFound = true;
                        lastImprovementAtStep = 2;
                    }
                }
            }
        }

        if (lastImprovementAtStep == 0) {
            break;  // No additional improvements could be made
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


std::vector<int> TSPSolver::solve(
    const std::vector<TSPPoint>& points,
    const TSPPoint* startPoint,
    const TSPPoint* endPoint
)
{
    return solve_impl(points, startPoint, endPoint);
}

std::vector<TSPTunnel> TSPSolver::solveTunnels(
    std::vector<TSPTunnel> tunnels,
    bool allowFlipping,
    const TSPPoint* routeStartPoint,
    const TSPPoint* routeEndPoint
)
{
    if (tunnels.empty()) {
        return tunnels;
    }

    // Set original indices
    for (size_t i = 0; i < tunnels.size(); ++i) {
        tunnels[i].index = static_cast<int>(i);
    }

    // STEP 1: Add the routeStartPoint (will be deleted at the end)
    if (routeStartPoint) {
        tunnels.insert(
            tunnels.begin(),
            TSPTunnel(routeStartPoint->x, routeStartPoint->y, routeStartPoint->x, routeStartPoint->y, false)
        );
    }
    else {
        tunnels.insert(tunnels.begin(), TSPTunnel(0.0, 0.0, 0.0, 0.0, false));
    }

    // STEP 2: Apply nearest neighbor algorithm
    std::vector<TSPTunnel> potentialNeighbours(tunnels.begin() + 1, tunnels.end());
    std::vector<TSPTunnel> route;
    route.push_back(tunnels[0]);

    while (!potentialNeighbours.empty()) {
        double costCurrent = std::numeric_limits<double>::max();
        bool toBeFlipped = false;
        auto nearestNeighbour = potentialNeighbours.begin();

        // Check normal orientation
        for (auto it = potentialNeighbours.begin(); it != potentialNeighbours.end(); ++it) {
            double dx = route.back().endX - it->startX;
            double dy = route.back().endY - it->startY;
            double costNew = dx * dx + dy * dy;

            if (costNew < costCurrent) {
                costCurrent = costNew;
                toBeFlipped = false;
                nearestNeighbour = it;
            }
        }

        // Check flipped orientation if allowed
        if (allowFlipping) {
            for (auto it = potentialNeighbours.begin(); it != potentialNeighbours.end(); ++it) {
                if (it->isOpen) {
                    double dx = route.back().endX - it->endX;
                    double dy = route.back().endY - it->endY;
                    double costNew = dx * dx + dy * dy;

                    if (costNew < costCurrent) {
                        costCurrent = costNew;
                        toBeFlipped = true;
                        nearestNeighbour = it;
                    }
                }
            }
        }

        // Apply flipping if needed
        if (toBeFlipped) {
            nearestNeighbour->flipped = !nearestNeighbour->flipped;
            std::swap(nearestNeighbour->startX, nearestNeighbour->endX);
            std::swap(nearestNeighbour->startY, nearestNeighbour->endY);
        }

        route.push_back(*nearestNeighbour);
        potentialNeighbours.erase(nearestNeighbour);
    }

    // STEP 3: Add the routeEndPoint (will be deleted at the end)
    if (routeEndPoint) {
        route.push_back(
            TSPTunnel(routeEndPoint->x, routeEndPoint->y, routeEndPoint->x, routeEndPoint->y, false)
        );
    }

    // STEP 4: Additional improvement of the route
    size_t limitReorderI = route.size() - 2;
    if (routeEndPoint) {
        limitReorderI -= 1;
    }
    size_t limitReorderJ = route.size();
    size_t limitFlipI = route.size() - 1;
    size_t limitRelocationI = route.size() - 1;
    size_t limitRelocationJ = route.size() - 1;
    int lastImprovementAtStep = 0;

    while (true) {

        if (allowFlipping) {
            // STEP 4.1: Apply 2-opt
            if (lastImprovementAtStep == 1) {
                break;
            }
            bool improvementFound = true;
            while (improvementFound) {
                improvementFound = false;
                for (size_t i = 0; i < limitReorderI; ++i) {
                    double subRouteLengthCurrentPart = std::sqrt(
                        std::pow(route[i].endX - route[i + 1].startX, 2)
                        + std::pow(route[i].endY - route[i + 1].startY, 2)
                    );
                    for (size_t j = i + 3; j < limitReorderJ; ++j) {
                        double subRouteLengthCurrent = subRouteLengthCurrentPart;
                        subRouteLengthCurrent += std::sqrt(
                            std::pow(route[j - 1].endX - route[j].startX, 2)
                            + std::pow(route[j - 1].endY - route[j].startY, 2)
                        );
                        double subRouteLengthNew = std::sqrt(
                            std::pow(route[i + 1].startX - route[j].startX, 2)
                            + std::pow(route[i + 1].startY - route[j].startY, 2)
                        );
                        subRouteLengthNew += std::sqrt(
                            std::pow(route[i].endX - route[j - 1].endX, 2)
                            + std::pow(route[i].endY - route[j - 1].endY, 2)
                        );
                        subRouteLengthNew += Base::Precision::Confusion();

                        if (subRouteLengthNew < subRouteLengthCurrent) {
                            // Flip direction of each tunnel between i-th and j-th tunnel
                            for (size_t k = i + 1; k < j; ++k) {
                                if (route[k].isOpen) {
                                    route[k].flipped = !route[k].flipped;
                                    std::swap(route[k].startX, route[k].endX);
                                    std::swap(route[k].startY, route[k].endY);
                                }
                            }
                            // Reverse the order of tunnels between i-th and j-th tunnel
                            std::reverse(route.begin() + i + 1, route.begin() + j);
                            subRouteLengthCurrentPart = std::sqrt(
                                std::pow(route[i].endX - route[i + 1].startX, 2)
                                + std::pow(route[i].endY - route[i + 1].startY, 2)
                            );
                            improvementFound = true;
                            lastImprovementAtStep = 1;
                        }
                    }
                    if (!routeEndPoint) {
                        double subRouteLengthCurrent = std::sqrt(
                            std::pow(route[i].endX - route[i + 1].startX, 2)
                            + std::pow(route[i].endY - route[i + 1].startY, 2)
                        );
                        double subRouteLengthNew = std::sqrt(
                            std::pow(route[i].endX - route[route.size() - 1].endX, 2)
                            + std::pow(route[i].endY - route[route.size() - 1].endY, 2)
                        );
                        subRouteLengthNew += Base::Precision::Confusion();
                        if (subRouteLengthNew < subRouteLengthCurrent) {
                            // Flip direction of each tunnel after i-th to the last tunnel
                            for (size_t k = i + 1; k < limitReorderJ; ++k) {
                                if (route[k].isOpen) {
                                    route[k].flipped = !route[k].flipped;
                                    std::swap(route[k].startX, route[k].endX);
                                    std::swap(route[k].startY, route[k].endY);
                                }
                            }
                            // Reverse the order of tunnels after i-th to the last tunnel
                            std::reverse(route.begin() + i + 1, route.begin() + limitReorderJ);
                            improvementFound = true;
                            lastImprovementAtStep = 1;
                        }
                    }
                }
            }

            // STEP 4.2: Apply flipping
            if (lastImprovementAtStep == 2) {
                break;
            }
            improvementFound = true;
            while (improvementFound) {
                improvementFound = false;
                for (size_t i = 1; i < limitFlipI; ++i) {
                    if (route[i].isOpen) {
                        double subRouteLengthCurrent = std::sqrt(
                            std::pow(route[i - 1].endX - route[i].startX, 2)
                            + std::pow(route[i - 1].endY - route[i].startY, 2)
                        );
                        subRouteLengthCurrent += std::sqrt(
                            std::pow(route[i].endX - route[i + 1].startX, 2)
                            + std::pow(route[i].endY - route[i + 1].startY, 2)
                        );

                        double subRouteLengthNew = std::sqrt(
                            std::pow(route[i - 1].endX - route[i].endX, 2)
                            + std::pow(route[i - 1].endY - route[i].endY, 2)
                        );
                        subRouteLengthNew += std::sqrt(
                            std::pow(route[i].startX - route[i + 1].startX, 2)
                            + std::pow(route[i].startY - route[i + 1].startY, 2)
                        );
                        subRouteLengthNew += Base::Precision::Confusion();

                        if (subRouteLengthNew < subRouteLengthCurrent) {
                            // Flip direction of i-th tunnel
                            route[i].flipped = !route[i].flipped;
                            std::swap(route[i].startX, route[i].endX);
                            std::swap(route[i].startY, route[i].endY);
                            improvementFound = true;
                            lastImprovementAtStep = 2;
                        }
                    }
                }
                if (!routeEndPoint) {
                    if (route[route.size() - 1].isOpen) {
                        double subRouteLengthCurrent = std::sqrt(
                            std::pow(route[route.size() - 2].endX - route[route.size() - 1].startX, 2)
                            + std::pow(route[route.size() - 2].endY - route[route.size() - 1].startY, 2)
                        );
                        double subRouteLengthNew = std::sqrt(
                            std::pow(route[route.size() - 2].endX - route[route.size() - 1].endX, 2)
                            + std::pow(route[route.size() - 2].endY - route[route.size() - 1].endY, 2)
                        );
                        subRouteLengthNew += Base::Precision::Confusion();
                        if (subRouteLengthNew < subRouteLengthCurrent) {
                            // Flip direction of the last tunnel
                            route[route.size() - 1].flipped = !route[route.size() - 1].flipped;
                            std::swap(route[route.size() - 1].startX, route[route.size() - 1].endX);
                            std::swap(route[route.size() - 1].startY, route[route.size() - 1].endY);
                            improvementFound = true;
                            lastImprovementAtStep = 2;
                        }
                    }
                }
            }
        }

        // STEP 4.3: Apply relocation
        if (lastImprovementAtStep == 3) {
            break;
        }
        bool improvementFound = true;
        while (improvementFound) {
            improvementFound = false;
            for (size_t i = 1; i < limitRelocationI; ++i) {
                double subRouteLengthCurrentPart = std::sqrt(
                    std::pow(route[i - 1].endX - route[i].startX, 2)
                    + std::pow(route[i - 1].endY - route[i].startY, 2)
                );
                subRouteLengthCurrentPart += std::sqrt(
                    std::pow(route[i].endX - route[i + 1].startX, 2)
                    + std::pow(route[i].endY - route[i + 1].startY, 2)
                );
                double subRouteLengthNewPart = std::sqrt(
                    std::pow(route[i - 1].endX - route[i + 1].startX, 2)
                    + std::pow(route[i - 1].endY - route[i + 1].startY, 2)
                );
                subRouteLengthNewPart += Base::Precision::Confusion();

                // Try relocating backward
                for (size_t j = 0; j + 2 < i; ++j) {
                    double subRouteLengthCurrent = subRouteLengthCurrentPart;
                    subRouteLengthCurrent += std::sqrt(
                        std::pow(route[j].endX - route[j + 1].startX, 2)
                        + std::pow(route[j].endY - route[j + 1].startY, 2)
                    );
                    double subRouteLengthNew = subRouteLengthNewPart;
                    subRouteLengthNew += std::sqrt(
                        std::pow(route[j].endX - route[i].startX, 2)
                        + std::pow(route[j].endY - route[i].startY, 2)
                    );
                    subRouteLengthNew += std::sqrt(
                        std::pow(route[i].endX - route[j + 1].startX, 2)
                        + std::pow(route[i].endY - route[j + 1].startY, 2)
                    );
                    if (subRouteLengthNew < subRouteLengthCurrent) {
                        // Relocate the i-th tunnel backward (after j-th tunnel)
                        TSPTunnel temp = route[i];
                        route.erase(route.begin() + i);
                        route.insert(route.begin() + j + 1, temp);
                        subRouteLengthCurrentPart = std::sqrt(
                            std::pow(route[i - 1].endX - route[i].startX, 2)
                            + std::pow(route[i - 1].endY - route[i].startY, 2)
                        );
                        subRouteLengthCurrentPart += std::sqrt(
                            std::pow(route[i].endX - route[i + 1].startX, 2)
                            + std::pow(route[i].endY - route[i + 1].startY, 2)
                        );
                        subRouteLengthNewPart = std::sqrt(
                            std::pow(route[i - 1].endX - route[i + 1].startX, 2)
                            + std::pow(route[i - 1].endY - route[i + 1].startY, 2)
                        );
                        subRouteLengthNewPart += Base::Precision::Confusion();
                        improvementFound = true;
                        lastImprovementAtStep = 3;
                    }
                }

                // Try relocating forward
                for (size_t j = i + 1; j < limitRelocationJ; ++j) {
                    double subRouteLengthCurrent = subRouteLengthCurrentPart;
                    subRouteLengthCurrent += std::sqrt(
                        std::pow(route[j].endX - route[j + 1].startX, 2)
                        + std::pow(route[j].endY - route[j + 1].startY, 2)
                    );
                    double subRouteLengthNew = subRouteLengthNewPart;
                    subRouteLengthNew += std::sqrt(
                        std::pow(route[j].endX - route[i].startX, 2)
                        + std::pow(route[j].endY - route[i].startY, 2)
                    );
                    subRouteLengthNew += std::sqrt(
                        std::pow(route[i].endX - route[j + 1].startX, 2)
                        + std::pow(route[i].endY - route[j + 1].startY, 2)
                    );
                    if (subRouteLengthNew < subRouteLengthCurrent) {
                        // Relocate the i-th tunnel forward (after j-th tunnel)
                        TSPTunnel temp = route[i];
                        route.erase(route.begin() + i);
                        route.insert(route.begin() + j, temp);
                        subRouteLengthCurrentPart = std::sqrt(
                            std::pow(route[i - 1].endX - route[i].startX, 2)
                            + std::pow(route[i - 1].endY - route[i].startY, 2)
                        );
                        subRouteLengthCurrentPart += std::sqrt(
                            std::pow(route[i].endX - route[i + 1].startX, 2)
                            + std::pow(route[i].endY - route[i + 1].startY, 2)
                        );
                        subRouteLengthNewPart = std::sqrt(
                            std::pow(route[i - 1].endX - route[i + 1].startX, 2)
                            + std::pow(route[i - 1].endY - route[i + 1].startY, 2)
                        );
                        subRouteLengthNewPart += Base::Precision::Confusion();
                        improvementFound = true;
                        lastImprovementAtStep = 3;
                    }
                }
            }
            if (!routeEndPoint) {
                double subRouteLengthCurrentPart = std::sqrt(
                    std::pow(route[route.size() - 2].endX - route[route.size() - 1].startX, 2)
                    + std::pow(route[route.size() - 2].endY - route[route.size() - 1].startY, 2)
                );
                for (size_t j = 0; j + 2 < route.size(); ++j) {
                    double subRouteLengthCurrent = subRouteLengthCurrentPart;
                    subRouteLengthCurrent += std::sqrt(
                        std::pow(route[j].endX - route[j + 1].startX, 2)
                        + std::pow(route[j].endY - route[j + 1].startY, 2)
                    );
                    double subRouteLengthNew = std::sqrt(
                        std::pow(route[j].endX - route[route.size() - 1].startX, 2)
                        + std::pow(route[j].endY - route[route.size() - 1].startY, 2)
                    );
                    subRouteLengthNew += std::sqrt(
                        std::pow(route[route.size() - 1].endX - route[j + 1].startX, 2)
                        + std::pow(route[route.size() - 1].endY - route[j + 1].startY, 2)
                    );
                    subRouteLengthNew += Base::Precision::Confusion();
                    if (subRouteLengthNew < subRouteLengthCurrent) {
                        // Relocate the last tunnel after j-th tunnel
                        TSPTunnel temp = route[route.size() - 1];
                        route.erase(route.begin() + route.size() - 1);
                        route.insert(route.begin() + j + 1, temp);
                        subRouteLengthCurrentPart = std::sqrt(
                            std::pow(route[route.size() - 2].endX - route[route.size() - 1].startX, 2)
                            + std::pow(route[route.size() - 2].endY - route[route.size() - 1].startY, 2)
                        );
                        improvementFound = true;
                        lastImprovementAtStep = 3;
                    }
                }
            }
        }

        if (lastImprovementAtStep == 0) {
            break;  // No additional improvements could be made
        }
    }

    // STEP 5: Delete temporary start and end point
    if (!route.empty()) {
        route.erase(route.begin());  // Remove temp start
    }
    if (routeEndPoint && !route.empty()) {
        route.pop_back();  // Remove temp end
    }

    return route;
}
