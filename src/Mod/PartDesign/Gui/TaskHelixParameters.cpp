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

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/WaitCursor.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureHelix.h>

#include "ReferenceSelection.h"
#include "ui_TaskHelixParameters.h"
#include "TaskHelixParameters.h"

using namespace PartDesignGui;
using PartDesign::HelixMode;
using namespace Gui;


/* TRANSLATOR PartDesignGui::TaskHelixParameters */

TaskHelixParameters::TaskHelixParameters(PartDesignGui::ViewProviderHelix* HelixView, QWidget* parent)
    : TaskSketchBasedParameters(HelixView, parent, "PartDesign_AdditiveHelix", tr("Helix parameters"))
    , ui(new Ui_TaskHelixParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    initializeHelix();

    assignProperties();
    setValuesFromProperties();

    updateUI();

    // enable use of parametric expressions for the numerical fields
    bindProperties();

    connectSlots();
    setFocus();
    showCoordinateAxes();
}

void TaskHelixParameters::initializeHelix()
{
    PartDesign::Helix* helix = static_cast<PartDesign::Helix*>(vp->getObject());
    if (!(helix->HasBeenEdited).getValue()) {
        helix->proposeParameters();
        recomputeFeature();
    }
}

void TaskHelixParameters::assignProperties()
{
    PartDesign::Helix* helix = static_cast<PartDesign::Helix*>(vp->getObject());
    propAngle = &(helix->Angle);
    propGrowth = &(helix->Growth);
    propPitch = &(helix->Pitch);
    propHeight = &(helix->Height);
    propTurns = &(helix->Turns);
    propReferenceAxis = &(helix->ReferenceAxis);
    propLeftHanded = &(helix->LeftHanded);
    propReversed = &(helix->Reversed);
    propMode = &(helix->Mode);
    propOutside = &(helix->Outside);
}

void TaskHelixParameters::setValuesFromProperties()
{
    double pitch = propPitch->getValue();
    double height = propHeight->getValue();
    double turns = propTurns->getValue();
    double angle = propAngle->getValue();
    double growth = propGrowth->getValue();
    bool leftHanded = propLeftHanded->getValue();
    bool reversed = propReversed->getValue();
    int index = propMode->getValue();
    bool outside = propOutside->getValue();

    ui->pitch->setValue(pitch);
    ui->height->setValue(height);
    ui->turns->setValue(turns);
    ui->coneAngle->setValue(angle);
    ui->coneAngle->setMinimum(propAngle->getMinimum());
    ui->coneAngle->setMaximum(propAngle->getMaximum());
    ui->growth->setValue(growth);
    ui->checkBoxLeftHanded->setChecked(leftHanded);
    ui->checkBoxReversed->setChecked(reversed);
    ui->inputMode->setCurrentIndex(index);
    ui->checkBoxOutside->setChecked(outside);
}

void TaskHelixParameters::bindProperties()
{
    PartDesign::Helix* helix = static_cast<PartDesign::Helix*>(vp->getObject());
    ui->pitch->bind(helix->Pitch);
    ui->height->bind(helix->Height);
    ui->turns->bind(helix->Turns);
    ui->coneAngle->bind(helix->Angle);
    ui->growth->bind(helix->Growth);
}

