// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <memory>

#include <App/PropertyGeo.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <Mod/Part/PartGlobal.h>

#include "PatternReferenceWidget.h"

namespace PartGui
{

class Ui_PatternPathParametersWidget;

class PartGuiExport PatternPathParametersWidget: public PatternReferenceWidget
{
    Q_OBJECT

public:
    explicit PatternPathParametersWidget(QWidget* parent = nullptr);
    ~PatternPathParametersWidget() override;

    void bindProperties(
        App::PropertyLinkSub* path,
        App::PropertyIntegerConstraint* count,
        App::PropertyEnumeration* spacingMode,
        App::PropertyLength* spacing,
        App::PropertyLength* startOffset,
        App::PropertyLength* endOffset,
        App::PropertyBool* reversePath,
        App::PropertyBool* align
    );
    void updateUI();
    void applyQuantitySpinboxes() const;
    void getPath(App::DocumentObject*& object, std::vector<std::string>& subnames) const;

private:
    void updateVisibility();
    void updatePathButton();

    std::unique_ptr<Ui_PatternPathParametersWidget> ui;
    App::PropertyLinkSub* pathProperty = nullptr;
    App::PropertyIntegerConstraint* countProperty = nullptr;
    App::PropertyEnumeration* spacingModeProperty = nullptr;
    App::PropertyLength* spacingProperty = nullptr;
    App::PropertyLength* startOffsetProperty = nullptr;
    App::PropertyLength* endOffsetProperty = nullptr;
    App::PropertyBool* reversePathProperty = nullptr;
    App::PropertyBool* alignProperty = nullptr;
    bool blockUpdate = false;
};

}  // namespace PartGui
