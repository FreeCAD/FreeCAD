// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/PropertyLinks.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Surface/SurfaceGlobal.h>

namespace Surface
{

class SurfaceExport IntersectionCurve: public Part::Feature
{
    // The shared property macro uses sizeof and type traits for its compile-time checks.
    // NOLINTNEXTLINE(bugprone-sizeof-expression,modernize-type-traits)
    PROPERTY_HEADER_WITH_OVERRIDE(Surface::IntersectionCurve);

public:
    IntersectionCurve();

    App::PropertyLink Curve1;
    App::PropertyLink Curve2;
    App::PropertyVector Direction1;
    App::PropertyVector Direction2;

    short mustExecute() const override;
    App::DocumentObjectExecReturn* execute() override;
    const char* getViewProviderName() const override
    {
        return "SurfaceGui::ViewProviderIntersectionCurve";
    }
};

}  // namespace Surface
