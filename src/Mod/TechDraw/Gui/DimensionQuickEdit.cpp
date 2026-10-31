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

#include "PreCompiled.h"

#include <regex>

#include <QApplication>
#include <QActionGroup>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QScreen>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <App/Document.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Gui/Command.h>
#include <Gui/Control.h>

#include <Mod/TechDraw/App/DimensionFormatter.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/Gui/QGIViewDimension.h>
#include <Mod/TechDraw/Gui/TaskDimension.h>
#include <Mod/TechDraw/Gui/ViewProviderDimension.h>

#include "DimensionQuickEdit.h"

using namespace TechDrawGui;
using DrawViewDimension = TechDraw::DrawViewDimension;
using Format = TechDraw::DimensionFormatter::Format;

namespace
{
constexpr int PopupVerticalOffset = 44;  // px below the click point

// size and spacing
constexpr int PopupMarginLeftRight = 4;
constexpr int PopupMarginTop = 4;
constexpr int PopupMarginBottom = 4;
constexpr int FieldSpacing = 4;
constexpr int IconRowSpacing = 3;
constexpr int PrefixSuffixWidth = 52;
constexpr int ValueFieldWidth = 70;
constexpr int DecimalsButtonWidth = 18;
constexpr int ToolButtonHeight = 26;
constexpr int ToolButtonWidth = 26;
constexpr int SignToggleButtonWidth = 40;
constexpr int MoreButtonWidth = 52;
constexpr int SymbolComboWidth = 44;

constexpr int DefaultSymbolIndex = 1;  // Diameter
constexpr int ToleranceSpinBoxWidth = 70;

// others
constexpr double ToleranceSpinBoxMax = 1000.0;
constexpr double ToleranceSingleStep = 0.01;
constexpr int ToleranceDecimalPlaces = 3;
constexpr int MaxDecimalPlaces = 6;
constexpr double DefaultToleranceValue = 0.01;
constexpr int LiveCommitDebounceMs = 250;

const std::regex FormatSpecRegex(R"(%\.([0-9]+)([fFrRgGwWeE]))");

struct SymbolEntry
{
    QString tooltip;
    QString symbol;
};

// Only COMMON symbols that can't be typed
QList<SymbolEntry> buildSymbolList()
{
    QString diameterSymbol = QString::fromStdString(
        TechDraw::Preferences::getPreferenceGroup("Dimensions")->GetASCII("DiameterSymbol", "\xe2\x8c\x80")
    );

    return {
        {.tooltip = QObject::tr("Degree"), .symbol = QStringLiteral("\u00B0")},
        {.tooltip = QObject::tr("Diameter"), .symbol = diameterSymbol},
        {.tooltip = QObject::tr("Counterbore"), .symbol = QStringLiteral("\u2334")},
        {.tooltip = QObject::tr("Countersink"), .symbol = QStringLiteral("\u2335")},
        {.tooltip = QObject::tr("Downward arrow"), .symbol = QStringLiteral("\u21A7")},
        {.tooltip = QObject::tr("Square"), .symbol = QStringLiteral("\u25A1")},
        {.tooltip = QObject::tr("Plus/minus"), .symbol = QStringLiteral("\u00B1")},
    };
}
}  // namespace

void DimensionQuickEdit::showFor(ViewProviderDimension* dimensionVP, const QPoint& globalPos)
{
    if (!dimensionVP) {
        return;
    }

    auto* popup = new DimensionQuickEdit(dimensionVP);
    QPoint topLeft(globalPos.x() - (popup->sizeHint().width() / 2), globalPos.y() + PopupVerticalOffset);
    popup->move(popup->clampToScreen(topLeft));
    popup->show();
    popup->setFocus(Qt::PopupFocusReason);
}

DimensionQuickEdit::DimensionQuickEdit(ViewProviderDimension* dimensionVP, QWidget* parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)
    , m_dimensionVP(dimensionVP)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("DimensionQuickEdit"));

    buildUi();
    readFromFeature();

    // one undo step for the whole popup session
    if (auto* doc = m_dimensionVP->getDocument()) {
        doc->openCommand("Edit dimension");
    }
}

DimensionQuickEdit::~DimensionQuickEdit() = default;

