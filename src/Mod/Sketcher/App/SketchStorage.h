#ifndef SKETCHER_SKETCH_STORAGE_H
#define SKETCHER_SKETCH_STORAGE_H

#include <Mod/Part/App/Geometry.h>
#include "planegcs/GCS.h"
#include "GeometryType.h"
#include "Constraint.h"

namespace Sketcher
{
class SketcherExport SketchStorage 
{ 
	public:
		SketchStorage();
		~SketchStorage();

	    void clear(void) {

	    	Points.clear();
		    Lines.clear();
		    Arcs.clear();
		    Circles.clear();
		    Ellipses.clear();
		    ArcsOfEllipse.clear();
		    ArcsOfHyperbola.clear();
		    ArcsOfParabola.clear();
		    BSplines.clear();

		      for (auto &it : Geoms)
		      {
				if (it.geo)
					delete it.geo;
		      }
        		
    		Geoms.clear();
		    Constrs.clear();

	    }
		
		std::vector<GCS::Point>  Points;
	    std::vector<GCS::Line>   Lines;
	    std::vector<GCS::Arc>    Arcs;
	    std::vector<GCS::Circle> Circles;
	    std::vector<GCS::Ellipse> Ellipses;
	    std::vector<GCS::ArcOfEllipse> ArcsOfEllipse;
	    std::vector<GCS::ArcOfHyperbola> ArcsOfHyperbola;
	    std::vector<GCS::ArcOfParabola> ArcsOfParabola;
	    std::vector<GCS::BSpline> BSplines;
    /// container element to store and work with the geometric elements of this sketch

	    struct GeoDef {
	        GeoDef() : geo(0),type(GeometryType::None),external(false),index(-1),
	                   startPointId(-1),midPointId(-1),endPointId(-1) {}
	        Part::Geometry  * geo;             // pointer to the geometry
	        GeometryType       type;            // type of the geometry
	        bool              external;        // flag for external geometries
	        int               index;           // index in the corresponding storage vector (Lines, Arcs, Circles, ...)
	        int               startPointId;    // index in Points of the start point of this geometry
	        int               midPointId;      // index in Points of the start point of this geometry
	        int               endPointId;      // index in Points of the end point of this geometry
	    };
    /// container element to store and work with the constraints of this sketch
	    struct ConstrDef {
	        ConstrDef() : constr(0)
	                    , driving(true)
	                    , value(0)
	                    , secondvalue(0) {}
	        Constraint *    constr;             // pointer to the constraint
	        bool            driving;
	        double *        value;
	        double *        secondvalue;        // this is needed for SnellsLaw
	    };

    std::vector<GeoDef> Geoms;
    std::vector<ConstrDef> Constrs;

};


}


#endif // SKETCHER_SKETCH_STORAGE_H