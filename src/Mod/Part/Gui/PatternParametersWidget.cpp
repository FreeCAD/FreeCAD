// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 AstoCAD                  <hello@astocad.com>        *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <QDebug>
#include <QTimer>
#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <QFormLayout>

#include "ui_PatternParametersWidget.h"
#include "PatternParametersWidget.h"

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <App/PropertyUnits.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Gui/ComboLinks.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/SpinBox.h>
#include <Gui/BitmapFactory.h>
#include <Base/Console.h>

using namespace PartGui;

PatternParametersWidget::PatternParametersWidget(PatternType type, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_PatternParametersWidget)
    , type(type)
{
    ui->setupUi(this);
    setupUiElements();
    connectSignals();
}

PatternParametersWidget::~PatternParametersWidget() = default;

void PatternParametersWidget::setupUiElements()
{
    // Configure UI elements if needed (e.g., icons for mode)
    QIcon iconExtent = Gui::BitmapFactory().iconFromTheme("Part_LinearPattern_extent");
    QIcon iconSpacing = Gui::BitmapFactory().iconFromTheme("Part_LinearPattern_spacing");

    ui->comboMode->setItemIcon(0, iconExtent);
    ui->comboMode->setItemIcon(1, iconSpacing);

    if (type == PatternType::Polar) {
        setTitle(tr("Axis"));
    }

    // Set combo box helper
    dirLinks.setCombo(ui->comboDirection);

    ParameterGrp::handle hPart = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Part"
    );
    ui->addSpacingButton->setVisible(hPart->GetBool("ExperimentalFeatures", false));

    ui->enableCheckbox->setVisible(false);
}

void PatternParametersWidget::connectSignals()
{
    connect(
        ui->comboDirection,
        qOverload<int>(&QComboBox::activated),
        this,
        &PatternParametersWidget::onDirectionChanged
    );
    connect(ui->PushButtonReverse, &QToolButton::pressed, this, &PatternParametersWidget::onReversePressed);
    connect(
        ui->comboMode,
        qOverload<int>(&QComboBox::activated),
        this,
        &PatternParametersWidget::onModeChanged
    );

    connect(
        ui->spinExtent,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &PatternParametersWidget::onLengthChanged
    );
    connect(
        ui->spinSpacing,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &PatternParametersWidget::onOffsetChanged
    );
    connect(
        ui->spinOccurrences,
        &Gui::UIntSpinBox::unsignedChanged,
        this,
        &PatternParametersWidget::onOccurrencesChanged
    );

    // Dynamic spacing buttons
    connect(
        ui->addSpacingButton,
        &QToolButton::clicked,
        this,
        &PatternParametersWidget::onAddSpacingButtonClicked
    );

    connect(ui->groupBox, &QGroupBox::toggled, this, &PatternParametersWidget::onGroupBoxToggled);
    connect(
        ui->enableCheckbox,
        &QCheckBox::toggled,
        this,
        &PatternParametersWidget::onEnableCheckBoxToggled
    );
    // Note: Connections for dynamic rows are done in addSpacingRow()
}

void PatternParametersWidget::bindProperties(
    App::PropertyLinkSub* directionProp,
    App::PropertyBool* reversedProp,
    App::PropertyEnumeration* modeProp,
    App::PropertyQuantity* lengthProp,
    App::PropertyQuantity* offsetProp,
    App::PropertyFloatList* spacingPatternProp,
    App::PropertyIntegerConstraint* occurrencesProp,
    App::DocumentObject* feature
)
{
    // Store pointers to the properties
    m_directionProp = directionProp;
    m_reversedProp = reversedProp;
    m_modeProp = modeProp;
    m_extentProp = lengthProp;
    m_spacingProp = offsetProp;
    m_spacingPatternProp = spacingPatternProp;
    m_occurrencesProp = occurrencesProp;
    m_feature = feature;  // Store feature for context (units, etc.)

    ui->spinExtent->bind(*m_extentProp);
    Base::Unit unit = type == PatternType::Linear ? Base::Unit::Length : Base::Unit::Angle;

    ui->spinExtent->blockSignals(true);
    ui->spinExtent->setUnit(unit);
    ui->spinExtent->blockSignals(false);

    ui->spinSpacing->bind(*m_spacingProp);
    ui->spinSpacing->blockSignals(true);
    ui->spinSpacing->setUnit(unit);
    ui->spinSpacing->blockSignals(false);

    ui->spinOccurrences->bind(*m_occurrencesProp);
    ui->spinOccurrences->blockSignals(true);
    ui->spinOccurrences->setMaximum(m_occurrencesProp->getMaximum());
    ui->spinOccurrences->setMinimum(m_occurrencesProp->getMinimum());
    ui->spinOccurrences->blockSignals(false);

    if (ui->groupBox->isCheckable()) {
        setChecked(m_occurrencesProp->getValue() > 1);
    }

    // Initial UI update from properties
    updateUI();
}

