// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Shai Seger <shaise at gmail>                       *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "SimShapes.h"
#include <vector>

#define PROFILE_BUFFER_POINTS(npoints) ((npoints) * 2 - 1)
#define PROFILE_BUFFER_SIZE(npoints) (PROFILE_BUFFER_POINTS(npoints) * 2)
#define MILL_HEIGHT 10

namespace MillSim
{
class EndMill
{
public:
    std::vector<float> profilePoints;
    float radius;
    int nPoints = 0;
    int toolId = -1;

    Shape pathShape;
    Shape halfToolShape;
    Shape toolShape;

public:
    EndMill(int toolid, float diameter);
    EndMill(const std::vector<float>& toolProfile, int toolid, float diameter);
    virtual ~EndMill();
    void GenerateDisplayLists(float quality);
    unsigned int GenerateArcSegmentDL(float radius, float angleRad, float zShift, Shape* retShape);

protected:
    void MirrorPointBuffer();
};
}  // namespace MillSim
