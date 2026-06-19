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

#include <memory>
#include <string>
#include <QWidget>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>

namespace TechDrawGui
{
namespace Ui
{
class TaskAddOffsetVertex;
}

class TaskAddOffsetVertex : public QWidget
{
    Q_OBJECT

public:
    TaskAddOffsetVertex(TechDraw::DrawViewPart* view, TechDraw::VertexPtr vertex, int projIndex);
    ~TaskAddOffsetVertex() override;

    bool accept();
    bool reject();

private Q_SLOTS:
    void offsetChanged();

private:
    std::unique_ptr<Ui::TaskAddOffsetVertex> ui;
    TechDraw::DrawViewPart* view;
    TechDraw::VertexPtr vertex;
    std::string previewVertex;
};

class TaskDlgAddOffsetVertex : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgAddOffsetVertex(TechDraw::DrawViewPart* view, TechDraw::VertexPtr vertex, int projIndex);
    ~TaskDlgAddOffsetVertex() override;

    bool accept() override;
    bool reject() override;

private:
    TaskAddOffsetVertex* widget;
    Gui::TaskView::TaskBox* taskbox;
};

}  // namespace TechDrawGui
