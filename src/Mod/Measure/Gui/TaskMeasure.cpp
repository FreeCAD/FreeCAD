/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#include <QApplication>
#include <QKeyEvent>
#endif


#include "TaskMeasure.h"

#include <App/DocumentObjectGroup.h>
#include <App/Link.h>
#include <Mod/Measure/App/MeasureDistance.h>
#include <App/PropertyStandard.h>
#include <Gui/MainWindow.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Gui/ViewProvider.h>

#include <QFormLayout>
#include <QPushButton>
#include <QSettings>

using namespace Gui;

namespace
{
constexpr auto taskMeasureSettingsGroup = "TaskMeasure";
constexpr auto taskMeasureShowDeltaSettingsName = "ShowDelta";
}  // namespace

TaskMeasure::TaskMeasure()
{
    qApp->installEventFilter(this);

    this->setButtonPosition(TaskMeasure::South);
    auto taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("umf-measurement"),
                                              tr("Measurement"),
                                              true,
                                              nullptr);

    QSettings settings;
    settings.beginGroup(QLatin1String(taskMeasureSettingsGroup));
    delta = settings.value(QLatin1String(taskMeasureShowDeltaSettingsName), true).toBool();

    showDelta = new QCheckBox();
    showDelta->setChecked(delta);
    showDeltaLabel = new QLabel(tr("Show Delta:"));
    connect(showDelta, &QCheckBox::stateChanged, this, &TaskMeasure::showDeltaChanged);

    // Create mode dropdown and add all registered measuretypes
    modeSwitch = new QComboBox();
    modeSwitch->addItem(QString::fromLatin1("Auto"));

    for (App::MeasureType* mType : App::MeasureManager::getMeasureTypes()) {
        modeSwitch->addItem(QString::fromLatin1(mType->label.c_str()));
    }

    // Connect dropdown's change signal to our onModeChange slot
    connect(modeSwitch,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &TaskMeasure::onModeChanged);

    // Result widget
    valueResult = new QLineEdit();
    valueResult->setReadOnly(true);

    // Main layout
    QBoxLayout* layout = taskbox->groupLayout();

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setHorizontalSpacing(10);
    // Note: How can the split between columns be kept in the middle?
    // formLayout->setFieldGrowthPolicy(QFormLayout::FieldGrowthPolicy::ExpandingFieldsGrow);
    formLayout->setFormAlignment(Qt::AlignCenter);

    formLayout->addRow(tr("Mode:"), modeSwitch);
    formLayout->addRow(showDeltaLabel, showDelta);
    formLayout->addRow(tr("Result:"), valueResult);
    layout->addLayout(formLayout);

    Content.emplace_back(taskbox);

    // engage the selectionObserver
    attachSelection();

    // Set selection style
    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::GreedySelection);

    if (!App::GetApplication().getActiveTransaction()) {
        App::GetApplication().setActiveTransaction("Add Measurement");
    }

    setAutoCloseOnDeletedDocument(true);
    // Call invoke method delayed, otherwise the dialog might not be fully initialized
    QTimer::singleShot(0, this, &TaskMeasure::invoke);
}

TaskMeasure::~TaskMeasure()
{
    Gui::Selection().setSelectionStyle(Gui::SelectionSingleton::SelectionStyle::NormalSelection);
    detachSelection();
    qApp->removeEventFilter(this);
}


void TaskMeasure::modifyStandardButtons(QDialogButtonBox* box)
{

    QPushButton* btn = box->button(QDialogButtonBox::Apply);
    btn->setText(tr("Save"));
    btn->setToolTip(tr("Save the measurement in the active document."));
    connect(btn, &QPushButton::released, this, &TaskMeasure::apply);

    // Disable button by default
    btn->setEnabled(false);
    btn = box->button(QDialogButtonBox::Abort);
    btn->setText(tr("Close"));
    btn->setToolTip(tr("Close the measurement task."));

    // Connect reset button
    btn = box->button(QDialogButtonBox::Reset);
    connect(btn, &QPushButton::released, this, &TaskMeasure::reset);
}


