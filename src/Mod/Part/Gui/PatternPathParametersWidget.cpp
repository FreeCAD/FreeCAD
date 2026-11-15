// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PatternPathParametersWidget.h"

#include <QComboBox>
#include <QPushButton>
#include <QSignalBlocker>

#include <App/DocumentObject.h>
#include <Base/Tools.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/SpinBox.h>
#include <Mod/Part/App/PathPatternExtension.h>

#include "ui_PatternPathParametersWidget.h"

using namespace PartGui;

PatternPathParametersWidget::PatternPathParametersWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_PatternPathParametersWidget)
    , pathWidget(new PatternReferenceWidget(this))
    , tangentAxisWidget(new PatternReferenceWidget(this))
    , verticalAxisWidget(new PatternReferenceWidget(this))
{
    ui->setupUi(this);
    pathWidget->setSelectionIndex(PATH_INDEX);
    tangentAxisWidget->setupReferenceCombo(ui->tangentAxis);
    tangentAxisWidget->setSelectionIndex(TANGENT_AXIS_INDEX);
    verticalAxisWidget->setupReferenceCombo(ui->verticalAxis);
    verticalAxisWidget->setSelectionIndex(VERTICAL_AXIS_INDEX);

    connect(ui->pathButton, &QPushButton::clicked, this, [this]() {
        Q_EMIT pathWidget->requestReferenceSelection();
    });

    connect(ui->count, &Gui::UIntSpinBox::unsignedChanged, this, [this](unsigned value) {
        if (!blockUpdate && countProperty) {
            countProperty->setValue(value);
            Q_EMIT pathWidget->parametersChanged();
        }
    });
    connect(ui->spacingMode, qOverload<int>(&QComboBox::activated), this, [this](int value) {
        if (!blockUpdate && spacingModeProperty) {
            spacingModeProperty->setValue(value);
            updateVisibility();
            Q_EMIT pathWidget->parametersChanged();
        }
    });
    const auto quantityChanged = [this](App::PropertyLength* property, double value) {
        if (!blockUpdate && property) {
            property->setValue(value);
            Q_EMIT pathWidget->parametersChanged();
        }
    };
    connect(
        ui->spacing,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        [this, quantityChanged](double value) { quantityChanged(spacingProperty, value); }
    );
    connect(
        ui->startOffset,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        [this, quantityChanged](double value) { quantityChanged(startOffsetProperty, value); }
    );
    connect(
        ui->endOffset,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        [this, quantityChanged](double value) { quantityChanged(endOffsetProperty, value); }
    );
    connect(
        ui->cyclicOffset,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        [this](double value) {
            if (!blockUpdate && cyclicOffsetProperty) {
                cyclicOffsetProperty->setValue(value);
                Q_EMIT pathWidget->parametersChanged();
            }
        }
    );
    connect(ui->reversePath, &QCheckBox::toggled, this, [this](bool value) {
        if (!blockUpdate && reversePathProperty) {
            reversePathProperty->setValue(value);
            Q_EMIT pathWidget->parametersChanged();
        }
    });
    connect(ui->align, &QCheckBox::toggled, this, [this](bool value) {
        if (!blockUpdate && alignProperty) {
            updateVisibility();
            alignProperty->setValue(value);
            Q_EMIT pathWidget->parametersChanged();
        }
    });
    connect(ui->flipVerticalAxis, &QCheckBox::toggled, this, [this](bool value) {
        if (!blockUpdate && flipVerticalAxisProperty) {
            flipVerticalAxisProperty->setValue(value);
            Q_EMIT pathWidget->parametersChanged();
        }
    });
    connect(ui->flipTangentAxis, &QCheckBox::toggled, this, [this](bool value) {
        if (!blockUpdate && flipTangentAxisProperty) {
            flipTangentAxisProperty->setValue(value);
            Q_EMIT pathWidget->parametersChanged();
        }
    });
    connect(ui->originPointIndex, &QSpinBox::valueChanged, this, [this](int value) {
        if (!blockUpdate && originPointIndexProperty) {
            originPointIndexProperty->setValue(value);
            Q_EMIT pathWidget->parametersChanged();
        }
    });
    connect(ui->relativeToFirst, &QCheckBox::toggled, this, [this](bool value) {
        if (!blockUpdate && relativeToFirstProperty) {
            relativeToFirstProperty->setValue(value);
            Q_EMIT pathWidget->parametersChanged();
        }
    });
}

PatternPathParametersWidget::~PatternPathParametersWidget() = default;

