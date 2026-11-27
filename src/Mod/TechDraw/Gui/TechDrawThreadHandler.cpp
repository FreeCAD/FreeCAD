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

#include "TechDrawThreadHandler.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QPointF>
#include <QGraphicsItem>
#include <vector>
#include <string>

#include <App/Document.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/Selection/Selection.h>
#include <Gui/InputHint.h>

#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "QGIEdge.h"
#include "QGIView.h"
#include "QGVPage.h"
#include "CommandHelpers.h"


using namespace TechDrawGui;
using namespace TechDraw;

void execThreadHoleSide(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat)
{
    constexpr double ThreadFactor{1.176};
    if (SubNames.size() >= 2) {
        _createThreadLines(SubNames, objFeat, ThreadFactor, true);
    }
    Gui::Selection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
}


void execThreadBoltSide(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat)
{
    constexpr double ThreadFactor{0.85};
    if (SubNames.size() >= 2) {
        _createThreadLines(SubNames, objFeat, ThreadFactor, false);
    }
    Gui::Selection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
}

std::string execThreadHoleBottom(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat)
{
    constexpr double ThreadFactor{1.176};
    std::string arcTag;
    for (const std::string& Name : SubNames) {
        arcTag = _createThreadCircle(Name, objFeat, ThreadFactor);
    }
    Gui::Selection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    return arcTag;
}

std::string execThreadBoltBottom(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat)
{
    constexpr double ThreadFactor{0.85};
    std::string arcTag;
    for (const std::string& Name : SubNames) {
        arcTag = _createThreadCircle(Name, objFeat, ThreadFactor);
    }
    Gui::Selection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    return arcTag;
}

void TechDrawThreadHandler::activated()
{
    if (viewPage) {
        QPoint hotspot(15, 15);
        QPixmap pixmap;
        if (m_commandName == "ThreadHoleSide") {
            pixmap = viewPage->prepareCursorPixmap("TechDraw_ThreadHoleSide_Pointer.svg", hotspot);
        } 
        else if (m_commandName == "ThreadBoltSide") {
            pixmap = viewPage->prepareCursorPixmap("TechDraw_ThreadBoltSide_Pointer.svg", hotspot);
        }
        else if (m_commandName == "ThreadHoleBottom") {
            pixmap = viewPage->prepareCursorPixmap("TechDraw_ThreadHoleBottom_Pointer.svg", hotspot);
        }
        else if (m_commandName == "ThreadBoltBottom") {
            pixmap = viewPage->prepareCursorPixmap("TechDraw_ThreadBoltBottom_Pointer.svg", hotspot);
        }

        cursor = QCursor(pixmap, hotspot.x(), hotspot.y());
        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }

    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::GreedySelection);
}

void TechDrawThreadHandler::deactivated()
{
    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::NormalSelection);
    if (viewPage) {
        if (auto vp = viewPage->viewport()) {
            vp->unsetCursor();
        }
    }

    if (!previewTag.empty()) {
        previewObjFeat->removeCosmeticEdge(previewTag);
        previewObjFeat->refreshCEGeoms();
        previewObjFeat->requestPaint();
        previewTag = "";
        previewSubNames.clear();
        previewObjFeat = nullptr;
    }
}

std::list<Gui::InputHint> TechDrawThreadHandler::getToolHints() const
{
    return {};
}

void TechDrawThreadHandler::mouseMoveEvent(QMouseEvent* event)
{
    if (auto vp = viewPage->viewport()) {
        vp->setCursor(cursor);
    }
    if (m_commandName == "ThreadHoleSide" || m_commandName == "ThreadBoltSide") {
        return;
    }

    QPointF scenePos = viewPage->mapToScene(event->pos());
    QPainterPath circle;
    circle.addEllipse(scenePos, 5.0, 5.0);
    auto items = viewPage->scene()->items(circle);

    std::vector<std::string> SubNames;
    TechDraw::DrawViewPart* objFeat = nullptr;

    bool foundEdge = false;
    for (QGraphicsItem* item : items) {
        QGIEdge* edge = dynamic_cast<QGIEdge*>(item);
        if (edge) {
            SubNames.push_back("Edge" + std::to_string(edge->getProjIndex()));
            auto* parent = edge->parentItem();
            QGIView* view = dynamic_cast<QGIView*>(parent);
            if (view) {
                objFeat = dynamic_cast<TechDraw::DrawViewPart*>(view->getViewObject());
            }

            int GeoId = TechDraw::DrawUtil::getIndexFromName("Edge" + std::to_string(edge->getProjIndex()));
            TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);

            if (geom->getGeomType() != GeomType::CIRCLE) {
                SubNames.clear();
                objFeat = nullptr;
                continue;
            }
            foundEdge = true;
            break;
        }
    }

    if (!foundEdge && !previewTag.empty()) {
        previewObjFeat->removeCosmeticEdge(previewTag);
        previewObjFeat->refreshCEGeoms();
        previewObjFeat->requestPaint();
        previewTag = "";
        previewSubNames.clear();
        previewObjFeat = nullptr;
    }

    if (SubNames.empty() || !objFeat) {
        return;
    }

    if (m_commandName == "ThreadHoleBottom") {
        if (!previewTag.empty()) {
            previewObjFeat->removeCosmeticEdge(previewTag);
        }
        previewTag = execThreadHoleBottom(SubNames, objFeat);
        previewSubNames = SubNames;
        previewObjFeat = objFeat;
    } 
    else if (m_commandName == "ThreadBoltBottom") {
        if (!previewTag.empty()) {
            previewObjFeat->removeCosmeticEdge(previewTag);
        }
        previewTag = execThreadBoltBottom(SubNames, objFeat);
        previewSubNames = SubNames;
        previewObjFeat = objFeat;
    }

    Q_UNUSED(event);
}