void PatternParametersWidget::addDirection(
    App::DocumentObject* linkObj,
    const std::string& linkSubname,
    const QString& itemText,
    int userData
)
{
    // Insert custom directions before "Select reference..."
    dirLinks.addLink(linkObj, linkSubname, itemText, userData);
}

void PatternParametersWidget::updateUI()
{
    if (blockUpdate || !m_feature) {  // Need properties to be bound
        return;
    }
    Base::StateLocker locker(blockUpdate, true);

    // Update direction combo
    if (dirLinks.setCurrentLink(*m_directionProp) == -1) {
        // failed to set current, because the link isn't in the list yet
        if (m_directionProp->getValue()) {
            QString refStr = QStringLiteral("%1:%2").arg(
                QString::fromLatin1(m_directionProp->getValue()->getNameInDocument()),
                QString::fromLatin1(m_directionProp->getSubValues().front().c_str())
            );
            dirLinks.addLink(*m_directionProp, refStr);
            dirLinks.setCurrentLink(*m_directionProp);
        }
    }

    // Update other controls directly from properties
    ui->comboMode->setCurrentIndex(m_modeProp->getValue());
    ui->spinExtent->setValue(m_extentProp->getValue());
    ui->spinSpacing->setValue(m_spacingProp->getValue());
    ui->spinOccurrences->setValue(m_occurrencesProp->getValue());

    rebuildDynamicSpacingUI();

    adaptVisibilityToMode();
}

void PatternParametersWidget::onGroupBoxToggled(bool checked)
{
    if (blockUpdate || !m_occurrencesProp) {
        return;
    }

    if (!checked) {
        // When unchecked, the pattern in this direction is disabled.
        // Set occurrences to 1, which effectively removes the pattern effect.
        if (m_occurrencesProp->getValue() != 1) {
            ui->spinOccurrences->setValue(1);
        }

        ui->groupBox->setVisible(false);
        ui->enableCheckbox->setVisible(true);
        ui->enableCheckbox->setChecked(false);
    }
}

void PatternParametersWidget::onEnableCheckBoxToggled(bool checked)
{
    if (blockUpdate || !m_occurrencesProp) {
        return;
    }

    if (checked) {
        // When unchecked, the pattern in this direction is disabled.
        // Set occurrences to 1, which effectively removes the pattern effect.
        ui->groupBox->setChecked(true);
        ui->groupBox->setVisible(true);
        ui->enableCheckbox->setVisible(false);
    }
}

void PatternParametersWidget::adaptVisibilityToMode()
{
    if (!m_modeProp) {
        return;
    }
    // Use the enum names defined in FeatureLinearPattern.h
    auto mode = static_cast<PartGui::PatternMode>(m_modeProp->getValue());

    ui->formLayout->labelForField(ui->spinExtent)->setVisible(mode == PartGui::PatternMode::Extent);
    ui->spinExtent->setVisible(mode == PartGui::PatternMode::Extent);
    ui->formLayout->labelForField(ui->spacingControlsWidget)
        ->setVisible(mode == PartGui::PatternMode::Spacing);
    ui->spacingControlsWidget->setVisible(mode == PartGui::PatternMode::Spacing);
}

const App::PropertyLinkSub& PatternParametersWidget::getCurrentDirectionLink() const
{
    return dirLinks.getCurrentLink();
}

bool PatternParametersWidget::isSelectReferenceMode() const
{
    return !dirLinks.getCurrentLink().getValue();
}

void PatternParametersWidget::setTitle(const QString& title)
{
    ui->groupBox->setTitle(title);
}

void PatternParametersWidget::setCheckable(bool on)
{
    ui->groupBox->setCheckable(on);
}

void PatternParametersWidget::setChecked(bool on)
{
    ui->groupBox->setChecked(on);
    ui->enableCheckbox->setChecked(on);
}

// --- Slots ---

