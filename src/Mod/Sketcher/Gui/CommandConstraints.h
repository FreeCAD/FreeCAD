/***************************************************************************
 *   Copyright (c) 2014 Abdullah.tahiri.yo@gmail.com                       *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef SKETCHERGUI_CommandConstraints_H
#define SKETCHERGUI_CommandConstraints_H

#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/Constraint.h>

namespace SketcherGui {

bool checkBothExternal(int GeoId1, int GeoId2);

bool checkBothExternalOrConstructionPoints(const Sketcher::SketchObject* Obj,int GeoId1, int GeoId2);

void getIdsFromName(const std::string &name, const Sketcher::SketchObject* Obj, int &GeoId, Sketcher::PointPos &PosId);

bool inline isVertex(int GeoId, Sketcher::PointPos PosId);

bool inline isEdge(int GeoId, Sketcher::PointPos PosId);

bool isSimpleVertex(const Sketcher::SketchObject* Obj, int GeoId, Sketcher::PointPos PosId);

bool isConstructionPoint(const Sketcher::SketchObject* Obj, int GeoId);

bool IsPointAlreadyOnCurve(int GeoIdCurve, int GeoIdPoint, Sketcher::PointPos PosIdPoint, Sketcher::SketchObject* Obj);


// These functions are declared here to promote code reuse from other modules
    
/// Makes a tangency constraint using external construction line between
/// geom1 => an ellipse
/// geom2 => any of an ellipse, an arc of ellipse, a circle, or an arc (of circle)
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp    
void makeTangentToEllipseviaNewPoint(const Sketcher::SketchObject* Obj,
                                             const Part::Geometry *geom1, 
                                             const Part::Geometry *geom2,
                                             int geoId1,
                                             int geoId2
                                            );
/// Makes a tangency constraint using external construction line between
/// geom1 => an arc of ellipse
/// geom2 => any of an arc of ellipse, a circle, or an arc (of circle)
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp 
void makeTangentToArcOfEllipseviaNewPoint(const Sketcher::SketchObject* Obj,
                                             const Part::Geometry *geom1, 
                                             const Part::Geometry *geom2,
                                             int geoId1,
                                             int geoId2
                                            );

/// Makes a tangency constraint using external construction line between
/// geom1 => an arc of hyperbola
/// geom2 => any of an arc of hyperbola, an arc of ellipse, a circle, or an arc (of circle)
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp 
void makeTangentToArcOfHyperbolaviaNewPoint(const Sketcher::SketchObject* Obj,
                                          const Part::Geometry *geom1, 
                                          const Part::Geometry *geom2,
                                          int geoId1,
                                          int geoId2
);

/// Makes a simple tangency constraint using extra point + tangent via point
/// geom1 => an arc of parabola
/// geom2 => any of an arc of parabola, an arc of hyperbola an arc of ellipse, a circle, or an arc (of circle)
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void makeTangentToArcOfParabolaviaNewPoint(const Sketcher::SketchObject* Obj,
                                                       const Part::Geometry *geom1, 
                                                       const Part::Geometry *geom2,
                                                       int geoId1,
                                                       int geoId2
);

std::string getStrippedPythonExceptionString(const Base::Exception);

}
#endif // SKETCHERGUI_DrawSketchHandler_H

