#include "PreCompiled.h"
#include "GeometryType.h"

#include <Mod/Part/App/Geometry.h>

using namespace Sketcher;
const char * GeometryType::str() const {

	switch (value) {
	    case Point:
	        return "point";
	    case Line:
	        return "line";
	    case Arc:
	        return "arc";
	    case Circle:
	        return "circle";
	    case Ellipse:
	        return "ellipse";
	    case ArcOfEllipse:
	        return "arcofellipse";
	    case ArcOfHyperbola:
	        return "arcofhyperbola";
	    case ArcOfParabola:
	        return "arcofparabola";
	    case BSpline:
	        return "bspline";
	    case None:
	    default:
	        return "unknown";
    }
}

GeometryType GeometryType::from(const Base::Type & geometryClassType) {

	if(geometryClassType == Part::GeomPoint::getClassTypeId()){
		return GeometryType::Point;
	}
	return GeometryType::None;
}