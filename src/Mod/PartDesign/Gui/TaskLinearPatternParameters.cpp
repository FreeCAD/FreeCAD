/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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
#include <QMessageBox>
#include <QTimer>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/FeatureLinearPattern.h>

#include "ui_TaskLinearPatternParameters.h"
#include "TaskLinearPatternParameters.h"
#include "ReferenceSelection.h"
#include "TaskMultiTransformParameters.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskLinearPatternParameters */

TaskLinearPatternParameters::TaskLinearPatternParameters(ViewProviderTransformed* TransformedView,
                                                         QWidget* parent)
    : TaskTransformedParameters(TransformedView, parent)
    , ui(new Ui_TaskLinearPatternParameters)
{
    setupUI();
}

TaskLinearPatternParameters::TaskLinearPatternParameters(TaskMultiTransformParameters* parentTask,
                                                         QWidget* parameterWidget)
    : TaskTransformedParameters(parentTask)
    , ui(new Ui_TaskLinearPatternParameters)
{
    setupParameterUI(parameterWidget);
}

void TaskLinearPatternParameters::setupParameterUI(QWidget* widget)
{
    ui->setupUi(widget);
    QMetaObject::connectSlotsByName(this);

    // Get the feature data
    auto pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());

    ui->spinLength->bind(pcLinearPattern->Length);
    ui->spinOffset->bind(pcLinearPattern->Offset);
    ui->spinOccurrences->bind(pcLinearPattern->Occurrences);
    ui->spinOccurrences->setMaximum(pcLinearPattern->Occurrences.getMaximum());
    ui->spinOccurrences->setMinimum(pcLinearPattern->Occurrences.getMinimum());

    ui->comboDirection->setEnabled(true);
    ui->checkReverse->setEnabled(true);
    ui->comboMode->setEnabled(true);
    ui->spinLength->blockSignals(true);
    ui->spinLength->setEnabled(true);
    ui->spinLength->setUnit(Base::Unit::Length);
    ui->spinLength->blockSignals(false);
    ui->spinOffset->blockSignals(true);
    ui->spinOffset->setEnabled(true);
    ui->spinOffset->setUnit(Base::Unit::Length);
    ui->spinOffset->blockSignals(false);
    ui->spinOccurrences->setEnabled(true);

    dirLinks.setCombo(*(ui->comboDirection));
    App::DocumentObject* sketch = getSketchObject();
    if (sketch && sketch->isDerivedFrom<Part::Part2DObject>()) {
        this->fillAxisCombo(dirLinks, static_cast<Part::Part2DObject*>(sketch));
    }
    else {
        this->fillAxisCombo(dirLinks, nullptr);
    }

    // show the parts coordinate system axis for selection
    PartDesign::Body* body = PartDesign::Body::findBodyOf(getObject());
    if (body) {
        try {
            App::Origin* origin = body->getOrigin();
            auto vpOrigin = static_cast<ViewProviderOrigin*>(
                Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->setTemporaryVisibility(true, false);
        }
        catch (const Base::Exception& ex) {
            Base::Console().Error("%s\n", ex.what());
        }
    }

    adaptVisibilityToMode();

    updateViewTimer = new QTimer(this);
    updateViewTimer->setSingleShot(true);
    updateViewTimer->setInterval(getUpdateViewTimeout());
    connect(updateViewTimer,
            &QTimer::timeout,
            this,
            &TaskLinearPatternParameters::onUpdateViewTimer);

    connect(ui->comboDirection,
            qOverload<int>(&QComboBox::activated),
            this,
            &TaskLinearPatternParameters::onDirectionChanged);
    connect(ui->checkReverse,
            &QCheckBox::toggled,
            this,
            &TaskLinearPatternParameters::onCheckReverse);
    connect(ui->comboMode,
            qOverload<int>(&QComboBox::activated),
            this,
            &TaskLinearPatternParameters::onModeChanged);
    connect(ui->spinLength,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &TaskLinearPatternParameters::onLength);
    connect(ui->spinOffset,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &TaskLinearPatternParameters::onOffset);
    connect(ui->spinOccurrences,
            &Gui::UIntSpinBox::unsignedChanged,
            this,
            &TaskLinearPatternParameters::onOccurrences);
}

void TaskLinearPatternParameters::retranslateParameterUI(QWidget* widget)
{
    ui->retranslateUi(widget);
}

void TaskLinearPatternParameters::updateUI()
{
    if (blockUpdate) {
        return;
    }
    blockUpdate = true;

    auto pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    auto mode = static_cast<PartDesign::LinearPatternMode>(pcLinearPattern->Mode.getValue());

    bool reverse = pcLinearPattern->Reversed.getValue();
    double length = pcLinearPattern->Length.getValue();
    double offset = pcLinearPattern->Offset.getValue();
    unsigned occurrences = pcLinearPattern->Occurrences.getValue();

    if (dirLinks.setCurrentLink(pcLinearPattern->Direction) == -1) {
        // failed to set current, because the link isn't in the list yet
        dirLinks.addLink(pcLinearPattern->Direction,
                         getRefStr(pcLinearPattern->Direction.getValue(),
                                   pcLinearPattern->Direction.getSubValues()));
        dirLinks.setCurrentLink(pcLinearPattern->Direction);
    }

    // Note: This block of code would trigger change signal handlers (e.g. onOccurrences())
    // and another updateUI() if we didn't check for blockUpdate
    ui->checkReverse->setChecked(reverse);
    ui->comboMode->setCurrentIndex(static_cast<int>(mode));
    ui->spinLength->setValue(length);
    ui->spinOffset->setValue(offset);
    ui->spinOccurrences->setValue(occurrences);

    blockUpdate = false;
}

