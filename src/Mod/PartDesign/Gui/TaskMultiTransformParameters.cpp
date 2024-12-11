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
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Origin.h>

#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureLinearPattern.h>
#include <Mod/PartDesign/App/FeatureMirrored.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>
#include <Mod/PartDesign/App/FeaturePolarPattern.h>
#include <Mod/PartDesign/App/FeatureScaled.h>

#include "ui_TaskMultiTransformParameters.h"
#include "TaskMultiTransformParameters.h"
#include "TaskMirroredParameters.h"
#include "TaskLinearPatternParameters.h"
#include "TaskPolarPatternParameters.h"
#include "TaskScaledParameters.h"
#include "Utils.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskMultiTransformParameters */

TaskMultiTransformParameters::TaskMultiTransformParameters(ViewProviderTransformed* TransformedView,
                                                           QWidget* parent)
    : TaskTransformedParameters(TransformedView, parent)
    , ui(new Ui_TaskMultiTransformParameters)
{
    setupUI();
}

void TaskMultiTransformParameters::setupParameterUI(QWidget* widget)
{
    ui->setupUi(widget);
    QMetaObject::connectSlotsByName(this);

    // Create a context menu for the listview of transformation features
    auto action = new QAction(tr("Edit"), ui->listTransformFeatures);
    action->connect(action,
                    &QAction::triggered,
                    this,
                    &TaskMultiTransformParameters::onTransformEdit);
    ui->listTransformFeatures->addAction(action);
    action = new QAction(tr("Delete"), ui->listTransformFeatures);
    action->connect(action,
                    &QAction::triggered,
                    this,
                    &TaskMultiTransformParameters::onTransformDelete);
    ui->listTransformFeatures->addAction(action);
    action = new QAction(tr("Add mirrored transformation"), ui->listTransformFeatures);
    action->connect(action,
                    &QAction::triggered,
                    this,
                    &TaskMultiTransformParameters::onTransformAddMirrored);
    ui->listTransformFeatures->addAction(action);
    action = new QAction(tr("Add linear pattern"), ui->listTransformFeatures);
    action->connect(action,
                    &QAction::triggered,
                    this,
                    &TaskMultiTransformParameters::onTransformAddLinearPattern);
    ui->listTransformFeatures->addAction(action);
    action = new QAction(tr("Add polar pattern"), ui->listTransformFeatures);
    action->connect(action,
                    &QAction::triggered,
                    this,
                    &TaskMultiTransformParameters::onTransformAddPolarPattern);
    ui->listTransformFeatures->addAction(action);
    action = new QAction(tr("Add scaled transformation"), ui->listTransformFeatures);
    action->connect(action,
                    &QAction::triggered,
                    this,
                    &TaskMultiTransformParameters::onTransformAddScaled);
    ui->listTransformFeatures->addAction(action);
    action = new QAction(tr("Move up"), ui->listTransformFeatures);
    action->connect(action, &QAction::triggered, this, &TaskMultiTransformParameters::onMoveUp);
    ui->listTransformFeatures->addAction(action);
    action = new QAction(tr("Move down"), ui->listTransformFeatures);
    action->connect(action, &QAction::triggered, this, &TaskMultiTransformParameters::onMoveDown);
    ui->listTransformFeatures->addAction(action);
    ui->listTransformFeatures->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->listTransformFeatures,
            &QListWidget::activated,
            this,
            &TaskMultiTransformParameters::onTransformActivated);

    connect(ui->buttonOK,
            &QToolButton::pressed,
            this,
            &TaskMultiTransformParameters::onSubTaskButtonOK);
    ui->buttonOK->hide();

    // Get the transformFeatures data
    auto pcMultiTransform = TransformedView->getObject<PartDesign::MultiTransform>();
    std::vector<App::DocumentObject*> transformFeatures =
        pcMultiTransform->Transformations.getValues();

    // Fill data into dialog elements
    ui->listTransformFeatures->setEnabled(true);
    ui->listTransformFeatures->clear();
    for (auto it : transformFeatures) {
        if (it) {
            ui->listTransformFeatures->addItem(QString::fromUtf8(it->Label.getValue()));
        }
    }
    if (!transformFeatures.empty()) {
        ui->listTransformFeatures->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
        editHint = false;
    }
    else {
        ui->listTransformFeatures->addItem(tr("Right-click to add"));
        editHint = true;
    }
}

void TaskMultiTransformParameters::retranslateParameterUI(QWidget* widget)
{
    ui->retranslateUi(widget);
}

void TaskMultiTransformParameters::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    if (Obj.getObject() == this->subFeature) {
        this->subFeature = nullptr;
    }
    TaskTransformedParameters::slotDeletedObject(Obj);
}

