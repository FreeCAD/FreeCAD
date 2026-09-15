// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ShapeUtils.h"

#include <App/GeoFeature.h>
#include <Mod/Part/App/PartFeature.h>

namespace Surface
{

Part::TopoShape getTopoShapeInGlobalCoordinates(const App::DocumentObject* obj)
{
    Part::TopoShape shape = Part::Feature::getTopoShape(
        obj,
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
    );
    if (shape.isNull()) {
        return shape;
    }

    const auto geo = dynamic_cast<const App::GeoFeature*>(obj);
    if (geo) {
        const Base::Placement containerPlacement = geo->globalPlacement()
            * geo->Placement.getValue().inverse();
        shape.transformShape(containerPlacement.toMatrix(), false, true);
    }
    return shape;
}

}  // namespace Surface
