#include "GeometryType.h"
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