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

#include "PreCompiled.h"

#include "TechDrawVertexHandler.h"

#include <Gui/Control.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/Selection/Selection.h>

#include <Mod/TechDraw/App/CosmeticVertex.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>

#include <Base/Tools.h>
#include <Base/BaseClass.h>

#include <App/Document.h>
#include <App/DocumentObject.h>


#include "TaskAddOffsetVertex.h"
#include "QGIEdge.h"
#include "QGIVertex.h"
#include "QGIView.h"
#include "QGVPage.h"
#include "DrawGuiUtil.h"
#include "Rez.h"
#include "ViewProviderDrawingView.h"

using namespace TechDrawGui;
using namespace TechDraw;

std::string AddVertex(const QPointF& point, DrawViewPart* viewPart)
{
    auto* vpdv = dynamic_cast<TechDrawGui::ViewProviderDrawingView*>(
        Gui::Application::Instance->getViewProvider(viewPart));
    
    if (!vpdv) {
        return ""; 
    }

    QGIView* qgiView = vpdv->getQView();
    if (!qgiView) {
        return "";
    }

    QPointF viewLocal = point - qgiView->scenePos();

    QPointF appPt = Rez::appX(viewLocal);

    Base::Vector3d placement(appPt.x(), appPt.y(), 0.0);
    placement = TechDraw::CosmeticVertex::makeCanonicalPointInverted(viewPart, placement);

    std::string vertexName = viewPart->addCosmeticVertex(placement, false);
    viewPart->refreshCVGeoms();
    viewPart->requestPaint();
    return vertexName;
}

std::vector<std::string> execQuadrants(int GeoId, DrawViewPart* dvp)
{

    const TechDraw::BaseGeomPtrVector edges = dvp->getEdgeGeometry();
    TechDraw::BaseGeomPtr geom = edges.at(GeoId);
    std::vector<Base::Vector3d> quads = geom->getQuads();
    std::vector<std::string> vertexNames;
    for (auto& q: quads) {
        // invert the point so the math works correctly
        Base::Vector3d placement = DrawUtil::invertY(q);
        placement = CosmeticVertex::makeCanonicalPoint(dvp, placement);
        std::string vertexName = dvp->addCosmeticVertex(placement);
        vertexNames.push_back(vertexName);
    }

    dvp->refreshCVGeoms();
    dvp->requestPaint();
    return vertexNames;
}

std::string execMidpoints(int GeoId, DrawViewPart* dvp)
{
    const TechDraw::BaseGeomPtrVector edges = dvp->getEdgeGeometry();
    TechDraw::BaseGeomPtr geom = edges.at(GeoId);
    Base::Vector3d mid = geom->getMidPoint();
    // invert the point so the math works correctly
    mid = DrawUtil::invertY(mid);
    mid = CosmeticVertex::makeCanonicalPoint(dvp, mid);
    std::string vertexName = dvp->addCosmeticVertex(mid);

    dvp->refreshCVGeoms();
    dvp->requestPaint();

    return vertexName;
}

std::vector<std::string> execIntersection(int GeoId1, int GeoId2, DrawViewPart* dvp)
{

    TechDraw::BaseGeomPtr geom1 = dvp->getGeomByIndex(GeoId1);
    TechDraw::BaseGeomPtr geom2 = dvp->getGeomByIndex(GeoId2);

    std::vector<std::string> vertexNames;

    std::vector<Base::Vector3d> interPoints = geom1->intersection(geom2);
    for (auto pt : interPoints) {
        // geometry points are inverted
        Base::Vector3d placement = CosmeticVertex::makeCanonicalPointInverted(dvp, pt);
        vertexNames.push_back(dvp->addCosmeticVertex(placement, false));
    }
    dvp->refreshCVGeoms();
    dvp->requestPaint();
    return vertexNames;
}

void TechDrawVertexHandler::activated()
{
    if (viewPage) {
        QPoint hotspot(15, 15);
        QPixmap pixmap = viewPage->prepareCursorPixmap("actions/TechDraw_Vertex_Pointer.svg", hotspot);
        cursor = QCursor(pixmap, hotspot.x(), hotspot.y());
        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }
}