void TaskMultiTransformParameters::closeSubTask()
{
    if (subTask) {
        ui->buttonOK->hide();
        exitSelectionMode();
        // The subfeature can already be deleted (e.g. cancel) so we have to check before
        // calling apply
        if (subFeature) {
            subTask->apply();
        }

        // Remove all parameter ui widgets and layout
        ui->subFeatureWidget->setUpdatesEnabled(false);
        qDeleteAll(
            ui->subFeatureWidget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly));
        qDeleteAll(
            ui->subFeatureWidget->findChildren<QLayout*>(QString(), Qt::FindDirectChildrenOnly));
        ui->subFeatureWidget->setUpdatesEnabled(true);

        delete subTask;
        subTask = nullptr;
        subFeature = nullptr;
    }
}

void TaskMultiTransformParameters::onTransformDelete()
{
    if (editHint) {
        return;  // Can't delete the hint...
    }
    int row = ui->listTransformFeatures->currentIndex().row();
    auto pcMultiTransform = TransformedView->getObject<PartDesign::MultiTransform>();
    std::vector<App::DocumentObject*> transformFeatures =
        pcMultiTransform->Transformations.getValues();

    App::DocumentObject* feature = transformFeatures[row];
    if (feature == this->subFeature) {
        this->subFeature = nullptr;
    }

    setupTransaction();
    pcMultiTransform->getDocument()->removeObject(feature->getNameInDocument());
    closeSubTask();

    transformFeatures.erase(transformFeatures.begin() + row);
    pcMultiTransform->Transformations.setValues(transformFeatures);
    // Note: When the last transformation is deleted, recomputeFeature does nothing, because
    // Transformed::execute() says: "No transformations defined, exit silently"
    recomputeFeature();

    ui->listTransformFeatures->model()->removeRow(row);
    ui->listTransformFeatures->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
}

void TaskMultiTransformParameters::onTransformEdit()
{
    if (editHint) {
        return;  // Can't edit the hint...
    }
    closeSubTask();  // For example if user is editing one subTask and then double-clicks on another
                     // without OK'ing first
    ui->listTransformFeatures->currentItem()->setSelected(true);
    int row = ui->listTransformFeatures->currentIndex().row();
    auto pcMultiTransform = TransformedView->getObject<PartDesign::MultiTransform>();
    std::vector<App::DocumentObject*> transformFeatures =
        pcMultiTransform->Transformations.getValues();

    subFeature = static_cast<PartDesign::Transformed*>(transformFeatures[row]);
    if (subFeature->is<PartDesign::Mirrored>()) {
        subTask = new TaskMirroredParameters(this, ui->subFeatureWidget);
    }
    else if (subFeature->is<PartDesign::LinearPattern>()) {
        subTask = new TaskLinearPatternParameters(this, ui->subFeatureWidget);
    }
    else if (subFeature->is<PartDesign::PolarPattern>()) {
        subTask = new TaskPolarPatternParameters(this, ui->subFeatureWidget);
    }
    else if (subFeature->is<PartDesign::Scaled>()) {
        subTask = new TaskScaledParameters(this, ui->subFeatureWidget);
    }
    else {
        return;  // TODO: Show an error?
    }

    ui->buttonOK->show();

    subTask->setEnabledTransaction(isEnabledTransaction());
}

void TaskMultiTransformParameters::onTransformActivated(const QModelIndex& index)
{
    Q_UNUSED(index);
    onTransformEdit();
}

void TaskMultiTransformParameters::onTransformAddMirrored()
{
    closeSubTask();
    std::string newFeatName = TransformedView->getObject()->getDocument()->getUniqueObjectName("Mirrored");
    auto pcBody = dynamic_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(getTopTransformedObject()));
    if (!pcBody) {
        return;
    }

    if (isEnabledTransaction()) {
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Mirrored"));
    }

    FCMD_OBJ_CMD(pcBody, "newObject('PartDesign::Mirrored','"<<newFeatName<<"')");
    auto Feat = pcBody->getDocument()->getObject(newFeatName.c_str());
    if (!Feat) {
        return;
    }
    // Gui::Command::updateActive();
    App::DocumentObject* sketch = getSketchObject();
    if (sketch) {
        FCMD_OBJ_CMD(Feat,
                     "MirrorPlane = (" << Gui::Command::getObjectCmd(sketch) << ",['V_Axis'])");
    }
    else {
        App::Origin* orig = pcBody->getOrigin();
        FCMD_OBJ_CMD(Feat, "MirrorPlane = ("<<Gui::Command::getObjectCmd(orig->getXY())<<",[''])");
    }
    finishAdd(newFeatName);
    // show the new view when no error
    if (!Feat->isError()) {
        TransformedView->getObject()->Visibility.setValue(true);
    }
}

