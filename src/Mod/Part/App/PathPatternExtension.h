// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <list>

#include <App/DocumentObjectExtension.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <gp_Trsf.hxx>

#include <Mod/Part/PartGlobal.h>

namespace Part
{

enum class PathPatternSpacingMode
{
    FixedCount,
    FixedSpacing,
    FixedCountAndSpacing
};

class PartExport PathPatternExtension: public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Part::PathPatternExtension);

public:
    PathPatternExtension();
    ~PathPatternExtension() override = default;

    App::PropertyLinkSub Path;
    App::PropertyIntegerConstraint Count;
    App::PropertyEnumeration SpacingMode;
    App::PropertyLength Spacing;
    App::PropertyLength StartOffset;
    App::PropertyLength EndOffset;
    App::PropertyBool ReversePath;
    App::PropertyBool Align;
    App::PropertyVector VerticalVector;

    std::list<gp_Trsf> calculateTransformations(bool relativeToFirst) const;

    short extensionMustExecute() override;
    void extensionOnChanged(const App::Property* prop) override;

    static const App::PropertyIntegerConstraint::Constraints intCount;
    static const char* SpacingModeEnums[];

private:
    void updatePropertyStatus();
};

}  // namespace Part
