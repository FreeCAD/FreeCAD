// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include <Mod/Part/App/Geometry.h>

#include "GeomReferencePlane3D.h"
#include "GeoEnum3D.h"


namespace Sketcher3D
{

GeoKind kindOfGeometry(const Part::Geometry* geo)
{
    if (!geo) {
        return GeoKind::Unknown;
    }
    if (geo->is<Part::GeomPoint>()) {
        return GeoKind::Point;
    }
    if (geo->is<Part::GeomLineSegment>()) {
        return GeoKind::Line;
    }
    if (geo->is<GeomReferencePlane3D>()) {
        return GeoKind::Plane;
    }
    if (geo->is<Part::GeomArcOfCircle>()) {
        return GeoKind::Arc;
    }
    if (geo->is<Part::GeomCircle>()) {
        return GeoKind::Circle;
    }
    return GeoKind::Unknown;
}

}  // namespace Sketcher3D
