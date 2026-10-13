// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <list>

#include <App/DocumentObjectExtension.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <gp_Ax2.hxx>
#include <gp_Trsf.hxx>

#include <Mod/Part/PartGlobal.h>

namespace Part
{

class PartExport CircularPatternExtension: public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Part::CircularPatternExtension);

public:
    CircularPatternExtension();
    ~CircularPatternExtension() override = default;

    App::PropertyLinkSub Axis;
    App::PropertyLength RadialDistance;
    App::PropertyLength TangentialDistance;
    App::PropertyIntegerConstraint NumberCircles;
    App::PropertyIntegerConstraint Symmetry;
    App::PropertyBool StartAtShape;
    App::PropertyLinkSub BinormalAxis;
    App::PropertyBool FlipBinormalAxis;

    std::list<gp_Trsf> calculateTransformations() const;

    short extensionMustExecute() override;

    static const App::PropertyIntegerConstraint::Constraints intNumberCircles;
    static const App::PropertyIntegerConstraint::Constraints intSymmetry;

protected:
    gp_Ax2 getRotation() const;
    virtual gp_Ax2 getRotation(const App::PropertyLinkSub& property) const;
};

}  // namespace Part
