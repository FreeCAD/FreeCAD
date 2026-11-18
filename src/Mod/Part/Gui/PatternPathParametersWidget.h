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

class PartGuiExport PatternPathParametersWidget: public QWidget
{
    Q_OBJECT

public:
    static inline const int PATH_INDEX = 0;
    static inline const int TANGENT_AXIS_INDEX = 1;
    static inline const int VERTICAL_AXIS_INDEX = 2;

    explicit PatternPathParametersWidget(QWidget* parent = nullptr);
    ~PatternPathParametersWidget() override;

    void bindProperties(
        App::PropertyLinkSub* path,
        App::PropertyIntegerConstraint* count,
        App::PropertyEnumeration* spacingMode,
        App::PropertyLength* spacing,
        App::PropertyLength* startOffset,
        App::PropertyLength* endOffset,
        App::PropertyDistance* cyclicOffset,
        App::PropertyBool* reversePath,
        App::PropertyBool* align,
        App::PropertyLinkSub* tangentAxis,
        App::PropertyBool* flipTangentAxis,
        App::PropertyLinkSub* verticalAxis,
        App::PropertyBool* flipVerticalAxis,
        App::PropertyInteger* originPointIndex,
        App::PropertyBool* relativeToFirst
    );
    void updateUI();
    void applyQuantitySpinboxes() const;
    void getPath(App::DocumentObject*& object, std::vector<std::string>& subnames) const;
    bool getFlipVerticalAxis() const;
    bool getFlipTangentAxis() const;
    int getOriginPointIndex() const;
    bool getRelativeToFirst() const;

    inline PatternReferenceWidget* getPathWidget() const
    {
        return pathWidget;
    }
    inline PatternReferenceWidget* getTangentAxisWidget() const
    {
        return tangentAxisWidget;
    }
    inline PatternReferenceWidget* getVerticalAxisWidget() const
    {
        return verticalAxisWidget;
    }

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
    App::PropertyDistance* cyclicOffsetProperty = nullptr;
    App::PropertyBool* reversePathProperty = nullptr;
    App::PropertyBool* alignProperty = nullptr;
    App::PropertyLinkSub* tangentAxisProperty = nullptr;
    App::PropertyBool* flipTangentAxisProperty = nullptr;
    App::PropertyLinkSub* verticalAxisProperty = nullptr;
    App::PropertyBool* flipVerticalAxisProperty = nullptr;
    App::PropertyInteger* originPointIndexProperty = nullptr;
    App::PropertyBool* relativeToFirstProperty = nullptr;
    bool blockUpdate = false;

    PatternReferenceWidget* pathWidget = nullptr;
    PatternReferenceWidget* tangentAxisWidget = nullptr;
    PatternReferenceWidget* verticalAxisWidget = nullptr;
};

}  // namespace PartGui
