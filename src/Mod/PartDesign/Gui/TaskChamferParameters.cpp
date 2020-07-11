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
# include <QAction>
# include <QFontMetrics>
# include <QKeyEvent>
# include <QListWidget>
# include <QMessageBox>
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
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/Tools.h>
#include <Mod/PartDesign/App/FeatureChamfer.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/Sketcher/App/SketchObject.h>


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskChamferParameters */

TaskChamferParameters::TaskChamferParameters(ViewProviderDressUp *DressUpView,QWidget *parent)
    : TaskDressUpParameters(DressUpView, true, true, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskChamferParameters();
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    PartDesign::Chamfer* pcChamfer = static_cast<PartDesign::Chamfer*>(DressUpView->getObject());

    setUpUI(pcChamfer);
    QMetaObject::invokeMethod(ui->chamferSize, "setFocus", Qt::QueuedConnection);

    QMetaObject::connectSlotsByName(this);

    connect(ui->chamferType, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onTypeChanged(int)));
    connect(ui->chamferSize, SIGNAL(valueChanged(double)),
        this, SLOT(onSizeChanged(double)));
    connect(ui->chamferSize2, SIGNAL(valueChanged(double)),
        this, SLOT(onSize2Changed(double)));
    connect(ui->chamferAngle, SIGNAL(valueChanged(double)),
        this, SLOT(onAngleChanged(double)));
    connect(ui->flipDirection, SIGNAL(toggled(bool)),
        this, SLOT(onFlipDirection(bool)));

    setup(ui->message, ui->listWidgetReferences, ui->buttonRefAdd);
}

void TaskChamferParameters::setUpUI(PartDesign::Chamfer* pcChamfer)
{
    const int index = pcChamfer->ChamferType.getValue();
    ui->chamferType->setCurrentIndex(index);

    ui->flipDirection->setEnabled(index != 0); // Enable if type is not "Equal distance"
    ui->flipDirection->setChecked(pcChamfer->FlipDirection.getValue());

    ui->chamferSize->setUnit(Base::Unit::Length);
    ui->chamferSize->setMinimum(0);
    ui->chamferSize->setValue(pcChamfer->Size.getValue());
    ui->chamferSize->bind(pcChamfer->Size);
    ui->chamferSize->selectNumber();

    ui->chamferSize2->setUnit(Base::Unit::Length);
    ui->chamferSize2->setMinimum(0);
    ui->chamferSize2->setValue(pcChamfer->Size2.getValue());
    ui->chamferSize2->bind(pcChamfer->Size2);

    ui->chamferAngle->setUnit(Base::Unit::Angle);
    ui->chamferAngle->setMinimum(0.0);
    ui->chamferAngle->setMaximum(180.0);
    ui->chamferAngle->setValue(pcChamfer->Angle.getValue());
    ui->chamferAngle->bind(pcChamfer->Angle);

    ui->stackedWidget->setFixedHeight(ui->chamferSize2->sizeHint().height());

    QFontMetrics fm(ui->typeLabel->font());
    int minWidth = Gui::QtTools::horizontalAdvance(fm, ui->typeLabel->text());
    minWidth = std::max<int>(minWidth, Gui::QtTools::horizontalAdvance(fm, ui->sizeLabel->text()));
    minWidth = std::max<int>(minWidth, Gui::QtTools::horizontalAdvance(fm, ui->size2Label->text()));
    minWidth = std::max<int>(minWidth, Gui::QtTools::horizontalAdvance(fm, ui->angleLabel->text()));
    minWidth = minWidth + 5; //spacing
    ui->typeLabel->setMinimumWidth(minWidth);
    ui->sizeLabel->setMinimumWidth(minWidth);
    ui->size2Label->setMinimumWidth(minWidth);
    ui->angleLabel->setMinimumWidth(minWidth);
}

void TaskChamferParameters::refresh()
{
    if(!DressUpView)
        return;

    TaskDressUpParameters::refresh();

    PartDesign::Chamfer* pcChamfer = static_cast<PartDesign::Chamfer*>(DressUpView->getObject());
    const int index = pcChamfer->ChamferType.getValue();
    {
        QSignalBlocker bocker(ui->chamferType);
        ui->chamferType->setCurrentIndex(index);
    }

    {
        ui->flipDirection->setEnabled(index != 0); // Enable if type is not "Equal distance"
        QSignalBlocker blocker(ui->flipDirection);
        ui->flipDirection->setChecked(pcChamfer->FlipDirection.getValue());
    }

    {
        QSignalBlocker blocker(ui->chamferSize);
        ui->chamferSize->setValue(pcChamfer->Size.getValue());
    }

    {
        QSignalBlocker blocker(ui->chamferSize2);
        ui->chamferSize2->setValue(pcChamfer->Size2.getValue());
    }

    {
        QSignalBlocker blocker(ui->chamferAngle);
        ui->chamferAngle->setValue(pcChamfer->Angle.getValue());
    }
}

