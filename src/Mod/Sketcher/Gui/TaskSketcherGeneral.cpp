/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef _PreComp_
#endif

#include "ui_TaskSketcherGeneral.h"
#include "TaskSketcherGeneral.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/UnitsApi.h>

#include "ViewProviderSketch.h"

using namespace SketcherGui;
using namespace Gui::TaskView;

TaskSketcherGeneral::TaskSketcherGeneral(ViewProviderSketch *sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Edit controls"),true, 0)
    , sketchView(sketchView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskSketcherGeneral();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    // connecting the needed signals
    QObject::connect(
        ui->checkBoxGridSnap, SIGNAL(stateChanged(int)),
        this              , SLOT  (toggleGridSnap(int))
       );

    QObject::connect(
        ui->comboBoxGridSize, SIGNAL(currentIndexChanged(QString)),
        this              , SLOT  (setGridSize(QString))
       );

    QObject::connect(
        ui->checkBoxAutoconstraints, SIGNAL(stateChanged(int)),
        this              , SLOT  (toggleAutoconstraints(int))
       );
    
    Gui::Selection().Attach(this);
}

TaskSketcherGeneral::~TaskSketcherGeneral()
{
    delete ui;
    Gui::Selection().Detach(this);
}

void TaskSketcherGeneral::setGridSize(const QString& val)
{
    float gridSize = (float) Base::UnitsApi::translateUnit(val);
    if (gridSize > 0)
        sketchView->GridSize.setValue(gridSize);
}

void TaskSketcherGeneral::toggleGridSnap(int state)
{
    setGridSize(ui->comboBoxGridSize->currentText()); // Ensure consistency
    sketchView->GridSnap.setValue(state == Qt::Checked);
}

void TaskSketcherGeneral::toggleAutoconstraints(int state)
{
    sketchView->Autoconstraints.setValue(state == Qt::Checked);
}

void TaskSketcherGeneral::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

/// @cond DOXERR
void TaskSketcherGeneral::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                              Gui::SelectionSingleton::MessageType Reason)
{
    //if (Reason.Type == SelectionChanges::AddSelection ||
    //    Reason.Type == SelectionChanges::RmvSelection ||
    //    Reason.Type == SelectionChanges::SetSelection ||
    //    Reason.Type == SelectionChanges::ClrSelection) {
    //}
}
/// @endcond DOXERR

#include "moc_TaskSketcherGeneral.cpp"
