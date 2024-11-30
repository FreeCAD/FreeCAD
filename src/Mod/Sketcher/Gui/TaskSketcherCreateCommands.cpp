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
#include <QEvent>
#endif

#include <Gui/BitmapFactory.h>

#include "TaskSketcherCreateCommands.h"


using namespace Gui::TaskView;

TaskSketcherCreateCommands::TaskSketcherCreateCommands(QWidget* parent)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"), tr("Appearance"), true, parent)
{
    // we need a separate container widget to add all controls to
    // proxy = new QWidget(this);
    // ui = new Ui_TaskAppearance();
    // ui->setupUi(proxy);
    // ui->textLabel1_3->hide();
    // ui->changePlot->hide();
    // QMetaObject::connectSlotsByName(this);

    // this->groupLayout()->addWidget(proxy);

    // std::vector<Gui::ViewProvider*> views;
    // setDisplayModes(views);
    // setPointSize(views);
    // setLineWidth(views);
    // setTransparency(views);
    Gui::Selection().Attach(this);
}

TaskSketcherCreateCommands::~TaskSketcherCreateCommands()
{
    // delete ui;
    Gui::Selection().Detach(this);
}

void TaskSketcherCreateCommands::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    // if (e->type() == QEvent::LanguageChange) {
    //     ui->retranslateUi(proxy);
    // }
}

/// @cond DOXERR
void TaskSketcherCreateCommands::OnChange(Gui::SelectionSingleton::SubjectType& rCaller,
                                          Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller);
    if (Reason.Type == SelectionChanges::AddSelection
        || Reason.Type == SelectionChanges::RmvSelection
        || Reason.Type == SelectionChanges::SetSelection
        || Reason.Type == SelectionChanges::ClrSelection) {}
}
/// @endcond DOXERR


#include "moc_TaskSketcherCreateCommands.cpp"
