// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "App/PropertyStandard.h"


#include <list>

#include <App/DocumentObjectExtension.h>
#include <App/PropertyLinks.h>
#include <gp_Trsf.hxx>

#include <Mod/Part/PartGlobal.h>

namespace Part
{

class PartExport PointPatternExtension: public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Part::PointPatternExtension);

public:
    PointPatternExtension();
    ~PointPatternExtension() override = default;

    App::PropertyLinkSub PointObject;
    App::PropertyInteger OriginPointIndex;
    App::PropertyBool RelativeToFirst;

    std::list<gp_Trsf> calculateTransformations() const;
    bool identityTransformSuppressed() const;
    short extensionMustExecute() override;

    void extensionOnChanged(const App::Property* prop) override;
};

}  // namespace Part
