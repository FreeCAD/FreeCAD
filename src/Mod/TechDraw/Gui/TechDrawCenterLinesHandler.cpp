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


#include "TechDrawCenterLinesHandler.h"

#include <QMouseEvent>
#include <QKeyEvent>

#include <Gui/Control.h>
#include <Gui/Application.h>

#include "QGVPage.h"
#include "QGIEdge.h"
#include "QGIVertex.h"
#include "QGIFace.h"
#include "QGIView.h"
#include "TaskCenterLine.h"
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>


using namespace TechDrawGui;
using namespace TechDraw;

void execCenterLine(TechDraw::DrawPage* page) {
    
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();

    if (selection.empty()) {
        return;
    }

    std::vector<std::string> subNames = selection[0].getSubNames();
    TechDraw::DrawViewPart* viewPart = dynamic_cast<TechDraw::DrawViewPart*>(selection[0].getObject());

    if (!viewPart || subNames.empty()) {
        return;
    }

    int edgeCount = 0;
    int vertexCount = 0;

    for (const auto& name : subNames) {
        std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(name);
        if (geomType == "Edge") { 
            edgeCount++;
        }
        else if (geomType == "Vertex") { 
            vertexCount++;
        }
        else if (geomType == "Face") { 
            Gui::Control().showDialog(new TaskDlgCenterLine(viewPart,
                                                        page,
                                                        subNames,
                                                        false));
            return;
        }
    }

    if (vertexCount == 2 || edgeCount == 2) {
        Gui::Control().showDialog(new TaskDlgCenterLine(viewPart,
                                                    page,
                                                    subNames,
                                                    false));
        Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::NormalSelection);
        Gui::Selection().clearSelection();
        return;
    }
    else if (vertexCount == 1 || edgeCount == 1) {
        Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::GreedySelection);
    }
}

void TechDrawCenterLinesHandler::activated()
{
    if (viewPage) {
        QPoint hotspot(15, 15);
        QPixmap pixmap = viewPage->prepareCursorPixmap("actions/TechDraw_CenterLine_Pointer.svg", hotspot);
        cursor = QCursor(pixmap, hotspot.x(), hotspot.y());
        if (auto vp = viewPage->viewport()) {
            vp->setCursor(cursor);
        }
    }
}

void TechDrawCenterLinesHandler::deactivated()
{
    if (viewPage) {
        if (auto vp = viewPage->viewport()) {
            vp->unsetCursor();
        }
    }

    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::NormalSelection);
    Gui::Selection().clearSelection();
}

void TechDrawCenterLinesHandler::mouseMoveEvent(QMouseEvent* event)
{
    if (auto vp = viewPage->viewport()) {
        vp->setCursor(cursor);
    }
}

void TechDrawCenterLinesHandler::mouseReleaseEvent(QMouseEvent* event)
{
    if (Gui::Control().activeDialog()) {
        return;
    }

    execCenterLine(getPage());

}

void TechDrawCenterLinesHandler::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawCenterLinesHandler::keyPressEvent(QKeyEvent* event)
{
    Q_UNUSED(event);
}

void TechDrawCenterLinesHandler::addPreselected() {
    execCenterLine(getPage());
}