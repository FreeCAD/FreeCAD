// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>

#include "FeatureDressUp.h"

namespace PartDesign
{

class PartDesignExport Thread: public DressUp
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Thread);

public:
    Thread();

    App::PropertyEnumeration ThreadFit;
    App::PropertyLength ThreadDiameter;
    App::PropertyEnumeration ThreadDirection;

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
    void addThreadType();
};

}  // namespace PartDesign