QPoint DimensionQuickEdit::clampToScreen(QPoint topLeft) const
{
    const QRect avail = QApplication::screenAt(topLeft)
        ? QApplication::screenAt(topLeft)->availableGeometry()
        : QApplication::primaryScreen()->availableGeometry();
    const QSize hint = sizeHint();

    int x = std::clamp(topLeft.x(), avail.left(), avail.right() - hint.width());
    int y = std::clamp(topLeft.y(), avail.top(), avail.bottom() - hint.height());
    return {x, y};
}

void DimensionQuickEdit::buildUi()
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(PopupMarginLeftRight, PopupMarginTop, PopupMarginLeftRight, PopupMarginBottom);
    outer->setSpacing(FieldSpacing);
    outer->setSizeConstraint(QLayout::SetFixedSize);

    auto* fields = new QGridLayout();
    fields->setSpacing(FieldSpacing);

    m_prefixEdit = new QLineEdit(this);
    m_prefixEdit->setPlaceholderText(tr("prefix"));
    m_prefixEdit->setFixedWidth(PrefixSuffixWidth);

    m_valueEdit = new QLineEdit(this);
    m_valueEdit->setReadOnly(true);
    m_valueEdit->setFocusPolicy(Qt::NoFocus);  // functions as a display so we dont want editing or focus
    m_valueEdit->setAlignment(Qt::AlignCenter);
    m_valueEdit->setFixedWidth(ValueFieldWidth);
    m_valueEdit->setToolTip(
        tr("The computed value, shown live as the prefix/"
           "suffix/tolerance is edited. Use the arrow for decimal places.")
    );

    m_decimalsBtn = new QToolButton(this);
    m_decimalsBtn->setText(QStringLiteral("\u25BE"));  // black down-pointing small triangle
    m_decimalsBtn->setToolTip(tr("Choose decimal places"));
    m_decimalsBtn->setFixedSize(DecimalsButtonWidth, ToolButtonHeight);
    auto* valueWithDecimals = new QHBoxLayout();
    valueWithDecimals->setSpacing(0);
    valueWithDecimals->addWidget(m_valueEdit);
    valueWithDecimals->addWidget(m_decimalsBtn);

    m_toleranceMode = new QComboBox(this);
    m_toleranceMode->addItem(tr("No tolerance"), static_cast<int>(ToleranceMode::None));
    m_toleranceMode->addItem(tr("Symmetric (\u00B1)"), static_cast<int>(ToleranceMode::Symmetric));
    m_toleranceMode->addItem(tr("Bilateral (+/-)"), static_cast<int>(ToleranceMode::Bilateral));

    m_toleranceOver = new QDoubleSpinBox(this);
    m_toleranceOver->setRange(0.0, ToleranceSpinBoxMax);
    m_toleranceOver->setDecimals(3);
    m_toleranceOver->setSingleStep(ToleranceSingleStep);
    m_toleranceOver->setFixedWidth(ToleranceSpinBoxWidth);

    m_toleranceUnder = new QDoubleSpinBox(this);
    m_toleranceUnder->setRange(0.0, ToleranceSpinBoxMax);
    m_toleranceUnder->setDecimals(3);
    m_toleranceUnder->setSingleStep(ToleranceSingleStep);
    m_toleranceUnder->setFixedWidth(ToleranceSpinBoxWidth);

    m_suffixEdit = new QLineEdit(this);
    m_suffixEdit->setPlaceholderText(tr("suffix"));
    m_suffixEdit->setFixedWidth(PrefixSuffixWidth);

    // Track cursor focus across the two editable text fields so the
    // symbol combo knows where to insert.
    m_prefixEdit->installEventFilter(this);
    m_suffixEdit->installEventFilter(this);
    m_lastFocusedField = m_prefixEdit;  // default target for inserting symbol before any click

    fields->addWidget(m_prefixEdit, 0, 0);
    fields->addLayout(valueWithDecimals, 0, 1);
    fields->addWidget(m_toleranceMode, 0, 2);
    fields->addWidget(m_toleranceOver, 0, 3);
    fields->addWidget(m_toleranceUnder, 0, 4);
    fields->addWidget(m_suffixEdit, 0, 5);
    outer->addLayout(fields);

    auto makeToolButton =
        [this](const QString& text, const QString& tip, bool checkable, int width = ToolButtonWidth) {
            auto* btn = new QToolButton(this);
            btn->setText(text);
            btn->setToolTip(tip);
            btn->setCheckable(checkable);
            btn->setFixedSize(width, ToolButtonHeight);
            return btn;
        };

    auto* icons = new QHBoxLayout();
    icons->setSpacing(IconRowSpacing);

    m_symbolCombo = new QComboBox(this);
    m_symbolCombo->setToolTip(tr("Insert a symbol at the cursor in the focused field"));
    m_symbolCombo->setFixedWidth(SymbolComboWidth);
    for (const SymbolEntry& entry : buildSymbolList()) {
        m_symbolCombo->addItem(entry.symbol);
        m_symbolCombo->setItemData(m_symbolCombo->count() - 1, entry.tooltip, Qt::ToolTipRole);
    }
    m_symbolCombo->setCurrentIndex(DefaultSymbolIndex);

    m_referenceBtn = makeToolButton(
        QStringLiteral("(x)"),
        tr("Converts the dimension to a reference dimension, wrapping the value in parentheses."),
        true,
        SignToggleButtonWidth
    );
    m_basicBtn = makeToolButton(
        QStringLiteral("[x]"),
        tr("Converts the dimension to a basic (theoretically exact) dimension, wrapping the value "
           "in square brackets. Enabling this clears any tolerances."),
        true,
        SignToggleButtonWidth
    );
    QFont basicFont = m_basicBtn->font();
    basicFont.setPointSize(basicFont.pointSize() + 3);
    m_basicBtn->setFont(basicFont);
    QFont referenceFont = m_referenceBtn->font();
    referenceFont.setPointSize(referenceFont.pointSize() + 3);
    m_referenceBtn->setFont(referenceFont);
    m_moreBtn = makeToolButton(
        tr("More…"),
        tr("Opens the dimension task panel for more configuration options."),
        false,
        MoreButtonWidth
    );

    icons->addWidget(m_symbolCombo);
    icons->addWidget(m_referenceBtn);
    icons->addWidget(m_basicBtn);
    icons->addStretch();
    icons->addWidget(m_moreBtn);
    outer->addLayout(icons);

    connect(m_prefixEdit, &QLineEdit::editingFinished, this, &DimensionQuickEdit::onPrefixOrSuffixChanged);
    connect(m_suffixEdit, &QLineEdit::editingFinished, this, &DimensionQuickEdit::onPrefixOrSuffixChanged);
    m_liveCommitTimer = new QTimer(this);
    m_liveCommitTimer->setSingleShot(true);
    m_liveCommitTimer->setInterval(LiveCommitDebounceMs);
    connect(m_liveCommitTimer, &QTimer::timeout, this, &DimensionQuickEdit::onPrefixOrSuffixChanged);
    connect(m_prefixEdit, &QLineEdit::textChanged, this, &DimensionQuickEdit::onPrefixOrSuffixLiveUpdate);
    connect(m_suffixEdit, &QLineEdit::textChanged, this, &DimensionQuickEdit::onPrefixOrSuffixLiveUpdate);
    connect(m_toleranceMode, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        onToleranceModeChanged(static_cast<ToleranceMode>(index));
    });
    connect(m_toleranceOver, &QDoubleSpinBox::editingFinished, this, [this]() {
        onToleranceValueChanged();
    });
    connect(m_toleranceUnder, &QDoubleSpinBox::editingFinished, this, [this]() {
        onToleranceValueChanged();
    });
    m_toleranceLiveCommitTimer = new QTimer(this);
    m_toleranceLiveCommitTimer->setSingleShot(true);
    m_toleranceLiveCommitTimer->setInterval(LiveCommitDebounceMs);
    connect(m_toleranceLiveCommitTimer, &QTimer::timeout, this, [this]() {
        onToleranceValueChanged();
    });
    connect(m_toleranceOver, &QDoubleSpinBox::valueChanged, this, [this](double) {
        if (!m_populating) {
            m_toleranceLiveCommitTimer->start();
        }
    });
    connect(m_toleranceUnder, &QDoubleSpinBox::valueChanged, this, [this](double) {
        if (!m_populating) {
            m_toleranceLiveCommitTimer->start();
        }
    });
    connect(m_decimalsBtn, &QToolButton::clicked, this, &DimensionQuickEdit::showDecimalsMenu);
    connect(
        m_symbolCombo,
        QOverload<int>::of(&QComboBox::activated),
        this,
        &DimensionQuickEdit::onSymbolChanged
    );
    connect(m_referenceBtn, &QToolButton::clicked, this, &DimensionQuickEdit::onToggleReference);
    connect(m_basicBtn, &QToolButton::clicked, this, &DimensionQuickEdit::onToggleBasic);
    connect(m_moreBtn, &QToolButton::clicked, this, &DimensionQuickEdit::onMoreOptions);
}

