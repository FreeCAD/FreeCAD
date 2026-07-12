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


#include <cmath>
#include <list>

#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewParams.h>

#include "MeasureSnapIndicator.h"

using namespace MeasureGui;

namespace
{
// Distinct built-in marker per snap type; nullptr for types this overlay does not draw.
const char* markerName(Measure::MeasureSnapMode type)
{
    switch (type) {
        case Measure::MeasureSnapMode::Vertex:
            return "SQUARE_FILLED";
        case Measure::MeasureSnapMode::Center:
            return "CIRCLE_LINE";
        case Measure::MeasureSnapMode::Midpoint:
            return "DIAMOND_FILLED";
        default:
            return nullptr;
    }
}

// Only a fixed set of bitmap sizes is registered; an unregistered size silently
// falls back to a filled circle, so snap the desired size to the nearest available.
int nearestSupportedSize(const char* name, int desired)
{
    const std::list<int> sizes = Gui::Inventor::MarkerBitmaps::getSupportedSizes(name);
    if (sizes.empty()) {
        return desired;
    }
    int best = sizes.front();
    for (int size : sizes) {
        if (std::abs(size - desired) < std::abs(best - desired)) {
            best = size;
        }
    }
    return best;
}

// Scene-graph root of the active 3D view, or nullptr when there is none.
SoGroup* activeSceneGraph()
{
    auto* guiDoc = Gui::Application::Instance->activeDocument();
    if (!guiDoc) {
        return nullptr;
    }
    auto* view = dynamic_cast<Gui::View3DInventor*>(guiDoc->getActiveView());
    if (!view) {
        return nullptr;
    }
    return static_cast<SoGroup*>(view->getViewer()->getSceneGraph());
}
}  // namespace

MeasureSnapIndicator::MeasureSnapIndicator()
{
    pRoot = new SoAnnotation;
    pRoot->ref();

    pSwitch = new SoSwitch;
    pSwitch->ref();
    pSwitch->whichChild = SO_SWITCH_NONE;

    pSep = new SoSeparator;
    pSep->ref();

    pColor = new SoBaseColor;
    pColor->ref();
    pColor->rgb.setValue(1.0F, 1.0F, 0.0F);

    pCoords = new SoCoordinate3;
    pCoords->ref();

    pMarkerSet = new SoMarkerSet;
    pMarkerSet->ref();

    pSep->addChild(pColor);
    pSep->addChild(pCoords);
    pSep->addChild(pMarkerSet);
    pSwitch->addChild(pSep);
    pRoot->addChild(pSwitch);
}

MeasureSnapIndicator::~MeasureSnapIndicator()
{
    if (attached && pSceneGraph) {
        pSceneGraph->removeChild(pRoot);
    }
    pMarkerSet->unref();
    pCoords->unref();
    pColor->unref();
    pSep->unref();
    pSwitch->unref();
    pRoot->unref();
}

bool MeasureSnapIndicator::attach()
{
    SoGroup* sg = activeSceneGraph();
    if (!sg) {
        return attached;
    }
    if (attached && pSceneGraph == sg) {
        return true;
    }
    // Active view changed: move off the previous (still-alive) view before re-attaching.
    if (attached && pSceneGraph) {
        pSceneGraph->removeChild(pRoot);
    }
    sg->addChild(pRoot);
    pSceneGraph = sg;
    attached = true;
    return true;
}

void MeasureSnapIndicator::show(const std::vector<gp_Pnt>& points, Measure::MeasureSnapMode type)
{
    const char* name = markerName(type);
    if (points.empty() || !name || !attach()) {
        hide();
        return;
    }

    const auto count = static_cast<int>(points.size());
    pCoords->point.setNum(count);
    SbVec3f* verts = pCoords->point.startEditing();
    for (int i = 0; i < count; ++i) {
        const gp_Pnt& p = points[i];
        verts[i].setValue(
            static_cast<float>(p.X()),
            static_cast<float>(p.Y()),
            static_cast<float>(p.Z())
        );
    }
    pCoords->point.finishEditing();

    const int desiredSize = static_cast<int>(Gui::ViewParams::instance()->getMarkerSize());
    const int size = nearestSupportedSize(name, desiredSize);
    pMarkerSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(name, size);
    pMarkerSet->numPoints = count;
    pSwitch->whichChild = SO_SWITCH_ALL;
}

void MeasureSnapIndicator::hide()
{
    pSwitch->whichChild = SO_SWITCH_NONE;
}

void MeasureSnapIndicator::dropHandle()
{
    attached = false;
    pSceneGraph = nullptr;
}