void TaskMeasure::enableAnnotateButton(bool state)
{
    // if the task ui is not init yet we don't have a button box.
    if (!this->buttonBox) {
        return;
    }
    // Enable/Disable annotate button
    auto btn = this->buttonBox->button(QDialogButtonBox::Apply);
    btn->setEnabled(state);
}


Measure::MeasureBase* TaskMeasure::createObject(const App::MeasureType* measureType)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        return nullptr;
    }

    if (measureType->isPython) {
        Base::PyGILStateLocker lock;
        auto pyMeasureClass = measureType->pythonClass;

        // Create a MeasurePython instance
        auto featurePython = doc->addObject("Measure::MeasurePython", measureType->label.c_str());
        _mMeasureObject = dynamic_cast<Measure::MeasureBase*>(featurePython);

        // Create an instance of the pyMeasureClass, the classe's initializer sets the object as
        // proxy
        Py::Tuple args(1);
        args.setItem(0, Py::asObject(featurePython->getPyObject()));
        PyObject* result = PyObject_CallObject(pyMeasureClass, args.ptr());
        Py_XDECREF(result);
    }
    else {
        // Create measure object
        _mMeasureObject = dynamic_cast<Measure::MeasureBase*>(
            doc->addObject(measureType->measureObject.c_str(), measureType->label.c_str()));
    }

    return _mMeasureObject;
}


void TaskMeasure::update()
{
    App::Document* doc = App::GetApplication().getActiveDocument();

    // Reset selection if the selected object is not valid
    for (auto sel : Gui::Selection().getSelection()) {
        App::DocumentObject* ob = sel.pObject;
        App::DocumentObject* sub = ob->getSubObject(sel.SubName);

        // Resolve App::Link
        if (sub->isDerivedFrom<App::Link>()) {
            auto link = static_cast<App::Link*>(sub);
            sub = link->getLinkedObject(true);
        }

        std::string mod = Base::Type::getModuleName(sub->getTypeId().getName());
        if (!App::MeasureManager::hasMeasureHandler(mod.c_str())) {
            Base::Console().Message("No measure handler available for geometry of module: %s\n",
                                    mod);
            clearSelection();
            return;
        }
    }

    valueResult->setText(QString::asprintf("-"));

    std::string mode = explicitMode ? modeSwitch->currentText().toStdString() : "";

    App::MeasureSelection selection;
    for (auto s : Gui::Selection().getSelection(doc->getName(), ResolveMode::NoResolve)) {
        App::SubObjectT sub(s.pObject, s.SubName);

        App::MeasureSelectionItem item = {sub, Base::Vector3d(s.x, s.y, s.z)};
        selection.push_back(item);
    }

    // Get valid measure type
    App::MeasureType* measureType = nullptr;
    auto measureTypes = App::MeasureManager::getValidMeasureTypes(selection, mode);
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
        if (!explicitMode) {
            setModeSilent(nullptr);
        }
        removeObject();
        enableAnnotateButton(false);
        return;
    }

    // Update tool mode display
    setModeSilent(measureType);

    if (!_mMeasureObject || measureType->measureObject != _mMeasureObject->getTypeId().getName()
        || _mMeasureObject->getDocument() != doc) {
        // we don't already have a measureobject or it isn't the same type as the new one
        removeObject();
        createObject(measureType);
    }

    // we have a valid measure object so we can enable the annotate button
    enableAnnotateButton(true);

    // Fill measure object's properties from selection
    _mMeasureObject->parseSelection(selection);

    // Get result
    valueResult->setText(_mMeasureObject->getResultString());

    // Initialite the measurement's viewprovider
    initViewObject();
}


void TaskMeasure::initViewObject()
{
    Gui::Document* guiDoc = Gui::Application::Instance->activeDocument();
    if (!guiDoc) {
        return;
    }

    Gui::ViewProvider* viewObject = guiDoc->getViewProvider(_mMeasureObject);
    if (!viewObject) {
        return;
    }

    // Init the position of the annotation
    dynamic_cast<MeasureGui::ViewProviderMeasureBase*>(viewObject)->positionAnno(_mMeasureObject);

    // Set the ShowDelta Property if it exists on the measurements view object
    auto* prop = dynamic_cast<App::PropertyBool*>(viewObject->getPropertyByName("ShowDelta"));
    setDeltaPossible(prop != nullptr);
    if (prop) {
        prop->setValue(showDelta->isChecked());
        viewObject->update(prop);
    }
}


