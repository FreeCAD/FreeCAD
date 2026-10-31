// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Morten Vajhøj
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "TechDrawCosmeticArcHandler.h"

#include <algorithm>
#include <cmath>

#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>

#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGVPage.h"
#include "QGSPage.h"
#include "QGIView.h"
#include "Rez.h"
#include "CommandHelpers.h"


using namespace TechDrawGui;
using namespace TechDraw;

using DU = TechDraw::DrawUtil;


void execDrawCosmArc()
{
    //draw a cosmetic arc of circle
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    if (selection.empty()) {
        return;
    }
    TechDraw::DrawViewPart* objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selection[0].getObject());

    const std::vector<std::string> SubNames = selection[0].getSubNames();
    std::vector<Base::Vector3d> vertexPoints;
    vertexPoints = _getVertexPoints(SubNames, objFeat);
    if (vertexPoints.size() >= 3) {
        // vertexPoints come from stored geometry, so are centered, scaled, rotated and inverted (CSRIz).
        // because the points are inverted, the start and end angles will be mirrored unless we invert the points
        // before calculating the angle.
        Base::Vector3d center = CosmeticVertex::makeCanonicalPoint(objFeat, DU::invertY(vertexPoints[0]));
        Base::Vector3d end1 = CosmeticVertex::makeCanonicalPoint(objFeat, DU::invertY(vertexPoints[1]));
        Base::Vector3d end2 = CosmeticVertex::makeCanonicalPoint(objFeat, DU::invertY(vertexPoints[2]));
        double arcRadius = (end1 - center).Length();
        double angle1 = _getAngle(center, end1);
        double angle2 = _getAngle(center, end2);
        TechDraw::BaseGeomPtr baseGeo = std::make_shared<TechDraw::AOC>(
            center, arcRadius, angle1, angle2);
        TechDraw::AOCPtr aoc = std::static_pointer_cast<TechDraw::AOC>(baseGeo);
        // having done our calculations in sensible coordinates, we convert to inverted coords
        std::string arcTag = objFeat->addCosmeticEdge(baseGeo->inverted());
        TechDraw::CosmeticEdge* arcEdge = objFeat->getCosmeticEdge(arcTag);
        _setLineAttributes(arcEdge);
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
    }
}

void TechDrawCosmeticArcHandler::reset() {
    previewObjFeat->refreshCEGeoms();
    previewObjFeat->requestPaint();
    previewTag.clear();
    previewView = nullptr;
    previewObjFeat = nullptr;
    previewCenter = Base::Vector3d();
    previewEnd1 = Base::Vector3d();
    previewRadius = 0.0;
    previewSweep = 0.0;
    previewLastAngle = 0.0;
}

void TechDrawCosmeticArcHandler::updatePreview(Base::Vector3d arcPoint) {
    double angle1 = _getAngle(previewCenter, previewEnd1);
    double angle2 = _getAngle(previewCenter, arcPoint);

    double delta = angle2 - previewLastAngle;
    if (delta > 180) {
        delta -= 360;
    } 
    else if (delta < -180) {
        delta += 360;
    }
    previewSweep = std::clamp(previewSweep + delta, -359.9, 359.9);
    previewLastAngle = angle2;

    double startAngle = angle1;
    double endAngle = angle1 + previewSweep;
    if (previewSweep < 0) {
        std::swap(startAngle, endAngle);
    }

    TechDraw::BaseGeomPtr baseGeo = std::make_shared<TechDraw::AOC>(
            previewCenter, previewRadius, startAngle, endAngle);

    TechDraw::CosmeticEdge* arcEdge = previewObjFeat->getCosmeticEdge(previewTag);
    arcEdge->m_geometry = baseGeo->inverted();

    previewObjFeat->refreshCEGeoms();
    previewObjFeat->requestPaint();
}

void TechDrawCosmeticArcHandler::activated()
{
    if (viewPage) {
        QPoint hotspot(15, 15);
        QPixmap pixmap = viewPage->prepareCursorPixmap("TechDraw_CosmeticArc_Pointer.svg", hotspot);
        cursor = QCursor(pixmap, hotspot.x(), hotspot.y());
        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }

    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::GreedySelection);
}

void TechDrawCosmeticArcHandler::deactivated()
{
    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::NormalSelection);
    if (viewPage) {
        if (auto vp = viewPage->viewport()) {
            vp->unsetCursor();
        }
    }
}

