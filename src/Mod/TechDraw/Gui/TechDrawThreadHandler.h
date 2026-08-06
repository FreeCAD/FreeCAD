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
#include <list>
#include <string>

#include <Gui/InputHint.h>
#include <Mod/TechDraw/App/DrawView.h>

#include "TechDrawHandler.h"

class QMouseEvent;
class QKeyEvent;

class TechDrawThreadHandler : public TechDrawGui::TechDrawHandler
{
public:
    explicit TechDrawThreadHandler(std::string commandName)
        : TechDrawHandler(), m_commandName(commandName)
    {}

    void activated() override;
    void deactivated() override;
    std::list<Gui::InputHint> getToolHints() const override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    void addPreselected();

private:
    std::string m_commandName;
    QCursor cursor;

    std::string previewTag;
    std::vector<std::string> previewSubNames;
    TechDraw::DrawViewPart* previewObjFeat{nullptr};
};