void DimensionQuickEdit::readFromFeature()
{
    if (!m_dimensionVP) {
        return;
    }
    auto* dim = freecad_cast<DrawViewDimension*>(m_dimensionVP->getObject());
    if (!dim) {
        return;
    }

    m_populating = true;

    std::string currentFormat = dim->FormatSpec.getStrValue();

    QStringList prefixSuffix = dim->getPrefixSuffixSpec(QString::fromStdString(currentFormat));
    m_formatPrefix = prefixSuffix.value(0).toStdString();
    m_formatSuffix = prefixSuffix.value(1).toStdString();

    m_referenceActive = !m_formatPrefix.empty() && m_formatPrefix.back() == '('
        && !m_formatSuffix.empty() && m_formatSuffix.front() == ')';
    if (m_referenceActive) {
        m_formatPrefix.pop_back();
        m_formatSuffix.erase(0, 1);
    }

    std::smatch match;
    int decimals = 2;
    if (std::regex_search(currentFormat, match, FormatSpecRegex) && match.size() > 2) {
        decimals = std::stoi(match[1].str());
        m_formatChar = match[2].str();
    }
    m_prefixEdit->setText(QString::fromStdString(m_formatPrefix));
    m_suffixEdit->setText(QString::fromStdString(m_formatSuffix));
    m_decimals = std::clamp(decimals, 0, MaxDecimalPlaces);
    m_referenceBtn->setChecked(m_referenceActive);
    m_symbolCombo->setCurrentIndex(DefaultSymbolIndex);
    updateValuePreview();

    bool equalTol = dim->EqualTolerance.getValue();
    double over = dim->OverTolerance.getValue();
    double under = dim->UnderTolerance.getValue();
    if (over == 0.0 && under == 0.0) {
        m_toleranceMode->setCurrentIndex(static_cast<int>(ToleranceMode::None));
    }
    else if (equalTol) {
        m_toleranceMode->setCurrentIndex(static_cast<int>(ToleranceMode::Symmetric));
    }
    else {
        m_toleranceMode->setCurrentIndex(static_cast<int>(ToleranceMode::Bilateral));
    }
    m_toleranceOver->setValue(over);
    m_toleranceUnder->setValue(under);
    const auto currentMode = static_cast<ToleranceMode>(m_toleranceMode->currentIndex());
    m_toleranceUnder->setEnabled(currentMode == ToleranceMode::Bilateral);
    m_toleranceOver->setVisible(currentMode != ToleranceMode::None);
    m_toleranceUnder->setVisible(currentMode == ToleranceMode::Bilateral);
    updateTolerancePrefixes(currentMode);

    m_basicBtn->setChecked(dim->TheoreticalExact.getValue());

    m_populating = false;
}

