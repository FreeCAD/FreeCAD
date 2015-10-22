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

#include "ui_TaskChamferParameters.h"
#include "TaskChamferParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Base/UnitsApi.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeatureChamfer.h>
#include <Mod/Sketcher/App/SketchObject.h>


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskChamferParameters */

TaskChamferParameters::TaskChamferParameters(ViewProviderChamfer *ChamferView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("Part_Chamfer"),tr("Chamfer parameters"),true, parent),ChamferView(ChamferView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskChamferParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->chamferDistance, SIGNAL(valueChanged(double)),
            this, SLOT(onLengthChanged(double)));

    this->groupLayout()->addWidget(proxy);

    PartDesign::Chamfer* pcChamfer = static_cast<PartDesign::Chamfer*>(ChamferView->getObject());
    double r = pcChamfer->Size.getValue();

    ui->chamferDistance->setUnit(Base::Unit::Length);
    ui->chamferDistance->setValue(r);
    ui->chamferDistance->setMinimum(0);
    ui->chamferDistance->selectNumber();
    ui->chamferDistance->bind(pcChamfer->Size);
    QMetaObject::invokeMethod(ui->chamferDistance, "setFocus", Qt::QueuedConnection);
}

void TaskChamferParameters::onLengthChanged(double len)
{
    PartDesign::Chamfer* pcChamfer = static_cast<PartDesign::Chamfer*>(ChamferView->getObject());
    pcChamfer->Size.setValue(len);
    pcChamfer->getDocument()->recomputeFeature(pcChamfer);
}

double TaskChamferParameters::getLength(void) const
{
    return ui->chamferDistance->value().getValue();
}

TaskChamferParameters::~TaskChamferParameters()
{
    delete ui;
}

void TaskChamferParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskChamferParameters::apply()
{
    std::string name = ChamferView->getObject()->getNameInDocument();

    //Gui::Command::openCommand("Chamfer changed");
    ui->chamferDistance->apply();
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::commitCommand();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgChamferParameters::TaskDlgChamferParameters(ViewProviderChamfer *ChamferView)
    : TaskDialog(),ChamferView(ChamferView)
{
    assert(ChamferView);
    parameter  = new TaskChamferParameters(ChamferView);

    Content.push_back(parameter);
}

TaskDlgChamferParameters::~TaskDlgChamferParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgChamferParameters::open()
{
    // a transaction is already open at creation time of the chamfer
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = tr("Edit chamfer");
        Gui::Command::openCommand((const char*)msg.toUtf8());
    }
}

void TaskDlgChamferParameters::clicked(int)
{

}

bool TaskDlgChamferParameters::accept()
{
    parameter->apply();

    return true;
}

bool TaskDlgChamferParameters::reject()
{
    // get the support and Sketch
    PartDesign::Chamfer* pcChamfer = static_cast<PartDesign::Chamfer*>(ChamferView->getObject());
    App::DocumentObject    *pcSupport;
    pcSupport = pcChamfer->Base.getValue();

    // role back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    // if abort command deleted the object the support is visible again
    if (!Gui::Application::Instance->getViewProvider(pcChamfer)) {
        if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
            Gui::Application::Instance->getViewProvider(pcSupport)->show();
    }

    return true;
}



#include "moc_TaskChamferParameters.cpp"
