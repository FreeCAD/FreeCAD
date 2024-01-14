/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
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

#ifndef GUI_TASKMEASURE_H
#define GUI_TASKMEASURE_H

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QApplication>
# include <QKeyEvent>
#endif


#include "TaskMeasure.h"

#include "Control.h"
#include "MainWindow.h"
#include "Application.h"
#include "App/Document.h"
#include "App/DocumentObjectGroup.h"
#include <Gui/BitmapFactory.h>

#include <QFormLayout>
#include <QPushButton>

using namespace Gui;


TaskMeasure::TaskMeasure()
{
    qApp->installEventFilter(this);

    this->setButtonPosition(TaskMeasure::South);
    auto taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("umf-measurement"), tr("Measurement"), true, nullptr);

    // Create mode dropdown and add all registered measuretypes
    modeSwitch = new QComboBox();
    modeSwitch->addItem(QString::fromLatin1("Auto"));

    for (App::MeasureType* mType : App::GetApplication().getMeasureTypes()){
        modeSwitch->addItem(QString::fromLatin1(mType->label.c_str()));
    }

    // Connect dropdown's change signal to our onModeChange slot
    connect(modeSwitch, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskMeasure::onModeChanged);

    // Result widget
    valueResult = new QLineEdit();
    valueResult->setReadOnly(true);

    // Main layout
    QBoxLayout *layout = taskbox->groupLayout();

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setHorizontalSpacing(10);
    // Note: How can the split between columns be kept in the middle?
    // formLayout->setFieldGrowthPolicy(QFormLayout::FieldGrowthPolicy::ExpandingFieldsGrow);
    formLayout->setFormAlignment(Qt::AlignCenter);

    formLayout->addRow(QString::fromLatin1("Mode:"), modeSwitch);
    formLayout->addRow(QString::fromLatin1("Result:"), valueResult);
    layout->addLayout(formLayout);

    Content.emplace_back(taskbox);

    // engage the selectionObserver
    attachSelection();

    if(!App::GetApplication().getActiveTransaction())
        App::GetApplication().setActiveTransaction("Add Measurement");


    // Call invoke method delayed, otherwise the dialog might not be fully initialized
    QTimer::singleShot(0, this, &TaskMeasure::invoke);
}

TaskMeasure::~TaskMeasure(){
    detachSelection();
    qApp->removeEventFilter(this);
}


void TaskMeasure::modifyStandardButtons(QDialogButtonBox* box) {

    QPushButton* btn = box->button(QDialogButtonBox::Ok);
    btn->setText(tr("Annotate"));
    btn->setToolTip(tr("Press the Annotate button to add measurement to the document."));

    // Disable button by default
    btn->setEnabled(false);
    btn = box->button(QDialogButtonBox::Abort);
    btn->setText(QString::fromLatin1("Close"));
    btn->setToolTip(tr("Press the Close button to exit."));

    // Connect reset button
    btn = box->button(QDialogButtonBox::Reset);
    connect(btn, &QPushButton::released, this, &TaskMeasure::reset);
}

bool canAnnotate(Measure::MeasureBase* obj) {
    if (obj == nullptr) {
        // null object, can't annotate this
        return false;
    }

    auto vpName = obj->getViewProviderName();
    // if there is not a vp, return false
    if ((vpName == NULL) || (vpName[0] == '\0')){
        return false;
    }

    return true;
}

void TaskMeasure::enableAnnotateButton(bool state) {
    // if the task ui is not init yet we don't have a button box.
    if (!this->buttonBox) {
        return;
    }
    // Enable/Disable annotate button
    auto btn = this->buttonBox->button(QDialogButtonBox::Ok);
    btn->setEnabled(state);
}

void TaskMeasure::setMeasureObject(Measure::MeasureBase* obj) {
    _mMeasureObject = obj;
}


void TaskMeasure::update() {
    valueResult->setText(QString::asprintf("-"));

    // Report selection stack
    Base::Console().Message("Selection: ");
    for (std::tuple<std::string, std::string> elem : selection) {
        Base::Console().Message("%s ", std::get<1>(elem));
    }
    Base::Console().Message("\n");


    // Get valid measure type
    App::MeasureType *measureType(nullptr);


    std::string mode = explicitMode ? modeSwitch->currentText().toStdString() : "";


    auto measureTypes = App::GetApplication().getValidMeasureTypes(selection, mode);
    if (measureTypes.size() > 0) {
        measureType = measureTypes.front();
    }
    

    if (!measureType) {

        // Note: If there's no valid measure type we might just restart the selection,
        // however this requires enough coverage of measuretypes that we can access all of them
        
        // std::tuple<std::string, std::string> sel = selection.back();
        // clearSelection();
        // addElement(measureModule.c_str(), get<0>(sel).c_str(), get<1>(sel).c_str());

        // Reset measure object
        removeObject();
        enableAnnotateButton(false);
        return;
    }

    // Update tool mode display
    setModeSilent(measureType);

    if (!_mMeasureObject || measureType->measureObject != _mMeasureObject->getTypeId().getName()) {
        // we don't already have a measureobject or it isn't the same type as the new one
        removeObject();

        App::Document *doc = App::GetApplication().getActiveDocument();
        if (measureType->isPython) {
            Base::PyGILStateLocker lock;
            auto pyMeasureClass = measureType->pythonClass;
            
            // Create a MeasurePython instance
            auto featurePython = doc->addObject("Measure::MeasurePython", measureType->label.c_str());
            setMeasureObject((Measure::MeasureBase*)featurePython);

            // Create an instance of the pyMeasureClass, the classe's initializer sets the object as proxy
            Py::Tuple args(1);
            args.setItem(0, Py::asObject(featurePython->getPyObject()));
            PyObject_CallObject(pyMeasureClass, args.ptr());
        }
        else {
            // Create measure object
            setMeasureObject(
                (Measure::MeasureBase*)doc->addObject(measureType->measureObject.c_str(), measureType->label.c_str())
            );
        }
    }

    // we have a valid measure object so we can enable the annotate button
    enableAnnotateButton(true);

    // Fill measure object's properties from selection
    _mMeasureObject->parseSelection(selection);

    // Get result
    valueResult->setText(_mMeasureObject->getResultString());
}