void DimensionQuickEdit::updateValuePreview()
{
    if (!m_dimensionVP) {
        return;
    }
    auto* dim = freecad_cast<DrawViewDimension*>(m_dimensionVP->getObject());
    if (!dim) {
        return;
    }
    QString bareSpec = QString::fromStdString("%." + std::to_string(m_decimals) + m_formatChar);
    m_valueEdit->setText(
        QString::fromStdString(dim->formatValue(dim->getDimValue(), bareSpec, Format::FORMATTED, true)));
}

void DimensionQuickEdit::onPrefixOrSuffixLiveUpdate()
{
    if (m_populating) {
        return;
    }
    m_liveCommitTimer->start();
}

void DimensionQuickEdit::rebuildFormatSpec()
{
    if (!m_dimensionVP || m_populating) {
        return;
    }
    auto* dim = freecad_cast<DrawViewDimension*>(m_dimensionVP->getObject());
    if (!dim) {
        return;
    }

    if (dim->Arbitrary.getValue()) {
        dim->Arbitrary.setValue(false);
    }

    std::string valuePattern = "%." + std::to_string(m_decimals) + m_formatChar;
    if (m_referenceActive) {
        valuePattern = "(" + valuePattern + ")";
    }
    std::string spec = m_formatPrefix + valuePattern + m_formatSuffix;
    dim->FormatSpec.setValue(spec);
    dim->recomputeFeature();
    markDirty();
    updateValuePreview();
}

