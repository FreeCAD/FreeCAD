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

#include "TechDrawCosmeticCircleHandler.h"

#include <algorithm>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainterPath>
#include <QPointF>
#include <QGraphicsItem>
#include <vector>
#include <string>
#include <cmath>

#include <Base/Console.h>
#include <Base/Precision.h>

#include <Gui/MainWindow.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Control.h>

#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/CosmeticVertex.h>

#include "DrawGuiUtil.h"
#include "QGIEdge.h"
#include "QGIVertex.h"
#include "QGIView.h"
#include "QGSPage.h"
#include "QGVPage.h"
#include "CommandHelpers.h"
#include "Rez.h"
#include "TaskCosmeticCircle.h"


using namespace TechDrawGui;
using namespace TechDraw;


void execCosmeticCircleCenter()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    TechDraw::DrawViewPart* baseFeat = nullptr;
    std::vector<std::string> subNames2D;
    std::vector< std::pair<Part::Feature*, std::string> > objs3D;
    if (selection.empty()) {
        return;
    }

    for (auto& so: selection) {
        if (so.getObject()->isDerivedFrom<TechDraw::DrawViewPart>()) {
            baseFeat = static_cast<TechDraw::DrawViewPart*> (so.getObject());
            subNames2D = so.getSubNames();
        } else if (so.getObject()->isDerivedFrom<Part::Feature>()) {
            std::vector<std::string> subNames3D = so.getSubNames();
            for (auto& sub3D: subNames3D) {
                std::pair<Part::Feature*, std::string> temp;
                temp.first = static_cast<Part::Feature*>(so.getObject());
                temp.second = sub3D;
                objs3D.push_back(temp);
            }
        } else {
            //garbage
        }
    }

    if (!baseFeat) {
        return;
    }

    std::vector<std::string> edgeNames;
    std::vector<std::string> vertexNames;
    for (auto& s: subNames2D) {
        std::string geomType = DrawUtil::getGeomTypeFromName(s);
        if (geomType == "Vertex") {
            vertexNames.push_back(s);
        } else if (geomType == "Edge") {
            edgeNames.push_back(s);
        }
    }

    //check if editing existing edge
    if (!edgeNames.empty() && (edgeNames.size() == 1)) {
        TechDraw::CosmeticEdge* ce = baseFeat->getCosmeticEdgeBySelection(edgeNames.front());
        if (!ce
            || !(ce->m_geometry->getGeomType() == GeomType::CIRCLE
                || ce->m_geometry->getGeomType() == GeomType::ARCOFCIRCLE)) {
            return;
        }

        Gui::Control().showDialog(new TaskDlgCosmeticCircle(baseFeat,
                                                          edgeNames.front()));
        return;
    }

    std::vector<Base::Vector3d> points;
    std::vector<bool> is3d;
    //get the 2D points
    if (!vertexNames.empty()) {
        for (auto& v2d: vertexNames) {
            int idx = DrawUtil::getIndexFromName(v2d);
            TechDraw::VertexPtr v = baseFeat->getProjVertexByIndex(idx);
            if (v) {
                points.push_back(v->point());
                is3d.push_back(false);
            }
        }
    }
    //get the 3D points
    if (!objs3D.empty()) {
        for (auto& o3D: objs3D) {
            int idx = DrawUtil::getIndexFromName(o3D.second);
            Part::TopoShape s = o3D.first->Shape.getShape();
            TopoDS_Vertex v = TopoDS::Vertex(s.getSubShape(TopAbs_VERTEX, idx));
            Base::Vector3d p = DrawUtil::vertex2Vector(v);
            points.push_back(p);
            is3d.push_back(true);
        }
    }

    if (points.empty()) {
        return;
    }

    bool centerIs3d = false;
    if (!is3d.empty()) {
        centerIs3d = is3d.front();
    }

    Gui::Control().showDialog(new TaskDlgCosmeticCircle(baseFeat,
                                                      points,
                                                      centerIs3d));
}