void TaskMeasure::close()
{
    Control().closeDialog();
}


void TaskMeasure::ensureGroup(Measure::MeasureBase* measurement)
{
    // Ensure measurement object is part of the measurements group

    const char* measurementGroupName = "Measurements";
    if (measurement == nullptr) {
        return;
    }

    App::Document* doc = measurement->getDocument();
    App::DocumentObject* obj = doc->getObject(measurementGroupName);


    if (!obj || !obj->isValid()
        || !obj->isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())) {
        obj = doc->addObject("App::DocumentObjectGroup",
                             measurementGroupName,
                             true,
                             "MeasureGui::ViewProviderMeasureGroup");
    }

    auto group = static_cast<App::DocumentObjectGroup*>(obj);
    group->addObject(measurement);
}


// Runs after the dialog is created
void TaskMeasure::invoke()
{
    update();
}

bool TaskMeasure::apply()
{
    ensureGroup(_mMeasureObject);
    _mMeasureObject = nullptr;
    reset();

    // Commit transaction
    App::GetApplication().closeActiveTransaction();
    App::GetApplication().setActiveTransaction("Add Measurement");
    return false;
}

bool TaskMeasure::reject()
{
    removeObject();
    close();

    // Abort transaction
    App::GetApplication().closeActiveTransaction(true);
    return false;
}

void TaskMeasure::reset()
{
    // Reset tool state
    this->clearSelection();

    // Should the explicit mode also be reset?
    // setModeSilent(nullptr);
    // explicitMode = false;

    this->update();
}


void TaskMeasure::removeObject()
{
    if (_mMeasureObject == nullptr) {
        return;
    }
    if (_mMeasureObject->isRemoving()) {
        return;
    }

    _mMeasureObject->getDocument()->removeObject(_mMeasureObject->getNameInDocument());
    _mMeasureObject = nullptr;
}

bool TaskMeasure::hasSelection()
{
    return !Gui::Selection().getSelection().empty();
}

void TaskMeasure::clearSelection()
{
    Gui::Selection().clearSelection();
}

void TaskMeasure::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // Skip non-relevant events
    if (msg.Type != SelectionChanges::AddSelection && msg.Type != SelectionChanges::RmvSelection
        && msg.Type != SelectionChanges::SetSelection
        && msg.Type != SelectionChanges::ClrSelection) {

        return;
    }

    update();
}

bool TaskMeasure::eventFilter(QObject* obj, QEvent* event)
{

    if (event->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Escape) {

            if (this->hasSelection()) {
                this->reset();
            }
            else {
                this->reject();
            }

            return true;
        }

        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            // Save object. Indirectly dependent on whether the apply button is enabled
            // enabled if valid measurement object.
            this->buttonBox->button(QDialogButtonBox::Apply)->click();
            return true;
        }
    }

    return TaskDialog::eventFilter(obj, event);
}

void TaskMeasure::setDeltaPossible(bool possible)
{
    showDelta->setVisible(possible);
    showDeltaLabel->setVisible(possible);
}

void TaskMeasure::onModeChanged(int index)
{
    explicitMode = (index != 0);

    this->update();
}

void TaskMeasure::showDeltaChanged(int checkState)
{
    delta = checkState == Qt::CheckState::Checked;

    QSettings settings;
    settings.beginGroup(QLatin1String(taskMeasureSettingsGroup));
    settings.setValue(QLatin1String(taskMeasureShowDeltaSettingsName), delta);

    this->update();
}

void TaskMeasure::setModeSilent(App::MeasureType* mode)
{
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
App::MeasureType* TaskMeasure::getMeasureType()
{
    for (App::MeasureType* mType : App::MeasureManager::getMeasureTypes()) {
        if (mType->label.c_str() == modeSwitch->currentText().toLatin1()) {
            return mType;
        }
    }
    return nullptr;
}