void DimensionQuickEdit::syncToleranceFormatSpecs()
{
    if (!m_dimensionVP) {
        return;
    }
    auto* dim = freecad_cast<DrawViewDimension*>(m_dimensionVP->getObject());
    if (!dim) {
        return;
    }
    QString spec = QStringLiteral("%+.") + QString::number(ToleranceDecimalPlaces) + QStringLiteral("f");
    dim->FormatSpecOverTolerance.setValue(spec.toStdString());
    dim->FormatSpecUnderTolerance.setValue(spec.toStdString());
}

void DimensionQuickEdit::onToggleReference()
{
    m_referenceActive = m_referenceBtn->isChecked();
    rebuildFormatSpec();
}

void DimensionQuickEdit::onPrefixOrSuffixChanged()
{
    if (m_populating) {
        return;
    }
    m_liveCommitTimer->stop();
    m_formatPrefix = m_prefixEdit->text().toStdString();
    m_formatSuffix = m_suffixEdit->text().toStdString();
    rebuildFormatSpec();
}

void DimensionQuickEdit::showDecimalsMenu()
{
    if (!m_dimensionVP) {
        return;
    }

    static const QString DemoDigits = QStringLiteral("123456");

    QMenu menu(this);
    auto* group = new QActionGroup(&menu);
    group->setExclusive(true);
    for (int decimals = 0; decimals <= MaxDecimalPlaces; ++decimals) {
        QString label = decimals == 0 ? QStringLiteral("0")
                                      : QStringLiteral("0.") + DemoDigits.left(decimals);
        QAction* action = menu.addAction(label);
        action->setCheckable(true);
        action->setChecked(decimals == m_decimals);
        group->addAction(action);
        connect(action, &QAction::triggered, this, [this, decimals]() {
            m_decimals = decimals;
            rebuildFormatSpec();
            if (static_cast<ToleranceMode>(m_toleranceMode->currentIndex()) != ToleranceMode::None) {
                syncToleranceFormatSpecs();
                if (auto* dim = freecad_cast<DrawViewDimension*>(m_dimensionVP->getObject())) {
                    dim->recomputeFeature();
                    updateValuePreview();
                }
            }
        });
    }
    menu.exec(m_decimalsBtn->mapToGlobal(QPoint(0, m_decimalsBtn->height())));
}

void DimensionQuickEdit::updateTolerancePrefixes(ToleranceMode mode)
{
    if (mode == ToleranceMode::Symmetric) {
        m_toleranceOver->setPrefix(QStringLiteral("\u00B1 "));
    }
    else if (mode == ToleranceMode::Bilateral) {
        m_toleranceOver->setPrefix(QStringLiteral("+ "));
        m_toleranceUnder->setPrefix(QStringLiteral("\u2212 "));
    }
    else {
        m_toleranceOver->setPrefix(QString());
    }
}