void TechDrawThreadHandler::mouseReleaseEvent(QMouseEvent* event)
{
    std::vector<Gui::SelectionObject> selObjs = Gui::Selection().getSelectionEx();
    if (selObjs.empty()) {
        return;
    }

    int edgeCount = 0;

    if (!previewTag.empty() && m_commandName == "ThreadHoleBottom") {
        previewObjFeat->removeCosmeticEdge(previewTag);
        previewObjFeat->refreshCEGeoms();
        previewObjFeat->requestPaint();
        previewObjFeat->getDocument()->openTransaction(QT_TRANSLATE_NOOP("Command", "Cosmetic thread hole bottom"));
        execThreadHoleBottom(previewSubNames, previewObjFeat);
        previewObjFeat->getDocument()->commitTransaction();
        previewTag = "";
        previewSubNames.clear();
        previewObjFeat = nullptr;
    } 
    else if (!previewTag.empty() && m_commandName == "ThreadBoltBottom") {
        previewObjFeat->removeCosmeticEdge(previewTag);
        previewObjFeat->refreshCEGeoms();
        previewObjFeat->requestPaint();
        previewObjFeat->getDocument()->openTransaction(QT_TRANSLATE_NOOP("Command", "Cosmetic thread bolt bottom"));
        execThreadBoltBottom(previewSubNames, previewObjFeat);
        previewObjFeat->getDocument()->commitTransaction();
        previewTag = "";
        previewSubNames.clear();
        previewObjFeat = nullptr;
    }

    for (auto& selObj : selObjs) {

        auto* selObjFeat = dynamic_cast<TechDraw::DrawViewPart*>(selObj.getObject());
        const std::vector<std::string>& subNames = selObj.getSubNames();
        for (auto& subName : subNames) {
            if (subName.empty()) {
                continue;
            }

            std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(subName);
            int geoId = TechDraw::DrawUtil::getIndexFromName(subName);

            if (geomType == "Edge") {
                edgeCount++;
            }
        }
    }

    if (edgeCount == 2 && m_commandName == "ThreadHoleSide") {
        addPreselected();
    } 
    else if (edgeCount == 2 && m_commandName == "ThreadBoltSide") {
        addPreselected();
    } 
    else if (edgeCount == 1 && m_commandName == "ThreadHoleBottom") {
        addPreselected();
    } 
    else if (edgeCount == 1 && m_commandName == "ThreadBoltBottom") {
        addPreselected();
    }
    Q_UNUSED(event);
}

void TechDrawThreadHandler::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawThreadHandler::keyPressEvent(QKeyEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawThreadHandler::addPreselected()
{
    std::vector<Gui::SelectionObject> selObjs = Gui::Selection().getSelectionEx();
    if (selObjs.empty()) {
        return;
    }
    TechDraw::DrawViewPart* objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selObjs[0].getObject());
    if (!objFeat) {
        return;
    }

    std::vector<std::string> SubNames = selObjs[0].getSubNames();

    if (m_commandName == "ThreadHoleSide") {
        objFeat->getDocument()->openTransaction(QT_TRANSLATE_NOOP("Command", "Cosmetic thread hole side"));
        execThreadHoleSide(SubNames, objFeat);
        objFeat->getDocument()->commitTransaction();
    }
    else if (m_commandName == "ThreadBoltSide") {
        objFeat->getDocument()->openTransaction(QT_TRANSLATE_NOOP("Command", "Cosmetic thread bolt side"));
        execThreadBoltSide(SubNames, objFeat);
        objFeat->getDocument()->commitTransaction();
    }
    else if (m_commandName == "ThreadHoleBottom") {
        objFeat->getDocument()->openTransaction(QT_TRANSLATE_NOOP("Command", "Cosmetic thread hole bottom"));
        execThreadHoleBottom(SubNames, objFeat);
        objFeat->getDocument()->commitTransaction();
    } 
    else if (m_commandName == "ThreadBoltBottom") {
        objFeat->getDocument()->openTransaction(QT_TRANSLATE_NOOP("Command", "Cosmetic thread bolt bottom"));
        execThreadBoltBottom(SubNames, objFeat);
        objFeat->getDocument()->commitTransaction();
    }
}