void PatternParametersWidget::onDirectionChanged(int /*index*/)
{
    if (blockUpdate || !m_directionProp) {
        return;
    }

    if (isSelectReferenceMode()) {
        // Emit signal for the task panel to handle reference selection
        requestReferenceSelection();
    }
    else {
        m_directionProp->Paste(dirLinks.getCurrentLink());  // Update the property
        parametersChanged();                                // Notify change
    }
}

void PatternParametersWidget::onReversePressed()
{
    if (blockUpdate || !m_reversedProp) {
        return;
    }

    m_reversedProp->setValue(!m_reversedProp->getValue());
    parametersChanged();
}

void PatternParametersWidget::onModeChanged(int index)
{
    if (blockUpdate || !m_modeProp) {
        return;
    }
    m_modeProp->setValue(index);  // Assuming enum values match index
    adaptVisibilityToMode();      // Update visibility based on new mode
    parametersChanged();
}

void PatternParametersWidget::onLengthChanged(double value)
{
    // Usually handled by bind(). If manual update needed:
    if (blockUpdate || !m_extentProp) {
        return;
    }
    m_extentProp->setValue(value);
    parametersChanged();  // Still emit signal even if bound
}

void PatternParametersWidget::onOffsetChanged(double value)
{
    if (blockUpdate || !m_spacingProp || !m_spacingPatternProp) {
        return;
    }

    m_spacingProp->setValue(value);

    // Crucially, also update the *first* element of the SpacingPattern list
    std::vector<double> currentSpacings = m_spacingPatternProp->getValues();
    if (currentSpacings.empty()) {
        currentSpacings.push_back(ui->spinSpacing->value().getValue());  // Use UI value which
                                                                         // includes units
    }
    else {
        currentSpacings[0] = ui->spinSpacing->value().getValue();
    }

    m_spacingPatternProp->setValues(currentSpacings);  // Update the property list
    parametersChanged();                               // Emit signal
}

void PatternParametersWidget::onOccurrencesChanged(unsigned int value)
{
    // Usually handled by bind(). If manual update needed:
    if (blockUpdate || !m_occurrencesProp) {
        return;
    }

    m_occurrencesProp->setValue(value);
    parametersChanged();  // Still emit signal even if bound
}


// --- Dynamic Spacing Logic ---

void PatternParametersWidget::clearDynamicSpacingRows()
{
    for (QWidget* fieldWidget : dynamicSpacingRows) {
        ui->formLayout->removeRow(fieldWidget);
    }
    dynamicSpacingRows.clear();
    dynamicSpacingSpinBoxes.clear();
}

void PatternParametersWidget::addSpacingRow(double value)
{
    if (!m_spacingProp) {
        return;  // Need context for units
    }

    // Find position to insert before "Occurrences"
    int insertPos = -1;
    QFormLayout::ItemRole role;
    ui->formLayout->getWidgetPosition(ui->spinOccurrences, &insertPos, &role);
    if (insertPos == -1) {
        insertPos = ui->formLayout->rowCount();  // Fallback to appending
    }

    int newIndex = dynamicSpacingRows.count();
    QLabel* label = new QLabel(tr("Spacing %1").arg(newIndex + 2), this);

    // Create the field widget (spinbox + remove button)
    QWidget* fieldWidget = new QWidget(this);
    QHBoxLayout* fieldLayout = new QHBoxLayout(fieldWidget);
    fieldLayout->setContentsMargins(0, 0, 0, 0);
    fieldLayout->setSpacing(3);

    Gui::QuantitySpinBox* spinBox = new Gui::QuantitySpinBox(fieldWidget);
    Base::Unit unit = type == PatternType::Linear ? Base::Unit::Length : Base::Unit::Angle;
    spinBox->setUnit(unit);
    spinBox->setKeyboardTracking(false);
    spinBox->setValue(value);  // Set initial value

    QToolButton* removeButton = new QToolButton(fieldWidget);
    removeButton->setIcon(Gui::BitmapFactory().iconFromTheme("list-remove"));
    removeButton->setToolTip(tr("Remove this spacing definition."));
    removeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    fieldLayout->addWidget(spinBox);
    fieldLayout->addWidget(removeButton);

    ui->formLayout->insertRow(insertPos, label, fieldWidget);
    dynamicSpacingRows.append(fieldWidget);
    dynamicSpacingSpinBoxes.append(spinBox);

    // Connect signals for the new row
    connect(
        spinBox,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &PatternParametersWidget::onDynamicSpacingChanged
    );
    connect(removeButton, &QToolButton::clicked, this, [this, fieldWidget]() {
        this->onRemoveSpacingButtonClicked(fieldWidget);
    });
}