void execDrawCosmCircle3Points(TechDraw::DrawViewPart* objFeat, const std::vector<std::string>& SubNames)
{
    if (!objFeat) {
        return;
    }

    std::vector<Base::Vector3d> vertexPoints = _getVertexPoints(SubNames, objFeat);
    if (vertexPoints.size() >= 3) {
        Base::Vector3d circleCenter = _circleCenter(vertexPoints[0],
                                                    vertexPoints[1],
                                                    vertexPoints[2]);
        double circleRadius = (vertexPoints[0] - circleCenter).Length() / objFeat->getScale();
        circleCenter = CosmeticVertex::makeCanonicalPointInverted(objFeat, circleCenter);
        TechDraw::BaseGeomPtr theCircle =
            std::make_shared<TechDraw::Circle>(circleCenter, circleRadius);
        std::string circleTag = objFeat->addCosmeticEdge(theCircle);
        TechDraw::CosmeticEdge* circleEdge = objFeat->getCosmeticEdge(circleTag);
        _setLineAttributes(circleEdge);

        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
    }
}

void execDrawCosmCircle(TechDraw::DrawViewPart* objFeat, const std::vector<std::string>& SubNames)
{
    if (!objFeat) {
        return;
    }

    std::vector<Base::Vector3d> vertexPoints = _getVertexPoints(SubNames, objFeat);
    if (vertexPoints.size() >= 2) {
        double circleRadius = (vertexPoints[1] - vertexPoints[0]).Length() / objFeat->getScale();
        auto center = CosmeticVertex::makeCanonicalPointInverted(objFeat, vertexPoints[0]);
        TechDraw::BaseGeomPtr baseGeo =
            std::make_shared<TechDraw::Circle>(center, circleRadius);
        std::string circleTag = objFeat->addCosmeticEdge(baseGeo);
        TechDraw::CosmeticEdge* circleEdge = objFeat->getCosmeticEdge(circleTag);
        _setLineAttributes(circleEdge);

        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
    }
}


std::list<Gui::InputHint> TechDrawCosmeticCircleHandler::getToolHints() const
{
    return {};
}

void TechDrawCosmeticCircleHandler::activated()
{
    if (viewPage) {
        QPoint hotspot(15, 15);
        QPixmap pixmap = viewPage->prepareCursorPixmap("actions/TechDraw_CosmeticCircle_Pointer.svg", hotspot);
        QCursor cursor(pixmap, hotspot.x(), hotspot.y());
        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }

    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::GreedySelection);
}

void TechDrawCosmeticCircleHandler::deactivated()
{
    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::NormalSelection);
    if (viewPage) {
        if (auto vp = viewPage->viewport()) {
            vp->unsetCursor();
        }
    }
}

void TechDrawCosmeticCircleHandler::mouseMoveEvent(QMouseEvent* event)
{
    if (previewTag.empty() || !previewView) {
        return;
    }

    Base::Vector3d center = centerPoint;
    QPointF localPos = previewView->mapFromScene(viewPage->mapToScene(event->pos()));
    Base::Vector3d edgePoint(Rez::appX(localPos.x()) / previewObjFeat->getScale(),
                             Rez::appX(localPos.y()) / previewObjFeat->getScale(),
                             0.0);

    double circleRadius = (edgePoint - center).Length() / previewObjFeat->getScale();

    if (circleRadius < Base::Precision::Confusion()) {
        return;
    }
    Base::Vector3d canonicalCenter = CosmeticVertex::makeCanonicalPointInverted(previewObjFeat, center);

    TechDraw::BaseGeomPtr baseGeo =
            std::make_shared<TechDraw::Circle>(canonicalCenter, circleRadius);

    TechDraw::CosmeticEdge* circleEdge = previewObjFeat->getCosmeticEdge(previewTag);

    circleEdge->m_geometry = baseGeo;

    previewObjFeat->refreshCEGeoms();
    previewObjFeat->requestPaint();
}

