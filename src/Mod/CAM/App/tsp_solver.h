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

#pragma once
#include <vector>
#include <utility>
#include <limits>
#include <cmath>

struct TSPPoint
{
    double x, y;
    TSPPoint(double x_, double y_)
        : x(x_)
        , y(y_)
    {}
};

struct TSPTunnel
{
    double startX, startY;
    double endX, endY;
    bool isOpen;   // Whether the tunnel can be flipped (entry/exit can be swapped)
    bool flipped;  // Tracks if tunnel has been flipped from original orientation
    int index;     // Original index in input array

    TSPTunnel(double sx, double sy, double ex, double ey, bool open = true)
        : startX(sx)
        , startY(sy)
        , endX(ex)
        , endY(ey)
        , isOpen(open)
        , flipped(false)
        , index(-1)
    {}
};

class TSPSolver
{
public:
    // Returns a vector of indices representing the visit order using 2-Opt
    // If startPoint or endPoint are provided, the path will start/end at the closest point to these
    // coordinates
    static std::vector<int> solve(
        const std::vector<TSPPoint>& points,
        const TSPPoint* startPoint = nullptr,
        const TSPPoint* endPoint = nullptr
    );

    // Solves TSP for tunnels (path segments with entry/exit points)
    // allowFlipping: whether tunnels can be reversed (entry becomes exit)
    // Returns vector of tunnels in optimized order (tunnels may be flipped)
    static std::vector<TSPTunnel> solveTunnels(
        std::vector<TSPTunnel> tunnels,
        bool allowFlipping = false,
        const TSPPoint* routeStartPoint = nullptr,
        const TSPPoint* routeEndPoint = nullptr
    );
};
