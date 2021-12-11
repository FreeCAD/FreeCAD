/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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


#ifndef SKETCHERGUI_Recompute_H
#define SKETCHERGUI_Recompute_H

namespace Sketcher {
    enum class PointPos : int;
    class SketchObject;
}

namespace SketcherGui {

/// This function tries to auto-recompute the active document if the option
/// is set in the user parameter. If the option is not set nothing will be done
/// @return true if a recompute was undertaken, false if not.
bool tryAutoRecompute(Sketcher::SketchObject* obj);
/// Same as the other overload, but also returns whether redundants shall be removed or not
bool tryAutoRecompute(Sketcher::SketchObject* obj, bool &autoremoveredundants);

/// This function tries to auto-recompute as tryAutoRecompute. If tryAutoRecompute
/// is not enabled, then it solves the SketchObject.
void tryAutoRecomputeIfNotSolve(Sketcher::SketchObject* obj);

/// Release any currently-active handler for the document.
/// Returns true if a handler was released, and false if not
bool ReleaseHandler(Gui::Document* doc);

std::string getStrippedPythonExceptionString(const Base::Exception&);

void getIdsFromName(const std::string &name, const Sketcher::SketchObject* Obj, int &GeoId, Sketcher::PointPos &PosId);

bool checkBothExternal(int GeoId1, int GeoId2);

bool checkBothExternalOrBSplinePoints(const Sketcher::SketchObject* Obj,int GeoId1, int GeoId2);

bool isPointOrSegmentFixed(const Sketcher::SketchObject* Obj, int GeoId);

bool areBothPointsOrSegmentsFixed(const Sketcher::SketchObject* Obj, int GeoId1, int GeoId2);

bool areAllPointsOrSegmentsFixed(const Sketcher::SketchObject* Obj, int GeoId1, int GeoId2, int GeoId3);

bool inline isVertex(int GeoId, Sketcher::PointPos PosId);

bool inline isEdge(int GeoId, Sketcher::PointPos PosId);

bool isSimpleVertex(const Sketcher::SketchObject* Obj, int GeoId, Sketcher::PointPos PosId);

bool isBsplineKnot(const Sketcher::SketchObject* Obj, int GeoId);

bool IsPointAlreadyOnCurve(int GeoIdCurve, int GeoIdPoint, Sketcher::PointPos PosIdPoint, Sketcher::SketchObject* Obj);

bool isBsplinePole(const Part::Geometry * geo);

bool isBsplinePole(const Sketcher::SketchObject* Obj, int GeoId);

/// Checks whether there is a constraint of the given type with a First element geoid and a FirstPos PosId
bool checkConstraint(const std::vector< Sketcher::Constraint * > &vals, Sketcher::ConstraintType type, int geoid, Sketcher::PointPos pos);

inline bool isVertex(int GeoId, Sketcher::PointPos PosId)
{
    return (GeoId != Sketcher::GeoEnum::GeoUndef && PosId != Sketcher::PointPos::none);
}

inline bool isEdge(int GeoId, Sketcher::PointPos PosId)
{
    return (GeoId != Sketcher::GeoEnum::GeoUndef && PosId == Sketcher::PointPos::none);
}

}
#endif // SKETCHERGUI_Recompute_H

