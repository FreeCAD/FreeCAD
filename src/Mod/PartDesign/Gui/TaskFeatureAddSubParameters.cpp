// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QComboBox>
#include <QLabel>

#include <Gui/CommandT.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>

#include "TaskFeatureAddSubParameters.h"

using namespace PartDesignGui;

TaskFeatureAddSubParameters::TaskFeatureAddSubParameters(
    ViewProvider* vp,
    QWidget* parent,
    const std::string& pixmapname,
    const QString& parname
)
    : TaskFeatureParameters(vp, parent, pixmapname, parname)
{}

void TaskFeatureAddSubParameters::setupOperation(QLabel* label, QComboBox* combo)
{
    auto feature = getObject<PartDesign::FeatureAddSub>();
    const bool subtractive = feature
        && feature->getAddSubType() == PartDesign::FeatureAddSub::Type::Subtractive;
    label->setVisible(subtractive);
    combo->setVisible(subtractive);
    if (!subtractive) {
        return;
    }

    combo->setCurrentIndex(feature->Operation.getValue());
    combo->setDisabled(feature->Operation.isReadOnly());
    connect(combo, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        if (auto feature = getObject<PartDesign::FeatureAddSub>()) {
            feature->Operation.setValue(index);
            recomputeFeature();
        }
    });
}

void TaskFeatureAddSubParameters::apply()
{
    auto feature = getObject<PartDesign::FeatureAddSub>();
    if (feature && feature->getAddSubType() == PartDesign::FeatureAddSub::Type::Subtractive
        && !feature->Operation.isReadOnly()) {
        FCMD_OBJ_CMD(feature, "Operation = \"" << feature->Operation.getValueAsString() << "\"");
    }
}

#include "moc_TaskFeatureAddSubParameters.cpp"
