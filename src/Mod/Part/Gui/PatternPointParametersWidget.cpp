// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PatternPointParametersWidget.h"

#include <QPushButton>

#include <App/DocumentObject.h>

#include "ui_PatternPointParametersWidget.h"

using namespace PartGui;

PatternPointParametersWidget::PatternPointParametersWidget(QWidget* parent)
    : PatternReferenceWidget(parent)
    , ui(new Ui_PatternPointParametersWidget)
{
    ui->setupUi(this);
    connect(ui->pointObjectButton, &QPushButton::clicked, this, [this]() {
        Q_EMIT requestReferenceSelection();
    });
}

PatternPointParametersWidget::~PatternPointParametersWidget() = default;

void PatternPointParametersWidget::bindProperty(App::PropertyLinkSub* pointObject)
{
    pointObjectProperty = pointObject;
    updateUI();
}

void PatternPointParametersWidget::updateUI()
{
    updatePointObjectButton();
}

void PatternPointParametersWidget::getPointObject(
    App::DocumentObject*& object,
    std::vector<std::string>& subnames
) const
{
    object = pointObjectProperty ? pointObjectProperty->getValue() : nullptr;
    subnames = pointObjectProperty ? pointObjectProperty->getSubValues() : std::vector<std::string>();
}

void PatternPointParametersWidget::updatePointObjectButton()
{
    if (!pointObjectProperty || !pointObjectProperty->getValue()) {
        ui->pointObjectButton->setText(tr("Select point object…"));
        return;
    }

    QString text = QString::fromUtf8(pointObjectProperty->getValue()->Label.getValue());
    if (text.isEmpty()) {
        text = QString::fromLatin1(pointObjectProperty->getValue()->getNameInDocument());
    }
    ui->pointObjectButton->setText(text);
}
