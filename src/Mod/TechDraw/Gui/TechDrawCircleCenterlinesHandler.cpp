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

#include "TechDrawCircleCenterlinesHandler.h"

#include <QMouseEvent>
#include <QMessageBox>
#include <QKeyEvent>
#include <QPointF>
#include <QObject>
#include <QGraphicsItem>
#include <vector>
#include <string>
#include <cmath>

#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Gui/Selection/Selection.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/ArrowPropEnum.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/CosmeticVertex.h>

#include <Base/Tools.h>

#include "QGIEdge.h"
#include "QGIFace.h"
#include "QGIVertex.h"
#include "QGIViewPart.h"
#include "QGIViewBalloon.h"
#include "QGSPage.h"
#include "QGIView.h"
#include "QGVPage.h"
#include "Rez.h"
#include "ViewProviderPage.h"
#include "TechDrawHandler.h"
#include "CommandHelpers.h"
#include "ViewProviderDrawingView.h"
#include "DrawGuiUtil.h"
#include "MDIViewPage.h"


using namespace TechDrawGui;
using namespace TechDraw;

void execHoleCircle(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat, 
    std::vector<std::string>& lines)
{
    //create centerlines of a hole/bolt circle

    std::vector<TechDraw::CirclePtr> Circles;
    for (const std::string& Name : SubNames) {
        int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
        TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);
        if (GeoType == "Edge") {
            if (geom->getGeomType() == GeomType::CIRCLE || geom->getGeomType() == GeomType::ARCOFCIRCLE) {
                TechDraw::CirclePtr cgen = std::static_pointer_cast<TechDraw::Circle>(geom);
                Circles.push_back(cgen);
            }
        }
    }

    if (Circles.size() <= 2) {
        return;
    }

    // make the bolt hole circle from 3 scaled and rotated points
    Base::Vector3d bigCenter =
        _circleCenter(Circles[0]->center, Circles[1]->center, Circles[2]->center);
    double bigRadius = (Circles[0]->center - bigCenter).Length();
    // now convert the center & radius to canonical form
    bigCenter = CosmeticVertex::makeCanonicalPointInverted(objFeat, bigCenter);
    bigRadius = bigRadius / objFeat->getScale();
    TechDraw::BaseGeomPtr bigCircle =
        std::make_shared<TechDraw::Circle>(bigCenter, bigRadius);
    std::string bigCircleTag = objFeat->addCosmeticEdge(bigCircle);
    lines.push_back(bigCircleTag);
    TechDraw::CosmeticEdge* ceCircle = objFeat->getCosmeticEdge(bigCircleTag);
    _setLineAttributes(ceCircle);

    // make the center lines for the individual bolt holes
    constexpr double ExtendFactor{1.1};
    for (const TechDraw::CirclePtr& oneCircle : Circles) {
        // If it is not on the big circle it should skip it
        if (std::sqrt(std::pow(oneCircle->center.x - bigCenter.x, 2) + std::pow(oneCircle->center.y - bigCenter.y, 2)) != bigRadius) {
            continue;
        }
        // convert the center to canonical form
        Base::Vector3d oneCircleCenter = CosmeticVertex::makeCanonicalPointInverted(objFeat, oneCircle->center);
        // oneCircle->radius is scaled.
        double oneRadius = oneCircle->radius / objFeat->getScale();
        // what is magic number 2 (now ExtendFactor)?  just a fudge factor to extend the line beyond the bolt
        // hole circle?  should it be a function of hole diameter? maybe 110% of oneRadius?
        Base::Vector3d delta = (oneCircleCenter - bigCenter).Normalize() * (oneRadius * ExtendFactor);
        Base::Vector3d startPt = oneCircleCenter + delta;
        Base::Vector3d endPt = oneCircleCenter - delta;
        std::string oneLineTag = objFeat->addCosmeticEdge(startPt, endPt);
        TechDraw::CosmeticEdge* ceLine = objFeat->getCosmeticEdge(oneLineTag);
        lines.push_back(oneLineTag);
        _setLineAttributes(ceLine);
    }
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
}


