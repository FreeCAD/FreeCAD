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
# include <QAction>
# include <QMessageBox>
# include <QTimer>
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

TaskLinearPatternParameters::TaskLinearPatternParameters(ViewProviderTransformed *TransformedView,QWidget *parent)
    : TaskTransformedParameters(TransformedView, parent)
    , ui(new Ui_TaskLinearPatternParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    ui->buttonOK->hide();
    ui->checkBoxUpdateView->setEnabled(true);

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

TaskLinearPatternParameters::TaskLinearPatternParameters(TaskMultiTransformParameters *parentTask, QLayout *layout)
        : TaskTransformedParameters(parentTask), ui(new Ui_TaskLinearPatternParameters)
{
    proxy = new QWidget(parentTask);
    ui->setupUi(proxy);
    connect(ui->buttonOK, &QToolButton::pressed,
            parentTask, &TaskLinearPatternParameters::onSubTaskButtonOK);
    QMetaObject::connectSlotsByName(this);

    layout->addWidget(proxy);

    ui->buttonOK->setEnabled(true);
    ui->buttonAddFeature->hide();
    ui->buttonRemoveFeature->hide();
    ui->listWidgetFeatures->hide();
    ui->checkBoxUpdateView->hide();

    selectionMode = none;

    // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    blockUpdate = false;
    setupUI();
}

void TaskLinearPatternParameters::connectSignals()
{
    connect(ui->buttonAddFeature, &QToolButton::toggled,
            this, &TaskLinearPatternParameters::onButtonAddFeature);
    connect(ui->buttonRemoveFeature, &QToolButton::toggled,
            this, &TaskLinearPatternParameters::onButtonRemoveFeature);

    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    action->setShortcut(QKeySequence::Delete);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    // display shortcut behind the context menu entry
    action->setShortcutVisibleInContextMenu(true);
#endif
    ui->listWidgetFeatures->addAction(action);
    connect(action, &QAction::triggered, this, &TaskLinearPatternParameters::onFeatureDeleted);
    ui->listWidgetFeatures->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(ui->listWidgetFeatures->model(), &QAbstractListModel::rowsMoved,
            this, &TaskLinearPatternParameters::indexesMoved);

    updateViewTimer = new QTimer(this);
    updateViewTimer->setSingleShot(true);
    updateViewTimer->setInterval(getUpdateViewTimeout());
    connect(updateViewTimer, &QTimer::timeout,
            this, &TaskLinearPatternParameters::onUpdateViewTimer);

    connect(ui->comboDirection, qOverload<int>(&QComboBox::activated),
            this, &TaskLinearPatternParameters::onDirectionChanged);
    connect(ui->checkReverse, &QCheckBox::toggled,
            this, &TaskLinearPatternParameters::onCheckReverse);
    connect(ui->comboMode, qOverload<int>(&QComboBox::activated),
            this, &TaskLinearPatternParameters::onModeChanged);
    connect(ui->spinLength, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskLinearPatternParameters::onLength);
    connect(ui->spinOffset, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskLinearPatternParameters::onOffset);
    connect(ui->spinOccurrences, &Gui::UIntSpinBox::unsignedChanged,
            this, &TaskLinearPatternParameters::onOccurrences);
    connect(ui->checkBoxUpdateView, &QCheckBox::toggled,
            this, &TaskLinearPatternParameters::onUpdateView);
}

void TaskLinearPatternParameters::setupUI()
{
    // Get the feature data
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    std::vector<App::DocumentObject*> originals = pcLinearPattern->Originals.getValues();

    // Fill data into dialog elements
    for (auto obj : originals) {
        if (obj) {
            QListWidgetItem* item = new QListWidgetItem();
            item->setText(QString::fromUtf8(obj->Label.getValue()));
            item->setData(Qt::UserRole, QString::fromLatin1(obj->getNameInDocument()));
            ui->listWidgetFeatures->addItem(item);
        }
    }
    // ---------------------

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
    if (sketch && sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
        this->fillAxisCombo(dirLinks, static_cast<Part::Part2DObject*>(sketch));
    }
    else {
        this->fillAxisCombo(dirLinks, nullptr);
    }

    //show the parts coordinate system axis for selection
    PartDesign::Body * body = PartDesign::Body::findBodyOf(getObject());
    if(body) {
        try {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->setTemporaryVisibility(true, false);
        } catch (const Base::Exception &ex) {
            Base::Console().Error ("%s\n", ex.what () );
        }
    }

    adaptVisibilityToMode();
    connectSignals();
}

void TaskLinearPatternParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    PartDesign::LinearPatternMode mode = static_cast<PartDesign::LinearPatternMode>(pcLinearPattern->Mode.getValue());

    bool reverse = pcLinearPattern->Reversed.getValue();
    double length = pcLinearPattern->Length.getValue();
    double offset = pcLinearPattern->Offset.getValue();
    unsigned occurrences = pcLinearPattern->Occurrences.getValue();

    if (dirLinks.setCurrentLink(pcLinearPattern->Direction) == -1){
        //failed to set current, because the link isn't in the list yet
        dirLinks.addLink(pcLinearPattern->Direction, getRefStr(pcLinearPattern->Direction.getValue(),
                                                               pcLinearPattern->Direction.getSubValues()));
        dirLinks.setCurrentLink(pcLinearPattern->Direction);
    }

    // Note: This block of code would trigger change signal handlers (e.g. onOccurrences())
    // and another updateUI() if we didn't check for blockUpdate
    ui->checkReverse->setChecked(reverse);
    ui->comboMode->setCurrentIndex((long)mode);
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

void TaskLinearPatternParameters::addObject(App::DocumentObject* obj)
{
    QString label = QString::fromUtf8(obj->Label.getValue());
    QString objectName = QString::fromLatin1(obj->getNameInDocument());

    QListWidgetItem* item = new QListWidgetItem();
    item->setText(label);
    item->setData(Qt::UserRole, objectName);
    ui->listWidgetFeatures->addItem(item);
}

void TaskLinearPatternParameters::removeObject(App::DocumentObject* obj)
{
    QString label = QString::fromUtf8(obj->Label.getValue());
    removeItemFromListWidget(ui->listWidgetFeatures, label);
}

void TaskLinearPatternParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode != none && msg.Type == Gui::SelectionChanges::AddSelection) {
        if (originalSelected(msg)) {
            exitSelectionMode();
        }
        else if (selectionMode == reference) {
            // TODO check if this works correctly (2015-09-01, Fat-Zer)
            exitSelectionMode();
            std::vector<std::string> directions;
            App::DocumentObject* selObj = nullptr;
            PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
            if (pcLinearPattern) {
                getReferencedSelection(pcLinearPattern, msg, selObj, directions);

                // Note: ReferenceSelection has already checked the selection for validity
                if (selObj && (selectionMode == reference ||
                               selObj->isDerivedFrom(App::Line::getClassTypeId()) ||
                               selObj->isDerivedFrom(Part::Feature::getClassTypeId()) ||
                               selObj->isDerivedFrom(PartDesign::Line::getClassTypeId()) ||
                               selObj->isDerivedFrom(PartDesign::Plane::getClassTypeId()))) {
                    setupTransaction();
                    pcLinearPattern->Direction.setValue(selObj, directions);
                    recomputeFeature();
                    updateUI();
                }
            }
        }
    }
}

