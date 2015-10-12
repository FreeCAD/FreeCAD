/***************************************************************************
  tag: Erwin Aertbelien  Mon May 10 19:10:36 CEST 2004  path_composite.cxx

                        path_composite.cxx -  description
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
 *		$Id: path_composite.cpp,v 1.1.1.1.2.7 2003/07/24 13:49:16 rwaarsin Exp $
 *		$Name:  $
 ****************************************************************************/


#include "path_composite.hpp"
#include "utilities/error.h"
#include <memory>

namespace KDL {

// s should be in allowable limits, this is not checked
// simple linear search : TODO : make it binary search
// uses cached_... variables
// returns the relative path length within the segment
// you propably want to use the cached_index variable
double Path_Composite::Lookup(double s) const
{
	assert(s>=-1e-12);
	assert(s<=pathlength+1e-12);
	if ( (cached_starts <=s) && ( s <= cached_ends) ) {
		return s - cached_starts;
	}
	double previous_s=0;
	for (unsigned int i=0;i<dv.size();++i) {
		if ((s <= dv[i])||(i == (dv.size()-1) )) {
			cached_index = i;
			cached_starts = previous_s;
			cached_ends   = dv[i];
			return s - previous_s;
		}
		previous_s = dv[i];
	}
	return 0;
}

Path_Composite::Path_Composite() {
	pathlength    = 0;
	cached_starts = 0;
	cached_ends   = 0;
	cached_index  = 0;
}

void Path_Composite::Add(Path* geom, bool aggregate ) {
	pathlength += geom->PathLength();
	dv.insert(dv.end(),pathlength);
	gv.insert( gv.end(),std::make_pair(geom,aggregate) );
}

double Path_Composite::LengthToS(double length) {
	throw Error_MotionPlanning_Not_Applicable();
	return 0;
}

double Path_Composite::PathLength() {
	return pathlength;
}


Frame Path_Composite::Pos(double s) const {
	s = Lookup(s);
	return gv[cached_index].first->Pos(s);
}

Twist Path_Composite::Vel(double s,double sd) const {
	s = Lookup(s);
	return gv[cached_index].first->Vel(s,sd);
}

Twist Path_Composite::Acc(double s,double sd,double sdd) const {
	s = Lookup(s);
	return gv[cached_index].first->Acc(s,sd,sdd);
}

Path* Path_Composite::Clone()  {
	std::auto_ptr<Path_Composite> comp( new Path_Composite() );
	for (unsigned int i = 0; i < dv.size(); ++i) {
		comp->Add(gv[i].first->Clone(), gv[i].second);
	}
	return comp.release();
}

void Path_Composite::Write(std::ostream& os)  {
	os << "COMPOSITE[ " << std::endl;
	os << "   " << dv.size() << std::endl;
	for (unsigned int i=0;i<dv.size();i++) {
		gv[i].first->Write(os);
	}
	os << "]" << std::endl;
}

int Path_Composite::GetNrOfSegments() {
	return dv.size();
}

Path* Path_Composite::GetSegment(int i) {
	assert(i>=0);
	assert(i<dv.size());
	return gv[i].first;
}

double Path_Composite::GetLengthToEndOfSegment(int i) {
	assert(i>=0);
	assert(i<dv.size());
	return dv[i];
}

void Path_Composite::GetCurrentSegmentLocation(double s, int& segment_number,
		double& inner_s)
{
	inner_s = Lookup(s);
	segment_number= cached_index;
}

Path_Composite::~Path_Composite() {
	PathVector::iterator it;
	for (it=gv.begin();it!=gv.end();++it) {
		if (it->second)
            delete it->first;
	}
}

} // namespace KDL
