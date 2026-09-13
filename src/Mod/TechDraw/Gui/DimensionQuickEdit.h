// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Ryan Kembrey
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <cstdint>
#include <QWidget>

class QLineEdit;
class QComboBox;
class QDoubleSpinBox;
class QToolButton;
class QTimer;

namespace TechDrawGui
{

enum class ToleranceMode : std::uint8_t { None, Symmetric, Bilateral };

class ViewProviderDimension;

class DimensionQuickEdit: public QWidget
{
    Q_OBJECT

public:
    static void showFor(ViewProviderDimension* dimensionVP, const QPoint& globalPos);

    ~DimensionQuickEdit() override;

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    explicit DimensionQuickEdit(ViewProviderDimension* dimensionVP, QWidget* parent = nullptr);

    void buildUi();
    void readFromFeature();
    QPoint clampToScreen(QPoint topLeft) const;

    void onPrefixOrSuffixChanged();
    void onPrefixOrSuffixLiveUpdate();
    void onToleranceModeChanged(ToleranceMode mode);
    void onToleranceValueChanged();
    void showDecimalsMenu();
    void onSymbolChanged(int index);
    void onToggleReference();
    void onToggleBasic();
    void onMoreOptions();

    void rebuildFormatSpec();
    void syncToleranceFormatSpecs();
    void updateValuePreview();
    void updateTolerancePrefixes(ToleranceMode mode);
    void markDirty()
    {
        m_dirty = true;
    }

    ViewProviderDimension* m_dimensionVP = nullptr;
    bool m_dirty = false;
    bool m_populating = false;

    QLineEdit* m_prefixEdit = nullptr;
    QLineEdit* m_valueEdit = nullptr;
    QToolButton* m_decimalsBtn = nullptr;
    int m_decimals = 2;
    QTimer* m_liveCommitTimer = nullptr;
    QTimer* m_toleranceLiveCommitTimer = nullptr;
    QComboBox* m_toleranceMode = nullptr;
    QDoubleSpinBox* m_toleranceOver = nullptr;
    QDoubleSpinBox* m_toleranceUnder = nullptr;
    QLineEdit* m_suffixEdit = nullptr;

    QLineEdit* m_lastFocusedField = nullptr;

    QComboBox* m_symbolCombo = nullptr;
    QToolButton* m_referenceBtn = nullptr;
    QToolButton* m_basicBtn = nullptr;
    QToolButton* m_moreBtn = nullptr;

    std::string m_formatPrefix;
    std::string m_formatSuffix;
    std::string m_formatChar = "w";
    bool m_referenceActive = false;
};

}  // namespace TechDrawGui