void TechDrawBoltCenterlinesHandler::activated()
{
    if (viewPage) {
        QPoint hotspot(15, 15);
        QPixmap pixmap = viewPage->prepareCursorPixmap("TechDraw_ExtensionHoleCircle_Pointer.svg", hotspot);
        QCursor cursor(pixmap, hotspot.x(), hotspot.y());
        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }

    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::GreedySelection);
}

void TechDrawBoltCenterlinesHandler::deactivated()
{
    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::NormalSelection);
    if (viewPage) {
        if (auto vp = viewPage->viewport()) {
            vp->unsetCursor();
        }
    }
    currentLines.clear();
    currentObjFeat = nullptr;
}

void TechDrawBoltCenterlinesHandler::mouseMoveEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawBoltCenterlinesHandler::mouseReleaseEvent(QMouseEvent* event)
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();

    TechDraw::DrawViewPart* objFeat{nullptr};
    std::vector<std::string> subNames;

    if (!selection.empty()) {
        objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selection[0].getObject());
        subNames = selection[0].getSubNames();

        // If the user clicks on cosmetic edges we stop the selection
        // So we dont attemt to reference it when we remove the former lines
        if (!currentLines.empty()) {
            for (const std::string& name : subNames) {
                TechDraw::CosmeticEdge* cosmeticEdge = objFeat->getCosmeticEdgeBySelection(name);
                if (cosmeticEdge) {
                    previousSubnames.clear();
                    currentLines.clear();
                    currentObjFeat = nullptr;
                    Gui::Selection().clearSelection();
                    return;
                }
            }
        }

    } else {
        return;
    }

    // If the selection does not change then they have finished selecting circles
    // It is possible to do this by just looking at the size of the selection since it is greedy selection
    if (subNames.size() == previousSubnames.size()) {
        previousSubnames.clear();
        Gui::Selection().clearSelection();
        currentLines.clear();
        currentObjFeat = nullptr;
        return;
    }

    previousSubnames = subNames;

    // The redrawing of the lines clears the selection somehow
    // But it works if it just gets blocked before we redraw
    auto* mdi = dynamic_cast<MDIViewPage*>(Gui::getMainWindow()->activeWindow());
    if (mdi) {
        mdi->blockSceneSelection(true);
    }
    
    if (!currentLines.empty() && currentObjFeat) {
        currentObjFeat->removeCosmeticEdge(currentLines);
        currentObjFeat->refreshCEGeoms();
        currentObjFeat->requestPaint();
        currentLines.clear();
        currentObjFeat = nullptr;
    }

    if (objFeat) {
        currentObjFeat = objFeat;

        execHoleCircle(subNames, objFeat, currentLines);
    }

    if (mdi) {
        if (objFeat) {
            mdi->selectQGIView(objFeat, true, subNames);
        }
        mdi->blockSceneSelection(false);
    }

    Q_UNUSED(event);
}

void TechDrawBoltCenterlinesHandler::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawBoltCenterlinesHandler::keyPressEvent(QKeyEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawBoltCenterlinesHandler::addPreselected()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    
    if (!selection.empty()) {
        std::vector<std::string> SubNames = selection[0].getSubNames();
        
        if (SubNames.size() < 3) {
            Gui::Selection().clearSelection();
            return;
        }
        
        TechDraw::DrawViewPart* objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selection[0].getObject());

        std::vector<std::string> currentLines;
        execHoleCircle(SubNames, objFeat, currentLines);
        Gui::Selection().clearSelection();
    }
}


