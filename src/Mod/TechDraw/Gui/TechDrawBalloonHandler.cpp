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

#include "TechDrawBalloonHandler.h"

#include <QMouseEvent>
#include <QMessageBox>
#include <QKeyEvent>
#include <QPointF>
#include <QObject>
#include <QGraphicsItem>
#include <vector>
#include <string>

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


using namespace TechDrawGui;
using DU = TechDraw::DrawUtil;

static bool _checkDirectPlacement(const QGIView* view, const std::vector<std::string>& subNames,
                           QPointF& placement, bool& isFace)
{
    // Let's see, if we can help speed up the placement of the balloon:
    // As of now we support:
    //     Single selected vertex: place the balloon tip end here
    //     Single selected edge:   place the balloon tip at its midpoint (suggested placement for e.g. chamfer dimensions)
    //
    // Single selected faces are currently not supported, but maybe we could in this case use the center of mass?

    if (subNames.size() != 1) {
        // If nothing or more than one subjects are selected, let the user decide, where to place the balloon
        return false;
    }

    const QGIViewPart* viewPart = dynamic_cast<const QGIViewPart*>(view);
    if (!viewPart) {
        //not a view of a part, so no geometry to attach to
        return false;
    }

    isFace = false;

    std::string geoType = TechDraw::DrawUtil::getGeomTypeFromName(subNames[0]);
    if (geoType == "Vertex") {
        int index = TechDraw::DrawUtil::getIndexFromName(subNames[0]);
        TechDraw::VertexPtr vertex =
            static_cast<TechDraw::DrawViewPart*>(viewPart->getViewObject())->getProjVertexByIndex(index);
        if (vertex) {
            placement = viewPart->mapToScene(Rez::guiX(vertex->x()), Rez::guiX(vertex->y()));
            return true;
        }
    }
    else if (geoType == "Edge") {
        int index = TechDraw::DrawUtil::getIndexFromName(subNames[0]);
        TechDraw::BaseGeomPtr geo =
            static_cast<TechDraw::DrawViewPart*>(viewPart->getViewObject())->getGeomByIndex(index);
        if (geo) {
            Base::Vector3d midPoint(Rez::guiX(geo->getMidPoint()));
            placement = viewPart->mapToScene(midPoint.x, midPoint.y);
            return true;
        }
    }
    else if (geoType == "Face") {
        // For a face we use the placement of the click, we do this in the handler
        isFace = true;
        return true;
    }

    return false;
}


std::list<Gui::InputHint> TechDrawBalloonHandler::getToolHints() const
{
    if (currentState == State::NotPlacing) {
        return {};
    }
    else if (currentState == State::PlacingHeadSecond) {
        return {
            {QObject::tr("%1 pick head placement", "TechDraw Balloon: hint"),
                {Gui::InputHint::UserInput::MouseLeft}},
            {QObject::tr("%1 change head shape", "TechDraw Balloon: hint"),
                {Gui::InputHint::UserInput::KeyM}},
        };
    }
    else if (currentState == State::PlacingOriginSecond) {
        return {
            {QObject::tr("%1 pick origin placement", "TechDraw Balloon: hint"),
                {Gui::InputHint::UserInput::MouseLeft}},
            {QObject::tr("%1 change head shape", "TechDraw Balloon: hint"),
                {Gui::InputHint::UserInput::KeyM}},
        };
    }

    return {};
}

void TechDrawBalloonHandler::resetPlacementState()
{
    pointSet = false;
    isFace = false;
    firstPoint = QPointF();
    headPlacement = QPointF();
    sourceView = nullptr;
    sourcePage = nullptr;
    previewBalloon = nullptr;
    currentState = State::NotPlacing;
    updateHint();
}

