// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#include <GeomAbs_Shape.hxx>

#include "TaskPatchOnMesh.h"
#include "ui_TaskPatchOnMesh.h"
#include "PatchOnMesh.h"

#include <Gui/View3DInventor.h>

using namespace MeshPartGui;

PatchOnMeshWidget::PatchOnMeshWidget(Gui::View3DInventor* view, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_TaskPatchOnMesh())
    , myGridHandler(new PatchOnMeshHandler(this))
    , myView(view)
{
    ui->setupUi(this);
    connect(ui->startButton, &QPushButton::clicked, this, &PatchOnMeshWidget::onStartButtonClicked);
    this->setup();
}

PatchOnMeshWidget::~PatchOnMeshWidget()
{
    ui.reset();
}

void PatchOnMeshWidget::setup()
{
    ui->continuity->addItem(QStringLiteral("C0"), static_cast<int>(GeomAbs_C0));
    ui->continuity->addItem(QStringLiteral("C1"), static_cast<int>(GeomAbs_C1));
    ui->continuity->addItem(QStringLiteral("C2"), static_cast<int>(GeomAbs_C2));
    ui->continuity->addItem(QStringLiteral("C3"), static_cast<int>(GeomAbs_C3));
    ui->continuity->setCurrentIndex(2);

    const int maxDegree = 8;
    for (int i = 0; i < maxDegree; i++) {
        ui->maxDegree->addItem(QString::number(i + 1));
    }
    ui->maxDegree->setCurrentIndex(4);
}

void PatchOnMeshWidget::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void PatchOnMeshWidget::onStartButtonClicked()
{
    const int index = ui->continuity->currentIndex();
    const int cont = ui->continuity->itemData(index).toInt();
    myGridHandler->setParameters(
        ui->maxDegree->currentIndex() + 1,
        static_cast<GeomAbs_Shape>(cont),
        ui->meshTolerance->value()
    );
    myGridHandler->enableCallback(myView);
}

void PatchOnMeshWidget::reject()
{
    myGridHandler->recomputeDocument();
}

// ----------------------------------------------------------------------------

TaskPatchOnMesh::TaskPatchOnMesh(Gui::View3DInventor* view)
    : widget {new PatchOnMeshWidget(view)}
{
    addTaskBox(widget);
}

bool TaskPatchOnMesh::reject()
{
    widget->reject();
    return true;
}

#include "moc_TaskPatchOnMesh.cpp"
