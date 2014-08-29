/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "ui_TaskSketcherMessages.h"
#include "TaskSketcherMessages.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>

#include <boost/bind.hpp>

#include "ViewProviderSketch.h"

using namespace SketcherGui;
using namespace Gui::TaskView;

TaskSketcherMessages::TaskSketcherMessages(ViewProviderSketch *sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Solver messages"),true, 0)
    , sketchView(sketchView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskSketcherMessages();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    connectionSetUp = sketchView->signalSetUp.connect(boost::bind(&SketcherGui::TaskSketcherMessages::slotSetUp, this,_1));
    connectionSolved = sketchView->signalSolved.connect(boost::bind(&SketcherGui::TaskSketcherMessages::slotSolved, this,_1));
    
    ui->labelConstrainStatus->setOpenExternalLinks(false);
    
    QObject::connect(
        ui->labelConstrainStatus, SIGNAL(linkActivated(const QString &)),
        this                     , SLOT  (on_labelConstrainStatus_linkActivated(const QString &))
       );
}

TaskSketcherMessages::~TaskSketcherMessages()
{
    connectionSetUp.disconnect();
    connectionSolved.disconnect();
    delete ui;
}

void TaskSketcherMessages::slotSetUp(QString msg)
{
    ui->labelConstrainStatus->setText(msg);
}

void TaskSketcherMessages::slotSolved(QString msg)
{
    ui->labelSolverStatus->setText(msg);
}

void TaskSketcherMessages::on_labelConstrainStatus_linkActivated(const QString &str)
{
    if( str == QString::fromLatin1("#conflicting"))
        Gui::Application::Instance->commandManager().runCommandByName("Sketcher_SelectConflictingConstraints");
    
    if( str == QString::fromLatin1("#redundant"))
        Gui::Application::Instance->commandManager().runCommandByName("Sketcher_SelectRedundantConstraints");            
}


#include "moc_TaskSketcherMessages.cpp"