void TaskChamferParameters::onTypeChanged(int index)
{
    PartDesign::Chamfer* pcChamfer = static_cast<PartDesign::Chamfer*>(DressUpView->getObject());
    pcChamfer->ChamferType.setValue(index);
    ui->stackedWidget->setCurrentIndex(index);
    ui->flipDirection->setEnabled(index != 0); // Enable if type is not "Equal distance"
    pcChamfer->getDocument()->recomputeFeature(pcChamfer);
}

void TaskChamferParameters::onSizeChanged(double len)
{
    if(!DressUpView)
        return;

    PartDesign::Chamfer* pcChamfer = static_cast<PartDesign::Chamfer*>(DressUpView->getObject());
    setupTransaction();
    pcChamfer->Size.setValue(len);
    recompute();
}

void TaskChamferParameters::onSize2Changed(double len)
{
    PartDesign::Chamfer* pcChamfer = static_cast<PartDesign::Chamfer*>(DressUpView->getObject());
    setupTransaction();
    pcChamfer->Size2.setValue(len);
    pcChamfer->getDocument()->recomputeFeature(pcChamfer);
}

void TaskChamferParameters::onAngleChanged(double angle)
{
    PartDesign::Chamfer* pcChamfer = static_cast<PartDesign::Chamfer*>(DressUpView->getObject());
    setupTransaction();
    pcChamfer->Angle.setValue(angle);
    pcChamfer->getDocument()->recomputeFeature(pcChamfer);
}

void TaskChamferParameters::onFlipDirection(bool flip)
{
    PartDesign::Chamfer* pcChamfer = static_cast<PartDesign::Chamfer*>(DressUpView->getObject());
    setupTransaction();
    pcChamfer->FlipDirection.setValue(flip);
    pcChamfer->getDocument()->recomputeFeature(pcChamfer);
}

int TaskChamferParameters::getType(void) const
{
    return ui->chamferType->currentIndex();
}

double TaskChamferParameters::getSize(void) const
{
    return ui->chamferSize->value().getValue();
}

double TaskChamferParameters::getSize2(void) const
{
    return ui->chamferSize2->value().getValue();
}

double TaskChamferParameters::getAngle(void) const
{
    return ui->chamferAngle->value().getValue();
}

bool TaskChamferParameters::getFlipDirection(void) const
{
    return ui->flipDirection->isChecked();
}

TaskChamferParameters::~TaskChamferParameters()
{
    Gui::Selection().rmvSelectionGate();
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
    if(!DressUpView)
        return;

    std::string name = DressUpView->getObject()->getNameInDocument();

    //Gui::Command::openCommand("Chamfer changed");

    PartDesign::Chamfer* pcChamfer = static_cast<PartDesign::Chamfer*>(DressUpView->getObject());

    const int chamfertype = pcChamfer->ChamferType.getValue();

    switch(chamfertype) {

        case 0: // "Equal distance"
            ui->chamferSize->apply();
            break;
        case 1: // "Two distances"
            ui->chamferSize->apply();
            ui->chamferSize2->apply();
            break;
        case 2: // "Distance and Angle"
            ui->chamferSize->apply();
            ui->chamferAngle->apply();
            break;
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgChamferParameters::TaskDlgChamferParameters(ViewProviderChamfer *DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter  = new TaskChamferParameters(DressUpView);

    Content.push_back(parameter);
}

TaskDlgChamferParameters::~TaskDlgChamferParameters()
{

}

//==== calls from the TaskView ===============================================================


//void TaskDlgChamferParameters::open()
//{
//    // a transaction is already open at creation time of the chamfer
//    if (!Gui::Command::hasPendingCommand()) {
//        QString msg = tr("Edit chamfer");
//        Gui::Command::openCommand((const char*)msg.toUtf8());
//    }
//}
bool TaskDlgChamferParameters::accept()
{
    parameter->showObject();
    parameter->apply();

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskChamferParameters.cpp"