void TaskLinearPatternParameters::adaptVisibilityToMode()
{
    auto pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    auto mode = static_cast<PartDesign::LinearPatternMode>(pcLinearPattern->Mode.getValue());

    ui->lengthWrapper->setVisible(mode == PartDesign::LinearPatternMode::length);
    ui->offsetWrapper->setVisible(mode == PartDesign::LinearPatternMode::offset);

    updateUI();
}

void TaskLinearPatternParameters::onUpdateViewTimer()
{
    setupTransaction();
    recomputeFeature();
}

void TaskLinearPatternParameters::kickUpdateViewTimer() const
{
    updateViewTimer->start();
}

void TaskLinearPatternParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode != SelectionMode::None && msg.Type == Gui::SelectionChanges::AddSelection) {
        if (originalSelected(msg)) {
            exitSelectionMode();
        }
        else {
            auto pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());

            std::vector<std::string> directions;
            App::DocumentObject* selObj = nullptr;
            getReferencedSelection(pcLinearPattern, msg, selObj, directions);
            if (!selObj) {
                return;
            }

            // Note: ReferenceSelection has already checked the selection for validity
            if (selectionMode == SelectionMode::Reference || selObj->isDerivedFrom<App::Line>()) {
                setupTransaction();
                pcLinearPattern->Direction.setValue(selObj, directions);
                recomputeFeature();
                updateUI();
            }
            exitSelectionMode();
        }
    }
}

void TaskLinearPatternParameters::onCheckReverse(const bool on)
{
    if (blockUpdate) {
        return;
    }
    auto pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Reversed.setValue(on);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onModeChanged(const int mode)
{
    if (blockUpdate) {
        return;
    }
    auto pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Mode.setValue(mode);

    adaptVisibilityToMode();

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onLength(const double length)
{
    if (blockUpdate) {
        return;
    }
    auto pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Length.setValue(length);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onOffset(const double offset)
{
    if (blockUpdate) {
        return;
    }
    auto pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Offset.setValue(offset);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onOccurrences(const uint number)
{
    if (blockUpdate) {
        return;
    }
    auto pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Occurrences.setValue(number);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onDirectionChanged(int /*num*/)
{
    if (blockUpdate) {
        return;
    }
    auto pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    try {
        if (!dirLinks.getCurrentLink().getValue()) {
            // enter reference selection mode
            hideObject();
            showBase();
            selectionMode = SelectionMode::Reference;
            Gui::Selection().clearSelection();
            addReferenceSelectionGate(AllowSelection::EDGE | AllowSelection::FACE
                                      | AllowSelection::PLANAR);
        }
        else {
            exitSelectionMode();
            pcLinearPattern->Direction.Paste(dirLinks.getCurrentLink());
        }
    }
    catch (Base::Exception& e) {
        QMessageBox::warning(nullptr, tr("Error"), QApplication::translate("Exception", e.what()));
    }

    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        // Do the same like in TaskDlgLinearPatternParameters::accept() but without doCommand
        auto pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
        std::vector<std::string> directions;
        App::DocumentObject* obj = nullptr;

        setupTransaction();
        getDirection(obj, directions);
        pcLinearPattern->Direction.setValue(obj, directions);
        pcLinearPattern->Reversed.setValue(getReverse());
        pcLinearPattern->Length.setValue(getLength());
        pcLinearPattern->Offset.setValue(getOffset());
        pcLinearPattern->Occurrences.setValue(getOccurrences());

        recomputeFeature();
    }
}

void TaskLinearPatternParameters::getDirection(App::DocumentObject*& obj,
                                               std::vector<std::string>& sub) const
{
    const App::PropertyLinkSub& lnk = dirLinks.getCurrentLink();
    obj = lnk.getValue();
    sub = lnk.getSubValues();
}

bool TaskLinearPatternParameters::getReverse() const
{
    return ui->checkReverse->isChecked();
}

int TaskLinearPatternParameters::getMode() const
{
    return ui->comboMode->currentIndex();
}

double TaskLinearPatternParameters::getLength() const
{
    return ui->spinLength->value().getValue();
}

double TaskLinearPatternParameters::getOffset() const
{
    return ui->spinOffset->value().getValue();
}

unsigned TaskLinearPatternParameters::getOccurrences() const
{
    return ui->spinOccurrences->value();
}

TaskLinearPatternParameters::~TaskLinearPatternParameters()
{
    try {
        // hide the parts coordinate system axis for selection
        PartDesign::Body* body = PartDesign::Body::findBodyOf(getObject());
        if (body) {
            App::Origin* origin = body->getOrigin();
            auto vpOrigin = static_cast<ViewProviderOrigin*>(
                Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->resetTemporaryVisibility();
        }
    }
    catch (const Base::Exception& ex) {
        Base::Console().Error("%s\n", ex.what());
    }
}

void TaskLinearPatternParameters::apply()
{
    std::vector<std::string> directions;
    App::DocumentObject* obj = nullptr;
    getDirection(obj, directions);
    std::string direction = buildLinkSingleSubPythonStr(obj, directions);

    auto tobj = getObject();
    FCMD_OBJ_CMD(tobj, "Direction = " << direction);
    FCMD_OBJ_CMD(tobj, "Reversed = " << getReverse());
    FCMD_OBJ_CMD(tobj, "Mode = " << getMode());
    ui->spinLength->apply();
    ui->spinOffset->apply();
    ui->spinOccurrences->apply();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgLinearPatternParameters::TaskDlgLinearPatternParameters(
    ViewProviderLinearPattern* LinearPatternView)
    : TaskDlgTransformedParameters(LinearPatternView)
{
    parameter = new TaskLinearPatternParameters(LinearPatternView);

    Content.push_back(parameter);
}

#include "moc_TaskLinearPatternParameters.cpp"
