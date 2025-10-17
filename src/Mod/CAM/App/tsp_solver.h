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

class TSPSolver
{
public:
    // Returns a vector of indices representing the visit order using 2-Opt
    // If startPoint or endPoint are provided, the path will start/end at the closest point to these
    // coordinates
    static std::vector<int> solve(const std::vector<TSPPoint>& points,
                                  const TSPPoint* startPoint = nullptr,
                                  const TSPPoint* endPoint = nullptr);
};
