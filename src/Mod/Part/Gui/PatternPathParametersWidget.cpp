// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PatternPathParametersWidget.h"

#include <QComboBox>
#include <QPushButton>
#include <QSignalBlocker>

#include <App/DocumentObject.h>
#include <Base/Tools.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/SpinBox.h>

#include "ui_PatternPathParametersWidget.h"

using namespace PartGui;

PatternPathParametersWidget::PatternPathParametersWidget(QWidget* parent)
    : PatternReferenceWidget(parent)
    , ui(new Ui_PatternPathParametersWidget)
{
    ui->setupUi(this);
    connect(ui->pathButton, &QPushButton::clicked, this, [this]() {
        Q_EMIT requestReferenceSelection();
    });

    connect(ui->count, &Gui::UIntSpinBox::unsignedChanged, this, [this](unsigned value) {
        if (!blockUpdate && countProperty) {
            countProperty->setValue(value);
            Q_EMIT parametersChanged();
        }
    });
    connect(ui->spacingMode, qOverload<int>(&QComboBox::activated), this, [this](int value) {
        if (!blockUpdate && spacingModeProperty) {
            spacingModeProperty->setValue(value);
            updateVisibility();
            Q_EMIT parametersChanged();
        }
    });
    const auto quantityChanged = [this](App::PropertyLength* property, double value) {
        if (!blockUpdate && property) {
            property->setValue(value);
            Q_EMIT parametersChanged();
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
    connect(ui->reversePath, &QCheckBox::toggled, this, [this](bool value) {
        if (!blockUpdate && reversePathProperty) {
            reversePathProperty->setValue(value);
            Q_EMIT parametersChanged();
        }
    });
    connect(ui->align, &QCheckBox::toggled, this, [this](bool value) {
        if (!blockUpdate && alignProperty) {
            alignProperty->setValue(value);
            Q_EMIT parametersChanged();
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
    App::PropertyBool* reversePath,
    App::PropertyBool* align
)
{
    pathProperty = path;
    countProperty = count;
    spacingModeProperty = spacingMode;
    spacingProperty = spacing;
    startOffsetProperty = startOffset;
    endOffsetProperty = endOffset;
    reversePathProperty = reversePath;
    alignProperty = align;

    {
        const QSignalBlocker countBlocker(ui->count);
        const QSignalBlocker spacingBlocker(ui->spacing);
        const QSignalBlocker startOffsetBlocker(ui->startOffset);
        const QSignalBlocker endOffsetBlocker(ui->endOffset);
        ui->count->setRange(countProperty->getMinimum(), countProperty->getMaximum());
        ui->count->bind(*countProperty);
        ui->spacing->bind(*spacingProperty);
        ui->startOffset->bind(*startOffsetProperty);
        ui->endOffset->bind(*endOffsetProperty);
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
    updatePathButton();
    ui->count->setValue(countProperty->getValue());
    ui->spacingMode->setCurrentIndex(spacingModeProperty->getValue());
    ui->spacing->setValue(spacingProperty->getValue());
    ui->startOffset->setValue(startOffsetProperty->getValue());
    ui->endOffset->setValue(endOffsetProperty->getValue());
    ui->reversePath->setChecked(reversePathProperty->getValue());
    ui->align->setChecked(alignProperty->getValue());
    updateVisibility();
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
        ui->pathButton->setText(tr("Select path..."));
        return;
        ui->pathButton->setText(tr("Select path…"));

    QString text = QString::fromUtf8(pathProperty->getValue()->Label.getValue());
    if (text.isEmpty()) {
        text = QString::fromLatin1(pathProperty->getValue()->getNameInDocument());
    }
    const auto subnames = pathProperty->getSubValues();
    if (!subnames.empty()) {
        text += QStringLiteral(" : %1").arg(QString::fromStdString(subnames.front()));
        if (subnames.size() > 1) {
            text += tr(" (+%1)").arg(subnames.size() - 1);
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
}

void PatternPathParametersWidget::updateVisibility()
{
    const int mode = ui->spacingMode->currentIndex();
    ui->countLabel->setVisible(mode != 1);
    ui->count->setVisible(mode != 1);
    ui->spacingLabel->setVisible(mode != 0);
    ui->spacing->setVisible(mode != 0);
}