void TaskHelixParameters::connectSlots()
{
    QMetaObject::connectSlotsByName(this);

    connect(ui->pitch, qOverload<double>(&QuantitySpinBox::valueChanged), this,
            &TaskHelixParameters::onPitchChanged);
    connect(ui->height, qOverload<double>(&QuantitySpinBox::valueChanged), this,
            &TaskHelixParameters::onHeightChanged);
    connect(ui->turns, qOverload<double>(&QuantitySpinBox::valueChanged), this,
            &TaskHelixParameters::onTurnsChanged);
    connect(ui->coneAngle, qOverload<double>(&QuantitySpinBox::valueChanged), this,
            &TaskHelixParameters::onAngleChanged);
    connect(ui->growth, qOverload<double>(&QuantitySpinBox::valueChanged), this,
            &TaskHelixParameters::onGrowthChanged);
    connect(ui->axis, qOverload<int>(&QComboBox::activated), this,
            &TaskHelixParameters::onAxisChanged);
    connect(ui->checkBoxLeftHanded, &QCheckBox::toggled, this,
            &TaskHelixParameters::onLeftHandedChanged);
    connect(ui->checkBoxReversed, &QCheckBox::toggled, this,
            &TaskHelixParameters::onReversedChanged);
    connect(ui->checkBoxUpdateView, &QCheckBox::toggled, this,
        &TaskHelixParameters::onUpdateView);
    connect(ui->inputMode, qOverload<int>(&QComboBox::activated), this,
            &TaskHelixParameters::onModeChanged);
    connect(ui->checkBoxOutside, &QCheckBox::toggled, this,
            &TaskHelixParameters::onOutsideChanged);
}