void TechDrawVertexHandler::deactivated()
{
    if (viewPage) {
        if (auto vp = viewPage->viewport()) {
            vp->unsetCursor();
        }
    }
    if (!previewNames.empty() && previewViewPart) {
        for (auto& name : previewNames) {
            previewViewPart->removeCosmeticVertex(name);
        }
        previewViewPart->refreshCVGeoms();
        previewViewPart->requestPaint();
        previewNames.clear();
        previewViewPart = nullptr;
        previewGeoIds.clear();
        isPreviewing = false;
    }
}

void TechDrawVertexHandler::mouseMoveEvent(QMouseEvent* event)
{

    if (auto vp = viewPage->viewport()) {
        vp->setCursor(cursor);
    }

    if (Gui::Control().activeDialog()) {
        return;
    }

    QPointF scenePos = viewPage->mapToScene(event->pos());
    QPainterPath circle;
    circle.addEllipse(scenePos, 5.0, 5.0);
    auto items = viewPage->scene()->items(circle);

    std::vector<std::string> subNames;
    TechDraw::DrawViewPart* viewPart{nullptr};

    bool foundEdge = false;

    std::vector<QGIEdge*> edges;
    int edgeCount = 0;
    std::vector<QGIVertex*> vertices;
    int vertexCount = 0;


    for (QGraphicsItem* item : items) {
        if (auto* edge = dynamic_cast<QGIEdge*>(item)) {
            edges.push_back(edge);
            edgeCount++;
        }
        if (auto* vertex = dynamic_cast<QGIVertex*>(item)) {
            vertices.push_back(vertex);
            vertexCount++;
        }
    }

    if (vertexCount >= 1) {
        edges.clear();
        edgeCount = 0;
    }

    if (edgeCount == 1) {
        QGIEdge* edge = edges[0];
        auto* parent = edge->parentItem();
        QGIView* view = dynamic_cast<QGIView*>(parent);
        if (view) {
            viewPart = dynamic_cast<TechDraw::DrawViewPart*>(view->getViewObject());
        }
    
        int GeoId = TechDraw::DrawUtil::getIndexFromName("Edge" + std::to_string(edge->getProjIndex()));
        TechDraw::BaseGeomPtr geom = viewPart->getGeomByIndex(GeoId);
    
        std::vector<int> newGeoIds = {GeoId};

        if (!isPreviewing || newGeoIds != previewGeoIds) {
            if (isPreviewing) {
                for (auto& name : previewNames) {
                    previewViewPart->removeCosmeticVertex(name);
                }
                previewViewPart->refreshCVGeoms();
                previewViewPart->requestPaint();
            }
            if (geom->getGeomType() == GeomType::CIRCLE) {
                previewNames = execQuadrants(GeoId, viewPart);
            }
            else {
                previewNames = std::vector<std::string>{execMidpoints(GeoId, viewPart)};
            }
            previewViewPart = viewPart;
            previewGeoIds = newGeoIds;
            isPreviewing = true;
        }
        foundEdge = true;
    }

    if (edgeCount >= 2) {
        std::string edge1 = "Edge" + std::to_string(edges[0]->getProjIndex());
        std::string edge2 = "Edge" + std::to_string(edges[1]->getProjIndex());

        int GeoId1 = TechDraw::DrawUtil::getIndexFromName(edge1);
        int GeoId2 = TechDraw::DrawUtil::getIndexFromName(edge2);

        auto* parent = edges[0]->parentItem();
        QGIView* view = dynamic_cast<QGIView*>(parent);
        if (view) {
            viewPart = dynamic_cast<TechDraw::DrawViewPart*>(view->getViewObject());
        }
        std::vector<int> newGeoIds = {GeoId1, GeoId2};
        if (!isPreviewing || newGeoIds != previewGeoIds) {
            if (isPreviewing) {
                for (auto& name : previewNames) {
                    previewViewPart->removeCosmeticVertex(name);
                }
                previewViewPart->refreshCVGeoms();
                previewViewPart->requestPaint();
            }
            previewNames = execIntersection(GeoId1, GeoId2, viewPart);
            previewViewPart = viewPart;
            previewGeoIds = newGeoIds;
            isPreviewing = true;
        }
        foundEdge = true;
    }

    if (vertexCount >= 1) {
        QGIVertex* vertex = vertices[0];
        auto* parent = vertex->parentItem();
        QGIView* view = dynamic_cast<QGIView*>(parent);
        if (view) {
            viewPart = dynamic_cast<TechDraw::DrawViewPart*>(view->getViewObject());
        }
        currentState = State::Offset;
        hoveredVertex = vertex;
        vertexViewPart = viewPart;

        foundEdge = false;
    }
    else {
        currentState = State::None;
        hoveredVertex = nullptr;
        vertexViewPart = nullptr;
    }

    if (!foundEdge && isPreviewing) {
        isPreviewing = false;
        for (auto& name : previewNames) {
            previewViewPart->removeCosmeticVertex(name);
        }
        previewViewPart->refreshCVGeoms();
        previewViewPart->requestPaint();
        previewNames.clear();
        previewViewPart = nullptr;
        previewGeoIds.clear();
    }
}

