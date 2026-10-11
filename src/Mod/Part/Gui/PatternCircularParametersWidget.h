// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <memory>

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <Mod/Part/PartGlobal.h>

#include "PatternReferenceWidget.h"

namespace PartGui
{

class Ui_PatternCircularParametersWidget;

class PartGuiExport PatternCircularParametersWidget: public QWidget
{
    Q_OBJECT

public:
    explicit PatternCircularParametersWidget(QWidget* parent = nullptr);
    ~PatternCircularParametersWidget() override;

    void bindProperties(
        App::PropertyLinkSub* axis,
        App::PropertyLength* radialDistance,
        App::PropertyLength* tangentialDistance,
        App::PropertyIntegerConstraint* numberCircles,
        App::PropertyIntegerConstraint* symmetry,
        App::PropertyBool* centerAroundFeature,
        App::PropertyLinkSub* tangentAxis,
        App::PropertyBool* flipTangentAxis
    );
    void updateUI();
    void applyQuantitySpinboxes() const;
    bool centerAroundFeature() const;
    bool flipTangentAxis() const;

    inline PatternReferenceWidget* getAxisWidget() const
    {
        return axisWidget;
    }
    inline PatternReferenceWidget* getTangentWidget() const
    {
        return tangentWidget;
    }

private:
    void onRadialDistanceChanged(double value);
    void onTangentialDistanceChanged(double value);
    void onNumberCirclesChanged(unsigned value);
    void onSymmetryChanged(unsigned value);
    void onCenterAroundFeatureChanged(bool value);

    std::unique_ptr<Ui_PatternCircularParametersWidget> ui;
    App::PropertyLength* radialDistanceProperty = nullptr;
    App::PropertyLength* tangentialDistanceProperty = nullptr;
    App::PropertyIntegerConstraint* numberCirclesProperty = nullptr;
    App::PropertyIntegerConstraint* symmetryProperty = nullptr;
    App::PropertyBool* centerAroundFeatureProperty = nullptr;
    App::PropertyBool* flipTangentAxisProperty = nullptr;
    bool blockUpdate = false;

    PatternReferenceWidget* axisWidget = nullptr;
    PatternReferenceWidget* tangentWidget = nullptr;
};

}  // namespace PartGui
