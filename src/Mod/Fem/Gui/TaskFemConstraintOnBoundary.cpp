/***************************************************************************
 *   Copyright (c) 2021 FreeCAD Developers                                 *
 *   Author: Ajinkya Dahale <dahale.a.p@gmail.com>                         *
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

#include "TaskFemConstraintOnBoundary.h"


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintOnBoundary */

TaskFemConstraintOnBoundary::TaskFemConstraintOnBoundary(ViewProviderFemConstraint* ConstraintView,
                                                         QWidget* parent,
                                                         const char* pixmapname)
    : TaskFemConstraint(ConstraintView, parent, pixmapname)
    , selChangeMode(SelectionChangeModes::none)
{
    ConstraintView->highlightReferences(true);

    buttonGroup = new ButtonGroup(this);
    buttonGroup->setExclusive(true);

    connect(buttonGroup,
            qOverload<QAbstractButton*, bool>(&QButtonGroup::buttonToggled),
            this,
            &TaskFemConstraintOnBoundary::onButtonToggled);
}

TaskFemConstraintOnBoundary::~TaskFemConstraintOnBoundary()
{
    if (!ConstraintView.expired()) {
        ConstraintView->highlightReferences(false);
    }
}
void TaskFemConstraintOnBoundary::onButtonToggled(QAbstractButton* button, bool checked)
{
    auto mode = static_cast<SelectionChangeModes>(buttonGroup->id(button));

    Gui::Selection().clearSelection();

    if (checked) {
        selChangeMode = mode;
        ConstraintView->highlightReferences(true);
    }
    else {
        if (selChangeMode == mode) {
            selChangeMode = SelectionChangeModes::none;
        }
    }
}

void TaskFemConstraintOnBoundary::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        switch (selChangeMode) {
            case SelectionChangeModes::refAdd:
                // TODO: Optimize to just perform actions on the newly selected item. Suggestion
                // from PartDesign: ui->lw_references->addItem(makeRefText(msg.pObjectName,
                // msg.pSubName));
                this->addToSelection();
                break;
            case SelectionChangeModes::refRemove:
                this->removeFromSelection();
                break;
            case SelectionChangeModes::none:
                return;
            default:
                return;
        }
        ConstraintView->highlightReferences(true);
    }
}

#include "moc_TaskFemConstraintOnBoundary.cpp"
