// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PatternCircularParametersWidget.h"

#include <QSignalBlocker>

#include <Base/Tools.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/SpinBox.h>

#include "ui_PatternCircularParametersWidget.h"

using namespace PartGui;

PatternCircularParametersWidget::PatternCircularParametersWidget(QWidget* parent)
    : PatternReferenceWidget(parent)
    , ui(new Ui_PatternCircularParametersWidget)
{
    ui->setupUi(this);
    setupReferenceCombo(ui->axisCombo);

    connect(ui->radialDistance,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &PatternCircularParametersWidget::onRadialDistanceChanged);
    connect(ui->tangentialDistance,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &PatternCircularParametersWidget::onTangentialDistanceChanged);
    connect(ui->numberCircles,
            &Gui::UIntSpinBox::unsignedChanged,
            this,
            &PatternCircularParametersWidget::onNumberCirclesChanged);
    connect(ui->symmetry,
            &Gui::UIntSpinBox::unsignedChanged,
            this,
            &PatternCircularParametersWidget::onSymmetryChanged);
}

PatternCircularParametersWidget::~PatternCircularParametersWidget() = default;

void PatternCircularParametersWidget::bindProperties(
    App::PropertyLinkSub* axis,
    App::PropertyLength* radialDistance,
    App::PropertyLength* tangentialDistance,
    App::PropertyIntegerConstraint* numberCircles,
    App::PropertyIntegerConstraint* symmetry
)
{
    bindReference(axis);
    radialDistanceProperty = radialDistance;
    tangentialDistanceProperty = tangentialDistance;
    numberCirclesProperty = numberCircles;
    symmetryProperty = symmetry;

    {
        const QSignalBlocker radialBlocker(ui->radialDistance);
        const QSignalBlocker tangentialBlocker(ui->tangentialDistance);
        const QSignalBlocker circleCountBlocker(ui->numberCircles);
        const QSignalBlocker symmetryBlocker(ui->symmetry);
        ui->radialDistance->setUnit(Base::Unit::Length);
        ui->tangentialDistance->setUnit(Base::Unit::Length);
        ui->radialDistance->bind(*radialDistanceProperty);
        ui->tangentialDistance->bind(*tangentialDistanceProperty);
        ui->numberCircles->setRange(numberCirclesProperty->getMinimum(),
                                    numberCirclesProperty->getMaximum());
        ui->symmetry->setRange(symmetryProperty->getMinimum(),
                               symmetryProperty->getMaximum());
        ui->numberCircles->bind(*numberCirclesProperty);
        ui->symmetry->bind(*symmetryProperty);
    }

    updateUI();
}

void PatternCircularParametersWidget::updateUI()
{
    if (blockUpdate || !radialDistanceProperty) {
        return;
    }

    Base::StateLocker locker(blockUpdate, true);
    updateReferenceUI();
    ui->radialDistance->setValue(radialDistanceProperty->getValue());
    ui->tangentialDistance->setValue(tangentialDistanceProperty->getValue());
    ui->numberCircles->setValue(numberCirclesProperty->getValue());
    ui->symmetry->setValue(symmetryProperty->getValue());
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
    Q_EMIT parametersChanged();
}

void PatternCircularParametersWidget::onTangentialDistanceChanged(double value)
{
    if (blockUpdate || !tangentialDistanceProperty) {
        return;
    }
    tangentialDistanceProperty->setValue(value);
    Q_EMIT parametersChanged();
}

void PatternCircularParametersWidget::onNumberCirclesChanged(unsigned value)
{
    if (blockUpdate || !numberCirclesProperty) {
        return;
    }
    numberCirclesProperty->setValue(value);
    Q_EMIT parametersChanged();
}

void PatternCircularParametersWidget::onSymmetryChanged(unsigned value)
{
    if (blockUpdate || !symmetryProperty) {
        return;
    }
    symmetryProperty->setValue(value);
    Q_EMIT parametersChanged();
}
