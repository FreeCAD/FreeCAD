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


#pragma once

#include <memory>
#include <vector>
#include <App/PropertyStandard.h>  // For Property types
#include <App/PropertyLinks.h>     // For PropertyLinkSub
#include <Gui/EditableDatumLabel.h>

#include <Mod/Part/PartGlobal.h>

#include "PatternReferenceWidget.h"

class Ui_PatternParametersWidget;

namespace App
{
class PropertyLinkSub;
class PropertyBool;
class PropertyEnumeration;
class PropertyQuantity;
class PropertyFloatList;
class PropertyIntegerConstraint;
class DocumentObject;
}  // namespace App
namespace Gui
{
class QuantitySpinBox;
class View3DInventorViewer;
}  // namespace Gui
class QToolButton;

namespace PartGui
{

enum class PatternType
{
    Linear,
    Polar
};

enum class PatternMode
{
    Extent,
    Spacing
};

/**
 * @brief A widget to configure the parameters for a single direction of a linear pattern.
 *
 * This widget encapsulates the UI and logic for Direction, Reverse, Mode,
 * Length/Spacing (including dynamic spacing), and Occurrences. It binds directly
 * to the corresponding properties of a Feature.
 */
class PartGuiExport PatternParametersWidget: public PatternReferenceWidget
{
    Q_OBJECT

public:
    explicit PatternParametersWidget(
        PatternType type,
        Gui::View3DInventorViewer* v,
        QWidget* parent = nullptr
    );
    ~PatternParametersWidget() override;

    /**
     * @brief Binds the widget's UI elements to the properties of a DocumentObject.
     *
     * This must be called after creating the widget to link its controls
     * to the underlying feature's data.
     *
     * @param directionProp Reference to the Direction property (PropertyLinkSub).
     * @param reversedProp Reference to the Reversed property (PropertyBool).
     * @param modeProp Reference to the Mode property (PropertyEnumeration).
     * @param lengthProp Reference to the Length property (PropertyQuantity).
     * @param offsetProp Reference to the Offset property (PropertyQuantity).
     * @param spacingPatternProp Reference to the SpacingPattern property (PropertyFloatList).
     * @param occurrencesProp Reference to the Occurrences property (PropertyIntegerConstraint).
     * @param feature The feature object itself, needed for context (e.g., units).
     */
    void bindProperties(
        App::PropertyLinkSub* directionProp,
        App::PropertyBool* reversedProp,
        App::PropertyEnumeration* modeProp,
        App::PropertyQuantity* lengthProp,
        App::PropertyQuantity* offsetProp,
        App::PropertyFloatList* spacingsOverridesProp,
        App::PropertyFloatList* spacingPatternProp,
        App::PropertyIntegerConstraint* occurrencesProp,
        App::DocumentObject* feature
    );  // Pass feature for context


    /**
     * @brief Adds a custom direction option to the Direction combo box.
     *
     * Used by consuming tools (like PartDesign tasks) to add context-specific
     * directions (e.g., Sketch axes).
     *
     * @param link The PropertyLinkSub representing the custom direction.
     * @param text The user-visible text for the combo box item.
     */
    /**
     * @brief Updates the UI elements to reflect the current values of the bound properties.
     */
    void updateUI();

    /**
     * @brief Returns the currently selected direction link from the combo box.
     * Returns an empty link if "Select reference..." is chosen.
     */
    bool getReverse() const;
    int getMode() const;
    double getExtent() const;
    double getSpacing() const;
    unsigned getOccurrences() const;
    std::string getSpacingPatternsAsString() const;

    void setTitle(const QString& title);
    void setCheckable(bool on);
    void setChecked(bool on);

    void applyQuantitySpinboxes() const;

    // Methods for managing on-view labels
    void updateSpacingLabels(
        const Base::Vector3d& center,
        const Base::Vector3d& axis,
        double radius,
        double startAngle
    );
    void updateSpacingLabels(const Base::Vector3d& startPoint, const Base::Vector3d& direction);
    void clearAllSpacingLabels();

private Q_SLOTS:
    // Slots connected to UI elements
    void onReversePressed();
    void onModeChanged(int index);
    // Note: Spinbox value changes are often handled by direct binding,
    // but we might need slots if extra logic (like updating SpacingPattern[0]) is needed.
    void onLengthChanged(double value);
    void onOffsetChanged(double value);
    void onOccurrencesChanged(unsigned int value);

    void onGroupBoxToggled(bool checked);
    void onEnableCheckBoxToggled(bool checked);

    // Slots for dynamic spacing
    void onAddSpacingButtonClicked();
    void onDynamicSpacingChanged();  // Simplified slot
    void onRemoveSpacingButtonClicked(QWidget* rowWidget);

    // Slots for on-view label interaction
    void onSpacingLabelClicked(Gui::EditableDatumLabel* label);
    void onSpacingLabelRightClicked(Gui::EditableDatumLabel* label, const QPoint& globalPos);

private:
    // Initialization and setup
    void setupUiElements();
    void connectSignals();

    // UI Update and state management
    void adaptVisibilityToMode();

    // Dynamic spacing helpers
    void addSpacingRow(double value);
    void clearDynamicSpacingRows();
    void rebuildDynamicSpacingUI();
    void updateSpacingPatternProperty();  // Updates the property from the UI

    std::unique_ptr<Ui_PatternParametersWidget> ui;

    // Pointers to bound properties (raw pointers, lifetime managed externally)
    App::PropertyBool* m_reversedProp = nullptr;
    App::PropertyEnumeration* m_modeProp = nullptr;
    App::PropertyQuantity* m_extentProp = nullptr;
    App::PropertyQuantity* m_spacingProp = nullptr;
    App::PropertyFloatList* m_spacingsOverrideProp = nullptr;
    App::PropertyFloatList* m_spacingPatternProp = nullptr;
    App::PropertyIntegerConstraint* m_occurrencesProp = nullptr;
    App::DocumentObject* m_feature = nullptr;  // Store feature for context

    bool blockUpdate = false;  // Prevents signal loops

    // Store pointers to dynamically created widgets for removal and access
    QList<QWidget*> dynamicSpacingRows;
    QList<Gui::QuantitySpinBox*> dynamicSpacingSpinBoxes;

    // Members for on-view labels
    Gui::View3DInventorViewer* viewer = nullptr;
    std::vector<std::unique_ptr<Gui::EditableDatumLabel>> spacingLabels;

    PatternType type;
};

}  // namespace PartGui
