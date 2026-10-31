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

#include "TechDrawDetailViewHandler.h"

#include <QMessageBox>
#include <QMouseEvent>
#include <QKeyEvent>

#include <Base/Vector3D.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <App/Document.h>

#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewDetail.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include <Gui/Selection/SelectionObject.h>

#include "QGVPage.h"
#include "ViewProviderDrawingView.h"
#include "QGSPage.h"
#include "QGIViewPart.h"
#include "TaskDetail.h"
#include "Rez.h"


using namespace TechDrawGui;

bool _checkDirectPlacement(const QGIView* view, const std::vector<std::string>& subNames,
                       Base::Vector3d& placement)
{
    if (subNames.size() != 1) {
        return false;
    }

    const QGIViewPart* viewPart = dynamic_cast<const QGIViewPart*>(view);
    if (!viewPart) {
        return false;
    }

    std::string geoType = TechDraw::DrawUtil::getGeomTypeFromName(subNames[0]);
    if (geoType == "Vertex") {
        int index = TechDraw::DrawUtil::getIndexFromName(subNames[0]);
        TechDraw::VertexPtr vertex =
            static_cast<TechDraw::DrawViewPart*>(viewPart->getViewObject())->getProjVertexByIndex(index);
        if (vertex) {
            placement = Base::Vector3d(vertex->x(), vertex->y(), 0.0);
        }
    }
    return true;
}

void TechDrawDetailViewHandler::activated()
{
    if (viewPage) {
        QPoint hotspot(15, 15);
        QPixmap pixmap = viewPage->prepareCursorPixmap("actions/TechDraw_DetailView_Pointer.svg", hotspot);
        cursor = QCursor(pixmap, hotspot.x(), hotspot.y());
        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }
}

void TechDrawDetailViewHandler::deactivated()
{
    if (viewPage) {
        if (auto vp = viewPage->viewport()) {
            vp->unsetCursor();
        }
    }
}


void TechDrawDetailViewHandler::mouseMoveEvent(QMouseEvent* event)
{
    if (viewPage) {
        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }

    if (currentState == PlacementState::PlacingDetail) {
        QPointF scenePos = viewPage->mapToScene(event->pos());
        Base::Vector3d pagePos(Rez::appX(scenePos.x()), -Rez::appX(scenePos.y()), 0.0);
        
        if (dlg == nullptr) {
            Gui::Control().closeDialog();
            currentState = PlacementState::None;
            return;
        }
        

        dlg->setViewPosition(pagePos);
    }
}

void TechDrawDetailViewHandler::mouseReleaseEvent(QMouseEvent* event)
{
    if (currentState == PlacementState::None) {
        if (Gui::Control().activeDialog() == dlg && dlg != nullptr) {
            Gui::Control().closeDialog();
        }
        dlg = nullptr;

        std::vector<App::DocumentObject*> baseObj =
            Gui::Selection().getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
        if (baseObj.empty()) {
            return;
        }
    
        if (!Gui::Selection().getObjectsOfType(TechDraw::DrawViewDetail::getClassTypeId()).empty()) {
            return;
        }
    
        Base::Vector3d placement(0.0, 0.0, 0.0);
    
        std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
        auto objFeat = dynamic_cast<TechDraw::DrawView*>(selection[0].getObject());
        if (!objFeat) {
            return;
        }
    
        TechDraw::DrawPage* page = objFeat->findParentPage();
        Gui::Document* guiDoc = Gui::Application::Instance->getDocument(page->getDocument());
        ViewProviderDrawingView* viewVP =
            freecad_cast<ViewProviderDrawingView*>(guiDoc->getViewProvider(objFeat));
        
        if (!viewVP) {
            return;
        }
        auto* view = dynamic_cast<QGIView*>(viewVP->getQView());
    
        if (!_checkDirectPlacement(view, Gui::Selection().getSelectionEx()[0].getSubNames(), placement)) {
            return;
        }
    
        TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(*(baseObj.begin()));
        dlg = new TaskDlgDetail(dvp);
        Gui::Control().showDialog(dlg);
    
        placement.y = -placement.y;
        dlg->setInitialAnchor(placement);
    
        Gui::Selection().clearSelection();
        currentState = PlacementState::PlacingDetail;
    }
    else if (currentState == PlacementState::PlacingDetail) {
        QPointF scenePos = viewPage->mapToScene(event->pos());
        Base::Vector3d pagePos = Base::Vector3d(Rez::appX(scenePos.x()), -Rez::appX(scenePos.y()), 0.0);
        
        if (dlg == nullptr) {
            Gui::Control().closeDialog();
            currentState = PlacementState::None;
            return;
        }

        dlg->setViewPosition(pagePos, true);
        dlg = nullptr;
        currentState = PlacementState::None;

        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }
}

void TechDrawDetailViewHandler::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawDetailViewHandler::keyPressEvent(QKeyEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawDetailViewHandler::addPreselected()
{
    std::vector<App::DocumentObject*> baseObj =
        Gui::Selection().getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
    if (baseObj.empty()) {
        return;
    }

    Base::Vector3d placement(0.0, 0.0, 0.0);

    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();

    if (!selection.empty()) {
        auto objFeat = dynamic_cast<TechDraw::DrawView*>(selection[0].getObject());

        if (objFeat) {
            TechDraw::DrawPage* page = objFeat->findParentPage();
            Gui::Document* guiDoc = Gui::Application::Instance->getDocument(page->getDocument());
            ViewProviderDrawingView* viewVP =
                freecad_cast<ViewProviderDrawingView*>(guiDoc->getViewProvider(objFeat));

            if (viewVP) {
                auto* view = dynamic_cast<QGIView*>(viewVP->getQView());
                _checkDirectPlacement(view, selection[0].getSubNames(), placement);
            }
        }
    }

    TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(*(baseObj.begin()));
    dlg = new TaskDlgDetail(dvp);
    Gui::Control().showDialog(dlg);

    placement.y = -placement.y;
    dlg->setInitialAnchor(placement);

    Gui::Selection().clearSelection();
    currentState = PlacementState::PlacingDetail;
}