std::list<Gui::InputHint> TechDrawCosmeticArcHandler::getToolHints() const
{
    return {};
}

void TechDrawCosmeticArcHandler::mouseMoveEvent(QMouseEvent* event) {
    if (auto vp = viewPage->viewport()) {
        vp->setCursor(cursor);
    }
    if (!previewView || !previewObjFeat) {
        return;
    }
    QPointF localPos = previewView->mapFromScene(viewPage->mapToScene(event->pos()));
    Base::Vector3d arcPoint(Rez::appX(localPos.x()) / previewObjFeat->getScale(),
                             -Rez::appX(localPos.y()) / previewObjFeat->getScale(),
                             0.0);
    
    updatePreview(arcPoint);
}

void TechDrawCosmeticArcHandler::mouseReleaseEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    std::vector<Gui::SelectionObject> selObjs = Gui::Selection().getSelectionEx();
    if (selObjs.empty() && !previewTag.empty()) {
        reset();
        return;
    }
    if (selObjs.empty()) {
        return;
    }

    int vertexCount = 0;
    std::vector<Base::Vector3d> vertexPoints;
    
    for (auto& selObj : selObjs) {

        auto* selObjFeat = dynamic_cast<TechDraw::DrawViewPart*>(selObj.getObject());
        const std::vector<std::string>& subNames = selObj.getSubNames();
        for (auto& subName : subNames) {
            if (subName.empty()) {
                continue;
            }

            std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(subName);
            int geoId = TechDraw::DrawUtil::getIndexFromName(subName);

            if ((geomType == "Edge" && !previewTag.empty()) || (geomType == "Face" && !previewTag.empty())) {
                reset();
                return;
            } 
            if (geomType == "Vertex" && selObjFeat && selObjFeat->getProjVertexByIndex(geoId)) {
                vertexCount++;
                vertexPoints.push_back(selObjFeat->getProjVertexByIndex(geoId)->point());
            }
        }
    }

    if (!previewTag.empty()) {
        Base::Vector3d arcPoint = CosmeticVertex::makeCanonicalPoint(previewObjFeat, DU::invertY(vertexPoints.back()));
        
        updatePreview(arcPoint);
        reset();
        return;
    }

    if (vertexCount != 2) {
        return;
    }

    vertexPoints.push_back(vertexPoints[1] + Base::Vector3d(1.0, 0.0, 0.0)); // This will be the cursor position

    TechDraw::DrawViewPart* objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selObjs[0].getObject());

    Base::Vector3d center = CosmeticVertex::makeCanonicalPoint(objFeat, DU::invertY(vertexPoints[0]));
    Base::Vector3d end1 = CosmeticVertex::makeCanonicalPoint(objFeat, DU::invertY(vertexPoints[1]));
    Base::Vector3d end2 = CosmeticVertex::makeCanonicalPoint(objFeat, DU::invertY(vertexPoints[2]));
    double arcRadius = (end1 - center).Length();
    double angle1 = _getAngle(center, end1);
    double angle2 = _getAngle(center, end2);
    TechDraw::BaseGeomPtr baseGeo = std::make_shared<TechDraw::AOC>(
        center, arcRadius, angle1, angle2);
    TechDraw::AOCPtr aoc = std::static_pointer_cast<TechDraw::AOC>(baseGeo);
    // having done our calculations in sensible coordinates, we convert to inverted coords
    previewTag = objFeat->addCosmeticEdge(baseGeo->inverted());
    TechDraw::CosmeticEdge* arcEdge = objFeat->getCosmeticEdge(previewTag);
    _setLineAttributes(arcEdge);
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();

    previewObjFeat = objFeat;
    previewView = viewPage->getScene()->getQGIVByName(objFeat->getNameInDocument());
    previewCenter = center;
    previewRadius = arcRadius;
    previewEnd1 = end1;
    previewSweep = 0.0;
    previewLastAngle = angle1;
}

void TechDrawCosmeticArcHandler::mousePressEvent(QMouseEvent* event) {
    Q_UNUSED(event);
}
void TechDrawCosmeticArcHandler::keyPressEvent(QKeyEvent* event) {
    Q_UNUSED(event);
}

void TechDrawCosmeticArcHandler::addPreselected() {
    execDrawCosmArc();
}