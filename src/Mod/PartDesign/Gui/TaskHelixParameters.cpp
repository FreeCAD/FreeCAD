/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *                 2020 David Österberg                                    *
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

#include <Base/UnitsApi.h>
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/FeatureHelix.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include <QString>

#include "ReferenceSelection.h"
#include "Utils.h"

#include "ui_TaskHelixParameters.h"
#include "TaskHelixParameters.h"

using namespace PartDesignGui;
using namespace Gui;


/* TRANSLATOR PartDesignGui::TaskHelixParameters */

TaskHelixParameters::TaskHelixParameters(PartDesignGui::ViewProviderHelix *HelixView, QWidget *parent)
    : TaskSketchBasedParameters(HelixView, parent, "PartDesign_Additive_Helix",tr("Helix parameters")),
    ui (new Ui_TaskHelixParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->pitch, SIGNAL(valueChanged(double)),
            this, SLOT(onPitchChanged(double)));
    connect(ui->height, SIGNAL(valueChanged(double)),
            this, SLOT(onHeightChanged(double)));
    connect(ui->turns, SIGNAL(valueChanged(double)),
            this, SLOT(onTurnsChanged(double)));
    connect(ui->coneAngle, SIGNAL(valueChanged(double)),
            this, SLOT(onAngleChanged(double)));
    connect(ui->axis, SIGNAL(activated(int)),
            this, SLOT(onAxisChanged(int)));
    connect(ui->checkBoxLeftHanded, SIGNAL(toggled(bool)),
            this, SLOT(onLeftHandedChanged(bool)));
    connect(ui->checkBoxReversed, SIGNAL(toggled(bool)),
            this, SLOT(onReversedChanged(bool)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));
    connect(ui->inputMode, SIGNAL(activated(int)),
            this, SLOT(onModeChanged(int)));
    connect(ui->checkBoxOutside, SIGNAL(toggled(bool)),
            this, SLOT(onOutsideChanged(bool)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->axis->blockSignals(true);
    ui->pitch->blockSignals(true);
    ui->height->blockSignals(true);
    ui->turns->blockSignals(true);
    ui->coneAngle->blockSignals(true);
    ui->checkBoxLeftHanded->blockSignals(true);
    ui->checkBoxReversed->blockSignals(true);
    ui->checkBoxOutside->blockSignals(true);

    //bind property mirrors
    PartDesign::ProfileBased* pcFeat = static_cast<PartDesign::ProfileBased*>(vp->getObject());

    PartDesign::Helix* rev = static_cast<PartDesign::Helix*>(vp->getObject());

    if (!(rev->HasBeenEdited).getValue()) {
        rev->proposeParameters();
        recomputeFeature();
    }

    this->propAngle = &(rev->Angle);
    this->propPitch = &(rev->Pitch);
    this->propHeight = &(rev->Height);
    this->propTurns = &(rev->Turns);
    this->propReferenceAxis = &(rev->ReferenceAxis);
    this->propLeftHanded = &(rev->LeftHanded);
    this->propReversed = &(rev->Reversed);
    this->propMode = &(rev->Mode);
    this->propOutside = &(rev->Outside);

    double pitch = propPitch->getValue();
    double height = propHeight->getValue();
    double turns = propTurns->getValue();
    double angle = propAngle->getValue();
    bool leftHanded = propLeftHanded->getValue();
    bool reversed = propReversed->getValue();
    int index = propMode->getValue();
    bool outside = propOutside->getValue();

    ui->pitch->setValue(pitch);
    ui->height->setValue(height);
    ui->turns->setValue(turns);
    ui->coneAngle->setValue(angle);
    ui->checkBoxLeftHanded->setChecked(leftHanded);
    ui->checkBoxReversed->setChecked(reversed);
    ui->inputMode->setCurrentIndex(index);
    ui->checkBoxOutside->setChecked(outside);

    blockUpdate = false;
    updateUI();

    // enable use of parametric expressions for the numerical fields
    ui->pitch->bind(static_cast<PartDesign::Helix *>(pcFeat)->Pitch);
    ui->height->bind(static_cast<PartDesign::Helix *>(pcFeat)->Height);
    ui->turns->bind(static_cast<PartDesign::Helix *>(pcFeat)->Turns);
    ui->coneAngle->bind(static_cast<PartDesign::Helix *>(pcFeat)->Angle);

    ui->axis->blockSignals(false);
    ui->pitch->blockSignals(false);
    ui->height->blockSignals(false);
    ui->turns->blockSignals(false);
    ui->coneAngle->blockSignals(false);
    ui->checkBoxLeftHanded->blockSignals(false);
    ui->checkBoxReversed->blockSignals(false);
    ui->checkBoxOutside->blockSignals(false);

    setFocus ();

    //show the parts coordinate system axis for selection
    PartDesign::Body * body = PartDesign::Body::findBodyOf ( vp->getObject () );
    if(body) {
        try {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->setTemporaryVisibility(true, false);
        } catch (const Base::Exception &ex) {
            ex.ReportException();
        }
     }
}

void TaskHelixParameters::fillAxisCombo(bool forceRefill)
{
    bool oldVal_blockUpdate = blockUpdate;
    blockUpdate = true;

    if (axesInList.empty())
        forceRefill = true;//not filled yet, full refill

    if (forceRefill){
        ui->axis->clear();

        this->axesInList.clear();

        //add sketch axes
        PartDesign::ProfileBased* pcFeat = static_cast<PartDesign::ProfileBased*>(vp->getObject());
        Part::Part2DObject* pcSketch = dynamic_cast<Part::Part2DObject*>(pcFeat->Profile.getValue());
        if (pcSketch){
            addAxisToCombo(pcSketch,"V_Axis",QObject::tr("Vertical sketch axis"));
            addAxisToCombo(pcSketch,"H_Axis",QObject::tr("Horizontal sketch axis"));
            for (int i=0; i < pcSketch->getAxisCount(); i++) {
                QString itemText = QObject::tr("Construction line %1").arg(i+1);
                std::stringstream sub;
                sub << "Axis" << i;
                addAxisToCombo(pcSketch,sub.str(),itemText);
            }
        }

        //add part axes
        PartDesign::Body * body = PartDesign::Body::findBodyOf ( pcFeat );
        if (body) {
            try {
                App::Origin* orig = body->getOrigin();
                addAxisToCombo(orig->getX(),"",tr("Base X axis"));
                addAxisToCombo(orig->getY(),"",tr("Base Y axis"));
                addAxisToCombo(orig->getZ(),"",tr("Base Z axis"));
            } catch (const Base::Exception &ex) {
                ex.ReportException();
            }
        }

        //add "Select reference"
        addAxisToCombo(0,std::string(),tr("Select reference..."));
    }//endif forceRefill

    //add current link, if not in list
    //first, figure out the item number for current axis
    int indexOfCurrent = -1;
    App::DocumentObject* ax = propReferenceAxis->getValue();
    const std::vector<std::string> &subList = propReferenceAxis->getSubValues();
    for (size_t i = 0; i < axesInList.size(); i++) {
        if (ax == axesInList[i]->getValue() && subList == axesInList[i]->getSubValues())
            indexOfCurrent = i;
    }
    if (indexOfCurrent == -1  &&  ax) {
        assert(subList.size() <= 1);
        std::string sub;
        if (!subList.empty())
            sub = subList[0];
        addAxisToCombo(ax, sub, getRefStr(ax, subList));
        indexOfCurrent = axesInList.size()-1;
    }

    //highlight current.
    if (indexOfCurrent != -1)
        ui->axis->setCurrentIndex(indexOfCurrent);

    blockUpdate = oldVal_blockUpdate;
}

void TaskHelixParameters::addAxisToCombo(App::DocumentObject* linkObj,
                                              std::string linkSubname,
                                              QString itemText)
{
    this->ui->axis->addItem(itemText);
    this->axesInList.emplace_back(new App::PropertyLinkSub);
    App::PropertyLinkSub &lnk = *(axesInList[axesInList.size()-1]);
    lnk.setValue(linkObj,std::vector<std::string>(1,linkSubname));
}

void TaskHelixParameters::updateUI()
{
    fillAxisCombo();

    auto pcHelix = static_cast<PartDesign::Helix*>(vp->getObject());
    auto status = std::string(pcHelix->getStatusString());
    if (status.compare("Valid")==0 || status.compare("Touched")==0) {
        if (pcHelix->safePitch() > propPitch->getValue())
            status = "Warning: helix might be self intersecting";
        else
            status = "";
    }
    ui->labelMessage->setText(QString::fromUtf8(status.c_str()));

    bool isPitchVisible  = false;
    bool isHeightVisible = false;
    bool isTurnsVisible  = false;
    bool isOutsideVisible = false;

    if(pcHelix->getAddSubType() == PartDesign::FeatureAddSub::Subtractive)
        isOutsideVisible = true;

    switch (propMode->getValue()) {
        case 0:
            isPitchVisible = true;
            isHeightVisible = true;
            break;
        case 1:
            isPitchVisible = true;
            isTurnsVisible = true;
            break;
        default:
            isHeightVisible = true;
            isTurnsVisible = true;
    }

    ui->pitch->setVisible(isPitchVisible);
    ui->labelPitch->setVisible(isPitchVisible);

    ui->height->setVisible(isHeightVisible);
    ui->labelHeight->setVisible(isHeightVisible);

    ui->turns->setVisible(isTurnsVisible);
    ui->labelTurns->setVisible(isTurnsVisible);

    ui->checkBoxOutside->setVisible(isOutsideVisible);

}

void TaskHelixParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {

        exitSelectionMode();
        std::vector<std::string> axis;
        App::DocumentObject* selObj;
        if (getReferencedSelection(vp->getObject(), msg, selObj, axis) && selObj) {
            propReferenceAxis->setValue(selObj, axis);
            recomputeFeature();
            updateUI();
        }
    }
}