void TechDrawCosmeticCircleHandler::mouseReleaseEvent(QMouseEvent* event)
{
    if (Gui::Control().activeDialog()) {
        return;
    }

    QPointF scenePos = viewPage->mapToScene(event->pos());
    QPainterPath circle;
    circle.addEllipse(scenePos, 5.0, 5.0);
    const auto items = viewPage->scene()->items(circle);

    std::string subName;
    TechDraw::DrawViewPart* objFeat = nullptr;

    for (QGraphicsItem* item : items) {
        auto* vertex = dynamic_cast<QGIVertex*>(item);
        if (!vertex) {
            continue;
        }

        auto* parent = dynamic_cast<QGIView*>(vertex->parentItem());
        if (!parent) {
            continue;
        }
        objFeat = dynamic_cast<TechDraw::DrawViewPart*>(parent->getViewObject());
        if (!objFeat) {
            continue;
        }

        subName = "Vertex" + std::to_string(vertex->getProjIndex());
        break;
    }

    if (subName.empty()) {
        if (!previewTag.empty()) {
            previewTag.clear();
            previewObjFeat->refreshCEGeoms();
            previewObjFeat->requestPaint();
            previewObjFeat = nullptr;
            previewView = nullptr;
            centerPoint = Base::Vector3d();
        }

        pickedVertices.clear();
        pickedObjFeat = nullptr;
        return;
    }

    pickedObjFeat = objFeat;

    bool alreadyPicked = std::find(pickedVertices.begin(), pickedVertices.end(), subName) != pickedVertices.end();

    if (alreadyPicked) {
        if (!previewTag.empty()) {
            previewTag.clear();
            previewObjFeat->refreshCEGeoms();
            previewObjFeat->requestPaint();
            previewObjFeat = nullptr;
            previewView = nullptr;
            centerPoint = Base::Vector3d();
        }
        pickedVertices.clear();
        pickedObjFeat = nullptr;
        return;
    }

    pickedVertices.push_back(subName);

    if (pickedVertices.size() == 1) {
        std::vector<Base::Vector3d> points = _getVertexPoints(pickedVertices, objFeat);
        if (points.empty()) {
            pickedVertices.clear();
            pickedObjFeat = nullptr;
            return;
        }

        QGIView* qgiv = viewPage->getScene()->getQGIVByName(objFeat->getNameInDocument());
        if (!qgiv) {
            pickedVertices.clear();
            pickedObjFeat = nullptr;
            return;
        }

        Base::Vector3d center = points[0];
        Base::Vector3d edgePoint = points[0] + Base::Vector3d(1, 0, 0);

        double circleRadius = (edgePoint - center).Length() / objFeat->getScale();
        Base::Vector3d canonicalCenter = CosmeticVertex::makeCanonicalPointInverted(objFeat, center);
        TechDraw::BaseGeomPtr baseGeo =
            std::make_shared<TechDraw::Circle>(canonicalCenter, circleRadius);
        previewTag = objFeat->addCosmeticEdge(baseGeo);
        TechDraw::CosmeticEdge* circleEdge = objFeat->getCosmeticEdge(previewTag);
        if (!circleEdge) {
            previewTag.clear();
            pickedVertices.clear();
            pickedObjFeat = nullptr;
            return;
        }
        _setLineAttributes(circleEdge);
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();

        centerPoint = points[0];
        previewObjFeat = objFeat;
        previewView = qgiv;
    }
    else if (pickedVertices.size() == 2) {
        if (!previewTag.empty()) {
            previewObjFeat->removeCosmeticEdge(previewTag);
            previewObjFeat->refreshCEGeoms();
            previewObjFeat->requestPaint();
            previewTag.clear();
            previewObjFeat = nullptr;
            previewView = nullptr;
            centerPoint = Base::Vector3d();
            execDrawCosmCircle(pickedObjFeat, pickedVertices);
            return;
        }
        execDrawCosmCircle(pickedObjFeat, pickedVertices);
        pickedVertices.clear();
        pickedObjFeat = nullptr;
    }
}