void PatternParametersWidget::rebuildDynamicSpacingUI()
{
    if (!m_spacingPatternProp) {
        return;
    }

    clearDynamicSpacingRows();  // Clear existing dynamic UI first

    std::vector<double> currentSpacings = m_spacingPatternProp->getValues();
    // Start from index 1, as index 0 corresponds to ui->spinSpacing
    for (size_t i = 1; i < currentSpacings.size(); ++i) {
        // Values in PropertyFloatList are unitless. Assume they match the Offset unit.
        addSpacingRow(currentSpacings[i]);
    }
}

void PatternParametersWidget::onAddSpacingButtonClicked()
{
    if (blockUpdate || !m_spacingProp) {
        return;
    }

    // Add a new row to the UI with a default value (same as main offset)
    addSpacingRow(ui->spinSpacing->value().getValue());

    // Update the underlying property list
    updateSpacingPatternProperty();  // This will emit parametersChanged
}

void PatternParametersWidget::onDynamicSpacingChanged()
{
    if (blockUpdate) {
        return;
    }
    // Update the entire property list based on the current UI state.
    updateSpacingPatternProperty();  // This will emit parametersChanged
}

void PatternParametersWidget::onRemoveSpacingButtonClicked(QWidget* fieldWidget)
{
    if (blockUpdate || !m_spacingPatternProp) {
        return;
    }

    int indexToRemove = dynamicSpacingRows.indexOf(fieldWidget);
    if (indexToRemove == -1) {
        return;
    }

    // removeRow also deletes the widgets (label and field)
    ui->formLayout->removeRow(fieldWidget);

    dynamicSpacingRows.removeAt(indexToRemove);
    dynamicSpacingSpinBoxes.removeAt(indexToRemove);

    // Update labels of subsequent rows
    for (int i = indexToRemove; i < dynamicSpacingRows.size(); ++i) {
        if (auto* label = qobject_cast<QLabel*>(ui->formLayout->labelForField(dynamicSpacingRows[i]))) {
            label->setText(tr("Spacing %1").arg(i + 2));
        }
    }

    // Update the underlying property list
    updateSpacingPatternProperty();  // This will emit parametersChanged
}

void PatternParametersWidget::updateSpacingPatternProperty()
{
    if (blockUpdate || !m_spacingPatternProp || !m_spacingProp) {
        return;
    }

    std::vector<double> newSpacings;

    // First element is always the main offset's value
    newSpacings.push_back(ui->spinSpacing->value().getValue());

    // Add values from dynamic spin boxes
    for (Gui::QuantitySpinBox* spinBox : dynamicSpacingSpinBoxes) {
        newSpacings.push_back(spinBox->value().getValue());
    }

    m_spacingPatternProp->setValues(newSpacings);  // Set the property list
    parametersChanged();                           // Emit signal after property is set
}

// --- Getters ---

void PatternParametersWidget::getAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    const App::PropertyLinkSub& lnk = dirLinks.getCurrentLink();
    obj = lnk.getValue();
    sub = lnk.getSubValues();
}

bool PatternParametersWidget::getReverse() const
{
    return m_reversedProp->getValue();
}

int PatternParametersWidget::getMode() const
{
    return ui->comboMode->currentIndex();
}

double PatternParametersWidget::getExtent() const
{
    return ui->spinExtent->value().getValue();
}

double PatternParametersWidget::getSpacing() const
{
    return ui->spinSpacing->value().getValue();
}

unsigned PatternParametersWidget::getOccurrences() const
{
    return ui->spinOccurrences->value();
}

std::string PatternParametersWidget::getSpacingPatternsAsString() const
{
    // Build the Python list string for SpacingPattern
    std::stringstream ss;
    ss << "[";
    const auto& spacingValues = m_spacingPatternProp->getValues();
    for (size_t i = 0; i < spacingValues.size(); ++i) {
        ss << (i > 0 ? ", " : "") << spacingValues[i];
    }
    ss << "]";
    return ss.str();
}

void PatternParametersWidget::applyQuantitySpinboxes() const
{
    ui->spinExtent->apply();
    ui->spinSpacing->apply();
    ui->spinOccurrences->apply();
}

// #include "moc_PatternParametersWidget.cpp"
