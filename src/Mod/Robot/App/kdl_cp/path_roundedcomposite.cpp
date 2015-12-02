/***************************************************************************
  tag: Erwin Aertbelien  Mon May 10 19:10:36 CEST 2004  path_roundedcomposite.cxx

                        path_roundedcomposite.cxx -  description
                           -------------------
    begin                : Mon May 10 2004
    copyright            : (C) 2004 Erwin Aertbelien
    email                : erwin.aertbelien@mech.kuleuven.ac.be

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/
/*****************************************************************************
 *  \author
 *  	Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *
 *  \version
 *		ORO_Geometry V0.2
 *
 *	\par History
 *		- $log$
 *
 *	\par Release
 *		$Id: path_roundedcomposite.cpp,v 1.1.1.1.2.5 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/


#include "path_roundedcomposite.hpp"
#include "path_line.hpp"
#include "path_circle.hpp"
#include "utilities/error.h"
#include <memory>


namespace KDL {

// private constructor, to keep the type when cloning a Path_RoundedComposite, such that getIdentifier keeps on returning
// the correct value:
Path_RoundedComposite::Path_RoundedComposite(Path_Composite* _comp,
		double _radius, double _eqradius, RotationalInterpolation* _orient,
		bool _aggregate,int _nrofpoints):
		comp(_comp), radius(_radius), eqradius(_eqradius), orient(_orient), nrofpoints(_nrofpoints), aggregate(_aggregate) {
}

Path_RoundedComposite::Path_RoundedComposite(double _radius,double _eqradius,RotationalInterpolation* _orient, bool _aggregate) :
	comp( new Path_Composite()), radius(_radius),eqradius(_eqradius), orient(_orient), aggregate(_aggregate)
{
		nrofpoints = 0;
		if (eqradius<=0) {
			throw Error_MotionPlanning_Not_Feasible(1);
		}
}

void Path_RoundedComposite::Add(const Frame& F_base_point) {
	double eps = 1E-7;
	if (nrofpoints == 0) {
		F_base_start = F_base_point;
	} else if (nrofpoints == 1) {
		F_base_via = F_base_point;
	} else {
		// calculate rounded segment : line + circle,
		// determine the angle between the line segments :
		Vector ab = F_base_via.p - F_base_start.p;
		Vector bc = F_base_point.p - F_base_via.p;
		double abdist = ab.Norm();
		double bcdist = bc.Norm();
		if (abdist < eps) {
			throw Error_MotionPlanning_Not_Feasible(2);
		}
		if (bcdist < eps) {
			throw Error_MotionPlanning_Not_Feasible(3);
		}
		double alpha = acos(dot(ab, bc) / abdist / bcdist);
		if ((PI - alpha) < eps) {
			throw Error_MotionPlanning_Not_Feasible(4);
		}
		if (alpha < eps) {
			// no rounding is done in the case of parallel line segments
			comp->Add(
					new Path_Line(F_base_start, F_base_via, orient->Clone(),
							eqradius));
			F_base_start = F_base_via;
			F_base_via = F_base_point;
		} else {
			double d = radius / tan((PI - alpha) / 2); // tan. is guaranteed not to return zero.
			if (d >= abdist)
				throw Error_MotionPlanning_Not_Feasible(5);

			if (d >= bcdist)
				throw Error_MotionPlanning_Not_Feasible(6);

			std::auto_ptr < Path
					> line1(
							new Path_Line(F_base_start, F_base_via,
									orient->Clone(), eqradius));
			std::auto_ptr < Path
					> line2(
							new Path_Line(F_base_via, F_base_point,
									orient->Clone(), eqradius));
			Frame F_base_circlestart = line1->Pos(line1->LengthToS(abdist - d));
			Frame F_base_circleend = line2->Pos(line2->LengthToS(d));
			// end of circle segment, beginning of next line
			Vector V_base_t = ab * (ab * bc);
			V_base_t.Normalize();
			comp->Add(
					new Path_Line(F_base_start, F_base_circlestart,
							orient->Clone(), eqradius));
			comp->Add(
					new Path_Circle(F_base_circlestart,
							F_base_circlestart.p - V_base_t * radius,
							F_base_circleend.p, F_base_circleend.M, alpha,
							orient->Clone(), eqradius));
			// shift for next line
			F_base_start = F_base_circleend; // end of the circle segment
			F_base_via = F_base_point;
		}
	}

	nrofpoints++;
}

void Path_RoundedComposite::Finish() {
	if (nrofpoints >= 1) {
		comp->Add(
				new Path_Line(F_base_start, F_base_via, orient->Clone(),
						eqradius));
	}
}

double Path_RoundedComposite::LengthToS(double length) {
	return comp->LengthToS(length);
}


double Path_RoundedComposite::PathLength() {
	return comp->PathLength();
}

Frame Path_RoundedComposite::Pos(double s) const {
	return comp->Pos(s);
}

Twist Path_RoundedComposite::Vel(double s, double sd) const {
	return comp->Vel(s, sd);
}

Twist Path_RoundedComposite::Acc(double s, double sd, double sdd) const {
	return comp->Acc(s, sd, sdd);
}

void Path_RoundedComposite::Write(std::ostream& os) {
	comp->Write(os);
}

int Path_RoundedComposite::GetNrOfSegments() {
	return comp->GetNrOfSegments();
}

Path* Path_RoundedComposite::GetSegment(int i) {
	return comp->GetSegment(i);
}

double Path_RoundedComposite::GetLengthToEndOfSegment(int i) {
	return comp->GetLengthToEndOfSegment(i);
}

void Path_RoundedComposite::GetCurrentSegmentLocation(double s,
		int& segment_number, double& inner_s) {
	comp->GetCurrentSegmentLocation(s,segment_number,inner_s);
}



Path_RoundedComposite::~Path_RoundedComposite() {
    if (aggregate)
        delete orient;
	delete comp;
}


Path* Path_RoundedComposite::Clone() {
	return new Path_RoundedComposite(static_cast<Path_Composite*>(comp->Clone()),radius,eqradius,orient->Clone(), true, nrofpoints);
}

}