void execCircleCenterLines(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat,
                           std::vector<std::string>& lines)
{
    // create circle centerlines
    for (const std::string& Name : SubNames) {
        int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
        TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
        if (GeoType == "Edge") {
            if (geom->getGeomType() == GeomType::CIRCLE || geom->getGeomType() == GeomType::ARCOFCIRCLE) {
                TechDraw::CirclePtr cgen = std::static_pointer_cast<TechDraw::Circle>(geom);
                // cgen->center is a scaled, rotated and inverted point
                Base::Vector3d center = CosmeticVertex::makeCanonicalPointInverted(objFeat, cgen->center);
                double radius = cgen->radius / objFeat->getScale();
                // right, left, top, bottom are formed from a canonical point (center)
                // so they do not need to be changed to canonical form.
                constexpr double lineOutsideCircle{2.0};
                Base::Vector3d right(center.x + radius + lineOutsideCircle, center.y, 0.0);
                Base::Vector3d top(center.x, center.y + radius + lineOutsideCircle, 0.0);
                Base::Vector3d left(center.x - radius - lineOutsideCircle, center.y, 0.0);
                Base::Vector3d bottom(center.x, center.y - radius - lineOutsideCircle, 0.0);
                std::string line1tag = objFeat->addCosmeticEdge(right, left);
                std::string line2tag = objFeat->addCosmeticEdge(top, bottom);
                lines.push_back(line1tag);
                lines.push_back(line2tag);
                TechDraw::CosmeticEdge* horiz = objFeat->getCosmeticEdge(line1tag);
                _setLineAttributes(horiz);
                TechDraw::CosmeticEdge* vert = objFeat->getCosmeticEdge(line2tag);
                _setLineAttributes(vert);
                // horiz & vert are centerlines, so they should use the default centerline
                // number and not the number from line attributes
                horiz->m_format.setLineNumber(Preferences::CenterLineStyle());
                vert->m_format.setLineNumber(Preferences::CenterLineStyle());
            }
        }
    }

    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
}




void TechDrawCircleCenterlinesHandler::activated()
{
    if (viewPage) {
        QPoint hotspot(15, 15);
        QPixmap pixmap = viewPage->prepareCursorPixmap("TechDraw_ExtensionCircleCenterLines_Pointer.svg", hotspot);
        QCursor cursor(pixmap, hotspot.x(), hotspot.y());
        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }
}

void TechDrawCircleCenterlinesHandler::deactivated()
{
    if (viewPage) {
        if (auto vp = viewPage->viewport()) {
            vp->unsetCursor();
        }
    }
    if (!previewLines.empty() && previewObjFeat) {
        previewObjFeat->removeCosmeticEdge(previewLines);
        previewObjFeat->refreshCEGeoms();
        previewObjFeat->requestPaint();
        previewLines.clear();
        previewObjFeat = nullptr;
    }
}

void TechDrawCircleCenterlinesHandler::mouseMoveEvent(QMouseEvent* event)
{
    if (!viewPage) {
        return;
    }
    auto items = viewPage->scene()->items(viewPage->mapToScene(event->pos()));
    
    std::vector<std::string> SubNames;
    TechDraw::DrawViewPart* objFeat{nullptr};

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

            if (geom->getGeomType() != GeomType::CIRCLE && geom->getGeomType() != GeomType::ARCOFCIRCLE) {
                SubNames.clear();
                objFeat = nullptr;
                continue;
            }
            break;
        }
    }

    if (SubNames.empty() || !objFeat) {
        if (!previewLines.empty()) {
            previewObjFeat->removeCosmeticEdge(previewLines);
            previewObjFeat->refreshCEGeoms();
            previewObjFeat->requestPaint();
            previewLines.clear();
            previewObjFeat = nullptr;
        }
    }
    else {
        execCircleCenterLines(SubNames, objFeat, previewLines);
        previewObjFeat = objFeat;
    }
}

void TechDrawCircleCenterlinesHandler::mouseReleaseEvent(QMouseEvent* event)
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();

    if (!selection.empty()) {
        TechDraw::DrawViewPart* objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selection[0].getObject());
        std::vector<std::string> SubNames = selection[0].getSubNames();
        std::vector<std::string> lines;

        int GeoId = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
        TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);

        if (geom->getGeomType() != GeomType::CIRCLE && geom->getGeomType() != GeomType::ARCOFCIRCLE) {
            return;
        }

        execCircleCenterLines(SubNames, objFeat, lines);
    }

    Gui::Selection().clearSelection();
}

void TechDrawCircleCenterlinesHandler::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawCircleCenterlinesHandler::keyPressEvent(QKeyEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawCircleCenterlinesHandler::addPreselected() {

    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    
    if (!selection.empty()) {
        TechDraw::DrawViewPart* objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selection[0].getObject());
        std::vector<std::string> SubNames = selection[0].getSubNames();
        std::vector<std::string> lines;
        execCircleCenterLines(SubNames, objFeat, lines);
    }

}