void TaskHelixParameters::showCoordinateAxes()
{
    //show the parts coordinate system axis for selection
    PartDesign::Body* body = PartDesign::Body::findBodyOf(vp->getObject());
    if (body) {
        try {
            App::Origin* origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->setTemporaryVisibility(true, false);
        }
        catch (const Base::Exception& ex) {
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

    if (forceRefill) {
        ui->axis->clear();
        this->axesInList.clear();

        //add sketch axes
        addSketchAxes();

        //add part axes
        addPartAxes();

        //add "Select reference"
        addAxisToCombo(nullptr, std::string(), tr("Select reference..."));
    }

    //add current link, if not in list and highlight it
    int indexOfCurrent = addCurrentLink();
    if (indexOfCurrent != -1)
        ui->axis->setCurrentIndex(indexOfCurrent);

    blockUpdate = oldVal_blockUpdate;
}

void TaskHelixParameters::addSketchAxes()
{
    PartDesign::ProfileBased* pcFeat = static_cast<PartDesign::ProfileBased*>(vp->getObject());
    Part::Part2DObject* pcSketch = dynamic_cast<Part::Part2DObject*>(pcFeat->Profile.getValue());
    if (pcSketch) {
        addAxisToCombo(pcSketch, "N_Axis", tr("Normal sketch axis"));
        addAxisToCombo(pcSketch, "V_Axis", tr("Vertical sketch axis"));
        addAxisToCombo(pcSketch, "H_Axis", tr("Horizontal sketch axis"));
        for (int i = 0; i < pcSketch->getAxisCount(); i++) {
            QString itemText = tr("Construction line %1").arg(i + 1);
            std::stringstream sub;
            sub << "Axis" << i;
            addAxisToCombo(pcSketch, sub.str(), itemText);
        }
    }
}

void TaskHelixParameters::addPartAxes()
{
    PartDesign::ProfileBased* pcFeat = static_cast<PartDesign::ProfileBased*>(vp->getObject());
    PartDesign::Body* body = PartDesign::Body::findBodyOf(pcFeat);
    if (body) {
        try {
            App::Origin* orig = body->getOrigin();
            addAxisToCombo(orig->getX(), "", tr("Base X axis"));
            addAxisToCombo(orig->getY(), "", tr("Base Y axis"));
            addAxisToCombo(orig->getZ(), "", tr("Base Z axis"));
        }
        catch (const Base::Exception& ex) {
            ex.ReportException();
        }
    }
}

int TaskHelixParameters::addCurrentLink()
{
    int indexOfCurrent = -1;
    App::DocumentObject* ax = propReferenceAxis->getValue();
    const std::vector<std::string>& subList = propReferenceAxis->getSubValues();
    for (size_t i = 0; i < axesInList.size(); i++) {
        if (ax == axesInList[i]->getValue() && subList == axesInList[i]->getSubValues()) {
            indexOfCurrent = i;
            break;
        }
    }

    if (indexOfCurrent == -1 && ax) {
        assert(subList.size() <= 1);
        std::string sub;
        if (!subList.empty())
            sub = subList[0];
        addAxisToCombo(ax, sub, getRefStr(ax, subList));
        indexOfCurrent = axesInList.size() - 1;
    }

    return indexOfCurrent;
}

void TaskHelixParameters::addAxisToCombo(App::DocumentObject* linkObj, std::string linkSubname, QString itemText)
{
    this->ui->axis->addItem(itemText);
    this->axesInList.emplace_back(new App::PropertyLinkSub);
    App::PropertyLinkSub& lnk = *(axesInList.back());
    lnk.setValue(linkObj, std::vector<std::string>(1, linkSubname));
}

void TaskHelixParameters::updateStatus()
{
    auto pcHelix = static_cast<PartDesign::Helix*>(vp->getObject());
    auto status = std::string(pcHelix->getStatusString());
    QString translatedStatus;
    if (status.compare("Valid") == 0 || status.compare("Touched") == 0) {
        if (pcHelix->safePitch() > pcHelix->Pitch.getValue()) {
            translatedStatus = tr("Warning: helix might be self intersecting");
        }
    }
    // if the helix touches itself along a single helical edge we get this error
    else if (status.compare("NCollection_IndexedDataMap::FindFromKey") == 0) {
        translatedStatus = tr("Error: helix touches itself");
    }
    ui->labelMessage->setText(translatedStatus);
}

void TaskHelixParameters::updateUI()
{
    fillAxisCombo();
    assignToolTipsFromPropertyDocs();
    updateStatus();
    adaptVisibilityToMode();
}

void TaskHelixParameters::adaptVisibilityToMode()
{
    bool isPitchVisible = false;
    bool isHeightVisible = false;
    bool isTurnsVisible = false;
    bool isOutsideVisible = false;
    bool isAngleVisible = false;
    bool isGrowthVisible = false;

    auto pcHelix = static_cast<PartDesign::Helix*>(vp->getObject());
    if (pcHelix->getAddSubType() == PartDesign::FeatureAddSub::Subtractive)
        isOutsideVisible = true;

    HelixMode mode = static_cast<HelixMode>(propMode->getValue());
    if (mode == HelixMode::pitch_height_angle) {
        isPitchVisible = true;
        isHeightVisible = true;
        isAngleVisible = true;
    }
    else if (mode == HelixMode::pitch_turns_angle) {
        isPitchVisible = true;
        isTurnsVisible = true;
        isAngleVisible = true;
    }
    else if (mode == HelixMode::height_turns_angle) {
        isHeightVisible = true;
        isTurnsVisible = true;
        isAngleVisible = true;
    }
    else if (mode == HelixMode::height_turns_growth) {
        isHeightVisible = true;
        isTurnsVisible = true;
        isGrowthVisible = true;
    }
    else {
        ui->labelMessage->setText(tr("Error: unsupported mode"));
    }

    ui->pitch->setVisible(isPitchVisible);
    ui->labelPitch->setVisible(isPitchVisible);

    ui->height->setVisible(isHeightVisible);
    ui->labelHeight->setVisible(isHeightVisible);

    ui->turns->setVisible(isTurnsVisible);
    ui->labelTurns->setVisible(isTurnsVisible);

    ui->coneAngle->setVisible(isAngleVisible);
    ui->labelConeAngle->setVisible(isAngleVisible);

    ui->growth->setVisible(isGrowthVisible);
    ui->labelGrowth->setVisible(isGrowthVisible);

    ui->checkBoxOutside->setVisible(isOutsideVisible);
}

void TaskHelixParameters::assignToolTipsFromPropertyDocs()
{
    auto pcHelix = static_cast<PartDesign::Helix*>(vp->getObject());
    const char* propCategory = "App::Property"; // cf. https://tracker.freecad.org/view.php?id=0002524
    QString toolTip;

    // Beware that "Axis" in the GUI actually represents the property "ReferenceAxis"!
    // The property "Axis" holds only the directional part of the reference axis and has no corresponding GUI element.
    toolTip = QApplication::translate(propCategory, pcHelix->ReferenceAxis.getDocumentation());
    ui->axis->setToolTip(toolTip);
    ui->labelAxis->setToolTip(toolTip);

    toolTip = QApplication::translate(propCategory, pcHelix->Mode.getDocumentation());
    ui->inputMode->setToolTip(toolTip);
    ui->labelInputMode->setToolTip(toolTip);

    toolTip = QApplication::translate(propCategory, pcHelix->Pitch.getDocumentation());
    ui->pitch->setToolTip(toolTip);
    ui->labelPitch->setToolTip(toolTip);

    toolTip = QApplication::translate(propCategory, pcHelix->Height.getDocumentation());
    ui->height->setToolTip(toolTip);
    ui->labelHeight->setToolTip(toolTip);

    toolTip = QApplication::translate(propCategory, pcHelix->Turns.getDocumentation());
    ui->turns->setToolTip(toolTip);
    ui->labelTurns->setToolTip(toolTip);

    toolTip = QApplication::translate(propCategory, pcHelix->Angle.getDocumentation());
    ui->coneAngle->setToolTip(toolTip);
    ui->labelConeAngle->setToolTip(toolTip);

    toolTip = QApplication::translate(propCategory, pcHelix->Growth.getDocumentation());
    ui->growth->setToolTip(toolTip);
    ui->labelGrowth->setToolTip(toolTip);

    toolTip = QApplication::translate(propCategory, pcHelix->LeftHanded.getDocumentation());
    ui->checkBoxLeftHanded->setToolTip(toolTip);

    toolTip = QApplication::translate(propCategory, pcHelix->Reversed.getDocumentation());
    ui->checkBoxReversed->setToolTip(toolTip);

    toolTip = QApplication::translate(propCategory, pcHelix->Outside.getDocumentation());
    ui->checkBoxOutside->setToolTip(toolTip);
}

void TaskHelixParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        std::vector<std::string> axis;
        App::DocumentObject* selObj;
        if (getReferencedSelection(vp->getObject(), msg, selObj, axis) && selObj) {
            exitSelectionMode();
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

void TaskHelixParameters::onGrowthChanged(double len)
{
    propGrowth->setValue(len);
    recomputeFeature();
    updateUI();
}

void TaskHelixParameters::onAxisChanged(int num)
{
    PartDesign::ProfileBased* pcHelix = static_cast<PartDesign::ProfileBased*>(vp->getObject());

    if (axesInList.empty())
        return;

    App::DocumentObject* oldRefAxis = propReferenceAxis->getValue();
    std::vector<std::string> oldSubRefAxis = propReferenceAxis->getSubValues();
    std::string oldRefName;
    if (!oldSubRefAxis.empty())
        oldRefName = oldSubRefAxis.front();

    App::PropertyLinkSub& lnk = *(axesInList[num]);
    if (!lnk.getValue()) {
        // enter reference selection mode
        // assure the sketch is visible
        if (auto sketch = dynamic_cast<Part::Part2DObject *>(pcHelix->Profile.getValue())) {
            Gui::cmdAppObjectShow(sketch);
        }
        TaskSketchBasedParameters::onSelectReference(
            AllowSelection::EDGE |
            AllowSelection::PLANAR |
            AllowSelection::CIRCLE);
        return;
    }
    else {
        if (!pcHelix->getDocument()->isIn(lnk.getValue())) {
            Base::Console().Error("Object was deleted\n");
            return;
        }
        propReferenceAxis->Paste(lnk);

        // in case user is in selection mode, but changed their mind before selecting anything.
        exitSelectionMode();
    }

    try {
        App::DocumentObject* newRefAxis = propReferenceAxis->getValue();
        const std::vector<std::string>& newSubRefAxis = propReferenceAxis->getSubValues();
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
        updateStatus();
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
    ui->turns->setValue(propTurns->getValue());
    ui->coneAngle->setValue(propAngle->getValue());
    ui->growth->setValue(propGrowth->getValue());

    recomputeFeature();
    updateUI();
}

void TaskHelixParameters::onLeftHandedChanged(bool on)
{
    propLeftHanded->setValue(on);
    recomputeFeature();
    updateUI();
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
        PartDesign::Body* body = vp ? PartDesign::Body::findBodyOf(vp->getObject()) : nullptr;
        if (body) {
            App::Origin* origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->resetTemporaryVisibility();
        }
    }
    catch (const Base::Exception& ex) {
        ex.ReportException();
    }

}

void TaskHelixParameters::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        // save current indexes
        int axis = ui->axis->currentIndex();
        int mode = ui->inputMode->currentIndex();
        ui->retranslateUi(proxy);
        assignToolTipsFromPropertyDocs();

        // Axes added by the user cannot be restored
        fillAxisCombo(true);

        // restore the indexes
        if (axis < ui->axis->count())
            ui->axis->setCurrentIndex(axis);
        ui->inputMode->setCurrentIndex(mode);
    }
}

void TaskHelixParameters::getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    if (axesInList.empty())
        throw Base::RuntimeError("Not initialized!");

    int num = ui->axis->currentIndex();
    const App::PropertyLinkSub& lnk = *(axesInList.at(num));
    if (!lnk.getValue()) {
        throw Base::RuntimeError("Still in reference selection mode; reference wasn't selected yet");
    }
    else {
        PartDesign::ProfileBased* pcRevolution = static_cast<PartDesign::ProfileBased*>(vp->getObject());
        if (!pcRevolution->getDocument()->isIn(lnk.getValue())) {
            throw Base::RuntimeError("Object was deleted");
        }

        obj = lnk.getValue();
        sub = lnk.getSubValues();
    }
}

