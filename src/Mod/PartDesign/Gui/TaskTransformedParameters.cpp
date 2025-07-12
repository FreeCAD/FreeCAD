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
#include <QAction>
#include <QListWidget>
#include <QListWidgetItem>
#include <QIcon>
#endif

#include <unordered_set>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Command.h>
#include <Gui/Tools.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>
#include <Mod/PartDesign/App/FeatureTransformed.h>

#include "ui_TaskTransformedParameters.h"
#include "TaskTransformedParameters.h"
#include "TaskMultiTransformParameters.h"
#include "ReferenceSelection.h"


FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskTransformedParameters */

TaskTransformedParameters::TaskTransformedParameters(ViewProviderTransformed* TransformedView,
                                                     QWidget* parent)
    : TaskBox(Gui::BitmapFactory().pixmap(TransformedView->featureIcon().c_str()),
              TransformedView->menuName,
              true,
              parent)
    , TransformedView(TransformedView)
    , ui(new Ui_TaskTransformedParameters)
{
    Gui::Document* doc = TransformedView->getDocument();
    this->attachDocument(doc);

    // remember initial transaction ID
    App::GetApplication().getActiveTransaction(&transactionID);
}

TaskTransformedParameters::TaskTransformedParameters(TaskMultiTransformParameters* parentTask)
    : TaskBox(QPixmap(), tr(""), true, parentTask)
    , parentTask(parentTask)
    , insideMultiTransform(true)
{}

TaskTransformedParameters::~TaskTransformedParameters()
{
    // make sure to remove selection gate in all cases
    Gui::Selection().rmvSelectionGate();

    delete proxy;
}

void TaskTransformedParameters::setupUI()
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->listWidgetFeatures,
            &QListWidget::itemChanged,
            this,
            &TaskTransformedParameters::onFeatureItemChanged);

    connect(ui->checkBoxUpdateView,
            &QCheckBox::toggled,
            this,
            &TaskTransformedParameters::onUpdateView);

    // Get the feature data
    auto pcTransformed = getObject<PartDesign::Transformed>();

    using Mode = PartDesign::Transformed::Mode;


    auto const mode = static_cast<Mode>(pcTransformed->TransformMode.getValue());
    bool featuresModeActive = (mode == Mode::Features);
    ui->groupFeatureList->setChecked(featuresModeActive);
    ui->listWidgetFeatures->setEnabled(featuresModeActive);

    // Connect the new GroupBox's toggled signal
    connect(ui->groupFeatureList,
            &QGroupBox::toggled,
            this,
            &TaskTransformedParameters::onGroupFeaturesToggled);

    // Populate the list *only* if starting in Features mode
    if (featuresModeActive) {
        populateFeatureList();
    }

    setupParameterUI(ui->featureUI);  // create parameter UI widgets
    this->groupLayout()->addWidget(proxy);
}

void TaskTransformedParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{}

void TaskTransformedParameters::populateFeatureList()
{
    blockUpdate = true;  // Prevent itemChanged signals during population
    ui->listWidgetFeatures->clear();

    auto pcTransformed = getObject<PartDesign::Transformed>();
    if (!pcTransformed) {
        blockUpdate = false;
        return;
    }
    auto body = pcTransformed->getFeatureBody();
    if (!body) {
        blockUpdate = false;
        return;
    }

    std::vector<App::DocumentObject*> currentOriginalsVec = pcTransformed->Originals.getValues();
    std::unordered_set<std::string> currentOriginalsNames;
    for (const auto* obj : currentOriginalsVec) {
        if (obj) {
            currentOriginalsNames.insert(obj->getNameInDocument());
        }
    }

    for (App::DocumentObject* obj : body->Group.getValues()) {
        if (!obj) {
            continue;
        }
        if (obj == pcTransformed) {
            break;  // Stop at self
        }

        auto addSubFeat = freecad_cast<PartDesign::FeatureAddSub*>(obj);
        if (addSubFeat) {
            auto item = new QListWidgetItem();
            item->setText(QString::fromUtf8(obj->Label.getValue()));
            item->setData(Qt::UserRole, QString::fromLatin1(obj->getNameInDocument()));
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

            Gui::ViewProvider* vp = Application::Instance->getViewProvider(obj);
            if (vp) {
                QIcon icon = vp->getIcon();  // Get the icon from the ViewProvider
                if (!icon.isNull()) {
                    item->setIcon(icon);  // Set the icon on the list item
                }
            }

            bool isChecked = currentOriginalsNames.count(obj->getNameInDocument()) > 0;
            item->setCheckState(isChecked ? Qt::Checked : Qt::Unchecked);
            ui->listWidgetFeatures->addItem(item);
        }
    }
    blockUpdate = false;  // Re-enable itemChanged signals
}