void TaskMultiTransformParameters::onTransformAddLinearPattern()
{
    // See CmdPartDesignLinearPattern
    //
    closeSubTask();
    std::string newFeatName = TransformedView->getObject()->getDocument()->getUniqueObjectName("LinearPattern");
    auto pcBody = dynamic_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(getTopTransformedObject()));
    if (!pcBody) {
        return;
    }

    if (isEnabledTransaction()) {
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Make LinearPattern"));
    }

    FCMD_OBJ_CMD(pcBody, "newObject('PartDesign::LinearPattern','"<<newFeatName<<"')");
    auto Feat = pcBody->getDocument()->getObject(newFeatName.c_str());
    if (!Feat) {
        return;
    }
    // Gui::Command::updateActive();
    App::DocumentObject* sketch = getSketchObject();
    if (sketch) {
        FCMD_OBJ_CMD(Feat, "Direction = (" << Gui::Command::getObjectCmd(sketch) << ",['H_Axis'])");
    }
    else {
        // set Direction value before filling up the combo box to avoid creating an empty item
        // inside updateUI()
        auto body = dynamic_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(getObject()));
        if (body) {
            FCMD_OBJ_CMD(Feat,
                         "Direction = (" << Gui::Command::getObjectCmd(body->getOrigin()->getX())
                                         << ",[''])");
        }
    }

    FCMD_OBJ_CMD(Feat, "Length = 100");
    FCMD_OBJ_CMD(Feat, "Occurrences = 2");

    finishAdd(newFeatName);
    // show the new view when no error
    if (!Feat->isError()) {
        TransformedView->getObject()->Visibility.setValue(true);
    }
}

void TaskMultiTransformParameters::onTransformAddPolarPattern()
{
    closeSubTask();
    std::string newFeatName = TransformedView->getObject()->getDocument()->getUniqueObjectName("PolarPattern");
    auto pcBody = dynamic_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(getTopTransformedObject()));
    if (!pcBody) {
        return;
    }

    if (isEnabledTransaction()) {
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "PolarPattern"));
    }

    FCMD_OBJ_CMD(pcBody, "newObject('PartDesign::PolarPattern','"<<newFeatName<<"')");
    auto Feat = pcBody->getDocument()->getObject(newFeatName.c_str());
    if (!Feat) {
        return;
    }
    // Gui::Command::updateActive();
    App::DocumentObject* sketch = getSketchObject();
    if (sketch) {
        FCMD_OBJ_CMD(Feat, "Axis = (" << Gui::Command::getObjectCmd(sketch) << ",['N_Axis'])");
    }
    else {
        App::Origin* orig = pcBody->getOrigin();
        FCMD_OBJ_CMD(Feat, "Axis = ("<<Gui::Command::getObjectCmd(orig->getX())<<",[''])");
    }
    FCMD_OBJ_CMD(Feat, "Angle = 360");
    FCMD_OBJ_CMD(Feat, "Occurrences = 2");

    finishAdd(newFeatName);
    // show the new view when no error
    if (!Feat->isError()) {
        TransformedView->getObject()->Visibility.setValue(true);
    }
}

void TaskMultiTransformParameters::onTransformAddScaled()
{
    closeSubTask();
    std::string newFeatName = TransformedView->getObject()->getDocument()->getUniqueObjectName("Scaled");
    auto pcBody = dynamic_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(getTopTransformedObject()));
    if (!pcBody) {
        return;
    }

    if (isEnabledTransaction()) {
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Scaled"));
    }

    FCMD_OBJ_CMD(pcBody, "newObject('PartDesign::Scaled','"<<newFeatName<<"')");
    auto Feat = pcBody->getDocument()->getObject(newFeatName.c_str());
    if (!Feat) {
        return;
    }
    // Gui::Command::updateActive();
    FCMD_OBJ_CMD(Feat, "Factor = 2");
    FCMD_OBJ_CMD(Feat, "Occurrences = 2");

    finishAdd(newFeatName);
    // show the new view when no error
    if (!Feat->isError()) {
        TransformedView->getObject()->Visibility.setValue(true);
    }
}