void TechDrawBalloonHandler::updatePreviewBalloon(const QPointF& cursorLocation)
{
    if (!previewBalloon || !sourcePage || !sourceView) {
        return;
    }

    if (currentState == State::PlacingOriginSecond) {
        auto items = sourcePage->items(cursorLocation);
        for (auto* item : items) {
            if (!dynamic_cast<QGIEdge*>(item) && !dynamic_cast<QGIVertex*>(item) && !dynamic_cast<QGIFace*>(item)) {
                continue;
            }

            auto* qgiView = dynamic_cast<QGIView*>(item->parentItem());
            if (!qgiView) {
                continue;
            }

            auto* hoveredView = dynamic_cast<TechDraw::DrawViewPart*>(qgiView->getViewObject());
            if (!hoveredView || hoveredView == sourceView) {
                break;
            }

            auto* qgi = sourcePage->getQGIVByName(previewBalloon->getNameInDocument());
            auto* qBalloon = dynamic_cast<QGIViewBalloon*>(qgi);

            QPointF balloonCenter;
            if (qBalloon && qBalloon->getBalloonLabel()) {
                auto* label = qBalloon->getBalloonLabel();
                balloonCenter = qBalloon->mapToScene(label->getCenterX(), label->getCenterY());
            }

            // Switch the source view to the view we are hovering over
            sourceView = hoveredView;
            previewBalloon->SourceView.setValue(hoveredView);

            // The location of the balloon should stay the same in relation to the page
            // But currently the balloon position is in relation to the view
            // So we need to find the new position of the balloon in the new view and move it there
            auto* qViewNew = sourcePage->findQViewForDocObj(hoveredView);
            if (qBalloon && qViewNew && qBalloon->getBalloonLabel()) {
                QPointF newCenter = qViewNew->mapFromScene(balloonCenter);
                double newX = Rez::appX(newCenter.x()) / hoveredView->getScale();
                double newY = -Rez::appX(newCenter.y()) / hoveredView->getScale();

                previewBalloon->X.setValue(newX);
                previewBalloon->Y.setValue(newY);
            }
            break;
        }
    }

    auto* qParent = sourcePage->getQGIVByName(sourceView->getNameInDocument());

    auto* qgi = sourcePage->getQGIVByName(previewBalloon->getNameInDocument());
    auto* qBalloon = dynamic_cast<QGIViewBalloon*>(qgi);
    
    if (!qParent || !qBalloon || !qBalloon->getBalloonLabel()) {
        return;
    }

    if (currentState == State::PlacingOriginSecond) {
        QPointF location = qParent->mapFromScene(cursorLocation);
        auto origin = DU::toVector3d(location);
        origin = Rez::appX(origin) / sourceView->getScale();
        origin = TechDraw::DrawUtil::invertY(origin);
        origin.RotateZ(Base::toRadians(-sourceView->Rotation.getValue()));

        previewBalloon->setOrigin(origin);
    }
    else if (currentState == State::PlacingHeadSecond) {
        QPointF balloonPos = qBalloon->mapFromScene(cursorLocation);
        qBalloon->getBalloonLabel()->setPosFromCenter(balloonPos.x(), balloonPos.y());
    }
    
    qBalloon->balloonLabelDragged(false);
}

void TechDrawBalloonHandler::cancelPreviewBalloon()
{
    if (!previewBalloon) {
        return;
    }

    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Cancel Balloon"));
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.removeObject('%s')",
                            previewBalloon->getNameInDocument());
    Gui::Command::commitCommand(tid);
    previewBalloon = nullptr;
}

void TechDrawBalloonHandler::activated()
{
    if (viewPage) {
        QPoint hotspot(15, 15);
        QPixmap pixmap = viewPage->prepareCursorPixmap("TechDraw_Balloon_Pointer", hotspot);
        viewPage->activateCursor(QCursor(pixmap, hotspot.x(), hotspot.y()));
    }
}

void TechDrawBalloonHandler::deactivated()
{
    cancelPreviewBalloon();
    resetPlacementState();
}

void TechDrawBalloonHandler::mouseMoveEvent(QMouseEvent* event)
{
    if (!viewPage || !pointSet || !previewBalloon) {
        return;
    }

    QPointF cursorPlacement = viewPage->mapToScene(event->pos());
    updatePreviewBalloon(cursorPlacement);
}

