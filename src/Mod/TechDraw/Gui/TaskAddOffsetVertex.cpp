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

#include "PreCompiled.h"

#include "TaskAddOffsetVertex.h"
#include "ui_TaskAddOffsetVertex.h"

#include <Gui/BitmapFactory.h>

#include <Mod/TechDraw/App/CosmeticVertex.h>
#include <Mod/TechDraw/App/DrawUtil.h>

using namespace TechDrawGui;

TaskAddOffsetVertex::TaskAddOffsetVertex(TechDraw::DrawViewPart* view, TechDraw::VertexPtr vertex, int projIndex)
    : ui(new Ui::TaskAddOffsetVertex)
    , view(view)
    , vertex(vertex)
{
    ui->setupUi(this);

    connect(ui->dSpinBoxX, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &TaskAddOffsetVertex::offsetChanged);
    connect(ui->dSpinBoxY, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &TaskAddOffsetVertex::offsetChanged);

    ui->le_SourceVertex->setText(QString("Vertex %1").arg(projIndex));
}

TaskAddOffsetVertex::~TaskAddOffsetVertex()
{
    if (!previewVertex.empty()) {
        view->removeCosmeticVertex(previewVertex);
        view->refreshCVGeoms();
        view->requestPaint();
    }
}

void TaskAddOffsetVertex::offsetChanged()
{
    Base::Vector3d point = TechDraw::CosmeticVertex::makeCanonicalPoint(view, vertex->point(), false);
    Base::Vector3d offset(ui->dSpinBoxX->value(), ui->dSpinBoxY->value(), 0.0);

    if (!previewVertex.empty()) {
        view->removeCosmeticVertex(previewVertex);
    }
    previewVertex = view->addCosmeticVertex(point + offset);
    view->refreshCVGeoms();
    view->requestPaint();
}

bool TaskAddOffsetVertex::accept()
{
    if (previewVertex.empty()) {
        Base::Vector3d point = TechDraw::CosmeticVertex::makeCanonicalPoint(view, vertex->point(), false);
        Base::Vector3d offset(ui->dSpinBoxX->value(), ui->dSpinBoxY->value(), 0.0);
        view->addCosmeticVertex(point + offset);
        view->refreshCVGeoms();
    }
    previewVertex.clear();
    view->requestPaint();
    return true;
}

bool TaskAddOffsetVertex::reject()
{
    if (!previewVertex.empty()) {
        view->removeCosmeticVertex(previewVertex);
        view->refreshCVGeoms();
        previewVertex.clear();
        view->requestPaint();
    }
    return true;
}

TaskDlgAddOffsetVertex::TaskDlgAddOffsetVertex(TechDraw::DrawViewPart* view, TechDraw::VertexPtr vertex, int projIndex)
    : TaskDialog()
{
    widget = new TaskAddOffsetVertex(view, vertex, projIndex);
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("actions/TechDraw_CosmeticVertex"),
        widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgAddOffsetVertex::~TaskDlgAddOffsetVertex() = default;

bool TaskDlgAddOffsetVertex::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgAddOffsetVertex::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskAddOffsetVertex.cpp>