void TaskHelixParameters::onPitchChanged(double len)
{
    propPitch->setValue(len);
    recomputeFeature();
    updateUI();
}

void TaskHelixParameters::onHeightChanged(double len)
{
    propHeight->setValue(len);
    recomputeFeature();
    updateUI();
}

void TaskHelixParameters::onTurnsChanged(double len)
{
    propTurns->setValue(len);
    recomputeFeature();
    updateUI();
}

void TaskHelixParameters::onAngleChanged(double len)
{
    propAngle->setValue(len);
    recomputeFeature();
    updateUI();
}

void TaskHelixParameters::onAxisChanged(int num)
{
    PartDesign::ProfileBased* pcHelix = static_cast<PartDesign::ProfileBased*>(vp->getObject());

    if (axesInList.empty())
        return;

    App::DocumentObject *oldRefAxis = propReferenceAxis->getValue();
    std::vector<std::string> oldSubRefAxis = propReferenceAxis->getSubValues();
    std::string oldRefName;
    if (!oldSubRefAxis.empty())
        oldRefName = oldSubRefAxis.front();

    App::PropertyLinkSub &lnk = *(axesInList[num]);
    if (lnk.getValue() == 0) {
        // enter reference selection mode
        TaskSketchBasedParameters::onSelectReference(true, true, false, true);
    } else {
        if (!pcHelix->getDocument()->isIn(lnk.getValue())){
            Base::Console().Error("Object was deleted\n");
            return;
        }
        propReferenceAxis->Paste(lnk);
        exitSelectionMode();
    }

    try {
        App::DocumentObject *newRefAxis = propReferenceAxis->getValue();
        const std::vector<std::string> &newSubRefAxis = propReferenceAxis->getSubValues();
        std::string newRefName;
        if (!newSubRefAxis.empty())
            newRefName = newSubRefAxis.front();

        if (oldRefAxis != newRefAxis ||
            oldSubRefAxis.size() != newSubRefAxis.size() ||
            oldRefName != newRefName) {
            bool reversed = propReversed->getValue();
            if (reversed != propReversed->getValue()) {
                propReversed->setValue(reversed);
                ui->checkBoxReversed->blockSignals(true);
                ui->checkBoxReversed->setChecked(reversed);
                ui->checkBoxReversed->blockSignals(false);
            }
        }

        recomputeFeature();
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
}

void TaskHelixParameters::onModeChanged(int index)
{

    propMode->setValue(index);

    ui->pitch->setValue(propPitch->getValue());
    ui->height->setValue(propHeight->getValue());
    ui->turns->setValue((propHeight->getValue())/(propPitch->getValue()));

    recomputeFeature();
    updateUI();
}

void TaskHelixParameters::onLeftHandedChanged(bool on)
{
    propLeftHanded->setValue(on);
    recomputeFeature();
}

void TaskHelixParameters::onReversedChanged(bool on)
{
    propReversed->setValue(on);
    recomputeFeature();
    updateUI();
}

void TaskHelixParameters::onOutsideChanged(bool on)
{
    propOutside->setValue(on);
    recomputeFeature();
    updateUI();
}


TaskHelixParameters::~TaskHelixParameters()
{
    try {
        //hide the parts coordinate system axis for selection
        PartDesign::Body * body = vp ? PartDesign::Body::findBodyOf(vp->getObject()) : 0;
        if (body) {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->resetTemporaryVisibility();
        }
    } catch (const Base::Exception &ex) {
        ex.ReportException();
    }

}

void TaskHelixParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskHelixParameters::getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    if (axesInList.empty())
        throw Base::RuntimeError("Not initialized!");

    int num = ui->axis->currentIndex();
    const App::PropertyLinkSub &lnk = *(axesInList[num]);
    if (lnk.getValue() == 0) {
        throw Base::RuntimeError("Still in reference selection mode; reference wasn't selected yet");
    } else {
        PartDesign::ProfileBased* pcRevolution = static_cast<PartDesign::ProfileBased*>(vp->getObject());
        if (!pcRevolution->getDocument()->isIn(lnk.getValue())){
            throw Base::RuntimeError("Object was deleted");
        }

        obj = lnk.getValue();
        sub = lnk.getSubValues();
    }
}

