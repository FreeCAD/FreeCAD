/***************************************************************************
  tag: Erwin Aertbelien  Mon May 10 19:10:36 CEST 2004  path_cyclic_closed.cxx

                        path_cyclic_closed.cxx -  description
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
 *		$Id: path_cyclic_closed.cpp,v 1.1.1.1.2.5 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/


#include "path_cyclic_closed.hpp"
#include "utilities/error.h"

namespace KDL {

Path_Cyclic_Closed::Path_Cyclic_Closed(Path* _geom,int _times, bool _aggregate):
times(_times),geom(_geom), aggregate(_aggregate) {}

double Path_Cyclic_Closed::LengthToS(double length) {
	throw Error_MotionPlanning_Not_Applicable();
	return 0;
}

double Path_Cyclic_Closed::PathLength(){
	return geom->PathLength()*times;
}

Frame Path_Cyclic_Closed::Pos(double s) const  {
	return geom->Pos( fmod(s,geom->PathLength()) );
}

Twist Path_Cyclic_Closed::Vel(double s,double sd) const  {
	return geom->Vel( fmod(s,geom->PathLength()),sd );
}

Twist Path_Cyclic_Closed::Acc(double s,double sd,double sdd) const  {
	return geom->Acc( fmod(s,geom->PathLength()),sd,sdd );
}


Path_Cyclic_Closed::~Path_Cyclic_Closed() {
    if (aggregate)
        delete geom;
}

Path* Path_Cyclic_Closed::Clone() {
	return new Path_Cyclic_Closed(geom->Clone(),times, aggregate);
}

void Path_Cyclic_Closed::Write(std::ostream& os)  {
	os << "CYCLIC_CLOSED[ ";
	os << "  ";geom->Write(os);os << std::endl;
	os << "  " << times << std::endl;
	os << "]"  << std::endl;
}

}

