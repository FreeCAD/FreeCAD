// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>

#include "FeatureDressUp.h"
#include "ThreadUtils.h"

namespace PartDesign
{

class PartDesignExport Thread: public DressUp
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Thread);

public:
    Thread();

    enum ThreadType
    {
        Metric,
        MetricFine
    };

    App::PropertyLinkSub LateralFace;
    App::PropertyLinkSub StartPlane;
    App::PropertyLength Depth;
    App::PropertyLength ThreadDepth;
    App::PropertyLength ThreadDiameter;
    App::PropertyLength ThreadPitch;

    App::PropertyEnumeration ThreadType;
    App::PropertyEnumeration ThreadSize;
    App::PropertyEnumeration ThreadSizePitch;
    App::PropertyEnumeration ThreadClass;
    App::PropertyEnumeration ThreadFit;
    App::PropertyEnumeration DepthType;
    App::PropertyEnumeration ThreadDirection;
    App::PropertyEnumeration ThreadDepthType;

    App::PropertyBool UseCustomThreadClearance;
    App::PropertyBool ModelThread;
    App::PropertyBool CosmeticThread;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn* execute() override;
    // short mustExecute() const override;

    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderThread";
    }
    //@}

protected:
    void onChanged(const App::Property* prop) override;

private:
    ThreadUtils threadUtils;
    void addThreadType();
    void updateDiameterParam();
};

}  // namespace PartDesign