void PatternPathParametersWidget::bindProperties(
    App::PropertyLinkSub* path,
    App::PropertyIntegerConstraint* count,
    App::PropertyEnumeration* spacingMode,
    App::PropertyLength* spacing,
    App::PropertyLength* startOffset,
    App::PropertyLength* endOffset,
    App::PropertyDistance* cyclicOffset,
    App::PropertyBool* reversePath,
    App::PropertyBool* align,
    App::PropertyLinkSub* tangentAxis,
    App::PropertyBool* flipTangentAxis,
    App::PropertyLinkSub* verticalAxis,
    App::PropertyBool* flipVerticalAxis,
    App::PropertyInteger* originPointIndex,
    App::PropertyBool* relativeToFirst
)
{
    tangentAxisWidget->bindReference(tangentAxis);
    verticalAxisWidget->bindReference(verticalAxis);

    pathProperty = path;
    countProperty = count;
    spacingModeProperty = spacingMode;
    spacingProperty = spacing;
    startOffsetProperty = startOffset;
    endOffsetProperty = endOffset;
    reversePathProperty = reversePath;
    alignProperty = align;
    cyclicOffsetProperty = cyclicOffset;
    tangentAxisProperty = tangentAxis;
    flipTangentAxisProperty = flipTangentAxis;
    verticalAxisProperty = verticalAxis;
    flipVerticalAxisProperty = flipVerticalAxis;
    originPointIndexProperty = originPointIndex;
    relativeToFirstProperty = relativeToFirst;

    {
        const QSignalBlocker countBlocker(ui->count);
        const QSignalBlocker spacingBlocker(ui->spacing);
        const QSignalBlocker startOffsetBlocker(ui->startOffset);
        const QSignalBlocker endOffsetBlocker(ui->endOffset);
        const QSignalBlocker cyclicOffsetBlocker(ui->cyclicOffset);
        ui->count->setRange(countProperty->getMinimum(), countProperty->getMaximum());
        ui->count->bind(*countProperty);
        ui->spacing->setUnit(Base::Unit::Length);
        ui->startOffset->setUnit(Base::Unit::Length);
        ui->endOffset->setUnit(Base::Unit::Length);
        ui->cyclicOffset->setUnit(Base::Unit::Length);
        ui->spacing->bind(*spacingProperty);
        ui->startOffset->bind(*startOffsetProperty);
        ui->endOffset->bind(*endOffsetProperty);
        ui->cyclicOffset->bind(*cyclicOffsetProperty);
    }
    ui->spacingMode->addItems({tr("Fixed count"), tr("Fixed spacing"), tr("Fixed count and spacing")});
    updateUI();
}

void PatternPathParametersWidget::updateUI()
{
    if (blockUpdate || !countProperty) {
        return;
    }
    Base::StateLocker locker(blockUpdate, true);
    tangentAxisWidget->updateReferenceUI();
    verticalAxisWidget->updateReferenceUI();
    updatePathButton();
    ui->count->setValue(countProperty->getValue());
    ui->spacingMode->setCurrentIndex(spacingModeProperty->getValue());
    ui->spacing->setValue(spacingProperty->getValue());
    ui->startOffset->setValue(startOffsetProperty->getValue());
    ui->endOffset->setValue(endOffsetProperty->getValue());
    ui->cyclicOffset->setValue(cyclicOffsetProperty->getValue());
    ui->reversePath->setChecked(reversePathProperty->getValue());
    ui->align->setChecked(alignProperty->getValue());
    ui->flipTangentAxis->setChecked(flipTangentAxisProperty->getValue());
    ui->flipVerticalAxis->setChecked(flipVerticalAxisProperty->getValue());
    ui->relativeToFirst->setChecked(relativeToFirstProperty->getValue());
    ui->originPointIndex->setValue(originPointIndexProperty->getValue());
    updateVisibility();
}

bool PatternPathParametersWidget::getFlipTangentAxis() const
{
    return ui->flipTangentAxis->isChecked();
}

bool PatternPathParametersWidget::getFlipVerticalAxis() const
{
    return ui->flipVerticalAxis->isChecked();
}

int PatternPathParametersWidget::getOriginPointIndex() const
{
    return ui->originPointIndex->value();
}

bool PatternPathParametersWidget::getRelativeToFirst() const
{
    return ui->relativeToFirst->isChecked();
}

void PatternPathParametersWidget::getPath(
    App::DocumentObject*& object,
    std::vector<std::string>& subnames
) const
{
    object = pathProperty ? pathProperty->getValue() : nullptr;
    subnames = pathProperty ? pathProperty->getSubValues() : std::vector<std::string>();
}

void PatternPathParametersWidget::updatePathButton()
{
    if (!pathProperty || !pathProperty->getValue()) {
        ui->pathButton->setText(tr("Select Path"));
        return;
    }

    QString text = QString::fromUtf8(pathProperty->getValue()->Label.getValue());
    if (text.isEmpty()) {
        text = QString::fromLatin1(pathProperty->getValue()->getNameInDocument());
    }
    const auto subnames = pathProperty->getSubValues();
    if (!subnames.empty()) {
        text += QStringLiteral(" : %1").arg(QString::fromStdString(subnames.front()));
        if (subnames.size() > 1) {
            text += QStringLiteral(" ") + tr("(+%1)").arg(subnames.size() - 1);
        }
    }
    ui->pathButton->setText(text);
}

void PatternPathParametersWidget::applyQuantitySpinboxes() const
{
    ui->count->apply();
    ui->spacing->apply();
    ui->startOffset->apply();
    ui->endOffset->apply();
    ui->cyclicOffset->apply();
}

void PatternPathParametersWidget::updateVisibility()
{
    const auto mode = static_cast<Part::PathPatternSpacingMode>(ui->spacingMode->currentIndex());
    ui->countLabel->setVisible(mode != Part::PathPatternSpacingMode::FixedSpacing);
    ui->count->setVisible(mode != Part::PathPatternSpacingMode::FixedSpacing);
    ui->spacingLabel->setVisible(mode != Part::PathPatternSpacingMode::FixedCount);
    ui->spacing->setVisible(mode != Part::PathPatternSpacingMode::FixedCount);
    ui->tangentAxisLabel->setVisible(ui->align->isChecked());
    ui->tangentAxis->setVisible(ui->align->isChecked());
    ui->flipTangentAxis->setVisible(ui->align->isChecked());
    ui->verticalAxisLabel->setVisible(ui->align->isChecked());
    ui->verticalAxis->setVisible(ui->align->isChecked());
    ui->flipVerticalAxis->setVisible(ui->align->isChecked());
    ui->originPointIndexLabel->setVisible(ui->relativeToFirst->isChecked() || ui->align->isChecked());
    ui->originPointIndex->setVisible(ui->relativeToFirst->isChecked() || ui->align->isChecked());
}