void TaskTransformedParameters::onFeatureItemChanged(QListWidgetItem* item)
{
    if (blockUpdate || !item) {
        return;
    }
    auto pcTransformed = getObject<PartDesign::Transformed>();
    if (!pcTransformed) {
        return;
    }
    QString name = item->data(Qt::UserRole).toString();
    if (name.isEmpty()) {
        return;
    }
    App::DocumentObject* changedObj =
        pcTransformed->getDocument()->getObject(name.toLatin1().constData());
    if (!changedObj) {
        return;
    }

    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();
    bool isChecked = (item->checkState() == Qt::Checked);
    auto it = std::ranges::find(originals, changedObj);
    bool changed = false;

    if (isChecked) {
        if (it == originals.end()) {
            originals.push_back(changedObj);
            changed = true;
        }
    }
    else {
        if (it != originals.end()) {
            originals.erase(it);
            changed = true;
        }
    }

    if (changed) {
        setupTransaction();
        pcTransformed->Originals.setValues(originals);
        if (ui->checkBoxUpdateView->isChecked()) {
            recomputeFeature();
        }
        else {
            pcTransformed->touch();
        }
    }
}

void TaskTransformedParameters::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    if (TransformedView == &Obj) {
        TransformedView = nullptr;
    }
}

void TaskTransformedParameters::changeEvent(QEvent* event)
{
    TaskBox::changeEvent(event);
    if (event->type() == QEvent::LanguageChange && proxy) {
        ui->retranslateUi(proxy);
        retranslateParameterUI(ui->featureUI);
    }
}

void TaskTransformedParameters::onGroupFeaturesToggled(bool checked)
{
    auto pcTransformed = getObject<PartDesign::Transformed>();
    if (!pcTransformed) {
        return;
    }

    using Mode = PartDesign::Transformed::Mode;
    Mode newMode = checked ? Mode::Features : Mode::WholeShape;
    Mode currentMode = static_cast<Mode>(pcTransformed->TransformMode.getValue());

    ui->listWidgetFeatures->setEnabled(checked);

    bool modeChanged = (newMode != currentMode);

    if (modeChanged) {
        pcTransformed->TransformMode.setValue(static_cast<long>(newMode));
    }

    if (checked) {  // Switched TO Features mode (or already was)
        // Populate the list (safe to call even if already populated)
        populateFeatureList();
        // Recompute only if mode actually changed to Features
        if (modeChanged && ui->checkBoxUpdateView->isChecked()) {
            setupTransaction();
            recomputeFeature();
        }
        else if (modeChanged) {
            setupTransaction();  // Still need transaction if mode changed
            pcTransformed->touch();
        }
    }
    else {  // Switched TO WholeShape mode
        blockUpdate = true;
        ui->listWidgetFeatures->clear();  // Clear visual list
        blockUpdate = false;

        // Clear Originals property only if necessary and mode actually changed
        bool originalsCleared = false;
        if (modeChanged && !pcTransformed->Originals.getValues().empty()) {
            pcTransformed->Originals.setValues({});
            originalsCleared = true;
        }

        // Recompute if mode changed or originals were cleared
        if ((modeChanged || originalsCleared) && ui->checkBoxUpdateView->isChecked()) {
            setupTransaction();
            recomputeFeature();
        }
        else if (modeChanged || originalsCleared) {
            setupTransaction();  // Need transaction if mode changed or originals cleared
            pcTransformed->touch();
        }
    }
}

int TaskTransformedParameters::getUpdateViewTimeout() const
{
    return 500;
}

void TaskTransformedParameters::setupTransaction()
{
    if (!isEnabledTransaction()) {
        return;
    }

    auto obj = getObject();
    if (!obj) {
        return;
    }

    int tid = 0;
    App::GetApplication().getActiveTransaction(&tid);
    if (tid != 0 && tid == transactionID) {
        return;
    }

    // open a transaction if none is active
    std::string name("Edit ");
    name += obj->Label.getValue();
    transactionID = App::GetApplication().setActiveTransaction(name.c_str());
}

void TaskTransformedParameters::setEnabledTransaction(bool on)
{
    enableTransaction = on;
}

bool TaskTransformedParameters::isEnabledTransaction() const
{
    return enableTransaction;
}

void TaskTransformedParameters::fillAxisCombo(ComboLinks& combolinks, Part::Part2DObject* sketch)
{
    combolinks.clear();

    // add sketch axes
    if (sketch) {
        combolinks.addLink(sketch, "H_Axis", tr("Horizontal sketch axis"));
        combolinks.addLink(sketch, "V_Axis", tr("Vertical sketch axis"));
        combolinks.addLink(sketch, "N_Axis", tr("Normal sketch axis"));
        for (int i = 0; i < sketch->getAxisCount(); i++) {
            QString itemText = tr("Construction line %1").arg(i + 1);
            std::stringstream sub;
            sub << "Axis" << i;
            combolinks.addLink(sketch, sub.str(), itemText);
        }
    }

    // add part axes
    App::DocumentObject* obj = getObject();
    PartDesign::Body* body = PartDesign::Body::findBodyOf(obj);

    if (body) {
        try {
            App::Origin* orig = body->getOrigin();
            combolinks.addLink(orig->getX(), "", tr("Base X axis"));
            combolinks.addLink(orig->getY(), "", tr("Base Y axis"));
            combolinks.addLink(orig->getZ(), "", tr("Base Z axis"));
        }
        catch (const Base::Exception& ex) {
            Base::Console().error("%s\n", ex.what());
        }
    }

    // add "Select reference"
    combolinks.addLink(nullptr, std::string(), tr("Select reference..."));
}

