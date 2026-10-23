// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <list>

#include <App/DocumentObjectExtension.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include "gp_Ax2.hxx"
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
    App::PropertyDistance CyclicOffset;
    App::PropertyBool ReversePath;
    App::PropertyBool Align;
    App::PropertyLinkSub VerticalAxis;
    App::PropertyBool FlipVerticalAxis;
    App::PropertyLinkSub TangentAxis;
    App::PropertyBool FlipTangentAxis;
    App::PropertyInteger OriginPointIndex;
    App::PropertyBool RelativeToFirst;

    std::list<gp_Trsf> calculateTransformations() const;
    bool identityTransformSuppressed() const;

    short extensionMustExecute() override;
    void extensionOnChanged(const App::Property* prop) override;

    static const App::PropertyIntegerConstraint::Constraints intCount;
    static const char* SpacingModeEnums[];

protected:
    virtual std::optional<gp_Ax2> getAxis(const App::PropertyLinkSub& property) const;

private:
    void updatePropertyStatus();
};

}  // namespace Part