// this is used for logging the command fully when recording macros
void TaskHelixParameters::apply()
{
    std::vector<std::string> sub;
    App::DocumentObject* obj;
    getReferenceAxis(obj, sub);
    std::string axis = buildLinkSingleSubPythonStr(obj, sub);
    auto tobj = vp->getObject();
    FCMD_OBJ_CMD(tobj,"ReferenceAxis = " << axis);
    FCMD_OBJ_CMD(tobj,"Mode = " << propMode->getValue());
    FCMD_OBJ_CMD(tobj,"Pitch = " << propPitch->getValue());
    FCMD_OBJ_CMD(tobj,"Height = " << propHeight->getValue());
    FCMD_OBJ_CMD(tobj,"Turns = " << propTurns->getValue());
    FCMD_OBJ_CMD(tobj,"Angle = " << propAngle->getValue());
    FCMD_OBJ_CMD(tobj,"LeftHanded = " << (propLeftHanded->getValue() ? 1 : 0));
    FCMD_OBJ_CMD(tobj,"Reversed = " << (propReversed->getValue() ? 1 : 0));
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
TaskDlgHelixParameters::TaskDlgHelixParameters(ViewProviderHelix *HelixView)
    : TaskDlgSketchBasedParameters(HelixView)
{
    assert(HelixView);
    Content.push_back(new TaskHelixParameters(HelixView));
}


#include "moc_TaskHelixParameters.cpp"