void TaskMultiTransformParameters::finishAdd(std::string& newFeatName)
{
    // Gui::Command::updateActive();
    // Gui::Command::copyVisual(newFeatName.c_str(), "ShapeColor",
    // getOriginals().front()->getNameInDocument().c_str());
    // Gui::Command::copyVisual(newFeatName.c_str(), "DisplayMode",
    // getOriginals().front()->getNameInDocument().c_str());

    setupTransaction();
    auto pcMultiTransform = TransformedView->getObject<PartDesign::MultiTransform>();
    if (editHint) {
        // Remove hint, first feature is being added
        ui->listTransformFeatures->model()->removeRow(0);
    }
    int row = ui->listTransformFeatures->currentIndex().row();
    if (row < 0) {
        // Happens when first row (first transformation) is created
        // Hide all the originals now (hiding them in Command.cpp presents the user with an empty
        // screen!)
        hideBase();
    }

    // Insert new transformation after the selected row
    // This means that in order to insert at the beginning, the user has to use "Move Up" in the
    // menu
    App::DocumentObject* newFeature =
        pcMultiTransform->getDocument()->getObject(newFeatName.c_str());
    std::vector<App::DocumentObject*> transformFeatures =
        pcMultiTransform->Transformations.getValues();
    if (row == ui->listTransformFeatures->model()->rowCount() - 1) {
        // Note: Inserts always happen before the specified iterator so in order to append at the
        // end we need to use push_back() and append()
        transformFeatures.push_back(newFeature);
        ui->listTransformFeatures->addItem(QString::fromLatin1(newFeature->Label.getValue()));
        ui->listTransformFeatures->setCurrentRow(row + 1, QItemSelectionModel::ClearAndSelect);
    }
    else {
        // Note: The feature tree always seems to append to the end, no matter what we say here
        transformFeatures.insert(transformFeatures.begin() + row + 1, newFeature);
        ui->listTransformFeatures->insertItem(row + 1,
                                              QString::fromLatin1(newFeature->Label.getValue()));
        ui->listTransformFeatures->setCurrentRow(row + 1, QItemSelectionModel::ClearAndSelect);
    }
    pcMultiTransform->Transformations.setValues(transformFeatures);

    recomputeFeature();

    // Set state to hidden - only the MultiTransform should be visible
    FCMD_OBJ_HIDE(newFeature);
    editHint = false;

    onTransformEdit();
}

void TaskMultiTransformParameters::moveTransformFeature(const int increment)
{
    setupTransaction();
    int row = ui->listTransformFeatures->currentIndex().row();
    auto pcMultiTransform = TransformedView->getObject<PartDesign::MultiTransform>();
    std::vector<App::DocumentObject*> transformFeatures =
        pcMultiTransform->Transformations.getValues();

    if (transformFeatures.empty()) {
        return;
    }

    App::DocumentObject* feature = transformFeatures[row];
    transformFeatures.erase(transformFeatures.begin() + row);
    auto item = new QListWidgetItem(*(ui->listTransformFeatures->item(row)));
    ui->listTransformFeatures->model()->removeRow(row);
    // After this operation, if we were to insert at index row again, things will remain unchanged

    row += increment;

    if (row < 0) {
        row = 0;
    }

    if (row >= ui->listTransformFeatures->model()->rowCount()) {
        // Note: Inserts always happen before the specified iterator so in order to append at the
        // end we need to use push_back() and append()
        transformFeatures.push_back(feature);
        ui->listTransformFeatures->addItem(item);
        ui->listTransformFeatures->setCurrentRow(row, QItemSelectionModel::ClearAndSelect);
    }
    else {
        transformFeatures.insert(transformFeatures.begin() + row, feature);
        ui->listTransformFeatures->insertItem(row, item);
        ui->listTransformFeatures->setCurrentRow(row, QItemSelectionModel::ClearAndSelect);
    }

    pcMultiTransform->Transformations.setValues(transformFeatures);
    recomputeFeature();
}

void TaskMultiTransformParameters::onMoveUp()
{
    moveTransformFeature(-1);
}

void TaskMultiTransformParameters::onMoveDown()
{
    moveTransformFeature(+1);
}

void TaskMultiTransformParameters::onSubTaskButtonOK()
{
    closeSubTask();
}

void TaskMultiTransformParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        recomputeFeature();
    }
}

void TaskMultiTransformParameters::apply()
{
    auto pcMultiTransform = getObject<PartDesign::MultiTransform>();
    std::vector<App::DocumentObject*> transformFeatures =
        pcMultiTransform->Transformations.getValues();
    std::stringstream str;
    str << Gui::Command::getObjectCmd(TransformedView->getObject()) << ".Transformations = [";
    for (auto it : transformFeatures) {
        if (it) {
            str << Gui::Command::getObjectCmd(it) << ",";
        }
    }
    str << "]";
    Gui::Command::runCommand(Gui::Command::Doc, str.str().c_str());
}

TaskMultiTransformParameters::~TaskMultiTransformParameters()
{
    try {
        closeSubTask();
    }
    catch (const Py::Exception&) {
        Base::PyException exc;  // extract the Python error text
        exc.ReportException();
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgMultiTransformParameters::TaskDlgMultiTransformParameters(
    ViewProviderMultiTransform* MultiTransformView)
    : TaskDlgTransformedParameters(MultiTransformView)
{
    parameter = new TaskMultiTransformParameters(MultiTransformView);
    parameter->setEnabledTransaction(false);

    Content.push_back(parameter);
}

#include "moc_TaskMultiTransformParameters.cpp"