void DimensionQuickEdit::onToleranceModeChanged(ToleranceMode mode)
{
    m_toleranceOver->setVisible(mode != ToleranceMode::None);
    m_toleranceUnder->setVisible(mode == ToleranceMode::Bilateral);
    m_toleranceUnder->setEnabled(mode == ToleranceMode::Bilateral);
    updateTolerancePrefixes(mode);

    if (m_populating || !m_dimensionVP) {
        return;
    }
    auto* dim = freecad_cast<DrawViewDimension*>(m_dimensionVP->getObject());
    if (!dim) {
        return;
    }

    if (mode == ToleranceMode::None) {
        dim->OverTolerance.setValue(0.0);
        dim->UnderTolerance.setValue(0.0);
    }
    else {
        dim->EqualTolerance.setValue(mode == ToleranceMode::Symmetric);
        if (dim->OverTolerance.getValue() == 0.0) {
            dim->OverTolerance.setValue(DefaultToleranceValue);
            m_toleranceOver->setValue(DefaultToleranceValue);
        }
        if (mode == ToleranceMode::Bilateral && dim->UnderTolerance.getValue() == 0.0) {
            dim->UnderTolerance.setValue(DefaultToleranceValue);
            m_toleranceUnder->setValue(DefaultToleranceValue);
        }
        syncToleranceFormatSpecs();
    }
    m_toleranceLiveCommitTimer->stop();
    dim->recomputeFeature();
    markDirty();
    updateValuePreview();
}

void DimensionQuickEdit::onToleranceValueChanged()
{
    if (m_populating || !m_dimensionVP) {
        return;
    }
    m_toleranceLiveCommitTimer->stop();
    auto* dim = freecad_cast<DrawViewDimension*>(m_dimensionVP->getObject());
    if (!dim) {
        return;
    }

    dim->OverTolerance.setValue(m_toleranceOver->value());
    if (dim->EqualTolerance.getValue()) {
        dim->UnderTolerance.setValue(m_toleranceOver->value());
    }
    else {
        dim->UnderTolerance.setValue(m_toleranceUnder->value());
    }
    syncToleranceFormatSpecs();
    dim->recomputeFeature();
    markDirty();
    updateValuePreview();
}

void DimensionQuickEdit::onSymbolChanged(int index)
{
    if (m_populating) {
        return;
    }
    QList<SymbolEntry> symbols = buildSymbolList();
    if (index < 0 || index >= symbols.size()) {
        return;
    }
    QLineEdit* target = m_lastFocusedField ? m_lastFocusedField : m_prefixEdit;

    target->insert(symbols[index].symbol);
    onPrefixOrSuffixChanged();

    m_symbolCombo->setCurrentIndex(DefaultSymbolIndex);
    target->setFocus();
}

void DimensionQuickEdit::onToggleBasic()
{
    if (!m_dimensionVP) {
        return;
    }
    auto* dim = freecad_cast<DrawViewDimension*>(m_dimensionVP->getObject());
    if (!dim) {
        return;
    }
    bool checked = m_basicBtn->isChecked();
    dim->TheoreticalExact.setValue(checked);

    // theoretically exact dimensions don't carry a tolerance
    if (checked
        && static_cast<ToleranceMode>(m_toleranceMode->currentIndex()) != ToleranceMode::None) {
        m_toleranceMode->setCurrentIndex(static_cast<int>(ToleranceMode::None));
        onToleranceModeChanged(ToleranceMode::None);
    }
    else {
        dim->recomputeFeature();
        markDirty();
    }
    updateValuePreview();
}

void DimensionQuickEdit::onMoreOptions()
{
    ViewProviderDimension* vp = m_dimensionVP;
    close();
    if (!vp) {
        return;
    }
    if (Gui::Control().activeDialog()) {
        return;
    }
    auto* qgivDimension = dynamic_cast<QGIViewDimension*>(vp->getQView());
    if (qgivDimension) {
        Gui::Control().showDialog(new TaskDlgDimension(qgivDimension, vp));
    }
}

void DimensionQuickEdit::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
}

bool DimensionQuickEdit::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::FocusIn && (watched == m_prefixEdit || watched == m_suffixEdit)) {
        m_lastFocusedField = qobject_cast<QLineEdit*>(watched);
    }
    return QWidget::eventFilter(watched, event);
}

void DimensionQuickEdit::hideEvent(QHideEvent* event)
{
    if (m_dimensionVP) {
        if (auto* doc = m_dimensionVP->getDocument()) {
            if (m_dirty) {
                doc->commitCommand();
            }
            else {
                doc->abortCommand();
            }
        }
    }
    QWidget::hideEvent(event);
}

void DimensionQuickEdit::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Return
        || event->key() == Qt::Key_Enter) {
        close();
        return;
    }
    QWidget::keyPressEvent(event);
}