void TaskMeasure::close(){
    Control().closeDialog();
}


void ensureGroup(Measure::MeasureBase* measurement) {
    // Ensure measurement object is part of the measurements group

    const char* measurementGroupName = "Measurements";
    if (measurement == nullptr) {
        return;
    }

    App::Document* doc = App::GetApplication().getActiveDocument();
    App::DocumentObject* obj = doc->getObject(measurementGroupName);
    if (!obj || !obj->isValid()) {
        obj = doc->addObject("App::DocumentObjectGroup", measurementGroupName);
    }

    auto group = static_cast<App::DocumentObjectGroup*>(obj);
    group->addObject(measurement);
}


// Runs after the dialog is created
void TaskMeasure::invoke() {
    gatherSelection();
}

bool TaskMeasure::accept(){
    ensureGroup(_mMeasureObject);
    close();

    // Commit transaction
    App::GetApplication().closeActiveTransaction();
    return false;
}

bool TaskMeasure::reject(){
    removeObject();
    close();

    // Abort transaction
    App::GetApplication().closeActiveTransaction(true);
    return false;
}

void TaskMeasure::reset() {
    // Reset tool state
    this->clearSelection();

    // Should the explicit mode also be reset? 
    // setModeSilent(nullptr);
    // explicitMode = false;

    this->update();
}


void TaskMeasure::addElement(const char* mod, const char* objectName, const char* subName) {

    if (!App::GetApplication().hasMeasureHandler(mod)) {
        Base::Console().Message("No measure handler available for geometry of module: %s\n", mod);
        return;
    }

    selection.emplace_back(std::make_tuple((std::string)objectName, (std::string)subName));
    update();
}

void TaskMeasure::gatherSelection() {
    // Fills the selection stack from the global selection and triggers an update

    if (!Gui::Selection().hasSelection()) {
        return;
    }

    App::Document* doc = App::GetApplication().getActiveDocument();

    for (auto sel : Gui::Selection().getSelection()) {
        const char* objectName = sel.pObject->getNameInDocument();
        App::DocumentObject* ob = doc->getObject(objectName);
        auto sub = ob->getSubObject(sel.SubName);
        std::string mod = sub->getClassTypeId().getModuleName(sub->getTypeId().getName());

        selection.emplace_back(objectName, sel.SubName);
    }

    update();
}

void TaskMeasure::removeObject() {
    if (_mMeasureObject == nullptr) {
        return;
    }
    if (_mMeasureObject->isRemoving() ) {
        return;
    }
    _mMeasureObject->getDocument()->removeObject (_mMeasureObject->getNameInDocument());
    setMeasureObject(nullptr);
}

bool TaskMeasure::hasSelection(){
    return !selection.empty();
}

void TaskMeasure::clearSelection(){
    selection.clear();
}

void TaskMeasure::onSelectionChanged(const Gui::SelectionChanges& msg)
{

    if (msg.Type != SelectionChanges::AddSelection && msg.Type != SelectionChanges::RmvSelection
        && msg.Type != SelectionChanges::SetSelection && msg.Type != SelectionChanges::ClrSelection) {

        return;
        }


    // Add Element 
    if (msg.Type == SelectionChanges::AddSelection || msg.Type == SelectionChanges::SetSelection) {

        App::Document* doc = App::GetApplication().getActiveDocument();
        App::DocumentObject* ob = doc->getObject(msg.pObjectName);
        App::DocumentObject* sub = ob->getSubObject(msg.pSubName);
        std::string mod = sub->getClassTypeId().getModuleName(sub->getTypeId().getName());

        addElement(mod.c_str(), msg.pObjectName, msg.pSubName);
    }

    // TODO: should there be an update here to reflect the new selection??
}

bool TaskMeasure::eventFilter(QObject* obj, QEvent* event) {

    if (event->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Escape) {

            if (this->hasSelection()) {
                this->reset();
            } else {
                this->reject();
            }

            return true;
        }

        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            this->accept();
            return true;
        }
    }

    return TaskDialog::eventFilter(obj, event);
}

void TaskMeasure::onModeChanged(int index) {
    explicitMode = (index != 0);
    this->update();
}

void TaskMeasure::setModeSilent(App::MeasureType* mode) {
    modeSwitch->blockSignals(true);
    
    if (mode == nullptr) {
        modeSwitch->setCurrentIndex(0);
    }
    else {
        modeSwitch->setCurrentText(QString::fromLatin1(mode->label.c_str()));
    }
    modeSwitch->blockSignals(false);
}

// Get explicitly set measure type from the mode switch
App::MeasureType* TaskMeasure::getMeasureType() {
    for (App::MeasureType* mType : App::GetApplication().getMeasureTypes()) {
        if (mType->label.c_str() == modeSwitch->currentText().toLatin1()) {
            return mType;
        }
    }
    return nullptr;
}


#endif //GUI_TASKMEASURE_H
