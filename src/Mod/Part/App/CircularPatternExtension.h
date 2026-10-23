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

inline const auto BASE_X_AXIS = gp_Ax2(gp_Pnt(), gp_Dir(1, 0, 0));
inline const auto BASE_Y_AXIS = gp_Ax2(gp_Pnt(), gp_Dir(0, 1, 0));
inline const auto BASE_Z_AXIS = gp_Ax2(gp_Pnt(), gp_Dir(0, 0, 1));

std::optional<gp_Ax2> getAxis(
    const App::PropertyLinkSub& property,
    const App::DocumentObject* extendedObject
);

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
    gp_Ax2 getAxis() const;
    virtual std::optional<gp_Ax2> getAxis(const App::PropertyLinkSub& property) const;
};

}  // namespace Part
