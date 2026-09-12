// SPDX-License-Identifier: LGPL-2.1-or-later

#include "FeatureCircularPattern.h"

#include <Mod/Part/App/Tools.h>

#include <gp_Dir.hxx>

#include "DatumLine.h"

using namespace PartDesign;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::CircularPattern, PartDesign::Transformed)

CircularPattern::CircularPattern()
{
    Part::CircularPatternExtension::initExtension(this);
}

const std::list<gp_Trsf> CircularPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    return calculateTransformations();
}

gp_Ax2 CircularPattern::getRotation() const
{
    if (auto* line = freecad_cast<PartDesign::Line*>(Axis.getValue())) {
        const Base::Vector3d point = line->getBasePoint();
        const Base::Vector3d vector = line->getDirection();
        gp_Pnt base = Base::convertTo<gp_Pnt>(point);
        gp_Dir direction = Base::convertTo<gp_Dir>(vector);

        const TopLoc_Location inverse = getLocation().Inverted();
        base.Transform(inverse.Transformation());
        direction.Transform(inverse.Transformation());
        return gp_Ax2(base, direction);
    }

    return Part::CircularPatternExtension::getRotation();
}
