// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <Base/Tools.h>

#include <QAbstractItemView>
#include <QAction>
#include <QKeySequence>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStringList>
#include <QVBoxLayout>

#include <App/Document.h>
#include <Gui/BitmapFactory.h>

#include <Mod/Sketcher3D/App/Constraint3D.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "TaskSketcher3DConstraints.h"
#include "TaskSketcher3DPanelHelpers.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;

TaskSketcher3DConstraints::TaskSketcher3DConstraints(ViewProviderSketch3D* view)
    : TaskBox(Gui::BitmapFactory().pixmap("Sketcher_CreateLineAngleLength"), tr("Constraints"), true, nullptr)
    , sketchView(view)
{
    auto* body = new QWidget(this);
    auto* root = new QVBoxLayout(body);
    root->setContentsMargins(0, 0, 0, 0);

    constraintsList = new QListWidget(body);
    constraintsList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    constraintsList->setUniformItemSizes(true);
    constraintsList->setAlternatingRowColors(true);
    constraintsList->setMinimumHeight(80);
    constraintsList->setContextMenuPolicy(Qt::ActionsContextMenu);

    deleteAction = new QAction(tr("Delete"), constraintsList);
    deleteAction->setShortcut(QKeySequence::Delete);
    deleteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    constraintsList->addAction(deleteAction);
    connect(deleteAction, &QAction::triggered, this, &TaskSketcher3DConstraints::onDeleteTriggered);

    root->addWidget(constraintsList);
    addWidget(body, true, false);

    connectionConstraintsChanged = sketchView->signalConstraintsChanged.connect([this]() {
        refresh();
    });
    refresh();
}

TaskSketcher3DConstraints::~TaskSketcher3DConstraints()
{
    connectionConstraintsChanged.disconnect();
}

void TaskSketcher3DConstraints::refresh()
{
    constraintsList->clear();

    auto* sketch = sketchView->getSketch3DObject();
    auto& cs = sketch->Constraints.getConstraints();
    std::map<int, int> pointLabels;
    std::map<int, int> lineLabels;
    PanelHelpers::buildGeometryLabels(sketch->Geometry.getValues(), pointLabels, lineLabels);

    for (std::size_t i = 0; i < cs.size(); ++i) {
        auto& c = cs[i];
        QStringList refs;
        for (auto& r : c.getElements()) {
            refs << PanelHelpers::displayNameForRef(r, pointLabels, lineLabels);
        }

        QString text = tr("%1  %2  [%3]")
                           .arg(static_cast<int>(i + 1))
                           .arg(QString::fromUtf8(Sketcher3D::Constraint3D::typeToString(c.Type)))
                           .arg(refs.join(QStringLiteral(", ")));

        if (c.Type == Sketcher3D::Constraint3D::Distance3D
            || c.Type == Sketcher3D::Constraint3D::DistanceX3D
            || c.Type == Sketcher3D::Constraint3D::DistanceY3D
            || c.Type == Sketcher3D::Constraint3D::DistanceZ3D
            || c.Type == Sketcher3D::Constraint3D::Radius3D) {
            text += tr(" = %1").arg(c.Value, 0, 'f', 3);
        }
        else if (c.Type == Sketcher3D::Constraint3D::Angle3D) {
            text += tr(" = %1 deg").arg(Base::toDegrees(c.Value), 0, 'f', 3);
        }

        auto* item
            = new QListWidgetItem(PanelHelpers::iconForConstraint(c.Type), text, constraintsList);
        item->setData(Qt::UserRole, static_cast<int>(i));
    }

    setHeaderText(tr("Constraints (%1)").arg(constraintsList->count()));
}

void TaskSketcher3DConstraints::onDeleteTriggered()
{
    std::vector<int> indices;
    for (auto* item : constraintsList->selectedItems()) {
        indices.push_back(item->data(Qt::UserRole).toInt());
    }
    if (indices.empty()) {
        return;
    }

    auto* sketch = sketchView->getSketch3DObject();
    App::Document* doc = sketch->getDocument();
    doc->openTransaction(QT_TRANSLATE_NOOP("Command", "Delete 3D sketch constraints"));
    if (sketch->delConstraints(std::move(indices)) < 0) {
        doc->abortTransaction();
        return;
    }
    sketch->recomputeFeature();
    doc->commitTransaction();
}

#include "moc_TaskSketcher3DConstraints.cpp"
