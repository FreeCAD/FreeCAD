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


#include "TechDrawCosmeticLinesHandler.h"

#include <QMouseEvent>
#include <QKeyEvent>

#include <Gui/Control.h>
#include <Gui/Application.h>

#include "QGVPage.h"
#include "QGSPage.h"
#include "QGIEdge.h"
#include "QGIVertex.h"
#include "QGIFace.h"
#include "QGIView.h"
#include "Rez.h"
#include "ViewProviderDrawingView.h"
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/InputHint.h>


using namespace TechDrawGui;
using namespace TechDraw;

void TechDrawCosmeticLinesHandler::activated()
{
    if (viewPage) {
        QPoint hotspot(15, 15);
        QPixmap pixmap = viewPage->prepareCursorPixmap("actions/TechDraw_CosmeticLine_Pointer.svg", hotspot);
        cursor = QCursor(pixmap, hotspot.x(), hotspot.y());
        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }
}

void TechDrawCosmeticLinesHandler::deactivated()
{
    if (viewPage) {
        if (auto vp = viewPage->viewport()) {
            vp->unsetCursor();
        }
    }

}

std::list<Gui::InputHint> TechDrawCosmeticLinesHandler::getToolHints() const
{
    if (currentState == State::Parallel) {
        return {
            {QObject::tr("%1 pick line location", "TechDraw CosmeticLine: hint"),
                {Gui::InputHint::UserInput::MouseLeft}},
            {QObject::tr("%1 change to perpendicular line", "TechDraw CosmeticLine: hint"),
                {Gui::InputHint::UserInput::KeyM}},
        };
    }
    else if (currentState == State::Perpendicular) {
        return {
            {QObject::tr("%1 pick line location", "TechDraw CosmeticLine: hint"),
                {Gui::InputHint::UserInput::MouseLeft}},
            {QObject::tr("%1 change to parallel line", "TechDraw CosmeticLine: hint"),
                {Gui::InputHint::UserInput::KeyM}},
        };
    }

    return {};
}

void TechDrawCosmeticLinesHandler::mouseMoveEvent(QMouseEvent* event)
{
    if (auto vp = viewPage->viewport()) {
        vp->setCursor(cursor);
    }

    if (!previewView || !previewViewPart) {
        return;
    }
    if (currentState == State::Free && !firstPointSet) {
        return;
    }
    if (currentState == State::Parallel && !edgePointsSet) {
        return;
    }
    if (currentState == State::Perpendicular && !edgePointsSet) {
        return;
    }

    QPointF localPos = previewView->mapFromScene(viewPage->mapToScene(event->pos()));
    Base::Vector3d cursorPoint = Base::Vector3d(Rez::appX(localPos.x()) / previewViewPart->getScale(),
                                                Rez::appX(localPos.y()) / previewViewPart->getScale(),
                                                0.0);

    if (cursorPoint == firstPoint) {
        return;
    }

    if (currentState == State::Free) {
        if (previewLine.empty()) {
            previewLine = previewViewPart->addCosmeticEdge(firstPoint, cursorPoint);
        }
    
        CosmeticEdge* ce = previewViewPart->getCosmeticEdge(previewLine);
        ce->setPoints(firstPoint, cursorPoint);
        previewViewPart->refreshCEGeoms();
        previewViewPart->requestPaint();
    }
    else if (currentState == State::Parallel) {
        Base::Vector3d dir = edgePoint2 - edgePoint1;
        dir.Normalize();

        Base::Vector3d tocursor = cursorPoint - edgePoint1;
        Base::Vector3d offset = tocursor - dir * tocursor.Dot(dir);

        Base::Vector3d start = edgePoint1 + offset;
        Base::Vector3d end = edgePoint2 + offset;
    
        if (previewLine.empty()) {
            previewLine = previewViewPart->addCosmeticEdge(start, end);
        }
    
        CosmeticEdge* ce = previewViewPart->getCosmeticEdge(previewLine);
        ce->setPoints(start, end);
        previewViewPart->refreshCEGeoms();
        previewViewPart->requestPaint();
    }
    else if (currentState == State::Perpendicular) {
        Base::Vector3d dir = edgePoint2 - edgePoint1;
        dir.Normalize();
        Base::Vector3d perp(-dir.y, dir.x, 0.0);

        double t = (cursorPoint - edgePoint1).Dot(dir);
        Base::Vector3d foot = edgePoint1 + dir * t;
        double len = (cursorPoint - edgePoint1).Dot(perp);
        Base::Vector3d end = foot + perp * len;

        if (previewLine.empty()) {
            previewLine = previewViewPart->addCosmeticEdge(foot, end);
        }

        CosmeticEdge* ce = previewViewPart->getCosmeticEdge(previewLine);
        ce->setPoints(foot, end);
        previewViewPart->refreshCEGeoms();
        previewViewPart->requestPaint();
    }
}

