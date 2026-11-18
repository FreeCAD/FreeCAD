// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "TaskFeatureParameters.h"

class QComboBox;
class QLabel;

namespace PartDesignGui
{

class TaskFeatureAddSubParameters: public TaskFeatureParameters
{
    Q_OBJECT

public:
    TaskFeatureAddSubParameters(
        ViewProvider* vp,
        QWidget* parent,
        const std::string& pixmapname,
        const QString& parname
    );

    void apply() override;

protected:
    void setupOperation(QLabel* label, QComboBox* combo);
};

}  // namespace PartDesignGui
