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
#include <QCursor>
#include <functional>
#include <list>
#include <string>

#include <Gui/InputHint.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>

#include "TechDrawHandler.h"

class QMouseEvent;
class QKeyEvent;

namespace TechDrawGui
{
class QGIView;
}

class TechDrawLeaderLineHandler : public TechDrawGui::TechDrawHandler
{
private:
    QCursor cursor;
    
    TechDraw::DrawLeaderLine* leaderLineObj = nullptr;
    
    std::vector<Base::Vector3d> waypoints;
    TechDraw::DrawView* baseFeat = nullptr;
    TechDrawGui::QGIView* baseFeatView = nullptr;
    bool finished = false;

public:
    enum class LeaderType {
        Normal,
        Straight,
        Complex,
        ComplexWithKink,
    };
    bool reverse = false;
    bool attached = false;
    std::function<void()> onFinished;
    std::vector<QPointF> sceneClickPoints;
    LeaderType currentLeaderType = LeaderType::Normal;
    LeaderType oldLeaderType = LeaderType::Normal;

    Base::Color color;
    int lineStyle = 1;
    double lineWidth = 0.5;
    long startSymbol = 0;
    long endSymbol = 7;
    double kinkLength = 5.0;

    void activated() override;
    void deactivated() override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    void addLeaderLine(TechDraw::DrawPage* basePage);
    void deleteLeaderLine();
    void createLeaderLineObject();

    TechDraw::DrawLeaderLine* getLeaderLine() { return leaderLineObj; }

    void updateLeader();
    void updateLeaderPoints(std::vector<QPointF> points = {});

    std::vector<Base::Vector3d> updateKinkLength(std::vector<Base::Vector3d> waypoints);
    std::vector<QPointF> updateKinkLength(std::vector<QPointF> scenePoints);
};