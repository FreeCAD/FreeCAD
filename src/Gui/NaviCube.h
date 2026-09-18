// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017 Kustaa Nyholm  <kustaa.nyholm@sparetimelabs.com>
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <CXX/Extensions.hxx>
#include <QColor>
#include <FCGlobal.h>

class SoEvent;
class SoNode;

namespace Gui
{
class View3DInventorViewer;
}

class NaviCubeImplementation;

class GuiExport NaviCube
{
public:
    enum Corner
    {
        TopLeftCorner,
        TopRightCorner,
        BottomLeftCorner,
        BottomRightCorner
    };
    NaviCube(Gui::View3DInventorViewer* viewer);
    virtual ~NaviCube();
    void createContextMenu(const std::vector<std::string>& cmd);
    bool processSoEvent(const SoEvent* ev);
    void setCorner(Corner);
    void setOffset(int x, int y);
    bool isDraggable();
    void updateColors();
    void setDraggable(bool draggable);
    void setSize(int size);
    void setChamfer(float size);
    void setNaviRotateToNearest(bool toNearest);
    void setNaviStepByTurn(int steps);
    void setFont(std::string font);
    void setFontWeight(int weight);
    void setFontStretch(int stretch);
    void setFontZoom(float zoom);
    void setBaseColor(QColor TextColor);
    void setEmphaseColor(QColor ButtonColor);
    void setHiliteColor(QColor HiliteColor);
    void setBorderWidth(double BorderWidth);
    void setShowCS(bool showCS);
    void setInactiveOpacity(float opacity);
    // Label order: front, top, right, rear, bottom, left
    void setNaviCubeLabels(const std::vector<std::string>& labels);
    static void setNaviCubeCommands(const std::vector<std::string>& cmd);
    static int getNaviCubeSize();
    SoNode* getCoinNode() const;

private:
    NaviCubeImplementation* naviCubeImplementation;
};
