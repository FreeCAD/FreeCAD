/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "ui_TaskGrooveParameters.h"
#include "TaskGrooveParameters.h"
#include <Base/UnitsApi.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/Sketcher/App/SketchObject.h>


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskGrooveParameters */

TaskGrooveParameters::TaskGrooveParameters(ViewProviderGroove *GrooveView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("PartDesign_Groove"),tr("Groove parameters"),true, parent),GrooveView(GrooveView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskGrooveParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->doubleSpinBox, SIGNAL(valueChanged(double)),
            this, SLOT(onAngleChanged(double)));
    connect(ui->axis, SIGNAL(activated(int)),
            this, SLOT(onAxisChanged(int)));
    connect(ui->checkBoxMidplane, SIGNAL(toggled(bool)),
            this, SLOT(onMidplane(bool)));
    connect(ui->checkBoxReversed, SIGNAL(toggled(bool)),
            this, SLOT(onReversed(bool)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature updates
    ui->doubleSpinBox->blockSignals(true);
    ui->axis->blockSignals(true);
    ui->checkBoxMidplane->blockSignals(true);
    ui->checkBoxReversed->blockSignals(true);

    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(GrooveView->getObject());
    double l = pcGroove->Angle.getValue();
    bool mirrored = pcGroove->Midplane.getValue();
    bool reversed = pcGroove->Reversed.getValue();

    ui->doubleSpinBox->setDecimals(Base::UnitsApi::getDecimals());
    ui->doubleSpinBox->setValue(l);

    int count=pcGroove->getSketchAxisCount();

    for (int i=ui->axis->count()-1; i >= count+2; i--)
        ui->axis->removeItem(i);
    for (int i=ui->axis->count(); i < count+2; i++)
        ui->axis->addItem(QString::fromAscii("Sketch axis %1").arg(i-2));

    int pos=-1;

    App::DocumentObject *pcReferenceAxis = pcGroove->ReferenceAxis.getValue();
    const std::vector<std::string> &subReferenceAxis = pcGroove->ReferenceAxis.getSubValues();
    if (pcReferenceAxis && pcReferenceAxis == pcGroove->Sketch.getValue()) {
        assert(subReferenceAxis.size()==1);
        if (subReferenceAxis[0] == "V_Axis")
            pos = 0;
        else if (subReferenceAxis[0] == "H_Axis")
            pos = 1;
        else if (subReferenceAxis[0].size() > 4 && subReferenceAxis[0].substr(0,4) == "Axis")
            pos = 2 + std::atoi(subReferenceAxis[0].substr(4,4000).c_str());
    }

    if (pos < 0 || pos >= ui->axis->count()) {
        ui->axis->addItem(QString::fromAscii("Undefined"));
        pos = ui->axis->count()-1;
    }

    ui->axis->setCurrentIndex(pos);

    ui->checkBoxMidplane->setChecked(mirrored);
    ui->checkBoxReversed->setChecked(reversed);

    ui->doubleSpinBox->blockSignals(false);
    ui->axis->blockSignals(false);
    ui->checkBoxMidplane->blockSignals(false);
    ui->checkBoxReversed->blockSignals(false);

    setFocus ();
}

void TaskGrooveParameters::onAngleChanged(double len)
{
    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(GrooveView->getObject());
    pcGroove->Angle.setValue(len);
    if (updateView())
        pcGroove->getDocument()->recomputeFeature(pcGroove);
}

void TaskGrooveParameters::onAxisChanged(int num)
{
    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(GrooveView->getObject());
    Sketcher::SketchObject *pcSketch = static_cast<Sketcher::SketchObject*>(pcGroove->Sketch.getValue());
    if (pcSketch) {
        App::DocumentObject *oldRefAxis = pcGroove->ReferenceAxis.getValue();
        std::vector<std::string> oldSubRefAxis = pcGroove->ReferenceAxis.getSubValues();

        int maxcount = pcSketch->getAxisCount()+2;
        if (num == 0)
            pcGroove->ReferenceAxis.setValue(pcSketch, std::vector<std::string>(1,"V_Axis"));
        else if (num == 1)
            pcGroove->ReferenceAxis.setValue(pcSketch, std::vector<std::string>(1,"H_Axis"));
        else if (num >= 2 && num < maxcount) {
            QString buf = QString::fromUtf8("Axis%1").arg(num-2);
            std::string str = buf.toStdString();
            pcGroove->ReferenceAxis.setValue(pcSketch, std::vector<std::string>(1,str));
        }
        if (num < maxcount && ui->axis->count() > maxcount)
            ui->axis->setMaxCount(maxcount);

        const std::vector<std::string> &newSubRefAxis = pcGroove->ReferenceAxis.getSubValues();
        if (oldRefAxis != pcSketch ||
            oldSubRefAxis.size() != newSubRefAxis.size() ||
            oldSubRefAxis[0] != newSubRefAxis[0]) {
            bool reversed = pcGroove->suggestReversed();
            if (reversed != pcGroove->Reversed.getValue()) {
                pcGroove->Reversed.setValue(reversed);
                ui->checkBoxReversed->blockSignals(true);
                ui->checkBoxReversed->setChecked(reversed);
                ui->checkBoxReversed->blockSignals(false);
            }
        }
    }
    if (updateView())
        pcGroove->getDocument()->recomputeFeature(pcGroove);
}

void TaskGrooveParameters::onMidplane(bool on)
{
    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(GrooveView->getObject());
    pcGroove->Midplane.setValue(on);
    if (updateView())
        pcGroove->getDocument()->recomputeFeature(pcGroove);
}

void TaskGrooveParameters::onReversed(bool on)
{
    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(GrooveView->getObject());
    pcGroove->Reversed.setValue(on);
    if (updateView())
        pcGroove->getDocument()->recomputeFeature(pcGroove);
}

void TaskGrooveParameters::onUpdateView(bool on)
{
    if (on) {
        PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(GrooveView->getObject());
        pcGroove->getDocument()->recomputeFeature(pcGroove);
    }
}

double TaskGrooveParameters::getAngle(void) const
{
    return ui->doubleSpinBox->value();
}

QString TaskGrooveParameters::getReferenceAxis(void) const
{
    // get the support and Sketch
    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(GrooveView->getObject());
    Sketcher::SketchObject *pcSketch = static_cast<Sketcher::SketchObject*>(pcGroove->Sketch.getValue());

    QString buf;
    if (pcSketch) {
        buf = QString::fromUtf8("(App.ActiveDocument.%1,[%2])");
        buf = buf.arg(QString::fromUtf8(pcSketch->getNameInDocument()));
        if (ui->axis->currentIndex() == 0)
            buf = buf.arg(QString::fromUtf8("'V_Axis'"));
        else if (ui->axis->currentIndex() == 1)
            buf = buf.arg(QString::fromUtf8("'H_Axis'"));
        else if (ui->axis->currentIndex() >= 2) {
            buf = buf.arg(QString::fromUtf8("'Axis%1'"));
            buf = buf.arg(ui->axis->currentIndex()-2);
        }
    }
    else
        buf = QString::fromUtf8("''");

    return buf;
}

bool   TaskGrooveParameters::getMidplane(void) const
{
    return ui->checkBoxMidplane->isChecked();
}

bool   TaskGrooveParameters::getReversed(void) const
{
    return ui->checkBoxReversed->isChecked();
}

const bool TaskGrooveParameters::updateView() const
{
    return ui->checkBoxUpdateView->isChecked();
}

TaskGrooveParameters::~TaskGrooveParameters()
{
    delete ui;
}

void TaskGrooveParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgGrooveParameters::TaskDlgGrooveParameters(ViewProviderGroove *GrooveView)
    : TaskDialog(),GrooveView(GrooveView)
{
    assert(GrooveView);
    parameter  = new TaskGrooveParameters(GrooveView);

    Content.push_back(parameter);
}

TaskDlgGrooveParameters::~TaskDlgGrooveParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgGrooveParameters::open()
{

}

void TaskDlgGrooveParameters::clicked(int)
{

}

bool TaskDlgGrooveParameters::accept()
{
    std::string name = GrooveView->getObject()->getNameInDocument();

    //Gui::Command::openCommand("Groove changed");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Angle = %f",name.c_str(),parameter->getAngle());
    std::string axis = parameter->getReferenceAxis().toStdString();
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.ReferenceAxis = %s",name.c_str(),axis.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Midplane = %i",name.c_str(),parameter->getMidplane()?1:0);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %i",name.c_str(),parameter->getReversed()?1:0);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::commitCommand();

    return true;
}

bool TaskDlgGrooveParameters::reject()
{
    // get the support and Sketch
    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(GrooveView->getObject());
    Sketcher::SketchObject *pcSketch;
    App::DocumentObject    *pcSupport;
    if (pcGroove->Sketch.getValue()) {
        pcSketch = static_cast<Sketcher::SketchObject*>(pcGroove->Sketch.getValue());
        pcSupport = pcSketch->Support.getValue();
    }

    // role back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    // if abort command deleted the object the support is visible again
    if (!Gui::Application::Instance->getViewProvider(pcGroove)) {
        if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
            Gui::Application::Instance->getViewProvider(pcSketch)->show();
        if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
            Gui::Application::Instance->getViewProvider(pcSupport)->show();
    }

    //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    //Gui::Command::commitCommand();

    return true;
}



#include "moc_TaskGrooveParameters.cpp"
