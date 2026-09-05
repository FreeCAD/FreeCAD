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

#pragma once

#include <QPointF>
#include <list>
#include <string>
#include <QCursor>

#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>


#include "QGIVertex.h"

#include "TechDrawHandler.h"

class QMouseEvent;
class QKeyEvent;

class TechDrawVertexHandler : public TechDrawGui::TechDrawHandler
{
private:
    enum class State
    {
        None,
        Offset,
    };

    State currentState = State::None;
    std::vector<std::string> previewNames;
    bool isPreviewing = false;
    std::vector<int> previewGeoIds;
    TechDraw::DrawViewPart* previewViewPart = nullptr;
    TechDrawGui::QGIVertex* hoveredVertex = nullptr;
    TechDraw::DrawViewPart* vertexViewPart = nullptr;

    QCursor cursor;

public:
    void activated() override;
    void deactivated() override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    void addPreselected();
};
