// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PatternPointParametersWidget.h"

#include <QPushButton>

#include <App/DocumentObject.h>

#include "ui_PatternPointParametersWidget.h"
#include "Base/Tools.h"

using namespace PartGui;

PatternPointParametersWidget::PatternPointParametersWidget(QWidget* parent)
    : PatternReferenceWidget(parent)
    , ui(new Ui_PatternPointParametersWidget)
{
    ui->setupUi(this);
    connect(ui->pointObjectButton, &QPushButton::clicked, this, [this]() {
        Q_EMIT requestReferenceSelection();
    });
    connect(ui->originPointIndex, &QSpinBox::valueChanged, this, [this](int newValue) {
        if (!blockUpdate && originPointIndexProperty) {
            originPointIndexProperty->setValue(newValue);
            Q_EMIT parametersChanged();
        }
    });
    connect(ui->relativeToFirst, &QCheckBox::toggled, this, [this](bool newValue) {
        if (!blockUpdate && relativeToFirstProperty) {
            relativeToFirstProperty->setValue(newValue);
            updateVisibility();
            Q_EMIT parametersChanged();
        }
    });
}

PatternPointParametersWidget::~PatternPointParametersWidget() = default;

void PatternPointParametersWidget::bindProperty(
    App::PropertyLinkSub* pointObject,
    App::PropertyBool* relativeToFirst,
    App::PropertyInteger* originPointIndex
)
{
    pointObjectProperty = pointObject;
    relativeToFirstProperty = relativeToFirst;
    originPointIndexProperty = originPointIndex;
    updateUI();
}

void PatternPointParametersWidget::updateUI()
{
    if (blockUpdate) {
        return;
    }
    Base::StateLocker locker(blockUpdate, true);
    updatePointObjectButton();
    ui->originPointIndex->setValue(static_cast<int>(originPointIndexProperty->getValue()));
    ui->relativeToFirst->setChecked(relativeToFirstProperty->getValue());
    ui->originPointIndex->setEnabled(!ui->relativeToFirst->isChecked());
}

void PatternPointParametersWidget::getPointObject(
    App::DocumentObject*& object,
    std::vector<std::string>& subnames
) const
{
    object = pointObjectProperty ? pointObjectProperty->getValue() : nullptr;
    subnames = pointObjectProperty ? pointObjectProperty->getSubValues() : std::vector<std::string>();
}

bool PatternPointParametersWidget::getRelativeToFirst() const
{
    return ui->relativeToFirst->isChecked();
}

int PatternPointParametersWidget::getOriginPointIndex() const
{
    return ui->originPointIndex->value();
}

void PatternPointParametersWidget::updatePointObjectButton()
{
    if (!pointObjectProperty || !pointObjectProperty->getValue()) {
        ui->pointObjectButton->setText(tr("Select Point Object"));
        return;
    }

    QString text = QString::fromUtf8(pointObjectProperty->getValue()->Label.getValue());
    if (text.isEmpty()) {
        text = QString::fromLatin1(pointObjectProperty->getValue()->getNameInDocument());
    }
    ui->pointObjectButton->setText(text);
}

void PatternPointParametersWidget::updateVisibility()
{
    ui->originPointIndex->setVisible(ui->relativeToFirst->isChecked());
}