void TechDrawBalloonHandler::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton && pointSet) {
        cancelPreviewBalloon();
        resetPlacementState();
        event->accept();
        return;
    }

    if (!viewPage) {
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }
    QGraphicsItem* item = viewPage->scene()->itemAt(viewPage->mapToScene(event->pos()), QTransform());

    updateHint();

    if (currentState == State::NotPlacing) {
        if (!item) {
            return;
        }

        auto* qgiView = dynamic_cast<QGIView*>(item);
        if (!qgiView) {
            QGraphicsItem* parent = item->parentItem();
            qgiView = dynamic_cast<QGIView*>(parent);
        }

        // If the user has not clicked directly on a view, or something on that view
        // Then they are trying to pick the placement of the head
        if (!qgiView) {
            headPlacement = viewPage->mapToScene(event->pos());
            currentState = State::PlacingHeadFirst;
        }
        else {
            sourceView = qgiView->getViewObject();
            if (!sourceView) {
                currentState = State::PlacingHeadFirst;
            }

            if (currentState == State::NotPlacing) {
                TechDraw::DrawPage* page = sourceView->findParentPage();
                Gui::Document* guiDoc = Gui::Application::Instance->getDocument(page->getDocument());
                ViewProviderPage* pageVP = freecad_cast<ViewProviderPage*>(guiDoc->getViewProvider(page));
                sourcePage = pageVP ? pageVP->getQGSPage() : nullptr;
            }
            

            if (!sourcePage) {
                currentState = State::PlacingHeadFirst;
            }

            std::vector<std::string> subNames;

            if (currentState == State::NotPlacing) {
                firstPoint = viewPage->mapToScene(event->pos());


                // This checks the type of item clicked
                // just using the subnames from selection had some weird edge cases (No pun intended)
                if (auto* edge = dynamic_cast<QGIEdge*>(item)) {
                    subNames = { "Edge" + std::to_string(edge->getProjIndex()) };
                }
                else if (auto* vertex = dynamic_cast<QGIVertex*>(item)) {
                    subNames = { "Vertex" + std::to_string(vertex->getProjIndex()) };
                }
                else if (auto* face = dynamic_cast<QGIFace*>(item)) {
                    subNames = { "Face" + std::to_string(face->getProjIndex()) };
                }

                if (subNames.empty()) {
                    currentState = State::PlacingHeadFirst;
                }
            }

            // If the user has not clicked directly on a vertex, face, or edge
            // Then they are also trying to pick the placement of the head
            if (!_checkDirectPlacement(qgiView, subNames, firstPoint, isFace)) {
                headPlacement = firstPoint;
                currentState = State::PlacingHeadFirst;
            }
            else {
                currentState = State::PlacingOriginFirst;
            }
        }
        updateHint();
    }
    else if (currentState == State::PlacingHeadSecond) {
        headPlacement = viewPage->mapToScene(event->pos());

        if (!sourcePage || !sourceView || !previewBalloon) {
            resetPlacementState();
            return;
        }

        updatePreviewBalloon(headPlacement);
        
        auto* qgi = sourcePage->getQGIVByName(previewBalloon->getNameInDocument());
        auto* qBalloon = dynamic_cast<QGIViewBalloon*>(qgi);

        if (qBalloon) {
            qBalloon->setPreviewMode(false);
            qBalloon->balloonLabelDragFinished();
        }
        
        resetPlacementState();
        return;
    }
    else if (currentState == State::PlacingOriginSecond) {
        QPointF originPlacement = viewPage->mapToScene(event->pos());
        auto items = viewPage->scene()->items(originPlacement);

        std::vector<std::string> subNames;
        QGraphicsItem* clickedItem = nullptr;
        for (auto* item : items) {
            if (auto* edge = dynamic_cast<QGIEdge*>(item)) {
                clickedItem = item;
                subNames.push_back("Edge" + std::to_string(edge->getProjIndex()));
                break;
            }
            if (auto* vertex = dynamic_cast<QGIVertex*>(item)) {
                clickedItem = item;
                subNames.push_back("Vertex" + std::to_string(vertex->getProjIndex()));
                break;
            }
            if (auto* face = dynamic_cast<QGIFace*>(item)) {
                clickedItem = item;
                subNames.push_back("Face" + std::to_string(face->getProjIndex()));
                break;
            }
        }

        if (subNames.empty()) {
            return;
        }

        QGraphicsItem* parent = clickedItem->parentItem();
        QGIView* qgiView = dynamic_cast<QGIView*>(parent);

        if (!qgiView) {
            return;
        }

        auto* selectedView = dynamic_cast<TechDraw::DrawViewPart*>(qgiView->getViewObject());
        
        if (!selectedView) {
            return;
        }

        if (!_checkDirectPlacement(qgiView, subNames, originPlacement, isFace)) {
            return;
        }

        if (!sourcePage || !sourceView || !previewBalloon) {
            resetPlacementState();
            return;
        }

        if (isFace) {
            previewBalloon->EndType.setValue(static_cast<int>(TechDraw::ArrowType::DOT));
        }

        auto* qgi = sourcePage->getQGIVByName(previewBalloon->getNameInDocument());
        auto* qBalloon = dynamic_cast<QGIViewBalloon*>(qgi);
    
        updatePreviewBalloon(originPlacement);

        if (qBalloon) {
            qBalloon->setPreviewMode(false);
            qBalloon->balloonLabelDragFinished();
        }
        
        resetPlacementState();
    }

    if (currentState == State::PlacingOriginFirst) {
        QPointF scenePoint = viewPage->mapToScene(event->pos());

        if (firstPoint.isNull()) {
            return;
        }
        else {
            previewBalloon = sourcePage->createBalloon(firstPoint, scenePoint, sourceView);
        }

        if (!previewBalloon) {
            resetPlacementState();
            return;
        }

        auto* qgi = sourcePage->getQGIVByName(previewBalloon->getNameInDocument());
        if (isFace) {
            previewBalloon->EndType.setValue(static_cast<int>(TechDraw::ArrowType::DOT));
        }
        previewBalloon->BubbleShape.setValue(balloonHeadType.c_str());
        auto* qBalloon = dynamic_cast<QGIViewBalloon*>(qgi);
        if (qBalloon) {
            qBalloon->setPreviewMode(true);
        }

        currentState = State::PlacingHeadSecond;
        pointSet = true;
        updateHint();
        return;
    }
    else if (currentState == State::PlacingHeadFirst) {

        // Find a random view on the page to be the parent of the preview balloon
        QGIView* qgiView = nullptr;
        auto items = viewPage->items();
        for (auto item : items) {
            if (auto view = dynamic_cast<QGIView*>(item)) {
                qgiView = view;
                break;
            }
        }

        if (!qgiView) {
            resetPlacementState();
            return;
        }

        sourceView = qgiView->getViewObject();
        if (!sourceView) {
            resetPlacementState();
            return;
        }

        TechDraw::DrawPage* page = sourceView->findParentPage();
        Gui::Document* guiDoc = Gui::Application::Instance->getDocument(page->getDocument());
        ViewProviderPage* pageVP = freecad_cast<ViewProviderPage*>(guiDoc->getViewProvider(page));
        sourcePage = pageVP ? pageVP->getQGSPage() : nullptr;
        
        if (!sourcePage) {
            resetPlacementState();
            return;
        }
        
        previewBalloon = sourcePage->createBalloon(headPlacement, headPlacement, sourceView);
        
        if (!previewBalloon) {
            resetPlacementState();
            return;
        }

        previewBalloon->BubbleShape.setValue(balloonHeadType.c_str());

        auto* qgi = sourcePage->getQGIVByName(previewBalloon->getNameInDocument());
        auto* qBalloon = dynamic_cast<QGIViewBalloon*>(qgi);
        if (qBalloon) {
            qBalloon->setPreviewMode(true);
        }

        pointSet = true;
        currentState = State::PlacingOriginSecond;
        Gui::Selection().clearSelection();
        updateHint();
        return;
    }


    TechDrawHandler::mouseReleaseEvent(event);
}