void TaskLinearPatternParameters::clearButtons()
{
    ui->buttonAddFeature->setChecked(false);
    ui->buttonRemoveFeature->setChecked(false);
}

void TaskLinearPatternParameters::onCheckReverse(const bool on) {
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Reversed.setValue(on);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onModeChanged(const int mode) {
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Mode.setValue(mode);

    adaptVisibilityToMode();

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onLength(const double l) {
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Length.setValue(l);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onOffset(const double o) {
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Offset.setValue(o);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onOccurrences(const uint n) {
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Occurrences.setValue(n);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onDirectionChanged(int /*num*/)
{
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    try{
        if (!dirLinks.getCurrentLink().getValue()) {
            // enter reference selection mode
            hideObject();
            showBase();
            selectionMode = reference;
            Gui::Selection().clearSelection();
            addReferenceSelectionGate(AllowSelection::EDGE | AllowSelection::FACE | AllowSelection::PLANAR);
        } else {
            exitSelectionMode();
            pcLinearPattern->Direction.Paste(dirLinks.getCurrentLink());
        }
    } catch (Base::Exception &e) {
        QMessageBox::warning(nullptr,tr("Error"),QApplication::translate("Exception", e.what()));
    }

    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        // Do the same like in TaskDlgLinearPatternParameters::accept() but without doCommand
        PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
        std::vector<std::string> directions;
        App::DocumentObject* obj;

        setupTransaction();
        getDirection(obj, directions);
        pcLinearPattern->Direction.setValue(obj,directions);
        pcLinearPattern->Reversed.setValue(getReverse());
        pcLinearPattern->Length.setValue(getLength());
        pcLinearPattern->Offset.setValue(getOffset());
        pcLinearPattern->Occurrences.setValue(getOccurrences());

        recomputeFeature();
    }
}

void TaskLinearPatternParameters::onFeatureDeleted()
{
    PartDesign::Transformed* pcTransformed = getObject();
    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();
    int currentRow = ui->listWidgetFeatures->currentRow();
    if (currentRow < 0) {
        Base::Console().Error("PartDesign LinearPattern: No feature selected for removing.\n");
        return; //no current row selected
    }
    originals.erase(originals.begin() + currentRow);
    setupTransaction();
    pcTransformed->Originals.setValues(originals);
    ui->listWidgetFeatures->model()->removeRow(currentRow);
    recomputeFeature();
}

void TaskLinearPatternParameters::getDirection(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    const App::PropertyLinkSub &lnk = dirLinks.getCurrentLink();
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
        //hide the parts coordinate system axis for selection
        PartDesign::Body * body = PartDesign::Body::findBodyOf(getObject());
        if (body) {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->resetTemporaryVisibility();
        }
    }
    catch (const Base::Exception &ex) {
        Base::Console().Error ("%s\n", ex.what () );
    }

    if (proxy)
        delete proxy;
}

void TaskLinearPatternParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskLinearPatternParameters::apply()
{
    std::vector<std::string> directions;
    App::DocumentObject* obj;
    getDirection(obj, directions);
    std::string direction = buildLinkSingleSubPythonStr(obj, directions);

    auto tobj = TransformedView->getObject();
    FCMD_OBJ_CMD(tobj,"Direction = " << direction);
    FCMD_OBJ_CMD(tobj,"Reversed = " << getReverse());

    ui->spinLength->apply();
    ui->spinOffset->apply();
    ui->spinOccurrences->apply();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgLinearPatternParameters::TaskDlgLinearPatternParameters(ViewProviderLinearPattern *LinearPatternView)
    : TaskDlgTransformedParameters(LinearPatternView)
{
    parameter = new TaskLinearPatternParameters(LinearPatternView);

    Content.push_back(parameter);
}

#include "moc_TaskLinearPatternParameters.cpp"