void TechDrawCosmeticCircleHandler::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawCosmeticCircleHandler::keyPressEvent(QKeyEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawCosmeticCircleHandler::addPreselected()
{
    if (Gui::Control().activeDialog()) {
        return;
    }

    std::vector<Gui::SelectionObject> selObjs = Gui::Selection().getSelectionEx();
    if (selObjs.empty()) {
        return;
    }

    TechDraw::DrawViewPart* objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selObjs[0].getObject());

    int vertexCount = 0;
    int edgeCount = 0;
    std::vector<std::string> vertexNames;
    for (auto& selObj : selObjs) {

        auto* selObjFeat = dynamic_cast<TechDraw::DrawViewPart*>(selObj.getObject());
        const std::vector<std::string>& subNames = selObj.getSubNames();
        for (auto& subName : subNames) {
            if (subName.empty()) {
                continue;
            }

            std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(subName);
            int geoId = TechDraw::DrawUtil::getIndexFromName(subName);

            if (geomType == "Edge")
            {
                edgeCount++;
            }

            else if (geomType == "Vertex" && selObjFeat && selObjFeat->getProjVertexByIndex(geoId)) {
                vertexCount++;
                vertexNames.push_back(subName);
            }
        }
    }

    if (edgeCount == 1 && vertexCount == 0) {
        execCosmeticCircleCenter();
    }
    else if (vertexCount == 1) {
        execCosmeticCircleCenter();
    }
    else if (vertexCount == 2 && objFeat) {
        execDrawCosmCircle(objFeat, vertexNames);
    }
}

std::list<Gui::InputHint> TechDrawCosmetic3PtCircleHandler::getToolHints() const
{
    return {};
}

void TechDrawCosmetic3PtCircleHandler::activated()
{
    if (viewPage) {
        QPoint hotspot(15, 15);
        QPixmap pixmap = viewPage->prepareCursorPixmap("TechDraw_Cosmetic3PtCircle_Pointer.svg", hotspot);
        QCursor cursor(pixmap, hotspot.x(), hotspot.y());
        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }

    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::GreedySelection);
}

void TechDrawCosmetic3PtCircleHandler::deactivated()
{
    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::NormalSelection);
    if (viewPage) {
        if (auto vp = viewPage->viewport()) {
            vp->unsetCursor();
        }
    }
}

void TechDrawCosmetic3PtCircleHandler::mouseMoveEvent(QMouseEvent* event) {
    Q_UNUSED(event);
}

void TechDrawCosmetic3PtCircleHandler::mouseReleaseEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    std::vector<Gui::SelectionObject> selObjs = Gui::Selection().getSelectionEx();
    if (selObjs.empty()) {
        return;
    }

    int vertexCount = 0;
    
    for (auto& selObj : selObjs) {

        auto* selObjFeat = dynamic_cast<TechDraw::DrawViewPart*>(selObj.getObject());
        const std::vector<std::string>& subNames = selObj.getSubNames();
        for (auto& subName : subNames) {
            if (subName.empty()) {
                continue;
            }

            std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(subName);
            int geoId = TechDraw::DrawUtil::getIndexFromName(subName);

            if (geomType == "Vertex" && selObjFeat && selObjFeat->getProjVertexByIndex(geoId)) {
                vertexCount++;
            }
        }
    }

    if (vertexCount == 3) {
        addPreselected();
    }
}

void TechDrawCosmetic3PtCircleHandler::mousePressEvent(QMouseEvent* event) {
    Q_UNUSED(event);
}
void TechDrawCosmetic3PtCircleHandler::keyPressEvent(QKeyEvent* event) {
    Q_UNUSED(event);
}

void TechDrawCosmetic3PtCircleHandler::addPreselected() {

    std::vector<Gui::SelectionObject> selObjs = Gui::Selection().getSelectionEx();
    
    if (selObjs.empty()) {
        return;
    }

    TechDraw::DrawViewPart* objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selObjs[0].getObject());

    if (!objFeat) {
        return;
    }

    std::vector<std::string> subNames = selObjs[0].getSubNames();

    execDrawCosmCircle3Points(objFeat, subNames);
}