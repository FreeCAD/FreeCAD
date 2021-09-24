/***************************************************************************
 *   Copyright (c) 2014 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

bool checkBothExternalOrBSplinePoints(const Sketcher::SketchObject* Obj,int GeoId1, int GeoId2);

bool isPointOrSegmentFixed(const Sketcher::SketchObject* Obj, int GeoId);

bool areBothPointsOrSegmentsFixed(const Sketcher::SketchObject* Obj, int GeoId1, int GeoId2);

bool areAllPointsOrSegmentsFixed(const Sketcher::SketchObject* Obj, int GeoId1, int GeoId2, int GeoId3);

void getIdsFromName(const std::string &name, const Sketcher::SketchObject* Obj, int &GeoId, Sketcher::PointPos &PosId);

bool inline isVertex(int GeoId, Sketcher::PointPos PosId);

bool inline isEdge(int GeoId, Sketcher::PointPos PosId);

bool isSimpleVertex(const Sketcher::SketchObject* Obj, int GeoId, Sketcher::PointPos PosId);

bool isBsplineKnot(const Sketcher::SketchObject* Obj, int GeoId);

bool IsPointAlreadyOnCurve(int GeoIdCurve, int GeoIdPoint, Sketcher::PointPos PosIdPoint, Sketcher::SketchObject* Obj);

bool isBsplinePole(const Part::Geometry * geo);

bool isBsplinePole(const Sketcher::SketchObject* Obj, int GeoId);

/// Release any currently-active handler for the document.
/// Returns true if a handler was released, and false if not
bool ReleaseHandler(Gui::Document* doc);

// These functions are declared here to promote code reuse from other modules

/// Makes a tangency constraint using external construction line between
/// ellipse => an ellipse
/// geom2 => any of an ellipse, an arc of ellipse, a circle, or an arc (of circle)
/// geoId1 => geoid of the ellipse
/// geoId2 => geoid of geom2
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void makeTangentToEllipseviaNewPoint(Sketcher::SketchObject* Obj,
                                             const Part::GeomEllipse *ellipse,
                                             const Part::Geometry *geom2,
                                             int geoId1,
                                             int geoId2
                                            );
/// Makes a tangency constraint using external construction line between
/// aoe => an arc of ellipse
/// geom2 => any of an arc of ellipse, a circle, or an arc (of circle)
/// geoId1 => geoid of the arc of ellipse
/// geoId2 => geoid of geom2
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void makeTangentToArcOfEllipseviaNewPoint(Sketcher::SketchObject* Obj,
                                             const Part::GeomArcOfEllipse *aoe,
                                             const Part::Geometry *geom2,
                                             int geoId1,
                                             int geoId2
                                            );

/// Makes a tangency constraint using external construction line between
/// aoh => an arc of hyperbola
/// geom2 => any of an arc of hyperbola, an arc of ellipse, a circle, or an arc (of circle)
/// geoId1 => geoid of the arc of hyperbola
/// geoId2 => geoid of geom2
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void makeTangentToArcOfHyperbolaviaNewPoint(Sketcher::SketchObject* Obj,
                                            const Part::GeomArcOfHyperbola *aoh,
                                            const Part::Geometry *geom2,
                                            int geoId1,
                                            int geoId2
                                            );

/// Makes a simple tangency constraint using extra point + tangent via point
/// aop => an arc of parabola
/// geom2 => any of an arc of parabola, an arc of hyperbola an arc of ellipse, a circle, or an arc (of circle)
/// geoId1 => geoid of the arc of parabola
/// geoId2 => geoid of geom2
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void makeTangentToArcOfParabolaviaNewPoint(Sketcher::SketchObject* Obj,
                                            const Part::GeomArcOfParabola *aop,
                                            const Part::Geometry *geom2,
                                            int geoId1,
                                            int geoId2
                                            );

std::string getStrippedPythonExceptionString(const Base::Exception&);

/// This function tries to auto-recompute the active document if the option
/// is set in the user parameter. If the option is not set nothing will be done
/// @return true if a recompute was undertaken, false if not.
bool tryAutoRecompute(Sketcher::SketchObject* obj);
/// Same as the other overload, but also returns whether redundants shall be removed or not
bool tryAutoRecompute(Sketcher::SketchObject* obj, bool &autoremoveredundants);

/// This function tries to auto-recompute as tryAutoRecompute. If tryAutoRecompute
/// is not enabled, then it solves the SketchObject.
void tryAutoRecomputeIfNotSolve(Sketcher::SketchObject* obj);

/// Checks whether there is a constraint of the given type with a First element geoid and a FirstPos PosId
bool checkConstraint(const std::vector< Sketcher::Constraint * > &vals, Sketcher::ConstraintType type, int geoid, Sketcher::PointPos pos);

/// Does an endpoint-to-endpoint tangency
void doEndpointTangency(Sketcher::SketchObject* Obj, int GeoId1, int GeoId2, Sketcher::PointPos PosId1, Sketcher::PointPos PosId2);

/// Does an endpoint-edge tangency
void doEndpointToEdgeTangency( Sketcher::SketchObject* Obj, int GeoId1, Sketcher::PointPos PosId1, int GeoId2);

/// shows constraint substitution information dialog box, enabling the user to forgo further notifications
void notifyConstraintSubstitutions(const QString & message);

}
#endif // SKETCHERGUI_DrawSketchHandler_H