void TechDrawVertexHandler::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (currentState == State::None) {
        std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx();
        if (sel.empty()) {
            return;
        }
        DrawViewPart* viewPart = dynamic_cast<TechDraw::DrawViewPart*>(sel[0].getObject());
        if (!viewPart) {
            return;
        }

        if (isPreviewing) {
            viewPart->refreshCVGeoms();
            viewPart->requestPaint();
            previewNames.clear();
            previewViewPart = nullptr;
            previewGeoIds.clear();
            isPreviewing = false;
        }
        else {
            if (Gui::Control().activeDialog()) {
                return;
            }
            AddVertex(viewPage->mapToScene(event->pos()), viewPart);
        }
    }

    if (currentState == State::Offset && hoveredVertex && vertexViewPart) {

        if (Gui::Control().activeDialog()) {
            return;
        }

        int projIndex = hoveredVertex->getProjIndex();
        TechDraw::VertexPtr vertex = vertexViewPart->getProjVertexByIndex(projIndex);
        if (!vertex) {
            return;
        }
        Gui::Control().showDialog(new TaskDlgAddOffsetVertex(vertexViewPart, vertex, projIndex));
        
        currentState = State::None;
    }
    
}

void TechDrawVertexHandler::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawVertexHandler::keyPressEvent(QKeyEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawVertexHandler::addPreselected()
{
    if (Gui::Control().activeDialog()) {
        return;
    }

    auto selection = Gui::Selection().getSelectionEx();

    TechDraw::DrawViewPart* dvp = nullptr;
    std::vector<int> edgeIndices;
    std::vector<int> vertexIndices;

    for (auto& sel : selection) {
        if (!sel.getObject()->isDerivedFrom<TechDraw::DrawViewPart>()) {
            continue;
        }

        dvp = static_cast<TechDraw::DrawViewPart*>(sel.getObject());

        for (auto& subName : sel.getSubNames()) {
            std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(subName);
            int index = TechDraw::DrawUtil::getIndexFromName(subName);
            if (geomType == "Edge") {
                edgeIndices.push_back(index);
            }
            else if (geomType == "Vertex") {
                vertexIndices.push_back(index);
            }
        }
    }

    if (!dvp) {
        return;
    }

    if (!vertexIndices.empty()) {
        int vertexIndex = vertexIndices[0];
        TechDraw::VertexPtr vertex = dvp->getProjVertexByIndex(vertexIndex);
        if (!vertex) {
            return;
        }
        Gui::Control().showDialog(new TaskDlgAddOffsetVertex(dvp, vertex, vertexIndex));
    }
    else if (edgeIndices.size() == 1) {
        for (int edgeIndex : edgeIndices) {
            const TechDraw::BaseGeomPtrVector edges = dvp->getEdgeGeometry();
            TechDraw::BaseGeomPtr geom = edges.at(edgeIndex);
            if (geom->getGeomType() == GeomType::CIRCLE) {
                execQuadrants(edgeIndex, dvp);
            }
            else {
                execMidpoints(edgeIndex, dvp);
            }
        }
    }
    else if (edgeIndices.size() >= 2) {
        execIntersection(edgeIndices[0], edgeIndices[1], dvp);
    }
}