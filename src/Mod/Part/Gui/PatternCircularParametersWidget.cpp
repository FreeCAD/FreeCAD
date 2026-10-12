// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PatternCircularParametersWidget.h"

#include <QSignalBlocker>
#include <QCheckBox>

#include <Base/Tools.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/SpinBox.h>

#include "ui_PatternCircularParametersWidget.h"

using namespace PartGui;

PatternCircularParametersWidget::PatternCircularParametersWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_PatternCircularParametersWidget)
    , axisWidget(new PatternReferenceWidget(this))
    , binormalAxisWidget(new PatternReferenceWidget(this))
{
    ui->setupUi(this);
    axisWidget->setupReferenceCombo(ui->axisCombo);
    binormalAxisWidget->setupReferenceCombo(ui->tangentAxisCombo);
    binormalAxisWidget->setAxisIndex(1);

    connect(
        ui->radialDistance,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &PatternCircularParametersWidget::onRadialDistanceChanged
    );
    connect(
        ui->tangentialDistance,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &PatternCircularParametersWidget::onTangentialDistanceChanged
    );
    connect(
        ui->numberCircles,
        &Gui::UIntSpinBox::unsignedChanged,
        this,
        &PatternCircularParametersWidget::onNumberCirclesChanged
    );
    connect(
        ui->symmetry,
        &Gui::UIntSpinBox::unsignedChanged,
        this,
        &PatternCircularParametersWidget::onSymmetryChanged
    );
    connect(
        ui->centerAroundFeature,
        &QCheckBox::toggled,
        this,
        &PatternCircularParametersWidget::onStartAtShapeChanged
    );
    connect(
        ui->flipTangentDirection,
        &QCheckBox::toggled,
        this,
        &PatternCircularParametersWidget::onFlipBinormalDirectionChanged
    );
}

PatternCircularParametersWidget::~PatternCircularParametersWidget() = default;

void PatternCircularParametersWidget::bindProperties(
    App::PropertyLinkSub* axis,
    App::PropertyLength* radialDistance,
    App::PropertyLength* tangentialDistance,
    App::PropertyIntegerConstraint* numberCircles,
    App::PropertyIntegerConstraint* symmetry,
    App::PropertyBool* startAtShape,
    App::PropertyLinkSub* binormalAxis,
    App::PropertyBool* flipBinormalAxis
)
{
    axisWidget->bindReference(axis);
    binormalAxisWidget->bindReference(binormalAxis);

    radialDistanceProperty = radialDistance;
    tangentialDistanceProperty = tangentialDistance;
    numberCirclesProperty = numberCircles;
    symmetryProperty = symmetry;
    startAtShapeProperty = startAtShape;
    flipBinormalAxisProperty = flipBinormalAxis;

    {
        const QSignalBlocker radialBlocker(ui->radialDistance);
        const QSignalBlocker tangentialBlocker(ui->tangentialDistance);
        const QSignalBlocker circleCountBlocker(ui->numberCircles);
        const QSignalBlocker symmetryBlocker(ui->symmetry);
        const QSignalBlocker centerAroundFeatureBlocker(ui->centerAroundFeature);
        const QSignalBlocker flipBinormalAxisBlocker(ui->flipTangentDirection);
        ui->radialDistance->setUnit(Base::Unit::Length);
        ui->tangentialDistance->setUnit(Base::Unit::Length);
        ui->radialDistance->bind(*radialDistanceProperty);
        ui->tangentialDistance->bind(*tangentialDistanceProperty);
        ui->numberCircles->setRange(
            numberCirclesProperty->getMinimum(),
            numberCirclesProperty->getMaximum()
        );
        ui->symmetry->setRange(symmetryProperty->getMinimum(), symmetryProperty->getMaximum());
        ui->numberCircles->bind(*numberCirclesProperty);
        ui->symmetry->bind(*symmetryProperty);
        ui->centerAroundFeature->setChecked(startAtShape->getValue());
        ui->flipTangentDirection->setChecked(flipBinormalAxis->getValue());
    }

    updateUI();
}

void PatternCircularParametersWidget::updateUI()
{
    if (blockUpdate || !radialDistanceProperty) {
        return;
    }

    Base::StateLocker locker(blockUpdate, true);
    axisWidget->updateReferenceUI();
    binormalAxisWidget->updateReferenceUI();
    ui->radialDistance->setValue(radialDistanceProperty->getValue());
    ui->tangentialDistance->setValue(tangentialDistanceProperty->getValue());
    ui->numberCircles->setValue(numberCirclesProperty->getValue());
    ui->symmetry->setValue(symmetryProperty->getValue());
}

bool PatternCircularParametersWidget::startAtShape() const
{
    return ui->centerAroundFeature->isChecked();
}

bool PatternCircularParametersWidget::flipBinormalAxis() const
{
    return ui->flipTangentDirection->isChecked();
}

void PatternCircularParametersWidget::applyQuantitySpinboxes() const
{
    ui->radialDistance->apply();
    ui->tangentialDistance->apply();
    ui->numberCircles->apply();
    ui->symmetry->apply();
}

void PatternCircularParametersWidget::onRadialDistanceChanged(double value)
{
    if (blockUpdate || !radialDistanceProperty) {
        return;
    }
    radialDistanceProperty->setValue(value);
    Q_EMIT axisWidget->parametersChanged();
    Q_EMIT binormalAxisWidget->parametersChanged();
}

void PatternCircularParametersWidget::onTangentialDistanceChanged(double value)
{
    if (blockUpdate || !tangentialDistanceProperty) {
        return;
    }
    tangentialDistanceProperty->setValue(value);
    Q_EMIT axisWidget->parametersChanged();
    Q_EMIT binormalAxisWidget->parametersChanged();
}

void PatternCircularParametersWidget::onStartAtShapeChanged(bool value)
{
    if (blockUpdate || !startAtShapeProperty) {
        return;
    }
    startAtShapeProperty->setValue(value);
    Q_EMIT axisWidget->parametersChanged();
    Q_EMIT binormalAxisWidget->parametersChanged();
}

void PatternCircularParametersWidget::onFlipBinormalDirectionChanged(bool value)
{
    if (blockUpdate || !flipBinormalAxisProperty) {
        return;
    }
    flipBinormalAxisProperty->setValue(value);
    Q_EMIT axisWidget->parametersChanged();
    Q_EMIT binormalAxisWidget->parametersChanged();
}

void PatternCircularParametersWidget::onNumberCirclesChanged(unsigned value)
{
    if (blockUpdate || !numberCirclesProperty) {
        return;
    }
    numberCirclesProperty->setValue(value);
    Q_EMIT axisWidget->parametersChanged();
    Q_EMIT binormalAxisWidget->parametersChanged();
}

void PatternCircularParametersWidget::onSymmetryChanged(unsigned value)
{
    if (blockUpdate || !symmetryProperty) {
        return;
    }
    symmetryProperty->setValue(value);
    Q_EMIT axisWidget->parametersChanged();
    Q_EMIT binormalAxisWidget->parametersChanged();
}
