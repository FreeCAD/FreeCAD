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

#include "ui_TaskRevolutionParameters.h"
#include "TaskRevolutionParameters.h"
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
#include <Mod/PartDesign/App/FeatureRevolution.h>
#include <Mod/Sketcher/App/SketchObject.h>


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskRevolutionParameters */

TaskRevolutionParameters::TaskRevolutionParameters(ViewProviderRevolution *RevolutionView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("PartDesign_Revolution"),tr("Revolution parameters"),true, parent),RevolutionView(RevolutionView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskRevolutionParameters();
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

    // Temporarily prevent unnecessary feature recomputes
    ui->doubleSpinBox->blockSignals(true);
    ui->axis->blockSignals(true);
    ui->checkBoxMidplane->blockSignals(true);
    ui->checkBoxReversed->blockSignals(true);

    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(RevolutionView->getObject());
    double l = pcRevolution->Angle.getValue();
    bool mirrored = pcRevolution->Midplane.getValue();
    bool reversed = pcRevolution->Reversed.getValue();

    ui->doubleSpinBox->setDecimals(Base::UnitsApi::getDecimals());
    ui->doubleSpinBox->setValue(l);

    int count=pcRevolution->getSketchAxisCount();

    for (int i=ui->axis->count()-1; i >= count+2; i--)
        ui->axis->removeItem(i);
    for (int i=ui->axis->count(); i < count+2; i++)
        ui->axis->addItem(QString::fromAscii("Sketch axis %1").arg(i-2));

    int pos=-1;

    App::DocumentObject *pcReferenceAxis = pcRevolution->ReferenceAxis.getValue();
    const std::vector<std::string> &subReferenceAxis = pcRevolution->ReferenceAxis.getSubValues();
    if (pcReferenceAxis && pcReferenceAxis == pcRevolution->Sketch.getValue()) {
        assert(subReferenceAxis.size()==1);
        if (subReferenceAxis[0] == "V_Axis")
            pos = 0;
        else if (subReferenceAxis[0] == "H_Axis")
            pos = 1;
        else if (subReferenceAxis[0].size() > 4 && subReferenceAxis[0].substr(0,4) == "Axis")
            pos = 2 + std::atoi(subReferenceAxis[0].substr(4,4000).c_str());
    }

    if (pos < 0 || pos >= ui->axis->count()) {
        ui->axis->addItem(tr("Undefined"));
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

void TaskRevolutionParameters::onAngleChanged(double len)
{
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(RevolutionView->getObject());
    pcRevolution->Angle.setValue(len);
    if (updateView())
        pcRevolution->getDocument()->recomputeFeature(pcRevolution);
}

void TaskRevolutionParameters::onAxisChanged(int num)
{
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(RevolutionView->getObject());
    Sketcher::SketchObject *pcSketch = static_cast<Sketcher::SketchObject*>(pcRevolution->Sketch.getValue());
    if (pcSketch) {
        App::DocumentObject *oldRefAxis = pcRevolution->ReferenceAxis.getValue();
        std::vector<std::string> oldSubRefAxis = pcRevolution->ReferenceAxis.getSubValues();

        int maxcount = pcSketch->getAxisCount()+2;
        if (num == 0)
            pcRevolution->ReferenceAxis.setValue(pcSketch, std::vector<std::string>(1,"V_Axis"));
        else if (num == 1)
            pcRevolution->ReferenceAxis.setValue(pcSketch, std::vector<std::string>(1,"H_Axis"));
        else if (num >= 2 && num < maxcount) {
            QString buf = QString::fromUtf8("Axis%1").arg(num-2);
            std::string str = buf.toStdString();
            pcRevolution->ReferenceAxis.setValue(pcSketch, std::vector<std::string>(1,str));
        }
        if (num < maxcount && ui->axis->count() > maxcount)
            ui->axis->setMaxCount(maxcount);

        const std::vector<std::string> &newSubRefAxis = pcRevolution->ReferenceAxis.getSubValues();
        if (oldRefAxis != pcSketch ||
            oldSubRefAxis.size() != newSubRefAxis.size() ||
            oldSubRefAxis[0] != newSubRefAxis[0]) {
            bool reversed = pcRevolution->suggestReversed();
            if (reversed != pcRevolution->Reversed.getValue()) {
                pcRevolution->Reversed.setValue(reversed);
                ui->checkBoxReversed->blockSignals(true);
                ui->checkBoxReversed->setChecked(reversed);
                ui->checkBoxReversed->blockSignals(false);
            }
        }
    }
    if (updateView())
        pcRevolution->getDocument()->recomputeFeature(pcRevolution);
}

void TaskRevolutionParameters::onMidplane(bool on)
{
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(RevolutionView->getObject());
    pcRevolution->Midplane.setValue(on);
    if (updateView())
        pcRevolution->getDocument()->recomputeFeature(pcRevolution);
}

void TaskRevolutionParameters::onReversed(bool on)
{
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(RevolutionView->getObject());
    pcRevolution->Reversed.setValue(on);
    if (updateView())
        pcRevolution->getDocument()->recomputeFeature(pcRevolution);
}

void TaskRevolutionParameters::onUpdateView(bool on)
{
    if (on) {
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(RevolutionView->getObject());
        pcRevolution->getDocument()->recomputeFeature(pcRevolution);
    }
}

double TaskRevolutionParameters::getAngle(void) const
{
    return ui->doubleSpinBox->value();
}

QString TaskRevolutionParameters::getReferenceAxis(void) const
{
    // get the support and Sketch
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(RevolutionView->getObject());
    Sketcher::SketchObject *pcSketch = static_cast<Sketcher::SketchObject*>(pcRevolution->Sketch.getValue());

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

bool   TaskRevolutionParameters::getMidplane(void) const
{
    return ui->checkBoxMidplane->isChecked();
}

bool   TaskRevolutionParameters::getReversed(void) const
{
    return ui->checkBoxReversed->isChecked();
}

const bool TaskRevolutionParameters::updateView() const
{
    return ui->checkBoxUpdateView->isChecked();
}

TaskRevolutionParameters::~TaskRevolutionParameters()
{
    delete ui;
}

void TaskRevolutionParameters::changeEvent(QEvent *e)
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

TaskDlgRevolutionParameters::TaskDlgRevolutionParameters(ViewProviderRevolution *RevolutionView)
    : TaskDialog(),RevolutionView(RevolutionView)
{
    assert(RevolutionView);
    parameter  = new TaskRevolutionParameters(RevolutionView);

    Content.push_back(parameter);
}

TaskDlgRevolutionParameters::~TaskDlgRevolutionParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgRevolutionParameters::open()
{

}

void TaskDlgRevolutionParameters::clicked(int)
{

}

bool TaskDlgRevolutionParameters::accept()
{
    std::string name = RevolutionView->getObject()->getNameInDocument();

    //Gui::Command::openCommand("Revolution changed");
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

bool TaskDlgRevolutionParameters::reject()
{
    // get the support and Sketch
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(RevolutionView->getObject());
    Sketcher::SketchObject *pcSketch;
    App::DocumentObject    *pcSupport;
    if (pcRevolution->Sketch.getValue()) {
        pcSketch = static_cast<Sketcher::SketchObject*>(pcRevolution->Sketch.getValue());
        pcSupport = pcSketch->Support.getValue();
    }

    // role back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    // if abort command deleted the object the support is visible again
    if (!Gui::Application::Instance->getViewProvider(pcRevolution)) {
        if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
            Gui::Application::Instance->getViewProvider(pcSketch)->show();
        if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
            Gui::Application::Instance->getViewProvider(pcSupport)->show();
    }

    //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    //Gui::Command::commitCommand();

    return true;
}



#include "moc_TaskRevolutionParameters.cpp"
