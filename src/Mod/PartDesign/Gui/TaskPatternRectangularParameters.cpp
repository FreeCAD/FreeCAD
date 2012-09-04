/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include "ui_TaskPatternRectangularParameters.h"
#include "TaskPatternRectangularParameters.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPatternRectangularParameters */

TaskPatternRectangularParameters::TaskPatternRectangularParameters(QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Parameters rectangular pattern"),true, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPatternRectangularParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    Gui::Selection().Attach(this);
}

TaskPatternRectangularParameters::~TaskPatternRectangularParameters()
{
    delete ui;
    Gui::Selection().Detach(this);
}

void TaskPatternRectangularParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

/// @cond DOXERR
void TaskPatternRectangularParameters::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                              Gui::SelectionSingleton::MessageType Reason)
{
    if (Reason.Type == SelectionChanges::AddSelection ||
        Reason.Type == SelectionChanges::RmvSelection ||
        Reason.Type == SelectionChanges::SetSelection ||
        Reason.Type == SelectionChanges::ClrSelection) {
    }
}
/// @endcond

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPatternRectangularParameters::TaskDlgPatternRectangularParameters(ViewProviderPatternRectangular *PatternRectangularView)
    : TaskDialog(),PatternRectangularView(PatternRectangularView)
{
    assert(PatternRectangularView);
    parameter  = new TaskPatternRectangularParameters();

    Content.push_back(parameter);
}

TaskDlgPatternRectangularParameters::~TaskDlgPatternRectangularParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgPatternRectangularParameters::open()
{

}

void TaskDlgPatternRectangularParameters::clicked(int)
{
    
}

bool TaskDlgPatternRectangularParameters::accept()
{
    return true;
}

bool TaskDlgPatternRectangularParameters::reject()
{
    Gui::Command::openCommand("PatternRectangular changed");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::commitCommand();

    return true;
}

void TaskDlgPatternRectangularParameters::helpRequested()
{

}




#include "moc_TaskPatternRectangularParameters.cpp"
