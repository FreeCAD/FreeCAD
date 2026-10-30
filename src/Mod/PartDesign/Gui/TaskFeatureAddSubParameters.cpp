// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cstring>

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

namespace
{
QString operationText(const char* operation)
{
    if (std::strcmp(operation, "Union") == 0) {
        return TaskFeatureAddSubParameters::tr("Join");
    }
    if (std::strcmp(operation, "Subtraction") == 0) {
        return TaskFeatureAddSubParameters::tr("Cut");
    }
    if (std::strcmp(operation, "Common") == 0) {
        return TaskFeatureAddSubParameters::tr("Intersect");
    }
    return QString::fromUtf8(operation);
}

bool hasOperationChoice(PartDesign::FeatureAddSub* feature)
{
    return feature && feature->Operation.getEnum().maxValue() > 0;
}
}  // namespace

void TaskFeatureAddSubParameters::setupOperation(QLabel* label, QComboBox* combo)
{
    auto feature = getObject<PartDesign::FeatureAddSub>();
    const bool choice = hasOperationChoice(feature);
    label->setVisible(choice);
    combo->setVisible(choice);
    if (!choice) {
        return;
    }

    combo->clear();
    for (const std::string& operation : feature->Operation.getEnumVector()) {
        combo->addItem(operationText(operation.c_str()));
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
    if (hasOperationChoice(feature) && !feature->Operation.isReadOnly()) {
        FCMD_OBJ_CMD(feature, "Operation = \"" << feature->Operation.getValueAsString() << "\"");
    }
}

#include "moc_TaskFeatureAddSubParameters.cpp"
