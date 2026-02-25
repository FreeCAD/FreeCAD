// SPDX-License-Identifier: LGPL-2.1-or-later

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


#include <QApplication>
#include <QKeyEvent>


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
#include <QVBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QAction>
#include <QMenu>
#include <QShortcut>
#include <QToolTip>
#include <QSignalBlocker>

#include <Base/Quantity.h>
#include <array>

using namespace MeasureGui;

namespace
{
constexpr auto taskMeasureSettingsGroup = "TaskMeasure";
constexpr auto taskMeasureShowDeltaSettingsName = "ShowDelta";
constexpr auto taskMeasureAutoSaveSettingsName = "AutoSave";
constexpr auto taskMeasureGreedySelection = "GreedySelection";

using SelectionStyle = Gui::SelectionSingleton::SelectionStyle;

constexpr std::array
    lengthUnitLabels {"nm", "µm", "mm", "cm", "dm", "m", "km", "in", "ft", "thou", "yd", "mi"};

constexpr std::array angleUnitLabels {"deg", "rad", "gon"};

constexpr std::array areaUnitLabels {"mm²", "cm²", "m²", "km²", "in²", "ft²", "yd²", "mi²"};

template<std::size_t N>
QStringList toQStringList(const std::array<const char*, N>& strings)
{
    QStringList result;
    result.reserve(N);
    for (const char* s : strings) {
        result.append(QString::fromUtf8(s));
    }
    return result;
}

QString extractUnitFromResultString(const QString& resultString)
{
    std::string str = resultString.toStdString();
    auto lastSpace = str.find_last_of(' ');

    if (lastSpace != std::string::npos && lastSpace < str.length() - 1) {
        return QString::fromStdString(str.substr(lastSpace + 1));
    }

    return QString();
}
}  // namespace

TaskMeasure::TaskMeasure()
{
    auto taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("umf-measurement"),
        tr("Measurement"),
        true,
        nullptr
    );

    setupShortcuts(taskbox);

    QSettings settings;
    settings.beginGroup(QLatin1String(taskMeasureSettingsGroup));
    delta = settings.value(QLatin1String(taskMeasureShowDeltaSettingsName), true).toBool();
    mAutoSave = settings.value(QLatin1String(taskMeasureAutoSaveSettingsName), mAutoSave).toBool();
    if (settings.value(QLatin1String(taskMeasureGreedySelection), false).toBool()) {
        Gui::Selection().setSelectionStyle(SelectionStyle::GreedySelection);
    }
    else {
        Gui::Selection().setSelectionStyle(SelectionStyle::NormalSelection);
    }
    settings.endGroup();

    showDelta = new QCheckBox();
    showDelta->setChecked(delta);
    showDeltaLabel = new QLabel(tr("Show Delta"));
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(showDelta, &QCheckBox::checkStateChanged, this, &TaskMeasure::showDeltaChanged);
#else
    connect(showDelta, &QCheckBox::stateChanged, this, &TaskMeasure::showDeltaChanged);
