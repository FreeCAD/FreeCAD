// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Krrish777 <777krrish[at]gmail.com>                 *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#pragma once

#include <vector>

#include <gp_Pnt.hxx>

#include <Mod/Measure/App/MeasureSnap.h>

class SoAnnotation;
class SoSwitch;
class SoSeparator;
class SoBaseColor;
class SoCoordinate3;
class SoMarkerSet;
class SoDrawStyle;
class SoLineSet;
class SoGroup;


namespace MeasureGui
{

// On-top overlay for the snap preview: point markers or a dashed axis line. The subtree
// is built once and mutated per hover; the SoSwitch selects one branch, never re-parents.
class MeasureSnapIndicator
{
public:
    MeasureSnapIndicator();
    ~MeasureSnapIndicator();

    // Preview the resolved snap: markers for point types, a dashed line through the two
    // Axis endpoints. Empty points or an unpreviewable type hides.
    void show(const std::vector<gp_Pnt>& points, Measure::MeasureSnapMode type);
    void hide();

    // Forget the scene-graph handle without touching it, for when the owning view is
    // destroyed first (removeChild on a freed viewer graph would be a use-after-free).
    void dropHandle();

private:
    bool attach();

    SoAnnotation* pRoot;
    SoSwitch* pSwitch;
    SoSeparator* pSep;
    SoBaseColor* pColor;
    SoCoordinate3* pCoords;
    SoMarkerSet* pMarkerSet;

    SoSeparator* pAxisSep;
    SoBaseColor* pAxisColor;
    SoDrawStyle* pAxisStyle;
    SoCoordinate3* pAxisCoords;
    SoLineSet* pAxisLine;

    SoGroup* pSceneGraph = nullptr;
    bool attached = false;
};

}  // namespace MeasureGui
