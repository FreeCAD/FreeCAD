// SPDX-License-Identifier: LGPL-2.1-or-later

#include "TaskRibParameters.h"
#include "ViewProviderRib.h"
#include "ui_TaskRibParameters.h"

#include <QEvent>
#include <Mod/PartDesign/App/FeatureRib.h>

namespace PartDesignGui
{

TaskRibParameters::TaskRibParameters(ViewProviderRib* view)
    : TaskFeatureParameters(view, nullptr, "PartDesign_Rib", tr("Rib Parameters"))
    , ui(std::make_unique<Ui_TaskRibParameters>())
{
    auto container = new QWidget(this);
    ui->setupUi(container);
    groupLayout()->addWidget(container);

    if (auto profile = getObject<PartDesign::Rib>()->Profile.getValue()) {
        ui->profileName->setText(QString::fromUtf8(profile->Label.getValue()));
    }
}

TaskRibParameters::~TaskRibParameters() = default;

void TaskRibParameters::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this->findChild<QWidget*>("ribParametersPanel"));
    }
    TaskFeatureParameters::changeEvent(event);
}

TaskDlgRibParameters::TaskDlgRibParameters(ViewProviderRib* view)
    : TaskDlgSketchBasedParameters(view)
{
    Content.push_back(new TaskRibParameters(view));
    Content.push_back(preview);
}

}  // namespace PartDesignGui