#endif
    autoSaveAction = new QAction(tr("Auto Save"));
    autoSaveAction->setCheckable(true);
    autoSaveAction->setChecked(mAutoSave);
    autoSaveAction->setToolTip(
        tr("Auto saving of the last measurement when starting a new "
           "measurement. Use the Shift key to temporarily invert the behaviour.")
    );
    connect(autoSaveAction, &QAction::triggered, this, &TaskMeasure::autoSaveChanged);

    newMeasurementBehaviourAction = new QAction(tr("Additive Selection"));
    newMeasurementBehaviourAction->setCheckable(true);
    newMeasurementBehaviourAction->setChecked(
        Gui::Selection().getSelectionStyle() == SelectionStyle::GreedySelection
    );
    newMeasurementBehaviourAction->setToolTip(
        tr("If checked, new selection will be added to the measurement. If unchecked, the Ctrl key "
           "must be "
           "pressed to add a "
           "selection to the current measurement otherwise a new measurement will be started")
    );
    connect(
        newMeasurementBehaviourAction,
        &QAction::triggered,
        this,
        &TaskMeasure::newMeasurementBehaviourChanged
    );

    mSettings = new QToolButton();
    mSettings->setToolTip(tr("Settings"));
    mSettings->setIcon(QIcon(QStringLiteral(":/icons/dialogs/Sketcher_Settings.svg")));
    auto* menu = new QMenu(mSettings);
    menu->setToolTipsVisible(true);
    mSettings->setMenu(menu);

    menu->addAction(autoSaveAction);
    menu->addAction(newMeasurementBehaviourAction);
    connect(mSettings, &QToolButton::clicked, mSettings, &QToolButton::showMenu);

    // Create mode dropdown and add all registered measuretypes
    modeSwitch = new QComboBox();
    modeSwitch->addItem(tr("Auto"));

    for (App::MeasureType* mType : App::MeasureManager::getMeasureTypes()) {
        modeSwitch->addItem(tr(mType->label.c_str()));
    }

    // Connect dropdown's change signal to our onModeChange slot
    connect(modeSwitch, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskMeasure::onModeChanged);

    unitSwitch = new QComboBox();
    unitSwitch->addItem(QLatin1String("-"));
    connect(unitSwitch, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskMeasure::onUnitChanged);


    // Result widget
    valueResult = new QLineEdit();
    valueResult->setReadOnly(true);

    // Main layout
    QBoxLayout* layout = taskbox->groupLayout();

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setHorizontalSpacing(10);
    formLayout->setVerticalSpacing(6);
    // Note: How can the split between columns be kept in the middle?
    // formLayout->setFieldGrowthPolicy(QFormLayout::FieldGrowthPolicy::ExpandingFieldsGrow);
    formLayout->setFormAlignment(Qt::AlignCenter);

    auto* settingsLayout = new QHBoxLayout();
    settingsLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    settingsLayout->addWidget(mSettings);
    formLayout->addRow(QLatin1String(), settingsLayout);
    formLayout->addRow(tr("Mode"), modeSwitch);

    auto* deltaLayout = new QHBoxLayout();
    deltaLayout->setContentsMargins(0, 0, 0, 0);
    deltaLayout->setSpacing(8);
    deltaLayout->addWidget(showDelta, 0, Qt::AlignVCenter | Qt::AlignLeft);
    deltaLayout->addWidget(showDeltaLabel, 0, Qt::AlignVCenter | Qt::AlignLeft);
    deltaLayout->addStretch(1);


    auto* resultLayout = new QHBoxLayout();
    resultLayout->setSpacing(8);
    resultLayout->addWidget(valueResult, 65);
    resultLayout->addWidget(unitSwitch, 30);
    formLayout->addRow(tr("Result"), resultLayout);
    formLayout->addRow(deltaLayout);
    layout->addLayout(formLayout);

    Content.emplace_back(taskbox);

    // engage the selectionObserver
    attachSelection();

    if (auto* doc = App::GetApplication().getActiveDocument()) {
        m_deletedConnection = doc->signalDeletedObject.connect([this](auto&& obj) {
            onObjectDeleted(obj);
        });
    }

    if (!App::GetApplication().getActiveTransaction()) {
        App::GetApplication().setActiveTransaction("Add Measurement");
    }

    setAutoCloseOnDeletedDocument(true);
    // Call invoke method delayed, otherwise the dialog might not be fully initialized
    QTimer::singleShot(0, this, &TaskMeasure::invoke);
}

TaskMeasure::~TaskMeasure()
{
    m_deletedConnection.disconnect();
    Gui::Selection().setSelectionStyle(SelectionStyle::NormalSelection);
    detachSelection();
}

void TaskMeasure::modifyStandardButtons(QDialogButtonBox* box)
{

    QPushButton* btn = box->button(QDialogButtonBox::Apply);
    btn->setText(QCoreApplication::translate("QPlatformTheme", "Save"));
    btn->setToolTip(tr("Saves the measurement in the active document"));
    connect(btn, &QPushButton::released, this, qOverload<>(&TaskMeasure::apply));

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


void TaskMeasure::createObject(const App::MeasureType* measureType)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        return;
    }

    if (measureType->isPython) {
        Base::PyGILStateLocker lock;
        auto pyMeasureClass = measureType->pythonClass;

        // Create a MeasurePython instance
        // Note: writing addObject<Measure::MeasurePython>() is not yet supported because
        // getClassName() will determine the string 'App::FeaturePythonT<FeatureT>' instead
        // of 'Measure::MeasurePython'
        auto featurePython = doc->addObject("Measure::MeasurePython", measureType->label.c_str());
        _mMeasureObject = dynamic_cast<Measure::MeasureBase*>(featurePython);

        // Create an instance of the pyMeasureClass, the classe's initializer sets the object as
        // proxy
        Py::Tuple args(1);
        args.setItem(0, Py::asObject(_mMeasureObject->getPyObject()));
        PyObject* result = PyObject_CallObject(pyMeasureClass, args.ptr());
        Py_XDECREF(result);
    }
    else {
        // Create measure object
        _mMeasureObject = dynamic_cast<Measure::MeasureBase*>(
            doc->addObject(measureType->measureObject.c_str(), measureType->label.c_str())
        );
    }
}

