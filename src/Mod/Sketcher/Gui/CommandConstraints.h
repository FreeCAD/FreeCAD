// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/SketchObject.h>

namespace Gui
{
class Command;
}

namespace SketcherGui
{

// These functions are declared here to promote code reuse from other modules

/// Makes an angle constraint between 2 lines
void makeAngleBetweenTwoLines(Sketcher::SketchObject* Obj, Gui::Command* cmd, int geoId1, int geoId2);

// Find the angle between two lines. Return false if geoIds are not lines.
bool calculateAngle(
    Sketcher::SketchObject* Obj,
    int& geoId1,
    int& geoId2,
    Sketcher::PointPos& PosId1,
    Sketcher::PointPos& PosId2,
    double& ActAngle
);

/// Makes a tangency constraint using external construction line between
/// ellipse => an ellipse
/// geom2 => any of an ellipse, an arc of ellipse, a circle, or an arc (of circle)
/// geoId1 => geoid of the ellipse
/// geoId2 => geoid of geom2
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void makeTangentToEllipseviaNewPoint(
    Sketcher::SketchObject* Obj,
    const Part::GeomEllipse* ellipse,
    const Part::Geometry* geom2,
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
void makeTangentToArcOfEllipseviaNewPoint(
    Sketcher::SketchObject* Obj,
    const Part::GeomArcOfEllipse* aoe,
    const Part::Geometry* geom2,
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
void makeTangentToArcOfHyperbolaviaNewPoint(
    Sketcher::SketchObject* Obj,
    const Part::GeomArcOfHyperbola* aoh,
    const Part::Geometry* geom2,
    int geoId1,
    int geoId2
);

/// Makes a simple tangency constraint using extra point + tangent via point
/// aop => an arc of parabola
/// geom2 => any of an arc of parabola, an arc of hyperbola an arc of ellipse, a circle, or an arc
/// (of circle)
/// geoId1 => geoid of the arc of parabola
/// geoId2 => geoid of geom2
/// NOTE: A command must
/// be opened before calling this function, which this function commits or aborts as appropriate.
/// The reason is for compatibility reasons with other code e.g. "Autoconstraints" in
/// DrawSketchHandler.cpp
void makeTangentToArcOfParabolaviaNewPoint(
    Sketcher::SketchObject* Obj,
    const Part::GeomArcOfParabola* aop,
    const Part::Geometry* geom2,
    int geoId1,
    int geoId2
);

/// Does an endpoint-to-endpoint tangency
void doEndpointTangency(
    Sketcher::SketchObject* Obj,
    int GeoId1,
    int GeoId2,
    Sketcher::PointPos PosId1,
    Sketcher::PointPos PosId2
);

/// Does an endpoint-edge tangency
void doEndpointToEdgeTangency(
    Sketcher::SketchObject* Obj,
    int GeoId1,
    Sketcher::PointPos PosId1,
    int GeoId2
);

/// shows constraint substitution information dialog box, enabling the user to forgo further
/// notifications
void notifyConstraintSubstitutions(const QString& message);

}  // namespace SketcherGui