void TaskTransformedParameters::fillPlanesCombo(Gui::ComboLinks& combolinks,
                                                Part::Part2DObject* sketch)
{
    combolinks.clear();

    // add sketch axes
    if (sketch) {
        combolinks.addLink(sketch, "V_Axis", QObject::tr("Vertical sketch axis"));
        combolinks.addLink(sketch, "H_Axis", QObject::tr("Horizontal sketch axis"));
        for (int i = 0; i < sketch->getAxisCount(); i++) {
            QString itemText = tr("Construction line %1").arg(i + 1);
            std::stringstream sub;
            sub << "Axis" << i;
            combolinks.addLink(sketch, sub.str(), itemText);
        }
    }

    // add part baseplanes
    App::DocumentObject* obj = getObject();
    PartDesign::Body* body = PartDesign::Body::findBodyOf(obj);

    if (body) {
        try {
            App::Origin* orig = body->getOrigin();
            combolinks.addLink(orig->getXY(), "", tr("Base XY plane"));
            combolinks.addLink(orig->getYZ(), "", tr("Base YZ plane"));
            combolinks.addLink(orig->getXZ(), "", tr("Base XZ plane"));
        }
        catch (const Base::Exception& ex) {
            Base::Console().error("%s\n", ex.what());
        }
    }

    // add "Select reference"
    combolinks.addLink(nullptr, std::string(), tr("Select reference..."));
}

void TaskTransformedParameters::recomputeFeature()
{
    getTopTransformedView()->recomputeFeature();
}

PartDesignGui::ViewProviderTransformed* TaskTransformedParameters::getTopTransformedView() const
{
    return insideMultiTransform ? parentTask->TransformedView : TransformedView;
}

PartDesign::Transformed* TaskTransformedParameters::getTopTransformedObject() const
{
    ViewProviderTransformed* vp = getTopTransformedView();
    if (!vp) {
        return nullptr;
    }

    App::DocumentObject* transform = vp->getObject();
    assert(transform->isDerivedFrom<PartDesign::Transformed>());
    return static_cast<PartDesign::Transformed*>(transform);
}

PartDesign::Transformed* TaskTransformedParameters::getObject() const
{
    if (insideMultiTransform) {
        return parentTask->getSubFeature();
    }
    if (TransformedView) {
        return TransformedView->getObject<PartDesign::Transformed>();
    }
    return nullptr;
}

App::DocumentObject* TaskTransformedParameters::getBaseObject() const
{
    PartDesign::Feature* feature = getTopTransformedObject();
    if (!feature) {
        return nullptr;
    }

    // NOTE: getBaseObject() throws if there is no base; shouldn't happen here.
    App::DocumentObject* base = feature->getBaseObject(true);
    if (!base) {
        auto body = feature->getFeatureBody();
        if (body) {
            base = body->getPrevSolidFeature(feature);
        }
    }
    return base;
}

App::DocumentObject* TaskTransformedParameters::getSketchObject() const
{
    PartDesign::Transformed* feature = getTopTransformedObject();
    return feature ? feature->getSketchObject() : nullptr;
}

void TaskTransformedParameters::hideObject()
{
    try {
        FCMD_OBJ_HIDE(getTopTransformedObject());
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
}

void TaskTransformedParameters::showObject()
{
    try {
        FCMD_OBJ_SHOW(getTopTransformedObject());
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
}

void TaskTransformedParameters::hideBase()
{
    try {
        FCMD_OBJ_HIDE(getBaseObject());
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
}

void TaskTransformedParameters::showBase()
{
    try {
        FCMD_OBJ_SHOW(getBaseObject());
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
}

void TaskTransformedParameters::exitSelectionMode()
{
    try {
        selectionMode = SelectionMode::None;
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().clearSelection();
        showObject();
    }
    catch (Base::Exception& exc) {
        exc.reportException();
    }
}

void TaskTransformedParameters::addReferenceSelectionGate(AllowSelectionFlags allow)
{
    std::unique_ptr<Gui::SelectionFilterGate> gateRefPtr(
        new ReferenceSelection(getBaseObject(), allow));
    std::unique_ptr<Gui::SelectionFilterGate> gateDepPtr(
        new NoDependentsSelection(getTopTransformedObject()));
    Gui::Selection().addSelectionGate(new CombineSelectionFilterGates(gateRefPtr, gateDepPtr));
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgTransformedParameters::TaskDlgTransformedParameters(ViewProviderTransformed* viewProvider)
    : TaskDlgFeatureParameters(viewProvider)
{
}

//==== calls from the TaskView ===============================================================

bool TaskDlgTransformedParameters::accept()
{
    parameter->exitSelectionMode();
    parameter->apply();

    return TaskDlgFeatureParameters::accept();
}

bool TaskDlgTransformedParameters::reject()
{
    // ensure that we are not in selection mode
    parameter->exitSelectionMode();
    return TaskDlgFeatureParameters::reject();
}


#include "moc_TaskTransformedParameters.cpp"