void TaskMeasure::update()
{
    try {
        tryUpdate();
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
}

void TaskMeasure::tryUpdate()
{
    App::Document* doc = App::GetApplication().getActiveDocument();

    // Reset selection if the selected object is not valid
    for (auto sel : Gui::Selection().getSelection()) {
        App::DocumentObject* ob = sel.pObject;
        App::DocumentObject* sub = ob->getSubObject(sel.SubName);

        // Resolve App::Link
        if (auto link = freecad_cast<App::Link*>(sub)) {
            sub = link->getLinkedObject(true);
        }

        std::string mod = Base::Type::getModuleName(sub->getTypeId().getName());
        if (!App::MeasureManager::hasMeasureHandler(mod.c_str())) {
            Base::Console().message("No measure handler available for geometry of module: %s\n", mod);
            clearSelection();
            return;
        }
    }

    valueResult->setText(QString::asprintf("-"));

    std::string mode = explicitMode ? modeSwitch->currentText().toStdString() : "";

    App::MeasureSelection selection;
    for (auto s : Gui::Selection().getSelection(doc->getName(), Gui::ResolveMode::NoResolve)) {
        App::SubObjectT sub(s.pObject, s.SubName);

        App::MeasureSelectionItem item = {sub, Base::Vector3d(s.x, s.y, s.z)};
        selection.push_back(item);
    }

    // Get valid measure type
    App::MeasureType* measureType = nullptr;
    auto measureTypes = App::MeasureManager::getValidMeasureTypes(selection, mode);
    if (!measureTypes.empty()) {
        measureType = measureTypes.front();
    }


    if (!measureType) {
        QSignalBlocker unitSwitchBlocker(unitSwitch);
        unitSwitch->clear();
        unitSwitch->addItem(QLatin1String("-"));
        mLastUnitSelection = QLatin1String("-");

        // Reset measure object
        if (!explicitMode) {
            setModeSilent(nullptr);
        }
        removeObject();
        enableAnnotateButton(false);
        return;
    }

    updateUnitDropdown(measureType);

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

    if (_mMeasureObject) {
        // Fill measure object's properties from selection
        _mMeasureObject->parseSelection(selection);

        setUnitFromResultString();

        updateResultWithUnit();

        // Initialite the measurement's viewprovider
        initViewObject(_mMeasureObject);
    }
    _mMeasureObject->purgeTouched();
}

void TaskMeasure::updateUnitDropdown(const App::MeasureType* measureType)
{
    const QString previousUnit = unitSwitch->currentText();
    QStringList units;

    if (measureType->identifier == "LENGTH" || measureType->identifier == "DISTANCE"
        || measureType->identifier == "DISTANCEFREE" || measureType->identifier == "RADIUS"
        || measureType->identifier == "DIAMETER" || measureType->identifier == "POSITION"
        || measureType->identifier == "CENTEROFMASS") {
        units = toQStringList(lengthUnitLabels);
    }
    else if (measureType->identifier == "ANGLE") {
        units = toQStringList(angleUnitLabels);
    }
    else if (measureType->identifier == "AREA") {
        units = toQStringList(areaUnitLabels);
    }
    else {
        units.clear();
    }

    QSignalBlocker unitSwitchBlocker(unitSwitch);

    unitSwitch->clear();
    if (!units.isEmpty()) {
        unitSwitch->addItems(units);
        // If unit from the same category was previously selected keep it
        if (!previousUnit.isEmpty()) {
            int unitIndex = unitSwitch->findText(previousUnit);
            if (unitIndex >= 0) {
                unitSwitch->setCurrentIndex(unitIndex);
            }
        }
    }
}

void TaskMeasure::setUnitFromResultString()
{
    if (!_mMeasureObject) {
        return;
    }

    // Only set default unit if user hasn't made a selection yet
    if (mLastUnitSelection != QLatin1String("-") && !mLastUnitSelection.isEmpty()) {
        return;
    }

    QString resultString = _mMeasureObject->getResultString();
    QString unitFromResult = extractUnitFromResultString(resultString);

    if (unitFromResult.isEmpty()) {
        return;
    }

    int unitIndex = unitSwitch->findText(unitFromResult);
    if (unitIndex >= 0) {
        QSignalBlocker unitSwitchBlocker(unitSwitch);
        unitSwitch->setCurrentIndex(unitIndex);

        mLastUnitSelection = unitFromResult;
    }
}

void TaskMeasure::updateResultWithUnit()
{
    if (!_mMeasureObject) {
        return;
    }

    QString resultString = _mMeasureObject->getResultString();
    QString currentUnit = unitSwitch->currentText();

    if (currentUnit != QLatin1String("-") && !resultString.isEmpty()) {
        Base::Quantity resultQty = Base::Quantity::parse(resultString.toStdString());
        // Parse unit string like "1 mm" to get the target quantity
        Base::Quantity targetUnit = Base::Quantity::parse(
            (QLatin1String("1 ") + currentUnit).toStdString()
        );
        double convertedValue = resultQty.getValueAs(targetUnit);

        QString formattedValue;
        // 4 decimal places, if between -1 and 1: 4 significant digits
        if (std::abs(convertedValue) < 1.0 && convertedValue != 0.0) {
            formattedValue = QString::number(convertedValue, 'g', 4);
        }
        else {
            formattedValue = QString::number(convertedValue, 'f', 4);
        }

        QString formattedResult = formattedValue + QLatin1String(" ") + currentUnit;
        valueResult->setText(formattedResult);
    }
    else {
        valueResult->setText(resultString);
    }
}


void TaskMeasure::initViewObject(Measure::MeasureBase* measure)
{
    Gui::Document* guiDoc = Gui::Application::Instance->activeDocument();
    if (!guiDoc) {
        return;
    }

    Gui::ViewProvider* viewObject = guiDoc->getViewProvider(measure);
    if (!viewObject) {
        return;
    }

    // Init the position of the annotation
    dynamic_cast<MeasureGui::ViewProviderMeasureBase*>(viewObject)->positionAnno(measure);

    // Set the ShowDelta Property if it exists on the measurements view object
    auto* prop = viewObject->getPropertyByName<App::PropertyBool>("ShowDelta");
    setDeltaPossible(prop != nullptr);
    if (prop) {
        prop->setValue(showDelta->isChecked());
        viewObject->update(prop);
    }
}


void TaskMeasure::closeDialog()
{
    Gui::Control().closeDialog();
}


void TaskMeasure::ensureGroup(Measure::MeasureBase* measurement)
{
    // Ensure measurement object is part of the measurements group

    const char* measurementGroupName = "Measurements";
    if (measurement == nullptr) {
        return;
    }

    App::Document* doc = measurement->getDocument();
    auto group = dynamic_cast<App::DocumentObjectGroup*>(doc->getObject(measurementGroupName));
    if (!group || !group->isValid()) {
        group = doc->addObject<App::DocumentObjectGroup>(
            measurementGroupName,
            true,
            "MeasureGui::ViewProviderMeasureGroup"
        );
    }

    group->addObject(measurement);
    group->purgeTouched();
}


// Runs after the dialog is created
void TaskMeasure::invoke()
{
    update();
}

bool TaskMeasure::apply()
{
    return apply(true);
}

bool TaskMeasure::apply(bool reset)
{
    ensureGroup(_mMeasureObject);
    _mMeasureObject = nullptr;
    if (reset) {
        this->reset();
    }

    // Commit transaction
    App::GetApplication().closeActiveTransaction();
    App::GetApplication().setActiveTransaction("Add Measurement");
    return false;
}

bool TaskMeasure::reject()
{
    removeObject();
    closeDialog();

    // Abort transaction
    App::GetApplication().closeActiveTransaction(true);
    return false;
}

void TaskMeasure::closed()
{
    reject();
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
    if (msg.Type != Gui::SelectionChanges::AddSelection
        && msg.Type != Gui::SelectionChanges::RmvSelection
        && msg.Type != Gui::SelectionChanges::SetSelection
        && msg.Type != Gui::SelectionChanges::ClrSelection) {

        return;
    }

    // If the control modifier is pressed, the object is just added to the current measurement
    // If the control modifier is not pressed, a new measurement will be started. If autosave is on,
    // the old measurement will be saved otherwise discharded. Shift inverts the autosave behaviour
    // temporarily
    const auto modifier = QGuiApplication::keyboardModifiers();
    const bool ctrl = (modifier & Qt::ControlModifier) > 0;
    const bool shift = (modifier & Qt::ShiftModifier) > 0;
    // shift inverts the current state temporarily
    const auto autosave = (mAutoSave && !shift) || (!mAutoSave && shift);
    if ((!ctrl && Gui::Selection().getSelectionStyle() == SelectionStyle::NormalSelection)
        || (ctrl && Gui::Selection().getSelectionStyle() == SelectionStyle::GreedySelection)) {
        if (autosave && buttonBox && buttonBox->button(QDialogButtonBox::Apply)->isEnabled()) {
            apply(false);
        }
    }
    update();
}

void TaskMeasure::setupShortcuts(QWidget* parent)
{
    auto shortcutSave = new QShortcut(parent);
    shortcutSave->setKey(QKeySequence(QStringLiteral("Return")));
    shortcutSave->setContext(Qt::ApplicationShortcut);
    connect(shortcutSave, &QShortcut::activated, this, &TaskMeasure::saveMeasurement);

    auto shortcutQuit = new QShortcut(parent);
    shortcutQuit->setKey(QKeySequence(QStringLiteral("ESC")));
    shortcutQuit->setContext(Qt::ApplicationShortcut);
    connect(shortcutQuit, &QShortcut::activated, this, &TaskMeasure::quitMeasurement);
}

void TaskMeasure::saveMeasurement()
{
    // Save object. Indirectly dependent on whether the apply button is enabled
    // enabled if valid measurement object.
    if (buttonBox) {
        buttonBox->button(QDialogButtonBox::Apply)->click();
    }
}

void TaskMeasure::quitMeasurement()
{
    if (this->hasSelection()) {
        this->reset();
    }
    else {
        this->reject();
    }
}

void TaskMeasure::onObjectDeleted(const App::DocumentObject& obj)
{
    if (&obj == _mMeasureObject) {
        _mMeasureObject = nullptr;
    }
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

void TaskMeasure::onUnitChanged(int index)
{
    const QString currentUnit = unitSwitch->itemText(index);
    const auto dash = QLatin1String("-");

    if (currentUnit != mLastUnitSelection && (mLastUnitSelection != dash || currentUnit != dash)) {
        updateResultWithUnit();
    }

    mLastUnitSelection = currentUnit;
}

void TaskMeasure::showDeltaChanged(int checkState)
{
    delta = checkState == Qt::CheckState::Checked;

    QSettings settings;
    settings.beginGroup(QLatin1String(taskMeasureSettingsGroup));
    settings.setValue(QLatin1String(taskMeasureShowDeltaSettingsName), delta);
    settings.endGroup();
    settings.sync();  // immediate write to the settings file

    this->update();
}

void TaskMeasure::autoSaveChanged(bool checked)
{
    mAutoSave = checked;

    QSettings settings;
    settings.beginGroup(QLatin1String(taskMeasureSettingsGroup));
    settings.setValue(QLatin1String(taskMeasureAutoSaveSettingsName), mAutoSave);
    settings.endGroup();
}

void TaskMeasure::newMeasurementBehaviourChanged(bool checked)
{
    QSettings settings;
    settings.beginGroup(QLatin1String(taskMeasureSettingsGroup));
    if (!checked) {
        Gui::Selection().setSelectionStyle(SelectionStyle::NormalSelection);
        settings.setValue(QLatin1String(taskMeasureGreedySelection), false);
    }
    else {
        Gui::Selection().setSelectionStyle(SelectionStyle::GreedySelection);
        settings.setValue(QLatin1String(taskMeasureGreedySelection), true);
    }
    settings.endGroup();
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