void TechDrawCosmeticLinesHandler::mouseReleaseEvent(QMouseEvent* event)
{
    TechDraw::DrawViewPart* objFeat = nullptr;
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    if (!selection.empty()) {
        objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selection[0].getObject());
    }
    if (!objFeat && previewViewPart) {
        objFeat = previewViewPart;
    }
    if (!objFeat) {
        return;
    }

    QGIView* qgiv = viewPage->getScene()->getQGIVByName(objFeat->getNameInDocument());
    if (!qgiv) {
        return;
    }

    QPointF localPos = qgiv->mapFromScene(viewPage->mapToScene(event->pos()));
    Base::Vector3d clickPos(Rez::appX(localPos.x()) / objFeat->getScale(),
                            Rez::appX(localPos.y()) / objFeat->getScale(),
                            0.0);

    if (!selection.empty()) {
        for (auto& subName : selection[0].getSubNames()) {
            std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(subName);
            if (geomType == "Vertex") {
                clickPos = objFeat->getVertex(subName)->point();
            }
            if (geomType == "Edge" && !previewViewPart) {
                int GeoId = TechDraw::DrawUtil::getIndexFromName(subName);
                TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);

                if (geom->getGeomType() != GeomType::GENERIC) {
                    continue;
                }

                currentState = State::Parallel;
                updateHint();
                edgePoint1 = geom->getStartPoint();
                edgePoint2 = geom->getEndPoint();
                edgePointsSet = true;
                previewView = qgiv;
                previewViewPart = objFeat;
                return;
            }
        }
    }

    if (currentState == State::Free) {
        if (!firstPointSet) {
            firstPoint = clickPos;
            firstPointSet = true;
            previewView = qgiv;
            previewViewPart = objFeat;
            return;
        }

        objFeat->addCosmeticVertex(firstPoint, false);
        objFeat->addCosmeticVertex(clickPos, false);

        CosmeticEdge* ce = previewViewPart->getCosmeticEdge(previewLine);
        ce->setPoints(firstPoint, clickPos);

        objFeat->refreshCEGeoms();
        objFeat->refreshCVGeoms();
        objFeat->requestPaint();

        firstPoint = Base::Vector3d();
        firstPointSet = false;
        previewLine.clear();
        previewView = nullptr;
        previewViewPart = nullptr;
    }
    else if (currentState == State::Parallel) {

        CosmeticEdge* ce = previewViewPart->getCosmeticEdge(previewLine);
        Base::Vector3d start = ce->getStartPoint();
        Base::Vector3d end = ce->getEndPoint();

        objFeat->addCosmeticVertex(start, false);
        objFeat->addCosmeticVertex(end, false);

        previewViewPart->refreshCEGeoms();
        previewViewPart->refreshCVGeoms();
        previewViewPart->requestPaint();

        previewLine.clear();
        previewView = nullptr;
        previewViewPart = nullptr;
        edgePointsSet = false;
        currentState = State::Free;
    }
    else if (currentState == State::Perpendicular) {
        CosmeticEdge* ce = previewViewPart->getCosmeticEdge(previewLine);
        Base::Vector3d start = ce->getStartPoint();
        Base::Vector3d end = ce->getEndPoint();

        objFeat->addCosmeticVertex(start, false);
        objFeat->addCosmeticVertex(end, false);

        previewViewPart->refreshCEGeoms();
        previewViewPart->refreshCVGeoms();
        previewViewPart->requestPaint();

        previewLine.clear();
        previewView = nullptr;
        previewViewPart = nullptr;
        edgePointsSet = false;
        currentState = State::Free;
    }
    updateHint();
}

void TechDrawCosmeticLinesHandler::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawCosmeticLinesHandler::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_M) {
        if (currentState != State::Parallel && currentState != State::Perpendicular) {
            return;
        }
        if (currentState == State::Parallel) {
            currentState = State::Perpendicular;
            updateHint();
        }
        else if (currentState == State::Perpendicular) {
            currentState = State::Parallel;
            updateHint();
        }
    }
}

void TechDrawCosmeticLinesHandler::addPreselected() {
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    if (selection.empty()) {
        return;
    }

    TechDraw::DrawViewPart* objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selection[0].getObject());

    int vertexAmount = 0;
    std::vector<std::string> vertexNames;
    int edgeAmount = 0;
    std::vector<std::string> edgeNames;
    for (auto& sel : selection) {
        for (auto& subName : sel.getSubNames()) {
            std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(subName);
            if (geomType == "Vertex") {
                vertexAmount++;
                vertexNames.push_back(subName);
            }
            if (geomType == "Edge") {
                edgeAmount++;
                edgeNames.push_back(subName);
            }
        }
    }

    if (edgeAmount >= 2 || vertexAmount >= 3) {
        return;
    }

    if (vertexAmount == 2) {
        Base::Vector3d v1 = objFeat->getVertex(vertexNames[0])->point();
        Base::Vector3d v2 = objFeat->getVertex(vertexNames[1])->point();

        objFeat->addCosmeticEdge(v1, v2);
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
        return;
    }
    else if (edgeAmount == 1 && vertexAmount == 1) {
        int GeoId = TechDraw::DrawUtil::getIndexFromName(edgeNames[0]);
        TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);

        if (geom->getGeomType() != GeomType::GENERIC) {
            return;
        }

        Base::Vector3d edgeStart = geom->getStartPoint();
        Base::Vector3d edgeEnd = geom->getEndPoint();
        Base::Vector3d vertexPoint = objFeat->getVertex(vertexNames[0])->point();

        Base::Vector3d dir = edgeEnd - edgeStart;
        dir.Normalize();

        Base::Vector3d toVertex = vertexPoint - edgeStart;
        Base::Vector3d offset = toVertex - dir * toVertex.Dot(dir);

        Base::Vector3d start = edgeStart + offset;
        Base::Vector3d end = edgeEnd + offset;
    
    
        objFeat->addCosmeticEdge(start, end);
        objFeat->addCosmeticVertex(start, false);
        objFeat->addCosmeticVertex(end, false);
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
    }
}