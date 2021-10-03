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

#ifndef _PreComp_
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Geom_Line.hxx>
# include <Geom_Plane.hxx>
# include <Precision.hxx>

# include <QAction>
# include <QKeyEvent>
# include <QMessageBox>
# include <QRegExp>
# include <QTextStream>

# include <TopoDS.hxx>
# include <gp_Ax1.hxx>
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>
# include <sstream>
#endif

#include "TaskFemConstraintOnBoundary.h"
#include <App/Application.h>
#include <Base/Tools.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>


using namespace FemGui;
using namespace Gui;

/* TRANSLATOR FemGui::TaskFemConstraintOnBoundary */

TaskFemConstraintOnBoundary::TaskFemConstraintOnBoundary(ViewProviderFemConstraint *ConstraintView, QWidget *parent, const char* pixmapname)
    : TaskFemConstraint(ConstraintView, parent, pixmapname)
{
}

TaskFemConstraintOnBoundary::~TaskFemConstraintOnBoundary()
{
}

void TaskFemConstraintOnBoundary::_addToSelection(bool checked)
{
    if (checked)
    {
        const auto& selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
        if (selection.empty()) {
            this->clearButtons(refAdd);
            selChangeMode = refAdd;
        } else {
            this->addToSelection();
            clearButtons(none);
        }
    } else {
        exitSelectionChangeMode();
    }
}

void TaskFemConstraintOnBoundary::_removeFromSelection(bool checked)
{
    if (checked)
    {
        const auto& selection = Gui::Selection().getSelectionEx(); //gets vector of selected objects of active document
        if (selection.empty()) {
            this->clearButtons(refRemove);
            selChangeMode = refRemove;
        } else {
            this->removeFromSelection();
            clearButtons(none);
        }
    } else {
        exitSelectionChangeMode();
    }
}

void TaskFemConstraintOnBoundary::onSelectionChanged(const Gui::SelectionChanges&msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        switch (selChangeMode) {
        case refAdd:
            // TODO: Optimize to just perform actions on the newly selected item. Suggestion from PartDesign:
            // ui->lw_references->addItem(makeRefText(msg.pObjectName, msg.pSubName));
            this->addToSelection();
            break;
        case refRemove:
            this->removeFromSelection();
            break;
        case none:
            return;
        default:
            return;
        }
    }
}

void TaskFemConstraintOnBoundary::exitSelectionChangeMode()
{
    selChangeMode = none;
    Gui::Selection().clearSelection();
}

#include "moc_TaskFemConstraintOnBoundary.cpp"