bool TaskHelixParameters::showPreview(PartDesign::Helix* helix)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/PartDesign");
    if ((hGrp->GetBool("SubractiveHelixPreview", true) && helix->getAddSubType() == PartDesign::FeatureAddSub::Subtractive) ||
        (hGrp->GetBool("AdditiveHelixPreview", false) && helix->getAddSubType() == PartDesign::FeatureAddSub::Additive)) {
        return true;
    }

    return false;
}

void TaskHelixParameters::startReferenceSelection(App::DocumentObject* profile, App::DocumentObject* base)
{
    PartDesign::Helix* pcHelix = dynamic_cast<PartDesign::Helix*>(vp->getObject());
    if (pcHelix && showPreview(pcHelix)) {
        Gui::Document* doc = vp->getDocument();
        if (doc) {
            doc->setHide(profile->getNameInDocument());
        }
    }
    else {
        TaskSketchBasedParameters::startReferenceSelection(profile, base);
    }
}

void TaskHelixParameters::finishReferenceSelection(App::DocumentObject* profile, App::DocumentObject* base)
{
    PartDesign::Helix* pcHelix = dynamic_cast<PartDesign::Helix*>(vp->getObject());
    if (pcHelix && showPreview(pcHelix)) {
        Gui::Document* doc = vp->getDocument();
        if (doc) {
            doc->setShow(profile->getNameInDocument());
        }
    }
    else {
        TaskSketchBasedParameters::finishReferenceSelection(profile, base);
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
    FCMD_OBJ_CMD(tobj, "ReferenceAxis = " << axis);
    FCMD_OBJ_CMD(tobj, "Mode = " << propMode->getValue());
    FCMD_OBJ_CMD(tobj, "Pitch = " << propPitch->getValue());
    FCMD_OBJ_CMD(tobj, "Height = " << propHeight->getValue());
    FCMD_OBJ_CMD(tobj, "Turns = " << propTurns->getValue());
    FCMD_OBJ_CMD(tobj, "Angle = " << propAngle->getValue());
    FCMD_OBJ_CMD(tobj, "Growth = " << propGrowth->getValue());
    FCMD_OBJ_CMD(tobj, "LeftHanded = " << (propLeftHanded->getValue() ? 1 : 0));
    FCMD_OBJ_CMD(tobj, "Reversed = " << (propReversed->getValue() ? 1 : 0));
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
TaskDlgHelixParameters::TaskDlgHelixParameters(ViewProviderHelix* HelixView)
    : TaskDlgSketchBasedParameters(HelixView)
{
    assert(HelixView);
    Content.push_back(new TaskHelixParameters(HelixView));
}


#include "moc_TaskHelixParameters.cpp"