void TechDrawBalloonHandler::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawBalloonHandler::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_M) {
        if (balloonHeadType == "Circular") {
            balloonHeadType = "None";
        }
        else if (balloonHeadType == "None") {
            balloonHeadType = "Triangle";
        }
        else if (balloonHeadType == "Triangle") {
            balloonHeadType = "Inspection";
        }
        else if (balloonHeadType == "Inspection") {
            balloonHeadType = "Hexagon";
        }
        else if (balloonHeadType == "Hexagon") {
            balloonHeadType = "Square";
        }
        else if (balloonHeadType == "Square") {
            balloonHeadType = "Rectangle";
        }
        else if (balloonHeadType == "Rectangle") {
            balloonHeadType = "Line";
        }
        else if (balloonHeadType == "Line") {
            balloonHeadType = "Circular";
        }

        updateHint();
    }

    if (previewBalloon) {
        previewBalloon->BubbleShape.setValue(balloonHeadType.c_str());
    }

    if (viewPage) {
        updatePreviewBalloon(viewPage->mapToScene(viewPage->mapFromGlobal(QCursor::pos())));
    }
}

void TechDrawBalloonHandler::addPreselected()
{

    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();

    if (selection.empty()) {
        return;
    }
    else if (selection.size() > 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                             QObject::tr("Too many objects selected"));
        return;
    }

    std::vector<App::DocumentObject*> pages = Gui::Application::Instance->activeDocument()->getDocument()->
        getObjectsOfType(TechDraw::DrawPage::getClassTypeId());

    if (pages.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                             QObject::tr("Create a page first"));
        return;
    }

    auto objFeat(dynamic_cast<TechDraw::DrawView*>(selection[0].getObject()));
    if (!objFeat) {
        return;
    }

    TechDraw::DrawPage* page = objFeat->findParentPage();
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(page->getDocument());
    ViewProviderPage* pageVP = freecad_cast<ViewProviderPage*>(guiDoc->getViewProvider(page));

    std::string PageName = page->getNameInDocument();

    ViewProviderDrawingView* viewVP =
        freecad_cast<ViewProviderDrawingView*>(guiDoc->getViewProvider(objFeat));

    if (pageVP && viewVP) {
        QGSPage* scenePage = pageVP->getQGSPage();
        if (viewPage) {
            auto* view = dynamic_cast<QGIView*>(viewVP->getQView());
            QPointF placement;
            bool isFace = false;
            if (view && _checkDirectPlacement(view, selection[0].getSubNames(), placement, isFace)) {
                if (placement.isNull()) {
                    // if the direct placement is null it is a face, we dont allow it in preselection
                    return;
                }
                QPointF noHeadPlacement;
                scenePage->createBalloon(placement, noHeadPlacement, objFeat);
                return;
            }
        }
    }
};